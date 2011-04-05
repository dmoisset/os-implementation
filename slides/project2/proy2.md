
Projecto 2: procesos de usuario
===============================

---

Resumen:
========

* Motivación
* Por arriba
* Funciones a completar
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
* Definir la interface de inicio del proceso: **`int main(argc,argv)`**.
* Definir la interface de comunicación del proceso con el kernel: **syscalls**.
* **Lanzar** el proceso.
* Aumentar los `kthread` con **información de procesos de usuarios**.
* **Cambiar el mapa de memoria** cuando cambia el ctxt a un proceso de usuario.

---

Funciones para completar
===================

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
* Clase 2 **ia32 segmentation**.
* Clase 3 **syscall ifce**.

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

