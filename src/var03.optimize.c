#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#define Max(a, b) ((a) > (b) ? (a) : (b))

#ifndef N
	#define N (2 * 2 * 2 * 2 * 2 * 2 + 2)
#endif

float maxeps = 0.1e-7;
int itmax = 100;

float eps;
float A[N][N][N];

void relax();
void init();
void verify();

int main(int an, char **as)
{
	printf("%d\n", N);
	int it;
	init();

	double start = omp_get_wtime();

	for (it = 1; it <= itmax; it++)
	{
		eps = 0.;
		relax(A);
		printf("it=%4i   eps=%f\n", it, eps);
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
	for (int k = 0; k <= N - 1; k++)
		for (int j = 0; j <= N - 1; j++)
			for (int i = 0; i <= N - 1; i++)
			{
				if (i == 0 || i == N - 1 || j == 0 || j == N - 1 || k == 0 || k == N - 1)
					A[i][j][k] = 0.f;
				else
					A[i][j][k] = (1.f + i + j + k);
			}
}

void relax(float A[N][N][N])
{
	for (int i = 1; i <= N - 2; i++)
		for (int j = 1; j <= N - 2; j++)
			for (int k = 3; k <= N - 4; k++)
			{
				A[i][j][k] = (A[i][j][k - 1] + A[i][j][k + 1] + A[i][j][k - 2] + A[i][j][k + 2] +
							  A[i][j][k - 3] + A[i][j][k + 3]) /
							 6.f;
			}

	for (int j = 1; j <= N - 2; j++)
		for (int k = 1; k <= N - 2; k++)
			for (int i = 3; i <= N - 4; i++)
			{
				A[i][j][k] = (A[i - 1][j][k] + A[i + 1][j][k] + A[i - 2][j][k] + A[i + 2][j][k] +
							  A[i - 3][j][k] + A[i + 3][j][k]) /
							 6.f;
			}

	for (int i = 1; i <= N - 2; i++)
		for (int k = 1; k <= N - 2; k++)
			for (int j = 3; j <= N - 4; j++)
			{
				float e;
				e = A[i][j][k];
				A[i][j][k] = (A[i][j - 1][k] + A[i][j + 1][k] + A[i][j - 2][k] + A[i][j + 2][k] +
							  A[i][j - 3][k] + A[i][j + 3][k]) /
							 6.f;
				eps = Max(eps, fabs(e - A[i][j][k]));
			}
}

void verify()
{
	float s;

	s = 0.f;
	for (int k = 0; k <= N - 1; k++)
		for (int j = 0; j <= N - 1; j++)
			for (int i = 0; i <= N - 1; i++)
			{
				s = s + A[i][j][k] * (i + 1) * (j + 1) * (k + 1) / (N * N * N);
			}
	printf("  S = %f\n", s);
}
