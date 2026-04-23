#ifndef SENSORS_H
#define SENSORS_H

/* =========================================================================
 * sensors.h  –  Sensor simulation (replace with real ADC reads on ESP32)
 *
 * Each sensor has a name, unit, sampling rate, and realistic noise model.
 * On real hardware, simulate_sensor() is replaced by a HAL read call.
 * ========================================================================= */

#include <stdint.h>
#include "config.h"
#include "circular_buffer.h"

typedef struct {
    const char *name;
    const char *unit;
    uint8_t     sample_rate;   /* sample every N ticks                       */
    uint8_t     buf_size;      /* circular buffer capacity                   */
} SensorMeta;

/* Sensor metadata table, indexed by SENSOR_* constants */
extern const SensorMeta sensor_meta[NUM_SENSORS];

/* Returns the simulated sensor reading as a Q8.8 fixed-point value.
 * On ESP32: replace body with  return float_to_fp(adc_read_sensor(id));  */
fp16_t simulate_sensor(uint8_t sensor_id, uint32_t tick);

/* Reset simulation state (call once at system_init) */
void sensors_reset(void);

#endif /* SENSORS_H */
