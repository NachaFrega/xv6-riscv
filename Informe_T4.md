# Informe Tarea 4 Ignacia Frega
El objetivo de esta tarea es implementar un sistema de permisos básicos en xv6 (RISC) que permita gestionar el acceso a los archivos de manera más controlada. Esto incluye habilitar permisos de solo lectura o lectura/escritura, además de introducir un permiso especial para marcar archivos como inmutables, de forma que no puedan ser modificados ni eliminados.

## Explicación de las modificaciones realizadas.

Antes de proceder con la implementación de la solución, se realizaron modificaciones en varios archivos para corregir errores y adecuar el entorno de desarrollo.

### En `riscv.h`**
```c
#ifndef RISCV_H
#define RISCV_H
```
Al final se agregó:
```c
#endif // RISCV_H
```
Esto para que las declaraciones se incluyan una sola vez.

### En `trampoline.S`**
Se modificó:
```asm
li a0, TRAPFRAME
```
Por:
```asm
li a0, 0x3FFFFFE000
```
Esto para ajustar la dirección del marco de interrupción y que coincida con el nuevo diseño de la memoria.

### En `memlayout.h`**
Se modificó:
```c
#define MAXVA 0x4000000000L
#define TRAMPOLINE 0x3FFFFFF000L
#define TRAPFRAME 0x3FFFFFE000L
```
Esto para un nuevo diseño del mapeo de memoria virtual para mejorar la compatibilidad y la organización.

### En `spinlock.h`**
Se implementó una protección para evitar múltiples inclusiones:
```c
#ifndef SPINLOCK_H
#define SPINLOCK_H
// Código de spinlock
#endif // SPINLOCK_H
```
Esto para asegurar una implementación correcta y evitar errores de redefinición.

### También se modificó:
- `defs.h`: Se añadió `#include "riscv.h"`.
- `proc.h`: Se añadieron:
  ```c
  #include "riscv.h"
  #include "spinlock.h"
  ```
- `sleeplock.h`: Se añadió `#include "spinlock.h"`.
- `stat.h`: Se añadió `#include "types.h"`.

Esto para que las dependencias estén correctamente resueltas.

Esos pasos se realizaron debido a los errores reportados en la consola durante la implementación, estos son los archivos principales que garantizaron que la primera parte se completara con éxito.

### A la estructura `inode` se le modificó:
En `file.h`, se añadió el campo `perm` para almacenar los permisos:
```c
struct inode {
    ...
    int perm; // 0 = no acceso, 1 = lectura, 2 = escritura, 3 = lectura/escritura
};
```

### Se hizo cambios en la asignación de inodos:
En `fs.c`, se modificó la función `ialloc` para inicializar los permisos:
```c
ip->perm = 3; // Permiso predeterminado: lectura/escritura
```

### A syscalls se le hizo las siguientes modificaciones:
En `sysfile.c`, se añadieron verificaciones de permisos en las funciones:
- `sys_read`:
  ```c
  if (f->ip && (f->ip->perm & 1) == 0) {
      return -1; // Sin permiso de lectura
  }
  ```
- `sys_write`:
  ```c
  if (f->ip && (f->ip->perm & 2) == 0) {
      return -1; // Sin permiso de escritura
  }
  ```
- `sys_open`:
  ```c
  if ((ip->perm & 1) == 0 && (omode & O_RDONLY)) {
      return -1; // Sin permiso de lectura
  }
  if ((ip->perm & 2) == 0 && (omode & O_WRONLY)) {
      return -1; // Sin permiso de escritura
  }
  ```

### Implementación de `chmod`:
En `sysproc.c`, se añadió la syscall `sys_chmod`:
```c
uint64 sys_chmod(void) {
    char path[MAXPATH];
    int mode;
    struct inode *ip;

    argstr(0, path, MAXPATH);
    argint(1, &mode);

    begin_op();
    if ((ip = namei(path)) == 0) {
        end_op();
        return -1; // Archivo no encontrado
    }
    ilock(ip);
    ip->perm = mode & 3; // Actualizar permisos
    iupdate(ip);
    iunlockput(ip);
    end_op();
    return 0; // Éxito
}
```
Esto porque la syscall `sys_chmod` permite cambiar los permisos de un archivo en `xv6-riscv`. Ajusta el campo de permisos (perm) del inodo del archivo para definir si este puede ser leído, escrito o ambos.

### Modificaciones en otros archivos:
- En `syscall.h`: Se añadió:
  ```c
  #define SYS_chmod 22
  ```
- En `syscall.c`: Se registró la syscall:
  ```c
  extern uint64 sys_chmod(void);
  [SYS_chmod] sys_chmod,
  ```
- En `usys.pl`: Se añadió:
  ```c
  entry("chmod");
  ```
- En `user.h`: Se añadió:
  ```c
  int chmod(const char *path, int mode);
  ```

