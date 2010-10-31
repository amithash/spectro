#include <stdio.h>
#include <math.h>

#include <fftw3.h>
#include "spect.h"

int spect_cepstrum(spect_t *out, spect_t *in)
{
  double t_spect_in[NBANDS];
  double t_spect_out[NBANDS];
  fftw_plan plan;
  int i,j;
  int rc = 0;
  if(!out || !in)
    return -1;


  plan = fftw_plan_r2r_1d(NBANDS, t_spect_in, t_spect_out,
                                FFTW_REDFT10, FFTW_ESTIMATE);


  memcpy(out->fname, in->fname, FNAME_LEN * sizeof(char));
  out->len = in->len;
  for(i = 0; i < in->len; i++) {

    /* copy array also perform log operation */
    for(j = 0; j < NBANDS; j++) {
      t_spect_in[j] = log((double)in->spect[j][i]);
    }

    fftw_execute(plan);

    /* Some processing will be required */
    for(j = 0; j < NBANDS; j++) {
	int val = ((int)t_spect_out[j]);
	val = val < 0 ? 0 : val;
	val = val > 255 ? 255 : val;
      out->spect[j][i] = val (unsigned char)val;
    }
  }

  fftw_destroy_plan(plan);

  return 0;

}
int spect_cepstrum_inplace(spect_t *inout)
{
  double t_spect_in[NBANDS];
  double t_spect_out[NBANDS];
  fftw_plan plan;
  int i,j;
  int rc = 0;
  if(!inout)
    return -1;


  plan = fftw_plan_r2r_1d(NBANDS, t_spect_in, t_spect_out,
                                FFTW_REDFT10, FFTW_ESTIMATE);


  for(i = 0; i < inout->len; i++) {

    /* copy array also perform log operation */
    for(j = 0; j < NBANDS; j++) {
      t_spect_in[i] = log(inout->spect[j][i]);
    }

    fftw_execute(plan);

    for(j = 0; j < NBANDS; j++) {
      inout->spect[j][i] = t_spect_out[j];
    }
  }

  fftw_destroy_plan(plan);

  return 0;
}



