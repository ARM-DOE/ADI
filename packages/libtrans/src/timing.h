#ifndef _TIMING_H
#define _TIMING_H

#include <sys/time.h>
#include <sys/resource.h>

#define TIMER_LOG(function) {		\
    struct timeval start,end,diff;	\
    gettimeofday(&start,NULL);		\
    {function} ;			\
    gettimeofday(&end,NULL);		\
    timersub(&end, &start,&diff);			   \
    DEBUG_LV4("libtrans", "Block time: %f seconds\n",		   \
	      (double) diff.tv_sec + (double) diff.tv_usec/1e6);   \
  }

void call_getrusage(const char *msg);

#endif 
