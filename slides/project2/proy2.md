
Projecto 2: procesos de usuario
===============================

---

Resumen:
========

* Motivación
* Por arriba
* Que hay que completar
* Como seguimos (entrega intermedia, 2 clases más)
* Material de Lectura

---

Motivación: ¿Qué necesito para tener procesos de usuario?
=============================================

---

Necesito
--------

* **Protección** (kernel vs. user).
* Definir un **layout** para la memoria de usuario y poner allí los pedazos del ELF.
* Aumentar los `kthread` con **información de procesos de usuarios**.
* Definir la interface de inicio del proceso: **`int main(argc,argv)`**.
* Definir la interface de comunicación del proceso con el kernel: **syscalls**.
* **Lanzar** el proceso.
* **Cambiar el mapa de memoria** cuando cambia el ctxt a un proceso de usuario.

---

Por arriba
==========

---

Protección
==========

* Estamos en modo protegido, luego hay segmentación.
* Diferenciar **Logical** address vs. **Linear** address.
    * process vs. kernel memory.
* Definir una `LDT` con los segmentos básicos para correr el proceso usuario.
    * Segmento de datos, segmento de código.

---

Layout de memoria
=================

Hay que usar el ELF y `argc, argv` para definir el layout

* Definir tamaño de la imagen de memoria.
* Copiar los segmentos ELF.
* Copiar `argc, argv` en algún lugar de la memoria.
* Dejar espacio para el stack.
* ¿Y el heap? ¿Cuánta memoria dejamos para alocación dinámica? (`brk()`)

---

Contexto de Usuario
===================

El hilo de kernel que manejará el proceso de usuario tiene que contener información extra.

* Memory layout.
    * LDT
    * Selectores
* Tamaño de la imagen de memoria.
* Punto de entrada.
* Ubicación de `argc, argv`.

Aumentar `struct Kernel_Thread` con el contexto de usuario: `struct User_Context`


* Create_User_Context
* Destroy_User_Context

---

Lanzar el proceso
=================

Dejar todo como si lo hubieran interrumpido en el punto de entrada y encolarlo.

---

Cambiar de contexto
===================

Cambiar el memory layout, o sea la segmenación.

---

Que hay que completar
=====================

---

`Spawn()`
=========

Crea un proceso de usuario con la imagen de un ELF cargando del FS.

* `Read_Fully` (done)
* `Parse_ELF_Executable` (proj1)
* `userseg.c:Load_User_Program` (completar)
    * Completa `struct Kernel_Thread` con el contexto de usuario: `struct User_Context`.
    * Crea la memoria del proceso.
    * Establece la segmentación.
    * Copia los segmentos ELF a la memoria.
    * Copia los argumentos a la memoria.
* `Start_User_Thread` (completar)

---
Recuerden
=========

* Hasta que no completen todos los `TODO()`, no van a tener **nada** funcionando.
* Hay que agregar pocas líneas (100?), asi que programen de manera muy cuidadosa.
* Debuggear es penoso:
    * Escriban código sencillo.
    * Usen `KASSERT` para todas las condiciones que están suponiendo.
    * De última `Print()` puede ser de utilidad.

---

Como seguimos
===================

---

Planes para las 3 semanas
=========================

* El proyecto se divide en dos partes:
    * Hacer funcionar `/c/null.exe` (2 semanas).
    * Implementar el resto de los syscalls para que funcione `/c/shell.exe` (1 semana).
* Clase 2: **ia32 segmentation**.
* Clase 3: **syscall ifce**.

---

Material de Lectura
===================

---

Imprescindibles
===================

* La "GeekOS Hacking Guide" (aunque no dice mucho).
* Proyecto de [CMSC 412](http://www.cs.umd.edu/class/spring2005/cmsc412/proj2/) del 2005.
* Las [filminas](http://www.cs.umd.edu/class/spring2005/cmsc412/proj2/proj2.ppt) con más información.
* Intel® 64 and IA-32 Architectures Software Developer's Manual Volume 3A: System Programming Guide, [Part 1, Chapter 3](http://www.intel.com/Assets/PDF/manual/253668.pdf).

