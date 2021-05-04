#include "url.h"
#include "error.h"
#include "events.h"
#include "filesystem.h"
#include <esp_system.h>
#include <string.h>

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
static const char *TAG = "URL";


bool valid_url(const char* url)
{
    for(int i = 0; i < strlen(url); i++)
    {
        if( (url[i] < '0' || url[i] > '9') &&
            (url[i] < 'A' || url[i] > 'Z') &&
            (url[i] < 'a' || url[i] > 'z') &&
            (url[i] != '.' && url[i] != '-' && 
             url[i] != '*' && url[i] != '?'))
        {
            return false;
        }
    }
    return true;
}

// This was shamlessly stolen off the internet 
// Only works with \0 terminated strings
static IRAM_ATTR bool wildcmp(const char *pattern, const char *string)
{
    if(*pattern=='\0' && *string=='\0')
        return true;

    if (*pattern == '*' && *(pattern+1) != '\0' && *string == '\0') 
        return false; 
		
    if(*pattern=='?' || *pattern==*string)
        return wildcmp(pattern+1,string+1);
		
    if(*pattern=='*')
        return wildcmp(pattern+1,string) || wildcmp(pattern,string+1);

    return false;
}

IRAM_ATTR bool in_blacklist(URL url)
{
    ESP_LOGD(TAG, "Checking Blacklist for %.*s", url.length, url.string);
    // xSemaphoreTake(blacklist_mutex, portMAX_DELAY);
    int64_t start = esp_timer_get_time();

    bool inBlacklist = false;
    try{
        using namespace fs;
        file blacklist = open("/blacklist.txt", "r");

        char hostname[255];
        while( fgets(hostname, 255, blacklist.handle) != NULL )
        {
            hostname[strcspn(hostname, "\r")] = 0;
            hostname[strcspn(hostname, "\n")] = 0;
            if( wildcmp(hostname, url.string) )
            {
                inBlacklist = true;
                break;
            }
        }
    }catch(std::string e){
        inBlacklist = false;
    }

    int64_t end = esp_timer_get_time();
    ESP_LOGI(TAG, "Processing Time: %lld ms", (end-start)/1000);

    return inBlacklist;
}

IRAM_ATTR bool in_blacklist(const char* domain)
{
    ESP_LOGD(TAG, "Checking Blacklist for %s", domain);
    int64_t start = esp_timer_get_time();

    bool inBlacklist = false;
    try{
        using namespace fs;
        file blacklist = open("/blacklist.txt", "r");

        char entry[255];
        while( fgets(entry, 255, blacklist.handle) != NULL )
        {
            entry[strcspn(entry, "\r")] = 0;
            entry[strcspn(entry, "\n")] = 0;
            if( wildcmp(entry, domain) )
            {
                inBlacklist = true;
                break;
            }
        }
    }catch(const Err& e){
        inBlacklist = false;
    }

    int64_t end = esp_timer_get_time();
    ESP_LOGI(TAG, "Processing Time: %lld ms", (end-start)/1000);

    return inBlacklist;
}

esp_err_t add_to_blacklist(const char* hostname)
{
    // Check url validity
    if ( !valid_url(hostname) )
        return URL_ERR_INVALID_URL;

    try{
        using namespace fs;
        file blacklist = open("/blacklist.txt", "a");

        fputs(hostname, blacklist.handle);
        fputc('\n', blacklist.handle);
    }catch(std::string e){
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

static bool remove_hostname(const char* hostname)
{
    using namespace fs;
    file blacklist = open("/blacklist.txt", "r");
    file tmp = open("/tmplist", "w");

    // Copy every url in blacklist to tmplist, except url to be removed
    bool found_url = false;
    char buffer[MAX_URL_LENGTH+1];
    while( fgets(buffer, MAX_URL_LENGTH, blacklist.handle) )
    {
        strtok(buffer, "\r\n");
        ESP_LOGD(TAG, "Read %s from blacklist.txt", buffer);
        if( strcmp( buffer, hostname) != 0 )
        {
            fputs(buffer, tmp.handle);
            fputc('\n', tmp.handle);
        }
        else
        {
            ESP_LOGD(TAG, "Found match");
            found_url = true;
        }
    }

    return found_url;
}

esp_err_t remove_from_blacklist(const char* hostname)
{
    // Check url validity
    if ( !valid_url(hostname) )
        return URL_ERR_INVALID_URL;

    bool found_url = false;
    try{
        found_url = remove_hostname(hostname);
        fs::unlink("/blacklist.txt");
        fs::rename("/tmplist", "/blacklist.txt");
    } catch(std::string e) {
        return ESP_FAIL;
    }

    if(found_url)
        return ESP_OK;
    else
        return URL_ERR_NOT_FOUND;
}