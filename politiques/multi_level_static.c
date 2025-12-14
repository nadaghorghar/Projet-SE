#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "process.h"
#include "gui.h"
#include "console_display.h"

#define QUANTUM_RR 2
#define MAXP 50
#define MAX_TIMELINE 2000

/* ================= FILE SIMPLE ================= */

typedef struct {
    int items[MAXP];
    int front, rear;
} Queue;

static void initQueue(Queue *q) {
    q->front = q->rear = 0;
}

static int isEmpty(Queue *q) {
    return q->front == q->rear;
}

static void enqueue(Queue *q, int val) {
    if (q->rear < MAXP)
        q->items[q->rear++] = val;
}

static int dequeue(Queue *q) {
    if (!isEmpty(q))
        return q->items[q->front++];
    return -1;
}

static int is_in_queue(Queue *q, int i) {
    for (int k = q->front; k < q->rear; k++)
        if (q->items[k] == i) return 1;
    return 0;
}

/* Trouver le niveau le plus prioritaire non vide */
static int find_highest_priority_level(Queue *queues, int levels) {
    for (int l = 0; l < levels; l++) {
        if (!isEmpty(&queues[l])) return l;
    }
    return -1;
}

/* ═══════════════════════════════════════════════════════════
   HELPER: Extraire et trier les priorités distinctes
   ═══════════════════════════════════════════════════════════ */
static int extract_distinct_priorities(Process procs[], int n, int *distinct) {
    int dcount = 0;
    
    // Extraire priorités distinctes
    for (int i = 0; i < n; i++) {
        int exists = 0;
        for (int j = 0; j < dcount; j++) {
            if (distinct[j] == procs[i].priority) {
                exists = 1;
                break;
            }
        }
        if (!exists)
            distinct[dcount++] = procs[i].priority;
    }
    
    // Trier décroissant
    for (int i = 0; i < dcount - 1; i++) {
        for (int j = i + 1; j < dcount; j++) {
            if (distinct[j] > distinct[i]) {
                int tmp = distinct[i];
                distinct[i] = distinct[j];
                distinct[j] = tmp;
            }
        }
    }
    
    return dcount;
}

/* ═══════════════════════════════════════════════════════════
   HELPER: Affecter les niveaux aux processus
   ═══════════════════════════════════════════════════════════ */
static void assign_levels(Process procs[], int n, int *level, int *distinct, int dcount) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < dcount; j++) {
            if (procs[i].priority == distinct[j]) {
                level[i] = j;
                break;
            }
        }
    }
}

/* ═══════════════════════════════════════════════════════════
   CORE: Simuler Multi-Level Static et retourner les résultats
   ═══════════════════════════════════════════════════════════ */
static SimulationResult simulate_multilevel_static(Process procs[], int n, int **level_out) {
    SimulationResult result;
    
    // Allouer mémoire pour les tableaux locaux
    static int remaining[MAXP];
    static int start[MAXP];
    static int end[MAXP];
    static int level[MAXP];
    static int start_done[MAXP];
    static int timeline[MAX_TIMELINE];
    static int distinct[MAXP];
    
    result.procs = procs;
    result.n = n;
    result.timeline = timeline;
    result.start = start;
    result.end = end;
    result.init_prio = NULL;
    
    // Initialisation
    int time = 0;
    int timeline_index = 0;
    int done = 0;
    
    for (int i = 0; i < n; i++) {
        remaining[i] = procs[i].duration;
        start[i] = end[i] = -1;
        start_done[i] = 0;
    }
    
    // Extraire et trier les priorités
    int dcount = extract_distinct_priorities(procs, n, distinct);
    int levels = (dcount > 0) ? dcount : 1;
    
    // Affecter les niveaux
    assign_levels(procs, n, level, distinct, dcount);
    result.levels = level;
    *level_out = level;
    
    // Créer les files d'attente
    Queue *queues = malloc(sizeof(Queue) * levels);
    if (!queues) {
        result.timeline_len = 0;
        result.total_time = 0;
        return result;
    }
    
    for (int l = 0; l < levels; l++)
        initQueue(&queues[l]);
    
    // Simulation
    int current_process = -1;
    int quantum_used = 0;
    
    while (done < n && timeline_index < MAX_TIMELINE) {
        
        // Ajouter processus arrivants
        for (int i = 0; i < n; i++) {
            if (procs[i].arrival <= time && remaining[i] > 0) {
                if (!is_in_queue(&queues[level[i]], i) &&
                    i != current_process) {
                    enqueue(&queues[level[i]], i);
                }
            }
        }
        
        // Préemption immédiate
        int highest_level = find_highest_priority_level(queues, levels);
        if (current_process != -1 &&
            highest_level != -1 &&
            highest_level < level[current_process]) {
            
            enqueue(&queues[level[current_process]], current_process);
            current_process = -1;
            quantum_used = 0;
        }
        
        // Sélection du processus
        if (current_process == -1 || quantum_used >= QUANTUM_RR) {
            
            if (current_process != -1 && remaining[current_process] > 0) {
                enqueue(&queues[level[current_process]], current_process);
            }
            
            highest_level = find_highest_priority_level(queues, levels);
            if (highest_level != -1) {
                current_process = dequeue(&queues[highest_level]);
                quantum_used = 0;
                
                if (!start_done[current_process]) {
                    start[current_process] = time;
                    start_done[current_process] = 1;
                }
            } else {
                current_process = -1;
            }
        }
        
        // Exécution
        if (current_process != -1) {
            timeline[timeline_index++] = current_process;
            remaining[current_process]--;
            quantum_used++;
            
            if (remaining[current_process] == 0) {
                end[current_process] = time + 1;
                done++;
                current_process = -1;
                quantum_used = 0;
            }
        } else {
            timeline[timeline_index++] = -1;
        }
        
        time++;
    }
    
    free(queues);
    
    result.timeline_len = timeline_index;
    result.total_time = time;
    
    return result;
}

