#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_CMP_SIZE 16
#define MAX_FILE_NAME_SIZE 256
#define INITIAL_LINE_SIZE 100

struct file_struct {
	struct file_struct *next;
	struct file_struct *parent;
	struct file_struct *children;
	struct acl_entry *aclHead;
	struct acl_entry *aclTail;
	char cmpName[MAX_CMP_SIZE + 1];	
};

struct user_struct {
	char *username;
	struct user_struct *next; // Only used to traverse all users
	struct user_group_list *groups;
	struct file_struct *file;
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

struct acl_entry {
	struct acl_entry *next;
	struct group_struct *group;
	struct user_struct *user;
	int readPermission;
	int writePermission;
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

	while (curr != NULL) {
		if (strncmp(curr->cmpName, cmpName, MAX_CMP_SIZE) == 0 ) {
			return curr;
		}

		curr = curr->next;
	}

	return NULL;
}

int addChildFile(struct file_struct *parent, struct file_struct *child) {
    if (findFileInListByName(parent->children, child->cmpName)) {
            printAndExit("Error: File name already exists");            
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
	file->aclHead = NULL;
	file->aclTail = NULL;

	strncpy(file->cmpName, cmpName, MAX_CMP_SIZE);
	file->cmpName[MAX_CMP_SIZE] = '\0';

	if (parent) {
		addChildFile(parent, file);
	}

	return file;
}

struct file_struct *findFileByPath(char *pathStart) {
	char cmpName[MAX_CMP_SIZE + 1];	
	char *path = pathStart;	
	struct file_struct *currentFile = root;
	int last = 0;

	if (!path) {
		printAndExit("Undefined path");
	}

	if (*path != '/') {
		printAndExit("File path must start with /");
	}

	path++;

	while (*path != '\0') {
		int cmpLength = 0;

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


		currentFile = findFileInListByName(currentFile->children, cmpName);

		if (currentFile == NULL) {			
			printAndExit("File does not exist");
		}

		if (last) {
			break;
		}

		path++;
	}

	return currentFile;	
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


		struct file_struct *temp = findFileInListByName(currentFile->children, cmpName);

		if(last && temp) {
			printAndExit("File already existed");
		}

		if (temp) {			
			currentFile = temp;			
		} else {			
			currentFile = createFile(cmpName, currentFile);
		}

		if(last){
			break;
		}

		path++;
	}

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

	user->username = strdup(username);
	user->next = usersHead;
	user->groups = NULL;
	user->file = NULL;

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

	group->groupname = strdup(groupname);
	group->next = groupsHead;
	group->users = NULL;

	groupsHead = group;	

	return group;		
}

struct group_struct *findUserGroup(struct user_struct *user, char *groupname) {
	struct user_group_list *userGroupContainer = user->groups;

	while (userGroupContainer != NULL) {
		struct group_struct *group = userGroupContainer->group;

		if (strcmp(group->groupname, groupname) == 0) {
			return group;
		}

		userGroupContainer = userGroupContainer->next;
	}

	return NULL;
}

struct user_struct *findGroupUser(struct group_struct *group, char *username) {
	struct group_user_list *groupUserContainer = group->users;

	while (groupUserContainer != NULL) {
		struct user_struct *user = groupUserContainer->user;

		if (strcmp(user->username, username) == 0) {
			return user;
		}

		groupUserContainer = groupUserContainer->next;
	}

	return NULL;
}

void addUserToGroup(struct user_struct *user, struct group_struct *group) {
	struct group_struct *userGroup = findUserGroup(user, group->groupname);
	struct user_struct *groupUser = findGroupUser(group, user->username);

	if(userGroup == NULL) {
		struct user_group_list *userGroupContainer = malloc(sizeof(struct user_group_list));

		if (userGroupContainer == NULL) {
			printAndExit(NULL);
		}

		userGroupContainer->group = group;
		userGroupContainer->next = user->groups;
		user->groups = userGroupContainer;
	}


	if (groupUser == NULL) {	
		struct group_user_list *groupUserContainer = malloc(sizeof(struct group_user_list));

		if (groupUserContainer == NULL) {
			printAndExit(NULL);
		}

		groupUserContainer->user = user;
		groupUserContainer->next = group->users;
		group->users = groupUserContainer;
	}
}

int addUserAndGroup(char *username, char *groupname) {
	struct user_struct *user = findUserByUsername(username);
	struct group_struct *group = findGroupByGroupname(groupname);

	if (user == NULL) {
		user = createUser(username);
	}


	if (group == NULL) {
		group = createGroup(groupname);
	}

	addUserToGroup(user, group);

	return 0;
}

void validatePermissions(char *permissions){
	if (permissions == NULL) {
		printAndExit("Permissions are invalid");
	}

	int len = strlen(permissions);

	if (len > 2) {
		printAndExit("Permissions are invalid");
	}

	if (*permissions == '-' && len > 1){
		printAndExit("Permissions are invalid");
	}

	if (len == 2 && (permissions[0] != 'r' || permissions[1] != 'w')) {
		printAndExit("Permissions are invalid");
	}

	if (len == 1 && (permissions[0] != 'r' && permissions[0] != 'w')) {
		printAndExit("Permissions are invalid");
	}
}

struct acl_entry *createAclEntry(char *permissions, struct user_struct *user, struct group_struct *group) {
	validatePermissions(permissions);

	int len = strlen(permissions);
	struct acl_entry *aclEntry = malloc(sizeof(struct acl_entry));

	if (aclEntry == NULL) {
		printAndExit(NULL);
	}

