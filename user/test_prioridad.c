#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  int pid;
  int num_procesos = 20;

  // Crear 20 procesos usando fork()
  for (int i = 0; i < num_procesos; i++) {
    pid = fork();

    if (pid < 0) {
      // Error al crear el proceso
      printf("Error al crear el proceso %d\n", i);
      exit(1);
    } else if (pid == 0) {
      // Código ejecutado por los procesos hijos
      printf("Ejecutando proceso %d\n", getpid());

      // Dormir el proceso por 2 segundos
      sleep(2);

      // Terminar el proceso hijo
      exit(0);
    } else {
      // El proceso padre espera a que el hijo termine antes de seguir creando más procesos
      wait(0);
    }
  }

  printf("Todos los procesos han terminado.\n");
  exit(0);
}