/* ═══════════════════════════════════════════════════════════
   HELPER: Copier les résultats vers la structure GUI
   ═══════════════════════════════════════════════════════════ */
static void copy_to_gui_result(SimulationResult *result) {
    if (!current_result) return;
    
    strcpy(current_result->algo_name, "Multi-Level Statique");
    current_result->quantum = QUANTUM_RR;
    current_result->process_count = result->n;
    
    // Copier les processus
    current_result->processes = malloc(sizeof(Process) * result->n);
    if (!current_result->processes) {
        fprintf(stderr, "Erreur: malloc échoué pour processes\n");
        return;
    }
    
    for (int i = 0; i < result->n; i++) {
        current_result->processes[i] = result->procs[i];
    }
    
    // Copier la timeline
    current_result->timeline_len = result->timeline_len;
    for (int i = 0; i < result->timeline_len; i++) {
        current_result->timeline[i] = result->timeline[i];
    }
    
    // Copier start/end
    for (int i = 0; i < result->n; i++) {
        current_result->start[i] = result->start[i];
        current_result->end[i] = result->end[i];
    }
    
    // Calculer moyennes
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

/* ═══════════════════════════════════════════════════════════
   MAIN: Point d'entrée Multi-Level Static (GUI ou Console)
   ═══════════════════════════════════════════════════════════ */
void multi_level_static(Process procs[], int n) {
    if (n <= 0) return;
    if (n > MAXP) n = MAXP;
    
    int *level = NULL;
    
    // ★★★ UNE SEULE SIMULATION ★★★
    SimulationResult result = simulate_multilevel_static(procs, n, &level);
    
    // Router selon le mode
    if (capture_mode && current_result) {
        // Mode GUI: copier vers current_result
        copy_to_gui_result(&result);
    } else {
        // Mode Console: afficher directement
        const char *rules =
            "╔═══════════════════════════════════════════════════════════════════════╗\n"
            "║ ORDONNANCEMENT MULTI-NIVEAUX STATIQUE (RR + PRÉEMPTION)               ║\n"
            "╠═══════════════════════════════════════════════════════════════════════╣\n"
            "║ RÈGLES :                                                              ║\n"
            "║ • Quantum fixe : 2 unités pour tous les processus                     ║\n"
            "║ • Priorités statiques (pas de dégradation ni promotion)               ║\n"
            "║ • Round Robin à l'intérieur de chaque niveau                          ║\n"
            "║ • Préemption immédiate par un processus plus prioritaire              ║\n"
            "╚═══════════════════════════════════════════════════════════════════════╝\n\n";
        
        display_console_results(
            "Ordonnancement Multi-Level Statique",
            QUANTUM_RR,
            &result,
            rules
        );
    }
}
