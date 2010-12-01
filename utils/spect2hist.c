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

int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("USAGE: %s <SPECT DB FILE> <OUT HIST DB FILE>\n",argv[0]);
		exit(-1);
	}
	if(spectdb2histdb(argv[1], argv[2])) {
		printf("ERROR in converting %s to %s\n",argv[1], argv[2]);
	}
	return 0;
}
