/* =========================================================================
 * serial_monitor.c  –  Serial monitor / console output
 * ========================================================================= */

#include <stdio.h>
#include <string.h>
#include "serial_monitor.h"

/* ── ANSI colour codes (ignored on terminals that don't support them) ────── */
#define COL_RESET   "\033[0m"
#define COL_RED     "\033[31m"
#define COL_YELLOW  "\033[33m"
#define COL_GREEN   "\033[32m"
#define COL_CYAN    "\033[36m"
#define COL_BOLD    "\033[1m"
#define COL_DIM     "\033[2m"
#define COL_MAGENTA "\033[35m"

static const char *prio_colour(uint8_t prio)
{
    if (prio == PRIORITY_CRITICAL) return COL_RED;
    if (prio == PRIORITY_WARNING)  return COL_YELLOW;
    return COL_GREEN;
}

static const char *prio_label(uint8_t prio)
{
    if (prio == PRIORITY_CRITICAL) return "CRITICAL";
    if (prio == PRIORITY_WARNING)  return "WARNING ";
    return "INFO    ";
}

/* ── Banner ──────────────────────────────────────────────────────────────── */
void monitor_print_banner(void)
{
    printf(COL_BOLD COL_CYAN);
    printf("=================================================================\n");
    printf("  REAL-TIME SENSOR DATA ACQUISITION SYSTEM\n");
    printf("  CPE2206 - Group Project 1  |  Week 7 Final System\n");
    printf("  Greenhouse Multi-Sensor Monitor  v2.0\n");
    printf("=================================================================\n");
    printf(COL_RESET);
    printf(COL_DIM);
    printf("  Sensors  : Temperature | Humidity | Soil Moisture | Light | CO2\n");
    printf("  Buffers  : Circular ring buffers (power-of-2, no malloc)\n");
    printf("  Alerts   : Priority queue (min-heap, O(log n))\n");
    printf("  Analytics: Sliding avg O(1) | Linear regression O(n)\n");
    printf("  Fixed-pt : Q8.8 storage (2 bytes/sample vs 4 for float)\n");
    printf(COL_RESET "\n");
}

/* ── Memory report ───────────────────────────────────────────────────────── */
void monitor_print_memory_report(void)
{
    /* Sizes computed from the actual struct/array sizes in the system */
    size_t buf_temp  = TEMP_BUF_SIZE     * sizeof(fp16_t);
    size_t buf_hum   = HUMIDITY_BUF_SIZE * sizeof(fp16_t);
    size_t buf_soil  = SOIL_BUF_SIZE     * sizeof(fp16_t);
    size_t buf_light = LIGHT_BUF_SIZE    * sizeof(fp16_t);
    size_t buf_co2   = CO2_BUF_SIZE      * sizeof(fp16_t);
    size_t cb_structs= NUM_SENSORS       * sizeof(CircularBuffer);
    size_t mavg      = NUM_SENSORS       * sizeof(MovingAvg);
    size_t pq        = sizeof(PriorityQueue);
    size_t alog      = sizeof(AlertLog);
    size_t total     = buf_temp + buf_hum + buf_soil + buf_light + buf_co2
                     + cb_structs + mavg + pq + alog;

    printf(COL_BOLD "[ MEMORY FOOTPRINT ]\n" COL_RESET);
    printf("  Temperature buffer  (%2d slots x 2 B) :  %3zu bytes\n", TEMP_BUF_SIZE,     buf_temp);
    printf("  Humidity buffer     (%2d slots x 2 B) :  %3zu bytes\n", HUMIDITY_BUF_SIZE, buf_hum);
    printf("  Soil buffer         (%2d slots x 2 B) :  %3zu bytes\n", SOIL_BUF_SIZE,     buf_soil);
    printf("  Light buffer        (%2d slots x 2 B) :  %3zu bytes\n", LIGHT_BUF_SIZE,    buf_light);
    printf("  CO2 buffer          (%2d slots x 2 B) :  %3zu bytes\n", CO2_BUF_SIZE,      buf_co2);
    printf("  CircularBuffer structs (x%d)          :  %3zu bytes\n", NUM_SENSORS,       cb_structs);
    printf("  MovingAvg structs   (x%d)             :  %3zu bytes\n", NUM_SENSORS,       mavg);
    printf("  PriorityQueue (heap, %d events)       :  %3zu bytes\n", MAX_EVENTS,        pq);
    printf("  AlertLog (%d entries)                :  %3zu bytes\n", ALERT_LOG_SIZE,    alog);
    printf("  -------------------------------------------------------\n");
    printf("  " COL_BOLD "Total data segment : %4zu bytes" COL_RESET "\n", total);
    printf("  64 KB RAM limit  : 65536 bytes\n");
    printf("  " COL_GREEN "Headroom remaining : %4zu bytes  (%.1f%% used)" COL_RESET "\n\n",
           65536 - total, (double)total / 65536.0 * 100.0);
}

/* ── Tick header ─────────────────────────────────────────────────────────── */
void monitor_print_tick_header(uint32_t tick)
{
    printf(COL_BOLD COL_CYAN "--- TICK %04u -------------------------------------------------------\n" COL_RESET, tick);
}

