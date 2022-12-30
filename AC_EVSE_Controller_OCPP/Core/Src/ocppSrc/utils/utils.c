/*
 * utils.c
 *
 *  Created on: Dec 23, 2022
 *      Author: Admin
 */


#include "utils.h"
#include "main.h"

queue_t ringBuffer = {
		.buffer_size = RINGBUFFER_SIZE
};

static inline __off_t min(__off_t a, __off_t b) {
    return a < b ? a : b;
}

bool put(queue_t *q, const uint8_t *data, size_t size) {
    if(q->buffer_size - q->bytes_avail < size){
        return false;
    }
const size_t part1 = min(size, q->buffer_size - q->tail);
const size_t part2 = size - part1;

memcpy(q->buffer + q->tail, data,         part1);
memcpy(q->buffer,           data + part1, part2);

q->tail = (q->tail + size) % q->buffer_size;
q->bytes_avail += size;

return true;

}

uint8_t ringBufferInit (queue_t * queue) {

	queue->bytes_avail = 0;
	queue->head = 0;
	queue->tail = 0;

	return true;
}
