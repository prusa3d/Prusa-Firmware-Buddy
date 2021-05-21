#include <unistd.h>
#include <limits.h>
#include <reent.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/iosupport.h>
#include <sys/param.h>

/* CWD always start with "/" */
static char _current_working_directory [PATH_MAX] = "/";
static char temp_cwd [PATH_MAX];

#define DIRECTORY_SEPARATOR_CHAR '/'
const char DIRECTORY_SEPARATOR[] = "/";
const char DIRECTORY_THIS[] = ".";
const char DIRECTORY_PARENT[] = "..";

int _concatenate_path (struct _reent *r, char *path, const char *extra, int maxLength) {
	char *pathEnd;
	int pathLength;
	const char *extraEnd;
	int extraSize;

	pathLength = strnlen (path, maxLength);

	/* assumes path ends in a directory separator */
	if (pathLength >= maxLength) {
		r->_errno = ENAMETOOLONG;
		return -1;
	}
	pathEnd = path + pathLength;
	if (pathEnd[-1] != DIRECTORY_SEPARATOR_CHAR) {
		pathEnd[0] = DIRECTORY_SEPARATOR_CHAR;
		pathEnd += 1;
	}

	extraEnd = extra;

	/* If the extra bit starts with a slash, start at root */
	if (extra[0] == DIRECTORY_SEPARATOR_CHAR) {
		pathEnd = strchr (path, DIRECTORY_SEPARATOR_CHAR) + 1;
		pathEnd[0] = '\0';
	}
	do {
		/* Advance past any separators in extra */
		while (extra[0] == DIRECTORY_SEPARATOR_CHAR) {
			extra += 1;
		}

		/* Grab the next directory name from extra */
		extraEnd = strchr (extra, DIRECTORY_SEPARATOR_CHAR);
		if (extraEnd == NULL) {
			extraEnd = strrchr (extra, '\0');
		} else {
			extraEnd += 1;
		}

		extraSize = (extraEnd - extra);
		if (extraSize == 0) {
			break;
		}

		if ((strncmp (extra, DIRECTORY_THIS, sizeof(DIRECTORY_THIS) - 1) == 0)
			&& ((extra[sizeof(DIRECTORY_THIS)-1] == DIRECTORY_SEPARATOR_CHAR)
				||(extra[sizeof(DIRECTORY_THIS)-1] == '\0')))
		{
			/* Don't copy anything */
		} else 	if ((strncmp (extra, DIRECTORY_PARENT, sizeof(DIRECTORY_PARENT) - 1) == 0)
			&& ((extra[sizeof(DIRECTORY_PARENT)-1] == DIRECTORY_SEPARATOR_CHAR)
				||(extra[sizeof(DIRECTORY_PARENT)-1] == '\0')))
		{
			/* Go up one level of in the path */
			if (pathEnd[-1] == DIRECTORY_SEPARATOR_CHAR) {
				// Remove trailing separator
				pathEnd[-1] = '\0';
			}
			pathEnd = strrchr (path, DIRECTORY_SEPARATOR_CHAR);
			if (pathEnd == NULL) {
				/* Can't go up any higher, return false */
				r->_errno = ENOENT;
				return -1;
			}
			pathLength = pathEnd - path;
			pathEnd += 1;
		} else {
			pathLength += extraSize;
			if (pathLength >= maxLength) {
				r->_errno = ENAMETOOLONG;
				return -1;
			}
			/* Copy the next part over */
			strncpy (pathEnd, extra, extraSize);
			pathEnd += extraSize;
		}
		pathEnd[0] = '\0';
		extra += extraSize;
	} while (extraSize != 0);

	if (pathEnd[-1] != DIRECTORY_SEPARATOR_CHAR) {
		pathEnd[0] = DIRECTORY_SEPARATOR_CHAR;
		pathEnd[1] = 0;
		pathEnd += 1;
	}

	return 0;
}

int chdir (const char *path) {
	struct _reent *r = _REENT;

	int dev;
	const char *pathPosition;

	/* Make sure the path is short enough */
	if (strnlen (path, PATH_MAX) >= PATH_MAX) {
		r->_errno = ENAMETOOLONG;
		return -1;
	}

	if (strchr (path, ':') != NULL) {
		strncpy (temp_cwd, path, PATH_MAX);
		/* Move path past device name */
		path = strchr (path, ':') + 1;
	} else {
		strncpy (temp_cwd, _current_working_directory, PATH_MAX);
	}

	pathPosition = strchr (temp_cwd , ':');

	if (pathPosition == NULL) {
		pathPosition = temp_cwd;
	} else {
		pathPosition++;
	}

	/* Make sure the path starts in the root directory */
	if (pathPosition[0] != DIRECTORY_SEPARATOR_CHAR) {
		r->_errno = ENOENT;
		return -1;
	}

	/* Concatenate the path to the CWD */
	if (_concatenate_path (r, temp_cwd, path, PATH_MAX) == -1) {
		return -1;
	}

	/* Get device from path name */
	dev = FindDevice(temp_cwd);

	if (dev < 0) {
		r->_errno = ENODEV;
		return -1;
	}

	r->deviceData = devoptab_list[dev]->deviceData;

	if ( devoptab_list[dev]->chdir_r == NULL) {
		r->_errno = ENOSYS;
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

	if ( size < (strnlen (_current_working_directory, PATH_MAX) + 1)) {
		r->_errno = ERANGE;
		return NULL;
	}

	strncpy (buf, _current_working_directory, size);

	return buf;
}
