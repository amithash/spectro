#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "decoder.h"
#include "spectgen.h"
#include "spect-config.h"
#include "find.h"
#include "histdb.h"

#define DEFAULT_NTHREADS 2

#define MAX_SUPPORTED_EXTENSIONS 15

#define HASH_TABLE_SIZE 1024

pthread_mutex_t hash_table_lock = PTHREAD_MUTEX_INITIALIZER;
Node *hash_table[HASH_TABLE_SIZE] = {NULL};

pthread_mutex_t hist_list_lock = PTHREAD_MUTEX_INITIALIZER;
hist_t *hist_list = NULL;
unsigned int hist_len = 0;
int completed = 0;
int to_complete = 0;

void init_hash_table(void)
{
	int i;
	pthread_mutex_lock(&hash_table_lock);
	for(i = 0; i < HASH_TABLE_SIZE; i++)
	      hash_table[i] = NULL;
	pthread_mutex_unlock(&hash_table_lock);
}

void purge_hash_table(void)
{
	int i;
	pthread_mutex_lock(&hash_table_lock);
	for(i = 0; i < HASH_TABLE_SIZE; i++){
		if(hash_table[i]) {
			list_purge(hash_table[i]);
			hash_table[i] = NULL;
		}
	}
	pthread_mutex_unlock(&hash_table_lock);
}

unsigned int get_idx(char *name, int max_ind)
{
	unsigned int idx = 0;
	int i;
	int len = strlen(name);
	for(i = 0; i < len; i++) {
		idx += (unsigned int)name[i];
	}
	return idx % max_ind;
}

void add_entry(char *name)
{
	Node *node;
	unsigned int idx;
	node = malloc(sizeof(Node));
	if(!node) {
		printf("Malloc failed\n");
		return;
	}
	strcpy(node->name, name);
	idx = get_idx(node->name, HASH_TABLE_SIZE);
	pthread_mutex_lock(&hash_table_lock);
	node->next = hash_table[idx];
	hash_table[idx] = node;
	pthread_mutex_unlock(&hash_table_lock);
}

int exists_in_hash(char *name)
{
	Node *node;
	unsigned int idx;

	idx = get_idx(name, HASH_TABLE_SIZE);
	pthread_mutex_lock(&hash_table_lock);
	node = hash_table[idx];
	while(node) {
		if(strcmp(node->name, name) == 0) {
			pthread_mutex_unlock(&hash_table_lock);
			return 1;
		}
		node = node->next;
	}
	pthread_mutex_unlock(&hash_table_lock);
	return 0;
}

Node *get_compute(Node *head, hist_t *hist_list, unsigned int hist_len)
{
	int i;
	Node *node;
	Node *to_free;
	Node *compute = NULL;

	BEGIN(init_hash_table);
	init_hash_table();
	END(init_hash_table);

	BEGIN(add_entries);
	for(i = 0; i < hist_len; i++) {
		add_entry(hist_list[i].fname);
	}
	END(add_entries);

	node = head;
	BEGIN(compare);
	while(node) {
		if(exists_in_hash(node->name)) {
			to_free = node;
			node = node->next;
			free(to_free);
		} else {
			/* Add to compute list */
			to_free = node->next;
			node->next = compute;
			compute = node;
			node = to_free;
		}
	}
	END(compare);
	purge_hash_table();
	return compute;
}

Node *GetMusicFiles(char *dir)
{
	int i;
	Node *head = NULL;
	unsigned int ext_len = MAX_SUPPORTED_EXTENSIONS;
	char _extensions[MAX_SUPPORTED_EXTENSIONS][5];
	char *extensions[MAX_SUPPORTED_EXTENSIONS];

	for(i = 0; i < MAX_SUPPORTED_EXTENSIONS; i++) {
		extensions[i] = &_extensions[i][0];
	}
	decoder_supported_extensions(extensions, &ext_len);

	if(ext_len == 0) {
		printf("Nothing supported!\n");
		return NULL;
	}
	return find(head, dir, extensions, ext_len);

}

void *process_thread(void *par)
{
	Node *node = (Node *)par;
	while(node) {
		hist_t *hist;
		hist = gen_hist(node->name);
		if(!hist) {
			printf("Hist generation for %s failed\n", node->name);
			node = node->next;
			continue;
		}
		pthread_mutex_lock(&hist_list_lock);
		memcpy(&hist_list[hist_len], hist, sizeof(hist_t));
		hist_len++;
		completed++;
		pthread_mutex_unlock(&hist_list_lock);
		free(hist);
		node = node->next;
	}
	pthread_exit(NULL);
}

void *progress_routine(void *unused)
{
	while(completed < to_complete) {
		pthread_mutex_lock(&hist_list_lock);
		progress2(completed,to_complete);
		pthread_mutex_unlock(&hist_list_lock);
		sleep(1);
	}
	pthread_exit(NULL);
}

int generate_histdb(char *dirname, char *dbname, unsigned int nr_threads, generate_mode_t mode)
{
	Node *head = NULL;
	Node *compute = NULL;
	unsigned int compute_len;
	Node *node;
	/* Threading */
	int i;
	Node **per_thread_list = NULL;
	pthread_t *threads;
	pthread_t progress_thread;
	void *par;
	int rc = -1;
	unsigned int threads_started = 0;

	if(nr_threads == 0)
		nr_threads = DEFAULT_NTHREADS;

	head = GetMusicFiles(dirname);

	if(mode == UPDATE_MODE) {
		if(read_histdb(&hist_list, &hist_len, dbname)) {
			compute = head;
		} else {
			compute = get_compute(head, hist_list, hist_len);
		}
	} else {
		compute = get_compute(head, hist_list, hist_len);
	}
	node = compute;
	compute_len = list_len(compute);

	hist_list = realloc(hist_list, sizeof(hist_t) * (hist_len + compute_len));
	if(!hist_list)
		goto hist_list_creation_failed;

	
	threads = (pthread_t *)calloc(nr_threads, sizeof(pthread_t));
	if(!threads)
	      goto threads_creation_failed;

	per_thread_list = (Node **)calloc(nr_threads, sizeof(Node *));
	if(!per_thread_list)
		goto list_creation_failed;

	list_split(per_thread_list, nr_threads, compute);

	completed = 0;
	to_complete = compute_len;

	/* Close stderr as taglib tends to print a lot of crap to stderr */
	fclose(stderr);
	stderr = fopen("/dev/null", "w");

	for(i = 0; i < nr_threads; i++) {
		if(pthread_create(&threads[i], 0, process_thread, per_thread_list[i])) {
			completed = to_complete;
			printf("Could not create thread %d\n",i);
			break;
		}
		threads_started++;
	}
	if(threads_started == nr_threads) {
		if(pthread_create(&progress_thread, 0, progress_routine, NULL)) {
			printf("Could not create progress thread\n");
			completed = to_complete;
		} else {
			pthread_join(progress_thread, &par);
		}
	}

	for(i = 0; i < threads_started; i++) {
		pthread_join(threads[i], &par);
		list_purge(per_thread_list[i]);
	}
	for(i = threads_started; i < nr_threads; i++)
	      list_purge(per_thread_list[i]);
	if(threads_started == nr_threads) {
		write_histdb(hist_list, hist_len, dbname);
		rc = 0;
	
	}
	free(per_thread_list);
list_creation_failed:
	free(threads);
threads_creation_failed:
	free(hist_list);
hist_list_creation_failed:
	if(rc != 0)
		printf("Errors occured in creating histdb, %s unmodified/uncreated\n",dbname);
	return rc;
}

