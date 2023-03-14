#ifndef __FIFO_H_
#define __FIFO_H_

#include <inttypes.h>
#include <stddef.h>

typedef struct {
    uint8_t *buffer;
    uint16_t head;
    uint16_t tail;
    uint16_t size;
    uint16_t free;
} fifo_t;

//#define CHECK_FIFO_NULL(fifo) MAC_FUNC(if (fifo == NULL) return 0;)
fifo_t *create_fifo(const size_t size);
void free_fifo(fifo_t * fifo);
uint8_t fifo_clear (fifo_t * fifo, uint16_t size);
size_t fifo_push_byte(fifo_t * fifo, const uint8_t byte);
size_t fifo_push(fifo_t * fifo, uint8_t * buff, const size_t count);
size_t fifo_pop_byte(fifo_t * fifo, uint8_t * byte);
uint8_t FIFO_POP_BYTE (fifo_t * fifo);
size_t fifo_pop(fifo_t * fifo, uint8_t * buff, const size_t count);
uint8_t fifo_check_full(fifo_t * fifo);
uint8_t fifo_check_empty(fifo_t * fifo);
size_t fifo_check_fill(fifo_t * fifo);

#endif
