#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_CMP_SIZE 16
#define MAX_FILE_NAME_SIZE 256

struct file_struct {
	struct file_struct *next;
	struct file_struct *parent;
	struct file_struct *children;
	char cmpName[MAX_CMP_SIZE + 1];	
};


static struct file_struct *root;


void printAndExit(char *msg) {
	if (msg == NULL) {
		msg = strerror(errno);
	}

	printf("Error: %s\n", msg);
	exit(1);
}

struct file_struct *findFileInListByName(struct file_struct *file, char *cmpName) {
	struct file_struct *curr = file;

	while (curr) {
		if (strncmp(curr->cmpName, cmpName, MAX_CMP_SIZE) == 0 ) {
			return curr;
		}

		curr = curr->next;
	}

	return NULL;
}

int addChild(struct file_struct *parent, struct file_struct *child) {
        if (findFileInListByName(parent->children, child->cmpName)) {
                printf("Error: File name already exists (%s)", child->cmpName);
                exit(1);
        }

        child->next = parent->children;
        parent->children = child;

	return 0;
}

struct file_struct *createFile(char *cmpName, struct file_struct *parent) {
	struct file_struct *file = malloc(sizeof(struct file_struct));

	if (!file) {
		printAndExit(NULL);
	}

	file->parent = parent;
	file->next = NULL;
	file->children = NULL;
	strncpy(file->cmpName, cmpName, MAX_CMP_SIZE);
	file->cmpName[MAX_CMP_SIZE] = '\0';

	if (parent) {
		addChild(parent, file);
	}

	return file;
}



struct file_struct *addFileByPath(char *pathStart) {
	char cmpName[MAX_CMP_SIZE + 1];	
	int i, pathLength;
	char *path = pathStart;	
	struct file_struct *currentFile = root;

	if (!path) {
		printAndExit("Undefined path");
	}

	if (*path != '/') {
		printAndExit("File path must start with /");
	}

	path++;
	pathLength = 1;

	while (*path != '\0') {
		int cmpLength = 0;
		int last = 0;

		while (*path != '/') {
			if (*path == '\0') {
				last = 1;
				break;
			}

			cmpName[cmpLength] = *path;
			cmpLength++;
			path++;

			if (cmpLength > MAX_CMP_SIZE) {
				cmpName[MAX_CMP_SIZE] = '\0';
				printAndExit("Component longer than allowed");
			}
		}

		cmpName[cmpLength] = '\0';

		printf("Component name(%s), last(%d)\n", cmpName, last);

		struct file_struct *temp = findFileInListByName(currentFile->children, cmpName);

		if(last && temp) {
			printAndExit("File already existed");
		}

		if (temp) {			
			currentFile = temp;			
			printf("File found: %s\n", currentFile->cmpName);
		} else {			
			currentFile = createFile(cmpName, currentFile);
			printf("File created: %s\n", currentFile->cmpName);
		}

		if(last){
			break;
		}

		path++;
	}

	printf("Path size: %ld\n", path - pathStart);

	if (path - pathStart > MAX_FILE_NAME_SIZE) {
		printAndExit("File name exceeds max file name size");
	}

	return currentFile;	
}

int initFs() {
	root = createFile("/", NULL);
	createFile("tmp", root);
	createFile("home", root);
	addFileByPath("/home/nmesa");
	addFileByPath("/home/nmesa/hola/como/estas");	
	addFileByPath("/tmp/nicolas/mesa");

	return 0;
}

int main(int argc, char *argv[]) {
	if (initFs()) {
		return 1;
	}

	printf("%s\n", root->cmpName);

	return 0;
}
