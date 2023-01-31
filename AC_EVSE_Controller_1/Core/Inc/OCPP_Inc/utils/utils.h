/*
 * utils.h
 *
 *  Created on: Dec 23, 2022
 *      Author: Admin
 */

#ifndef INC_OCPPINC_UTILS_UTILS_H_
#define INC_OCPPINC_UTILS_UTILS_H_

#include <stdbool.h>
#include <string.h>
#include "main.h"


#define RINGBUFFER_SIZE 1024

typedef struct {
    uint8_t  buffer [1024];
    const size_t   buffer_size;
    size_t   head;
    size_t   tail;
    size_t   bytes_avail;
} queue_t;

extern queue_t ringBuffer;

bool put (queue_t *q, const uint8_t *data, size_t size);

uint8_t ringBufferInit (queue_t * queue);

char * strcasestr(const char *s, const char *find);


#endif /* INC_OCPPINC_UTILS_UTILS_H_ */
