Proyecto 3: planificación y sincronización
==========================================

---


Este proyecto
=============

1. nueva política de planificación
2. nueva primitiva de sincronización
3. mecanismo para obtener la hora del sistema
4. comparación de políticas de planificación


Tiempo: 2 semanas


---


Implementación actual
=====================

GeekOS implementa Round-Robin con una única cola

Todos los procesos son asignados turnos secuencialmente

Cada proceso recibe un turno de tamaño fijo


---

Planificador MLFQ
=================

Planificador multinivel con retroalimentación (Multi-Level Feedback Queue)

Descripto por Corbato et al. en 1962 (CTSS)

Ajusta la prioridad de los procesos de acuerdo a su comportamiento

Procesos con mucho I/O son más prioritarios

[Explicación completa de MLFQ](http://pages.cs.wisc.edu/~remzi/OSFEP/cpu-sched-mlfq.pdf "Scheduling: The Multi-Level Feedback Queue")


---


Algoritmo
=========

* 4 colas (0: máxima prioridad - 3: mínima prioridad)
* todo nuevo thread comienza con máxima prioridad
* para planificar un proceso, se toma la cabeza de la cola con máxima prioridad
* si está vacía, pasar a la siguiente (repetir hasta encontrar un proceso)
* si un proceso termina su _quantum_, pasa a la próxima cola con menos prioridad
	* si llega a la cola 3 no puede bajar más
* si un proceso se bloquea, pasa a la próxima cola con más prioridad
	* si llega a la cola 0 no puede subir más
* caso especial: Idle (prioridad 3 siempre!)


---


Cambio de política
==================

Implementar la _system call_ Sys_SetSchedulingPolicy

* política de planificación (state->ebx)
	* 0 => round-robin
	* 1 => mlfq
	* otros valores => error
* _quantum_ en número de ticks (state->ecx)
	* cambiar constante MAX_TICKS por variable (timer.c)
	* valores permitidos [2, 100]


---


Interpretación gráfica
======================


[Posibles escenarios](http://www.cs.umd.edu/class/spring2005/cmsc412/proj3/scenarios.pdf "escenarios")


---


Sincronización
==============

Implementaremos semáforos (src/geekos/syscall.c)

4 nuevas _system calls_:

* Sys_CreateSemaphore
* Sys_P
* Sys_V
* Sys_DestroySemaphore

Sugerencia: mover la implementación de semáforos a nuevos archivos (sem.c y sem.h)

---


Sys_CreateSemaphore
===================

Crea un semáforo

state->ebx: nombre del semáforo

state->ecx: longitud del nombre

cantidad de semáforos >= 20

nombres hasta 25 caracteres

Si el nombre es nuevo, crea un semáforo y lo agrega a la lista de semáforos.

Si el nombre ya existe, devuelve el semáforo existente.

---


Sys_P
=====

Obtiene el semáforo.

Espera a que el valor del semáforo sea mayor a 0, lo decrementa en 1 y luego continúa.

Puede bloquear el proceso.


---


Sys_V
=====

Libera el semáforo

Incrementa el valor del semáforo en 1 y continúa.

Nunca bloquea.


---


Sys_DestroySemaphore
====================

Elimina el semáforo

Si no hay más referencias al semáforo, lo quita de la lista.

Debe ser llamada automáticamente si un proceso termina y no destruyó sus semáforos.


---

Manejo de listas
================

Implementación _genérica_ de listas (include/geekos/list.h)


---


Sys_GetTimeOfDay: Hora del sistema
==================================


return g_numTicks

Podemos usarlo para calcular la duración de un proceso.


---


Comparemos políticas de planificación
=====================================

Probemos los límites de nuestras políticas:

% /c/workload.exe rr 2

% /c/workload.exe rr 100

% /c/workload.exe mlf 2

% /c/workload.exe mlf 100


---

Otras pruebas
=============

Hacer programas que hagan I/O-bound y CPU-bound que calculen _wall time_.

Comparar el wall time bajo distintas políticas y distintos quantum.


---


Nota
====

Modificar shell.c para que acepte el operador &
