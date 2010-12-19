#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef FDWT_TEST
#include <stdio.h>
#endif

#include "dwt.h"

#define _IS_POW_2(val) (((val) != 0) && !((val) & ((val) - 1)))

#define IS_POW_2(val) _IS_POW_2((unsigned int)val)

typedef void (*kernel_t)(dwt_plan_t plan, const int n);

typedef struct
{
	float 		*tmp_vec;
	int   		size;
	int		inplace;
	float		*user_in_vec;
	float 		*in_vec;
	float 		*out_vec;
	dwt_dir_t 	dir;
	float		h0, h1, h2, h3;
	float		g0, g1, g2, g3;
	float		Ih0, Ih1, Ih2, Ih3;
	float		Ig0, Ig1, Ig2, Ig3;
	kernel_t	kernel;
} dwt_plan_s_t;

static void transform( dwt_plan_t _plan, const int n)
{
	int i, j;
	const int half = n >> 1;
	dwt_plan_s_t *plan = (dwt_plan_s_t *)_plan;

	if (n < 4) 
	      return;

	for (i = 0, j = 0; j < n-3; j += 2, i++) {
		plan->tmp_vec[i]      = plan->in_vec[j]   * plan->h0 + 
		    			plan->in_vec[j+1] * plan->h1 + 
					plan->in_vec[j+2] * plan->h2 +
					plan->in_vec[j+3] * plan->h3;

		plan->tmp_vec[i+half] = plan->in_vec[j]   * plan->g0 +
		    			plan->in_vec[j+1] * plan->g1 +
					plan->in_vec[j+2] * plan->g2 +
					plan->in_vec[j+3] * plan->g3;
	}

	plan->tmp_vec[i]      = plan->in_vec[n-2] * plan->h0 +
	    			plan->in_vec[n-1] * plan->h1 +
				plan->in_vec[0]   * plan->h2 +
				plan->in_vec[1]   * plan->h3;

	plan->tmp_vec[i+half] = plan->in_vec[n-2] * plan->g0 +
	    			plan->in_vec[n-1] * plan->g1 +
				plan->in_vec[0]   * plan->g2 +
				plan->in_vec[1]   * plan->g3;

	memcpy(plan->in_vec,  plan->tmp_vec, sizeof(float) * n);
}

static void itransform( dwt_plan_t _plan, const int n )
{
       int i, j;
       const int half = n >> 1;
       const int halfPls1 = half + 1;
       dwt_plan_s_t *plan = (dwt_plan_s_t *)_plan;

       if(n < 4)
	     return;

	//      last smooth val  last coef.  first smooth  first coef
	plan->tmp_vec[0] = plan->in_vec[half-1] * plan->Ih0 + 
	    		   plan->in_vec[n-1]    * plan->Ih1 +
			   plan->in_vec[0]      * plan->Ih2 +
			   plan->in_vec[half]   * plan->Ih3;

	plan->tmp_vec[1] = plan->in_vec[half-1] * plan->Ig0 +
	    		   plan->in_vec[n-1]    * plan->Ig1 +
			   plan->in_vec[0]      * plan->Ig2 +
			   plan->in_vec[half]   * plan->Ig3;
	for (i = 0, j = 2; i < half-1; i++) {
		//     smooth val     coef. val       smooth val    coef. val
		plan->tmp_vec[j++] = plan->in_vec[i]          * plan->Ih0 +
		    		     plan->in_vec[i+half]     * plan->Ih1 +
				     plan->in_vec[i+1]        * plan->Ih2 +
				     plan->in_vec[i+halfPls1] * plan->Ih3;

		plan->tmp_vec[j++] = plan->in_vec[i]          * plan->Ig0 +
		                     plan->in_vec[i+half]     * plan->Ig1 +
				     plan->in_vec[i+1]        * plan->Ig2 +
				     plan->in_vec[i+halfPls1] * plan->Ig3;
	}
	memcpy(plan->in_vec, plan->tmp_vec, sizeof(float) * n);
}


