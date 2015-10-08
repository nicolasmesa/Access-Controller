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

struct user_struct {
	char *username;
	struct user_struct *next; // Only used to traverse all users
	struct user_group_list *groups;
};

struct group_struct {
	char *groupname;
	struct group_struct *next; //Only used to traverse all groups
	struct group_user_list *users;	
};

struct group_user_list {
	struct user_struct *user;
	struct group_user_list *next;
};

struct user_group_list {
	struct group_struct *group;
	struct user_group_list *next;
};


static struct file_struct *root;
static struct user_struct *usersHead = NULL;
static struct group_struct *groupsHead = NULL;


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
	int pathLength;
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

struct user_struct *findUserByUsername(char *username) {
	struct user_struct *window = usersHead;

	while (window != NULL) {
		if (strcmp(username, window->username) == 0) {
			return window;
		}

		window = window->next;
	}

	return NULL;
}

struct group_struct *findGroupByGroupname(char *groupname) {
	struct group_struct *window = groupsHead;

	while (window != NULL) {
		if (strcmp(groupname, window->groupname) == 0) {
			return window;
		}

		window = window->next;
	}

	return NULL;
}

struct user_struct *createUser(char *username) {
	struct user_struct *user = findUserByUsername(username);

	if (user != NULL) {
		printAndExit("User already exists");
	}

	user = malloc(sizeof(struct user_struct));

	if(user == NULL) {
		printAndExit(NULL);
	}

	user->username = username;
	user->next = usersHead;
	user->groups = NULL;

	usersHead = user;	

	return user;		
}

struct group_struct *createGroup(char *groupname) {
	struct group_struct *group = findGroupByGroupname(groupname);

	if (group != NULL) {
		printAndExit("Group already exists");
	}

	group = malloc(sizeof(struct group_struct));

	if(group == NULL) {
		printAndExit(NULL);
	}

	group->groupname = groupname;
	group->next = groupsHead;
	group->users = NULL;

	groupsHead = group;	

	return group;		
}

int addUserAndGroup(char *username, char *groupname) {
	struct user_struct *user = findUserByUsername(username);
	struct group_struct *group = findGroupByGroupname(groupname);

	if (user == NULL) {
		printf("Creating user %s\n", username);
		user = createUser(username);
	} else {
		printf("User %s already existed\n", username);
		free(username);
	}


	if (group == NULL) {
		printf("Creatring group %s\n", groupname);
		group = createGroup(groupname);
	} else {
		printf("Group %s already existed\n", groupname);
		free(groupname);
	}



	return 0;
	// if any of them is not found, free memory for the string (username or groupname)
}

int parseUserDefinitionLine(char *line) {
	char *userStart = line;
	char *groupStart;
	char c;
	int len = 0;

	//@todo *.group, user.*, *.*

	// Get user name
	while((c = *line) != '.') {
		if (c == '\0' || c == '\n') {
			printAndExit("Invalid string supplied for the users");
		}		

		len++;
		line++;
	}


	if (!len) {
		printAndExit("Invalid string supplied for the users");
	}

	char *username = strndup(userStart, len);
	int noFile = 0;

	len = 0;
	line++;
	groupStart = line;	

	// Get group name
	while((c = *line) != ' ') {
		if (c == '\n'){
			noFile = 1;
			break;
		}

		if (c == '\0') {
			// @todo
			//printAndExit("Unexpected end of input");
			break;
		}		

		len++;
		line++;
	}


	char *groupname = strndup(groupStart, len);

	addUserAndGroup(username, groupname);

	return 0;
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

	parseUserDefinitionLine("nicolas.mesa");
	parseUserDefinitionLine("nicolas.mesaaassa");
	parseUserDefinitionLine("nicolas.caasd");
	parseUserDefinitionLine("sdfas.mesa");
	parseUserDefinitionLine("ff.f");
	parseUserDefinitionLine("ff.mesa");
	parseUserDefinitionLine("ff.mesaaassa");

	printf("%s\n", root->cmpName);

	return 0;
}