/* ── Sensor data line ────────────────────────────────────────────────────── */
void monitor_print_sensor_line(uint8_t sensor_id,
                                fp16_t raw, fp16_t avg,
                                const TrendResult *trend,
                                AnomalyType atype)
{
    const SensorMeta *m = &sensor_meta[sensor_id];
    const char *trend_arrow = "->";
    if (trend->slope >  0.1f)  trend_arrow = "UP";
    if (trend->slope >  1.0f)  trend_arrow = "UP UP";
    if (trend->slope < -0.1f)  trend_arrow = "DOWN";
    if (trend->slope < -1.0f)  trend_arrow = "DOWN DOWN";

    printf("  %-12s | raw=%7.2f %-4s | avg=%7.2f | next~=%7.2f | slope=%+.3f %s | R^2=%.2f",
           m->name,
           fp_to_float(raw),  m->unit,
           fp_to_float(avg),
           trend->predicted,
           trend->slope,  trend_arrow,
           trend->r_squared);

    if (atype != ANOMALY_NONE) {
        const char *label;
        const char *col;
        switch (atype) {
            case ANOMALY_LOW:   label = " [!] LOW  "; col = COL_RED;    break;
            case ANOMALY_HIGH:  label = " [!] HIGH "; col = COL_RED;    break;
            case ANOMALY_SPIKE: label = " [*] SPIKE"; col = COL_YELLOW; break;
            default:            label = " [?]";        col = COL_RESET;
        }
        printf(" %s%s%s", col, label, COL_RESET);
    }
    printf("\n");
}

/* ── Alert line (from priority queue) ───────────────────────────────────────*/
void monitor_print_alert(const Event *e)
{
    printf("  %s%s[%s]%s  t=%-4u | %-12s | %s %-6.2f %s\n",
           COL_BOLD,
           prio_colour(e->priority),
           prio_label(e->priority),
           COL_RESET,
           e->timestamp,
           e->sensor_name,
           e->type,
           fp_to_float(e->value),
           e->unit);
}

/* ── Per-tick performance footer ─────────────────────────────────────────── */
void monitor_print_tick_footer(const PerfMetrics *pm)
{
    printf(COL_DIM "  - perf: tick_time=%.1f us | samples=%u | throughput=%.1f sps | avg_latency=%.1f us\n" COL_RESET,
           pm->tick_duration_us,
           pm->samples_this_tick,
           pm->throughput_sps,
           pm->avg_latency_us);
}

/* ── Final summary ───────────────────────────────────────────────────────── */
void monitor_print_final_summary(const AlertLog *log, const PerfMetrics *pm)
{
    printf("\n");
    printf(COL_BOLD COL_CYAN "=================================================================\n");
    printf("  SIMULATION COMPLETE - SUMMARY\n");
    printf("=================================================================\n" COL_RESET);
    printf("\n");

    /* Performance metrics */
    printf(COL_BOLD "  [PERFORMANCE METRICS]\n" COL_RESET);
    printf("    Total ticks processed : %u\n", pm->tick);
    printf("    Total samples acquired: %llu\n", (unsigned long long)pm->total_samples);
    printf("    Average tick latency  : %.2f us\n", pm->avg_latency_us);
    printf("    Average throughput    : %.2f samples/sec\n", pm->throughput_sps);
    printf("    Peak latency observed : %.2f us\n", pm->tick_duration_us);
    printf("\n");

    /* Alert statistics */
    printf(COL_BOLD "  [ALERT STATISTICS]\n" COL_RESET);
    printf("    Total alerts raised   : %u\n", log->total_alerts);
    printf("    " COL_RED "Critical alerts      : %u" COL_RESET "\n", log->critical_count);
    printf("    " COL_YELLOW "Warning alerts       : %u" COL_RESET "\n", log->warning_count);
    printf("    Info alerts           : %u\n",
           log->total_alerts - log->critical_count - log->warning_count);
    printf("\n");
}

/* ── Full alert log dump ─────────────────────────────────────────────────── */
void monitor_print_alert_log(const AlertLog *log)
{
    if (log->total_alerts == 0) {
        printf("  No alerts were raised during this run.\n");
        return;
    }

    printf(COL_BOLD "  [FULL ALERT LOG]\n" COL_RESET);
    uint32_t count = log->total_alerts < ALERT_LOG_SIZE
                   ? log->total_alerts : ALERT_LOG_SIZE;

    /* Print from oldest to newest */
    uint8_t start = (log->total_alerts >= ALERT_LOG_SIZE)
                  ? log->head   /* head is oldest when log is full */
                  : 0;

    for (uint32_t i = 0; i < count; i++) {
        uint8_t idx = (start + i) % ALERT_LOG_SIZE;
        const Event *e = &log->entries[idx];
        printf("    %s[%s]%s  t=%-4u  %-12s  %s %.2f %s\n",
               prio_colour(e->priority),
               prio_label(e->priority),
               COL_RESET,
               e->timestamp,
               e->sensor_name,
               e->type,
               fp_to_float(e->value),
               e->unit);
    }
}
