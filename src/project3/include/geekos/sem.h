#include <geekos/ktypes.h>
#include <geekos/syscall.h>
#include <geekos/kthread.h>
#include <geekos/list.h>

#define MAX_NUM_SEMAPHORES 20
#define MAX_SEMAPHORE_NAME 25

struct Semaphore {
    char name[MAX_SEMAPHORE_NAME+1]; /* '\0' terminated */
    unsigned int resources;
    bool available;
    unsigned int references;
    struct Thread_Queue waitingThreads;
};

extern struct Semaphore g_Semaphores[MAX_NUM_SEMAPHORES];

void Init_Semaphores(void);
extern int Create_Semaphore(char *name, int nameLenght, int ival);
int P(int sem);
int V(int sem);
int Destroy_Semaphore(int sem);
