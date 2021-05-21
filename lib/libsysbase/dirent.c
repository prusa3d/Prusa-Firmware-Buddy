#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/param.h>
#include <sys/dirent.h>
#include <sys/iosupport.h>

static DIR_ITER * __diropen (const char *path) {
	struct _reent *r = _REENT;
	DIR_ITER *handle = NULL;
	DIR_ITER *dir = NULL;
	int dev;

	dev = FindDevice(path);

	if(dev!=-1) {
		if(devoptab_list[dev]->diropen_r) {

			r->deviceData = devoptab_list[dev]->deviceData;

			handle = (DIR_ITER *)malloc( sizeof(DIR_ITER) + devoptab_list[dev]->dirStateSize );

			if ( NULL != handle ) {
				handle->device = dev;
				handle->dirStruct = ((void *)handle) + sizeof(DIR_ITER);

				dir = devoptab_list[dev]->diropen_r(r, handle, path);

				if ( dir == NULL ) {
					free (handle);
					handle = NULL;
				}
			} else {
				r->_errno = ENOSR;
				handle = NULL;
			}
		} else {
			r->_errno = ENOSYS;
		}
	} else {
		r->_errno = ENODEV;
	}

	return handle;
}

static int __dirreset (DIR_ITER *dirState) {
	struct _reent *r = _REENT;
	int ret = -1;
	int dev = 0;

	if (dirState != NULL) {
		dev = dirState->device;

		if(devoptab_list[dev]->dirreset_r) {
			r->deviceData = devoptab_list[dev]->deviceData;
			ret = devoptab_list[dev]->dirreset_r(r, dirState);
		} else {
			r->_errno = ENOSYS;
		}
	}
	return ret;
}

static int __dirnext (DIR_ITER *dirState, char *filename, struct stat *filestat) {
	struct _reent *r = _REENT;
	int ret = -1;
	int dev = 0;

	if (dirState != NULL) {
		dev = dirState->device;

		if(devoptab_list[dev]->dirnext_r) {
			r->deviceData = devoptab_list[dev]->deviceData;
			ret = devoptab_list[dev]->dirnext_r(r, dirState, filename, filestat);
		} else {
			r->_errno = ENOSYS;
		}
	}
	return ret;
}

static int __dirclose (DIR_ITER *dirState) {
	struct _reent *r = _REENT;
	int ret = -1;
	int dev = 0;

	if (dirState != NULL) {
		dev = dirState->device;

		if (devoptab_list[dev]->dirclose_r) {
			r->deviceData = devoptab_list[dev]->deviceData;
			ret = devoptab_list[dev]->dirclose_r (r, dirState);
		} else {
			r->_errno = ENOSYS;
		}

		free (dirState);
	}
	return ret;
}

DIR* opendir (const char *dirname) {
	DIR* dirp = malloc (sizeof(DIR));
	if (!dirp) {
		errno = ENOMEM;
		return NULL;
	}

	dirp->dirData = __diropen (dirname);
	if (!dirp->dirData) {
		free (dirp);
		return NULL;
	}

	dirp->position = 0;	// 0th position means no file name has been returned yet
	dirp->fileData.d_ino = -1;
	dirp->fileData.d_name[0] = '\0';

	return dirp;
}


int closedir (DIR *dirp) {
	int res;

	if (!dirp) {
		errno = EBADF;
		return -1;
	}

	res = __dirclose (dirp->dirData);
	free (dirp);
	return res;
}


struct dirent* readdir (DIR *dirp) {
	struct stat st;
	char filename[NAME_MAX];
	int res;
	int olderrno = errno;

	if (!dirp) {
		errno = EBADF;
		return NULL;
	}

	res = __dirnext (dirp->dirData, filename, &st);

	if (res < 0) {
		if (errno == ENOENT) {
			// errno == ENONENT set by dirnext means it's end of directory
			// But readdir should not touch errno in case of dir end
			errno = olderrno;
		}
		return NULL;
	}

	// We've moved forward in the directory
	dirp->position += 1;

	if (strnlen(filename, NAME_MAX) >= sizeof(dirp->fileData.d_name)) {
		errno = EOVERFLOW;
		return NULL;
	}

	strncpy (dirp->fileData.d_name, filename, sizeof(dirp->fileData.d_name));
	dirp->fileData.d_ino = st.st_ino;
	dirp->fileData.d_type = S_ISDIR(st.st_mode)?DT_DIR:DT_REG;

	return &(dirp->fileData);
}


int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result) {
	struct stat st;
	char filename[NAME_MAX];
	int res;

	if (!dirp) {
		return EBADF;
	}

	res = __dirnext (dirp->dirData, filename, &st);

	if (res < 0) {
		res = errno;
		*result = NULL;
		if (errno == ENOENT) {
			// errno == ENONENT set by dirnext means it's end of directory
			// But readdir should not touch errno in case of dir end
			res = 0;
		}
		return res;
	}

	// We've moved forward in the directory
	dirp->position += 1;

	if (strnlen(filename, NAME_MAX) >= sizeof(entry->d_name)) {
		errno = EOVERFLOW;
		return EOVERFLOW;
	}

	strncpy (entry->d_name, filename, sizeof(entry->d_name));
	entry->d_ino = st.st_ino;

	*result = entry;
	return 0;
}


void rewinddir (DIR *dirp) {
	if (!dirp) {
		return;
	}

	__dirreset (dirp->dirData);
	dirp->position = 0;
}


void seekdir(DIR *dirp, long int loc) {
	char filename[NAME_MAX];

	if (!dirp || loc < 0) {
		return;
	}

	if (dirp->position > loc) {
		// The entry we want is before the one we have,
		// so we have to start again from the begining
		__dirreset (dirp->dirData);
		dirp->position = 0;
	}

	// Keep reading entries until we reach the one we want
	while ((dirp->position < loc) &&
		   (__dirnext (dirp->dirData, filename, NULL) >= 0))
	{
		dirp->position += 1;
	}
}


long int telldir(DIR *dirp) {
	if (!dirp) {
		return -1;
	}

	return dirp->position;
}
