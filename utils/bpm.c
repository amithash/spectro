/*******************************************************************************
    This file is part of spectro
    Copyright (C) 2010  Amithash Prasad <amithash@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/

#include "bpm.h"
#include <fftw3.h>
#include <tag_c.h>
#include <time.h>
#include <float.h>

#define FREQ2BPM(f,len,sr) ((unsigned int)(30 * (((float)sr) * ((float)f / (float)(len - 1)))))

#define PREC 0.000001
#define FLOAT_EQUAL(val1, val2) (((val1) < ((val2) + PREC)) && ((val1 > ((val2) - PREC))))

#define MAX_PEAKS 20
#define BPM 0
#define PROB 1

void smooth_vec(float *vec, unsigned int len, unsigned int window)
{
	int i,j;
	for(i = 0; i < len - window; i++) {
		float val = 0;
		for(j = i; j < i + window; j++) {
			val += vec[j];
		}
		vec[i] = val / (float)window;
	}
	for(i = len - window; i < len; i++) {
		float count = 0, val = 0;
		for(j = i; j < len; j++) {
			val += vec[j];
			count = count + 1;
		}
		vec[i] = count > 0 ? val / count : vec[i];
	}
}


int find_peaks(float peaks[MAX_PEAKS][2], float *bpm, int len)
{
  int i,j;
  float *tmp_peaks;
  float sum = 0;
  unsigned int cur_peak = 0;
  for(i = 0; i < MAX_PEAKS; i++) {
    peaks[i][BPM] = peaks[i][PROB] = 0;
  }
  tmp_peaks = (float *)calloc(len, sizeof(float));
  if(!tmp_peaks)
    return -1;
  for(i = 1; i < len - 1; i++) {
    if(bpm[i] > bpm[i-1] && bpm[i] > bpm[i+1]) {
      tmp_peaks[i] = 1;
    } else {
      tmp_peaks[i] = 0;
    }
  }
  tmp_peaks[0] = tmp_peaks[len - 1] = 0;
  for(i = 0; i < len; i++) {
    bpm[i] = bpm[i] * tmp_peaks[i];
  }
  free(tmp_peaks);
  for(i = 0; i < len; i++) {
    sum += bpm[i];
  }
  /* empty music file */
  if(!sum)
    return 0;

  for(i = 0; i < len; i++) {
    bpm[i] = bpm[i] / sum;
  }


  for(i = 0; i < len; i++) {
    if(bpm[i] > 0) {
      peaks[cur_peak][BPM] = i + BPM_MIN;
      peaks[cur_peak][PROB] = bpm[i];
      cur_peak++;
      if(cur_peak >= MAX_PEAKS)
        break;
    }
  }
  for(i = 0; i < MAX_PEAKS; i++) {
  	for(j = 0; j < MAX_PEAKS; j++) {
		if(peaks[i][1] > peaks[j][1]) {
			float tmp_bpm = peaks[i][0];
			float tmp_pro = peaks[i][1];
			peaks[i][0] = peaks[j][0];
			peaks[i][1] = peaks[j][1];
			peaks[j][0] = tmp_bpm;
			peaks[j][1] = tmp_pro;
		}
	}
  }

  return 0;
}

int is_zero(spect_t *spect, int ind)
{
	int i; 
	for(i = 0; i < NBANDS; i++) {
		if(!FLOAT_EQUAL(spect->spect[i][ind], 0.0)) {
			return 0;
		}
	}
	return 1;
}

void bounds(unsigned int *_start, unsigned int *_end, spect_t *spect)
{
	int i;
	unsigned int start, end;
	for(i = 0; i < spect->len; i++) {
		if(!is_zero(spect, i)) {
			break;
		}
	}
	start = i;
	for(i = spect->len - 1; i > start; i--) {
		if(!is_zero(spect, i)) {
			break;
		}
	}
	end = i;

	start = start + 128;
	end   = end   - 128;

	start = (start >> 2) << 2;
	end   = (end >> 2) << 2;

	*_start = start;
	*_end   = end;
}

void free_out(float **out)
{
	int i;
	if(!out)
	      return;
	for(i = 0; i < NBANDS; i++) {
		if(out[i])
		      free(out[i]);
	}
	free(out);
}

