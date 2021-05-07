#ifndef DATETIME_H
#define DATETIME_H

#include <esp_system.h>
#include "sys/time.h"
#include <string>

/**
  * @brief Convert time_t scruct into string
  *
  * @param time any time_t struct
  *
  * @return pointer to static buffer strftime_buf[64]
  */
std::string get_time_str(time_t time);

/**
  * @brief Initialize sntp.
  * 
  * Sends out SNTP requests to configured server for current time
  * Method blocks until it receives a time 
  *
  * @param None
  */
void initialize_sntp();

#endif