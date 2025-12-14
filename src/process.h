#ifndef PROCESS_H
#define PROCESS_H

#define MAXP 50

typedef struct {
    char name[100];
    int arrival;
    int duration;
    int priority;
    int remaining_time;
} Process;


int read_processes_from_file(const char *filename, Process procs[], int *count);

#endif
