/* =========================================================================
 * main.c  –  Real-Time Sensor Data Acquisition System
 *            CPE2206 Group Project 1 | Week 7 Final Integration
 *
 * System flow (executed once per tick):
 *  1. For each sensor (if its sampling rate is due):
 *       a. Read sensor value (simulated; replace with ADC on ESP32)
 *       b. Write value into its circular buffer
 *       c. Update O(1) moving average
 *       d. Detect anomalies (threshold + spike check)
 *       e. If anomaly: push Event onto priority queue & log it
 *       f. Compute linear regression trend over buffer history
 *       g. Print sensor line to serial monitor
 *  2. Drain priority queue → print alerts in urgency order
 *  3. Record performance metrics for the completed tick
 *  4. Sleep SIM_TICK_DELAY_MS milliseconds (simulates 1 Hz hardware timer)
 *
 * After SIM_TOTAL_TICKS ticks (or Ctrl+C if 0):
 *  - Print performance summary, alert statistics, and full alert log
 * ========================================================================= */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Platform-specific sleep / timing */
#ifdef _WIN32
  #include <windows.h>
  #define sleep_ms(ms)   Sleep(ms)
  #include <time.h>
  static double get_time_us(void) {
      LARGE_INTEGER freq, cnt;
      QueryPerformanceFrequency(&freq);
      QueryPerformanceCounter(&cnt);
      return (double)cnt.QuadPart / (double)freq.QuadPart * 1e6;
  }
#else
  #include <unistd.h>
  #include <time.h>
  #define sleep_ms(ms)   usleep((ms)*1000)
  static double get_time_us(void) {
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      return (double)ts.tv_sec * 1e6 + (double)ts.tv_nsec / 1e3;
  }
#endif

#include "config.h"
#include "circular_buffer.h"
#include "priority_queue.h"
#include "processing.h"
#include "sensors.h"
#include "serial_monitor.h"

/* =========================================================================
 * Global system state  (all statically allocated – no malloc anywhere)
 * ========================================================================= */

/* Backing storage arrays for each sensor's circular buffer */
static fp16_t store_temp [TEMP_BUF_SIZE];
static fp16_t store_hum  [HUMIDITY_BUF_SIZE];
static fp16_t store_soil [SOIL_BUF_SIZE];
static fp16_t store_light[LIGHT_BUF_SIZE];
static fp16_t store_co2  [CO2_BUF_SIZE];

static fp16_t *sensor_storage[NUM_SENSORS] = {
    store_temp, store_hum, store_soil, store_light, store_co2
};

static CircularBuffer sensor_buf [NUM_SENSORS];
static MovingAvg      sensor_mavg[NUM_SENSORS];
static PriorityQueue  event_queue;
static AlertLog       alert_log;

static uint32_t global_tick = 0;

/* Performance tracking */
static PerfMetrics perf;
static double      sum_latency_us  = 0.0;
static double      peak_latency_us = 0.0;

/* =========================================================================
 * system_init  –  One-time setup
 * ========================================================================= */
static void system_init(void)
{
    sensors_reset();

    for (int i = 0; i < NUM_SENSORS; i++) {
        cb_init(&sensor_buf[i], sensor_storage[i], sensor_meta[i].buf_size);
        mavg_init(&sensor_mavg[i]);
    }

    pq_init(&event_queue);
    alert_log_init(&alert_log);
    memset(&perf, 0, sizeof(perf));

    monitor_print_banner();
    monitor_print_memory_report();
    printf("  Starting acquisition loop  (ticks=%d, delay=%d ms)\n\n",
           SIM_TOTAL_TICKS, SIM_TICK_DELAY_MS);
}

/* =========================================================================
 * process_tick  –  One complete sensor acquisition cycle
 * ========================================================================= */
