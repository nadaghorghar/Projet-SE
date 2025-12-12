#ifndef GUI_H
#define GUI_H

#include "process.h"

// Structure pour stocker les résultats d'exécution
typedef struct {
    int timeline[2000];      // Augmenté pour Multi-Level
    int timeline_len;
    int start[MAXP];
    int end[MAXP];
    int turnaround[MAXP];
    int wait[MAXP];
    float avg_turnaround;
    float avg_wait;
    int process_count;
    Process *processes;      // Pointeur vers les processus triés
    int quantum;             // Pour Round Robin
    char algo_name[50];      // Nom de l'algorithme utilisé
} SchedulingResult;

// Variables globales pour capturer les résultats
extern SchedulingResult *current_result;
extern int capture_mode;

// Fonction principale pour lancer l'interface GTK
void lancer_interface_gtk(Process procs[], int count);

// Déclarations des fonctions d'ordonnancement
void fifo(Process procs[], int n);
void round_robin(Process procs[], int n, int quantum);
void priorite(Process procs[], int n);
void multi_level(Process procs[], int n);
void multi_level_static(Process procs[], int n);


#endif
