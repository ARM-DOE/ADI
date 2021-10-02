/*******************************************************************************
*
*  Copyright © 2014, Battelle Memorial Institute
*  All rights reserved.
*
********************************************************************************
*
*  Author:
*     name:  Brian Ermold
*     phone: (509) 375-2277
*     email: brian.ermold@pnl.gov
*
*******************************************************************************/

/** @file dsproc_exec.c
 *  Wrappers To exec functions.
 */

#define __USE_GNU
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "dsproc3.h"

/** @privatesection */

/** @publicsection */

/**
 *  Wrapper to the execvp function.
 *
 *  This function is a wrapper to the execvp function that
 *  redirects stdout and stderr to the process log file, and checks
 *  exit value of the spawned process for errors.  If an error occurs
 *  it will be appended to the log and error mail messages, and the
 *  process status will be set appropriately.
 *
 *  From the exec man page:
 *
 *  The execvp() function provides an array of pointers to
 *  null-terminated strings that represent the argument list available to
 *  the new program. The first argument, by convention, should point to the
 *  filename associated with the file being executed. The array of pointers
 *  *must* be terminated by a NULL pointer.
 *
 *  The execvp() function duplicates the actions of the shell in
 *  searching for an executable file if the specified filename does not contain
 *  a slash (/) character. The file is sought in the colon-separated list of
 *  directory pathnames specified in the PATH environment variable. If this
 *  variable isn't defined, the path list defaults to the current directory
 *  followed by the list of directories returned by confstr(_CS_PATH).
 *  (This confstr(3) call typically returns the value "/bin:/usr/bin".)
 *
 *  The execvp() function takes the environment for the new process
 *  image from the external variable environ in the calling process.
 *
 *  @param  file  - The name or path to the file that is to be executed.
 *
 *  @param  argv  - A NULL terminated argument list. By convention the first
 *                  argument should point to the name of the file being executed.
 *
 *  @param  flags - reserved for control flags
 *
 *  @retval  exit_value  exit value of the process, this is typically
 *                       0 for success and non-zero for errors.
 *  @retval  -1          if the process could not be executed
 */
int dsproc_execvp(
    const char *file,
    char *const argv[],
    int         flags)
{
    LogFile *log    = msngr_get_log_file();
    FILE    *log_fp = log->fp;
    char    *command;
    size_t   length;
    int      i;

    pid_t    pid;
    int      status;
    char     status_string[128];
    int      exit_value;
    int      signal_number;
    int      core_dumped;

    /************************************************************
    *  Create log message
    *************************************************************/

    length = strlen(file) + 1;
    for (i = 1; argv[i]; ++i) {
        length += strlen(argv[i]) + 1;
    }

    command = calloc(length, sizeof(char));
    if (!command) {

        ERROR( DSPROC_LIB_NAME,
            "Could not execute: %s\n"
            " -> memory allocation error creating command string\n",
            file);

        dsproc_set_status(DSPROC_ENOMEM);
        return(-1);
    }

    strcpy(command, file);

    for (i = 1; argv[i]; ++i) {
        strcat(command, " ");
        strcat(command, argv[i]);
    }

    LOG( DSPROC_LIB_NAME,
        "Executing:  %s",
        command);

    free(command);

    /************************************************************
    *  Fork off the new process
    *************************************************************/

    pid = fork();

    if (pid == (pid_t)-1) {

        ERROR( DSPROC_LIB_NAME,
            "Could not create fork for process: %s\n"
            " -> %s\n",
            file,
            strerror(errno));

        dsproc_set_status(DSPROC_EFORK);
        return(-1);
    }

    /************************************************************
    *  Child Process
    *************************************************************/

    if (pid == 0) {

        /* Redirect stdout and stderr to the log file */

        dup2(fileno(log_fp), fileno(stdout));
        dup2(fileno(log_fp), fileno(stderr));

        /* Execute the new process */

        execvp(file, argv);

        ERROR( DSPROC_LIB_NAME,
            "Could not execute process: %s\n"
            " -> %s\n",
            file,
            strerror(errno));

        exit(255);
    }

    /************************************************************
    *  Parent Process
    *************************************************************/

    waitpid(pid, &status, 0);

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s Exit Status: %d",
        file, status);

    if (status) {

        exit_value    = status >> 8;  /* exit value is in upper 8 bits    */
        signal_number = status & 127; /* signal number is in lower 7 bits */
        core_dumped   = status & 128; /* bit 8 is set if core dumped      */

        if (core_dumped) {
            snprintf(status_string, 128,
                "Process Core Dumped With Signal %d: %s",
                signal_number, strsignal(signal_number));
        }
        else if (signal_number) {
            snprintf(status_string, 128,
                "Process Exited With Signal %d: %s",
                signal_number, strsignal(signal_number));
        }
        else if (exit_value == 255) {
            snprintf(status_string, 128,
                "Could Not Execute Process");
        }
        else {
            return(exit_value);
        }

        ERROR( DSPROC_LIB_NAME,
            "%s\n", status_string);

        dsproc_set_status(status_string);

        return(-1);
    }

    return(0);
}

