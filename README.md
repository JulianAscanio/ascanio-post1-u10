# Laboratorio Práctico: Memorias del Computador — Benchmark de Jerarquía de Caché y Análisis de Memoria Virtual en C sobre Linux

**Asignatura:** Arquitectura de Computadores  
**Unidad 10:** Post-contenido (Laboratorio)  
**Estudiante:** Julián Andrés Ascanio  
**Repositorio:** `ascanio-post1-u10`  
**Entorno:** Ubuntu Linux 24.04 (`x86_64`), GCC 13.3.0, AMD Ryzen 5 7520U

---

## 1. Contexto y Objetivos

El presente laboratorio guiado implementa en lenguaje C sobre un sistema Linux real dos módulos complementarios que validan empíricamente la jerarquía de memorias y la administración de memoria virtual:

1. **Parte 1 (`src/cache_bench.c`):**
   - Detección de la topología de caché del procesador mediante `/sys/devices/system/cpu/cpu0/cache/`.
   - Medición de latencia de acceso secuencial (`bench_seq`) y aleatorio con mezcla Fisher-Yates (`bench_rand`) para arreglos desde `4 KiB` hasta `64 MiB`.
   - Evaluación de conflictos de asociatividad (`bench_stride`) comparando *stride* de `64 B` frente a `4096 B` sobre un arreglo de `1 MiB`.

2. **Parte 2 (`src/mymaps.c`, `src/pagefault_demo.c`, `src/mmap_fault_cost.c`):**
   - Inspección del mapa de memoria virtual de un proceso en `/proc/PID/maps` y clasificación de páginas en `/proc/PID/smaps` y `pmap -x`.
   - Observación del crecimiento de `VmRSS` y del conteo de *minor page faults* mediante `pagefault_demo.c` y `/usr/bin/time -v`.
   - Medición aislada del costo de un *minor page fault* individual con `mmap` anónimo y `clock_gettime(CLOCK_MONOTONIC)` en `mmap_fault_cost.c`.

---

## 2. Estructura del Repositorio

```text
ascanio-post1-u10/
├── README.md                 # Documentación técnica, análisis y cuatro decisiones justificadas
├── .gitignore                # Exclusión de binarios ELF, objetos y scripts auxiliares
├── src/
│   ├── cache_bench.c         # Benchmark de jerarquía de caché (secuencial, aleatorio y stride)
│   ├── mymaps.c              # Programa pausado para inspección de /proc/PID/maps y smaps
│   ├── pagefault_demo.c      # Demostración de demand paging y crecimiento de VmRSS (100 MiB)
│   └── mmap_fault_cost.c     # Medición del overhead de un minor page fault individual
└── capturas/                 # Evidencias de compilación y ejecución en terminal Linux
    ├── CP1_cache_topologia.png
    ├── CP2_cache_bench_O0.png
    ├── CP3_cache_bench_O2.png
    ├── CP4_compilacion_parte2.png
    ├── CP5_mymaps_proc_maps.png
    ├── CP6_smaps_pmap.png
    ├── CP7_pagefault_demo.png
    └── CP8_mmap_fault_cost.png
```

---

## 3. Parte 1: Benchmark de Jerarquía de Caché

### 3.1 Topología de Caché Detectada
En el sistema de pruebas (**AMD Ryzen 5 7520U**, 4 núcleos / 8 hilos, `x86_64`), `/sys/devices/system/cpu/cpu0/cache/` reporta:
- **L1d (`index0`):** `32 KiB`, 8 vías asociativas, líneas de `64 B` (64 *sets*).
- **L1i (`index1`):** `32 KiB`, 8 vías asociativas.
- **L2 (`index2`):** `512 KiB`, unificada, 8 vías asociativas.
- **L3 (`index3`):** `4096 KiB` (`4 MiB`), unificada, 16 vías asociativas.

