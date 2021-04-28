#include "url.h"
#include "error.h"
#include "events.h"
#include "filesystem.h"
#include <esp_system.h>
#include <string.h>

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
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

IRAM_ATTR URL convert_qname_to_url(char* qname_ptr)
{
    // QNAME is a sequence of labels, where each label consists of a length octet followed by that number of octets. 
    // The domain name terminates with the zero length octet for the null label of the root. 
    URL url = {0};
    char* url_ptr = url.string;

    uint8_t label_length = (uint8_t)*qname_ptr;
    while( label_length != 0 )
    {
        memcpy(url_ptr, qname_ptr+1, label_length);
        *(url_ptr+label_length) = '.';
        url_ptr += label_length + 1;
        qname_ptr += label_length + 1;
        url.length += label_length + 1;
        label_length = (uint8_t)*qname_ptr;
    }
    *(url_ptr-1) = '\0'; // remove last '.'
    url.length--;
    
    return url;
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
    FILE* blacklist = open_file("/app/blacklist.txt", "r");
    if( blacklist == NULL ){
        ESP_LOGE(TAG, "Could not open blacklist");
        return false;
    }

    char hostname[255];
    while( fgets(hostname, 255, blacklist) != NULL )
    {
        hostname[strcspn(hostname, "\r")] = 0;
        hostname[strcspn(hostname, "\n")] = 0;
        if( wildcmp(hostname, url.string) )
        {
            inBlacklist = true;
            break;
        }
    }
    fclose(blacklist);

    int64_t end = esp_timer_get_time();
    ESP_LOGI(TAG, "Processing Time: %lld ms", (end-start)/1000);

    return inBlacklist;
}

esp_err_t add_to_blacklist(const char* hostname)
{
    // Check url validity
    if ( !valid_url(hostname) )
        return URL_ERR_INVALID_URL;

    FILE* blacklist = open_file("/app/blacklist.txt", "a");
    if( blacklist == NULL ){
        ESP_LOGE(TAG, "Could not open blacklist");
        return ESP_FAIL;
    }

    fputs(hostname, blacklist);
    fputc('\n', blacklist);
    fclose(blacklist);

    return ESP_OK;
}

esp_err_t remove_from_blacklist(const char* hostname)
{
    // Check url validity
    if ( !valid_url(hostname) )
        return URL_ERR_INVALID_URL;

    // Open blacklist, as well as a temporary file
    FILE* f = open_file("/app/blacklist.txt", "r");
    FILE* tmp = open_file("/app/tmplist", "w");

    if ( !f || !tmp)
        return URL_ERR_LIST_UNAVAILBLE;

    // Copy every url in blacklist to tmplist, except url to be removed
    bool found_url = false;
    char buffer[MAX_URL_LENGTH+1];
    while( fgets(buffer, MAX_URL_LENGTH, f) )
    {
        strtok(buffer, "\r\n");
        // buffer[strlen(buffer)-1] = '\0'; // remove '\n'
        ESP_LOGD(TAG, "Read %s from blacklist.txt", buffer);
        if( strcmp( buffer, hostname) != 0 )
        {
            fputs(buffer, tmp);
            fputc('\n', tmp);
        }
        else
        {
            ESP_LOGD(TAG, "Found match");
            found_url = true;
        }
    }

    fclose(tmp);
    fclose(f);
    delete_file("/app/blacklist.txt");
    rename_file("/app/tmplist", "/app/blacklist.txt");
    
    if(found_url)
        return ESP_OK;
    else
        return URL_ERR_NOT_FOUND;
}