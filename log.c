#include "log.h"
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static int global_loglevel = LOG_INFO;

void logline(int loglevel, const char *format, ...) //ellipsis used to allow the user to pass more than 1 arguement
 {
    if (loglevel > global_loglevel) //wont continue if loglevel = 3 (debug) while global_loglevel is at 2 (info)
	 return;

    char timestr[20]; 
    time_t lt = time(NULL);
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&lt)); //formats an unformatted number to date form yyyy-mm-dd and time as HH:MM:SS

    const char *tag; //we give a tag to the loglevels that are passed
    switch (loglevel) {
        case LOG_ERROR: tag = "E"; break;
        case LOG_DEBUG: tag = "D"; break;
        default:        tag = "I"; break;
    }

   
    va_list args; //va used becaus of ellipsis '...' va_list reads all arguements from stack the user has passed. 
    va_start(args, format);  //starts reading after the variable format
	 
    pthread_mutex_lock(&log_mutex);
	 
	printf("[%s %s] ", timestr, tag); 
	vprintf(format, args);
    printf("\n");
    fflush(stdout); //displays text on screen and removes from computers memory
	 
	pthread_mutex_unlock(&log_mutex);
	 
	va_end(args);
}

void set_loglevel(int loglevel) {
    if (loglevel >= LOG_ERROR && loglevel <= LOG_DEBUG) //checks if valid loglevel has been entered
        global_loglevel = loglevel;   //updating the global_loglevel allows us to limit unnecessary logs
}
