#ifndef __KEY_H
#define __KEY_H

#include <stdint.h>

typedef enum
{
  KEY_PB0 = 0,
  KEY_PB1 = 1,
  KEY_PB10 = 2,
  KEY_COUNT
} key_id_t;

#define KEY_DEBOUNCE_MS (20U)

void key_init(void);
uint8_t key_scan_event(key_id_t key);

#endif
