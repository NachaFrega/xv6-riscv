# Informe Tarea 3 Ignacia Frega
El objetivo de esta tarea fue añadir las funciones `mprotect` y `munprotect` al sistema operativo xv6. Estas permiten gestionar los permisos de escritura en regiones específicas de memoria, habilitando un control de solo lectura en páginas seleccionadas y restaurando el acceso de escritura cuando sea necesario.

### Explicación de las modificaciones realizadas.
### En `kernel`
#### Archivo `proc.h`
Se realizaron las siguientes inclusiones y declaraciones:
```c
#include "spinlock.h"

int mprotect(void *addr, int len);
int munprotect(void *addr, int len);
```
Estas líneas definen las nuevas funciones encargadas de gestionar la protección de memoria.
#### Archivo `spinlock.h`
Se modificó para incluir guards y definir las funciones necesarias para manejar locks:
```c
#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "types.h"

// Mutual exclusion lock.
struct spinlock {
  uint locked;        // Indica si el lock está activo.

  // Para depuración:
  char *name;         // Nombre del lock.
  struct cpu *cpu;    // La CPU que tiene el lock actualmente.
};

// Declaraciones de funciones para manipular spinlocks
void initlock(struct spinlock *lock, char *name);
void acquire(struct spinlock *lock);
void release(struct spinlock *lock);
int holding(struct spinlock *lock);

#endif // SPINLOCK_H
```
#### Archivo `vm.c`
Se implementaron las funciones mprotect y munprotect, que manipulan los permisos de las páginas:
```c
int mprotect(void *addr, int len) {
    struct proc *p = myproc();  // Proceso actual
    uint64 start = PGROUNDDOWN((uint64)addr);  // Redondear dirección inicial
    uint64 end = start + len;

    // Verificación 1: Dirección o longitud inválida
    if (len <= 0 || start >= p->sz || end > p->sz) {
        return -1; // Error si la longitud es no positiva o si las direcciones no son válidas
    }

    for (uint64 a = start; a < end; a += PGSIZE) {
        pte_t *pte = walk(p->pagetable, a, 0); // Buscar la entrada PTE
        if (pte == 0) {
            return -1; // Error si la página no está mapeada
        }
        *pte &= ~PTE_W; // Desactivar el bit de escritura (solo lectura)
    }
    return 0;
}

int munprotect(void *addr, int len) {
    struct proc *p = myproc();  // Proceso actual
    uint64 start = PGROUNDDOWN((uint64)addr);  // Redondear dirección inicial
    uint64 end = start + len;

    // Verificación 1: Dirección o longitud inválida
    if (len <= 0 || start >= p->sz || end > p->sz) {
        return -1; // Error si la longitud es no positiva o si las direcciones no son válidas
    }

    for (uint64 a = start; a < end; a += PGSIZE) {
        pte_t *pte = walk(p->pagetable, a, 0); // Buscar la entrada PTE
        if (pte == 0) {
            return -1; // Error si la página no está mapeada
        }
        *pte |= PTE_W; // Activar el bit de escritura (lectura-escritura)
    }
    return 0;
}
```

### En `Makefile`
```makefile
UPROGS=\  
  $U/_test_prioridad\
  ...
```
Se modificó el `Makefile` para incluir el programa de prueba. Y se añadió una regla para compilarlo:
```makefile
$U/_test_prioridad: $U/test_prioridad.o $(ULIB)
	$(LD) $(LDFLAGS) -T $U/user.ld -o $U/_test_prioridad $U/test_prioridad.o $(ULIB)
	$(OBJDUMP) -S $U/_test_prioridad > $U/test_prioridad.asm
	$(OBJDUMP) -t $U/_test_prioridad | sed '1/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $U/test_prioridad.sym
```
### Modificación del `scheduler()`
La versión final del `scheduler` implementa la lógica completa de prioridad y boost:
```c
void 
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();  // Obtener la CPU actual

  c->proc = 0;

  for(;;){
    intr_on();  // Habilitar interrupciones

    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state != RUNNABLE) {
        release(&p->lock);
        continue;
      }

      // Ajustar la prioridad del proceso usando el boost
      p->priority += p->boost;

      // Verificar y ajustar los límites de la prioridad
      if(p->priority >= 9) {
        p->priority = 9;
        p->boost = -1;
      } else if(p->priority <= 0) {
        p->priority = 0;
        p->boost = 1;
      }

      // Ejecutar el proceso
      p->state = RUNNING;
      c->proc = p;
      swtch(&c->context, &p->context);

      c->proc = 0;
      release(&p->lock);
    }
  }
}
```
Esta implementación asegura que la prioridad se ajuste dinámicamente dentro del rango 0-9, usando el boost para regular su comportamiento.

## Ejecución de código

Al ejecutar por consola `test_prioridad.c`, se obtiene el siguiente resultado:
```
init: starting sh
$ test_prioridad
Ejecutando proceso 4
Ejecutando proceso 5
Ejecutando proceso 6
Ejecutando proceso 7
Ejecutando proceso 8
Ejecutando proceso 9
Ejecutando proceso 10
Ejecutando proceso 11
Ejecutando proceso 12
Ejecutando proceso 13
Ejecutando proceso 14
Ejecutando proceso 15
Ejecutando proceso 16
Ejecutando proceso 17
Ejecutando proceso 18
Ejecutando proceso 19
Ejecutando proceso 20
Ejecutando proceso 21
Ejecutando proceso 22
Ejecutando proceso 23
Todos los procesos han terminado.
```
Esto confirma la correcta ejecución del programa de prueba.

### Dificultades encontradas y soluciones implementadas.
El primer problema que se enfrento fue la confusión entre la versión estándar de xv6 y xv6-riscv. Muchas fuentes sugerían implementar el código usando la variable ptable, que no está presente en xv6-riscv, lo que ocasionó algunos contratiempos iniciales.
Después se tuvo el error "panic: kerneltrap". Al realizar una investigación más a fondo sobre la versión exacta de xv6-riscv me permitió entender las diferencias y resolver el problema, sin tener que seguir soluciones inaplicables a esta versión.
Una vez aclaradas las diferencias entre versiones, el progreso fue más fluido y los errores relacionados con ptable y sti() desaparecieron.

### Conclusión
En esta tarea aprendí la importancia de investigar adecuadamente la versión específica de un sistema operativo, ya que distintas versiones pueden tener diferencias significativas que afectan el desarrollo y la implementación del código.
