#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define VECTOR_SIZE 2048
#define MATRIX_SIZE 2048
#define AVX_FLOATS 8
#define DEFAULT_REPETITIONS 1

static inline __m256 simd_loadu_ps(const float *values)
{
    return _mm256_loadu_ps(values);
}

static inline __m256 simd_mul_ps(__m256 a, __m256 b)
{
    // _mm256_mul_ps multiplica 8 floats de "a" por 8 floats de "b"
    // en paralelo (una sola instrucción SIMD), y devuelve el
    // vector resultado también de 8 floats.
    return _mm256_mul_ps(a, b);
}

static inline float simd_reduce_add_ps(__m256 value)
{
    // Separar el vector AVX de 256 bits en dos mitades de 128 bits
    __m128 low  = _mm256_castps256_ps128(value);
    __m128 high = _mm256_extractf128_ps(value, 1);

    // Suma parte baja y parte alta -> 4 floats
    __m128 sum = _mm_add_ps(low, high);

    // Reducción horizontal: suma pares adyacentes
    // 4 floats -> 2 floats (primera pasada)
    // 2 floats -> 1 float, repetido en las 4 posiciones (segunda pasada)
    sum = _mm_hadd_ps(sum, sum);
    sum = _mm_hadd_ps(sum, sum);

    // Extraer el resultado escalar (primer elemento del vector de 128 bits)
    return _mm_cvtss_f32(sum);
}


float dot_product_avx2(const float a[VECTOR_SIZE], const float b[VECTOR_SIZE])
{
    float result = 0.0f;

    // Recorremos el vector de 8 en 8, porque un __m256 guarda
    // 8 floats (256 bits / 32 bits por float = 8).
    for (int i = 0; i < VECTOR_SIZE; i += AVX_FLOATS) {

        // &a[i] apunta al float en la posición i.
        // Ejemplo: en la primera vuelta &a[0] carga a[0..7],
        // en la segunda vuelta &a[8] carga a[8..15], etc.
        __m256 vec_a = simd_loadu_ps(&a[i]);
        __m256 vec_b = simd_loadu_ps(&b[i]);

        // Multiplica los 8 pares (a[i]*b[i], a[i+1]*b[i+1], ...)
        __m256 vec_mul = simd_mul_ps(vec_a, vec_b);

        // Suma esos 8 productos parciales en un solo float
        // y lo acumula al resultado total.
        result += simd_reduce_add_ps(vec_mul);
    }

    return result;return result;
}

void transpose_matrix_1024(
    const float matrix[MATRIX_SIZE][MATRIX_SIZE],
    float transposed[MATRIX_SIZE][MATRIX_SIZE])
{
    for (int row = 0; row < MATRIX_SIZE; ++row) {
        for (int col = 0; col < MATRIX_SIZE; ++col) {
            transposed[col][row] = matrix[row][col];
        }
    }
}

void matrix_multiply_avx2_1024(
    const float a[MATRIX_SIZE][MATRIX_SIZE],
    const float b[MATRIX_SIZE][MATRIX_SIZE],
    float result[MATRIX_SIZE][MATRIX_SIZE])
{
    static float b_transposed[MATRIX_SIZE][MATRIX_SIZE];

    transpose_matrix_1024(b, b_transposed);

    for (int row = 0; row < MATRIX_SIZE; ++row) {
        for (int col = 0; col < MATRIX_SIZE; ++col) {
            result[row][col] = dot_product_avx2(a[row], b_transposed[col]);
        }
    }
}

static double get_time_seconds(void)
{
    struct timespec time;

    clock_gettime(CLOCK_MONOTONIC, &time);

    return (double)time.tv_sec + (double)time.tv_nsec / 1000000000.0;
}

static void init_matrix_1024(float matrix[MATRIX_SIZE][MATRIX_SIZE], float scale)
{
    for (int row = 0; row < MATRIX_SIZE; ++row) {
        for (int col = 0; col < MATRIX_SIZE; ++col) {
            matrix[row][col] = scale * (float)((row + col) % 17 + 1);
        }
    }
}

static double checksum_matrix_1024(const float matrix[MATRIX_SIZE][MATRIX_SIZE])
{
    double sum = 0.0;

    for (int row = 0; row < MATRIX_SIZE; ++row) {
        for (int col = 0; col < MATRIX_SIZE; ++col) {
            sum += matrix[row][col];
        }
    }

    return sum;
}

static float matrix_a[MATRIX_SIZE][MATRIX_SIZE];
static float matrix_b[MATRIX_SIZE][MATRIX_SIZE];
static float matrix_result[MATRIX_SIZE][MATRIX_SIZE];

int main(int argc, char **argv)
{
    int repetitions = DEFAULT_REPETITIONS;

    if (argc > 1) {
        repetitions = atoi(argv[1]);
    }

    if (repetitions <= 0) {
        fprintf(stderr, "Uso: %s [repeticiones]\n", argv[0]);
        return EXIT_FAILURE;
    }

    init_matrix_1024(matrix_a, 0.5f);
    init_matrix_1024(matrix_b, 0.25f);

    double start = get_time_seconds();

    for (int rep = 0; rep < repetitions; ++rep) {
        matrix_multiply_avx2_1024(matrix_a, matrix_b, matrix_result);
    }

    double end = get_time_seconds();
    double elapsed = end - start;
    double operations = (double)repetitions * 2.0 * MATRIX_SIZE * MATRIX_SIZE * MATRIX_SIZE;
    double gflops = operations / elapsed / 1000000000.0;
    double checksum = checksum_matrix_1024(matrix_result);

    printf("Tamano matriz: %dx%d\n", MATRIX_SIZE, MATRIX_SIZE);
    printf("Repeticiones: %d\n", repetitions);
    printf("Checksum: %.6f\n", checksum);
    printf("C[0][0]: %.6f\n", matrix_result[0][0]);
    printf("C[1023][1023]: %.6f\n", matrix_result[1023][1023]);
    printf("Operaciones: %.0f\n", operations);
    printf("Tiempo: %.6f segundos\n", elapsed);
    printf("Rendimiento: %.6f GFLOP/s\n", gflops);

    return EXIT_SUCCESS;
}
