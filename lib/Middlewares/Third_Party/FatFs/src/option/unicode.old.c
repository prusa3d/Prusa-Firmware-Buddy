#include "../ff.h"

#if FF_USE_LFN != 0

#if   FF_CODE_PAGE == 932	/* Japanese Shift_JIS */
#include "cc932.c"
#elif FF_CODE_PAGE == 936	/* Simplified Chinese GBK */
#include "cc936.c"
#elif FF_CODE_PAGE == 949	/* Korean */
#include "cc949.c"
#elif FF_CODE_PAGE == 950	/* Traditional Chinese Big5 */
#include "cc950.c"
#else					/* Single Byte Character-Set */
#include "ccsbcs.c"
#endif

#endif
