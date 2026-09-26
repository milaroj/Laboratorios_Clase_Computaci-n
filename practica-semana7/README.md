# Practica en clase 6 - Programacion en CUDA (EL5859)

Laboratorio del Capitulo 3 (Unidades de Procesamiento Grafico): tres ejercicios de paralelismo de datos en GPU con CUDA - suma de vectores, producto punto y softmax.

## Entorno de ejecucion

- GPU: NVIDIA GeForce RTX 3060 Laptop (local, no el cluster remoto del curso)
- Driver: 595.91.07 - CUDA 13.2 (runtime del driver)
- Compilador: nvcc release 12.4, V12.4.131
- SO: Ubuntu (dual boot)

## Como compilar y correr cada ejercicio

Cada carpeta tiene su propio Makefile:

    cd semana_7/<carpeta>
    make clean
    make
    make run

El tamano de entrada se puede cambiar al vuelo, por ejemplo:

    make run N=2097152          # vector-add, dot-product
    make run ROWS=256 COLS=2048 # softmax

---

## Ejercicio A: suma de vectores (vector-add/)

Cada hilo calcula un unico elemento C[i] = A[i] + B[i], usando su indice global:

    int i = blockIdx.x * blockDim.x + threadIdx.x;

Es paralelismo "embarazosamente simple": cada salida es independiente, asi que el mapeo natural es un hilo por elemento.

Resultado obtenido:

    vector-add n=1048576: OK

Respuestas:

1. ¿Cuantos bloques se lanzan cuando N=1048576 y cada bloque tiene 256 hilos?
   blocks = (n + threads_per_block - 1) / threads_per_block = 1048576 / 256 = 4096 bloques (division exacta, ya que ambos son potencias de 2).

2. ¿Que ocurre si N no es multiplo del tamano del bloque?
   La division entera redondea hacia arriba, asi que se lanza un bloque adicional "de sobra". Ese bloque tiene hilos cuyo indice i cae fuera del vector (i >= n). La condicion if (i < n) evita que esos hilos sobrantes escriban fuera de los limites de memoria reservada.

3. ¿Que transferencias de memoria ocurren entre CPU y GPU?
   Dos copias Host->Device (h_a->d_a, h_b->d_b) antes del kernel, y una copia Device->Host (d_c->h_c) despues, para traer el resultado. El cudaMemset sobre d_c no es una transferencia - solo pone ceros directamente en memoria de GPU.

---

## Ejercicio B: producto punto (dot-product/)

A diferencia del ejercicio A, el resultado es un unico escalar (la suma de todos los productos a[i]*b[i]), asi que los hilos deben cooperar. Cada bloque:

1. Calcula productos locales y los guarda en memoria compartida (extern __shared__ float cache[]) - memoria rapida, visible solo entre los hilos de ese bloque.
2. Hace una reduccion en arbol: en cada ronda, stride empieza en blockDim.x/2 y se divide a la mitad (128 -> 64 -> ... -> 1); los hilos con tid < stride suman cache[tid] += cache[tid + stride]. Asi, en log2(256) = 8 pasos -con trabajo repartido entre muchos hilos a la vez- se colapsan 256 valores a uno solo, en vez de sumarlos en serie (255 pasos).
   - Se usa tid = threadIdx.x (indice local dentro del bloque, para cache[]) junto con i (indice global, para leer a[]/b[]).
   - Empezar la reduccion "desde la mitad y partiendo a la mitad" mantiene el patron de acceso contiguo entre hilos activos en cada ronda; el patron alternativo (empezar en 1 e ir duplicando) necesita la misma cantidad de pasos pero con hilos activos dispersos, que es menos eficiente en la practica.
   - __syncthreads() despues de cada ronda es indispensable: garantiza que todos los hilos terminaron de escribir en cache[] antes de que cualquiera lea los valores de la ronda anterior - sin eso hay condicion de carrera.
3. El hilo tid==0 de cada bloque escribe su resultado parcial en partials[blockIdx.x].
4. La CPU suma esos pocos valores parciales para obtener el resultado final.

Resultados obtenidos:

    dot-product n=1048576: gpu=-21.250000 cpu=-21.250000 error=0.000000 OK
    dot-product n=4194304: gpu=-0.500000 cpu=-0.500000 error=0.000000 OK

Respuestas:

1. ¿Por que este ejercicio no puede resolverse solamente escribiendo un valor independiente por hilo?
   Porque el resultado final es un solo numero (la suma total), no un vector de resultados independientes. Cada hilo puede calcular su producto local sin problema, pero combinarlos en un unico total requiere que los hilos se comuniquen entre si - de ahi la reduccion con memoria compartida.

2. ¿Cuantos valores parciales se copian de GPU a CPU?
   Uno por bloque. Con N=1048576 y 256 hilos/bloque, son 4096 valores parciales (el arreglo partials[]).

3. ¿Que pasaria si se elimina alguna sincronizacion dentro de la reduccion?
   Se romperia la garantia de orden entre escritura y lectura de cache[]: un hilo podria leer cache[tid+stride] antes de que el hilo dueno de esa posicion terminara de escribir su valor de la ronda anterior (condicion de carrera). El resultado seria no deterministico - a veces correcto, a veces no, dependiendo del orden real de ejecucion del hardware.

---

## Ejercicio C: softmax por fila (softmax/)

Cada bloque procesa una fila completa de la matriz: row = blockIdx.x. Como una fila puede tener mas columnas que hilos tiene el bloque, cada hilo recorre varias columnas con un bucle de salto:

    for (int col = tid; col < cols; col += blockDim.x)

El algoritmo hace dos reducciones (mismo patron de arbol que en el ejercicio B, reutilizado dos veces):

1. Reduccion de maximo (fmaxf) para encontrar row_max, el maximo de la fila.
2. Se calcula expf(input[idx] - row_max) para cada elemento (restar el maximo evita desbordamientos de expf), se guarda en output[idx] y se acumula en local_sum.
3. Reduccion de suma para encontrar row_sum, el total de exponenciales de la fila.
4. Normalizacion final: output[idx] /= row_sum.

Resultados obtenidos:

    softmax rows=128 cols=1024: OK
    softmax rows=256 cols=2048: OK

Respuestas:

1. ¿Por que se calcula primero el maximo de cada fila?
   Por estabilidad numerica. expf() de un numero grande se desborda (inf). Al restar el maximo antes de exponenciar, el exponente mas grande posible es 0, asi que expf(x - max) siempre da un valor entre 0 y 1 - nunca se desborda, sin importar la magnitud de los datos originales.

2. ¿Que partes del algoritmo requieren cooperacion entre hilos del mismo bloque?
   Las dos reducciones (de maximo y de suma): cada hilo calcula un valor parcial, pero combinarlos en un solo numero por fila requiere memoria compartida y __syncthreads().

3. ¿Que limitacion tiene usar un solo bloque por fila cuando cols crece mucho?
   Un bloque tiene un tope duro de 1024 hilos (aca se usan 256 fijos). Si cols crece mucho, cada hilo debe procesar mas columnas de forma secuencial dentro de su propio bucle, y el trabajo por fila deja de escalar con mas paralelismo. Ademas, un bloque entero corre en un solo streaming multiprocessor - una fila muy larga nunca puede aprovechar mas de un SM a la vez, sin importar cuantos SMs tenga la GPU disponibles.
