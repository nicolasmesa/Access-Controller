/*
 * I chose to allow modifications to the permissions of /tmp.
 * /home doesn't allow this since no user will have write permissions
 * on it so it is not possible to execute the ACL command on it.
 */

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
static int endOfInput = 0;


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

struct acl_entry *createAclEntry(char *permissions, struct user_struct *user, struct group_struct *group) {
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

char *getUsername(char *userStart, char **username) {
	char *line = userStart;
	char c;
	int len = 0;

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

	return line;
}

char *getGroupname(char *groupStart, char **groupname) {
	char *line = groupStart;
	char c;
	int len = 0;	

	// Get group name
	while((c = *line) != ' ') {
		if (c == '\0'){
			break;
		}

		if (!validateOnlyLetter(c)) {			
			setError("Invalid characters in the group name");
			return NULL;
		}

		len++;
		line++;
	}

	if (!len) {
		setError("Empty string supplied for the group name");
		return NULL;
	}

	*groupname = strndup(groupStart, len);

	return line;
}

char *getUsernameAndGroupname(char *userStart, char **username, char **groupname) {
	char *line = userStart;		

	line = getUsername(line, username);

	if (line == NULL) {
		return NULL;
	}

	line++;

	line = getGroupname(line, groupname);

	if (line == NULL) {
		free(*username);
		return NULL;
	}
	

	return line;
}


char *getUsernameAndGroupnameForAcl(char *userStart, char **username, char **groupname) {	
	char *line = userStart;

	if (*userStart == '*') {
		*username = strdup("*");

		if (username == NULL) {
			printAndExit(NULL);
		}

		line++;
	} else {
		line = getUsername(line, username);

		if (line == NULL) {
			return NULL;
		}		
	}

	if (*line != '.') {
		setError("Expected . between username and groupname");
		return NULL;
	}

	line++;


	if(*line == '*') {
		*groupname = strdup("*");

		if (groupname == NULL) {
			printAndExit(NULL);
		}

		line++;
	} else {
		line = getGroupname(line, groupname);

		if (line == NULL) {
			return NULL;
		}
	}

	return line;
}

char *getFilepath(char *line, char **filePath) {
	int len = 0;	
	char *filePathStart = line;	
	char c;
	char *lastSlash;

	if (*filePathStart != '/') {
		setError("File path must start with /");
		return NULL;
	}

	lastSlash = filePathStart;

	// Get file
	while((c = *line) != '\0') {

		if (c != '/' && c != '.' && !validateOnlyLetter(c)) {
			setError("Invalid characters in the file name");
			return NULL;
		}		

		len++;
		line++;

		if (*line == '/') {
			if (line - lastSlash < 2) {
				setError("Can't have two consecutive slashes (/) in a file");
				return NULL;
			}

			lastSlash = line;
		}
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

	while ((c = getchar()) != EOF && c != '\n') {
		line[index] = c;
		index++;

		if (index > len - 1) {			
			len *= 2;
			line = realloc(line, len);
		}
	}


	if (c == EOF) {
		endOfInput = 1;
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

		if (endOfInput) {
			free(line);
			break;
		}

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

void getPermissionsAsText(struct acl_entry *aclEntry, char permissions[3]) {
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

		getPermissionsAsText(aclEntry, permissions);		

		printf("\t%s.%s %s\n", username, groupname, permissions);
	}
}

int userBelongsToGroup(struct user_struct *user, struct group_struct *group){
	if (findGroupUser(group, user->username) == NULL) {
		return 0;
	}

	return 1;
}

char *getPermissions(char *line, char permissions[3]) {

	// rw
	if (strlen(line) == 2) {
		if (line[0] != 'r' || line[1] != 'w') {
			setError("Invalid permissions");
			return NULL;
		}

		permissions[0] = 'r';
		permissions[1] = 'w';
		permissions[2] = '\0';

		return (line + 2);
	}

	// r, w, -
	if (strlen(line) == 1) {
		if (*line != 'r' && *line != 'w' && *line != '-') {
			setError("Invalid permissions");
			return NULL;
		}

		permissions[0] = *line;
		permissions[1] = '\0';
		return (line + 1);
	}

	setError("Invalid permissions");
	return NULL;
}

void clearAclList(struct acl_entry *aclEntryHead) {
	struct acl_entry *aclEntry = aclEntryHead;
		
	while (aclEntry != NULL) {		
		struct acl_entry *temp = aclEntry;
		aclEntry = aclEntry->next;
		free(temp);
	}	
}

void ignoreRestOfAcl() {
	while (1) {		
		char *line = getLine();

		if (*line == '\0') {						
			free(line);		
			break;	
		}

		if (strcmp(line, ".") == 0){
			free(line);
			break;
		}

		free(line);
	}
}

int parseAclList(struct acl_entry **aclEntryHead, struct acl_entry **aclEntryTail) {
	char *line;
	char *username;
	char *groupname;
	char permissions[3];
	struct user_struct *user;
	struct group_struct *group;

	*aclEntryHead = NULL;
	*aclEntryTail = NULL;


	while (1) {
		line = getLine();
		char *startOfLine = line;

		if (*line == '\0') {						
			free(startOfLine);
			setError("Unexpected end of file");
			return C_INVALID;			
		}

		if (strcmp(line, ".") == 0) {
			free(startOfLine);
			return C_YES;
		}

		line = getUsernameAndGroupnameForAcl(line, &username, &groupname);

		if (line == NULL) {
			free(startOfLine);
			return C_INVALID;
		}

		if (strcmp(username, "*") == 0){
			user = NULL;
		} else {			
			user = findUserByUsername(username);

			if (user == NULL) {				
				user = createUser(username);
			}	
		}

		if (strcmp(groupname, "*") == 0) {
			group = NULL;
		} else {
			group = findGroupByGroupname(groupname);			

			if (group == NULL) {
				group = createGroup(groupname);
			}			
		}

		// Add user to group if necessary
		if (user != NULL && group != NULL) {
			addUserToGroup(user, group);
		}

		if (*line != ' ') {
			setError("Missing permissions");
			free(startOfLine);
			return C_INVALID;
		}

		// Skip ' '
		line++;

		line = getPermissions(line, permissions);		

		if (line == NULL) {
			free(startOfLine);
			return C_INVALID;
		}

		if (*aclEntryHead == NULL) {
			*aclEntryHead = createAclEntry(permissions, user, group);
			*aclEntryTail = *aclEntryHead;
		} else {
			(*aclEntryTail)->next = createAclEntry(permissions, user, group);
			*aclEntryTail = (*aclEntryTail)->next;
		}		

		free(startOfLine);
	}


	return C_YES;
}

void clearAclForFile(struct file_struct *file) {
	clearAclList(file->aclHead);
}

void copyAcl(struct file_struct *dst, struct file_struct *src) {	
	clearAclForFile(dst);

	struct acl_entry *srcAclEntry = src->aclHead;
	struct acl_entry *dstAclEntry = NULL;
	struct acl_entry *prev = NULL;
	struct acl_entry *dstHead = NULL;	

	while (srcAclEntry != NULL){
		char permissions[3];

		getPermissionsAsText(srcAclEntry, permissions);		

		dstAclEntry = createAclEntry(permissions, srcAclEntry->user, srcAclEntry->group);		

		if (dstAclEntry == NULL) {
			printAndExit(NULL);
		}

		if (dstHead == NULL) {
			dstHead = dstAclEntry;
		}

		if (prev != NULL) {
			prev->next = dstAclEntry;
		}

		prev = dstAclEntry;
		srcAclEntry = srcAclEntry->next;		
	}	

	dst->aclHead = dstHead;
	dst->aclTail = dstAclEntry;

}

int executeRead(struct user_struct *user, struct group_struct *group, struct file_struct *file) {
	struct file_struct *currentFile = file;	

	while (currentFile != NULL) {		
		struct acl_entry *aclEntry = findAclByFileUserAndGroup(currentFile, user, group);

		if (aclEntry == NULL || !aclEntry->readPermission) {
			setError("Can't read file");		
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

int executeAcl(struct user_struct *user, struct group_struct *group, struct file_struct *file) {
	int result;
	struct acl_entry *aclEntryHead;
	struct acl_entry *aclEntryTail;

	result = executeWrite(user, group, file);

	if (result != C_YES) {
		return result;
	}

	result = parseAclList(&aclEntryHead, &aclEntryTail);

	// Error already set
	if (result != C_YES) {		
		return result;
	}

	if (aclEntryHead == NULL || aclEntryTail == NULL) {
		setError("The file can't have a NULL ACL");
		return C_INVALID;
	}

	clearAclForFile(file);	

	file->aclHead = aclEntryHead;
	file->aclTail = aclEntryTail;

	return C_YES;
}

int executeCreate(struct user_struct *user, struct group_struct *group, char *filename) {
	char *fileLine = filename;
	char *lastSlash = fileLine;
	char *parentPath;	
	char *cmpName;
	int index = 0;
	int result;
	char c;
	struct file_struct *parentFile;
	struct file_struct *newFile;
	struct acl_entry *aclEntryHead;
	struct acl_entry *aclEntryTail;

	if (*lastSlash != '/') {		
		setError("File path must start with /");
		
		ignoreRestOfAcl();
		return C_INVALID;
	}

	while ((c = *fileLine) != '\0') {
		if (c == '/') {
			lastSlash = fileLine;
		}

		fileLine++;
	}

	index = lastSlash - filename;

	parentPath = strndup(filename, index);

	if(parentPath == NULL) {
		printAndExit(NULL);
	}


	lastSlash++;
	cmpName = strdup(lastSlash);	

	if (cmpName == NULL) {
		printAndExit(NULL);
	}


	parentFile = findFileByPath(parentPath);

	if (parentFile == NULL) {
		setError("Parent file does not exist");

		free(parentPath);
		free(cmpName);
		ignoreRestOfAcl();

		return C_INVALID;
	}


	result = executeWrite(user, group, parentFile);

	if (result != C_YES) {		
		free(parentPath);
		free(cmpName);

		ignoreRestOfAcl();
		return result;
	}

	if (findFileByPath(filename) != NULL) {
		free(parentPath);
		free(cmpName);

		setError("File already exists");
		ignoreRestOfAcl();
		return C_INVALID;
	}

	result = parseAclList(&aclEntryHead, &aclEntryTail);


	if (result != C_YES) {
		free(parentPath);
		free(cmpName);	
		clearAclList(aclEntryHead);

		ignoreRestOfAcl();

		return result;
	}


	newFile = createFile(cmpName, parentFile);

	if (newFile == NULL) {		
		free(parentPath);
		free(cmpName);
		return C_INVALID;
	}

	// If there was no ACL, inherit from parent directory
	if (aclEntryHead == NULL) {
		copyAcl(newFile, parentFile);		
	} else {
		newFile->aclHead = aclEntryHead;
		newFile->aclTail = aclEntryTail;
	}

	free(parentPath);
	free(cmpName);
	

	return C_YES;
}

int executeDelete(struct user_struct *user, struct group_struct *group, struct file_struct *file) {
	struct file_struct *parentFile = file->parent;
	struct file_struct *window = parentFile->children;
	int result;

	if (file->children != NULL)	{
		setError("Can't delete a file that has children");
		return C_NO;
	}

	// Root
	if (parentFile == NULL) {
		setError("Can't delete the root file");
		return C_NO;
	}


	result = executeWrite(user, group, parentFile);

	// Can't write
	if (result != C_YES) {		
		return result;
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

			ignoreRestOfAcl();
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

			ignoreRestOfAcl();
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
	int result;
	int createOrAcl = 0;

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

	if (strcmp(command, "CREATE") != 0 || strcmp(command, "ACL") != 0) {
		createOrAcl = 1;
	}

	line++;

	line = getUsernameAndGroupname(line, &username, &groupname);

	if (line == NULL) {
		if (createOrAcl) {
			ignoreRestOfAcl();
		}

		return C_INVALID;		
	}

	if (*line != ' ') {
		if (createOrAcl) {
			ignoreRestOfAcl();
		}

		setError("You have to include a file name");
		return C_INVALID;		
	}

	line++;
	line = getFilepath(line, &filename);

	// Error msg already set. 
	if (line == NULL) {
		if (createOrAcl) {
			ignoreRestOfAcl();
		}

		return C_INVALID;
	}

	result = executeCommand(command, username, groupname, filename);	

	free(username);
	free(groupname);
	free(filename);

	return result;
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
