#ifdef __cplusplus
extern "C" {
#endif

#ifndef PROGRESS_H
#define PROGRESS_H

/*****************************************************\
 *                     Includes                      *
\*****************************************************/
#include <unistd.h>

/*****************************************************\
 *                      Types                        *
\*****************************************************/
typedef struct {} progressBar;

/*****************************************************\
 *                Exported functions                 *
\*****************************************************/

/*
 * Creates a new progress bar instance with its own mutices, threads et c
 * <polling_time> determines how how long to wait between printout checks.
 * <width> is how many characters wide the printout will be in total.
 * <numerator> is used to determine when to write numbers instead of periods.
 */
progressBar *progressBarCreate( unsigned int total, 
				useconds_t   polling_time,
				unsigned int width, 
				unsigned int numerator,
				int          suspended );

/* Clean up, free and invalidate a progress bar */
void progressBarDestroy( progressBar *pb );

/* Resets the progress bar */
int progressBarReset( progressBar *pb );

/* Increases progress with <prog> points */
int progressBarInc( progressBar *pb, unsigned int prog );
/* Maxes the progress bar out, useful when aborting or similar */
int progressBarSetComplete( progressBar *pb );
/* Returns 1 if progress has reached 100 %, 0 if not */
int progressBarComplete( progressBar *pb );
/* Suspends and resumes printout of progress bar */
void progressBarSuspend( progressBar *pb );
void progressBarResume ( progressBar *pb );
/*
 * Prints a new progressBar, useful if something else has sent text to stdout
 *  and disrupted the original bar.
 */
void progressBarSoFar( progressBar *pb );

/* Return number of points registered so far, or -1 on failure */
int progressBarGetCurrent( progressBar *pb );

#ifdef __cplusplus
}
#endif

#endif /* PROGRESS_H */
