#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <string.h>

#define Max(a, b) ((a) > (b) ? (a) : (b))
#define GHOST 3

#ifndef N
	#define N (2 * 2 * 2 * 2 * 2 * 2 + 2)
#endif

float maxeps = 0.1e-7f;
int itmax = 10;

void init(int i_start, int local_n, float A[local_n + 2 * GHOST][N][N])
{
	for (int li = 0; li < local_n; li++)
	{
		int i = i_start + li;
		int ai = li + GHOST;

		for (int j = 0; j <= N - 1; j++)
			for (int k = 0; k <= N - 1; k++)
			{
				if (i == 0 || i == N - 1 || j == 0 || j == N - 1 || k == 0 || k == N - 1)
					A[ai][j][k] = 0.0f;
				else
					A[ai][j][k] = 1.0f + i + j + k;
			}
	}
}

void pass_k(int i_start, int local_n, float A[local_n + 2 * GHOST][N][N])
{
	for (int li = 0; li < local_n; li++)
	{
		int i = i_start + li;
		int ai = li + GHOST;

		if (i < 1 || i > N - 2)
			continue;

		for (int j = 1; j <= N - 2; j++)
			for (int k = 3; k <= N - 4; k++)
			{
				A[ai][j][k] = (A[ai][j][k - 1] + A[ai][j][k + 1] + A[ai][j][k - 2] +
							   A[ai][j][k + 2] + A[ai][j][k - 3] + A[ai][j][k + 3]) /
							  6.0f;
			}
	}
}

void exchange_right_ghost(int rank, int size, int local_n, float A[local_n + 2 * GHOST][N][N])
{
	int left = (rank == 0) ? MPI_PROC_NULL : rank - 1;
	int right = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;

	/*
	 * Отправляем первые 3 свои плоскости левому соседу.
	 * Получаем первые 3 плоскости правого соседа.
	 *
	 * Это нужно перед проходом по i,
	 * потому что формула использует i+1, i+2, i+3.
	 */
	MPI_Sendrecv(&A[GHOST][0][0], // буфер отправки
				 GHOST * N * N,	  // сколько float отправляем
				 MPI_FLOAT,		  // тип данных
				 left,			  // кому отправляем
				 100,			  // тег отправки

				 &A[GHOST + local_n][0][0], // буфер приема
				 GHOST * N * N,				// сколько float принимаем
				 MPI_FLOAT,					// тип данных
				 right,						// от кого принимаем
				 100,						// тег приема

				 MPI_COMM_WORLD,   // коммуникатор
				 MPI_STATUS_IGNORE // статус не нужен
	);
}

void pass_i(int rank, int size, int i_start, int i_end, int local_n,
			float A[local_n + 2 * GHOST][N][N])
{
	int left = rank - 1;
	int right = rank + 1;

	/*
	 * Проход по i нельзя делать независимо на всех процессах.
	 * Есть зависимость от i-1, i-2, i-3.
	 *
	 * Поэтому rank 1 ждёт данные от rank 0,
	 * rank 2 ждёт данные от rank 1 и так далее.
	 */
	if (rank != 0)
	{
		MPI_Recv(&A[0][0][0],	   // сюда принимаем левые ghost-плоскости
				 GHOST * N * N,	   // 3 плоскости
				 MPI_FLOAT,		   // тип данных
				 left,			   // от левого соседа
				 200,			   // тег
				 MPI_COMM_WORLD,   // коммуникатор
				 MPI_STATUS_IGNORE // статус не нужен
		);
	}

	int first = Max(i_start, 3);
	int last = (i_end < N - 4) ? i_end : N - 4;

	for (int i = first; i <= last; i++)
	{
		int ai = i - i_start + GHOST;

		for (int j = 1; j <= N - 2; j++)
			for (int k = 1; k <= N - 2; k++)
			{
				A[ai][j][k] = (A[ai - 1][j][k] + A[ai + 1][j][k] + A[ai - 2][j][k] +
							   A[ai + 2][j][k] + A[ai - 3][j][k] + A[ai + 3][j][k]) /
							  6.0f;
			}
	}

	/*
	 * Отправляем правому соседу последние 3 уже обновлённые плоскости.
	 */
	if (rank != size - 1)
	{
		MPI_Send(&A[GHOST + local_n - GHOST][0][0], // последние 3 реальные плоскости
				 GHOST * N * N,						// 3 плоскости
				 MPI_FLOAT,							// тип данных
				 right,								// правому соседу
				 200,								// тег
				 MPI_COMM_WORLD						// коммуникатор
		);
	}
}

