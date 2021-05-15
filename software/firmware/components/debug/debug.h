#ifndef DEBUG_H
#define DEBUG_H

#include "esp_system.h"

/**
  * @brief Start stack watching task to log stack usage 
  *
  */
void init_stack_watcher();

#endif