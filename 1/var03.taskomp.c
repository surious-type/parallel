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
	init();

	for(it=1; it<=itmax; it++)
	{
		eps = 0.;
		relax();
		printf( "it=%4i   eps=%f\n", it,eps);
		if (eps < maxeps) break;
	}

	verify();

	return 0;
}


void init()
{
    #pragma omp parallel for collapse(3) private(i,j,k) schedule(static)
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
    const int IB = 8;
    const int JB = 8;
    const int KB = 8;

    #pragma omp parallel
    {
        #pragma omp single
        {
            for (int k_layer = 3; k_layer <= N - 4; k_layer++)
            {
                for (int jb = 1; jb <= N - 2; jb += JB)
                for (int ib = 1; ib <= N - 2; ib += IB)
                {
                    int j0 = jb;
                    int j1 = (jb + JB - 1 <= N - 2) ? (jb + JB - 1) : (N - 2);
                    int i0 = ib;
                    int i1 = (ib + IB - 1 <= N - 2) ? (ib + IB - 1) : (N - 2);

                    #pragma omp task firstprivate(k_layer, j0, j1, i0, i1)
                    {
                        for (int j = j0; j <= j1; j++)
                        for (int i = i0; i <= i1; i++)
                        {
                            A[i][j][k_layer] =
                                (A[i][j][k_layer-1] + A[i][j][k_layer+1] +
                                 A[i][j][k_layer-2] + A[i][j][k_layer+2] +
                                 A[i][j][k_layer-3] + A[i][j][k_layer+3]) / 6.f;
                        }
                    }
                }
                #pragma omp taskwait
            }

            for (int i_layer = 3; i_layer <= N - 4; i_layer++)
            {
                for (int kb = 1; kb <= N - 2; kb += KB)
                for (int jb = 1; jb <= N - 2; jb += JB)
                {
                    int k0 = kb;
                    int k1 = (kb + KB - 1 <= N - 2) ? (kb + KB - 1) : (N - 2);
                    int j0 = jb;
                    int j1 = (jb + JB - 1 <= N - 2) ? (jb + JB - 1) : (N - 2);

                    #pragma omp task firstprivate(i_layer, k0, k1, j0, j1)
                    {
                        for (int k = k0; k <= k1; k++)
                        for (int j = j0; j <= j1; j++)
                        {
                            A[i_layer][j][k] =
                                (A[i_layer-1][j][k] + A[i_layer+1][j][k] +
                                 A[i_layer-2][j][k] + A[i_layer+2][j][k] +
                                 A[i_layer-3][j][k] + A[i_layer+3][j][k]) / 6.f;
                        }
                    }
                }
                #pragma omp taskwait
            }

            for (int j_layer = 3; j_layer <= N - 4; j_layer++)
            {
                float layer_eps = 0.f;

                #pragma omp taskgroup task_reduction(max: layer_eps)
                {
                    for (int kb = 1; kb <= N - 2; kb += KB)
                    for (int ib = 1; ib <= N - 2; ib += IB)
                    {
                        int k0 = kb;
                        int k1 = (kb + KB - 1 <= N - 2) ? (kb + KB - 1) : (N - 2);
                        int i0 = ib;
                        int i1 = (ib + IB - 1 <= N - 2) ? (ib + IB - 1) : (N - 2);

                        #pragma omp task firstprivate(j_layer, k0, k1, i0, i1) in_reduction(max: layer_eps)
                        {
                            float local_max = 0.f;

                            for (int k = k0; k <= k1; k++)
                            for (int i = i0; i <= i1; i++)
                            {
                                float oldv = A[i][j_layer][k];
                                float newv =
                                    (A[i][j_layer-1][k] + A[i][j_layer+1][k] +
                                     A[i][j_layer-2][k] + A[i][j_layer+2][k] +
                                     A[i][j_layer-3][k] + A[i][j_layer+3][k]) / 6.f;

                                A[i][j_layer][k] = newv;
                                local_max = Max(local_max, fabs(oldv - newv));
                            }

                            layer_eps = Max(layer_eps, local_max);
                        }
                    }
                }
                eps = Max(eps, layer_eps);
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
