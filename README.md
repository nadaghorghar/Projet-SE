# Linux Multitasking Process Scheduler

This project is a CPU scheduling simulator developed in C on a Linux system. It loads a list of processes defined in a configuration file and simulates their execution according to different scheduling policies.

The simulator is built on a modular architecture, where each scheduling policy is implemented in a separate module, making it easy to extend and add new strategies.


---



## Main Features

The goal of this project is to develop a CPU scheduling simulator in C capable of:

- Loading a set of processes from a configuration file.
- Applying multiple scheduling policies.
- Displaying simulation results (text-based output and graphical visualization).
- Dynamically loading available scheduling policies from a dedicated directory.
- Automatically generating the executable using a **Makefile**.


---



##  Project Structure

```
.
├── src/            # Main C source code of the simulator
├── politiques/     # Scheduling policies (separate .c files)
├── processus.txt   # Process configuration file
├── Makefile        # Project build and compilation script
├── LICENSE         # Project license (MIT)
├── README.md       # Project documentation and usage guide
└── docs/           # Documentation (SCRUM + user guide)
```



---



## Technologies Used

The project uses the following technologies and concepts:

- **Langage : C**  
  The entire simulator is written in C, offering efficient low-level control over memory and process workflow.

- **Platform : Linux**  
  The project is designed to be compiled and executed on Linux. All scripts and the Makefile rely on Linux commands.

- **Core Concepts and Tools: :**
  - **Data Structures** : To store and manage process information. 
  - **Makefile** : To automate compilation and execution.
  - **Modular Architecture** : To allow easy integration of new policies or components.


---



## Implemented Scheduling Policies

- FIFO (First In First Out)
- Round Robin
- Preemptive Priority
- Multi-level Queue with Static Priority
- Multi-level Queue with Dynamic Priority (Aging)

Each policy is implemented as a separate C file located in a specific directory (politiques/).

---


## General Operation

The program reads a text file describing a list of processes. Each process entry includes:
- **Process name**  
- **Arrival time**  
- **CPU burst duration**  
- **Static priority**
- Support for **comments** and **empty lines**

The file is passed as a command-line argument:

```bash
./ordonnanceur processus.txt
```

The program then displays a menu allowing the user to dynamically select a scheduling policy.


---



##  Process File Format

Example :

```
# Sample input
P1 0 5 3   # name | arrival | burst | priority
P2 2 4 1
P3 4 3 2
# Empty lines are allowed
# Comments are allowed
```


---



## Compilation and Execution

### Compile
Le projet est compilé via un Makefile :

```bash
make
```

### Run
```bash
./ordonnanceur chemin/vers/processus.txt
```

###  Clean Build Files
```bash
make clean
```



## License

This project is distributed under the MIT License, a permissive open-source license that allows use, modification, distribution, and integration in commercial software.

See the `LICENSE` file for more details.
