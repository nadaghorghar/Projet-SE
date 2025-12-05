#include <stdio.h>
#include <stdlib.h>
#include "process.h"

void priorite(Process procs[], int n) {
    printf("\n═══════════════════════════════════════════════════\n");
    printf("           ORDONNANCEMENT PRIORITÉ PRÉEMPTIVE\n");
    printf("═══════════════════════════════════════════════════\n\n");
    
    int time = 0;
    int timeline[1000];
    int timeline_len = 0;
    int remaining[n], start[n], end[n];
    int done = 0;
    
    for (int i = 0; i < n; i++) {
        remaining[i] = procs[i].duration;
        start[i] = -1;
        end[i] = -1;
    }
    
    while (done < n) {
        // Trouver le meilleur processus prêt
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (procs[i].arrival <= time && remaining[i] > 0) {
                if (best == -1 || procs[i].priority > procs[best].priority ||
                    (procs[i].priority == procs[best].priority && procs[i].arrival < procs[best].arrival)) {
                    best = i;
                }
            }
        }
        
        if (best == -1) {
            timeline[timeline_len++] = -1;
            time++;
            continue;
        }
        
        if (start[best] == -1)
            start[best] = time;
        
        timeline[timeline_len++] = best;
        remaining[best]--;
        time++;
        
        if (remaining[best] == 0) {
            end[best] = time;
            done++;
        }
    }
    
    // ═══ CHRONOLOGIE ═══
    printf("CHRONOLOGIE D'EXÉCUTION:\n");
    printf("─────────────────────────\n");
    for (int t = 0; t < timeline_len; t++) {
        if (timeline[t] == -1)
            printf("[IDLE:%d→%d] ", t, t+1);
        else
            printf("[%s:%d→%d] ", procs[timeline[t]].name, t, t+1);
        if ((t+1)%8==0) printf("\n");
    }
    
    // ═══ GANTT ═══
    printf("\n\nDIAGRAMME DE GANTT:\n");
    printf("───────────────────\n");
    printf("Time ");
    for (int t = 0; t <= timeline_len && t <= 50; t++) printf("%2d ", t);
    printf("\n");
    
    for (int i = 0; i < n; i++) {
        printf("%-4s ", procs[i].name);
        for (int t = 0; t < timeline_len && t <= 50; t++)
            printf("%s", timeline[t] == i ? "## " : "   ");
        printf("\n");
    }
    
    // ═══ STATISTIQUES ═══
    printf("\nSTATISTIQUES DES PROCESSUS:\n");
    printf("┌──────┬─────────┬───────┬─────────┬───────┬────────────┬─────────┐\n");
    printf("│ Proc │ Arrivée │ Durée │ Début   │ Fin   │ Turnaround │ Attente │\n");
    printf("├──────┼─────────┼───────┼─────────┼───────┼────────────┼─────────┤\n");
    
    float sumT=0, sumW=0;
    for (int i = 0; i < n; i++) {
        int turn = end[i] - procs[i].arrival;
        int wait = turn - procs[i].duration;
        sumT += turn;
        sumW += wait;
        printf("│ %-4s │ %7d │ %5d │ %7d │ %5d │ %10d │ %7d │\n",
               procs[i].name, procs[i].arrival, procs[i].duration,
               start[i], end[i], turn, wait);
    }
    printf("└──────┴─────────┴───────┴─────────┴───────┴────────────┴─────────┘\n\n");
    printf("Temps de rotation moyen: %.2f\n", sumT / n);
    printf("Temps d'attente moyen:   %.2f\n\n", sumW / n);
}