---

### Por último, se realizó la prueba:
Se creó el archivo `testchmod.c`:
```c
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fcntl.h"

int main() {
    char *filename = "testfile.txt";
    int fd;

    // Creación del Archivo con permisos de lectura y escritura
    fd = open(filename, O_CREATE | O_RDWR);
    if (fd < 0) {
        printf("Error: no se pudo crear el archivo %s\n", filename);
        exit(1);
    }
    printf("Archivo %s creado con permisos de lectura y escritura.\n", filename);

    // Escritura Inicial
    if (write(fd, "Hello, xv6!\n", 13) != 13) {
        printf("Error: no se pudo escribir en el archivo %s durante la escritura inicial.\n", filename);
        close(fd);
        exit(1);
    }
    printf("Escritura inicial en el archivo completada.\n");
    close(fd); // Asegurarse de cerrar después de escribir

    // Cambio de Permisos a Solo Lectura
    if (chmod(filename, 1) < 0) {
        printf("Error: no se pudieron cambiar los permisos a solo lectura para el archivo %s.\n", filename);
        exit(1);
    }
    printf("Permisos cambiados a solo lectura para el archivo %s.\n", filename);

    // Prueba de Escritura con Solo Lectura
    fd = open(filename, O_WRONLY); // Intentar abrir en modo escritura
    if (fd >= 0) {
        printf("Error: se pudo abrir el archivo %s en modo escritura cuando está en solo lectura.\n", filename);
        close(fd); // Cerrar si se abre por error
        exit(1);
    }
    printf("Intento de abrir en modo escritura falló como era esperado.\n");

    // Cambio de Permisos a Lectura/Escritura
    if (chmod(filename, 3) < 0) {
        printf("Error: no se pudieron cambiar los permisos a lectura/escritura para el archivo %s.\n", filename);
        exit(1);
    }
    printf("Permisos cambiados a lectura/escritura para el archivo %s.\n", filename);

    // Escritura Final
    fd = open(filename, O_RDWR); // Abrir nuevamente en modo lectura/escritura
    if (fd < 0) {
        printf("Error: no se pudo abrir el archivo %s en modo lectura/escritura.\n", filename);
        exit(1);
    }
    if (write(fd, "Permisos restaurados.\n", 22) != 22) {
        printf("Error: no se pudo escribir en el archivo %s después de restaurar los permisos.\n", filename);
        close(fd);
        exit(1);
    }
    printf("Escritura final en el archivo completada.\n");
    close(fd); // Cerrar después de escribir

    printf("Prueba de chmod completada exitosamente.\n");
    exit(0);
}

```

## Resultados:
Al ejecutar el programa de prueba:
```bash
$ testchmod
Archivo testfile.txt creado con permisos de lectura y escritura.
Escritura inicial en el archivo completada.
Permisos cambiados a solo lectura para el archivo testfile.txt.
Intento de abrir en modo escritura falló como era esperado.
Permisos cambiados a lectura/escritura para el archivo testfile.txt.
Escritura final en el archivo completada.
Prueba de chmod completada exitosamente.
$ cat testfile.txt
Permisos restaurados.
```

# Segunda Parte

El objetivo de la segunda parte de la tarea es agregar un permiso especial (número 5) a los archivos, marcándolos como inmutables. Esto implica que los archivos inmutables son considerados como solo lectura, además que no se pueden modificar los permisos de un archivo inmutable usando `chmod`.

## Modificaciones iniciales:

### En `defs.h` se modificó:
```c
int argint(int, int*);
```
Esto porque la declaración se ajustó para que coincida con la implementación de `argint`.

### En `syscall.c` se modificó:
```c
int
argint(int n, int *ip)
{
    if (n < 0 || n >= 3) // Validar que el índice del argumento sea válido
        return -1;

    *ip = argraw(n);
    return 0; // Éxito
}
```
Esto porque la función `argint` se modificó para devolver un valor que indique éxito (`0`) o error (`-1`). Esto permite manejar errores al obtener argumentos en las llamadas al sistema.

#### Modificaciones a `sys_open` y `sys_write`:
En `sys_open`:
   - Se bloquea la apertura de archivos en modo escritura si el archivo es inmutable.
   - Código relevante:
     ```c
     if (ip->perm == 5 && (omode & (O_WRONLY | O_RDWR))) {
         end_op();
         iunlockput(ip);
         return -1; // No se puede abrir en modo escritura si es inmutable
     }
     ```
En `sys_write`:
   - Se bloquea la escritura en archivos inmutables.
   - Código relevante:
     ```c
     if (f->ip && (f->ip->perm == 5 || (f->ip->perm & 2) == 0)) {
         return -1; // Bloquear escritura
     }
     ```

Se realizaron cambios para manejar el permiso especial `5` (inmutable).

