#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

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

void relax(int rank, int size, int i_start, int i_end, int local_n,
		   float A[local_n + 2 * GHOST][N][N], float *eps)
{
	/*
	 * Первый проход.
	 *
	 * Обновление по направлению k.
	 * MPI-обмен не нужен, потому что каждый процесс хранит полную ось k.
	 */
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

	/*
	 * Перед вторым проходом.
	 *
	 * Для вычисления по i нужны старые значения i+1, i+2, i+3.
	 * Поэтому каждый процесс получает первые 3 плоскости правого соседа
	 * в свой правый ghost.
	 */
	int old_left = (rank == 0) ? MPI_PROC_NULL : rank - 1;
	int old_right = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;

	MPI_Sendrecv(&A[GHOST][0][0], // первые 3 реальные плоскости отправляем влево
				 GHOST * N * N,				// количество float-элементов
				 MPI_FLOAT,					// тип данных
				 old_left,					// левый сосед
				 100,						// тег отправки
				 &A[GHOST + local_n][0][0], // сюда принимаем правый ghost
				 GHOST * N * N, // количество принимаемых float-элементов
				 MPI_FLOAT,		// тип данных
				 old_right,		// правый сосед
				 100,			// тег приема
				 MPI_COMM_WORLD,   // коммуникатор
				 MPI_STATUS_IGNORE // статус не используем
	);

	/*
	 * Второй проход.
	 *
	 * Обновление по направлению i.
	 * Здесь есть зависимость от уже обновленных i-1, i-2, i-3.
	 *
	 * Поэтому процессы идут конвейером:
	 * rank 0 считает первым,
	 * rank 1 ждёт rank 0,
	 * rank 2 ждёт rank 1,
	 * и так далее.
	 */
	int pipe_left = rank - 1;
	int pipe_right = rank + 1;

	if (rank != 0)
	{
		MPI_Recv(&A[0][0][0], // сюда принимаем левые обновленные ghost-плоскости
				 GHOST * N * N,	   // 3 плоскости по N*N float
				 MPI_FLOAT,		   // тип данных
				 pipe_left,		   // левый сосед
				 200,			   // тег сообщения
				 MPI_COMM_WORLD,   // коммуникатор
				 MPI_STATUS_IGNORE // статус не используем
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

	if (rank != size - 1)
	{
		MPI_Send(
			&A[GHOST + local_n - GHOST][0][0], // последние 3 уже обновленные реальные плоскости
			GHOST * N * N, // 3 плоскости по N*N float
			MPI_FLOAT,	   // тип данных
			pipe_right,	   // правый сосед
			200,		   // тег сообщения
			MPI_COMM_WORLD // коммуникатор
		);
	}

	/*
	 * Третий проход.
	 *
	 * Обновление по направлению j.
	 * MPI-обмен не нужен, потому что каждый процесс хранит полную ось j.
	 * Здесь же считаем eps.
	 */
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

float verify(int i_start, int i_end, int local_n, float A[local_n + 2 * GHOST][N][N])
{
	float s = 0.0f;

	for (int k = 0; k <= N - 1; k++)
		for (int j = 0; j <= N - 1; j++)
			for (int i = i_start; i < i_end; i++)
			{
				int ai = i - i_start + GHOST;

				s += A[ai][j][k] * (i + 1) * (j + 1) * (k + 1) / (N * N * N);
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
			fprintf(stderr, "Много процессов для N=%d\n", N);

		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	float(*A)[N][N] = calloc(local_n + 2 * GHOST, sizeof(*A));

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

		MPI_Allreduce(&eps_local, &eps_global, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD);

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

	float s_local = verify(i_start, i_end, local_n, A);
	float s_global = 0.0f;

	MPI_Reduce(&s_local, &s_global, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);

	if (rank == 0)
		printf("  S = %f\n", s_global);

	free(A);
	MPI_Finalize();

	return 0;
}
