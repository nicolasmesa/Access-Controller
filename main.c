#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_CMP_SIZE 16
#define MAX_FILE_NAME_SIZE 256
#define INITIAL_LINE_SIZE 100

#define U_VALID 0
#define U_INVALID 1

#define C_YES 0
#define C_NO 1
#define C_INVALID 2

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

struct error_struct {
	int read;
	char *message;
};


static struct file_struct *root;
static struct user_struct *usersHead = NULL;
static struct group_struct *groupsHead = NULL;
static struct error_struct error = {1, NULL};
static char defaultErrorMsg[] = "Error with this entry";


void setError(char *msg) {
	if (error.read == 0) {
		printf("Warning. Setting error without reading prior message (%s)\n", error.message);
	}

	error.read = 0;
	error.message = strdup(msg);
}

char *getError(){
	if (error.read) {
		printf("Warning. Reading error twice (%s)\n", error.message);
		return defaultErrorMsg;
	}

	error.read = 1;
	return error.message;
}

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
    	// Shouldn't happen
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
			return NULL;
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
	char *path = pathStart;	
	struct file_struct *currentFile = root;

	if (!path) {
		setError("Undefined file path");
		return NULL;
	}

	if (*path != '/') {
		setError("File path must start with /");
		return NULL;
	}

	path++;

	if (strlen(path) > MAX_FILE_NAME_SIZE) {
		setError("File name exceeds max file name size");
		return NULL;
	}

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
				setError("Component longer than allowed");
				return NULL;
			}
		}

		cmpName[cmpLength] = '\0';


		struct file_struct *temp = findFileInListByName(currentFile->children, cmpName);

		if(last && temp) {
			setError("File already existed");
			return NULL;
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
		// Should never happen
		printAndExit("User already exists.");
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

int aclUserMatch(struct acl_entry *aclEntry, struct user_struct *user) {
	if (aclEntry->user == NULL || aclEntry->user == user) {
		return 1;
	}

	return 0;
}


int aclGroupMatch(struct acl_entry *aclEntry, struct group_struct *group) {
	if (aclEntry->group == NULL || aclEntry->group == group) {
		return 1;
	}

	return 0;
}

struct acl_entry *findAclByFileUserAndGroup(struct file_struct *file, struct user_struct *user, struct group_struct *group){
	struct acl_entry *aclEntry;

	for (aclEntry = file->aclHead; aclEntry != NULL; aclEntry = aclEntry->next) {
		if (aclUserMatch(aclEntry, user) && aclGroupMatch(aclEntry, group)) {
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

int validateOnlyLetter(char c) {
	if (c < 'a' || c > 'z') {
		return 0;
	}

	return 1;
}

char *getUsernameAndGroupname(char *userStart, char **username, char **groupname) {
	char *line = userStart;
	char *groupStart;
	char c;
	int len = 0;

	//@todo *.group, user.*, *.*

	// Get user name
	while((c = *line) != '.') {
		if (!validateOnlyLetter(c)) {
			setError("Invalid characters in the user name");
			return NULL;
		}

		len++;
		line++;
	}


	if (!len) {
		setError("Empty string supplied for the users");
		return NULL;
	}

	*username = strndup(userStart, len);	

	len = 0;
	line++;
	groupStart = line;	

	// Get group name
	while((c = *line) != ' ') {
		if (c == '\0'){
			break;
		}

		if (!validateOnlyLetter(c)) {
			free(*username);
			setError("Invalid characters in the group name");
			return NULL;
		}

		len++;
		line++;
	}

	if (!len) {
		free(*username);
		setError("Empty string supplied for the group name");
		return NULL;
	}

	*groupname = strndup(groupStart, len);

	return line;
}

char *getFilepath(char *line, char **filePath) {
	int len = 0;	
	char *filePathStart = line;	
	char c;

	if (*filePathStart != '/') {
		setError("File path must start with /");
		return NULL;
	}

	// Get file
	while((c = *line) != '\0') {		
		len++;
		line++;
	}

	if (len > MAX_FILE_NAME_SIZE) {
		setError("File name exceeds max file name size");
		return NULL;
	}

	*filePath = strndup(filePathStart, len);

	return line;	
}

int parseUserDefinitionLine(char *line) {	
	char *filePathStart;
	char *fileName = NULL;
	struct user_struct *user;
	struct group_struct *group;
	struct user_struct *possibleUser;
	struct file_struct *file;
	char *username;
	char *groupname;	
	int len = 0;

	line = getUsernameAndGroupname(line, &username, &groupname);

	// An error ocurred
	if (line == NULL) {
		return U_INVALID;
	}

	possibleUser = findUserByUsername(username);

	// Means no file specified and first time we see the user
	if (*line != ' ' && possibleUser == NULL) {			
		free(groupname);
		free(username);

		setError("The first instance of a user must have a file name");			
		return U_INVALID;			
	}

	// Means no file specified but it is ot the first time we see the user
	if (*line != ' ') {
		addUserAndGroup(username, groupname);
		user = findUserByUsername(username);
		group = findGroupByGroupname(groupname);

		addAclToFile(user->file, "rw", user, group);

		return U_VALID;
	}

	// Skip ' '
	line++;
	line = getFilepath(line, &filePathStart);

	// Error with the file. Error msg is already set
	if (line == NULL) {		
		free(groupname);
		free(username);

		return U_INVALID;
	}

	len = strlen(filePathStart);

	// Means file specified but it is not the first time we see the user
	if (len && possibleUser != NULL) {
		free(groupname);
		free(username);
		setError("Only the first instance of the user can contain a file");		
		return U_INVALID;
	}

	// Means no file specified but it is the first time we see the user
	if (!len && possibleUser == NULL) {
		free(groupname);
		free(username);

		setError("The first instance of a user must have a file name");	
		return U_INVALID;
	}	

	fileName = strndup(filePathStart, len);		
	file = addFileByPath(fileName);

	// Error creating the file. Error msg is already set
	if (file == NULL) {
		free(groupname);
		free(username);
		return U_INVALID;
	}

	addUserAndGroup(username, groupname);
	user = findUserByUsername(username);
	group = findGroupByGroupname(groupname);

	user->file = file;

	addAclToFile(user->file, "rw", user, group);

	if (fileName != NULL) {
		free(fileName);
	}

	free(groupname);
	free(username);

	return U_VALID;
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
	int num = 1;
	int result;
	char *error;

	while (1) {
		line = getLine();

		if (strcmp(line, ".") == 0) {
			free(line);
			break;
		}

		result = parseUserDefinitionLine(line);

		if (result == U_VALID) {
			printf("%d\tY\n", num);
		} else {			
			error = getError();
			printf("%d\tX\t%s\n", num, error);
		}

		num++;
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

int userBelongsToGroup(struct user_struct *user, struct group_struct *group){
	if (findGroupUser(group, user->username) == NULL) {
		return 0;
	}

	return 1;
}

void clearAclForFile(struct file_struct *file) {
	struct acl_entry *aclEntry = file->aclHead;	

	while (aclEntry != NULL) {		
		struct acl_entry *temp = aclEntry;
		aclEntry = aclEntry->next;
		free(temp);
	}
}

int executeRead(struct user_struct *user, struct group_struct *group, struct file_struct *file) {
	struct file_struct *currentFile = file;	

	while (currentFile != NULL) {		
		struct acl_entry *aclEntry = findAclByFileUserAndGroup(currentFile, user, group);

		if (aclEntry == NULL || !aclEntry->readPermission) {
			setError("No read permissions on path to file");		
			return C_NO;
		}

		currentFile = currentFile->parent;
	}

	return C_YES;
}

int executeWrite(struct user_struct *user, struct group_struct *group, struct file_struct *file) {	
	struct acl_entry *aclEntry = findAclByFileUserAndGroup(file, user, group);
	struct file_struct *parentFile = file->parent;

	if (aclEntry == NULL || !aclEntry->writePermission) {
		setError("No write permissions on this file");
		return C_NO;
	}

	if (parentFile == NULL) {
		setError("No write permissions on root file");
		return C_NO;
	}

	return executeRead(user, group, parentFile);
}

int executeCreate(struct user_struct *user, struct group_struct *group, char *filename) {
	setError("Execute create not implemented");
	return C_INVALID;
}

int executeDelete(struct user_struct *user, struct group_struct *group, struct file_struct *file) {
	struct file_struct *parentFile = file->parent;
	struct file_struct *window = parentFile->children;

	if (file->children != NULL)	{
		setError("Can't delete a file that has children");
		return C_NO;
	}

	// Root
	if (parentFile == NULL) {
		setError("Can't delete the root file");
		return C_NO;
	}

	// Can't write
	if (executeWrite(user, group, file) == C_NO) {		
		return C_NO;
	}

	if (window == file) {
		parentFile->children = file->next;
	} else  {
		while (window != NULL) {
			if (window->next == file) {
				window->next = file->next;
			}

			window = window->next;
		}
	}

	clearAclForFile(file);
	free(file);

	return C_YES;
}

int executeAcl(struct user_struct *user, struct group_struct *group, struct file_struct *file) {
	setError("Execute acl not implemented");
	return C_INVALID;
}

int executeCommand(char *command, char *username, char *groupname, char *filename) {
	struct user_struct *user = findUserByUsername(username);
	struct group_struct *group = findGroupByGroupname(groupname);
	struct file_struct *file = findFileByPath(filename);	

	if (user == NULL) {
		setError("User does not exist");
		return C_INVALID;
	}

	if (group == NULL) {
		setError("Group does not exist");
		return C_INVALID;
	}

	if (!userBelongsToGroup(user, group)) {
		setError("User does not belong to group");
		return C_INVALID;		
	}


	if (strcmp(command, "READ") == 0) {		
		if (file == NULL) {
			setError("File does not exist");
			return C_INVALID;
		}

		return executeRead(user, group, file);		
	}


	if (strcmp(command, "WRITE") == 0) {		
		if (file == NULL) {
			setError("File does not exist");
			return C_INVALID;
		}

		return executeWrite(user, group, file);		
	}


	if (strcmp(command, "CREATE") == 0) {		
		if (file != NULL) {
			setError("File already exists");
			return C_INVALID;
		}		

		return executeCreate(user, group, filename);		
	}


	if (strcmp(command, "DELETE") == 0) {		
		if (file == NULL) {
			setError("File does not exist");
			return C_INVALID;
		}

		return executeDelete(user, group, file);		
	}


	if (strcmp(command, "ACL") == 0) {		
		if (file == NULL) {
			setError("File does not exist");
			return C_INVALID;
		}

		return executeAcl(user, group, file);		
	}

	setError("Invalid command");
	return C_INVALID;
}

int parseCommandLine(char *line) {
	char command[7];
	int len = 0;
	char *username;
	char *groupname;
	char *filename;	
	char c;

	while ((c = *line) != ' ') {
		if (len > 6) {
			setError("Invalid command");
			return C_INVALID;
		}

		command[len] = *line;		

		len++;
		line++;
	}

	command[len] = '\0';

	line++;

	line = getUsernameAndGroupname(line, &username, &groupname);

	if (*line != ' ') {
		return C_INVALID;
		setError("You have to include a file name");
	}

	line++;
	line = getFilepath(line, &filename);

	// Error msg already set
	if (line == NULL) {
		return C_INVALID;
	}

	return executeCommand(command, username, groupname, filename);
}

void parseFileOpearationSection(){
	char *line;	
	int num = 1;
	int result;
	char *error;

	while (1) {
		line = getLine();

		if (*line == '\0') {			
			free(line);		
			break;	
		}

		result = parseCommandLine(line);

		if (result == C_YES) {
			printf("%d\tY\t%s\n", num, line);
		}

		if (result == C_NO) {
			error = getError();
			printf("%d\tN\t%s\t%s\n", num, line, error);
		}

		if (result == C_INVALID) {			
			error = getError();
			printf("%d\tX\t%s\t%s\n", num, line, error);
		}

		num++;
		free(line);
	}
}

int main(int argc, char *argv[]) {
	initFs();
	parseUserDefinitionSection();
	parseFileOpearationSection();

	return 0;
}