/**
 *  Run DQ Inspector.
 *
 *  This function will run dq_inspector for the specified datastream and
 *  time range.  The following arguments will be automatically added to
 *  the dq_inspector command line so they do not need to be specified by
 *  the calling process in the args[] array:
 *
 *      -P
 *      -r read_path
 *      -d datastream
 *      -s start_date
 *      -e end_date
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  A warning message will also be generated if dq_inpsector returns a
 *  non-zero exit value;
 *
 *  \b Example quicklook_hook function:
 *  \code
 *
 *  int adi_test3_quicklook_hook(
 *          void   *user_data,
 *          time_t  begin_time,
 *          time_t  end_time)
 *  {
 *      const char  *dsc_name  = "aditest3met";
 *      const char  *dsc_level = "c1";
 *      int          dsid;
 *      const char  *args[32];
 *      int          exit_value;
 *      int          ai;
 *
 *      // prevent "unused argument" compiler warning
 *      user_data = user_data;
 *  
 *      dsid = dsproc_get_output_datastream_id(dsc_name, dsc_level);
 *      if (dsid < 0) return(-1);
 *  
 *      // only create plot for rh variable
 *      ai = 0;
 *      args[ai++] = "-v";
 *      args[ai++] = "rh";
 *      args[ai++] = NULL;
 *  
 *      exit_value = dsproc_run_dq_inspector(
 *          dsid, begin_time, end_time, args, 0);
 *  
 *      if (exit_value < 0) return(-1);
 *      if (exit_value > 0) return(0);
 *  
 *      return(1);
 *  }
 *
 *  \endcode
 *
 *  @param  dsid       - Id of the output datastream.
 *  @param  begin_time - Begin time of plot interval
 *  @param  end_time   - End time of plot interval
 *
 *  @param  args       - Command line argument list to send to dq_instector.
 *                       This list must be NULL terminated.
 *
 *  @param  flags      - Reserved for control flags. Pass in 0 for this
 *                       argument to maintain backward compatibility with
 *                       future updates.
 *
 *  @retval  exit_value  dq_inspector exit value (0 == success).
 *  @retval  -1          if the process could not be executed
 */
int dsproc_run_dq_inspector(
    int         dsid,
    time_t      begin_time,
    time_t      end_time,
    const char *args[],
    int         flags)
{
    const char  *datastream;
    const char  *read_path;
    char         start_date[32];
    char         end_date[32];
    const char  *command;
    const char  *argv[128];

    int         exit_value;
    int         i, j;

    command     = "dq_inspector";
    datastream  = dsproc_datastream_name(dsid);
    read_path   = getenv("DATASTREAM_DATA");

    dsproc_create_timestamp(begin_time, start_date);
    dsproc_create_timestamp(end_time,   end_date);

    i = 0;
    argv[i++] = command;
    argv[i++] = "-P";
    argv[i++] = "-r";
    argv[i++] = read_path;
    argv[i++] = "-d";
    argv[i++] = datastream;
    argv[i++] = "-s";
    argv[i++] = start_date;
    argv[i++] = "-e";
    argv[i++] = end_date;

    for (j = 0; args[j]; ++j) {
        argv[i++] = args[j];
    }

    argv[i++] = NULL;
    
    exit_value = dsproc_execvp(command, (char * const*)argv, 0);
    
    if (exit_value > 0) {
        WARNING( DSPROC_LIB_NAME,
            "%s exited with non-zero value: %d\n",
            command, exit_value);
    }

    return(exit_value);
}
