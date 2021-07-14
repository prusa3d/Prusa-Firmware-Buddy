struct __argv {
	int argvMagic;
	char *commandLine;
	int length;
	int argc;
	char **argv;
	char **endARGV;
};


void build_argv (struct __argv* argstruct ) {

	char *data = argstruct->commandLine;
	int len = argstruct->length;

	char** argv = (char**)(((int)data + len + sizeof(char **)) & ~(sizeof(char **)-1));
	char* end = data + len - 1;
	int argCount = 0;

	do {
		argv[argCount++] = data;			// Add next arg to argv list
		while (*(data) && data < end) data++;		// Move to next NULL delimiter
		data++;						// Move to one after the NULL delimiter
	} while (data < end);

	*end = '\0';						// Force NULL terminator for last arg

	argstruct->argv = argv;
	argstruct->argc = argCount;
	argstruct->endARGV = &argv[argCount];
}
