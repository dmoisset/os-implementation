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
static struct Semaphore g_Semaphore[MAX_NUM_SEMAPHORES];

/* Funciones auxiliares */
static int isSemaphoreCreated(char *namesem, int nameLength);
static int getFreeSemaphore();
/* static bool hasAccess(int sid); */
static bool validateSID(int sid);


int CreateSemaphore(char *name, int nameLength, int initCount)
{
    int sid = 0;
    int ret = -1;

    /* Paranoico, soy entrypoint de userspace */
    if (name==NULL || nameLength<=0 || initCount<0 ||
        MAX_SEMAPHORE_NAME<nameLength ||
        strnlen(name, MAX_SEMAPHORE_NAME)!=nameLength) {
            return EINVALID;
    }

    bool atomic = Begin_Int_Atomic();
    sid = isSemaphoreCreated(name, nameLength);
    Free(name);

    /* sid es negativo si no fue creado, de lo contrario
     * sid es el ID del semaforo ya creado
     */
    if (sid < 0) {
        /* Semaforo ya creado */
        g_Semaphore[sid].resourcesCount++;
        Set_Bit(g_currentThread->semaphores, sid);
        ret = sid;
    }

    if (sid > 0) {
        /* Semaforo no creado. Crear */
        sid = getFreeSemaphore();
        if (sid < 0) 
            /* No hay semaforos disponibles nos vamos rapido */
            return -1;

        g_Semaphore[sid].available = false;
        g_Semaphore[sid].resourcesCount++;
        Set_Bit(g_currentThread->semaphores, sid);
        ret = sid;
    }

    End_Int_Atomic(atomic);
    return ret;
}

int V(int sid)
{
    if (!validateSID(sid)) {
        return EINVALID;
    }

    bool atomic = Begin_Int_Atomic();
    g_Semaphore[sid].resourcesCount++;
    Wake_Up(&g_Semaphore[sid].waitingThreads);
    End_Int_Atomic(atomic);
    return 0;
}

int P(int sid)
{
    KASSERT(g_Semaphore[sid].available == false);

    if (!validateSID(sid)) {
        return EINVALID;
    }

    bool atomic = Begin_Int_Atomic();
    g_Semaphore[sid].resourcesCount--;
    Wait(&g_Semaphore[sid].waitingThreads);
    End_Int_Atomic(atomic);

    return 0;
}

int DestroySemaphore(int sid){

    if (!validateSID(sid)) {
        return EINVALID;
    }

    bool atomic = Begin_Int_Atomic();
    g_Semaphore[sid].refCounter--;

    if (g_Semaphore[sid].refCounter == 0)
        g_Semaphore[sid].available = true;

    End_Int_Atomic(atomic);

    return 0;
}


/* --- Funciones auxiliares --- */

static bool validateSID(int sid) {
    if (sid < 0 ||
        sid >= MAX_NUM_SEMAPHORES ||
        !g_Semaphore[sid].available ||
        !Is_Bit_Set(g_currentThread->semaphores, sid))

        return false;

    return true; 
}

static int isSemaphoreCreated(char *namesem, int lengthName)
{
    int sid = 0;
    int ret = -1;

    for (sid = 0; sid < MAX_NUM_SEMAPHORES; sid++) {
        if (strncmp(g_Semaphore[sid].name, namesem, lengthName) == 0) {
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
        if (g_Semaphore[sid].available) {
            ret = sid;
            break;
        }
    }

    return ret;
}
