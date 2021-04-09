#include "url.h"
#include "error.h"
#include <esp_system.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_spiffs.h"
#include <sys/stat.h>
#include <sys/unistd.h>

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "URL";

static char blacklist[MAX_BLACKLIST_SIZE];      // Blacklist buffer

static int url_in_blacklist;                    // Number of urls in blacklist
static int blacklist_size;                      // Bytes of blacklist buffer being used
static SemaphoreHandle_t blacklist_mutex;       // Lock for buffer


char* get_blacklist(int* size)
{
    xSemaphoreTake(blacklist_mutex, portMAX_DELAY);
    *size = url_in_blacklist; 
    return blacklist;
}

void return_blacklist()
{
    xSemaphoreGive(blacklist_mutex);
}

// Load blacklist from Flash into RAM
static esp_err_t reload_blacklist()
{
    // Open blacklist file
    FILE* f = NULL;
    f = fopen("/spiffs/blacklist", "r");
    if ( !f )
        return URL_ERR_LIST_UNAVAILBLE;

    xSemaphoreTake(blacklist_mutex, portMAX_DELAY);
    url_in_blacklist = 0;

    // Read through file, saving into buffer
    URL url = {0};
    blacklist_size = 0;
    while(fread(&url.length, sizeof(url.length), 1, f) == sizeof(url.length))
    {
        fread(&url.string, url.length, 1, f);
        memcpy(blacklist+blacklist_size, &url, url.length+sizeof(url.length));
        blacklist_size += url.length+sizeof(url.length);
        url_in_blacklist++;
    }
    xSemaphoreGive(blacklist_mutex);
    ESP_LOGI(TAG, "Blacklist size: %d bytes", blacklist_size);
    return ESP_OK;
}

esp_err_t add_to_blacklist(URL url)
{
    // Check if list if full
    if ( MAX_BLACKLIST_SIZE-blacklist_size <= url.length+1 )
        return URL_ERR_LIST_FUll;

    // Check url validity
    if ( !valid_url(url) )
        return URL_ERR_INVALID_URL;

    // Make sure url isn't already in blacklist
    if ( in_blacklist(url) )
        return URL_ERR_ALREADY_EXISTS;

    // Save to blacklist file
    FILE* f = fopen("/spiffs/blacklist", "a");
    if( !f )
        return URL_ERR_LIST_UNAVAILBLE;

    fwrite(&url, url.length+sizeof(url.length), 1, f);
    fclose(f);

    // Load blacklist file into RAM
    reload_blacklist();
    return ESP_OK;
}

esp_err_t remove_from_blacklist(URL removal)
{
    // Open blacklist, as well as a temporary file
    FILE* f = fopen("/spiffs/blacklist", "r");
    FILE* tmp = fopen("/spiffs/tmplist", "w");

    if ( !f || !tmp)
        return URL_ERR_LIST_UNAVAILBLE;

    // Copy every url in blacklist to tmplist, except url to be removed
    bool found_url = false;
    URL url = {0};
    while(fread(&url.length, sizeof(url.length), 1, f) == sizeof(url.length))
    {
        fread(&url.string, url.length, 1, f);
        ESP_LOGD(TAG, "Read %*s(%d) from list.txt", url.length, url.string, url.length);
        if(memcmp(&url, &removal, sizeof(url.length)+url.length) == 0)
        {
            ESP_LOGI(TAG, "Removing %*s(%d) from list.txt", url.length, url.string, url.length);
            found_url = true;
        }
        else
        {
            ESP_LOGD(TAG, "Writing %*s(%d) to list.txt", url.length, url.string, url.length);
            fwrite(&url, sizeof(url.length)+url.length, 1, tmp);
        }

    }

    fclose(tmp);
    fclose(f);
    unlink("/spiffs/blacklist");                    // Delete old blacklist
    rename("/spiffs/tmplist", "/spiffs/blacklist"); // Rename updated list to blacklist

    // Load new blacklist into RAM
    reload_blacklist(); 
    
    if(found_url)
        return ESP_OK;
    else
        return URL_ERR_NOT_FOUND;
}

