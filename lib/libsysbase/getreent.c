
/* default reentrant pointer when multithread enabled */

#include <_ansi.h>
#include <reent.h>
#include <sys/iosupport.h>

#ifdef __getreent
#undef __getreent
#endif

struct _reent *__getreent() {
	if ( __syscalls.getreent ) {
		return __syscalls.getreent();
	} else {
		return _impure_ptr;
	}
}