float **alloc_out(int len)
{
	float **out;
	int i;
	out = (float **)calloc(NBANDS, sizeof(float *));
	if(!out)
	      return NULL;
	for(i = 0; i < NBANDS; i++) {
		out[i] = NULL;
	}
	for(i = 0; i < NBANDS; i++) {
		out[i] = (float *)calloc(len, sizeof(float));
		if(!out[i]) {
		      free_out(out);
		      return NULL;
		}
	}
	return out;
}

int hp_filter(float *vec, int len, float cut)
{
  fftwf_plan plan;
  fftwf_complex *out;
  unsigned int out_len = (len / 2) + 1;
  unsigned int end =  (unsigned int)((float)out_len * cut);
  unsigned int pre_end = end - (out_len / 10);
  float scale;
  int i;
  if(cut < 0 || cut >= 1.0)
    return -1;
  out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * out_len);
  if(!out)
    return -1;
  plan = fftwf_plan_dft_r2c_1d(len, vec, out, FFTW_ESTIMATE);
  fftwf_execute(plan);
  fftwf_destroy_plan(plan);
  for(i = 0; i < pre_end; i++) {
    out[i][0] = out[i][1] = 0;
  }
  for(i = pre_end; i < end; i++) {
    scale = (float)(i - pre_end) / (float)(end - pre_end);
    out[i][0] *= scale;
    out[i][1] *= scale;
   
  }
  /*XXX: Check if the parmeter should be out_len instead of len */
  memset(vec, 0, sizeof(float)*len);
  plan = fftwf_plan_dft_c2r_1d(len, out, vec, FFTW_ESTIMATE);
  fftwf_execute(plan);
  fftwf_destroy_plan(plan);
  return 0;
}

int spect_fft(int *_out_len, float ***_out, spect_t *spect)
{
	fftwf_plan fftw_plan;
	float      *fftw_in = NULL;
	float      *fftw_out = NULL;
	float      **out = NULL;
	int        rc = -1;
	int        i,j;
	int        out_len;
	unsigned int start, end;
	unsigned int real_len;
	bounds(&start, &end, spect);

	real_len = end - start;

	out_len = (real_len/2) + 1;

	if(!(out = alloc_out(out_len)))
	      goto out_alloc_failed;

	fftw_in  = (float *) fftwf_malloc (sizeof(float) * real_len);
	fftw_out = (float *) fftwf_malloc (sizeof(fftwf_complex) * out_len);

	if(!fftw_in || !fftw_out)
	      goto malloc_failed;


	*_out = out;
	fftw_plan = fftwf_plan_dft_r2c_1d(real_len, fftw_in, 
					(fftwf_complex *) fftw_out, 
					FFTW_ESTIMATE);
	for(i = 0; i < NBANDS; i++) {
		smooth_vec(&spect->spect[i][start], real_len, 2);
		for(j = start; j < end; j++) {
			fftw_in[j - start] = spect->spect[i][j];
		}

		fftwf_execute(fftw_plan);

		for(j = 0; j < 2*out_len; j+=2) {
			float re = fftw_out[j];
			float im = fftw_out[j+1];
			float abs = sqrt((re*re) + (im *im)) / sqrt((float)real_len);
			out[i][j/2] = abs;
		}
	}

	fftwf_destroy_plan (fftw_plan);
	*_out_len = out_len;
	rc = 0;

malloc_failed:
	if(fftw_in)
	      fftwf_free(fftw_in);
	if(fftw_out)
	      fftwf_free(fftw_out);
	if(out && rc != 0)
	      free_out(out);
out_alloc_failed:

	return rc;
}

unsigned int get_sampling_rate(spect_t *spect)
{
	TagLib_File *tf;
	unsigned int seconds;
	float rate;

	tf = taglib_file_new(spect->fname);
	seconds = taglib_audioproperties_length(taglib_file_audioproperties(tf));
	taglib_file_free(tf);
	if(seconds == 0) {
		return 43;
	}
	rate =  (((float)spect->len) / ((float)seconds)) + 0.5;
	return (unsigned int)rate;
}

