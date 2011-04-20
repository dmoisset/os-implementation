
Mecanismos de System calls
==========================

---

Resumen:
========

* Motivación/problema.
* Mecanismos de syscalls varios en ia32.
* Mecanismo en GeekOS
* Aspectos a tener en cuenta

---

La definición obvia
===================

System calls es un mecanismo a través del cual los programas de usuario
pueden obtener servicios del sistema operativo.

Keywords:

* Los programas de usuario pueden...
* Servicios...
* ...del sistema operativo.

¿Que diferencia tiene esto con una función pedestre?

---

Las partes no tan obvias
========================

* Los programas de usuario pueden...

Con lo cual el disparador del mecanismo tiene que funcionar en user-space

Las opciones:

* Usar instrucciones/operaciones no-privilegiadas
* Usar soporte especial del hardware

---
Las partes no tan obvias
========================

* Servicios

O sea, funciones.

Para tener funciones hace falta:

* Haber escrito las funciones en el kernel (fácil)
* Resolver como llegan los argumentos y vuelve el resultado (difícil)

---
Las partes no tan obvias
========================

* Del sistema operativo

El grueso de la funcionalidad va en el kernel (si esta todo bien diseñado, por
necesidad)

O sea que:

* En algún momento hay que cambiar a un contexto privilegiado
* Y además poder volver
 
---

No, nene, con `CALL` y `RET` no alcanza
=======================================

* Seguramente las funciones no están mapeadas en el address space
* Y si lo estuvieran, el proceso no sabría la dirección
* Y si lo supiera corre con los privilegios del proceso
    * Con lo cual no tiene acceso a las estructuras de datos del kernel (a menos que estemos haciendo algo _muy_ mal)

---

Y con `CALLF` y `RETF`?
=======================

Nos vamos acercando... idealmente algo como:

	!c
	CALLF KERNEL_CODE_SELECTOR:funcion
	/* KERNEL_CODE_SELECTOR es un selector al segmento de codigo del kernel,
	 * con el nivel de privilegio que corresponde
	 */

¿Pero que pasa si alguien hace trampa al llamar?

	!c
	int remove_file(char *filename) {
		if (!has_perms(user, filename))
			return ENOPERM;
		
		... /* codigo que borra un archivo */
		return 0;
	}

Por esto, ia32 solo permite far calls a selectores de segmento de igual
privilegio. Sino, fault

---

Intel (1982) al rescate
=======================

Call gates:

	!c
	CALLF SYSCALL_SELECTOR:xxx

* SYSCALL_SELECTOR es un selector, normalmente en la GDT de tipo `0xc`
* El offset `xxx` se ignora. Se usa en vez la base del descriptor
* El descriptor indica privilegio de origen y de destino
* Hace un context switch de `CS:EIP` (del gate) y de `SS:ESP` (del TSS)
* Transfiere datos de un stack a otro!

---

No los usa nadie
================

---

Intel (1997) al rescate
=======================

`SYSENTER` / `SYSEXIT`

* KISS (and fast)

Ejemplo:

	!c
	/* User-level */
	MOV edx, next
	MOV ecx, esp
	SYSENTER
	next: ...
	/* Kernel */
	sysenter:
	PUSH edx
	PUSH ecx
	...
	POP ecx
	POP edx
	SYSEXIT

`SYSCALL` / `SYSRETURN` (Lo mismo, marca AMD)

Usado en:

* Linux >= 2.5
* Windows >= XP

---

Así de simple:
==============

* Sin paso de argumentos
    * La gente usa registros
* Ni siquiera usa el stack para guardar estado!
* Requiere Pentium II
    * En realidad es anterior, pero funcionaba mal
* Sólo cambia de ring 0 a ring 3
* Usa 3 registros ad-hoc (MSR) para segmentos (los 4!), EIP y ESP para syscalls.
* Un solo punto de acceso
    * Se usa un argumento para seleccionar cual syscall

Lectura interesante:

