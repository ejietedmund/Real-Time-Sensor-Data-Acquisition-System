#ifndef SERIAL_MONITOR_H
#define SERIAL_MONITOR_H

/* =========================================================================
 * serial_monitor.h  –  Serial monitor output (mimics embedded UART console)
 *
 * On a real ESP32 this maps directly to printf() over USB-UART.
 * The display functions provide formatted, human-readable output that looks
 * identical to what you would see in the Arduino IDE Serial Monitor or
 * PlatformIO terminal.
 * ========================================================================= */

#include <stdint.h>
#include "config.h"
#include "circular_buffer.h"
#include "priority_queue.h"
#include "processing.h"
#include "sensors.h"

/* Performance metrics collected each tick */
typedef struct {
    uint32_t tick;
    double   tick_duration_us;   /* microseconds to process one full tick    */
    uint32_t samples_this_tick;  /* sensor samples acquired this tick        */
    uint32_t alerts_this_tick;
    uint64_t total_samples;      /* all-time sample count                    */
    double   throughput_sps;     /* samples per second (rolling)             */
    double   avg_latency_us;     /* rolling average tick duration            */
} PerfMetrics;

/* ── API ─────────────────────────────────────────────────────────────────── */
void monitor_print_banner       (void);
void monitor_print_memory_report(void);
void monitor_print_tick_header  (uint32_t tick);
void monitor_print_sensor_line  (uint8_t sensor_id,
                                 fp16_t raw, fp16_t avg,
                                 const TrendResult *trend,
                                 AnomalyType atype);
void monitor_print_alert        (const Event *e);
void monitor_print_tick_footer  (const PerfMetrics *pm);
void monitor_print_final_summary(const AlertLog *log, const PerfMetrics *pm);
void monitor_print_alert_log    (const AlertLog *log);

#endif /* SERIAL_MONITOR_H */