esp_err_t build_blacklist_json(cJSON* json)
{
    ESP_LOGI(TAG, "%d URLs in blacklist", url_in_blacklist);

    // Initialize json object with empty array
    cJSON* blacklist_array = cJSON_CreateArray();
    if ( json == NULL || blacklist_array == NULL )
    {
        return ESP_FAIL;
    }
    cJSON_AddItemToObject(json, "blacklist", blacklist_array);

    // Add urls in blacklist to json object
    xSemaphoreTake(blacklist_mutex, portMAX_DELAY);
    int array_index = 0;
    for(int i = 0; i < url_in_blacklist; i++)
    {
        // Create url struct
        URL url = {0};
        url.length = blacklist[array_index];
        memcpy(url.string, blacklist+array_index+sizeof(url.length), url.length);

        cJSON* str = cJSON_CreateString(url.string);
        cJSON_AddItemToArray(blacklist_array, str);

        array_index += url.length+sizeof(url.length);
    }
    xSemaphoreGive(blacklist_mutex);

    return ESP_OK;
}

esp_err_t store_default_blacklists()
{
    ESP_LOGI(TAG, "Saving Blacklist");
    FILE* b = fopen("/spiffs/blacklist", "w");
    if(b == NULL)
    {
        ESP_LOGE(TAG, "Error opening blacklist file");
        return ESP_FAIL;
    }

    extern const unsigned char blacklist_txt_start[] asm("_binary_default_blacklist_txt_start");
    extern const unsigned char blacklist_txt_end[]   asm("_binary_default_blacklist_txt_end");
    const size_t blacklist_txt_size = (blacklist_txt_end - blacklist_txt_start);

    fwrite(blacklist_txt_start, blacklist_txt_size, 1, b);
    fclose(b);

    return ESP_OK;
}

esp_err_t initialize_blocklists()
{
    blacklist_mutex = xSemaphoreCreateMutex();

    return reload_blacklist();
}

bool valid_url(URL url)
{
    for(int i = 0; i < url.length; i++)
    {
        if( (url.string[i] < '0' || url.string[i] > '9') &&
            (url.string[i] < 'A' || url.string[i] > 'Z') &&
            (url.string[i] < 'a' || url.string[i] > 'z') &&
            (url.string[i] != '.' && url.string[i] != '-' && 
             url.string[i] != '*' && url.string[i] != '?'))
        {
            return false;
        }
    }
    return true;
}

// This was shamlessly stolen off the internet 
// Only works with \0 terminated strings
static IRAM_ATTR bool wildcmp(char *pattern, char *string)
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
    ESP_LOGD(TAG, "Checking Blacklist for %*s", url.length, url.string);
    xSemaphoreTake(blacklist_mutex, portMAX_DELAY);

    bool match = false;
    int array_index = 0;
    while(match == false && array_index < blacklist_size)
    {
        URL comparison = {0};
        comparison.length = blacklist[array_index];
        memcpy(comparison.string, blacklist+array_index+sizeof(comparison.length), comparison.length);
        ESP_LOGV(TAG, "Comparing %*s and %*s", url.length, url.string, comparison.length, comparison.string);
        match = wildcmp(comparison.string, url.string); // \0 initialized array means strings are already \0 terminated
        array_index += comparison.length + sizeof(comparison.length);
    }
    xSemaphoreGive(blacklist_mutex);

    return match;
}

IRAM_ATTR URL convert_qname_to_url(char* qname_ptr)
{
    // QNAME is a sequence of labels, where each label consists of a length octet followed by that number of octets. 
    // The domain name terminates with the zero length octet for the null label of the root. 
    URL url = {0};
    char* url_ptr = url.string;

    uint8_t label_length = (uint8_t)*qname_ptr;
    while(label_length != 0)
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