#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#define  Max(a,b) ((a)>(b)?(a):(b))

#define  N   (2*2*2*2*2*2+2)
float   maxeps = 0.1e-7;
int itmax = 100;
int i,j,k;

float eps;
float A [N][N][N];

void relax();
void init();
void verify(); 

int main(int an, char **as)
{
	int it;
    double relax_total = 0.0;
	init();

	for(it=1; it<=itmax; it++)
	{
		eps = 0.;
        double t0 = omp_get_wtime();
		relax();
        double t1 = omp_get_wtime();
        relax_total += (t0 - t1);
		printf( "it=%4i   eps=%f relax_time=%f\n", it, eps, t0 - t1);
		if (eps < maxeps) break;
	}

    printf("Time=%f\n", relax_total);

	verify();

	return 0;
}

void init()
{
	for(k=0; k<=N-1; k++)
	for(j=0; j<=N-1; j++)
	for(i=0; i<=N-1; i++)
	{
		if(i==0 || i==N-1 || j==0 || j==N-1 || k==0 || k==N-1) A[i][j][k] = 0.;
		else A[i][j][k] = ( 1. + i + j + k ) ;
	}
} 

void relax()
{
    #pragma omp parallel
    {

        for(k=3; k<=N-4; k++)
        {
            #pragma omp for collapse(2) private(i,j) schedule(static)
            for(j=1; j<=N-2; j++)
            for(i=1; i<=N-2; i++)
            {
                A[i][j][k] = (A[i][j][k-1]+A[i][j][k+1]+A[i][j][k-2]+A[i][j][k+2]+A[i][j][k-3]+A[i][j][k+3])/6.f;
            }
        }	

        for(i=3; i<=N-4; i++)
        {
            #pragma omp for collapse(2) private(k,j) schedule(static)
            for(k=1; k<=N-2; k++)
            for(j=1; j<=N-2; j++)
            {
                A[i][j][k] = (A[i-1][j][k]+A[i+1][j][k]+A[i-2][j][k]+A[i+2][j][k]+A[i-3][j][k]+A[i+3][j][k])/6.;
            }
        }

        for(j=3; j<=N-4; j++)
        {
            #pragma omp for collapse(2) private(k,i) reduction(max:eps) schedule(static)
            for(k=1; k<=N-2; k++)
            for(i=1; i<=N-2; i++)
            {
                float e;
                e=A[i][j][k];
                A[i][j][k] =(A[i][j-1][k]+A[i][j+1][k]+A[i][j-2][k]+A[i][j+2][k]+A[i][j-3][k]+A[i][j+3][k])/6.;
                eps=Max(eps,fabs(e-A[i][j][k]));
            }
        }
    }
}

void verify()
{ 
	float s;

	s=0.;

	for(k=0; k<=N-1; k++)
	for(j=0; j<=N-1; j++)
	for(i=0; i<=N-1; i++)
	{
		s=s+A[i][j][k]*(i+1)*(j+1)*(k+1)/(N*N*N);
	}
	printf("  S = %f\n",s);
}
