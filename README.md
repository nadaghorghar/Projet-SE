# Ordonnanceur Multitâche de Processus sous Linux

Ce projet est un simulateur d’ordonnancement CPU développé en C sous Linux. Il permet de charger une liste de processus décrits dans un fichier de configuration 
et de simuler leur exécution selon différentes politiques d’ordonnancement. 
Le simulateur repose sur une architecture modulaire où chaque politique est implémentée dans un module séparé, ce qui facilite l’extension et l’ajout de nouvelles stratégies.


---


## Fonctionnalités principales

L’objectif de ce projet est de développer en langage C sous Linux un simulateur d’ordonnancement capable de :

- Charger un ensemble de processus depuis un fichier de configuration.
- Appliquer différentes politiques d’ordonnancement.
- Afficher les résultats de simulation (version textuelle et affichage graphique).
- Charger dynamiquement les politiques disponibles depuis un répertoire dédié.
- Générer l’exécutable automatiquement grâce à un **Makefile**.


---



##  Structure du projet

```
.
├── src/           # Code source C du simulateur
├── politiques/    # Politique d’ordonnancement (fichiers .c séparés)
├── processus.txt  # Fichier de configuration des processus
├── Makefile       # Script de construction et compilation du projet
├── LICENSE        # Fichier de licence du projet (MIT)
├── README.md      # Documentation du projet et instructions d'utilisation
└── docs/          # Documentation (SCRUM + guide utilisateur)
```



---


## Technologies utilisées

Le projet a été développé en utilisant les technologies et concepts suivants :

- **Langage : C**  
  Le simulateur est entièrement écrit en langage C, un langage bas niveau performant et adapté pour la gestion fine des processus et de la mémoire.

- **Plateforme : Linux**  
  Le projet est conçu pour être compilé et exécuté sur un système Linux. Les scripts et le Makefile utilisent des commandes spécifiques à Linux.

- **Concepts et outils utilisés :**
  - **Structures de données** : pour gérer les informations sur les processus.  
  - **Makefile** : facilite la compilation et le lancement du projet.  
  - **Modularité** : certaines parties du projet peuvent être ajoutées ou modifiées facilement.


---



## Politiques d’ordonnancement implémentées

- FIFO (First In First Out)
- Round Robin
- Priorité préemptive
- Multi-level avec priorité statique
- Multi-level avec priorité dynamique (aging)

Chaque politique est implémentée sous forme d’une fonction contenue dans un fichier séparé, dans un répertoire dédié (`politiques/`).



---


## Fonctionnement général

Le programme lit un fichier texte de processus contenant la description d’un ensemble de processus :  
- **Nom du processus**  
- **Date d’arrivée**  
- **Durée des cycles CPU**  
- **Priorité (statique)**  
- Support des **commentaires** et **lignes blanches**

Ce fichier est passé en paramètre sur la ligne de commande :

```bash
./ordonnanceur processus.txt
```

Le programme présente ensuite un menu permettant de sélectionner dynamiquement la politique d’ordonnancement.



---



##  Format du fichier texte de processus

Exemple :

```
# Exemple d’entrée
P1 0 5 3 # nom | arrivée | durée | priorité
P2 2 4 1
P3 4 3 2
# Lignes vides autorisées
```



---



## Compilation et exécution

### Compilation
Le projet est compilé via un Makefile :

```bash
make
```

### Exécution
```bash
./ordonnanceur chemin/vers/processus.txt
```

###  Nettoyage
```bash
make clean
```


---



## Licence

Ce projet est distribué sous la licence MIT, une licence open-source permissive qui autorise l’utilisation, la modification, la distribution 
et la copie du code, y compris dans des projets commerciaux.

Voir fichier `LICENSE` pour plus d’informations.

---