#### En `sys_chmod` se modificó:
```c
uint64
sys_chmod(void)
{
    char path[MAXPATH];
    int mode;
    struct inode *ip;

    // Obtener argumentos
    if (argstr(0, path, MAXPATH) < 0) {
        return -1; // Error al obtener el primer argumento
    }
    if (argint(1, &mode) < 0) {
        return -1; // Error al obtener el segundo argumento
    }

    begin_op();
    if ((ip = namei(path)) == 0) {
        end_op();
        return -1; // Archivo no encontrado
    }

    ilock(ip);

    // Verificar si el archivo es inmutable
    if (ip->perm == 5) {
        iunlockput(ip);
        end_op();
        return -1; // No se pueden cambiar los permisos de un archivo inmutable
    }

    ip->perm = mode & 7; // Máscara para asegurar valores válidos (0-7)
    iupdate(ip);         // Guardar los cambios en el disco
    iunlockput(ip);
    end_op();

    return 0; // Éxito
}
```
Esto porque se añadió la lógica para evitar que los permisos de archivos inmutables sean modificados, garantizando su inmutabilidad.

### `testchmod.c`:
```c
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fcntl.h"

int main() {
    char *filename = "testfile.txt";
    int fd;

    // Creación del Archivo con permisos de lectura y escritura
    fd = open(filename, O_CREATE | O_RDWR);
    if (fd < 0) {
        printf("Error: no se pudo crear el archivo %s\n", filename);
        exit(1);
    }
    printf("Archivo %s creado con permisos de lectura y escritura.\n", filename);

    // Escritura Inicial
    if (write(fd, "Hello, xv6!\n", 13) != 13) {
        printf("Error: no se pudo escribir en el archivo %s.\n", filename);
        close(fd);
        exit(1);
    }
    printf("Escritura inicial en el archivo completada.\n");
    close(fd);

    // Cambio de Permisos a Lectura/Escritura
    if (chmod(filename, 3) < 0) {
        printf("Error: no se pudieron cambiar los permisos a lectura/escritura.\n");
        exit(1);
    }
    printf("Permisos cambiados a lectura/escritura para el archivo.\n");

    // Escritura Final
    fd = open(filename, O_RDWR);
    if (fd < 0) {
        printf("Error: no se pudo abrir el archivo %s en modo lectura/escritura.\n", filename);
        exit(1);
    }
    if (write(fd, "Permisos restaurados.\n", 22) != 22) {
        printf("Error: no se pudo escribir en el archivo después de restaurar los permisos.\n");
        close(fd);
        exit(1);
    }
    printf("Escritura final en el archivo completada.\n");
    close(fd);

    // Cambio de Permisos a Inmutable
    if (chmod(filename, 5) < 0) {
        printf("Error: no se pudieron cambiar los permisos a inmutable.\n");
        exit(1);
    }
    printf("Permisos cambiados a inmutable para el archivo.\n");

    // Prueba de Escritura con Solo Lectura
    fd = open(filename, O_WRONLY);
    if (fd >= 0) {
        printf("Error: se pudo abrir el archivo en modo escritura cuando está marcado como inmutable.\n");
        close(fd);
        exit(1);
    }
    printf("Intento de abrir en modo escritura falló como era esperado.\n");

    // Intento de Cambiar Permisos de Vuelta a Lectura/Escritura
    if (chmod(filename, 3) == 0) {
        printf("Error: se pudieron cambiar los permisos de un archivo inmutable, lo cual no debería ser posible.\n");
        exit(1);
    }
    printf("Intento de cambiar permisos de un archivo inmutable falló como era esperado.\n");

    printf("Prueba de chmod con inmutabilidad completada exitosamente.\n");
    exit(0);
}
```
Se creó un programa de prueba con los cambios implementados en el sistema de permisos, incluyendo el manejo de archivos inmutables.

## Resultados:
Al ejecutar el programa de prueba:
```bash
$ testchmod
Archivo testfile.txt creado con permisos de lectura y escritura.
Escritura inicial en el archivo completada.
Permisos cambiados a lectura/escritura para el archivo.
Escritura final en el archivo completada.
Permisos cambiados a inmutable para el archivo.
Intento de abrir en modo escritura falló como era esperado.
Intento de cambiar permisos de un archivo inmutable falló como era esperado.
Prueba de chmod con inmutabilidad completada exitosamente.
$ cat testfile.txt
Permisos restaurados.
```

## Conclusiones
Esta tarea presentó un alto nivel de complejidad debido a la cantidad de modificaciones requeridas en archivos secundarios del kernel, como defs.h, syscall.c y sysproc.c. La implementación y prueba de los permisos especiales demandaron un conocimiento detallado del flujo interno de xv6-riscv. No obstante, los resultados finales confirman que el sistema es capaz de gestionar de forma robusta tanto los permisos estándar como los inmutables.
