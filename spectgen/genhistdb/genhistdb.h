#ifndef __GENHISTDB_H_
#define __GENHISTDB_H_

typedef enum {
	UPDATE_MODE = 0,
	RECREATE_MODE
} generate_mode_t;

int generate_histdb(char *dirname, char *dbname, 
		unsigned int nr_threads, generate_mode_t mode);

#endif