int _spect2bpm(float bpm[NBANDS][BPM_LEN], spect_t *spect)
{
	float bpm_nr[NBANDS][BPM_LEN];
	unsigned int sampling_rate;
	int rc = -1;
	int i, band;
	int out_len;
	float **out;
	
	sampling_rate = get_sampling_rate(spect);

	for(i = 0; i < NBANDS; i++) {
		memset(bpm[i], 0, sizeof(float) * BPM_LEN);
		memset(bpm_nr[i], 0, sizeof(float) * BPM_LEN);
	}

	if(spect_fft(&out_len, &out, spect))
		goto spect_fft_failed;

	for(band = 0; band < NBANDS; band++) {
		float min = FLT_MAX;
		float total = 0;
		for(i = 1; i < out_len; i++) {
			unsigned int b = FREQ2BPM(i, out_len, sampling_rate);
			if(b >= BPM_LEN + BPM_MIN)
			      continue;
			if(b < BPM_MIN)
			      continue;
			b = b - BPM_MIN;
			bpm[band][b] += out[band][i];
			bpm_nr[band][b] += 1;
		}
		for(i = 0; i < BPM_LEN; i++) {
			if(bpm_nr[band][i] > 1) {
				bpm[band][i] = bpm[band][i] / bpm_nr[band][i];
			}
			total += bpm[band][i];
			if(bpm[band][i] > 0 && bpm[band][i] < min)
			      min = bpm[band][i];
		}
		total = total - (BPM_LEN * min);

		for(i = 0; i < BPM_LEN; i++) {
			if(bpm[band][i] > 0)
				bpm[band][i] = bpm[band][i] - min;
			bpm[band][i] /= total;
		}
	}

	rc = 0;
	free_out(out);

spect_fft_failed:
	return rc;
}

#define ABS(val) ((val) > 0 ? (val) : -1.0 * (val))
#define START_BAND 8
#define END_BAND   19
int spect2bpm(float bpm[BPM_LEN], spect_t *spect)
{
	float _bpm[NBANDS][BPM_LEN];
	float mean = 0;
	int rc;
	float total = 0;
	int i,j;
	rc = _spect2bpm(_bpm, spect);
	if(rc != 0) {
		return rc;
	}
	for(i = 0; i < BPM_LEN; i++) {
		bpm[i] = 0;
	}

	for(i = START_BAND; i < END_BAND; i++) {
		for(j = 0; j < BPM_LEN; j++) {
			bpm[j] += _bpm[i][j];
		}
	}
	return 0;
}



void plot_bpm(float bpm[BPM_LEN])
{
	FILE *f;
	int i,j;
	float mean = 0;
	float peaks[MAX_PEAKS][2];
	f = fopen("__out.txt", "w");
	if(!f) {
		spect_error("FOPEN failed!");
		return;
	}
	for(i = 0; i < BPM_LEN; i++) {
		int b = i + BPM_MIN;
		fprintf(f, "%d %f\n", b,  bpm[i]);
	}
	fclose(f);
	
	f = fopen("__out.gp", "w");
	fprintf(f, "plot '__out.txt' using 1:2 with lines\npause -1\n");
	fclose(f);

	system("gnuplot __out.gp");
	system("rm -f __out.gp __out.txt");
}
void _plot_bpm(float bpm[NBANDS][BPM_LEN])
{
	FILE *f;
	int i, j;
	f = fopen("__out.txt", "w");
	if(!f) {
		spect_error("FOPEN failed!");
		return;
	}

	for(i = 1; i < BPM_LEN; i++) {
		int b = i + BPM_MIN;
		fprintf(f, "%d", b);
		for(j = 0; j < NBANDS; j++) {
			fprintf(f, " %f", bpm[j][i]);
		}
		fprintf(f, "\n");
	}
	fclose(f);
	f = fopen("__out.gp", "w");
	fprintf(f, "plot \\\n");
	for(i = 0; i < NBANDS; i++) {
		fprintf(f, "\"__out.txt\" using 1:%d with lines", i + 2);
		if(i < (NBANDS - 1)) {
		      fprintf(f, ", \\");
		}
		fprintf(f, "\n");
	}
	fprintf(f," pause -1\n");
	fclose(f);

	system("gnuplot __out.gp");
	system("rm -f __out.gp __out.txt");
}


