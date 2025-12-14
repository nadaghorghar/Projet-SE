
#include <stdio.h>
#include <string.h>
#include "console_display.h"


static void display_header(const char *algo_name, int quantum, const char *rules);
static void display_input_processes(Process procs[], int n, int show_prio);
static void display_timeline(int *timeline, int timeline_len, Process procs[]);
static void display_gantt(int *timeline, int timeline_len, Process procs[], int n);
static void display_statistics(Process procs[], int n, int *start, int *end, 
                               int *init_prio, int show_init_prio);
static void display_summary(Process procs[], int n, int *start, int *end, int total_time);

void display_console_results(
    const char *algo_name,
    int quantum,
    SimulationResult *result,
    const char *rules
) {
    if (!result || !result->procs || !result->timeline || !result->start || !result->end) {
        printf("❌ Erreur: Résultats de simulation invalides\n");
        return;
    }
    
    
    int show_prio = (strstr(algo_name, "Priorité") != NULL || 
                     strstr(algo_name, "Multi-Level") != NULL ||
                     strstr(algo_name, "Multi-Niveau") != NULL);
    int show_init_prio = (result->init_prio != NULL);
    
   
    display_header(algo_name, quantum, rules);
    display_input_processes(result->procs, result->n, show_prio);
    
    if (rules != NULL) {
        printf("Simulation en cours...\n\n");
        printf("════════════════════════ RÉSULTATS ════════════════════════\n\n");
    }
    
    display_timeline(result->timeline, result->timeline_len, result->procs);
    display_gantt(result->timeline, result->timeline_len, result->procs, result->n);
    display_statistics(result->procs, result->n, result->start, result->end, 
                      result->init_prio, show_init_prio);
    display_summary(result->procs, result->n, result->start, result->end, result->total_time);
}


static void display_header(const char *algo_name, int quantum, const char *rules) {
    if (rules != NULL) {
        printf("\n%s\n", rules);
    } else {
        printf("\n═══════════════════════════════════════════════════\n");
        if (quantum > 0) {
            printf("           %s (q=%d)\n", algo_name, quantum);
        } else {
            printf("           %s\n", algo_name);
        }
        printf("═══════════════════════════════════════════════════\n\n");
    }
}


static void display_input_processes(Process procs[], int n, int show_prio) {
    printf("PROCESSUS EN ENTRÉE:\n");
    
    if (show_prio) {
        
        printf("┌──────┬─────────┬───────┬──────────┐\n");
        printf("│ Nom  │ Arrivée │ Durée │ Priorité │\n");
        printf("├──────┼─────────┼───────┼──────────┤\n");
        for (int i = 0; i < n; i++) {
            printf("│ %-4s │ %7d │ %5d │ %8d │\n",
                   procs[i].name, procs[i].arrival, procs[i].duration, 
                   procs[i].priority);
        }
        printf("└──────┴─────────┴───────┴──────────┘\n\n");
    } else {
        
        printf("┌──────┬─────────┬───────┐\n");
        printf("│ Nom  │ Arrivée │ Durée │\n");
        printf("├──────┼─────────┼───────┤\n");
        for (int i = 0; i < n; i++) {
            printf("│ %-4s │ %7d │ %5d │\n",
                   procs[i].name, procs[i].arrival, procs[i].duration);
        }
        printf("└──────┴─────────┴───────┘\n\n");
    }
}

static void display_timeline(int *timeline, int timeline_len, Process procs[]) {
    printf("CHRONOLOGIE D'EXÉCUTION:\n");
    printf("─────────────────────────\n");
    
    for (int t = 0; t < timeline_len; t++) {
        if (timeline[t] == -1) {
            printf("[IDLE:%d→%d] ", t, t + 1);
        } else {
            printf("[%s:%d→%d] ", procs[timeline[t]].name, t, t + 1);
        }
        if ((t + 1) % 8 == 0) printf("\n");
    }
    printf("\n\n");
}


static void display_gantt(int *timeline, int timeline_len, Process procs[], int n) {
    printf("DIAGRAMME DE GANTT:\n");
    printf("───────────────────\n");
    
    printf("Time ");
    for (int t = 0; t <= timeline_len && t <= 50; t++) printf("%2d ", t);
    printf("\n");
    
    for (int i = 0; i < n; i++) {
        printf("%-4s ", procs[i].name);
        for (int t = 0; t < timeline_len && t <= 50; t++) {
            printf("%s", timeline[t] == i ? "## " : "   ");
        }
        printf("\n");
    }
    printf("\n");
}


static void display_statistics(Process procs[], int n, int *start, int *end, 
                               int *init_prio, int show_init_prio) {
    printf("STATISTIQUES DES PROCESSUS:\n");
    
    if (show_init_prio && init_prio) {
        
        printf("┌──────┬─────────┬───────┬──────────┬───────┬─────┬────────────┬─────────┐\n");
        printf("│ Proc │ Arrivée │ Durée │ Prio_ini │ Début │ Fin │ Turnaround │ Attente │\n");
        printf("├──────┼─────────┼───────┼──────────┼───────┼─────┼────────────┼─────────┤\n");
        for (int i = 0; i < n; i++) {
            int turn = end[i] - procs[i].arrival;
            int wait = turn - procs[i].duration;
            printf("│ %-4s │ %7d │ %5d │ %8d │ %5d │ %3d │ %10d │ %7d │\n",
                   procs[i].name, procs[i].arrival, procs[i].duration,
                   init_prio[i], start[i], end[i], turn, wait);
        }
        printf("└──────┴─────────┴───────┴──────────┴───────┴─────┴────────────┴─────────┘\n");
    } else {
        
        printf("┌──────┬─────────┬───────┬───────┬─────┬────────────┬─────────┐\n");
        printf("│ Proc │ Arrivée │ Durée │ Début │ Fin │ Turnaround │ Attente │\n");
        printf("├──────┼─────────┼───────┼───────┼─────┼────────────┼─────────┤\n");
        for (int i = 0; i < n; i++) {
            int turn = end[i] - procs[i].arrival;
            int wait = start[i] - procs[i].arrival;
            printf("│ %-4s │ %7d │ %5d │ %5d │ %3d │ %10d │ %7d │\n",
                   procs[i].name, procs[i].arrival, procs[i].duration,
                   start[i], end[i], turn, wait);
        }
        printf("└──────┴─────────┴───────┴───────┴─────┴────────────┴─────────┘\n");
    }
    printf("\n");
}


static void display_summary(Process procs[], int n, int *start, int *end, int total_time) {
    float sum_turnaround = 0.0f, sum_wait = 0.0f;
    int valid_count = 0;
    
    for (int i = 0; i < n; i++) {
        if (end[i] >= 0) {
            int turnaround = end[i] - procs[i].arrival;
            int wait = start[i] - procs[i].arrival;
            sum_turnaround += turnaround;
            sum_wait += wait;
            valid_count++;
        }
    }
    
    if (valid_count > 0) {
        printf("Temps de rotation moyen: %.2f unités\n", sum_turnaround / valid_count);
        printf("Temps d'attente moyen:   %.2f unités\n", sum_wait / valid_count);
    }
    if (total_time > 0) printf("Temps total simulation:  %d unités\n", total_time);
    printf("\n");
}
