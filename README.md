# Laboratorio 2 - Instrucciones SIMD (AVX2)

## Objetivo
Comprender los efectos de utilizar instrucciones vectoriales (AVX2) comparando
una multiplicación de matrices escalar contra una versión vectorizada.

## Ejercicios A, B y C
Se completaron las funciones `simd_mul_ps`, `simd_reduce_add_ps` y
`dot_product_avx2` en `matmul_avx2.c` usando intrinsics de AVX2:
- `simd_mul_ps`: multiplicación elemento a elemento con `_mm256_mul_ps`.
- `simd_reduce_add_ps`: reducción horizontal de 8 floats a 1 usando
  `_mm_add_ps` y `_mm_hadd_ps`.
- `dot_product_avx2`: producto punto combinando las dos funciones anteriores,
  usado para armar la multiplicación de matrices.

## Ejercicio D - Comparación de tiempos

| Versión | Tiempo   | Rendimiento |
|---------|----------|-------------|
| Escalar | 7.382633 s | 2.327065 GFLOP/s |
| AVX2    | 2.624091 s | 6.546978 GFLOP/s |

Speedup: ~2.81x

Matriz: 2048x2048, checksum idéntico en ambas versiones (86972906452.000000),
confirmando que el resultado es correcto.

### Explicación
AVX2 procesa 8 floats por instrucción, por lo que en teoría el máximo
especulativo sería 8x. En la práctica se obtuvo ~2.8x porque:
- La matriz (~16 MB por matriz) no cabe en caché, por lo que el cuello de
  botella pasa a ser el ancho de banda de memoria, no la aritmética.
- `simd_reduce_add_ps` añade overhead por iteración (varias instrucciones
  `hadd` + extracción escalar) que no existe en la versión escalar.
- Solo la parte de multiplicación y suma se vectorizó; la transposición y el
  acceso a memoria siguen siendo iguales en ambas versiones.
