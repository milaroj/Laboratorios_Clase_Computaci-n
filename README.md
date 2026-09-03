# Práctica de clase 3 y 4

## Práctica 3 — Ejercicio A: Threading, afinidad y escalabilidad

Se compararon dos versiones del mismo programa (`cpu-naive.c` y `cpu-affinity.c`), variando
el número de hilos de 1 a 20, midiendo el tiempo real de ejecución con `time`.

Los resultados completos (tabla y gráfico) están en `practica3_ejercicioA_hilos.xlsx`.

### Escalabilidad

En un caso ideal, dado que cada hilo procesa su propio bloque de memoria de forma
independiente (sin dependencias entre hilos ni secciones críticas), y hay 20 CPUs lógicos
disponibles, se esperaría una escalabilidad cercana a la ideal: el tiempo debería mantenerse
aproximadamente constante al aumentar el número de hilos.

Sin embargo, se observó lo contrario: el tiempo real **aumenta** conforme crece el número de
hilos, tanto en `cpu-naive` (de ~3.0s con 1 hilo a ~5.3s con 20 hilos) como en `cpu-affinity`
(de ~2.3s a ~3.4s). Esto ocurre porque el problema es memory-bound, no CPU-bound: cada hilo
mueve 256 MB de datos repetidamente, y todos los hilos comparten el mismo bus de memoria y
jerarquía de caché. Con pocos hilos, el ancho de banda de memoria está disponible casi en su
totalidad; con muchos hilos compitiendo por el mismo recurso, ese ancho de banda se convierte
en el cuello de botella, y el tiempo total crece en vez de mantenerse plano.

### Proporción de código paralelo

Aunque `cpu_burn` es teóricamente 100% paralelizable (no hay dependencias entre hilos), la Ley
de Amdahl clásica no explica completamente el comportamiento observado, ya que asume que el
único límite al paralelismo es la fracción secuencial del código. En este caso, el límite real
es un recurso físico compartido (el ancho de banda de memoria), que actúa como un cuello de
botella adicional no capturado por el modelo clásico, reduciendo el paralelismo efectivo aunque
el código no tenga una sección secuencial significativa.

### Eficiencia

Eficiencia = Speedup / N. Como el tiempo de ejecución aumenta en vez de disminuir al agregar
hilos, el speedup respecto a 1 hilo es menor a 1 y empeora progresivamente. En consecuencia, la
eficiencia cae fuertemente conforme aumenta el número de hilos, quedando muy por debajo de lo
ideal (1/N por hilo).

### Comparación naive vs. affinity

`cpu-affinity` es consistentemente más rápido que `cpu-naive` en todo el rango de hilos
medido. Esto se debe a dos factores: (1) cada hilo queda fijado a un CPU específico
(`pthread_setaffinity_np`), evitando que el planificador del sistema operativo lo migre entre
cores durante la ejecución; y (2) cada hilo reserva e inicializa su propia memoria *después*
de fijar su afinidad (técnica de "first touch"), lo que permite que Linux ubique esas páginas
físicamente más cerca del core que las usa, reduciendo la latencia de acceso a memoria.
