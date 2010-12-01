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

#include "spect.h"
#include <pthread.h>

#define NTHREADS_FREAD   4
#define NTHREADS_COMPUTE 2

int main(int argc, char *argv[])
{
	int rc;

	if(argc != 3) {
		spect_error("USAGE: SPECT_FILE_LIST SPECT_DB_FILE_NAME");
		exit(-1);
	}

	if((rc = combine_spect_list(argv[1], argv[2], NTHREADS_FREAD)) < 0) {
		spect_error("Failed to read spect list file (rc=%d): %s",rc,argv[1]);
		exit(-1);
	}

	return 0;

}
