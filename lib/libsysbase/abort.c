#include <stdlib.h>
#include <unistd.h>

void abort(void) {
  write (2, "Abort called.\n", sizeof("Abort called.\n")-1);
  _exit (1);
}

