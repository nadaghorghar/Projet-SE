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

static void initQueue(Queue *q) { q->front = q->rear = 0; }
static int isEmpty(Queue *q) { return q->front == q->rear; }
static void enqueue(Queue *q, int val) { if (q->rear < MAXP) q->items[q->rear++] = val; }
static int dequeue(Queue *q) { if (!isEmpty(q)) return q->items[q->front++]; return -1; }
static int is_in_queue(Queue *q, int i) {
    for (int k = q->front; k < q->rear; k++) if (q->items[k] == i) return 1;
    return 0;
}

/* multi_level_static : Round Robin par niveau (statique) */
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
            if (distinct[j] == procs[i].priority) { exists = 1; break; }
        if (!exists) distinct[dcount++] = procs[i].priority;
    }

    /* 2) trier priorités décroissant (plus grand = plus prioritaire) */
    for (int i = 0; i < dcount - 1; i++)
        for (int j = i + 1; j < dcount; j++)
            if (distinct[j] > distinct[i]) { int tmp = distinct[i]; distinct[i] = distinct[j]; distinct[j] = tmp; }

    /* 3) affecter niveau selon priorité */
    for (int i = 0; i < n; i++) {
        level[i] = 0;
        for (int j = 0; j < dcount; j++)
            if (procs[i].priority == distinct[j]) { level[i] = j; break; }
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
        /* remplir current_result de manière sûre */
        strncpy(current_result->algo_name, "Multi-Level Static", sizeof(current_result->algo_name)-1);
        current_result->algo_name[sizeof(current_result->algo_name)-1] = '\0';
        current_result->quantum = QUANTUM_RR;
        current_result->process_count = n;

        /* allouer et copier processus dans current_result->processes */
        current_result->processes = malloc(sizeof(Process) * n);
        if (!current_result->processes) { free(queues); return; }
        for (int i = 0; i < n; i++) {
            current_result->processes[i] = procs[i];
            /* assure la cohérence du champ remaining_time si présent */
            current_result->processes[i].remaining_time = procs[i].duration;
        }

        /* simulation (remplissage timeline sans affichage console) */
        while (done < n && timeline_index < MAX_TIMELINE) {
            /* ajouter arrivants */
            for (int i = 0; i < n; i++) {
                if (procs[i].arrival <= time && remaining[i] > 0) {
                    if (!is_in_queue(&queues[level[i]], i)) enqueue(&queues[level[i]], i);
                }
            }

            int executed = 0;
            for (int l = 0; l < levels; l++) {
                if (!isEmpty(&queues[l])) {
                    int p = dequeue(&queues[l]);
                    if (!start_done[p]) { start[p] = time; start_done[p] = 1; }

                    int to_run = (remaining[p] < QUANTUM_RR) ? remaining[p] : QUANTUM_RR;
                    for (int u = 0; u < to_run && timeline_index < MAX_TIMELINE; u++) {
                        timeline[timeline_index++] = p;
                        time++;
                        remaining[p]--;

                        /* ajouter arrivants durant l'exécution */
                        for (int i = 0; i < n; i++) {
                            if (procs[i].arrival <= time && remaining[i] > 0)
                                if (!is_in_queue(&queues[level[i]], i) && i != p) enqueue(&queues[level[i]], i);
                        }

                        if (remaining[p] == 0) break;
                    }

                    if (remaining[p] == 0) { end[p] = time; done++; }
                    else enqueue(&queues[l], p);

                    executed = 1;
                    break;
                }
            }

            if (!executed) { if (timeline_index < MAX_TIMELINE) timeline[timeline_index++] = -1; time++; }
        }

        /* copier résultats dans current_result (en respectant limites) */
        current_result->timeline_len = timeline_index;
        for (int t = 0; t < timeline_index; t++) {
            /* protection supplémentaire si struct contient tableau fixe */
            current_result->timeline[t] = timeline[t];
        }

        /* calcul des statistiques et copie start/end/turnaround/wait */
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
        return; /* IMPORTANT : ne pas afficher en console en mode GTK */
    }

    /* -------------- MODE CONSOLE (identique à ce que tu avais) -------------- */
    printf("\nMULTI LEVEL STATIC (Round Robin par niveau)\n");
    printf("═══════════════════════════════════════════════════\n\n");

    printf("Mapping Priorité -> Niveau (Auto):\n");
    for (int i = 0; i < n; i++) printf("Processus %s : Priorité = %d -> Niveau = %d\n",
                                       procs[i].name, procs[i].priority, level[i]);
    printf("\n");

    /* refaire la simulation pour affichage console (séparée) */
    done = 0; time = 0; timeline_index = 0;
    for (int i = 0; i < n; i++) { remaining[i] = procs[i].duration; start[i] = end[i] = -1; start_done[i] = 0; }

    while (done < n && timeline_index < MAX_TIMELINE) {
        for (int i = 0; i < n; i++) {
            if (procs[i].arrival <= time && remaining[i] > 0) {
                if (!is_in_queue(&queues[level[i]], i)) enqueue(&queues[level[i]], i);
            }
        }

        int executed = 0;
        for (int l = 0; l < levels; l++) {
            if (!isEmpty(&queues[l])) {
                int p = dequeue(&queues[l]);
                if (!start_done[p]) { start[p] = time; start_done[p] = 1; }

                int to_run = (remaining[p] < QUANTUM_RR) ? remaining[p] : QUANTUM_RR;
                for (int u = 0; u < to_run && timeline_index < MAX_TIMELINE; u++) {
                    timeline[timeline_index++] = p;
                    printf("[%s] ", procs[p].name);
                    time++;
                    remaining[p]--;

                    for (int i = 0; i < n; i++)
                        if (procs[i].arrival <= time && remaining[i] > 0 && i != p)
                            if (!is_in_queue(&queues[level[i]], i)) enqueue(&queues[level[i]], i);

                    if (remaining[p] == 0) break;
                }

                if (remaining[p] == 0) { end[p] = time; done++; }
                else enqueue(&queues[l], p);

                executed = 1;
                break;
            }
        }

        if (!executed) { timeline[timeline_index++] = -1; printf("[IDLE] "); time++; }
    }

    /* affichage GANTT (console) */
    printf("\n\nGANTT (premiers 50):\nTime ");
    for (int t = 0; t <= 50; t++) printf("%2d ", t);
    printf("\n");
    for (int i = 0; i < n; i++) {
        printf("%-4s ", procs[i].name);
        for (int t = 0; t < timeline_index && t <= 50; t++)
            printf("%s", timeline[t] == i ? "## " : "   ");
        printf("\n");
    }

    /* statistiques console et préparation pour GUI (optionnel) */
    printf("\nSTATISTIQUES:\n");
    float sumT = 0.0f, sumW = 0.0f;
    for (int i = 0; i < n; i++) {
        int turn = (end[i] >= 0) ? (end[i] - procs[i].arrival) : -1;
        int wait = (turn >= 0) ? (turn - procs[i].duration) : -1;
        if (turn >= 0) { sumT += turn; sumW += wait; }
        printf("%s Arr=%d Dur=%d Deb=%d Fin=%d Turn=%d Wait=%d\n",
               procs[i].name, procs[i].arrival, procs[i].duration, start[i], end[i], turn, wait);
    }
    printf("\nTemps moyen Rotation = %.2f\n", sumT / n);
    printf("Temps moyen Attente = %.2f\n", sumW / n);

    /* Optionnel : remplir current_result si l'appelant souhaite l'utiliser après exécution console.
       gui.c alloue current_result avant d'appeler l'algorithme en mode GTK, mais en exécution pure
       console current_result peut être NULL — on protège. */
    if (current_result) {
        strncpy(current_result->algo_name, "Multi-Level Static", sizeof(current_result->algo_name)-1);
        current_result->algo_name[sizeof(current_result->algo_name)-1] = '\0';
        current_result->quantum = QUANTUM_RR;
        current_result->process_count = n;

        /* allouer/copie des processus */
        if (current_result->processes) free(current_result->processes);
        current_result->processes = malloc(sizeof(Process) * n);
        if (current_result->processes) {
            for (int i = 0; i < n; i++) current_result->processes[i] = procs[i];
        }

        current_result->timeline_len = timeline_index;
        for (int t = 0; t < timeline_index; t++) current_result->timeline[t] = timeline[t];

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

