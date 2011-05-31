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


struct Semaphore g_Semaphore[MAX_NUM_SEMAPHORES];

int CreateSemaphore(char *name, int nameLenght, int initCount)
{
    int SID = 0;
    int ret = -1;

    bool atomic = Begin_Int_Atomic();
    SID = isSemaphoreCreated(name, nameLenght);
    Free(name);

    /* SID es negativo si no fue creado, de lo contrario
     * SID es el ID del semaforo ya creado
     */
    if (SID < 0) {
        /* Semaforo ya creado */
        g_Semaphore[SID].resourcesCount++;
        Set_Bit(g_currentThread->semaphorBitset, SID);
        ret = SID;
    }

    if (SID > 0) {
        /* Semaforo no creado. Crear */
        SID = getFreeSemaphore();
        if (SID < 0) 
            /* No hay semaforos disponibles nos vamos rapido */
            return -1;

        g_Semaphore[SID].available = false;
        g_Semaphore[SID].resourcesCount++;
        Set_Bit(g_currentThread->semaphorBitset, SID);
        ret = SID;
    }

    End_Int_Atomic(atomic);
    return ret;
}

int V(int SID)
{
    if (!validateSID(SID)) {
	    return EINVALID;
    }

    bool atomic = Begin_Int_Atomic();
    g_Semaphore[SID].resourcesCount++;
    Wake_Up(&g_Semaphore[SID].waitingThreads);
    End_Int_Atomic(atomic);
    return 0;
}

int P(int SID)
{
    KASSERT(g_Semaphore[SID].available == false);
    
    if (!validateSID(SID)) {
	    return EINVALID;
    }
    
    bool atomic = Begin_Int_Atomic();
    g_Semaphore[SID].resourcesCount--;
    Wait(&g_Semaphore[SID].waitingThreads);
    End_Int_Atomic(atomic);

    return 0;
}

int DestroySemaphore(int SID){

    if (!validateSID(SID)) {
	    return EINVALID;
    }

    bool atomic = Begin_Int_Atomic();
    g_Semaphore[SID].refCounter--;

    if (g_Semaphore[SID].refCounter == 0)
        g_Semaphore[SID].available = true;

    End_Int_Atomic(atomic);

    return 0;
}


/* --- Funciones auxiliares --- */
bool validateSID(int SID) {
    if (SID < 0 || 
        SID >= MAX_NUM_SEMAPHORES || 
        g_Semaphore[SID].available == false || 
        !Is_Bit_Set(g_currentThread->semaphorBitset, SID))
        
        return false;

    return true; 
}

int isSemaphoreCreated(char *namesem, int lenghtName)
{
    int SID = 0;
    int ret = -1;

    for (SID = 0; SID < MAX_NUM_SEMAPHORES; SID++) {
	    if (strncmp(g_Semaphore[SID].name, namesem, lenghtName) == 0) {
	        ret = SID;
	        break;
	    }
    }

    return ret;
}

int getFreeSemaphore()
{
    int SID = 0;
    int ret = -1;

    for (SID = 0; SID < MAX_NUM_SEMAPHORES; SID++) {
	    if (g_Semaphore[SID].available) {
	        ret = SID;
	        break;
	    }
    }

    return ret;
}
