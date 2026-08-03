/*
 * sample.h
 *
 *  Created on: Aug 3, 2026
 *      Author: WalterWhite
 */

#ifndef SAMPLE_H
#define SAMPLE_H

#include <stdint.h>

typedef enum {
  SRC_GYRO = 0,
  SRC_SYSTEM,
  SRC_COUNT
} sample_src_t;

typedef struct {
  uint32_t     timestamp_ms;
  uint32_t     seq;
  sample_src_t src;

  union {
    struct {
      int16_t x;
      int16_t y;
      int16_t z;
    } gyro;

    struct {
      uint32_t uptime_s;
      uint16_t free_heap_kb;
    } sys;
  } data;

} sample_t;

#endif /* SAMPLE_H */
