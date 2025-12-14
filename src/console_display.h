/* src/console_display.h - Module d'affichage console centralisé */
#ifndef CONSOLE_DISPLAY_H
#define CONSOLE_DISPLAY_H

#include "process.h"

/* Structure pour les résultats de simulation */
typedef struct {
    Process *procs;           // Tableau des processus
    int n;                    // Nombre de processus
    int *timeline;            // Timeline d'exécution
    int timeline_len;         // Longueur de la timeline
    int *start;               // Temps de début de chaque processus
    int *end;                 // Temps de fin de chaque processus
    int *init_prio;           // Priorités initiales (pour multi-level dynamique)
    int *levels;              // Niveaux (pour multi-level statique)
    int total_time;           // Temps total de simulation
} SimulationResult;

/* 
 * FONCTION PRINCIPALE - À UTILISER DANS TOUS LES ALGORITHMES
 * 
 * Paramètres:
 *   - algo_name: nom de l'algorithme (ex: "FIFO", "Round Robin")
 *   - quantum: valeur du quantum (0 si pas de quantum)
 *   - result: pointeur vers la structure SimulationResult
 *   - rules: règles spéciales (NULL pour algorithmes simples)
 */
void display_console_results(
    const char *algo_name,
    int quantum,
    SimulationResult *result,
    const char *rules
);

#endif /* CONSOLE_DISPLAY_H */