	aclEntry->next = NULL;
	aclEntry->group = group;
	aclEntry->user = user;
	aclEntry->readPermission = 0;
	aclEntry->writePermission = 0;

	if (len == 1) {
		if (*permissions == 'r') {
			aclEntry->readPermission = 1;
		}

		if (*permissions == 'w') {
			aclEntry->writePermission = 1;
		}
	}

	if (len == 2) {
		aclEntry->readPermission = 1;
		aclEntry->writePermission = 1;
	}

	return aclEntry;
}

struct acl_entry *findAclByFileUserAndGroup(struct file_struct *file, struct user_struct *user, struct group_struct *group){
	struct acl_entry *aclEntry;

	for (aclEntry = file->aclHead; aclEntry != NULL; aclEntry = aclEntry->next) {
		if (aclEntry->user == user && aclEntry->group == group) {
			return aclEntry;
		}
	}

	return NULL;
}

void addAclToFile(struct file_struct *file, char *permissions, struct user_struct *user, struct group_struct *group) {		
	if (findAclByFileUserAndGroup(file, user, group)) {
		printf("File already had ACL for that group and user\n");
		// Adding another entry for the same user and
		// group doesn't make sense since the first one
		// is the one that counts. 
		// @todo validate this
		return;
	}

	struct acl_entry *aclEntry = createAclEntry(permissions, user, group);

	if (file->aclTail == NULL){
		file->aclTail = file->aclHead = aclEntry;
		return;
	}

	file->aclTail->next = aclEntry;
	file->aclTail = aclEntry;
}

int parseUserDefinitionLine(char *line) {
	char *userStart = line;
	char *groupStart;
	char *filePathStart;
	char *fileName = NULL;
	struct user_struct *user;
	struct group_struct *group;
	struct user_struct *possibleUser;
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

	possibleUser = findUserByUsername(username);

	int noFile = 0;

	len = 0;
	line++;
	groupStart = line;	

	// Get group name
	while((c = *line) != ' ') {
		if (c == '\0'){
			noFile = 1;
			break;
		}

		len++;
		line++;
	}


	char *groupname = strndup(groupStart, len);

	addUserAndGroup(username, groupname);

	user = findUserByUsername(username);
	group = findGroupByGroupname(groupname);


	if (noFile) {
		if (possibleUser == NULL) {
			printAndExit("The first instance of a user must have a file name");
		}

		addAclToFile(user->file, "rw", user, group);

		free(groupname);
		free(username);

		return 0;
	}

	
	len = 0;
	line++;
	filePathStart = line;	

	// Get file
	while((c = *line) != '\0') {		
		len++;
		line++;
	}

	// File name
	if (len) {
		if (possibleUser != NULL) {
			printAndExit("Only the first instance of the user can contain a file");		
		}

		fileName = strndup(filePathStart, len);		
		user->file = addFileByPath(fileName);
	} else {
		if (possibleUser == NULL) {			
			printAndExit("The first instance of a user must have a file name");	
		}		
	}

	addAclToFile(user->file, "rw", user, group);


	if (fileName != NULL) {
		free(fileName);
	}

	free(groupname);
	free(username);

	return 0;
}

int initFs() {
	root = createFile("/", NULL);

	struct file_struct *tmp = createFile("tmp", root);
	struct file_struct *home = createFile("home", root);

	addAclToFile(root, "r", NULL, NULL);
	addAclToFile(tmp, "rw", NULL, NULL);
	addAclToFile(home, "r", NULL, NULL);

	return 0;
}

char *getLine() {
	int len = INITIAL_LINE_SIZE;
	int index = 0;
	char *line = malloc(len);	
	char *returnLine;
	char c;

	if (line == NULL) {
		printAndExit(NULL);
	}

	while ((c = getchar()) != '\0' && c != '\n') {
		line[index] = c;
		index++;

		if (index > len - 1) {			
			len *= 2;
			line = realloc(line, len);
		}
	}

	line[index] = '\0';

	returnLine = strndup(line, index);

	if (returnLine == NULL) {
		printAndExit(NULL);
	}

	free(line);

	return returnLine;
}

int parseUserDefinitionSection() {
	char *line;

	while (1) {
		line = getLine();

		if (strcmp(line, ".") == 0) {
			free(line);
			break;
		}

		parseUserDefinitionLine(line);

		free(line);
	}

	return 0;
}


void printAclForFile(struct file_struct *file) {
	printf("ACL for file %s\n", file->cmpName);
	struct acl_entry *aclEntry;

	for (aclEntry = file->aclHead; aclEntry != NULL; aclEntry = aclEntry->next) {
		struct user_struct *user = aclEntry->user;
		struct group_struct *group = aclEntry->group;
		char permissions[3];

		char *username;
		char *groupname;

		if (user != NULL) {
			username = user->username;
		} else {
			username = "*";
		}

		if (group != NULL) {
			groupname = group->groupname;
		} else {
			groupname = "*";
		}

		if (aclEntry->readPermission && aclEntry->writePermission) {
			permissions[0] = 'r';
			permissions[1] = 'w';
			permissions[2] = '\0';
		} else if(aclEntry->readPermission) {
			permissions[0] = 'r';
			permissions[1] = '\0';
		} else if (aclEntry->writePermission) {
			permissions[0] = 'w';
			permissions[1] = '\0';
		} else {
			permissions[0] = '-';
			permissions[1] = '\0';
		}		

		printf("\t%s.%s %s\n", username, groupname, permissions);
	}
}

int main(int argc, char *argv[]) {
	initFs();

	printAclForFile(root);

	parseUserDefinitionSection();

	struct file_struct *file = findFileByPath("/home/smb");
	printAclForFile(file);

	return 0;
}
