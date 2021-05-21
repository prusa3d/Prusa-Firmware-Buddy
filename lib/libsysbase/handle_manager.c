#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <sys/iosupport.h>

#define MAX_HANDLES 1024

static __handle __stdin_handle  = {0, 1, NULL};
static __handle __stdout_handle = {1, 1, NULL};
static __handle __stderr_handle = {2, 1, NULL};

static __handle* handles[MAX_HANDLES] = {
	&__stdin_handle,
	&__stdout_handle,
	&__stderr_handle
};

__LOCK_INIT(static, __hndl_lock);

static void __free_handle(__handle *handle) {

	if ( NULL != handle
	&& handle != &__stdin_handle
	&& handle != &__stdout_handle
	&& handle != &__stderr_handle ) {

		free(handle);

	}

}

void __release_handle(int fd) {

	if ( fd <0 || fd >= MAX_HANDLES ) return;

	__lock_acquire (__hndl_lock);

	__free_handle(handles[fd]);
	handles[fd] = NULL;

	__lock_release (__hndl_lock);

}

int __alloc_handle(int device) {

	int i, ret = -1;

	__lock_acquire (__hndl_lock);

	for ( i = 0; i < MAX_HANDLES; i++ ) {
		if ( handles[i] == NULL ) break;
	}

	size_t size = sizeof(__handle) + devoptab_list[device]->structSize;

	if ( i < MAX_HANDLES ) {
		handles[i] = malloc(size);
		__handle *handle = handles[i];
		if ( NULL != handles[i] ) {

			handles[i]->refcount = 1;
			ret =  i;
			handles[i]->device = device;
			handles[i]->fileStruct = (void *)&handle[1];

		} else {

			errno = ENOMEM;

		}
	} else {

		errno = ENFILE;

	}

	__lock_release (__hndl_lock);

	return ret;
}

__handle *__get_handle(int fd) {

	if ( fd < 0 || fd >= MAX_HANDLES ) return NULL;

	__lock_acquire (__hndl_lock);
	__handle *handle = handles[fd];
	__lock_release (__hndl_lock);

	return handle;
}

int dup(int oldfd) {
	int i, ret =-1;

	__lock_acquire (__hndl_lock);

	if (handles[oldfd]==NULL) {
		__lock_release (__hndl_lock);
		errno = EBADF;
		return -1;
	}


	for ( i = 0; i < MAX_HANDLES; i++ ) {
		if ( handles[i] == NULL ) break;
	}

	if (i<MAX_HANDLES) {
		handles[i] = handles[oldfd];
		handles[oldfd]->refcount++;
		ret = i;
	}
	__lock_release (__hndl_lock);

	return ret;

}

int dup2(int oldfd, int newfd) {

	if ( newfd < 0 || newfd >= MAX_HANDLES ||
		 oldfd < 0 || oldfd >= MAX_HANDLES) {

		errno = EBADF;

		return -1;
	}

	if ( newfd == oldfd ) {
		return newfd;
	}

	__lock_acquire (__hndl_lock);

	if ( handles[oldfd] == NULL ) {
		__lock_release (__hndl_lock);
		errno = EBADF;

		return -1;
	}

	__handle *handle = handles[newfd];

	handles[newfd] = handles[oldfd];
	handles[newfd]->refcount++;

	if ( handle ) {

		int ref = --handle->refcount;
		__lock_release (__hndl_lock);

		if ( ref == 0 ) {

			if ( devoptab_list[handle->device]->close_r ) {

				_REENT->deviceData = devoptab_list[handle->device]->deviceData;
				devoptab_list[handle->device]->close_r(_REENT,handle->fileStruct);

			}

			__free_handle(handle);
		}

	} else {

		__lock_release (__hndl_lock);

	}

	return newfd;
}

#ifdef REENTRANT_SYSCALLS_PROVIDED
//---------------------------------------------------------------------------------
int _close_r(struct _reent *ptr, int fd) {
//---------------------------------------------------------------------------------
#else
//---------------------------------------------------------------------------------
int _close(int fd) {
//---------------------------------------------------------------------------------
	struct _reent *ptr = _REENT;
#endif
	int ret = 0;

	if (fd < 0 || fd >= MAX_HANDLES) {

		ptr->_errno = EBADF;
		return -1;

	}

	__lock_acquire (__hndl_lock);

	__handle *handle = handles[fd];

	if ( !handle ) {

		__lock_release (__hndl_lock);
		ptr->_errno = EBADF;
		return -1;

	}

	int ref = --handle->refcount;
	handles[fd] = NULL;

	__lock_release (__hndl_lock);

	if ( ref == 0 ) {

		if ( devoptab_list[handle->device]->close_r ) {

			ptr->deviceData = devoptab_list[handle->device]->deviceData;
			ret = devoptab_list[handle->device]->close_r(ptr,handle->fileStruct);

		}

		__free_handle(handle);
	}

	return ret;
}
