#ifndef __GETOPT_EASY_H_
#define __GETOPT_EASY_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GETOPT_EASY_NO_MEM                   (-1)
#define GETOPT_EASY_INVALID_OPT_STRING       (-2)
#define GETOPT_EASY_INVALID_VALUE_PTR        (-3)
#define GETOPT_EASY_INVALID_TYPE             (-4)
#define GETOPT_EASY_NO_REQUIRED_OPTION_VALUE (-5)
#define GETOPT_EASY_ARGC_INVALID             (-6)
#define GETOPT_EASY_ARGV_INVALID             (-7)
#define GETOPT_EASY_OPT_ARRAY_INVALID        (-8)
#define GETOPT_EASY_OPT_ARRAY_LEN_INVALID    (-9)
#define GETOPT_EASY_INTERNAL_ERROR           (-10)

typedef enum {
	TYPE_START,
	FLAG,
	INT,
	UNSIGNED_INT,
	FLOAT,
	DOUBLE,
	CHAR,
	STRING,
	TYPE_END
} getopt_easy_opt_type_t;

typedef struct {
	char                   *opt;
	getopt_easy_opt_type_t type;
	void                   *value;
} getopt_easy_opt_t;

static int error_option_index = 0;

static inline int getopt_easy
(
	int               *_argc,   /* In/Out - pointer to argc */
	char              ***_argv, /* In/Out - Pointer to argv */
	getopt_easy_opt_t *opt,     /* In     - Pointer to array of options */
	unsigned int      len       /* In     - Number of options */
)
{
	int argc = 1; /* argc will at least be 1 */
	char **argv = NULL;
	char opt_string[256] = "";
	int c;
	int i;
	int rc = GETOPT_EASY_INTERNAL_ERROR;


	if(!_argc)
	      return GETOPT_EASY_ARGC_INVALID;
	if(!_argv)
	      return GETOPT_EASY_ARGV_INVALID;
	if(!opt)
	      return GETOPT_EASY_OPT_ARRAY_INVALID;
	if(!len)
	      return GETOPT_EASY_OPT_ARRAY_LEN_INVALID;

	for(i = 0; i < len; i++) {
		error_option_index = i;
		if(!opt[i].opt)
		      return GETOPT_EASY_INVALID_OPT_STRING;
		if(!opt[i].value)
		      return GETOPT_EASY_INVALID_VALUE_PTR;
		if(opt[i].type <= TYPE_START || opt[i].type >= TYPE_END)
		      return GETOPT_EASY_INVALID_TYPE;
		/* If not a flag, then a value is required (the : or = */
		if(opt[i].type > FLAG && opt[i].opt[1] != ':') {
			if(opt[i].opt[1] != ':') {
				return GETOPT_EASY_INVALID_OPT_STRING;
			}
		}
		strcat(opt_string, opt[i].opt);
	}
	while((c = getopt(*_argc, *_argv, opt_string)) != -1) {
		for(i = 0; i < len; i++) {
			if((char)c == opt[i].opt[0])
			      break;
		}
		if(i < len) {
			switch(opt[i].type) {
				case FLAG:
					*((unsigned int *)opt[i].value) = 1;
					break;
				case INT:
					if(!optarg)
						goto bailout;
					*((int *)opt[i].value) = (int)atoi(optarg);
					break;
				case UNSIGNED_INT:
					if(!optarg)
					      goto bailout;
					*((unsigned int *)opt[i].value) = (unsigned int)atoi(optarg);
					break;
				case FLOAT:
					if(!optarg)
					      goto bailout;
					*((float *)opt[i].value) = (float)atof(optarg);
					break;
				case DOUBLE:
					if(!optarg)
					      goto bailout;
					*((double *)opt[i].value) = (double)atof(optarg);
					break;
				case CHAR:
					if(!optarg)
					      goto bailout;
					*((char *)opt[i].value) = optarg[0];
					break;
				case STRING:
					if(!optarg)
						goto bailout;
					*((char **)opt[i].value) = optarg;
					break;
				default:
					break; /* Not possible, this is already checked before */
			}
			continue;
		}
		if(c == '?') {
			fprintf(stderr, "Option %c requires an argument\n", optopt);
			rc = GETOPT_EASY_NO_REQUIRED_OPTION_VALUE;
			goto bailout;
		} else {
			printf("Unknown option!\n");
		}
	}
	if(optind < *_argc) {
		argc = *_argc - optind + 1;
		argv = (char **)calloc(argc, sizeof(char *));
		printf("Allocating %d pointers!\n", argc); fflush(stdout);
		if(!argv)
		      return GETOPT_EASY_NO_MEM;
		argv[0] = (*_argv)[0];
		for(i = optind; i < *_argc; i++)  {
			int ind = 1 + i - optind;
			argv[ind] = (*_argv)[i];
		}
		*_argv = argv;
		*_argc = argc;
	} else {
		*_argc = 1;
	}
	rc = 0;
bailout:
	return rc;
}

#if 0
/*****************************************************
 *           EXAMPLE MAIN (USAGE)
 ****************************************************/

int ain(int argc, char **argv)
{
	int help = 0; /* The default value of flag */
	int val = 0;  /* The default value of integer option */
	char *str = "Default"; /* The default value of string */
	getopt_easy_opt_t opt[] = {

		    /* {option, type, pointer to output}, */

		    {"h", FLAG, &help}, /* Help (-h) is a flag does not have an argument */

		    {"i:", INT, &val},  /* -i takes an integer argument */

		    {"s:", STRING, (void *)&str} /* -s takes a string, &str needs to be 
						    typecasted to avoid a warning */
	};
	int i;

	if(getopt_easy(&argc, &argv, opt, 3) != 0) {
		printf("Something went wrong!\n");
		exit(-1);
	}


	if(help) {
		printf("Help!\n");
	} else {
		printf("No Help!\n");
	}
	printf("Value: %d\n", val);
	printf("String: %s\n", str);

	/* The program name will always be argv[0] */
	printf("Program name: %s\n", argv[0]);

	/* All other parameters not accounted by the arguments in opt */
	printf("Leftover:\n");
	for(i = 1; i < argc; i++) {
		printf("%s\n", argv[i]);
	}

	return 0;
}

#endif

#endif
