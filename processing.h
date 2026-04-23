#ifndef PROCESSING_H
#define PROCESSING_H

/* =========================================================================
 * processing.h  –  Signal processing algorithms
 *
 * Covers:
 *   MovingAvg  –  O(1) sliding-window average using a running sum
 *   detect_anomaly  –  threshold + derivative spike check, O(1)
 *   compute_trend   –  linear regression over buffer history, O(n)
 * ========================================================================= */

#include <stdint.h>
#include "config.h"
#include "circular_buffer.h"

/* ── Moving Average ──────────────────────────────────────────────────────── */
typedef struct {
    float   window[WINDOW_SIZE]; /* ring of last WINDOW_SIZE values (float)  */
    float   running_sum;         /* maintained incrementally                 */
    uint8_t pos;                 /* next write position in window ring       */
    uint8_t count;               /* samples seen, capped at WINDOW_SIZE      */
} MovingAvg;

void   mavg_init   (MovingAvg *m);
fp16_t mavg_update (MovingAvg *m, fp16_t sample); /* O(1) update            */

/* ── Anomaly Detection ───────────────────────────────────────────────────── */
typedef enum {
    ANOMALY_NONE  = 0,
    ANOMALY_LOW,      /* value below low threshold                           */
    ANOMALY_HIGH,     /* value above high threshold                          */
    ANOMALY_SPIKE     /* value changed too fast between consecutive samples  */
} AnomalyType;

typedef struct {
    int    low_thresh;      /* real-world unit (e.g. 10 for 10°C)            */
    int    high_thresh;
    int    spike_thresh;    /* max acceptable delta between ticks             */
} AnomalyConfig;

/* One AnomalyConfig per sensor, indexed by SENSOR_* constants */
extern AnomalyConfig anomaly_cfg[NUM_SENSORS];

AnomalyType detect_anomaly(uint8_t sensor_id, fp16_t current, fp16_t previous);

/* ── Trend Prediction (Linear Regression) ───────────────────────────────── */
typedef struct {
    float slope;        /* rate of change per sample (units/tick)            */
    float intercept;
    float predicted;    /* estimated value at next tick                      */
    float r_squared;    /* goodness of fit [0-1], 1 = perfect line           */
} TrendResult;

/* Compute least-squares linear regression over all samples in the buffer.
 * Time complexity: O(n) where n = cb->count (≤ buffer capacity).
 * The DP insight: Σ-statistics are built up sample-by-sample. */
TrendResult compute_trend(const CircularBuffer *cb);

#endif /* PROCESSING_H */
