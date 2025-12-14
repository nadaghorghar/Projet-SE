#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "process.h"
#include "gui.h"
#include "console_display.h"


static void sort_by_arrival(Process procs[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (procs[j].arrival < procs[i].arrival) {
                Process tmp = procs[i];
                procs[i] = procs[j];
                procs[j] = tmp;
            }
        }
    }
}


static SimulationResult simulate_fifo(Process procs[], int n) {
    SimulationResult result;
    
    
    static int timeline[1000];
    static int start[100];
    static int end[100];
    
    result.procs = procs;
    result.n = n;
    result.timeline = timeline;
    result.start = start;
    result.end = end;
    result.init_prio = NULL;
    result.levels = NULL;
    result.total_time = 0;
    
   
    sort_by_arrival(procs, n);
    
    
    int time = 0;
    int timeline_len = 0;
    
    for (int i = 0; i < n; i++) {
      
        while (time < procs[i].arrival) {
            timeline[timeline_len++] = -1;
            time++;
        }
        
        
        start[i] = time;
        for (int d = 0; d < procs[i].duration; d++) {
            timeline[timeline_len++] = i;
            time++;
        }
        end[i] = time;
    }
    
    result.timeline_len = timeline_len;
    result.total_time = time;
    
    return result;
}


static void copy_to_gui_result(SimulationResult *result) {
    if (!current_result) return;
    
    strcpy(current_result->algo_name, "FIFO");
    current_result->quantum = 0;
    current_result->process_count = result->n;
    
    
    current_result->processes = malloc(sizeof(Process) * result->n);
    if (!current_result->processes) {
        fprintf(stderr, "Erreur: malloc échoué pour processes\n");
        return;
    }
    
    for (int i = 0; i < result->n; i++) {
        current_result->processes[i] = result->procs[i];
    }
    
    
    current_result->timeline_len = result->timeline_len;
    for (int i = 0; i < result->timeline_len; i++) {
        current_result->timeline[i] = result->timeline[i];
    }
    
    
    for (int i = 0; i < result->n; i++) {
        current_result->start[i] = result->start[i];
        current_result->end[i] = result->end[i];
    }
    
   
    float sum_turnaround = 0.0f, sum_wait = 0.0f;
    for (int i = 0; i < result->n; i++) {
        current_result->turnaround[i] = result->end[i] - result->procs[i].arrival;
        current_result->wait[i] = result->start[i] - result->procs[i].arrival;
        sum_turnaround += current_result->turnaround[i];
        sum_wait += current_result->wait[i];
    }
    
    current_result->avg_turnaround = sum_turnaround / result->n;
    current_result->avg_wait = sum_wait / result->n;
}


void fifo(Process procs[], int n) {
    
    SimulationResult result = simulate_fifo(procs, n);
    
    
    if (capture_mode && current_result) {
        
        copy_to_gui_result(&result);
    } else {
        
        display_console_results("ORDONNANCEMENT FIFO", 0, &result, NULL);
    }
}
