
void cent_clear(hist_t *hist)
{
	int i,j;
	hist->fname[0] = '\0';
	for(i = 0; i < NBANDS; i++) {
		for(j = 0; j < HIST_LEN; j++) {
			hist->spect_hist[i][j] = 0;
		}
	}
}

void cent_accum(hist_t *out, hist_t *in)
{
	int i,j;
	for(i = 0; i < NBANDS; i++) {
		for(j = 0; j < HIST_LEN; j++) {
			out->spect_hist[i][j] += sqrt(in->spect_hist[i][j]);
		}
	}
}

void cent_final(hist_t *out, hist_t *in, unsigned int len)
{
	int i,j;
	for(i = 0; i < NBANDS; i++) {
		for(j = 0; j < HIST_LEN; j++) {
			out->spect_hist[i][j] = pow(in->spect_hist[i][j] / len, 2);
		}
	}
}


void cent_copy(hist_t *out, hist_t *in) 
{
	memcpy(out, in, sizeof(hist_t));
}

