#include <unistd.h>
#include <limits.h>
#include <reent.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/iosupport.h>
#include <sys/param.h>

#include "path.h"

/* CWD always start with "/" */
static char _current_working_directory [PATH_MAX] = "/";
static char temp_cwd [PATH_MAX];

int chdir (const char *path) {
	struct _reent *r = _REENT;

	int dev;

	dev = FindDevice(path);

	if (dev < 0) {
		r->_errno = ENODEV;
		return -1;
	}

	if ( devoptab_list[dev]->chdir_r == NULL) {
		r->_errno = ENOSYS;
		return -1;
	}

	/* Work with a copy of current working directory */
	strncpy (temp_cwd, _current_working_directory, PATH_MAX);

	/* Concatenate the path to the CWD */
	r->_errno = update_path(temp_cwd, path, PATH_MAX);
	if (r->_errno) {
		return -1;
	}

	/* Try changing directories on the device */
	if (devoptab_list[dev]->chdir_r (r, temp_cwd) == -1) {
		return -1;
	}

	/* Since it worked, set the new CWD and default device */
	setDefaultDevice(dev);
	strncpy (_current_working_directory, temp_cwd, PATH_MAX);

	return 0;
}

char *getcwd(char *buf, size_t size) {

	struct _reent *r = _REENT;

	if (size < (strnlen (_current_working_directory, PATH_MAX) + 1)) {
		r->_errno = ERANGE;
		return NULL;
	}

	if (size == 0) {
		if (buf != NULL) {
			r->_errno = EINVAL;
			return NULL;
		}
		buf = malloc(PATH_MAX);
		size = PATH_MAX;
	}

	if (buf == NULL) {
		r->_errno = EINVAL;
		return NULL;
	}

	strncpy (buf, _current_working_directory, size);

	return buf;
}