static void process_tick(void)
{
    global_tick++;
    double tick_start = get_time_us();

    monitor_print_tick_header(global_tick);

    perf.samples_this_tick = 0;
    perf.alerts_this_tick  = 0;

    /* ── Step 1: Process each sensor ─────────────────────────────────── */
    for (uint8_t sid = 0; sid < NUM_SENSORS; sid++) {

        /* Sampling rate check: skip if not due this tick */
        if ((global_tick % sensor_meta[sid].sample_rate) != 0) continue;

        /* a. Read sensor */
        fp16_t raw  = simulate_sensor(sid, global_tick);

        /* b. Remember previous reading (before writing) for spike check */
        fp16_t prev = cb_peek_latest(&sensor_buf[sid]);

        /* c. Write to circular buffer */
        cb_write(&sensor_buf[sid], raw);
        perf.samples_this_tick++;
        perf.total_samples++;

        /* d. Update moving average – O(1) */
        fp16_t avg = mavg_update(&sensor_mavg[sid], raw);

        /* e. Anomaly detection – O(1) */
        AnomalyType atype = ANOMALY_NONE;
        if (sensor_buf[sid].count > 1)   /* skip first tick: no valid prev */
            atype = detect_anomaly(sid, raw, prev);

        /* f. Push alert if anomaly detected */
        if (atype != ANOMALY_NONE) {
            const char *type_str;
            uint8_t     prio;
            switch (atype) {
                case ANOMALY_LOW:   type_str = "LOW";   prio = PRIORITY_CRITICAL; break;
                case ANOMALY_HIGH:  type_str = "HIGH";  prio = PRIORITY_CRITICAL; break;
                case ANOMALY_SPIKE: type_str = "SPIKE"; prio = PRIORITY_WARNING;  break;
                default:            type_str = "???";   prio = PRIORITY_INFO;
            }

            Event ev;
            ev.priority  = prio;
            ev.sensor_id = sid;
            ev.value     = raw;
            ev.timestamp = global_tick;
            strncpy(ev.type,        type_str,               sizeof(ev.type)        - 1);
            strncpy(ev.sensor_name, sensor_meta[sid].name,  sizeof(ev.sensor_name) - 1);
            strncpy(ev.unit,        sensor_meta[sid].unit,  sizeof(ev.unit)        - 1);
            ev.type[sizeof(ev.type)-1]               = '\0';
            ev.sensor_name[sizeof(ev.sensor_name)-1] = '\0';
            ev.unit[sizeof(ev.unit)-1]               = '\0';

            if (pq_push(&event_queue, ev) < 0) {
                printf("  [WARNING] Priority queue full – event dropped!\n");
            }
            alert_log_add(&alert_log, &ev);
            perf.alerts_this_tick++;
        }

        /* g. Compute trend (linear regression) – O(n) */
        TrendResult trend = compute_trend(&sensor_buf[sid]);

        /* h. Print line to serial monitor */
        monitor_print_sensor_line(sid, raw, avg, &trend, atype);
    }

    /* ── Step 2: Drain priority queue → print alerts by urgency ──────── */
    if (event_queue.size > 0) {
        printf("  ========================================== ALERTS ==================\n");
        Event ev;
        while (pq_pop(&event_queue, &ev) == 0) {
            monitor_print_alert(&ev);
        }
    }

    /* ── Step 3: Tick performance ─────────────────────────────────────── */
    double tick_end = get_time_us();
    double duration = tick_end - tick_start;
    sum_latency_us += duration;
    if (duration > peak_latency_us) peak_latency_us = duration;

    perf.tick             = global_tick;
    perf.tick_duration_us = duration;
    perf.avg_latency_us   = sum_latency_us / (double)global_tick;
    /* throughput = total samples / total elapsed time in seconds */
    double elapsed_s      = sum_latency_us / 1e6;
    perf.throughput_sps   = elapsed_s > 0.0
                          ? (double)perf.total_samples / elapsed_s
                          : 0.0;

    monitor_print_tick_footer(&perf);
}

/* =========================================================================
 * main  –  Entry point
 * ========================================================================= */
int main(void)
{
    system_init();

    uint32_t ticks = (SIM_TOTAL_TICKS == 0) ? 0xFFFFFFFF : SIM_TOTAL_TICKS;

    for (uint32_t t = 0; t < ticks; t++) {
        process_tick();

#if SIM_TICK_DELAY_MS > 0
        sleep_ms(SIM_TICK_DELAY_MS);
#endif
    }

    /* Update peak latency in final perf record */
    perf.tick_duration_us = peak_latency_us;

    monitor_print_final_summary(&alert_log, &perf);
    monitor_print_alert_log(&alert_log);

    printf("\n  System halted. Total ticks: %u\n\n", global_tick);
    return 0;
}
