
Syscalls
========

---

Resumen:
========

* Generalidades.
* ¿Que hay que completar?

---

Generalidades
=============

---

¿Qué es un syscall?
===================

Simplemente una **software interrupt**.

---

Asi deja el stack una interrupción
=======================================

![Stack after Interrupt](Figure6-4.jpg)

---

Veamos el prototipo de un syscall
=================================

	!c
	static int Sys_GetPID(struct Interrupt_State* state)

Un `struct Interrupt_State` es exactamente lo mismo que se deja para cualquier interrupción (software o hardware).

En `include/int.h`

	!c
	struct Interrupt_State {
		/*
		 * The register contents at the time of the exception.
		 * We save these explicitly.
		 */
		uint_t gs;
		uint_t fs;
		uint_t es;
		uint_t ds;
		uint_t ebp;
		uint_t edi;
		uint_t esi;
		uint_t edx;
		uint_t ecx;
		uint_t ebx;
		uint_t eax;

		/*
		 * We explicitly push the interrupt number.
		 * This makes it easy for the handler function to determine
		 * which interrupt occurred.
		 */
		uint_t intNum;

---

	!c
		/*
		 * This may be pushed by the processor; if not, we push
		 * a dummy error code, so the stack layout is the same
		 * for every type of interrupt.
		 */
		uint_t errorCode;

		/* These are always pushed on the stack by the processor. */
		uint_t eip;
		uint_t cs;
		uint_t eflags;
	};


Entonces leemos los registros que tienen los parámetros y devolvemos resultados así de **fácil**:

	!c
	static int Sys_Wait(struct Interrupt_State* state)
	{
		int pid = state->ebx;
		...
	}

	static int Sys_Spawn(struct Interrupt_State* state)
	{
		...
		return pid;
	}

---

`Copy_From_User`/`Copy_To_User`
===============================

Se necesita más:

* Que saber que `pid` nos dieron (`Spawn`).
* Que pasar un `pid` para esperar (`Wait`).
* Que pasarle las coordenadas para mover el cursor (`PutCursor`).

Para bajar y subir cosas desde y hacia userspace (ptrs):

	!c
	bool Copy_From_User(void* destInKernel, ulong_t srcInUser, ulong_t bufSize)
	
	Copy_To_User(ulong_t destInUser, void* srcInKernel, ulong_t bufSize)

---

¿Que hay que completar?
=======================

---

Listado de SysCalls en grupos
=============================

Solo 10 syscalls, que a medida que avancen los proyectos serán más.

Triviales
---------

* `Null`, `GetKey`, `SetAttr`, `PutCursor`, `GetPID`.

Intermedias
-----------

* `Exit`, `PrintString`, `GetCursor`, `Wait`.

Complicadas
-----------

* `Spawn`.

Además de `Copy_From_User`/`Copy_To_User`.

De nuevo las [filminas](http://www.cs.umd.edu/class/spring2005/cmsc412/proj2/proj2.ppt) tienen información extra de ayuda.

---

Finalmente
==========

* Probar `src/user/shell.c`

* Hacer más userspace programas para probar cuestiones como:
	* Ver si la interface `argc,argv` y exit code funcionan.
	* Fortaleza de los syscalls, implementando un [*stress testing*](http://people.freebsd.org/~pho/linuxforum06/linuxforum06.pdf)
		¿Armar un stress test para GeekOS?

