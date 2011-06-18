#include <geekos/sem.h>
#include <geekos/ktypes.h>
#include <geekos/string.h>
#include <geekos/screen.h>
#include <geekos/malloc.h>
#include <geekos/kthread.h>
#include <geekos/user.h>
#include <geekos/errno.h>
#include <geekos/bitset.h>
#include <geekos/int.h>


/* Los semáforos */
struct Semaphore g_Semaphores[MAX_NUM_SEMAPHORES];

/* Funciones auxiliares */
static int isSemaphoreCreated(char *namesem, int nameLength);
static int getFreeSemaphore();
static bool validateSID(int sid);


/* Global initialization */
void Init_Semaphores(void)
{
    unsigned int sid = 0;
    for (sid = 0; sid < MAX_NUM_SEMAPHORES; ++sid) {
        g_Semaphores[sid].available = true;
        memset(g_Semaphores[sid].name, '\0', MAX_SEMAPHORE_NAME+1);
    }
}

int Create_Semaphore(char *name, int nameLength, int initCount)
{
    int sid = 0;
    int ret = -1;

    /* Syscall contract */
    KASSERT(name != NULL);
    KASSERT(0 < nameLength && nameLength <= MAX_SEMAPHORE_NAME);
    KASSERT(0 <= initCount);
    KASSERT(strnlen(name, MAX_SEMAPHORE_NAME) == nameLength);

    bool atomic = Begin_Int_Atomic();
    sid = isSemaphoreCreated(name, nameLength);
    KASSERT(sid < 0 || sid < MAX_NUM_SEMAPHORES);
    KASSERT(g_currentThread->semaphores != NULL);

    /* Negative sid for non existing, otherwise sid is Semaphore ID */
    if (0 <= sid) { /* Already created semaphore */
        g_Semaphores[sid].references++;
        Set_Bit(g_currentThread->semaphores, sid);
        ret = sid;
    } else { /* Semaphore not created. Create. */
        sid = getFreeSemaphore();
        if (sid < 0) { /* No semaphores available */
            ret = EUNSPECIFIED;
        } else {
            strncpy(g_Semaphores[sid].name, name, MAX_SEMAPHORE_NAME);
            g_Semaphores[sid].resources = initCount;
            g_Semaphores[sid].available = false;
            g_Semaphores[sid].references = 1;
            Clear_Thread_Queue(&g_Semaphores[sid].waitingThreads);
            Set_Bit(g_currentThread->semaphores, sid);
            ret = sid;
        }
    }
    End_Int_Atomic(atomic);

    return ret;
}

/* Probeer te verlagen */
int P(int sid)
{
    if (!validateSID(sid)) {
        return EINVALID;
    }

    bool atomic = Begin_Int_Atomic();
    if (g_Semaphores[sid].resources == 0) {
        Wait(&g_Semaphores[sid].waitingThreads);
        KASSERT(g_Semaphores[sid].resources == 1);
    }
    KASSERT(0 < g_Semaphores[sid].resources);
    g_Semaphores[sid].resources--;
    End_Int_Atomic(atomic);

    return 0;
}

/* Verhogen */
int V(int sid)
{
    if (!validateSID(sid)) {
        return EINVALID;
    }

    bool atomic = Begin_Int_Atomic();
    g_Semaphores[sid].resources++;
    if (g_Semaphores[sid].resources == 1) {
        /* from 0 to 1 we might wake up one */
        Wake_Up_One(&g_Semaphores[sid].waitingThreads);
    }
    End_Int_Atomic(atomic);
    return 0;
}

int Destroy_Semaphore(int sid)
{
    if (!validateSID(sid)) {
        return EINVALID;
    }

    bool atomic = Begin_Int_Atomic();
    KASSERT(0 < g_Semaphores[sid].references);
    g_Semaphores[sid].references--;
    Clear_Bit(g_currentThread->semaphores, sid);

    if (g_Semaphores[sid].references == 0) {
        g_Semaphores[sid].available = true; /* mark it available */
        Wake_Up(&g_Semaphores[sid].waitingThreads); /* wake all threads */
    }

    End_Int_Atomic(atomic);
    return 0;
}


/* Auxiliary functions */

static bool validateSID(int sid) {
    KASSERT(g_currentThread->semaphores != NULL);
    return (0 <= sid
            && sid < MAX_NUM_SEMAPHORES
            && !g_Semaphores[sid].available
            && Is_Bit_Set(g_currentThread->semaphores, sid));
}

static int isSemaphoreCreated(char *namesem, int lengthName)
{
    int sid = 0;
    int ret = -1;

    for (sid = 0; sid < MAX_NUM_SEMAPHORES; sid++) {
        if (strncmp(g_Semaphores[sid].name, namesem, lengthName) == 0) {
            ret = sid;
            break;
        }
    }

    return ret;
}

static int getFreeSemaphore()
{
    int sid = 0;
    int ret = -1;

    for (sid = 0; sid < MAX_NUM_SEMAPHORES; sid++) {
        if (g_Semaphores[sid].available) {
            ret = sid;
            break;
        }
    }

    return ret;
}