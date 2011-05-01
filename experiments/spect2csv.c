/*******************************************************************************
    This file is part of spectro
    Copyright (C) 2011  Amithash Prasad <amithash@gmail.com>

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

#include <stdio.h>
#include <unistd.h>

#include "spect-config.h"

#define SEP " "

#define PRINT_FORMAT "%f"
#define TRANSFORM(val) val

int main(void)
{
	int n = 0;
	float c;
	int ind = 0;
	while((n = read(fileno(stdin), &c, sizeof(float))) > 0) {
		printf(PRINT_FORMAT, TRANSFORM(c));

		if(++ind == NBANDS) {
			printf("\n");
			ind = 0;
		} else {
			printf(SEP);
		}
	}
	return 0;
}