void pass_j(int i_start, int local_n, float A[local_n + 2 * GHOST][N][N], float *eps)
{
	for (int li = 0; li < local_n; li++)
	{
		int i = i_start + li;
		int ai = li + GHOST;

		if (i < 1 || i > N - 2)
			continue;

		for (int j = 3; j <= N - 4; j++)
			for (int k = 1; k <= N - 2; k++)
			{
				float e = A[ai][j][k];

				A[ai][j][k] = (A[ai][j - 1][k] + A[ai][j + 1][k] + A[ai][j - 2][k] +
							   A[ai][j + 2][k] + A[ai][j - 3][k] + A[ai][j + 3][k]) /
							  6.0f;

				*eps = Max(*eps, fabsf(e - A[ai][j][k]));
			}
	}
}

void relax(int rank, int size, int i_start, int i_end, int local_n,
		   float A[local_n + 2 * GHOST][N][N], float *eps)
{
	pass_k(i_start, local_n, A);

	exchange_right_ghost(rank, size, local_n, A);

	pass_i(rank, size, i_start, i_end, local_n, A);

	pass_j(i_start, local_n, A, eps);
}

float verify(int i_start, int local_n, float A[local_n + 2 * GHOST][N][N])
{
	float s = 0.0f;

	for (int li = 0; li < local_n; li++)
	{
		int i = i_start + li;
		int ai = li + GHOST;

		for (int j = 0; j <= N - 1; j++)
			for (int k = 0; k <= N - 1; k++)
			{
				s += A[ai][j][k] * (i + 1) * (j + 1) * (k + 1) / (N * N * N);
			}
	}

	return s;
}

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);

	int rank;
	int size;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	int i_start = (rank * N) / size;
	int i_end = ((rank + 1) * N) / size - 1;
	int local_n = i_end - i_start + 1;

	if (local_n < GHOST)
	{
		if (rank == 0)
			printf("Too many MPI processes for N=%d\n", N);

		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	/*
	 * Локальный массив процесса.
	 *
	 * Первые 3 плоскости — ghost слева.
	 * Потом local_n реальных плоскостей.
	 * Последние 3 плоскости — ghost справа.
	 */
	float A[local_n + 2 * GHOST][N][N];

	/*
	 * Обнуляем весь локальный массив.
	 * Это нужно, чтобы ghost-слои на краях были нулевыми.
	 */
	memset(A, 0, sizeof(A));

	if (rank == 0)
		printf("%d\n", N);

	init(i_start, local_n, A);

	MPI_Barrier(MPI_COMM_WORLD);
	double start = MPI_Wtime();

	for (int it = 1; it <= itmax; it++)
	{
		float eps_local = 0.0f;
		float eps_global = 0.0f;

		relax(rank, size, i_start, i_end, local_n, A, &eps_local);

		MPI_Allreduce(&eps_local,  // локальный eps
					  &eps_global, // общий eps
					  1,		   // одно число
					  MPI_FLOAT,   // тип float
					  MPI_MAX,	   // максимум
					  MPI_COMM_WORLD);

		if (rank == 0)
			printf("it=%4i   eps=%f\n", it, eps_global);

		if (eps_global < maxeps)
			break;
	}

	MPI_Barrier(MPI_COMM_WORLD);
	double end = MPI_Wtime();

	double local_time = end - start;
	double time = 0.0;

	MPI_Reduce(&local_time, &time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

	if (rank == 0)
		printf("time=%f\n", time);

	float s_local = verify(i_start, local_n, A);
	float s_global = 0.0f;

	MPI_Reduce(&s_local, &s_global, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);

	if (rank == 0)
		printf("  S = %f\n", s_global);

	MPI_Finalize();

	return 0;
}
