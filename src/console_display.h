
#ifndef CONSOLE_DISPLAY_H
#define CONSOLE_DISPLAY_H

#include "process.h"


typedef struct {
    Process *procs;           
    int n;                    
    int *timeline;            
    int timeline_len;         
    int *start;               
    int *end;                 
    int *init_prio;          
    int *levels;             
    int total_time;           
} SimulationResult;


void display_console_results(
    const char *algo_name,
    int quantum,
    SimulationResult *result,
    const char *rules
);

#endif 
