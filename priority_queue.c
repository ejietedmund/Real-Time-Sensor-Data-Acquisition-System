/* =========================================================================
 * priority_queue.c  –  Min-heap priority queue and alert log
 * ========================================================================= */

#include <string.h>
#include "priority_queue.h"

/* ── Internal helpers ────────────────────────────────────────────────────── */

static void swap(PriorityQueue *pq, uint8_t i, uint8_t j)
{
    Event tmp   = pq->heap[i];
    pq->heap[i] = pq->heap[j];
    pq->heap[j] = tmp;
}

/* After inserting at [idx], bubble up until heap property restored.
 * Parent of node i is at (i-1)/2. */
static void sift_up(PriorityQueue *pq, uint8_t idx)
{
    while (idx > 0) {
        uint8_t parent = (idx - 1) / 2;
        if (pq->heap[parent].priority > pq->heap[idx].priority) {
            swap(pq, parent, idx);
            idx = parent;
        } else {
            break;
        }
    }
}

/* After removing root, place last element at [0] and bubble down.
 * Children of node i are at 2i+1 and 2i+2. */
static void sift_down(PriorityQueue *pq, uint8_t idx)
{
    while (1) {
        uint8_t smallest = idx;
        uint8_t left     = 2 * idx + 1;
        uint8_t right    = 2 * idx + 2;

        if (left  < pq->size && pq->heap[left].priority  < pq->heap[smallest].priority)
            smallest = left;
        if (right < pq->size && pq->heap[right].priority < pq->heap[smallest].priority)
            smallest = right;

        if (smallest == idx) break;
        swap(pq, idx, smallest);
        idx = smallest;
    }
}

/* ── Public API ──────────────────────────────────────────────────────────── */

void pq_init(PriorityQueue *pq)
{
    pq->size = 0;
}

/* Insert event. Returns 0 on success, -1 if queue is full. */
int pq_push(PriorityQueue *pq, Event e)
{
    if (pq->size >= MAX_EVENTS) return -1;
    pq->heap[pq->size] = e;
    sift_up(pq, pq->size);
    pq->size++;
    return 0;
}

/* Extract the highest-priority (lowest number) event.
 * Returns 0 on success, -1 if queue is empty. */
int pq_pop(PriorityQueue *pq, Event *out)
{
    if (pq->size == 0) return -1;
    *out = pq->heap[0];
    pq->heap[0] = pq->heap[--pq->size];
    if (pq->size > 0) sift_down(pq, 0);
    return 0;
}

/* Peek at highest-priority event without removing it. */
int pq_peek(const PriorityQueue *pq, Event *out)
{
    if (pq->size == 0) return -1;
    *out = pq->heap[0];
    return 0;
}

/* ── Alert Log ───────────────────────────────────────────────────────────── */

void alert_log_init(AlertLog *log)
{
    memset(log, 0, sizeof(AlertLog));
}

/* Append an event to the circular alert log (overwrites oldest if full). */
void alert_log_add(AlertLog *log, const Event *e)
{
    log->entries[log->head] = *e;
    log->head = (log->head + 1) % ALERT_LOG_SIZE;
    log->total_alerts++;
    if (e->priority == PRIORITY_CRITICAL) log->critical_count++;
    if (e->priority == PRIORITY_WARNING)  log->warning_count++;
}
