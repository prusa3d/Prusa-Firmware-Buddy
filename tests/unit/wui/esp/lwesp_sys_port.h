#include "lwesp/lwesp_opt.h"

// typedef struct {
    // lwesp_sys_sem_t sem_not_empty; [>!< Semaphore indicates not empty <]
    // lwesp_sys_sem_t sem_not_full;  [>!< Semaphore indicates not full <]
    // lwesp_sys_sem_t sem;           [>!< Semaphore to lock access <]
    // size_t in, out, size;
    // void *entries[1];
// } custom_mbox_t;

typedef struct mutex {} lwesp_sys_mutex_t;
typedef struct semaprhore {} lwesp_sys_sem_t;
typedef struct mbox {} lwesp_sys_mbox_t;
typedef struct thread {} lwesp_sys_thread_t;
typedef int lwesp_sys_thread_prio_t;

#define LWESP_SYS_MBOX_NULL   ((lwesp_sys_mbox_t)0)
#define LWESP_SYS_SEM_NULL    ((lwesp_sys_mutex_t)0)
#define LWESP_SYS_MUTEX_NULL  ((lwesp_sys_mutex_t)0)
#define LWESP_SYS_TIMEOUT     (1000)
#define LWESP_SYS_THREAD_PRIO (0)
#define LWESP_SYS_THREAD_SS   (1024)
