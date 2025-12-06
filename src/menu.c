#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include "menu.h"
#include "process.h"
#include "gui.h"

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
        printf("%d. %s\n", i + 1, politiques[i]);
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

    return choix - 1;
}

/* -----------------------------------------
   Exécuter la politique choisie
------------------------------------------ */
void executer_politique(int index, Process procs[], int count) {
    char path[300];
    char lib[300];

    snprintf(path, sizeof(path), "politiques/%s", politiques[index]);
    snprintf(lib, sizeof(lib), "politiques/%s.so", politiques[index]);

    // Nettoyer les anciens .so
    remove(lib);

    // IMPORTANT : Désactiver le mode capture pour le mode console
    capture_mode = 0;

    // Compilation dynamique avec export des symboles globaux
    char cmd[800];
    snprintf(cmd, sizeof(cmd),
             "gcc -shared -fPIC -Isrc %s build/gui_globals.o -o %s",
             path, lib);

    int compile_result = system(cmd);
    if (compile_result != 0) {
        printf("Erreur lors de la compilation de la politique.\n");
        exit(1);
    }

    // Charger la bibliothèque avec RTLD_LAZY
    void *handle = dlopen(lib, RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {
        printf("Erreur dlopen: %s\n", dlerror());
        exit(1);
    }

    // Nom de la fonction = nom du fichier sans .c
    char func_name[200];
    strcpy(func_name, politiques[index]);
    func_name[strlen(func_name) - 2] = '\0';

    printf("\n>>> Exécution de : %s\n", func_name);

    /* Round Robin */
    if (strcmp(func_name, "round_robin") == 0) {
        int q;
        printf("➡ Vous avez choisi Round Robin.\n");
        printf("➡ Entrez le quantum : ");
        scanf("%d", &q);
        getchar();

        if (q <= 0) q = 1;

        void (*rr_func)(Process*, int, int) = dlsym(handle, func_name);

        if (!rr_func) {
            printf("Erreur dlsym: %s\n", dlerror());
            dlclose(handle);
            exit(1);
        }

        rr_func(procs, count, q);
    }
    /* Autres politiques */
    else {
        void (*alg_func)(Process*, int) = dlsym(handle, func_name);

        if (!alg_func) {
            printf("Erreur dlsym: %s\n", dlerror());
            dlclose(handle);
            exit(1);
        }

        alg_func(procs, count);
    }

    dlclose(handle);
}
