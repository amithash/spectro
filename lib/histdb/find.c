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
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include "find.h"

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
		      a = 'a' + a - 'A';
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
	char parent[PATH_MAX] = "";
	char entry[PATH_MAX] = "";
	Node *node;

	d = opendir(dir_name);
	if(!d) {
		printf("Could not open dir %s\n",dir_name);
		return head;
	}
	if(!realpath(dir_name, parent)) {
		printf("Real path failed!\n");
		return head;
	}
	while((dcon = readdir(d))) {
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

	list_purge(head);

	return 0;
	
}
#endif
