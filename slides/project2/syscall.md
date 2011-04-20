
Syscalls
========

---

Resumen:
========

* a
* b
* c

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
------------------

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


Entonces leemos los registros que tienen los parámetros así de fácil:

	!c
	static int Sys_Wait(struct Interrupt_State* state)
	{
		int pid = state->ebx;
		...
	}

---