### 3.2 Resultados y Análisis (Secuencial, Aleatorio y Stride)
- **Secuencial vs. Aleatorio:** En `bench_seq`, la localidad espacial y el *hardware prefetcher* mantienen la latencia baja (`~1.55–1.96 ns/B` con `-O0` y `~0.16–0.29 ns/B` con `-O2`) incluso al superar la L3. En cambio, `bench_rand` destruye la localidad espacial mediante permutación Fisher-Yates y expone con nitidez la jerarquía real: en `-O0` pasa de `~2.7 ns/elem` (L1/L2, `4–256 KiB`) a `3.558 ns/elem` en el borde de L2 (`512 KiB`), salta a `11.176 ns/elem` al alcanzar la L3 (`4096 KiB`) por *cache misses* y alcanza `16.1–16.4 ns/elem` en RAM (`16–64 MiB`) sumando *TLB misses*.
- **Stride (`64 B` vs. `4096 B`):** Sobre `1 MiB`, ambos *strides* muestran `~3.28 ns/acceso` debido a que el cálculo `(i * stride) % n_bytes` en cada iteración introduce una división entera que domina el tiempo del lazo y a que `256` líneas con *stride* `4096 B` aún caben en la L2 unificada de `512 KiB` (donde el *stride* de conflicto de L2 es `512 KiB / 8 = 64 KiB`), mientras que la L1d de `32 KiB` sí sufre desalojos por asociatividad a partir del noveno acceso en el mismo *set*.

### 3.3 Checkpoints de la Parte 1
| Checkpoint | Evidencia | Resultado Observado |
| :--- | :--- | :--- |
| Checkpoint 1 | [`CP1_cache_topologia.png`](capturas/CP1_cache_topologia.png) | Topología detectada: L1d `32K` (8-way), L1i `32K` (8-way), L2 `512K` (8-way), L3 `4096K` (16-way). |
| Checkpoints 2–4 (`-O0`) | [`CP2_cache_bench_O0.png`](capturas/CP2_cache_bench_O0.png) | Saltos en `Rand`: `~2.7 ns` (L1/L2) → `11.18 ns` (L3) → `16.42 ns` (RAM); tabla `Stride 64 B` vs `4096 B`. |
| Checkpoints 2–4 (`-O2`) | [`CP3_cache_bench_O2.png`](capturas/CP3_cache_bench_O2.png) | Latencias optimizadas con `-O2`: `Seq` `~0.17–0.29 ns/B`, `Rand` `~0.95 ns` (L1) → `~9.62 ns` (L3/RAM). |
| Checkpoint 5 | Sección 3.4 | Dos decisiones técnicas justificadas de la Parte 1 (≥100 palabras cada una). |

### 3.4 Decisiones Técnicas de la Parte 1

#### Decisión Técnica 1: ¿Qué stride expone mejor los conflictos de asociatividad en esta caché?
Para una caché L1d de `32 KiB` con `8` vías asociativas y líneas de `64` bytes (`64` conjuntos o *sets* en total), el *stride* exacto que garantiza que todos los accesos de `bench_stride` mapeen sistemáticamente al mismo *set* es el tamaño total de la caché dividido entre su asociatividad: `32 768 / 8 = 4096` bytes. En este caso concreto, `4096` bytes coincide además con el tamaño de página virtual (`4 KiB`), manteniendo fijos tanto los `6` bits de *offset* dentro de la línea (`bits 5:0`) como los `6` bits de índice de *set* (`bits 11:6`). Si se empleara un *stride* de `2048` bytes en lugar de `4096`, el bit `11` de la dirección alternaría entre `0` y `1` en cada paso; por consiguiente, los accesos recorrerían dos *sets* distintos de forma alternada en vez de concentrarse en uno solo, reduciendo a la mitad la presión de conflictos por conjunto. Con el *stride* de `4096` bytes sobre `1 MiB` (`256` páginas), las primeras `8` direcciones llenan las `8` vías del *set*; a partir del noveno acceso, cada nueva línea debe desalojar a una de las ocho residentes según la política de reemplazo, degradando todos los accesos posteriores a la latencia de L2 o niveles inferiores en lugar de resolverse en L1 como ocurre en el recorrido secuencial del Checkpoint 2.