dwt_plan_t dwt_create_plan(int size, float *in_vec, float *out_vec, dwt_dir_t dir)
{
	dwt_plan_s_t *plan;
	const double sqrt_3 = sqrt( 3 );
	const double denom = 4 * sqrt( 2 );
	kernel_t kernel;

	if(!in_vec || !out_vec)
	      return NULL;
	if(!IS_POW_2(size))
	      return NULL;

	switch(dir) {
		case DWT_FORWARD:
			kernel = transform;
			break;
		case DWT_REVERSE:
			kernel = itransform;
			break;
		default:
			return NULL;
	}

	plan = (dwt_plan_s_t *) malloc(sizeof(dwt_plan_s_t));
	if(!plan)
	      return NULL;

	plan->size     	  = size;
	plan->dir         = dir;
	plan->kernel	  = kernel;
	plan->tmp_vec     = calloc(size, sizeof(float));
	if(!plan->tmp_vec)
	      goto tmp_vec_failed;

	plan->out_vec     = out_vec;
	plan->user_in_vec = in_vec;
	if(in_vec == out_vec) {
		plan->inplace = 1;
		plan->in_vec = in_vec;
	} else {
		plan->inplace = 0;
		plan->in_vec	  = calloc(size, sizeof(float));
		if(!plan->in_vec)
		      goto in_vec_failed;
	}

	// forward transform scaling (smoothing) coefficients
	plan->h0 = (1 + sqrt_3)/denom;
	plan->h1 = (3 + sqrt_3)/denom;
	plan->h2 = (3 - sqrt_3)/denom;
	plan->h3 = (1 - sqrt_3)/denom;

	// forward transform wavelet coefficients
	plan->g0 =  plan->h3;
	plan->g1 = -1 * plan->h2;
	plan->g2 =  plan->h1;
	plan->g3 = -1 * plan->h0;

	plan->Ih0 = plan->h2;
	plan->Ih1 = plan->g2;  // h1
	plan->Ih2 = plan->h0;
	plan->Ih3 = plan->g0;  // h3

	plan->Ig0 = plan->h3;
	plan->Ig1 = plan->g3;  // -h0
	plan->Ig2 = plan->h1;
	plan->Ig3 = plan->g1;  // -h2

	return (dwt_plan_t)plan;

in_vec_failed:
	free(plan->tmp_vec);
tmp_vec_failed:
	free(plan);
	return NULL;
}

void dwt_destroy_plan(dwt_plan_t _plan)
{
	dwt_plan_s_t *plan = (dwt_plan_s_t *)_plan;

	if(!plan)
	      return;
	if(plan->tmp_vec)
	      free(plan->tmp_vec);
	if(plan->inplace == 0 && plan->in_vec)
	      free(plan->in_vec);
	free(plan);
}

void dwt_execute(dwt_plan_t _plan)
{
	int n;
	dwt_plan_s_t *plan = (dwt_plan_s_t *)_plan;
	if(!plan)
	      return;
	if(!plan->inplace)
		memcpy(plan->in_vec, plan->user_in_vec, sizeof(float) * plan->size);
	switch(plan->dir) {
		case DWT_FORWARD:
			for(n = plan->size; n >= 4; n >>= 1) {
				plan->kernel(_plan, n);
			}
			break;
		case DWT_REVERSE:
			for(n = 4; n <= plan->size; n <<= 1) {
				plan->kernel(_plan, n);
			}
			break;
		default:
			return;
	}
	if(!plan->inplace)
		memcpy(plan->out_vec, plan->in_vec, sizeof(float) * plan->size);
}

#ifdef FDWT_TEST
void print_arr(float *arr, int len)
{
	int i;
	if(len == 0)
	      return;
	printf("%.2f", arr[0]);
	for(i = 1; i < len; i++) {
		printf(" %.2f", arr[i]);
	}
	printf("\n");
}
int main(void)
{
	float in[8] = {0,1,2,3,4,5,6,7};
	float out[8];
	dwt_plan_t for_plan;
	dwt_plan_t rev_plan;
	for_plan = dwt_create_plan(8, in, out, DWT_FORWARD);
	rev_plan = dwt_create_plan(8, out, out, DWT_REVERSE);
	if(!for_plan || !rev_plan) {
		printf("Plan creation failed\n");
		exit(-1);
	}
	printf("Doing forward\n");
	printf("Input: ");
	print_arr(in, 8);
	dwt_execute(for_plan);
	printf("Output: ");
	print_arr(out, 8);
	printf("Doing Reverse\n");
	dwt_execute(rev_plan);
	printf("Output: ");
	print_arr(out, 8);
	dwt_destroy_plan(for_plan);
	dwt_destroy_plan(rev_plan);

	return 0;
}
#endif
