/* =========================================================================
 * circular_buffer.c  –  Ring buffer implementation
 * ========================================================================= */

#include "circular_buffer.h"

void cb_init(CircularBuffer *cb, fp16_t *storage, uint8_t capacity)
{
    cb->data           = storage;
    cb->head           = 0;
    cb->tail           = 0;
    cb->count          = 0;
    cb->capacity       = capacity;
    cb->total_writes   = 0;
    cb->overflow_count = 0;
    memset(storage, 0, capacity * sizeof(fp16_t));
}

/* Write one sample.
 * If full, advance tail to drop the oldest value (overflow policy). */
void cb_write(CircularBuffer *cb, fp16_t value)
{
    cb->data[cb->head] = value;
    /* Bitwise AND replaces modulo – valid because capacity is power of 2 */
    cb->head = (cb->head + 1) & (cb->capacity - 1);

    if (cb->count == cb->capacity) {
        /* Buffer full: discard oldest by moving tail forward */
        cb->tail = (cb->tail + 1) & (cb->capacity - 1);
        cb->overflow_count++;
    } else {
        cb->count++;
    }
    cb->total_writes++;
}

/* Read (consume) the oldest sample. Returns 0 if empty. */
fp16_t cb_read(CircularBuffer *cb)
{
    if (cb->count == 0) return 0;
    fp16_t value = cb->data[cb->tail];
    cb->tail  = (cb->tail + 1) & (cb->capacity - 1);
    cb->count--;
    return value;
}

/* Peek at the most recently written sample without removing it. */
fp16_t cb_peek_latest(const CircularBuffer *cb)
{
    if (cb->count == 0) return 0;
    uint8_t idx = (cb->head - 1) & (cb->capacity - 1);
    return cb->data[idx];
}

/* Non-destructive indexed access. index 0 = oldest, count-1 = newest. */
fp16_t cb_get(const CircularBuffer *cb, uint8_t index)
{
    uint8_t idx = (cb->tail + index) & (cb->capacity - 1);
    return cb->data[idx];
}

int cb_is_full (const CircularBuffer *cb) { return cb->count == cb->capacity; }
int cb_is_empty(const CircularBuffer *cb) { return cb->count == 0; }
