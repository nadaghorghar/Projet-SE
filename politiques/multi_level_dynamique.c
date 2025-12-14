#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "process.h"
#include "gui.h"
#include "console_display.h"

#define MAX_PRIO 6
#define FAMINE_THRESHOLD 6

typedef struct {
    int items[MAXP];
    int size;
} PrioQueue;

typedef struct {
    PrioQueue queues[MAX_PRIO];
    int time;
    int *wait_time;
    int *first_run;
    int *end_time;
    int *timeline;
    int timeline_len;
    int running;
    int quantum_used;
    int quantum_size;
} State;
static int get_quantum(int prio) {
    if (prio >= 4) return 1;
    if (prio >= 2) return 2;
    return 4;
}

static void q_init(PrioQueue *q) { 
    q->size = 0; 
}

static void q_push(PrioQueue *q, int idx) {
    if (q->size < MAXP) q->items[q->size++] = idx;
}

static int q_pop(PrioQueue *q) {
    if (q->size == 0) return -1;
    int v = q->items[0];
    for (int i = 0; i < q->size - 1; i++)
        q->items[i] = q->items[i + 1];
    q->size--;
    return v;
}

static int q_contains(PrioQueue *q, int idx) {
    for (int i = 0; i < q->size; i++)
        if (q->items[i] == idx) return 1;
    return 0;
}

static void q_remove(PrioQueue *q, int idx) {
    for (int i = 0; i < q->size; i++) {
        if (q->items[i] == idx) {
            for (int j = i; j < q->size - 1; j++)
                q->items[j] = q->items[j + 1];
            q->size--;
            return;
        }
    }
}

static int in_any_queue(State *s, int idx) {
    for (int p = 0; p < MAX_PRIO; p++)
        if (q_contains(&s->queues[p], idx)) return 1;
    return 0;
}

static int highest_prio_waiting(State *s) {
    for (int p = MAX_PRIO - 1; p >= 0; p--)
        if (s->queues[p].size > 0) return p;
    return -1;
}

static int select_next(State *s) {
    for (int p = MAX_PRIO - 1; p >= 0; p--)
        if (s->queues[p].size > 0)
            return q_pop(&s->queues[p]);
    return -1;
}


static void init_state(State *s, int n) {
    for (int i = 0; i < MAX_PRIO; i++) 
        q_init(&s->queues[i]);

    s->time = 0;
    s->running = -1;
    s->quantum_used = 0;
    s->quantum_size = 0;
    s->timeline_len = 0;

    s->wait_time = calloc(n, sizeof(int));
    s->first_run = malloc(n * sizeof(int));
    s->end_time = malloc(n * sizeof(int));
    s->timeline = malloc(2000 * sizeof(int));

    for (int i = 0; i < n; i++) {
        s->first_run[i] = -1;
        s->end_time[i] = -1;
    }
}

static void free_state(State *s) {
    free(s->wait_time);
    free(s->first_run);
    free(s->end_time);
    free(s->timeline);
}


