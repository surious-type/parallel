## FOROMP VERSION

1. Write for omp version code of initial version code

2. Compile omp version:

gcc -O3 var03.foromp.c -fopenmp -lm -o var03.foromp

-O3 - level optimization
-fopenmp - include openmp library
-lm - include library libm.so

3. Test its version program

## TASKOMP VERSION

Параллелизация кода заключается в разбитие циклов на задачи.
Каждая задача группирует N количество итераций цикла.

1. Implementation omp task version code

2. Compile:

gcc -O3 var03.taskomp.c -fopenmp -lm -o var03.taskomp

3. Test
