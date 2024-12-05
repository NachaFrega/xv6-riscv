# **Tarea 5**

## **Autor**
- **Diego Troncoso Bustamante**

---

# **Primera Parte**

### **Objetivo General**
Implementar un sistema de permisos en el sistema operativo `xv6-riscv`, incluyendo lectura, escritura y un nuevo comando `chmod` para cambiar estos permisos.

---

## **Pasos Previos**

Antes de implementar la solución, se realizaron ajustes en varios archivos para corregir errores y preparar el entorno de desarrollo.

### **1. `riscv.h`**
Se añadió al inicio del archivo:
```c
#ifndef RISCV_H
#define RISCV_H
```
Y al final:
```c
#endif // RISCV_H
```
**Objetivo:** Garantizar que las declaraciones solo se incluyan una vez.

---

### **2. `trampoline.S`**
Se modificó la línea 112:
```asm
li a0, TRAPFRAME
```
por:
```asm
li a0, 0x3FFFFFE000
```
**Objetivo:** Ajustar la dirección del marco de interrupción para que coincida con el nuevo diseño de la memoria.

---

### **3. `memlayout.h`**
Se actualizaron los siguientes parámetros:
```c
#define MAXVA 0x4000000000L
#define TRAMPOLINE 0x3FFFFFF000L
#define TRAPFRAME 0x3FFFFFE000L
```
**Objetivo:** Rediseñar el mapeo de memoria virtual para mejorar la compatibilidad y la organización.

---

### **4. `spinlock.h`**
Se añadió protección contra múltiples inclusiones:
```c
#ifndef SPINLOCK_H
#define SPINLOCK_H
// Código de spinlock
#endif // SPINLOCK_H
```
**Objetivo:** Asegurar una implementación correcta y evitar errores de redefinición.

---

### **5. Otros Archivos Modificados**
- **`defs.h`:** Se añadió `#include "riscv.h"`.
- **`proc.h`:** Se añadieron:
  ```c
  #include "riscv.h"
  #include "spinlock.h"
  ```
- **`sleeplock.h`:** Se añadió `#include "spinlock.h"`.
- **`stat.h`:** Se añadió `#include "types.h"`.

**Objetivo:** Asegurar que las dependencias estén correctamente resueltas.

---

## **Implementación de la Tarea**

Despues de todo ese paso previo que fue gracias a los erorres arrojados por consola al momento de implementar la solución, estos son los archivos principales para que la primera parte resultada satisfecha al 100%

### **1. Modificaciones a la estructura `inode`**
En **`file.h`**, se añadió el campo `perm` para almacenar los permisos:
```c
struct inode {
    ...
    int perm; // 0 = no acceso, 1 = lectura, 2 = escritura, 3 = lectura/escritura
};
```
**Objetivo:** Almacenar los permisos de los archivos.

---

### **2. Cambios en la asignación de inodos**
En **`fs.c`**, se modificó la función `ialloc` para inicializar los permisos:
```c
ip->perm = 3; // Permiso predeterminado: lectura/escritura
```
**Objetivo:** Asegurar que los nuevos archivos tengan permisos adecuados por defecto.

---

### **3. Modificaciones en las syscalls**
En **`sysfile.c`**, se añadieron verificaciones de permisos en las funciones:
- **`sys_read`:**
  ```c
  if (f->ip && (f->ip->perm & 1) == 0) {
      return -1; // Sin permiso de lectura
  }
  ```
- **`sys_write`:**
  ```c
  if (f->ip && (f->ip->perm & 2) == 0) {
      return -1; // Sin permiso de escritura
  }
  ```
- **`sys_open`:**
  ```c
  if ((ip->perm & 1) == 0 && (omode & O_RDONLY)) {
      return -1; // Sin permiso de lectura
  }
  if ((ip->perm & 2) == 0 && (omode & O_WRONLY)) {
      return -1; // Sin permiso de escritura
  }
  ```

