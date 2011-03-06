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

#include "hist.h"

int main(int argc, char *argv[])
{
	hist_t *hist_list = NULL;
	hist_t *hist = NULL;
	unsigned int len;
	int i;
	if(argc <= 2) {
		spect_error("USAGE: %s <Hist DB File> <MP3 file>\n",argv[0]);
		exit(-1);
	}
	if(read_hist_db(&hist_list, &len, argv[1])) {
		spect_error("Reading DB:%s failed", argv[1]);
		exit(-1);
	}

	for(i = 0; i < len; i++) {
		if(strcmp(hist_list[i].fname, argv[2]) == 0) {
			printf("Found at index %d\n", i);
			hist = &hist_list[i];
			break;
		}
	}
	if(!hist) {
		spect_error("%s was not found in the db", argv[2]);
		goto cleanup;
	}

	plot_hist(hist);
cleanup:
	free(hist_list);

	return 0;
}
