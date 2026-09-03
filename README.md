# Práctica de clase 3 — Ejercicio A

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