#### Decisión Técnica 2: ¿`-fno-prefetch-loop-arrays` deshabilita el mismo prefetching que el prefetcher de hardware del procesador?
El flag `-fno-prefetch-loop-arrays` de GCC actúa exclusivamente sobre el generador de código del compilador, suprimiendo la emisión de instrucciones explícitas de *prefetch* por software (como `PREFETCHT0`) en bucles que recorren arreglos; no ejerce ningún efecto sobre el *prefetcher* de hardware integrado en el procesador, el cual analiza el flujo de direcciones en tiempo de ejecución con total independencia del binario ejecutado. Al compilar `cache_bench.c` con `-O2 -fprefetch-loop-arrays` frente a `-O2 -fno-prefetch-loop-arrays`, el patrón más sensible a esa diferencia es el acceso secuencial (`bench_seq`), porque su progresión lineal es predecible en tiempo de compilación y permite al compilador insertar *prefetches* adelantados. En contraste, el patrón aleatorio (`bench_rand`) y el de *stride* de `4096` bytes son prácticamente insensibles: el primero depende de una permutación Fisher-Yates impredecible estáticamente y el segundo falla por conflictos de asociatividad dentro del mismo *set*, no por falta de *prefetch* lineal. Dado que desactivar el *prefetcher* de hardware no es portable desde GCC (exige modificar registros `MSR` específicos del modelo de CPU o ajustes de BIOS), comparar ambos flags mide únicamente el aporte del *prefetch* por software del compilador y no el del *prefetcher* de hardware.

---

## 4. Parte 2: Análisis de Memoria Virtual en Linux

### 4.1 Regiones, Clasificación de Páginas y Page Faults
- **`/proc/PID/maps` (`mymaps.c`):** Se identifican `.text` (`r-xp`), `.rodata` (`r--p`), `.data` (`rw-p`), `[heap]` (`rw-p`), bloque anónimo de `1 MiB` (`rw-p`), `libc.so.6`, `ld-linux-x86-64.so.2`, `[vvar]`, `[vdso]`, `[stack]` (`rw-p`) y `[vsyscall]` (`--xp`).
- **`smaps` vs. `pmap -x`:** En `smaps`, el segmento `[heap]` presenta `Private_Dirty: 12 kB` (páginas privadas modificadas no compartibles), mientras que la sección ejecutable `r-xp` de `libc.so.6` presenta `Shared_Clean: 972 kB` (código de solo lectura compartido entre todos los procesos del sistema).
- **`pagefault_demo.c`:** Al reservar `100 MiB` con `malloc`, `VmSize` sube a `105 100 kB` pero `VmRSS` permanece en `1340 kB` (asignación diferida). Al tocar cada página de `4 KiB`, `VmRSS` crece a `103 800 kB` y `/usr/bin/time -v` registra `25 671` *minor page faults* y `0` *major page faults*.
- **`mmap_fault_cost.c`:** Sobre `200` páginas anónimas, el primer toque promedia `3778.3 ns` y el segundo toque `41.1 ns`, arrojando un *overhead* de **`3737.2 ns`** (~`3.74 µs`) por *minor page fault* —aproximadamente dos órdenes de magnitud superior a la latencia de acceso a RAM (`~16.4 ns`).

