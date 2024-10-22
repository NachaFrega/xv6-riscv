# Informe Tarea 0 Ignacia Frega
El objetivo de esta tarea fue modificar el sistema operativo Xv6-riscv para añadir un sistema de planificación basado en prioridades con un mecanismo de Boost. En este esquema, los procesos con una prioridad numérica menor se ejecutan primero. El Boost ajusta dinámicamente dicha prioridad: cuando un proceso alcanza el valor 9, el boost cambia a -1; y cuando llega a 0, el boost regresa a 1.

### Explicación de las modificaciones realizadas.
### En `kernel`
#### Archivo `proc.h`
Se añadieron dos nuevos campos a la estructura de proceso en `proc.h`:
```c
int priority;  // Prioridad del proceso
int boost;     // Valor de boost
```
Estos campos controlan la prioridad de cada proceso. Un valor más bajo en priority indica mayor prioridad, y boost ajusta dinámicamente el valor dentro de un rango.
#### Archivo `proc.c`
En la función `static struct proc* allocproc(void),` se inicializaron los nuevos campos para cada proceso:
```c
p->priority = 0;  // Inicializar prioridad en 0
p->boost = 1;     // Inicializar boost en 1
```
Esto garantiza que todos los procesos comiencen con la prioridad más alta (0) y un boost positivo (1), que incrementará la prioridad con el tiempo.
En la función `scheduler(),` se verificó inicialmente el correcto funcionamiento del sistema antes de añadir la lógica de prioridades completa:
```c
void scheduler(void)
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

      // Asignar el proceso a la CPU
      p->state = RUNNING;
      c->proc = p;
      
      swtch(&c->context, &p->context);  // Cambiar el contexto de CPU al proceso

      // Proceso ha terminado de correr
      c->proc = 0;
      release(&p->lock);
    }
  }
}
```
Este código inicial asegura que el sistema operativo funciona con un planificador básico antes de implementar la lógica de prioridades completa.
### En `user`
Se creó un programa para generar múltiples procesos y observar su comportamiento, programa de prueba `test_prioridad.c`:
```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(void)
{
  int pid;
  int num_procesos = 20;

  for (int i = 0; i < num_procesos; i++) {
    pid = fork();

    if (pid < 0) {
      printf("Error al crear el proceso %d\n", i);
      exit(1);
    } else if (pid == 0) {
      printf("Ejecutando proceso %d\n", getpid());
      sleep(2);  // Dormir el proceso por 2 segundos
      exit(0);
    } else {
      wait(0);  // El proceso padre espera a que cada hijo termine
    }
  }

  printf("Todos los procesos han terminado.\n");
  exit(0);
}
```
Este programa genera 20 procesos, cada uno imprime su PID, duerme por 2 segundos y luego finaliza, mientras el proceso padre espera que todos terminen.
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
