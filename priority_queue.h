#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

/* =========================================================================
 * priority_queue.h  –  Min-heap priority queue for sensor alerts
 *
 * Key design choices:
 *   - Array-based binary min-heap (no pointers, no malloc)
 *   - Lower priority number = higher urgency (1 = CRITICAL floats to top)
 *   - O(log n) insert and extract, where n ≤ MAX_EVENTS (= 16)
 * ========================================================================= */

#include <stdint.h>
#include "config.h"
#include "circular_buffer.h"   /* for fp16_t */

/* ── Event record stored in the heap ────────────────────────────────────── */
typedef struct {
    uint8_t  priority;          /* 1=CRITICAL, 2=WARNING, 3=INFO             */
    uint8_t  sensor_id;         /* which sensor raised the event             */
    fp16_t   value;             /* raw sensor value at time of event         */
    char     type[8];           /* "LOW", "HIGH", "SPIKE"                    */
    char     sensor_name[16];   /* human-readable sensor name                */
    char     unit[6];           /* "°C", "%RH", etc.                         */
    uint32_t timestamp;         /* system tick when event was raised         */
} Event;

/* ── Persistent alert log ────────────────────────────────────────────────── */
typedef struct {
    Event    entries[ALERT_LOG_SIZE];
    uint8_t  head;              /* next write position (ring log)            */
    uint32_t total_alerts;      /* all-time alert count                      */
    uint32_t critical_count;
    uint32_t warning_count;
} AlertLog;

/* ── Priority queue (min-heap) ───────────────────────────────────────────── */
typedef struct {
    Event   heap[MAX_EVENTS];
    uint8_t size;
} PriorityQueue;

/* ── API ─────────────────────────────────────────────────────────────────── */
void pq_init  (PriorityQueue *pq);
int  pq_push  (PriorityQueue *pq, Event e);   /* returns -1 if full        */
int  pq_pop   (PriorityQueue *pq, Event *out);/* returns -1 if empty       */
int  pq_peek  (const PriorityQueue *pq, Event *out);

void alert_log_init(AlertLog *log);
void alert_log_add (AlertLog *log, const Event *e);

#endif /* PRIORITY_QUEUE_H */
