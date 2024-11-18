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
