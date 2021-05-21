#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*
 * The DIRSIZ macro gives the minimum record length which will hold
 * the directory entry.  This requires the amount of space in struct dirent
 * without the d_name field, plus enough space for the name with a terminating
 * null byte (dp->d_namlen+1), rounded up to a 4 byte boundary.
 */
#undef DIRSIZ
#ifdef _DIRENT_HAVE_D_NAMLEN
#define DIRSIZ(dp) \
    (offsetof (struct dirent, d_name) + (((dp)->d_namlen+1 + 3) &~ 3))
#else
#define DIRSIZ(dp) \
    (offsetof (struct dirent, d_name) + ((strlen((dp)->d_name)+1 + 3) &~ 3))
#endif

int
scandir (const char *dirname,
	struct dirent ***namelist,
	int (*filter) __P((const struct dirent *)),
	int (*compar) __P((const struct dirent **, const struct dirent **)))
{
	DIR *d = opendir(dirname);

	if (!d) return -1;

	struct dirent *de, **names = NULL, **tmp;
	size_t cnt = 0, len = 0;

	while (de = readdir(d)) {
		if (filter && ! filter(de)) continue;
		if (cnt >= len) {
			len = 2*len+1;
			if (len > SIZE_MAX/sizeof(*names)) break;
			tmp = realloc(names, len * sizeof(*names));
			if (!tmp) break;
			names = tmp;
		}
		names[cnt] = malloc(DIRSIZ(de));
		if (!names[cnt]) break;
		memcpy(names[cnt++], de, DIRSIZ(de));
	}

	closedir(d);

	if(errno) {
		if (names) while(cnt-- > 0) free(names[cnt]);
		free(names);
		return -1;
	}

	if (compar) qsort(names, cnt, sizeof(*names), (int (*)(const void *, const void *))compar);

	*namelist = names;
	return cnt;
}

/*
 * Alphabetic order comparison routine for those who want it.
 */
int
alphasort (const struct dirent **d1,
       const struct dirent **d2)
{
       return(strcmp((*d1)->d_name, (*d2)->d_name));
}

