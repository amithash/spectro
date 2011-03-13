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
#include "hash.h"

#define DEFAULT_NTHREADS 2

#define MAX_SUPPORTED_EXTENSIONS 15

#define HASH_TABLE_SIZE 1024

#define LOCK_HANDLE(handle, id) do {				\
	pthread_mutex_lock(&(handle)->hist_list_lock);		\
} while(0)

#define UNLOCK_HANDLE(handle, id) do {				\
	pthread_mutex_unlock(&(handle)->hist_list_lock);	\
} while(0)

struct genhistdb_struct {
	pthread_mutex_t hist_list_lock;
	hist_t *hist_list;
	unsigned int hist_len;

	int completed;
	int to_complete;

	pthread_t *threads;
	Node **per_thread_list;
	pthread_t progress_thread;
	unsigned int nr_threads;
	pthread_barrier_t barrier;

	char dbname[1024];
	void *priv;

	genhistdb_notification_type notification;
	int every_perc;

	int error;
};

static Node *get_compute(Node *head, hist_t *hist_list, unsigned int hist_len)
{
	int i;
	Node *node;
	Node *to_free;
	Node **hash_table;
	Node *compute = NULL;

	hash_table = (Node **)malloc(sizeof(Node *) * HASH_TABLE_SIZE);
	if(!hash_table) {
		printf("Malloc failed\n");
		return NULL;
	}

	init_hash_table(hash_table, HASH_TABLE_SIZE);

	for(i = 0; i < hist_len; i++) {
		add_entry(hash_table, HASH_TABLE_SIZE, hist_list[i].fname);
	}

	node = head;
	while(node) {
		if(exists_in_hash(hash_table, HASH_TABLE_SIZE, node->name)) {
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
	purge_hash_table(hash_table, HASH_TABLE_SIZE);
	free(hash_table);
	return compute;
}

static Node *GetMusicFiles(char *dir)
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

static void *process_thread(void *par)
{
	struct genhistdb_struct *handle = (struct genhistdb_struct *)par;
	pthread_t self_thread;
	Node *node;
	hist_t *hist = NULL;
	int self_ind;
	if(!handle)
	      goto bailout;

	pthread_barrier_wait(&handle->barrier);

	if(handle->error != 0) {
		goto bailout;
	}
	self_thread = pthread_self();
	for(self_ind = 0; self_ind < handle->nr_threads; self_ind++) {
		if(handle->threads[self_ind] == self_thread)
		      break;
	}
	if(self_ind == handle->nr_threads) {
		printf("ERROR! I am not one of the started threads for this handle\n");
		goto bailout;
	}
	node = handle->per_thread_list[self_ind];

	while(node) {
		hist = gen_hist(node->name);
		LOCK_HANDLE(handle, self_ind);
		handle->completed++;
		if(!hist) {
			printf("Hist generation for %s failed\n", node->name);
		} else {
			memcpy(&handle->hist_list[handle->hist_len], hist, sizeof(hist_t));
			free(hist);
			handle->hist_len++;
		}
		UNLOCK_HANDLE(handle, self_ind);
		node = node->next;
	}
bailout:
	pthread_exit(NULL);
}

static int generate_histdb_finalize(genhistdb_handle_type _handle)
{
	int i;
	void *par;
	int rc = -1;
	struct genhistdb_struct *handle = (struct genhistdb_struct *)_handle;
	if(!handle)
	      return -1;
	for(i = 0; i < handle->nr_threads; i++) {
		if(handle->error == 0)
			pthread_join(handle->threads[i], &par);
		list_purge(handle->per_thread_list[i]);
	}
	free(handle->per_thread_list);
	free(handle->threads);
	if(handle->error == 0) {
	      if(write_histdb(handle->hist_list, handle->hist_len, handle->dbname)) {
	      } else {
	      	rc = 0;
	      }
	} else {
		printf("There was an error. Not commiting to %s\n", handle->dbname);
	}
	free(handle->hist_list);
	pthread_mutex_destroy(&handle->hist_list_lock);
	pthread_barrier_destroy(&handle->barrier);
	free(handle);
	return rc;
}

static void *progress_routine(void *par)
{
	struct genhistdb_struct *handle = (struct genhistdb_struct *)par;
	int perc;
	int last_perc = 0;
	genhistdb_notification_type notification;
	void *priv;

	if(!handle)
	      goto bailout;

	pthread_barrier_wait(&handle->barrier);

	if(handle->error != 0) {
		goto bailout;
	}
	notification = handle->notification;
	priv         = handle->priv;

	notification(priv, 0);
	while(handle->to_complete > 0) {
		perc = (100 * handle->completed) / handle->to_complete;
		if(perc == 100) {
			break;
		}
		if(perc >= (last_perc + handle->every_perc)) {
			last_perc = perc;
			notification(priv, perc);
		}
		sleep(1);
	}
	/* You will get here only when perc = 100%. At this point, we send out
	 * the last notification, the client may call finalize and free the
	 * handle from underneath us. So do not depend on it. */
	generate_histdb_finalize(handle);
	notification(priv, 100);
bailout:
	pthread_exit(NULL);
}

int generate_histdb_prepare(genhistdb_handle_type *_handle, char *dirname, char *dbname, 
			unsigned int nr_threads, generate_mode_t mode)
{
	struct genhistdb_struct *handle;
	Node *head = NULL;
	Node *compute = NULL;
	unsigned int compute_len = 0;

	if(!dirname || !dbname)
	      return -1;

	handle = (struct genhistdb_struct *)malloc(sizeof(struct genhistdb_struct));
	if(!handle)
	      return -1;
	if(nr_threads == 0)
	      handle->nr_threads = DEFAULT_NTHREADS;
	else
	      handle->nr_threads = nr_threads;
	
	head = GetMusicFiles(dirname);

	handle->hist_list = NULL;
	handle->hist_len = 0;
	if(mode == UPDATE_MODE) {
		if(read_histdb(&handle->hist_list, &handle->hist_len, dbname)) {
			compute = head;
		} else {
			compute = get_compute(head, handle->hist_list, handle->hist_len);
		}
	} else {
		compute = get_compute(head, handle->hist_list, handle->hist_len);
	}
	compute_len = list_len(compute);
	handle->hist_list = realloc(handle->hist_list, sizeof(hist_t) * (handle->hist_len + compute_len));
	if(compute_len <= handle->nr_threads)
	      handle->nr_threads = 1;

	if(!handle->hist_list) {
		free(handle);
		goto realloc_failed;
	}

	handle->threads = (pthread_t *)calloc(handle->nr_threads, sizeof(pthread_t));
	if(!handle->threads)
	      goto threads_failed;

	handle->per_thread_list = (Node **)calloc(handle->nr_threads, sizeof(Node *));
	if(!handle->per_thread_list)
		goto list_creation_failed;

	handle->completed = 0;
	handle->to_complete = compute_len;
	list_split(handle->per_thread_list, handle->nr_threads, compute);

	strcpy(handle->dbname, dbname);
	*_handle = handle;
	return 0;

list_creation_failed:
	free(handle->threads);
threads_failed:
	free(handle->hist_list);
realloc_failed:
	free(handle);
	return -1;
}

int generate_histdb_start(genhistdb_handle_type _handle, 
			genhistdb_notification_type cb, void *priv, int perc)
{
	int i;
	int threads_started = 0;
	void *par;
	pthread_attr_t attr;
	struct genhistdb_struct *handle = (struct genhistdb_struct *)_handle;
	if(!handle)
	      return -1;
	handle->error = -1;
	handle->notification = cb;
	handle->priv = priv;
	if(perc <= 0)
	      perc = 1;
	handle->every_perc = perc;

	pthread_mutex_init(&handle->hist_list_lock, NULL);
	pthread_barrier_init(&handle->barrier, NULL, handle->nr_threads + 2);

	for(i = 0; i < handle->nr_threads; i++) {
		if(pthread_create(&handle->threads[i], 0, process_thread, handle)) {
			printf("Could not create thread: %d\n", i);
			break;
		}
	}
	if(i != handle->nr_threads) {
		threads_started = i;
		for(i = 0; i < threads_started; i++) {
			pthread_join(handle->threads[i], &par);
		}
		return -1;
	}
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if(pthread_create(&handle->progress_thread, &attr, progress_routine, handle)){
		for(i = 0; i < handle->nr_threads; i++)
		      pthread_join(handle->threads[i], &par);
		return -1;
	}
	pthread_attr_destroy(&attr);

	fclose(stderr);
	stderr = fopen("/dev/null", "w");

	handle->error = 0;
	pthread_barrier_wait(&handle->barrier);
	return 0;
}

