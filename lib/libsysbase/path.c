#include <errno.h>
#include <string.h>
#include "path.h"

#define DIRECTORY_SEPARATOR_CHAR '/'
const char DIRECTORY_SEPARATOR[] = "/";
const char DIRECTORY_THIS[] = ".";
const char DIRECTORY_PARENT[] = "..";

int update_path(char *path, const char *new_path, int max_length) {
	char *pathEnd;
	int path_length;
	const char *new_path_end;
	int new_path_size;

	/* If the new_path bit starts with a slash, start at root */
	if (new_path[0] == DIRECTORY_SEPARATOR_CHAR) {
		path[0] = '/';
		pathEnd = path + 1;
		pathEnd[0] = '\0';
		path_length = 1;
	} else {
		path_length = strnlen (path, max_length);

		/* assumes path ends in a directory separator */
		if (path_length >= max_length) {
			return ENAMETOOLONG;
		}
		pathEnd = path + path_length;
		if (pathEnd[-1] != DIRECTORY_SEPARATOR_CHAR) {
			pathEnd[0] = DIRECTORY_SEPARATOR_CHAR;
			pathEnd += 1;
		}
	}

	new_path_end = new_path;
	do {
		/* Advance past any separators in new_path */
		while (new_path[0] == DIRECTORY_SEPARATOR_CHAR) {
			new_path += 1;
		}

		/* Grab the next directory name from new_path */
		new_path_end = strchr (new_path, DIRECTORY_SEPARATOR_CHAR);
		if (new_path_end == NULL) {
			new_path_end = strrchr (new_path, '\0');
		} else {
			new_path_end += 1;
		}

		new_path_size = (new_path_end - new_path);
		if (new_path_size == 0) {
			break;
		}

		if ((strncmp (new_path, DIRECTORY_THIS, sizeof(DIRECTORY_THIS) - 1) == 0)
			&& ((new_path[sizeof(DIRECTORY_THIS)-1] == DIRECTORY_SEPARATOR_CHAR)
				||(new_path[sizeof(DIRECTORY_THIS)-1] == '\0')))
		{
			/* Don't copy anything */
		} else 	if ((strncmp (new_path, DIRECTORY_PARENT, sizeof(DIRECTORY_PARENT) - 1) == 0)
			&& ((new_path[sizeof(DIRECTORY_PARENT)-1] == DIRECTORY_SEPARATOR_CHAR)
				||(new_path[sizeof(DIRECTORY_PARENT)-1] == '\0')))
		{
			/* Go up one level of in the path */
			if (pathEnd[-1] == DIRECTORY_SEPARATOR_CHAR) {
				// Remove trailing separator
				pathEnd[-1] = '\0';
			}
			pathEnd = strrchr (path, DIRECTORY_SEPARATOR_CHAR);
			if (pathEnd == NULL) {
				/* Can't go up any higher, return false */
				return ENOENT;
			}
			path_length = pathEnd - path;
			pathEnd += 1;
		} else {
			path_length += new_path_size;
			if (path_length >= max_length) {
				return ENAMETOOLONG;
			}
			/* Copy the next part over */
			strncpy (pathEnd, new_path, new_path_size);
			pathEnd += new_path_size;
		}
		pathEnd[0] = '\0';
		new_path += new_path_size;
	} while (new_path_size != 0);

	if (pathEnd[-1] != DIRECTORY_SEPARATOR_CHAR) {
		if (path_length + 1 >= max_length) {
			return ENAMETOOLONG;
		}
		pathEnd[0] = DIRECTORY_SEPARATOR_CHAR;
		pathEnd[1] = 0;
		pathEnd += 1;
	}

	return 0;
}
