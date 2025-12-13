#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "process.h"

#define MAX_PRIORITE 5  // Priorité maximale autorisée

int read_processes_from_file(const char *filename, Process procs[], int *count) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "❌ ERREUR : Impossible d'ouvrir le fichier %s.\n", filename);
        return -1;
    }
    
    *count = 0;
    char line[200];
    int line_num = 0;
    int erreurs_trouvees = 0;
    
    printf("=== Début de la lecture du fichier ===\n");
    
    while (fgets(line, sizeof(line), f) != NULL && *count < MAXP) {
        line_num++;
        
        // Nettoyage de la ligne
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r' || line[len-1] == ' ')) {
            line[len-1] = '\0';
            len--;
        }
        
        // Ignorer les lignes vides et commentaires
        if (strlen(line) == 0 || line[0] == '#')
            continue;
        
        // Extraction des données
        char temp_name[100];
        int temp_arrival, temp_duration, temp_priority;
        
        int r = sscanf(line, "%99s %d %d %d",
                      temp_name, &temp_arrival, &temp_duration, &temp_priority);
        
        if (r == 4) {
            // ========== VALIDATIONS ==========
            int erreur_ligne = 0;
            
            // Validation 1 : Temps d'arrivée négatif
            if (temp_arrival < 0) {
                fprintf(stderr, "❌ ERREUR ligne %d : Le processus '%s' a un temps d'arrivée négatif (%d)\n", 
                        line_num, temp_name, temp_arrival);
                erreur_ligne = 1;
                erreurs_trouvees = 1;
            }
            
            // Validation 2 : Durée d'exécution invalide (négative ou nulle)
            if (temp_duration <= 0) {
                fprintf(stderr, "❌ ERREUR ligne %d : Le processus '%s' a une durée d'exécution invalide (%d). La durée doit être > 0\n", 
                        line_num, temp_name, temp_duration);
                erreur_ligne = 1;
                erreurs_trouvees = 1;
            }
            
            // Validation 3 : Priorité invalide (< 0 ou > MAX_PRIORITE)
            if (temp_priority < 0 || temp_priority > MAX_PRIORITE) {
                fprintf(stderr, "❌ ERREUR ligne %d : Le processus '%s' a une priorité invalide (%d). La priorité doit être entre 0 et %d\n", 
                        line_num, temp_name, temp_priority, MAX_PRIORITE);
                erreur_ligne = 1;
                erreurs_trouvees = 1;
            }
            
            // Si aucune erreur, on ajoute le processus
            if (!erreur_ligne) {
                strcpy(procs[*count].name, temp_name);
                procs[*count].arrival = temp_arrival;
                procs[*count].duration = temp_duration;
                procs[*count].priority = temp_priority;
                procs[*count].remaining_time = temp_duration;
                (*count)++;
                printf("✓ Processus '%s' chargé avec succès\n", temp_name);
            }
            
        } else if (r > 0) {
            fprintf(stderr, "❌ ERREUR ligne %d : Format invalide (attendu: nom arrivee duree priorite)\n", line_num);
            erreurs_trouvees = 1;
        }
    }
    
    fclose(f);
    
    // Si des erreurs ont été trouvées, on arrête le programme
    if (erreurs_trouvees) {
        fprintf(stderr, "\n🛑 Des erreurs ont été détectées dans le fichier. Le programme ne peut pas continuer.\n");
        fprintf(stderr, "Veuillez corriger le fichier et réessayer.\n\n");
        return -1;  // Code d'erreur
    }
    
    // Vérifier qu'au moins un processus a été chargé
    if (*count == 0) {
        fprintf(stderr, "❌ ERREUR : Aucun processus valide n'a été trouvé dans le fichier.\n");
        return -1;
    }
    
    printf("\n✅ %d processus chargé(s) avec succès\n", *count);
    return 0;
}
