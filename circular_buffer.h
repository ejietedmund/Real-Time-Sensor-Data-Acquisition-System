#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

/* =========================================================================
 * circular_buffer.h  –  Fixed-size ring buffer for sensor samples
 *
 * Key design choices:
 *   - Backing storage is caller-provided (no malloc)
 *   - Capacity must be a power of 2 → wraparound uses bitwise AND (fast)
 *   - Overflow policy: silently discard oldest sample (never blocks)
 * ========================================================================= */

#include <stdint.h>
#include <string.h>
#include "config.h"

/* Q8.8 fixed-point type: stores values with 8 fractional bits.
 * Range ≈ -128.00 to +127.99, resolution ≈ 0.004 units.
 * Uses int16_t (2 bytes) instead of float (4 bytes) → halves buffer RAM. */
typedef int16_t fp16_t;

static inline fp16_t float_to_fp(float v) { return (fp16_t)(v * FP_SCALE); }
static inline float  fp_to_float(fp16_t v){ return (float)v  / FP_SCALE;   }

/* ── Struct ──────────────────────────────────────────────────────────────── */
typedef struct {
    fp16_t  *data;      /* pointer to caller-provided backing array           */
    uint8_t  head;      /* next write slot                                    */
    uint8_t  tail;      /* oldest unread slot                                 */
    uint8_t  count;     /* current number of valid samples                    */
    uint8_t  capacity;  /* total slots (must be power of 2)                   */
    uint32_t total_writes;   /* performance counter: samples written ever     */
    uint32_t overflow_count; /* performance counter: overwrites due to full   */
} CircularBuffer;

/* ── API ─────────────────────────────────────────────────────────────────── */
void    cb_init        (CircularBuffer *cb, fp16_t *storage, uint8_t capacity);
void    cb_write       (CircularBuffer *cb, fp16_t value);
fp16_t  cb_read        (CircularBuffer *cb);
fp16_t  cb_peek_latest (const CircularBuffer *cb);
fp16_t  cb_get         (const CircularBuffer *cb, uint8_t index);
int     cb_is_full     (const CircularBuffer *cb);
int     cb_is_empty    (const CircularBuffer *cb);

#endif /* CIRCULAR_BUFFER_H */
