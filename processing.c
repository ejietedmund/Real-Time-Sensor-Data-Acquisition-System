/* =========================================================================
 * processing.c  –  Moving average, anomaly detection, trend prediction
 * ========================================================================= */

#include <string.h>
#include <math.h>
#include "processing.h"

/* ── Anomaly configuration table ─────────────────────────────────────────── */
/* Indexed by SENSOR_TEMP, SENSOR_HUMIDITY, etc. */
AnomalyConfig anomaly_cfg[NUM_SENSORS] = {
    /* SENSOR_TEMP     */ { TEMP_LOW_THRESH,     TEMP_HIGH_THRESH,     TEMP_SPIKE_THRESH     },
    /* SENSOR_HUMIDITY */ { HUMIDITY_LOW_THRESH, HUMIDITY_HIGH_THRESH, HUMIDITY_SPIKE_THRESH },
    /* SENSOR_SOIL     */ { SOIL_LOW_THRESH,     SOIL_HIGH_THRESH,     SOIL_SPIKE_THRESH     },
    /* SENSOR_LIGHT    */ { LIGHT_LOW_THRESH,    LIGHT_HIGH_THRESH,    LIGHT_SPIKE_THRESH    },
    /* SENSOR_CO2      */ { CO2_LOW_THRESH,      CO2_HIGH_THRESH,      CO2_SPIKE_THRESH      },
};

/* ── Moving Average ──────────────────────────────────────────────────────── */

void mavg_init(MovingAvg *m)
{
    memset(m->window, 0, sizeof(m->window));
    m->running_sum = 0.0f;
    m->pos         = 0;
    m->count       = 0;
}

/* O(1) update:
 *   new_sum = old_sum - outgoing_value + incoming_value
 * No loop needed; constant time regardless of window size. */
fp16_t mavg_update(MovingAvg *m, fp16_t sample)
{
    float fs = fp_to_float(sample);

    /* Subtract the slot we're about to overwrite, then overwrite it */
    m->running_sum += fs - m->window[m->pos];
    m->window[m->pos] = fs;

    /* Advance ring position */
    m->pos = (m->pos + 1) % WINDOW_SIZE;
    if (m->count < WINDOW_SIZE) m->count++;

    return float_to_fp(m->running_sum / (float)m->count);
}

/* ── Anomaly Detection ───────────────────────────────────────────────────── */

/* O(1): three comparisons, no loops. */
AnomalyType detect_anomaly(uint8_t sensor_id, fp16_t current, fp16_t previous)
{
    AnomalyConfig *cfg = &anomaly_cfg[sensor_id];

    /* Convert FP thresholds for comparison */
    fp16_t lo    = float_to_fp((float)cfg->low_thresh);
    fp16_t hi    = float_to_fp((float)cfg->high_thresh);
    fp16_t spike = float_to_fp((float)cfg->spike_thresh);

    if (current < lo) return ANOMALY_LOW;
    if (current > hi) return ANOMALY_HIGH;

    /* Derivative check: |current - previous| > spike threshold */
    fp16_t delta = (fp16_t)(current > previous ? current - previous
                                               : previous - current);
    if (delta > spike) return ANOMALY_SPIKE;

    return ANOMALY_NONE;
}

/* ── Trend Prediction: Linear Regression ────────────────────────────────── */

/* O(n) pass through the circular buffer computing:
 *   Σx, Σy, Σx², Σxy   (sufficient statistics for least-squares fit)
 *
 * Dynamic programming connection:
 *   Each statistic is built by accumulating one term at a time.
 *   The final result depends on all sub-problems (all prior samples),
 *   which is the essence of DP: solve from sub-problems upward.
 *
 * slope     = (n·Σxy  −  Σx·Σy)   / (n·Σx²  −  (Σx)²)
 * intercept = (Σy  −  slope·Σx)   / n
 * predicted = slope * n + intercept   (value at index n = "next tick")
 * R²        = 1 − SS_res / SS_tot    (goodness of fit)
 */
TrendResult compute_trend(const CircularBuffer *cb)
{
    TrendResult r = {0.0f, 0.0f, 0.0f, 0.0f};
    uint8_t n = cb->count;
    if (n < 2) return r;

    float sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (uint8_t i = 0; i < n; i++) {
        float x = (float)i;
        float y = fp_to_float(cb_get(cb, i));
        sx  += x;
        sy  += y;
        sxx += x * x;
        sxy += x * y;
    }

    float denom = (float)n * sxx - sx * sx;
    if (fabsf(denom) < 1e-6f) return r;   /* vertical line / degenerate case */

    r.slope     = ((float)n * sxy - sx * sy) / denom;
    r.intercept = (sy - r.slope * sx) / (float)n;
    r.predicted = r.slope * (float)n + r.intercept;

    /* R² calculation */
    float y_mean  = sy / (float)n;
    float ss_tot  = 0, ss_res = 0;
    for (uint8_t i = 0; i < n; i++) {
        float y     = fp_to_float(cb_get(cb, i));
        float y_hat = r.slope * (float)i + r.intercept;
        ss_res += (y - y_hat) * (y - y_hat);
        ss_tot += (y - y_mean) * (y - y_mean);
    }
    r.r_squared = (ss_tot > 1e-6f) ? 1.0f - ss_res / ss_tot : 1.0f;
    if (r.r_squared < 0.0f) r.r_squared = 0.0f;

    return r;
}
