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
