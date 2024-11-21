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

#### Archivo `syscall.h`
Se agregaron los códigos de sistema para las nuevas llamadas:
```c
#define SYS_mprotect  22
#define SYS_munprotect  23
```

#### Archivo `syscall.c`
Se añadieron las declaraciones y entradas en el array de llamadas al sistema:
```c
uint64 sys_mprotect(void);
uint64 sys_munprotect(void);

[SYS_mprotect]  sys_mprotect,
[SYS_munprotect] sys_munprotect,
```

#### Archivo `sysproc.c`
Se implementaron las funciones de sistema:
```c
uint64 sys_mprotect(void) {
    uint64 addr;
    int len;
    argaddr(0, &addr);
    argint(1, &len);
    return mprotect((void *)addr, len);
}

uint64 sys_munprotect(void) {
    uint64 addr;
    int len;
    argaddr(0, &addr);
    argint(1, &len);
    return munprotect((void *)addr, len);
}
```

### En `User`

#### Archivo `usys.pl`
Se añadieron las entradas:
```plaintext
entry("mprotect");  
entry("munprotect");
```

#### Archivo `user.h`
Se declararon las funciones para el espacio de usuario:
```c
int mprotect(void *addr, int len);
int munprotect(void *addr, int len);
```

## Código de Prueba
Se desarrolló un programa para verificar las funciones:

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {
    // Alocar memoria con sbrk
    char *mem = sbrk(4096);  // Alocar una página

    // Verificar si la memoria es escribible
    printf("Escribiendo en la memoria antes de mprotect...\n");
    mem[0] = 'A';
    printf("Escribió correctamente en memoria antes de mprotect: %c\n", mem[0]);

    // Llamar a mprotect para hacer la página de solo lectura
    if (mprotect(mem, 4096) < 0) {
        printf("Error: mprotect falló\n");
        exit(1);
    }

    // Intentar escribir en la memoria protegida (esto debería fallar)
    printf("Intentando escribir en la memoria después de mprotect (debería fallar)...\n");
    if (fork() == 0) {  // Hacemos un fork para que el proceso hijo falle y no afecte al padre
        mem[0] = 'B';  // Esto debería causar una falla de segmentación
        printf("Error: se escribió en memoria protegida (esto no debería imprimirse)\n");
        exit(1);
    } else {
        wait(0);  // Espera al proceso hijo
    }

    // Llamar a munprotect para restaurar el permiso de escritura
    if (munprotect(mem, 4096) < 0) {
        printf("Error: munprotect falló\n");
        exit(1);
    }

    // Intentar escribir en la memoria nuevamente después de munprotect
    printf("Intentando escribir en la memoria después de munprotect...\n");
    mem[0] = 'C';
    printf("Escribió correctamente en memoria después de munprotect: %c\n", mem[0]);

    printf("Prueba completada.\n");
    exit(0);
}
```

Este programa reserva una página de memoria y la configura como de solo lectura utilizando mprotect, comprobando que cualquier intento de escritura genera un error. Posteriormente, emplea munprotect para restaurar los permisos de escritura y verifica que la memoria vuelve a ser modificable.

## Resultados de la Ejecución

Al ejecutar el código de prueba `test_protect`, se ovtuvo lo siguiente:

```plaintext
Escribiendo en la memoria antes de mprotect...
Escribió correctamente en memoria antes de mprotect: A
Intentando escribir en la memoria después de mprotect (debería fallar)...
usertrap(): unexpected scause 0xf pid=4
            sepc=0x58 stval=0x4000
Intentando escribir en la memoria después de munprotect...
Escribió correctamente en memoria después de munprotect: C
Prueba completada.
```

### Dificultades encontradas y soluciones implementadas.
La principal dificultad encontrada es que surgió un inconveniente relacionado con spinlock.h, que ocasionaba errores de compilación. La solución consistió en incluir explícitamente spinlock.h en otros archivos del kernel, como proc.h, para garantizar que la estructura struct spinlock estuviera correctamente definida antes de ser utilizada en diversas partes del sistema.

### Conclusión
En esta tarea aprendí la implementación correcta de las funciones mprotect y munprotect, aprendiendo sobre la gestión de permisos en tablas de páginas. A pesar de los problemas iniciales, las soluciones aplicadas fueron exitosas y permitieron afianzar conocimientos sobre sistemas operativos.