static SimulationResult simulate_multilevel(Process procs[], int n, int **init_prio_out) {
    SimulationResult result;
    State s;
    
    init_state(&s, n);
    
    int *init_prio = malloc(n * sizeof(int));
    for (int i = 0; i < n; i++)
        init_prio[i] = procs[i].priority;
    
    *init_prio_out = init_prio;
    
    int done = 0;

    while (done < n && s.time < 10000) {

      
        for (int i = 0; i < n; i++) {
            if (procs[i].arrival == s.time && procs[i].remaining_time > 0) {
                if (!in_any_queue(&s, i) && s.running != i) {
                    int p = procs[i].priority;
                    if (p < 0) p = 0;
                    if (p >= MAX_PRIO) p = MAX_PRIO - 1;
                    q_push(&s.queues[p], i);
                }
            }
        }

        
        for (int i = 0; i < n; i++) {
            if (s.wait_time[i] >= FAMINE_THRESHOLD &&
                procs[i].priority < MAX_PRIO - 1 &&
                procs[i].remaining_time > 0) {

                for (int p = 0; p < MAX_PRIO; p++) {
                    if (q_contains(&s.queues[p], i)) {
                        q_remove(&s.queues[p], i);
                        procs[i].priority++;
                        q_push(&s.queues[procs[i].priority], i);
                        s.wait_time[i] = 0;
                        break;
                    }
                }
            }
        }

        
        
        if (s.running != -1 && s.quantum_used >= s.quantum_size) {
            if (procs[s.running].priority > 0)
                procs[s.running].priority--;

            q_push(&s.queues[procs[s.running].priority], s.running);
            s.running = -1;
            s.quantum_used = 0;
        }

        
        
        if (s.running != -1) {
            int best = highest_prio_waiting(&s);
            if (best > procs[s.running].priority) {
                q_push(&s.queues[procs[s.running].priority], s.running);
                s.running = -1;
            }
        }

        
        
        if (s.running == -1) {
            int next = select_next(&s);
            if (next != -1) {
                s.running = next;
                s.quantum_used = 0;
                s.quantum_size = get_quantum(procs[next].priority);
                if (s.first_run[next] == -1)
                    s.first_run[next] = s.time;
            }
        }

        
        
        if (s.running == -1) {
            s.timeline[s.timeline_len++] = -1;
        } else {
            s.timeline[s.timeline_len++] = s.running;
            procs[s.running].remaining_time--;
            s.quantum_used++;
            s.wait_time[s.running] = 0;

            if (procs[s.running].remaining_time == 0) {
                s.end_time[s.running] = s.time + 1;
                done++;
                s.running = -1;
            }
        }
        
      
      
        for (int i = 0; i < n; i++)
            if (s.running != i && in_any_queue(&s, i))
                s.wait_time[i]++;

        s.time++;
    }
    
    
    
    result.procs = procs;
    result.n = n;
    result.timeline = s.timeline;
    result.timeline_len = s.timeline_len;
    result.start = s.first_run;
    result.end = s.end_time;
    result.init_prio = init_prio;
    result.levels = NULL;
    result.total_time = s.time;
    
    free(s.wait_time);
    
    return result;
}


static void copy_to_gui_result(SimulationResult *result) {
    if (!current_result) return;
    
    strcpy(current_result->algo_name, "Multi-Level Dynamique");
    current_result->quantum = 0;
    current_result->process_count = result->n;
    current_result->processes = malloc(sizeof(Process) * result->n);
    if (!current_result->processes) {
        fprintf(stderr, "Erreur: malloc échoué pour processes\n");
        return;
    }
    
    for (int i = 0; i < result->n; i++) {
        current_result->processes[i] = result->procs[i];
    }
    current_result->timeline_len = result->timeline_len;
    for (int i = 0; i < result->timeline_len; i++) {
        current_result->timeline[i] = result->timeline[i];
    }
    for (int i = 0; i < result->n; i++) {
        current_result->start[i] = result->start[i];
        current_result->end[i] = result->end[i];
    }

    float sumT = 0.0f, sumW = 0.0f;
    for (int i = 0; i < result->n; i++) {
        int duration = 0;
        for (int j = 0; j < result->timeline_len; j++) {
            if (result->timeline[j] == i) duration++;
        }
        current_result->turnaround[i] = result->end[i] - result->procs[i].arrival;
        current_result->wait[i] = current_result->turnaround[i] - duration;
        sumT += current_result->turnaround[i];
        sumW += current_result->wait[i];
    }
    
    current_result->avg_turnaround = sumT / result->n;
    current_result->avg_wait = sumW / result->n;
}


void multi_level(Process procs[], int n) {
    int *init_prio = NULL;
    SimulationResult result = simulate_multilevel(procs, n, &init_prio);
    if (capture_mode && current_result) {
        // Mode GUI: copier vers current_result
        copy_to_gui_result(&result);
        free(init_prio);
        free(result.timeline);
        free(result.start);
        free(result.end);
    } else {
        const char *rules =
            "╔═══════════════════════════════════════════════════════════════════════╗\n"
            "║     ORDONNANCEMENT MULTI-NIVEAUX À PRIORITÉS DYNAMIQUES               ║\n"
            "╠═══════════════════════════════════════════════════════════════════════╣\n"
            "║ RÈGLES:                                                               ║\n"
            "║  • Quantum: Priorité 5,4 → 1 | Priorité 3,2 → 2 | Priorité 1,0 → 4    ║\n"
            "║  • Priorité -1 UNIQUEMENT si quantum TERMINÉ (jamais < 0)             ║\n"
            "║  • Préemption si processus plus prioritaire arrive (prio inchangée)   ║\n"
            "║  • Anti-famine: attente ≥6 unités → priorité +1 (max 5)               ║\n"
            "╚═══════════════════════════════════════════════════════════════════════╝\n\n";
        
        display_console_results(
            "Ordonnancement Multi-Level Dynamique",
            0,
            &result,
            rules
        );
        
        free(init_prio);
        free(result.timeline);
        free(result.start);
        free(result.end);
    }
}
