/* =========================================================================
 * sensors.c  –  Sensor simulation with realistic noise and anomaly injection
 * ========================================================================= */

#include <math.h>
#include "sensors.h"

/* ── Sensor metadata ─────────────────────────────────────────────────────── */
const SensorMeta sensor_meta[NUM_SENSORS] = {
    /* name           unit    rate  buf_size */
    { "Temperature",  "C",    TEMP_SAMPLE_RATE,     TEMP_BUF_SIZE     },
    { "Humidity",     "%RH",  HUMIDITY_SAMPLE_RATE, HUMIDITY_BUF_SIZE },
    { "SoilMoisture", "%VWC", SOIL_SAMPLE_RATE,     SOIL_BUF_SIZE     },
    { "Light",        "lux",  LIGHT_SAMPLE_RATE,    LIGHT_BUF_SIZE    },
    { "CO2",          "ppm",  CO2_SAMPLE_RATE,      CO2_BUF_SIZE      },
};

/* ── Simulation state ────────────────────────────────────────────────────── */
static uint32_t lcg_state = 12345;  /* LCG random seed                      */
static float sim_temp  = 25.0f;
static float sim_hum   = 60.0f;
static float sim_soil  = 45.0f;
static float sim_light = 400.0f;
static float sim_co2   = 600.0f;

void sensors_reset(void)
{
    lcg_state = 12345;
    sim_temp  = 25.0f;
    sim_hum   = 60.0f;
    sim_soil  = 45.0f;
    sim_light = 400.0f;
    sim_co2   = 600.0f;
}

/* ── Simple LCG pseudo-random noise in range [-amplitude, +amplitude] ───── */
static float rand_noise(float amplitude)
{
    lcg_state = lcg_state * 1664525u + 1013904223u;
    /* Map uint32 to [-1, 1] */
    float r = (float)(int32_t)lcg_state / (float)0x7FFFFFFF;
    return r * amplitude;
}

static float clamp(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* ── Sensor simulation ───────────────────────────────────────────────────── */
/*
 * Anomaly schedule (designed to demonstrate all detection types):
 *
 *   Tick 10  – Temperature drops to 7°C   (LOW threshold breach)
 *   Tick 11  – Temperature recovers        (SPIKE on recovery)
 *   Tick 25  – Soil moisture drops to 8%   (LOW threshold breach)
 *   Tick 26  – Soil moisture recovers      (SPIKE on recovery)
 *   Tick 35  – CO2 spikes to 1600 ppm      (HIGH threshold breach)
 *   Tick 36  – CO2 recovers                (SPIKE on recovery)
 *   Tick 45  – Humidity drops to 15%       (LOW threshold breach)
 *   Tick 46  – Humidity recovers           (SPIKE on recovery)
 *   Tick 55  – Light exceeds 950 lux       (HIGH threshold breach)
 */
fp16_t simulate_sensor(uint8_t sensor_id, uint32_t tick)
{
    float val;

    switch (sensor_id)
    {
        case SENSOR_TEMP:
            sim_temp += rand_noise(0.4f);
            if (tick == 10) sim_temp = 7.0f;
            if (tick == 11) sim_temp = 25.0f;
            sim_temp = clamp(sim_temp, -10.0f, 60.0f);
            val = sim_temp;
            break;

        case SENSOR_HUMIDITY:
            sim_hum += rand_noise(0.8f);
            if (tick == 45) sim_hum = 15.0f;
            if (tick == 46) sim_hum = 60.0f;
            sim_hum = clamp(sim_hum, 5.0f, 99.0f);
            val = sim_hum;
            break;

        case SENSOR_SOIL:
            sim_soil += rand_noise(0.5f);
            if (tick == 25) sim_soil = 8.0f;
            if (tick == 26) sim_soil = 45.0f;
            sim_soil = clamp(sim_soil, 0.0f, 100.0f);
            val = sim_soil;
            break;

        case SENSOR_LIGHT:
            sim_light += rand_noise(10.0f);
            if (tick == 55) sim_light = 960.0f;
            sim_light = clamp(sim_light, 0.0f, 1000.0f);
            val = sim_light;
            break;

        case SENSOR_CO2:
            sim_co2 += rand_noise(5.0f);
            if (tick == 35) sim_co2 = 1600.0f;
            if (tick == 36) sim_co2 = 620.0f;
            sim_co2 = clamp(sim_co2, 200.0f, 5000.0f);
            val = sim_co2;
            break;

        default:
            val = 0.0f;
    }

    return float_to_fp(val);
}