* [Sysenter Based System Call Mechanism in Linux 2.6](http://articles.manugarg.com/systemcallinlinux2_6.html)

---

Interrupciones
==============

* Lo bueno
    * Son casi tan comunes como un load memoria a registro
    * Y el sistema operativo de todos modos se tiene que hacer cargo
* Lo malo: Son interrupciones, no para hacer syscalls
* Pero... tienen muchas cosas en común
    * Hay que saltar de privilegio
    * Y el diseñador de CPU tiene que haber pensado como asegurar protección
    * Y pueden volver a donde estaban
    * Y normalmente (ej: en ia32) se pueden disparar por software (`INT nn`)

---

Interrupciones (ia32, modo protegido)
=====================================

* Oooootra tabla de descriptores, la IDT
    * Y su registro correspondiente `idtr`
* Cada descriptor contiene un Interrupt Gate (muy parecido a un Call Gate!)
    * Sin paso de argumentos automático
* Las primeras 32 usadas para traps
* Unas cuantas mas para el hardware
* El resto (de las 256) disponibles para soft interrupts

Para syscalls:

	!c
	MOV reg, arg
	INT nn
	/* El resultado en algun registro */

* Hay que tener el privilegio correcto en la IDT
* Y ahora tenemos todos los registros! (si sos Linus, no importa)
* Pero en algunos micros, esto es leeeeeeentooooooooooooooooooo

---

Otro truco divertido: vsyscalls
===============================

Sé que esto se hace por lo menos en Linux:

* Toda la vuelta de rings, chequeos, paso de control es caro
* A veces todo lo que hace falta es dejar al proceso acceder a información,
  no correr código
* Se puede dejar la información en una página accesible al proceso:

Ejemplos

* `gettimeofday()`
* `getpid()`

---

En GeekOS
=========

En user space, `syscall.h`:

	!c
	#define SYSCALL "int $0x90"
	...
	/* Register conventions: 
	 * eax - system call number [input], return value [output] 
	 * ebx - first argument [input] 
	 * ecx - second argument [input] 
	 * edx - third argument [input] 
	 * esi - fourth argument [input] 
	 * edi - fifth argument [input] 
	 */

En user space, `libc.c`:

	!c
	DEF_SYSCALL(Exit,SYS_EXIT,int,(int exitCode), int arg0 = exitCode;, SYSCALL_REGS_1)

---

En GeekOS
=========

En kernel space:

 * `idt.c` setea la tabla, definida en `lowlevel.asm`
 * `trap.c` inicializa la entrada `0x90` a `Syscall_Handler()`
 * `Syscall_Handler()` busca en la tabla `g_syscallTable` la funcion que corresponde a `eax`
 * `g_syscallTable` se define estáticamente en `syscall.c`

Ustedes nomás escriben el cuerpo de las funciones:

	!c
	static int Sys_GetPID(struct Interrupt_State* state)
	{
		TODO("GetPID system call");
		return 0;
	}


---
Entonces al final, es una función!
==================================

---
Bueno, casi
===========

Se acuerdan que pasar parámetros era difícil?
 * Si son números, un registro alcanza
 * Y si son punteros?
 
	!c
	char buffer[100];
	int fd = open("filename", "rw");
	read(fd, buffer, 100);

* `"filename"` y `buffer` son direcciones de memoria
	* Relativas al `ds` _del proceso_
* Por lo tanto, la copia de los punteros no va a ser usable en kernel
	* Que funciona relativo a su propio `ds`

---
Pasando arreglos de un lado a otro
==================================

`Copy_From_User`

* Es lo que necesitaría una llamada como `open()` arriba
	* Hace por software lo mismo que el CPU hace en el otro contexto
	* Incluyendo chequeos de permisos/límites

`Copy_From_User`

* Es lo que necesitaría una llamada como `read()` arriba
	* Hace por software lo mismo que el CPU hace en el otro contexto
	* Incluyendo chequeos de permisos/límites

---
Chequeos importantes
====================

* Que el puntero este en un área del proceso
* Que el área tenga la cantidad de bytes necesarios
    * Hint: algo de esto puede ya estar
* Que el área sea legible/escribible
* Devolver el valor de retorno que corresponda a lo que chequearon
* El puntero en kernelSpace viene de mas código de Kernel, o sea que
debería ser "confiable". Un `KASSERT` nunca sobra
* Si es un string, NULL termination

No chequear esto es causa de bugs. [De verdad](http://butnotyet.tumblr.com/post/175132533/the-story-of-a-simple-and-dangerous-kernel-bug).

---
Y más!
======

No sólo al copiar datos puede haber problemas

* Los syscalls son la puerta de entrada desde user-space a kernel
* Misma paranoia que al escribir código networked
* Todo el input es "untrusted"

_(si este slide se ve mal, es por que no lo testee)_

---

Material de Lectura
===================

No más que el que les dio Nico
