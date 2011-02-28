#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

typedef struct Node_s Node;

struct Node_s{
	char name[256];
	Node *next;
};

int isdir(const char *dname) {
  struct stat sbuf;
 
  if (lstat(dname, &sbuf) == -1) {
      fprintf(stderr, "lstat() Failed.\n");
      return 0;
    }
 
  if(S_ISDIR(sbuf.st_mode)) {
      return 1;
    }
  return 0;
}


int has_extension(char *name, char *ext)
{
	int len_name;
	int len_ext;
	int i;
	int j = 0;
	if(!name || !ext)
	      return 0;
	len_name = strlen(name);
	len_ext = strlen(ext);
	for(i = len_name - 1; i >= 0; i--) {
		if(name[i] == '.')
		      break;
	}
	i++;
	for(;i < len_name && j < len_ext; i++, j++) {
		char a = name[i];
		if(a >= 'A' && a <= 'Z')
		      a = a - 'A';
		if(a != ext[j])
		      return 0;
	}
	return 1;
}

int has_extension_array(char *name, char **exts, unsigned int len)
{
	int i;

	for(i = 0; i < len; i++) {
		if(has_extension(name, exts[i]))
		      return 1;
	}
	return 0;
}

Node *find(Node *head, char *dir_name, char **ext, int ext_len)
{
	DIR *d;
	struct dirent *dcon;
	struct stat dstuff;
	char parent[256];
	char entry[256];
	int len;
	Node *node;

	d = opendir(dir_name);
	if(!d) {
		return head;
	}
	realpath(dir_name, parent);
	while(dcon = readdir(d)) {
		if(strcmp(dcon->d_name, ".") == 0)
		      continue;
		if(strcmp(dcon->d_name, "..") == 0)
		      continue;
		strcpy(entry, parent);
		strcat(entry, "/");
		strcat(entry, dcon->d_name);
		if(isdir(entry)) {
			head = find(head, entry, ext, ext_len);
			continue;
		}
		if(has_extension_array(entry, ext, ext_len)) {
			node = malloc(sizeof(Node));
			if(!node)
			      return head;
			strcpy(node->name, entry);
			node->next = head;
			head = node;
		}
	}
	closedir(d);
	return head;
}

void purge_results(Node *head)
{
	Node *node;
	Node *to_free;

	node = head;
	while(node) {
		to_free = node;
		node = node->next;
		free(to_free);
	}
}
#ifdef FIND_TEST
int main(int argc, char *argv[])
{
	Node *head = NULL;
	Node *node;
	char *exts[] = {"mp3", "mp2", "ogg"};

	if(argc <= 1) {
		printf("USAGE: %s Dir\n",argv[0]);
		exit(-1);
	}

	node = head = find(head, argv[1], exts, 3);
	if(!head) {
		printf("Find failed\n");
	}

	while(node) {
		printf("%s\n", node->name);
		node = node->next;
	}

	purge_results(head);

	return 0;
	
}
#endif
