/*
 * JSMN is a bit weird library. It lives inside a single header. If we want to
 * use it from multiple places (in future, we'll), we mark all the uses with
 * the #define JSMN_HEADER and include it once somewhere else without the mark.
 * That'll generate the bodies.
 */
#include <jsmn.h>
