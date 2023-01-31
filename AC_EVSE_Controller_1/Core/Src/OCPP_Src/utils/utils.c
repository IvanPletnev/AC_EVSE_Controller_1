/*
 * utils.c
 *
 *  Created on: Dec 23, 2022
 *      Author: Admin
 */


#include "utils.h"
#include "main.h"
#include <ctype.h>
#include "cmsis_os.h"

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

taskENTER_CRITICAL();
memcpy(q->buffer + q->tail, data,         part1);
memcpy(q->buffer,           data + part1, part2);
taskEXIT_CRITICAL();
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

char * strcasestr(const char *s, const char *find){
	char c, sc;
	size_t len;

	if ((c = *find++) != 0) {
		c = (char)tolower((unsigned char)c);
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == 0)
					return (NULL);
			} while ((char)tolower((unsigned char)sc) != c);
		} while (strncasecmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}
