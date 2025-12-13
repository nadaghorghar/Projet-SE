/* politiques/multi_level_static.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "process.h"
#include "gui.h"

#define QUANTUM_RR 2
#define MAXP 50
#define MAX_TIMELINE 2000

/* Simple queue structure */
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
    if (q->rear < MAXP) q->items[q->rear++] = val;
}

static int dequeue(Queue *q) {
    if (!isEmpty(q)) return q->items[q->front++];
    return -1;
}

static int is_in_queue(Queue *q, int i) {
    for (int k = q->front; k < q->rear; k++)
        if (q->items[k] == i) return 1;
    return 0;
}

/* Trouver le niveau le plus prioritaire avec des processus prêts */
static int find_highest_priority_level(Queue *queues, int levels) {
    for (int l = 0; l < levels; l++) {
        if (!isEmpty(&queues[l])) return l;
    }
    return -1;
}

/* multi_level_static : Round Robin par niveau avec préemption immédiate */
void multi_level_static(Process procs[], int n) {
    if (n <= 0) return;
    if (n > MAXP) n = MAXP;

    int remaining[MAXP];
    int start[MAXP], end[MAXP], level[MAXP];
    int start_done[MAXP];
    int done = 0, time = 0;

    /* initialisations */
    for (int i = 0; i < n; i++) {
        remaining[i] = procs[i].duration;
        start[i] = end[i] = -1;
        start_done[i] = 0;
    }

    /* 1) récupérer priorités distinctes */
    int distinct[MAXP], dcount = 0;
    for (int i = 0; i < n; i++) {
        int exists = 0;
        for (int j = 0; j < dcount; j++)
            if (distinct[j] == procs[i].priority) {
                exists = 1;
                break;
            }
        if (!exists) distinct[dcount++] = procs[i].priority;
    }

    /* 2) trier priorités décroissant (plus grand = plus prioritaire) */
    for (int i = 0; i < dcount - 1; i++)
        for (int j = i + 1; j < dcount; j++)
            if (distinct[j] > distinct[i]) {
                int tmp = distinct[i];
                distinct[i] = distinct[j];
                distinct[j] = tmp;
            }

    /* 3) affecter niveau selon priorité */
    for (int i = 0; i < n; i++) {
        level[i] = 0;
        for (int j = 0; j < dcount; j++)
            if (procs[i].priority == distinct[j]) {
                level[i] = j;
                break;
            }
    }

    /* 4) créer queues par niveau */
    int levels = (dcount > 0) ? dcount : 1;
    Queue *queues = malloc(sizeof(Queue) * levels);
    if (!queues) return;
    for (int l = 0; l < levels; l++) initQueue(&queues[l]);

    /* timeline local */
    int timeline[MAX_TIMELINE];
    int timeline_index = 0;

    /* -------------- MODE GTK (capture) -------------- */
    if (capture_mode && current_result) {
        strncpy(current_result->algo_name, "Multi-Level Static", sizeof(current_result->algo_name)-1);
        current_result->algo_name[sizeof(current_result->algo_name)-1] = '\0';
        current_result->quantum = QUANTUM_RR;
        current_result->process_count = n;

        current_result->processes = malloc(sizeof(Process) * n);
        if (!current_result->processes) {
            free(queues);
            return;
        }
        for (int i = 0; i < n; i++) {
            current_result->processes[i] = procs[i];
            current_result->processes[i].remaining_time = procs[i].duration;
        }

        /* simulation avec préemption */
        int current_process = -1;
        int quantum_used = 0;

        while (done < n && timeline_index < MAX_TIMELINE) {
            /* ajouter tous les processus arrivants */
            for (int i = 0; i < n; i++) {
                if (procs[i].arrival <= time && remaining[i] > 0) {
                    if (!is_in_queue(&queues[level[i]], i) && i != current_process) {
                        enqueue(&queues[level[i]], i);
                    }
                }
            }

            /* vérifier si préemption nécessaire */
            int highest_level = find_highest_priority_level(queues, levels);
            
            if (current_process != -1 && highest_level != -1 && highest_level < level[current_process]) {
                /* Préemption : processus de plus haute priorité est arrivé */
                enqueue(&queues[level[current_process]], current_process);
                current_process = -1;
                quantum_used = 0;
            }

            /* choisir processus à exécuter */
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

            /* exécuter une unité de temps */
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

        /* copier résultats */
        current_result->timeline_len = timeline_index;
        for (int t = 0; t < timeline_index; t++) {
            current_result->timeline[t] = timeline[t];
        }

        float sumT = 0.0f, sumW = 0.0f;
        for (int i = 0; i < n; i++) {
            current_result->start[i] = start[i];
            current_result->end[i] = end[i];
            if (end[i] >= 0) {
                current_result->turnaround[i] = end[i] - current_result->processes[i].arrival;
                current_result->wait[i] = current_result->turnaround[i] - current_result->processes[i].duration;
                sumT += current_result->turnaround[i];
                sumW += current_result->wait[i];
            } else {
                current_result->turnaround[i] = -1;
                current_result->wait[i] = -1;
            }
        }

        int valid = 0;
        for (int i = 0; i < n; i++) if (current_result->turnaround[i] >= 0) valid++;
        current_result->avg_turnaround = (valid > 0) ? (sumT / valid) : 0.0f;
        current_result->avg_wait = (valid > 0) ? (sumW / valid) : 0.0f;

        free(queues);
        return;
    }

    /* -------------- MODE CONSOLE (AFFICHAGE AMÉLIORÉ) -------------- */
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════════╗\n");
    printf("║ ORDONNANCEMENT MULTI-NIVEAUX STATIQUE (ROUND ROBIN + PRÉEMPTION)    ║\n");
    printf("╠═══════════════════════════════════════════════════════════════════════╣\n");
    printf("║ RÈGLES:                                                               ║\n");
    printf("║ • Quantum fixe: %d unités pour tous les processus                     ║\n", QUANTUM_RR);
    printf("║ • Priorités statiques (pas de changement dynamique)                  ║\n");
    printf("║ • Round Robin par niveau de priorité                                 ║\n");
    printf("║ • PRÉEMPTION IMMÉDIATE : processus haute priorité préempte           ║\n");
    printf("║   automatiquement un processus de priorité inférieure                ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════╝\n\n");

    printf("PROCESSUS EN ENTRÉE:\n");
    printf("┌──────┬─────────┬───────┬──────────┬─────────┐\n");
    printf("│ Nom  │ Arrivée │ Durée │ Priorité │ Niveau  │\n");
    printf("├──────┼─────────┼───────┼──────────┼─────────┤\n");
    for (int i = 0; i < n; i++)
        printf("│ %-4s │ %7d │ %5d │ %8d │ %7d │\n",
               procs[i].name, procs[i].arrival, procs[i].duration, procs[i].priority, level[i]);
    printf("└──────┴─────────┴───────┴──────────┴─────────┘\n\n");

    printf("Simulation en cours (avec préemption)...\n\n");

    /* refaire simulation pour console */
    done = 0;
    time = 0;
    timeline_index = 0;
    for (int i = 0; i < n; i++) {
        remaining[i] = procs[i].duration;
        start[i] = end[i] = -1;
        start_done[i] = 0;
    }
    for (int l = 0; l < levels; l++) initQueue(&queues[l]);

    int current_process = -1;
    int quantum_used = 0;

    while (done < n && timeline_index < MAX_TIMELINE) {
        /* ajouter arrivants */
        for (int i = 0; i < n; i++) {
            if (procs[i].arrival <= time && remaining[i] > 0) {
                if (!is_in_queue(&queues[level[i]], i) && i != current_process) {
                    enqueue(&queues[level[i]], i);
                }
            }
        }

        /* vérifier préemption */
        int highest_level = find_highest_priority_level(queues, levels);
        
        if (current_process != -1 && highest_level != -1 && highest_level < level[current_process]) {
            /* PRÉEMPTION */
            enqueue(&queues[level[current_process]], current_process);
            current_process = -1;
            quantum_used = 0;
        }

        /* sélection processus */
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

        /* exécution */
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

    /* affichage résultats */
    printf("\n════════════════════════ RÉSULTATS ════════════════════════\n\n");
    printf("CHRONOLOGIE D'EXÉCUTION:\n");
    printf("─────────────────────────\n");
    for (int t = 0; t < timeline_index; t++) {
        if (timeline[t] == -1)
            printf("[IDLE:%d→%d] ", t, t+1);
        else
            printf("[%s:%d→%d] ", procs[timeline[t]].name, t, t+1);
        if ((t + 1) % 8 == 0) printf("\n");
    }
    printf("\n\n");

    printf("DIAGRAMME DE GANTT:\n");
    printf("───────────────────\n");
    printf("Time ");
    for (int t = 0; t <= timeline_index && t <= 50; t++) printf("%2d ", t);
    printf("\n");
    for (int i = 0; i < n; i++) {
        printf("%-4s ", procs[i].name);
        for (int t = 0; t < timeline_index && t <= 50; t++)
            printf("%s", timeline[t] == i ? "## " : "   ");
        printf("\n");
    }

    printf("\nSTATISTIQUES DES PROCESSUS:\n");
    printf("┌──────┬─────────┬───────┬──────────┬───────┬─────┬────────────┬─────────┐\n");
    printf("│ Proc │ Arrivée │ Durée │ Priorité │ Début │ Fin │ Turnaround │ Attente │\n");
    printf("├──────┼─────────┼───────┼──────────┼───────┼─────┼────────────┼─────────┤\n");

    float sumT = 0.0f, sumW = 0.0f;
    for (int i = 0; i < n; i++) {
        int turn = (end[i] >= 0) ? (end[i] - procs[i].arrival) : -1;
        int wait = (turn >= 0) ? (turn - procs[i].duration) : -1;
        if (turn >= 0) {
            sumT += turn;
            sumW += wait;
        }
        printf("│ %-4s │ %7d │ %5d │ %8d │ %5d │ %3d │ %10d │ %7d │\n",
               procs[i].name, procs[i].arrival, procs[i].duration,
               procs[i].priority, start[i], end[i], turn, wait);
    }
    printf("└──────┴─────────┴───────┴──────────┴───────┴─────┴────────────┴─────────┘\n");
    printf("\n");
    printf("Temps de rotation moyen: %.2f unités\n", sumT / n);
    printf("Temps d'attente moyen: %.2f unités\n", sumW / n);
    printf("Temps total simulation: %d unités\n", time);

    /* Remplir current_result si nécessaire */
    if (current_result) {
        strncpy(current_result->algo_name, "Multi-Level Static", sizeof(current_result->algo_name)-1);
        current_result->algo_name[sizeof(current_result->algo_name)-1] = '\0';
        current_result->quantum = QUANTUM_RR;
        current_result->process_count = n;

        if (current_result->processes) free(current_result->processes);
        current_result->processes = malloc(sizeof(Process) * n);
        if (current_result->processes) {
            for (int i = 0; i < n; i++) current_result->processes[i] = procs[i];
        }

        current_result->timeline_len = timeline_index;
        for (int t = 0; t < timeline_index; t++)
            current_result->timeline[t] = timeline[t];

        for (int i = 0; i < n; i++) {
            current_result->start[i] = start[i];
            current_result->end[i] = end[i];
            if (end[i] >= 0) {
                current_result->turnaround[i] = end[i] - procs[i].arrival;
                current_result->wait[i] = current_result->turnaround[i] - procs[i].duration;
            } else {
                current_result->turnaround[i] = -1;
                current_result->wait[i] = -1;
            }
        }
        current_result->avg_turnaround = (n > 0) ? (sumT / n) : 0.0f;
        current_result->avg_wait = (n > 0) ? (sumW / n) : 0.0f;
    }

    free(queues);
}