### 4.2 Checkpoints de la Parte 2
| Checkpoint | Evidencia | Resultado Observado |
| :--- | :--- | :--- |
| Compilación Parte 2 | [`CP4_compilacion_parte2.png`](capturas/CP4_compilacion_parte2.png) | `mymaps`, `pagefault_demo` y `mmap_fault_cost` compilados sin errores ni advertencias con `gcc -O2`. |
| Checkpoint 1 | [`CP5_mymaps_proc_maps.png`](capturas/CP5_mymaps_proc_maps.png) | Mapa completo en `/proc/PID/maps` identificando `.text`, `.data`, `[heap]`, `libc.so.6`, `[stack]` y `[vdso]`. |
| Checkpoint 2 | [`CP6_smaps_pmap.png`](capturas/CP6_smaps_pmap.png) | `[heap]` clasificado como `Private_Dirty: 12 kB` y `libc.so.6` (`r-xp`) como `Shared_Clean: 972 kB`. |
| Checkpoint 3 | [`CP7_pagefault_demo.png`](capturas/CP7_pagefault_demo.png) | `VmRSS` pasa de `1340 kB` a `103 800 kB`; `25 671` *minor faults* y `0` *major faults*. |
| Checkpoint 4 | [`CP8_mmap_fault_cost.png`](capturas/CP8_mmap_fault_cost.png) | Primer toque: `3778.3 ns`; segundo toque: `41.1 ns`; *overhead* del *fault*: `3737.2 ns`. |
| Checkpoint 5 | Sección 4.3 | Dos decisiones técnicas justificadas de la Parte 2 (≥100 palabras cada una). |

### 4.3 Decisiones Técnicas de la Parte 2

#### Decisión Técnica 3: ¿`/proc/PID/smaps` o `pmap -x` para este análisis?
Aunque `pmap -x` reporta de forma inmediata el tamaño virtual (`Kbytes`), la memoria residente (`RSS`) y las páginas sucias totales por mapeo en una tabla compacta, no desglosa la diferencia entre `Private_Dirty` y `Shared_Clean` ni expone contadores por página como `Referenced`, `AnonPages` o `Swap`, los cuales solo están disponibles en `/proc/PID/smaps`. No obstante, leer `/proc/PID/smaps` obliga al kernel a recorrer todas las tablas de páginas del proceso para sumar estadísticas detalladas de cada área de memoria virtual (`VMA`), lo que resulta notablemente más costoso en CPU cuando el proceso posee cientos de regiones mapeadas. En un escenario de monitoreo continuo —por ejemplo, un *script* que consulta el consumo de memoria cada segundo—, ese costo hace preferible `pmap -x` o `/proc/PID/statm` para evitar perturbar al proceso observado. En síntesis, `pmap -x` es la herramienta adecuada para una inspección rápida en terminal por su formato tabular resumido, mientras que `/proc/PID/smaps` es indispensable cuando el diagnóstico requiere clasificar páginas privadas frente a compartidas o limpias frente a sucias por región, como en el Checkpoint 2 de este laboratorio.

#### Decisión Técnica 4: ¿`mmap` con `MAP_POPULATE` o *demand paging* por defecto?
Si `mmap_fault_cost.c` añadiera el flag `MAP_POPULATE` a la llamada de `mmap`, el kernel asignaría y mapearía los `200` marcos físicos por adelantado antes de retornar de la llamada al sistema; en consecuencia, durante el bucle posterior el «primer toque» de cada página ya encontraría la traducción presente en las tablas de páginas y mediría una latencia cercana a la del «segundo toque» (`~41 ns`), ocultando el *overhead* de `~3.74 µs` observado en el Checkpoint 4. En aplicaciones reales, pagar el costo de `MAP_POPULATE` durante el arranque conviene a servidores de baja latencia o sistemas en tiempo real que necesitan tiempos de respuesta predecibles por petición, sin sufrir picos provocados por *page faults* en la ruta crítica. Por el contrario, el *demand paging* por defecto es superior en programas que reservan rangos amplios de memoria de forma especulativa: si un proceso reserva `100 MiB` con `mmap` pero solo llega a escribir `10 MiB` antes de finalizar, el *demand paging* evita consumir tiempo de kernel y `90 MiB` de marcos físicos en RAM que nunca se utilizaron, desperdicio que `MAP_POPULATE` sí habría materializado desde el primer instante.

---

## 5. Compilación y Ejecución

```bash
mkdir -p bin
gcc -O0 -Wall -Wextra -o bin/cache_bench src/cache_bench.c
gcc -O2 -Wall -Wextra -o bin/mymaps src/mymaps.c
gcc -O2 -Wall -Wextra -o bin/pagefault_demo src/pagefault_demo.c
gcc -O2 -Wall -Wextra -o bin/mmap_fault_cost src/mmap_fault_cost.c
```
