#include <pthread.h>

#ifndef JOBHANDLER_H
#define JOBHANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  JH_LINEAR,
  JH_RANDOM,
  JH_PRIORITY,
} jobHandlerType;

typedef int (*jobHandlerPrio) (void*);

typedef struct {
  jobHandlerType  type;
  unsigned int    length;   /* Maximum number of job elements */
  unsigned int    used;     /* Number of jobs currently in use */
  unsigned int    elemSize; /* Size of each element */
  jobHandlerPrio  prioFunc; /* Optional priority function */

  unsigned int    firstJob; /* These variables are used to */
  unsigned int    lastJob;  /*  handle the queue */
  pthread_mutex_t mutex;

  void           *jobs;
} jobHandler;



/**
 *
 * Creates a new job handler capable of holding \c length
 *  elements, each of size \c size.
 *
 * \c type determines how the job handler retrieves jobs.
 *
 *
 * \param  type     IN  A type descriptor for the job handler to
 *                      create.  Makes the handler return jobs
 *                      either in the order they were entered 
 *                      (JH_LINEAR), randomly (JH_RANDOM) or
 *                      according to a priority function.
 *
 * \param  length   IN  Number of elements the handler can hold.
 *
 * \param  elemSize IN  Size of each element.
 *
 * \retval  Pointer to a new jobhandler if successful, NULL on failure.
 *
 */
jobHandler *jobHandlerCreate( jobHandlerType type, 
							  unsigned int   length, 
							  unsigned int   elemSize,
							  jobHandlerPrio prioFunc );

/**
 *
 *  Destroys a job handler and clears the memory allocated 
 *   in its construction.
 *
 * \param  jh  IN  Pointer to the relevant jobhandler.
 *
 */
void jobHandlerDestroy( jobHandler *jh );

/**
 *
 * Asks job handler to retrieve a job description.  The descriptor
 *  will be unlinked from the job queue.  Function call may block if
 *  necessary.
 *
 * The pointer must be freed by the caller.
 *
 * \param  jh  IN  Pointer to the the relevant jobhandler.
 *
 * \retval  Pointer to a job descriptor unless job queue is empty.
 *
 */
void *jobHandlerGetJob( jobHandler *jh );

/**
 *
 * Adds a job to the end of the job list.
 *
 * \param  jh  IN  Pointer to the the relevant jobhandler.
 *
 * \param  job IN  Pointer to a job description to be copied 
 *                 into job list.  The memory pointed to by 
 *                 job can be freed after the call has returned.
 *
 * \retval  0 on success, -1 on failure
 *
 */
int jobHandlerAddJob( jobHandler *jh, void *job );

#ifdef __cplusplus
}
#endif

#endif