---

### **4. Implementación de `chmod`**
En **`sysproc.c`**, se añadió la syscall `sys_chmod`:
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
**Objetivo:** La syscall **`sys_chmod`** permite cambiar los permisos de un archivo en `xv6-riscv`. Modifica el campo de permisos (`perm`) del inodo del archivo para controlar si puede ser leído, escrito o ambas cosas. 

---

### **5. Cambios en otros archivos**
- **`syscall.h`:** Se añadió:
  ```c
  #define SYS_chmod 22
  ```
- **`syscall.c`:** Se registró la syscall:
  ```c
  extern uint64 sys_chmod(void);
  [SYS_chmod] sys_chmod,
  ```
- **`usys.pl`:** Se añadió:
  ```c
  entry("chmod");
  ```
- **`user.h`:** Se añadió:
  ```c
  int chmod(const char *path, int mode);
  ```

---

### **6. Prueba**
Se creó el archivo **`testchmod.c`**:
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

---

## **Resultados Obtenidos**
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

**Conclusión:** La implementación de permisos y la syscall `chmod` funcionan correctamente para la primera parte, ahora, veremos la segunda parte de la tarea.

---

# Segunda Parte

### Objetivo

Agregar un permiso especial (número 5) a los archivos, marcándolos como inmutables. Esto implica:

1. Los archivos inmutables son considerados como **solo lectura**.
2. No se pueden modificar los permisos de un archivo inmutable usando `chmod`.

---

## Modificaciones Previas

### Archivo: `defs.h`

Se modificó:

```c
int argint(int, int*);
```

**¿Por qué?**  
La declaración se ajustó para que coincida con la implementación de `argint`, ya que esta debe devolver un valor indicando éxito o error.

---

### Archivo: `syscall.c`

Se modificó:

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

**¿Por qué?**  
La función `argint` se modificó para devolver un valor que indique éxito (`0`) o error (`-1`). Esto permite manejar errores al obtener argumentos en las llamadas al sistema, mejorando la robustez del sistema.

---

### Implementación Segunda Parte

#### Modificaciones a `sys_open` y `sys_write`

Se realizaron cambios para manejar el permiso especial `5` (inmutable).

1. **`sys_open`**
   - Se bloquea la apertura de archivos en modo escritura si el archivo es inmutable.
   - Código relevante:
     ```c
     if (ip->perm == 5 && (omode & (O_WRONLY | O_RDWR))) {
         end_op();
         iunlockput(ip);
         return -1; // No se puede abrir en modo escritura si es inmutable
     }
     ```

2. **`sys_write`**
   - Se bloquea la escritura en archivos inmutables.
   - Código relevante:
     ```c
     if (f->ip && (f->ip->perm == 5 || (f->ip->perm & 2) == 0)) {
         return -1; // Bloquear escritura
     }
     ```

**¿Por qué?**  
Estos cambios aseguran que los archivos marcados como inmutables no puedan ser modificados ni escritos.

---

#### Modificación a `sys_chmod`

Código actualizado:

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

**¿Por qué?**  
Se añadió la lógica para evitar que los permisos de archivos inmutables sean modificados, garantizando su inmutabilidad.

---

### Programa de Prueba: `testchmod.c`

Código:

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

**¿Por qué?**  
Este programa prueba los cambios implementados en el sistema de permisos, incluyendo el manejo de archivos inmutables.

---

## **Resultados Obtenidos**
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

Obteniendo los resultados esperados para la segunda parte de la tarea.




## Conclusiones

Esta tarea fue compleja debido a la cantidad de modificaciones necesarias en archivos secundarios del kernel, como `defs.h`, `syscall.c` y `sysproc.c`. Implementar y probar los permisos especiales requirió un profundo entendimiento del flujo interno de **`xv6-riscv`**. Sin embargo, los resultados demuestran que el sistema puede manejar permisos estándar e inmutables de manera robusta.

Muchas gracias!!!