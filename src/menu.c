#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include "menu.h"
#include "process.h"

#define MAX_POLITIQUES 100

char politiques[MAX_POLITIQUES][200];
int politique_count = 0;

/* -----------------------------------------
   Charger dynamiquement la liste des .c
------------------------------------------ */
void charger_politiques() {
    DIR *dir = opendir("politiques");
    if (!dir) {
        printf("Erreur : impossible d'ouvrir le dossier 'politiques'\n");
        exit(1);
    }

    struct dirent *entry;
    politique_count = 0;

    while ((entry = readdir(dir)) != NULL) {

        // Ignorer . et ..
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        // Ne garder que les .c
        int len = strlen(entry->d_name);
        if (len < 3 || strcmp(entry->d_name + len - 2, ".c") != 0)
            continue;

        // Ajouter
        strcpy(politiques[politique_count], entry->d_name);
        politique_count++;
    }

    closedir(dir);
}

/* -----------------------------------------
   Afficher les politiques disponibles
------------------------------------------ */
void afficher_policies() {
    charger_politiques();

    printf("\n=== Politiques détectées dans /politiques ===\n");

    for (int i = 0; i < politique_count; i++) {
        // Créer une copie du nom sans l'extension .c
        char nom_sans_extension[200];
        strcpy(nom_sans_extension, politiques[i]);
        
        // Enlever les 2 derniers caractères (".c")
        int len = strlen(nom_sans_extension);
        nom_sans_extension[len - 2] = '\0';
        
        printf("%d. %s\n", i + 1, nom_sans_extension);
    }
}

/* -----------------------------------------
   Choisir une politique
------------------------------------------ */
int choisir_politique() {
    char input[50];
    int choix;

    printf("\nChoisissez une politique : ");
    fgets(input, sizeof(input), stdin);

    if (sscanf(input, "%d", &choix) != 1 || choix < 1 || choix > politique_count) {
        printf("Choix invalide.\n");
        exit(1);
    }

    return choix - 1; // index réel
}

/* -----------------------------------------
   Exécuter la politique choisie
------------------------------------------ */
void executer_politique(int index, Process procs[], int count) {

    char path[300];
    char lib[300];

    // Ex : "politiques/fifo.c"
    snprintf(path, sizeof(path), "politiques/%s", politiques[index]);

    // Ex : "politiques/fifo.c.so"
    snprintf(lib, sizeof(lib), "politiques/%s.so", politiques[index]);

    // Nettoyer les anciens .so
    remove(lib);

    // Compilation dynamique
    char cmd[800];
    snprintf(cmd, sizeof(cmd),
             "gcc -shared -fPIC -Isrc %s -o %s",
             path, lib);

    system(cmd);

    // Charger la bibliothèque
    void *handle = dlopen(lib, RTLD_NOW);
    if (!handle) {
        printf("Erreur dlopen: %s\n", dlerror());
        exit(1);
    }

    // Nom de la fonction = nom du fichier sans .c
    char func_name[200];
    strcpy(func_name, politiques[index]);
    func_name[strlen(func_name) - 2] = '\0'; // enlever ".c"

    printf("\n>>> Exécution de : %s\n", func_name);

    /* ----------------------------
       Cas particulier Round Robin
       Fonction = round_robin(procs, count, quantum)
    ----------------------------- */
    if (strcmp(func_name, "round_robin") == 0) {

        int q;
        printf("➡ Vous avez choisi Round Robin.\n");
        printf("➡ Entrez le quantum : ");
        scanf("%d", &q);
        getchar(); // vider buffer

        if (q <= 0) q = 1;

        // Charger la fonction RR
        void (*rr_func)(Process*, int, int) = dlsym(handle, func_name);

        if (!rr_func) {
            printf("Erreur dlsym: %s\n", dlerror());
            exit(1);
        }

        // Exécuter
        rr_func(procs, count, q);
    }

    /* ----------------------------
       Autres politiques
       Fonction = fifo(procs, count)
    ----------------------------- */
    else {
        void (*alg_func)(Process*, int) = dlsym(handle, func_name);

        if (!alg_func) {
            printf("Erreur dlsym: %s\n", dlerror());
            exit(1);
        }

        alg_func(procs, count);
    }

    dlclose(handle);
}
