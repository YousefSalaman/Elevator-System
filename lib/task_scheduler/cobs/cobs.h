#ifndef COBS_H
#define COBS_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


size_t cobs_decode(const uint8_t * input, size_t length, uint8_t * output);
size_t cobs_encode(const uint8_t * input, size_t length, uint8_t * output);


#ifdef __cplusplus
}
#endif

#endif


