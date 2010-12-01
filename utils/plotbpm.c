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

int main(int argc, char *argv[])
{
	spect_t spect;
	int rc;
	int i;
	double bpm[BPM_LEN];
	double _bpm[NBANDS][BPM_LEN];
	if(argc <= 1) {
		spect_error("USAGE: %s <spect file>", argv[0]);
		exit(-1);
	}

	if((rc = read_spectf(argv[1], &spect)) != RM_SUCCESS) {
		spect_error("Reading %s returned in error=%s",argv[1],RM_RC_STR(rc));
		exit(-1);
	}
	if(argc >= 3) {
		if(_spect2bpm(_bpm, &spect)) {
			spect_error("Conversion failed!");
			goto cleanup_spect;
		}
		for(i = 0; i < NBANDS; i++)
			smooth_bpm(_bpm[i]);

		_plot_bpm(_bpm);
	} else {
		if(spect2bpm(bpm, &spect)) {
			spect_error("Conversion failed!");
			goto cleanup_spect;
		}
		plot_bpm(bpm);	
	}


cleanup_spect:
	free_spect(&spect);

	return 0;
}
