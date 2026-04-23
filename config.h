#ifndef CONFIG_H
#define CONFIG_H

/* =========================================================================
 * config.h  –  System-wide configuration for the Sensor Acquisition System
 * CPE2206 Group Project 1 | Week 7
 *
 * Change values here to tune the system without touching any logic code.
 * ========================================================================= */

/* ── Sensors ─────────────────────────────────────────────────────────────── */
#define NUM_SENSORS          5       /* Total number of sensors in system     */

#define SENSOR_TEMP          0       /* Temperature (°C)                      */
#define SENSOR_HUMIDITY      1       /* Relative humidity (%RH)               */
#define SENSOR_SOIL          2       /* Soil moisture (%VWC)                  */
#define SENSOR_LIGHT         3       /* Light intensity (lux, 0-1000)         */
#define SENSOR_CO2           4       /* CO2 concentration (ppm)               */

/* ── Circular Buffer Sizes (MUST be powers of 2) ─────────────────────────── */
#define TEMP_BUF_SIZE        16      /* 16 × 2 B = 32 B                       */
#define HUMIDITY_BUF_SIZE    8       /*  8 × 2 B = 16 B                       */
#define SOIL_BUF_SIZE        8       /*  8 × 2 B = 16 B                       */
#define LIGHT_BUF_SIZE       8       /*  8 × 2 B = 16 B                       */
#define CO2_BUF_SIZE         16      /* 16 × 2 B = 32 B  (needs longer trend) */

/* ── Moving Average ───────────────────────────────────────────────────────── */
#define WINDOW_SIZE          5       /* Samples used in sliding average        */

/* ── Priority Queue ──────────────────────────────────────────────────────── */
#define MAX_EVENTS           16      /* Max simultaneous queued alerts         */

/* ── Alert Log ───────────────────────────────────────────────────────────── */
#define ALERT_LOG_SIZE       64      /* Persistent log of past alerts          */

/* ── Sampling Rates (every N ticks each sensor is sampled) ──────────────── */
/* Set to 1 to sample every tick, 2 for every other tick, etc.               */
#define TEMP_SAMPLE_RATE     1
#define HUMIDITY_SAMPLE_RATE 1
#define SOIL_SAMPLE_RATE     2       /* Soil changes slowly – sample less often*/
#define LIGHT_SAMPLE_RATE    1
#define CO2_SAMPLE_RATE      2

/* ── Fixed-Point Arithmetic Q8.8 ─────────────────────────────────────────── */
#define FP_SCALE             256     /* 2^8: multiply float by this to encode  */

/* ── Anomaly Thresholds ──────────────────────────────────────────────────── */
/* Values are in real-world units (converted to FP internally in sensors.h)  */
#define TEMP_LOW_THRESH      10      /* °C – below this = cold alert           */
#define TEMP_HIGH_THRESH     40      /* °C – above this = heat alert           */
#define HUMIDITY_LOW_THRESH  20      /* %RH                                    */
#define HUMIDITY_HIGH_THRESH 90      /* %RH                                    */
#define SOIL_LOW_THRESH      15      /* %VWC – below = dry soil alert          */
#define SOIL_HIGH_THRESH     85      /* %VWC                                   */
#define LIGHT_LOW_THRESH     50      /* lux – too dark for plants              */
#define LIGHT_HIGH_THRESH    900     /* lux – too bright / sensor fault        */
#define CO2_LOW_THRESH       300     /* ppm – unusually low                    */
#define CO2_HIGH_THRESH      1500    /* ppm – dangerously high                 */

/* Derivative spike thresholds (change per tick, same units as above)        */
#define TEMP_SPIKE_THRESH    5       /* °C/tick                                */
#define HUMIDITY_SPIKE_THRESH 10     /* %RH/tick                               */
#define SOIL_SPIKE_THRESH    10      /* %VWC/tick                              */
#define LIGHT_SPIKE_THRESH   200     /* lux/tick                               */
#define CO2_SPIKE_THRESH     200     /* ppm/tick                               */

/* ── Simulation Settings ──────────────────────────────────────────────────── */
#define SIM_TOTAL_TICKS      0        /* 0 = run forever (Ctrl+C to stop)       */
#define SIM_TICK_DELAY_MS    500     /* Milliseconds between ticks (0 = max)   */

/* ── Priority Levels ─────────────────────────────────────────────────────── */
#define PRIORITY_CRITICAL    1
#define PRIORITY_WARNING     2
#define PRIORITY_INFO        3

#endif /* CONFIG_H */
