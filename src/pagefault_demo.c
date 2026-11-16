#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void imprimir_estado_memoria(const char *titulo) {
    FILE *f = fopen("/proc/self/status", "r");
    char linea[256];
    if (!f) {
        perror("fopen");
        return;
    }
    printf("%s\n", titulo);
    while (fgets(linea, sizeof(linea), f)) {
        if (strncmp(linea, "VmPeak:", 7) == 0 ||
            strncmp(linea, "VmSize:", 7) == 0 ||
            strncmp(linea, "VmRSS:", 6) == 0) {
            printf("  %s", linea);
        }
    }
    fclose(f);
}

int main(void) {
    size_t N = 100 * 1024 * 1024;   /* 100 MiB */
    volatile char *buf = (volatile char *)malloc(N);
    if (!buf) {
        perror("malloc");
        return 1;
    }

    imprimir_estado_memoria("=== Antes de tocar la memoria (solo reserva virtual) ===");

    /* Forzar minor page faults tocando cada pagina (100 MiB / 4 KiB = 25600) */
    for (size_t i = 0; i < N; i += 4096) {
        buf[i] = 0xAA;
    }

    imprimir_estado_memoria("\n=== Despues de tocar 100 MiB (demand paging) ===");

    free((void *)buf);
    return 0;
}
