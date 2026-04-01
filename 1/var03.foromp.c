#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#define  Max(a,b) ((a)>(b)?(a):(b))

#define  N   (2*2*2*2*2*2+2)
float   maxeps = 0.1e-7;
int itmax = 100;

float eps;
float A [N][N][N];

void relax();
void init();
void verify(); 

int main(int an, char **as)
{
    init();

    for(int it = 1; it <= itmax; it++)
    {
        eps = 0.;

        relax();
        if (it%25 == 0)
        {
            printf("it=%d eps=%f\n", it, eps);
        }

        if (eps < maxeps) break;
    }

    verify();

    return 0;
}

void init()
{
	for(int k = 0; k <= N - 1; k++)
	for(int j = 0; j <= N - 1; j++)
	for(int i = 0; i <= N - 1; i++)
	{
		if(i==0 || i==N-1 || j==0 || j==N-1 || k==0 || k==N-1) A[i][j][k] = 0.;
		else A[i][j][k] = ( 1. + i + j + k ) ;
	}
} 

void relax()
{
    float layer_eps = 0.0f;
    #pragma omp parallel
    {
        for(int k = 3; k <= N - 4; k++)
        {
            #pragma omp for schedule(static)
            for(int i = 1; i <= N - 2; i++)
            for(int j = 1; j <= N - 2; j++)
            {
                A[i][j][k] = (A[i][j][k-1]+A[i][j][k+1]+A[i][j][k-2]+A[i][j][k+2]+A[i][j][k-3]+A[i][j][k+3])/6.f;
            }
        }	

        for(int i = 3; i <= N - 4; i++)
        {
            #pragma omp for schedule(static)
            for(int j = 1; j <= N - 2; j++)
            for(int k = 1; k <= N - 2; k++)
            {
                A[i][j][k] = (A[i-1][j][k]+A[i+1][j][k]+A[i-2][j][k]+A[i+2][j][k]+A[i-3][j][k]+A[i+3][j][k])/6.;
            }
        }

        for(int j = 3; j <= N - 4; j++)
        {
            #pragma omp for reduction(max:layer_eps) schedule(static)
            for (int i = 1; i <= N - 2; i++)
            for (int k = 1; k <= N - 2; k++)
            {
                float e = A[i][j][k];
                A[i][j][k] =
                    (A[i][j-1][k] + A[i][j+1][k] +
                     A[i][j-2][k] + A[i][j+2][k] +
                     A[i][j-3][k] + A[i][j+3][k]) / 6.0f;

                layer_eps = Max(layer_eps, fabsf(e - A[i][j][k]));
            }
        }
    }
    eps = layer_eps;
}

void verify()
{ 
	float s;

	s=0.;

	for(int k = 0; k <= N - 1; k++)
	for(int j = 0; j <= N - 1; j++)
	for(int i = 0; i <= N - 1; i++)
	{
		s=s+A[i][j][k]*(i+1)*(j+1)*(k+1)/(N*N*N);
	}
	printf("  S = %f\n",s);
}
