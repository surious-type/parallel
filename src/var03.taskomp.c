#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Min(a, b) ((a) < (b) ? (a) : (b))

#ifndef N
	#define N (2 * 2 * 2 * 2 * 2 * 2 + 2)
#endif
#ifndef THREADS
	#define THREADS 1
#endif

float maxeps = 0.1e-7;
int itmax = 10;

float eps;
float A[N][N][N];

void relax();
void init();
void verify();

int main(int an, char **as)
{
	omp_set_num_threads(THREADS);

	init();

	double start = omp_get_wtime();

	for (int it = 1; it <= itmax; it++)
	{
		eps = 0.0f;

		relax();

		printf("it=%d eps=%f\n", it, eps);

		if (eps < maxeps)
			break;
	}

	double end = omp_get_wtime();

	printf("time=%f\n", end - start);

	verify();

	return 0;
}

void init()
{
	for (int i = 0; i <= N - 1; i++)
		for (int j = 0; j <= N - 1; j++)
			for (int k = 0; k <= N - 1; k++)
			{
				if (i == 0 || i == N - 1 || j == 0 || j == N - 1 || k == 0 || k == N - 1)
					A[i][j][k] = 0.0f;
				else
					A[i][j][k] = (1.0f + i + j + k);
			}
}

void relax()
{
	int block = (N - 2) / THREADS * 2; // * 2, как минимум 2 задачи на нить

	if (block <= 8)
		block = 8;
	else if (block <= 16)
		block = 16;
	else if (block <= 32)
		block = 32;
	else
		block = 64;

	// фаза 1: хорошая локальность - последовательная память
	int IB1 = block;
	int JB1 = block;

	// фаза 2: плохая локальность, уменьшаем рабочий набор - N прыжки по памяти
	int KB2 = block > 8 ? block / 2 : 8;
	int JB2 = block > 8 ? block / 2 : 8;

	// фаза 3: средняя локальность - N * N - прыжки по памяти
	int KB3 = block;
	int IB3 = block;

#pragma omp parallel
	{
#pragma omp single
		{
#pragma omp taskgroup
			for (int ib = 1; ib <= N - 2; ib += IB1)
				for (int jb = 1; jb <= N - 2; jb += JB1)
				{
					int j0 = jb;
					int j1 = Min(jb + JB1 - 1, N - 2);
					int i0 = ib;
					int i1 = Min(ib + IB1 - 1, N - 2);
#pragma omp task firstprivate(j0, j1, i0, i1)
					{
						for (int i = i0; i <= i1; i++)
							for (int j = j0; j <= j1; j++)
								for (int k = 3; k <= N - 4; k++)
								{
									A[i][j][k] =
										(A[i][j][k - 1] + A[i][j][k + 1] + A[i][j][k - 2] +
										 A[i][j][k + 2] + A[i][j][k - 3] + A[i][j][k + 3]) /
										6.0f;
								}
					}
				}
		}
	}

#pragma omp parallel
	{
#pragma omp single
		{
#pragma omp taskgroup
			for (int jb = 1; jb <= N - 2; jb += JB2)
				for (int kb = 1; kb <= N - 2; kb += KB2)
				{
					int k0 = kb;
					int k1 = Min(kb + KB2 - 1, N - 2);
					int j0 = jb;
					int j1 = Min(jb + JB2 - 1, N - 2);
#pragma omp task firstprivate(k0, k1, j0, j1)
					{
						for (int i = 3; i <= N - 4; i++)
							for (int j = j0; j <= j1; j++)
								for (int k = k0; k <= k1; k++)
								{
									A[i][j][k] =
										(A[i - 1][j][k] + A[i + 1][j][k] + A[i - 2][j][k] +
										 A[i + 2][j][k] + A[i - 3][j][k] + A[i + 3][j][k]) /
										6.0f;
								}
					}
				}
		}
	}
#pragma omp parallel
	{
#pragma omp single
		{
#pragma omp taskgroup
			{
				for (int ib = 1; ib <= N - 2; ib += IB3)
					for (int kb = 1; kb <= N - 2; kb += KB3)
					{
						int k0 = kb;
						int k1 = Min(kb + KB3 - 1, N - 2);
						int i0 = ib;
						int i1 = Min(ib + IB3 - 1, N - 2);

#pragma omp task firstprivate(k0, k1, i0, i1) shared(eps)
						{
							float local_max = 0.0f;

							for (int i = i0; i <= i1; i++)
								for (int j = 3; j <= N - 4; j++)
									for (int k = k0; k <= k1; k++)
									{
										float oldv = A[i][j][k];
										float newv =
											(A[i][j - 1][k] + A[i][j + 1][k] + A[i][j - 2][k] +
											 A[i][j + 2][k] + A[i][j - 3][k] + A[i][j + 3][k]) /
											6.0f;

										A[i][j][k] = newv;
										local_max = Max(local_max, fabsf(oldv - newv));
									}

#pragma omp critical
							{
								eps = Max(eps, local_max);
							}
						}
					}
			}
		}
	}
}

void verify()
{
	float s;

	s = 0.0f;

	for (int k = 0; k <= N - 1; k++)
		for (int j = 0; j <= N - 1; j++)
			for (int i = 0; i <= N - 1; i++)
			{
				s = s + A[i][j][k] * (i + 1) * (j + 1) * (k + 1) / (N * N * N);
			}
	printf("  S = %f\n", s);
}
