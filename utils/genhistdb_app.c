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
#include <stdlib.h>
#include "histdb.h"
#include <pthread.h>
#include "spect-config.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void print_perc(void *unused, int perc)
{
	double my_perc = (double)perc;
	progress(my_perc, stdout);
	if(perc == 100) {
		pthread_mutex_lock(&mutex);
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mutex);
	}
}

int main(int argc, char *argv[])
{
	unsigned int nr_threads = 0;
	char *dirname;
	char *dbname;
	genhistdb_handle_type handle;

	if(argc < 3) {
		printf("Usage: %s <Music Dir> <Out File>\n",argv[0]);
		return -1;
	}
	dirname = argv[1];
	dbname  = argv[2];
	if(argc > 3)
		nr_threads = atoi(argv[3]);
	if(generate_histdb_prepare(&handle, dirname, dbname, nr_threads, UPDATE_MODE)) {
		printf("Preparing HISTDB generation failed\n");
		exit(-1);
	}
	if(generate_histdb_start(handle, print_perc, NULL, 1)) {
		printf("Starting generation failed\n");
	}
	pthread_mutex_lock(&mutex);
	pthread_cond_wait(&cond, &mutex);
	pthread_mutex_unlock(&mutex);
	return 0;
}
