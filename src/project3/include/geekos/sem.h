#include <geekos/ktypes.h>
#include <geekos/syscall.h>
#include <geekos/kthread.h>
#include <geekos/list.h>

#define MAX_NUM_SEMAPHORES  20
#define MAX_SEMAPHORES_NAME 25

struct Semaphore {
    char name[MAX_SEMAPHORES_NAME];
    unsigned int resourcesCount;
    bool available;
    unsigned int refCounter;
    struct Thread_Queue waitingThreads;
};

extern struct Semaphore g_Semaphores[MAX_NUM_SEMAPHORES];

extern int CreateSemaphore(char *name, int nameLenght, int ival);
int P(int sem);
int V(int sem);
int DestroySemaphore(int sem);

int isSemaphoreCreated(char *namesem, int nameLenght);
int getFreeSemaphore();
bool hasAccess(int SID);
bool validateSID(int SID);
