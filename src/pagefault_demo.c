#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    size_t N = 100 * 1024 * 1024;   /* 100 MiB */
    char *buf = (char *)malloc(N);
    if (!buf) {
        perror("malloc");
        return 1;
    }

    printf("=== Antes de tocar la memoria (solo reserva virtual) ===\n");
    fflush(stdout);
    system("grep -E \"VmSize|VmRSS|VmPeak\" /proc/self/status");

    /* Forzar minor page faults escribiendo en cada pagina (100 MiB / 4 KiB = 25600) */
    memset(buf, 0xAA, N);

    printf("\n=== Despues de memset(100 MiB) ===\n");
    fflush(stdout);
    system("grep -E \"VmSize|VmRSS|VmPeak\" /proc/self/status");

    free(buf);
    return 0;
}
