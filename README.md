# Práctica de clase 3 

## Ejercicio A

Comparación de `cpu-naive.c` vs `cpu-affinity.c`, variando hilos de 1 a 20.
Datos y gráfico completos en `practica3_ejercicioA_hilos.xlsx`.

**Escalabilidad:** No hay escalabilidad — el tiempo aumenta con más hilos en vez de
mantenerse constante (naive: 3.0s -5.3s; affinity: 2.3s - 3.4s). El problema es
memory-bound: todos los hilos comparten el mismo bus de memoria, y a más hilos, más
contención por ese ancho de banda.

**Proporción de código paralelo:** El cómputo en sí es 100% paralelo (cada hilo procesa
su propio bloque sin dependencias), pero el ancho de banda de memoria compartido actúa
como un cuello de botella no capturado por la Ley de Amdahl clásica, limitando el
paralelismo real.

**Eficiencia:** Baja y decreciente. Como el tiempo sube en vez de bajar, el speedup es
menor a 1 y empeora con más hilos, por lo que la eficiencia (Speedup/N) cae fuertemente.

**Nota:** `cpu-affinity` es más rápido que `cpu-naive` en todo el rango.


## Ejercicio B

Datos y gráficos en `practica3_ejercicioB_hilos.xlsx`.

**softmax_openmp:**
- Escalabilidad: nula, empeora con más hilos (1.08s → 5.53s).
- Proporción paralela: código 100% paralelizado, pero el problema es tan pequeño (1000
  elementos, 300,000 regiones paralelas totales) que el overhead de sincronización supera
  el trabajo real.
- Eficiencia: muy baja, decrece con más hilos.

**matmul_tiled_openmp:**
- Escalabilidad: buena hasta ~9-10 hilos (0.70s → ~0.10s), luego se estabiliza.
- Proporción paralela: alta y efectiva, hay suficiente trabajo por hilo.
- Eficiencia: alta al inicio, cae después de la meseta (Ley de Amdahl).

**Conclusión:** paralelizar ayuda solo cuando el trabajo por hilo es suficientemente
grande frente al overhead de sincronización.


## Práctica 4 — Ejercicio A: Biblioteca estática

Ejecución de `bench-static 1000000 1000 1.0 2.0`:

| Métrica | Por iteración |
|---|---|
| fill A | 446.687 µs |
| fill B | 441.706 µs |
| add    | 943.492 µs |

Tamaño de `libvectorops.a`: **1.8 KB**.

## Ejercicio B: Biblioteca dinámica

Ejecución de `bench-dynamic 1000000 1000 1.0 2.0`:

| Métrica | Por iteración |
|---|---|
| fill A | 1679.598 µs |
| fill B | 1631.095 µs |
| add    | 1713.662 µs |

Tamaño de `libvectorops.so`: **16 KB**.

**Comparación:** La versión dinámica es notablemente más lenta en las tres métricas (2-4x)
y el archivo generado es más grande. Esto se debe a que las llamadas a funciones en una
biblioteca dinámica pasan por la PLT (indirección en tiempo de ejecución), y el compilador
no puede optimizar entre el programa y la biblioteca (sin inlining ni optimizaciones
inter-procedurales) porque el contenido del `.so` no se conoce en tiempo de compilación.
El `.so` es más grande porque incluye metadata para carga dinámica (tabla de símbolos,
información de reubicación) que la estática no necesita.
