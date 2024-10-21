#include "../headers/custom_rwlock.h"

/* Struct for list nodes */
struct list_node_s {
    int    data;
    struct list_node_s* next;
};

/* Setup and cleanup */
void        Usage(char* prog_name);
void        Get_input(int* inserts_in_main_p);

/* Thread function */
void*       Thread_work(void* rank);
void*       Custom_Thread_work(void* rank);

/* List operations */
int         Insert(int value);
void        Print(void);
int         Member(int value);
int         Delete(int value);
void        Free_list(void);
int         Is_empty(void);

int readWriteLock(int argc, char* argv[]);