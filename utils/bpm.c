#include "bpm.h"
#include <fftw3.h>
#include <tag_c.h>

#define FREQ2BPM(f,len,sr) ((unsigned int)(60 * (((float)sr) * ((float)f / len))))

int spect2bpm(double bpm[BPM_LEN], spect_t *spect)
{
	double bpm_nr[BPM_LEN];
	int sampling_rate;
	int seconds;
	fftw_complex *vals;
	double *out;
	TagLib_File *tf;
	int rc = 0;
	int i,j;
	fftw_plan plan;
	double total = 0;

	tf = taglib_file_new(spect->fname);
	seconds = taglib_audioproperties_length(taglib_file_audioproperties(tf));
	taglib_file_free(tf);
	sampling_rate = (int)((((double)spect->len) / ((double)seconds)) + 0.5);

	for(i = 0; i < BPM_LEN; i++) {
		bpm[i] = 0;
		bpm_nr[i] = 0;
	}

	vals = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * spect->len);
	if(!vals) {
		rc = -1;
		goto vals_failed;
	}
	out = (double *)malloc(sizeof(double) * (spect->len / 2));
	if(!out) {
		rc = -1;
		goto out_failed;
	}
	for(i = 0; i < spect->len / 2; i++) {
		out[i] = 0;
	}
	plan = fftw_plan_dft_1d(spect->len, vals, vals, FFTW_FORWARD, FFTW_MEASURE);

	for(i = 0; i < NBANDS; i++) {
		for(j = 0; j < spect->len; j++) {
			vals[j][0] = spect->spect[i][j] * 1000;
			vals[j][1] = 0;
		}

		fftw_execute(plan);

		for(j = 0; j < spect->len / 2; j++) {
			double re,im;
			double abs;
			re = vals[j][0] / (double)spect->len; 
			im = vals[j][1] / (double)spect->len;
			abs = (re*re) + (im * im);
			out[j] += abs;
		}
	}

	for(i = 1; i < spect->len / 2; i++) {
		unsigned int b = FREQ2BPM(i, spect->len, sampling_rate);
		if(b >= BPM_LEN)
		      continue;
		if(b < BPM_MIN)
		      continue;
		b = b - BPM_MIN;
		bpm[b] += out[i];
		bpm_nr[b] += 1;
	}

	for(i = 0; i < BPM_LEN; i++) {
		if(bpm_nr[i] > 1) {
			bpm[i] = bpm[i] / bpm_nr[i];
		}
		total += bpm[i];
	}
	for(i = 0; i < BPM_LEN; i++) {
		bpm[i] /= total;
	}

	fftw_destroy_plan(plan);

	free(out);
out_failed:
	fftw_free(vals);
vals_failed:
	return rc;
}

void plot_bpm(double bpm[BPM_LEN])
{
	FILE *f;
	int i;
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

