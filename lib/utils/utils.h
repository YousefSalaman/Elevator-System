#ifndef ELEVATOR_UTILS_H
#define ELEVATOR_UTILS_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// Utility functions

uint32_t bin_to_int(uint8_t * num, int size);
size_t cobs_decode(const uint8_t * input, size_t length, uint8_t * output);
size_t cobs_encode(const uint8_t * input, size_t length, uint8_t * output);


#ifdef __cplusplus
}
#endif

#endif