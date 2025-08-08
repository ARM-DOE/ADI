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

/** @file dsproc.c
 *  Data System Process Library Functions.
 */

#include <signal.h>
#include <sys/resource.h>

#ifndef __GNUC__
#include <ucontext.h>
#endif

#include "dsproc3.h"
#include "dsproc_private.h"
#include "trans.h"

#define COREDUMPSIZE 12000000 /**< Maximum size of core files */

extern int _InsideFinishProcessHook;

/** @privatesection */

/*******************************************************************************
 *  Private Data
 */

DSProc *_DSProc = (DSProc *)NULL; /**< DSProc structure */
int     _DisableDBUpdates = 0;    /**< flag used to disable database updates */
int     _DisableLockFile  = 0;    /**< flag used to disable the lock file    */
int     _DisableMail      = 0;    /**< flag used to disable mail messages    */

static char *_LogsRoot = (char *)NULL; /**< root path to the logs directory  */
static char *_LogsDir  = (char *)NULL; /**< full path to the logs directory  */
static char *_LogFile  = (char *)NULL; /**< name of the log file             */
static char *_LogID    = (char *)NULL; /**< replace time with ID in log file */
static int   _Reprocessing = 0;        /**< reprocessing mode flag           */
static int   _DynamicDODs  = 0;        /**< dynamic DODs mode flag           */
static int   _Force        = 0;        /**< force process past non-fatal errors */
static int   _LogInterval  = 0;        /**< log file interval                */
static int   _LogDataTime  = 0;        /**< use data time for log time       */

static char  _InputDir[PATH_MAX];      /**< input dir from ingest file loop  */
static char  _InputFile[PATH_MAX];     /**< input file from ingest file loop */
static char  _InputSource[PATH_MAX*2]; /**< full path to input file          */

static int   _RealTimeMode = 0;        /**< real time mode flag              */

static int   _MaxRunTime   = -1;       /**< max runtime of process           */

static int   _AsynchronousMode =  0;   /**< allow asynchronous processing    */

/** maximum wait time for input data when running in real-time mode */
static time_t _MaxRealTimeWait = 3 * 86400;

/*******************************************************************************
 *  Static Functions Visible Only To This Module
 */

static char *_get_logs_root()
{
    int status;

    if (!_LogsRoot) {

        status = dsenv_get_logs_root(&_LogsRoot);

        if (status < 0) {
            dsproc_set_status(DSPROC_ENOMEM);
            return((char *)NULL);
        }
        else if (status == 0) {

            ERROR( DSPROC_LIB_NAME,
                "Could not get path to logs directory\n"
                " -> the LOGS_DATA environment variable was not found\n");

            dsproc_set_status(DSPROC_ELOGSPATH);
            return((char *)NULL);
        }
    }

    return(_LogsRoot);
}

static int _lock_process(
    const char *site,
    const char *facility,
    const char *proc_name,
    const char *proc_type)
{
    char *lockfile_root;
    int   status;
    char  errstr[MAX_LOCKFILE_ERROR];

    if (_DisableLockFile) {
        return(1);
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Creating process lockfile:\n");

    /* Determine path to the lockfiles directory */

    if (_LogsDir) {
        _DSProc->lockfile_path = strdup(_LogsDir);
    }
    else {

        /* Put lockfiles under the the logs root directory */

        lockfile_root = _get_logs_root();
        if (!lockfile_root) {
            return(0);
        }

        /* Create lockfile path string */

        _DSProc->lockfile_path = msngr_create_string(
            "%s/%s/lockfiles",
            lockfile_root, site);
    }

    if (!_DSProc->lockfile_path) {

        ERROR( DSPROC_LIB_NAME,
            "%s%s-%s-%s: Could not create lockfile\n"
            " -> memory allocation error\n",
            site, facility, proc_name, proc_type);

        return(0);
    }

    /* Create the lockfile name */

    _DSProc->lockfile_name = msngr_create_string("%s%s-%s-%s.lock",
        site, facility, proc_name, proc_type);

    if (!_DSProc->lockfile_name) {

        ERROR( DSPROC_LIB_NAME,
            "%s%s-%s-%s: Could not create lockfile\n"
            " -> memory allocation error\n",
            site, facility, proc_name, proc_type);

        free(_DSProc->lockfile_path);

        _DSProc->lockfile_path = (char *)NULL;

        return(0);
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        " - path: %s\n"
        " - name: %s\n",
        _DSProc->lockfile_path, _DSProc->lockfile_name);

    status = lockfile_create(
        _DSProc->lockfile_path, _DSProc->lockfile_name, 0,
        MAX_LOCKFILE_ERROR, errstr);

    if (status <= 0) {

        ERROR( DSPROC_LIB_NAME,
            "%s%s-%s-%s: %s\n",
            site, facility, proc_name, proc_type, errstr);

        free(_DSProc->lockfile_path);
        free(_DSProc->lockfile_name);

        _DSProc->lockfile_path = (char *)NULL;
        _DSProc->lockfile_name = (char *)NULL;

        return(0);
    }

    if (status == 2) {

        WARNING( DSPROC_LIB_NAME,
            "%s%s-%s-%s: Removed stale lockfile\n"
            " -> %s/%s\n",
            site, facility, proc_name, proc_type,
            _DSProc->lockfile_path, _DSProc->lockfile_name);
    }

    return(1);
}

static void _unlock_process(void)
{
    int   status;
    char  errstr[MAX_LOCKFILE_ERROR];

    if (_DisableLockFile) {
        return;
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Removing process lockfile:\n"
        " - path: %s\n"
        " - name: %s\n",
        _DSProc->lockfile_path, _DSProc->lockfile_name);

    status = lockfile_remove(
        _DSProc->lockfile_path, _DSProc->lockfile_name,
        MAX_LOCKFILE_ERROR, errstr);

    if (status < 0) {

        ERROR( DSPROC_LIB_NAME,
            "%s%s-%s-%s: %s\n",
            _DSProc->site, _DSProc->facility,
            _DSProc->name, _DSProc->type, errstr);
    }

    free(_DSProc->lockfile_path);
    free(_DSProc->lockfile_name);

    _DSProc->lockfile_path = (char *)NULL;
    _DSProc->lockfile_name = (char *)NULL;
}

static int _init_process_log(
    const char *site,
    const char *facility,
    const char *proc_name,
    const char *proc_type)
{
    char       *logs_root;
    char       *log_path;
    char       *log_name;
    time_t      log_time;
    struct tm   gmt;
    int         status;
    char        errstr[MAX_LOG_ERROR];

    memset(&gmt, 0, sizeof(struct tm));

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Opening process log file:\n");

    /* Determine path to the log files directory */

    if (_LogsDir) {
        log_path = strdup(_LogsDir);
    }
    else {

        /* Get the root logs directory */

        logs_root = _get_logs_root();
        if (!logs_root) {
            return(0);
        }

        /* Create log file path string */

        log_path = msngr_create_string("%s/%s/proc_logs/%s%s%s",
            logs_root, site, site, proc_name, facility);
    }

    if (!log_path) {

        ERROR( DSPROC_LIB_NAME,
            "%s%s-%s-%s: Could not open process log\n"
            " -> memory allocation error\n",
            site, facility, proc_name, proc_type);

        dsproc_set_status(DSPROC_ENOMEM);
        return(0);
    }

    if (_LogFile) {
        log_name = strdup(_LogFile);
    }
    else if (_LogID) {
        log_name = msngr_create_string("%s%s%s.%s.%s",
            site, proc_name, facility, _LogID, proc_type);
    }
    else {
        /* Determine the time to use in the log file name */

        if (_LogDataTime && _DSProc->cmd_line_begin) {
            log_time = _DSProc->cmd_line_begin;
        }
        else {
            log_time = time(NULL);
        }

        gmtime_r(&log_time, &gmt);

        /* Create the log file name */

        switch (_LogInterval) {
            case LOG_DAILY:
                log_name = msngr_create_string("%s%s%s.%04d%02d%02d.000000.%s",
                    site, proc_name, facility,
                    (gmt.tm_year + 1900), (gmt.tm_mon + 1), gmt.tm_mday, proc_type);
                break;
            case LOG_RUN:
                log_name = msngr_create_string("%s%s%s.%04d%02d%02d.%02d%02d%02d.%s",
                    site, proc_name, facility,
                    (gmt.tm_year + 1900), (gmt.tm_mon + 1), gmt.tm_mday,
                    gmt.tm_hour, gmt.tm_min, gmt.tm_sec, proc_type);
                break;
            default: /* LOG_MONTHLY */
                log_name = msngr_create_string("%s%s%s.%04d%02d00.000000.%s",
                    site, proc_name, facility,
                    (gmt.tm_year + 1900), (gmt.tm_mon + 1), proc_type);
        }
    }

    if (!log_name) {

        ERROR( DSPROC_LIB_NAME,
            "%s%s-%s-%s: Could not open process log\n"
            " -> memory allocation error\n",
            site, facility, proc_name, proc_type);

        dsproc_set_status(DSPROC_ENOMEM);
        free(log_path);
        return(0);
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        " - path: %s\n"
        " - name: %s\n",
        log_path, log_name);

    status = msngr_init_log(
        log_path, log_name, (LOG_TAGS | LOG_STATS), MAX_LOG_ERROR, errstr);

    free(log_path);
    free(log_name);

    if (status == 0) {

        ERROR( DSPROC_LIB_NAME,
            "%s%s-%s-%s: Could not open process log\n"
            " -> %s\n",
            site, facility, proc_name, proc_type, errstr);

        dsproc_set_status(DSPROC_ELOGOPEN);
        return(0);
    }

    return(1);
}

static int _init_provenance_log(
    const char *site,
    const char *facility,
    const char *proc_name,
    const char *proc_type)
{
    char       *logs_root;
    char       *log_path;
    char       *log_name;
    time_t      log_time;
    struct tm   gmt;
    int         status;
    char        errstr[MAX_LOG_ERROR];

    memset(&gmt, 0, sizeof(struct tm));

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Opening provenance log for: %s%s-%s-%s\n",
        site, facility, proc_name, proc_type);

    /* Determine path to the log files directory */

    if (_LogsDir) {
        log_path = strdup(_LogsDir);
    }
    else {

        /* Get the root logs directory */

        logs_root = _get_logs_root();
        if (!logs_root) {
            return(0);
        }

        /* Create log file path string */

        log_path = msngr_create_string("%s/%s/provenance/%s%s%s",
            logs_root, site, site, proc_name, facility);
    }

    if (!log_path) {

        ERROR( DSPROC_LIB_NAME,
            "%s%s-%s-%s: Could not open provenance log\n"
            " -> memory allocation error\n",
            site, facility, proc_name, proc_type);

        dsproc_set_status(DSPROC_ENOMEM);
        return(0);
    }

    if (_LogFile) {
        log_name = msngr_create_string("%s.Provenance", _LogFile);
    }
    else if (_LogID) {
        log_name = msngr_create_string("%s%s%s.%s.%s.Provenance",
            site, proc_name, facility, _LogID, proc_type);
    }
    else {
        /* Determine the time to use in the log file name */

        if (_LogDataTime && _DSProc->cmd_line_begin) {
            log_time = _DSProc->cmd_line_begin;
        }
        else {
            log_time = time(NULL);
        }

        gmtime_r(&log_time, &gmt);

        /* Create the log file name */

        log_name = msngr_create_string("%s%s%s.%04d%02d%02d.%02d%02d%02d.%s.Provenance",
            site, proc_name, facility,
            (gmt.tm_year + 1900), (gmt.tm_mon + 1), gmt.tm_mday,
            gmt.tm_hour, gmt.tm_min, gmt.tm_sec, proc_type);

    }

    if (!log_name) {

        ERROR( DSPROC_LIB_NAME,
            "%s%s-%s-%s: Could not open provenance log\n"
            " -> memory allocation error\n",
            site, facility, proc_name, proc_type);

        dsproc_set_status(DSPROC_ENOMEM);
        free(log_path);
        return(0);
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        " - path: %s\n"
        " - name: %s\n",
        log_path, log_name);

    status = msngr_init_provenance(
        log_path, log_name, (LOG_TAGS | LOG_STATS), MAX_LOG_ERROR, errstr);

    free(log_path);
    free(log_name);

    if (status == 0) {

        ERROR( DSPROC_LIB_NAME,
            "%s%s-%s-%s: Could not open provenance log\n"
            " -> %s\n",
            site, facility, proc_name, proc_type, errstr);

        dsproc_set_status(DSPROC_EPROVOPEN);
        return(0);
    }

    return(1);
}

static int _init_mail(
    MessageType  mail_type,
    char        *mail_from,
    char        *mail_subject,
    const char  *config_key)
{
    int        status;
    ProcConf **proc_conf;
    char      *mail_to;
    size_t     mail_to_length;
    char       errstr[MAX_MAIL_ERROR];
    int        i;

    if (_DisableMail) {
        return(1);
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Checking database for '%s' custodians\n", config_key);

    /* Get the process configuration for this key */

    status = dsdb_get_process_config_values(_DSProc->dsdb,
        _DSProc->site, _DSProc->facility, _DSProc->type, _DSProc->name,
        config_key, &proc_conf);

    if (status == 1) {

        /* Get the length of the mail_to string */

        mail_to_length = 0;

        for (i = 0; proc_conf[i]; i++) {
            mail_to_length += strlen(proc_conf[i]->value) + 2;
        }

        /* Allocate memory for the mail_to string */

        mail_to = (char *)calloc(mail_to_length, sizeof(char));
        if (!mail_to) {

            ERROR( DSPROC_LIB_NAME,
                "Could not initialize mail message for: %s\n"
                " -> memory allocation error\n", config_key);

            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }

        /* Create the mail_to string */

        for (i = 0; proc_conf[i]; i++) {

            DEBUG_LV1( DSPROC_LIB_NAME, " - %s\n", proc_conf[i]->value);

            if (i) strcat(mail_to, ",");

            strcat(mail_to, proc_conf[i]->value);
        }

        /* Initialize the mail message */

        status = msngr_init_mail(
            mail_type,
            mail_from,
            mail_to,
            NULL,
            mail_subject,
            MAIL_ADD_NEWLINE,
            MAX_MAIL_ERROR,
            errstr);

        /* Cleanup and exit */

        dsdb_free_process_config_values(proc_conf);
        free(mail_to);

        if (status == 0) {

            if (strstr(errstr, "Could not find sendmail")) {

                LOG( DSPROC_LIB_NAME,
                    "Disabling mail messages: %s",
                    errstr);

                _DisableMail = 1;
            }
            else {
                ERROR( DSPROC_LIB_NAME,
                    "Could not initialize mail message for: %s\n"
                    " -> %s\n", config_key, errstr);

                dsproc_set_status(DSPROC_EMAILINIT);
                return(0);
            }
        }
    }
    else if (status < 0) {
        dsproc_set_status(DSPROC_EDBERROR);
        return(0);
    }
    else {
        DEBUG_LV1( DSPROC_LIB_NAME, " - none found\n");
    }

    return(1);
}

static void _finish_mail(
    MessageType  mail_type,
    int          mail_error_status,
    char        *status_message,
    char        *last_status_text,
    time_t       last_completed,
    time_t       last_successful,
    char        *finish_time_string)
{
    Mail *mail;
    char  last_completed_string[32];
    char  last_successful_string[32];

    if (_DisableMail) {
        return;
    }

    mail = msngr_get_mail(mail_type);
    if (!mail) {
        return;
    }

    if (mail->body[0] != '\0' || mail_error_status) {

        if (last_status_text) {
            format_secs1970(last_completed, last_completed_string);
            format_secs1970(last_successful, last_successful_string);
        }

        if (status_message && last_status_text) {
            mail_printf(mail,
                "%s\n"
                "Last Status:     %s\n"
                "Last Completed:  %s\n"
                "Last Successful: %s\n",
                status_message,
                last_status_text,
                last_completed_string,
                last_successful_string);
        }
        else if (status_message) {
            mail_printf(mail,
                "%s\n"
                "No Previous Status Has Been Recorded\n",
                status_message);
        }
        else if (last_status_text) {
            mail_printf(mail,
                "Current Status: %s\n"
                "Status: Memory allocation error creating status message\n"
                "\n"
                "Last Status:     %s\n"
                "Last Completed:  %s\n"
                "Last Successful: %s\n",
                finish_time_string,
                last_status_text,
                last_completed_string,
                last_successful_string);
        }
        else {
            mail_printf(mail,
                "Current Status: %s\n"
                "Status: Memory allocation error creating status message\n"
                "\n"
                "No Previous Status Has Been Recorded\n",
                finish_time_string);
        }
    }
}

static void _signal_handler(int sig, siginfo_t *si, void *uc)
{
    int         rename = 1;
    const char *status;
    int         exit_value;

    switch (sig) {
        case SIGQUIT:
            status = "SIGQUIT: Quit (see termio(7I))";
            rename = 0;
            break;
        case SIGILL:
            status = "SIGILL: Illegal Instruction";
            break;
        case SIGTRAP:
            status = "SIGTRAP: Trace or Breakpoint Trap";
            rename = 0;
            break;
        case SIGABRT:
            status = "SIGABRT: Abort";
            break;
#ifndef __GNUC__
        case SIGEMT:
            status = "SIGEMT: Emulation Trap";
            break;
#endif
        case SIGFPE:
            status = "SIGFPE: Arithmetic Exception";
            break;
        case SIGBUS:
            status = "SIGBUS: Bus Error";
            break;
        case SIGSEGV:
            status = "SIGSEGV: Segmentation Fault";
            break;
        case SIGSYS:
            status = "SIGSYS: Bad System Call";
            break;
        case SIGHUP:
            status = "SIGHUP: Hangup (see termio(7I))";
            rename = 0;
            break;
        case SIGINT:
            status = "SIGINT: Interrupt (see termio(7I))";
            rename = 0;
            break;
        case SIGPIPE:
            status = "SIGPIPE: Broken Pipe";
            rename = 0;
            break;
        case SIGALRM:
            status = "SIGALRM: Alarm Clock";
            rename = 0;
            break;
        case SIGTERM:
            status = "SIGTERM: Terminated";
            rename = 0;
            break;
        default:
            status = "Trapped Unknown Signal Type";
    }

    ERROR( DSPROC_LIB_NAME,
        "Received Signal: %s\n", status);

    dsproc_set_status(status);

#ifndef __GNUC__
    if (msngr_debug_level) {
        printstack(fileno(stdout));
    }
#endif

    /* If this is an ingest and the force option is enabled
     * we need to try to move the file that caused the problem. */

    if (rename  &&
        _DSProc &&
        _DSProc->model == PM_INGEST &&
        dsproc_get_force_mode()) {

        const char *input_dir  = dsproc_get_input_dir();
        const char *input_file = dsproc_get_input_file();

        if (input_dir && input_file) {
            dsproc_force_rename_bad(input_dir, input_file);
        }
    }

    /* Cleanup and exit the process */

    _dsproc_run_finish_process_hook();

    exit_value = dsproc_finish();
    exit(exit_value);
}

static int _init_signal_handlers(void)
{
    struct sigaction act;
#ifndef __GNUC__
    struct rlimit    rl;
#endif

    memset(&act, 0, sizeof(act));

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Initializing signal handlers\n");

    /*  act.sa_mask = sigset_t(0); */
    act.sa_handler   = 0;
    act.sa_flags     = (SA_SIGINFO);
    act.sa_sigaction = _signal_handler;

    if (sigaction(SIGHUP,  &act, 0) != 0 ||   /* Hangup (see termio(7I))    */
        sigaction(SIGINT,  &act, 0) != 0 ||   /* Interrupt (see termio(7I)) */
        sigaction(SIGQUIT, &act, 0) != 0 ||   /* Quit (see termio(7I))      */
        sigaction(SIGILL,  &act, 0) != 0 ||   /* Illegal Instruction        */
        sigaction(SIGTRAP, &act, 0) != 0 ||   /* Trace or Breakpoint Trap   */
        sigaction(SIGABRT, &act, 0) != 0 ||   /* Abort                      */
#ifndef __GNUC__
        sigaction(SIGEMT,  &act, 0) != 0 ||   /* Emulation Trap             */
#endif
        sigaction(SIGFPE,  &act, 0) != 0 ||   /* Arithmetic Exception       */
        sigaction(SIGBUS,  &act, 0) != 0 ||   /* Bus Error                  */
        sigaction(SIGSEGV, &act, 0) != 0 ||   /* Segmentation Fault         */
        sigaction(SIGSYS,  &act, 0) != 0 ||   /* Bad System Call            */
        sigaction(SIGPIPE, &act, 0) != 0 ||   /* Broken Pipe                */
        sigaction(SIGALRM, &act, 0) != 0 ||   /* Alarm Clock                */
        sigaction(SIGTERM, &act, 0) != 0) {   /* Terminated                 */

        ERROR( DSPROC_LIB_NAME,
            "Could not initialize signal handlers:\n"
            " -> %s\n", strerror(errno));

        dsproc_set_status(DSPROC_EINITSIGS);
        return(0);
    }

#ifndef __GNUC__
    /* Limit the core file size */

    rl.rlim_cur = COREDUMPSIZE;
    rl.rlim_max = COREDUMPSIZE;

    if (setrlimit(RLIMIT_CORE, &rl) == -1) {

        ERROR( DSPROC_LIB_NAME,
            "Could not set core file size limit:\n"
            " -> %s\n", strerror(errno));

        dsproc_set_status(DSPROC_EINITSIGS);
        return(0);
    }
#endif

    return(1);
}

/**
 *  Static: Initialize a data system process.
 *
 *  This function will:
 *
 *    - Initialize the mail messages
 *    - Update the process start time in the database
 *    - Initialize the signal handlers
 *    - Define non-standard unit symbols
 *    - Get process configuration information from database
 *
 *  @return
 *    - 1 if succesful
 *    - 0 if an error occurred
 */
static int _dsproc_init(void)
{
    const char *site       = _DSProc->site;
    const char *facility   = _DSProc->facility;
    const char *proc_name  = _DSProc->name;
    const char *proc_type  = _DSProc->type;
    time_t      start_time = _DSProc->start_time;
    ProcLoc    *proc_loc;
    const char *site_desc;
    char        mail_from[64];
    char        mail_subject[128];
    char       *config_value;
    int         status;

    /************************************************************
    *  Initialize mail messages
    *************************************************************/

    if (!_DisableMail) {

        snprintf(mail_from, 64, "%s%s%s", site, proc_name, facility);

        /* Error Mail */

        snprintf(mail_subject, 128,
            "%s Error: %s%s.%s ", proc_type, site, facility, proc_name);

        if (!_init_mail(MSNGR_ERROR, mail_from, mail_subject, "error_mail")) {
            return(0);
        }

        /* Warning Mail */

        snprintf(mail_subject, 128,
            "%s Warning: %s%s.%s ", proc_type, site, facility, proc_name);

        if (!_init_mail(MSNGR_WARNING, mail_from, mail_subject, "warning_mail")) {
            return(0);
        }

        /* Mentor Mail */

        snprintf(mail_subject, 128,
            "%s Message: %s%s.%s ", proc_type, site, facility, proc_name);

        if (!_init_mail(MSNGR_MAINTAINER, mail_from, mail_subject, "mentor_mail")) {
            return(0);
        }
    }

    /************************************************************
    *  Update process start time in the database
    *************************************************************/

    if (!_DisableDBUpdates) {

        if (msngr_debug_level || msngr_provenance_level) {

            char time_string[32];

            format_secs1970(start_time, time_string);

            DEBUG_LV1( DSPROC_LIB_NAME,
                "Updating process start time in database: %s\n", time_string);
        }

        status = dsdb_update_process_started(
            _DSProc->dsdb, site, facility, proc_type, proc_name, start_time);

        if (status <= 0) {

            if (status == 0) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not update process start time in database\n"
                    " -> unexpected NULL result from database");
            }

            dsproc_set_status(DSPROC_EDBERROR);
            return(0);
        }
    }

    /************************************************************
    *  Initialize the signal handlers
    *************************************************************/

    if (!_init_signal_handlers()) {
        return(0);
    }

    /************************************************************
    *  Map non-standard unit symbols used by ARM to standard
    *  units in the UDUNITS-2 dictionary.
    *************************************************************/

    if (!cds_map_symbol_to_unit("C",    "degree_Celsius") ||
        !cds_map_symbol_to_unit("deg",  "degree")         ||
        !cds_map_symbol_to_unit("mb",   "millibar")       ||
        !cds_map_symbol_to_unit("srad", "steradian")      ||
        !cds_map_symbol_to_unit("ster", "steradian")      ||
        !cds_map_symbol_to_unit("unitless", "1")) {

        return(0);
    }

    /************************************************************
    *  Set the standard attributes we should exclude from the
    *  dod compare.
    *************************************************************/

    if (!_dsproc_set_standard_exclude_atts()) {
        return(0);
    }

    /************************************************************
    *  Get the process location
    *************************************************************/

    status = dsproc_get_location(&proc_loc);

    if (status <= 0) {

        if (status == 0) {

            ERROR( DSPROC_LIB_NAME,
                "Could not get process location from database\n"
                " -> unexpected NULL result from database query\n");

            dsproc_set_status(DSPROC_EDBERROR);
        }

        return(0);
    }

    /************************************************************
    *  Get the site description
    *************************************************************/

    status = dsproc_get_site_description(&site_desc);

    if (status <= 0) {

        if (status == 0) {

            ERROR( DSPROC_LIB_NAME,
                "Could not get site description from database\n"
                " -> unexpected NULL result from database query\n");

            dsproc_set_status(DSPROC_EDBERROR);
        }

        return(0);
    }

    /************************************************************
    *  Get the max runtime
    *************************************************************/

    if (_MaxRunTime >= 0) {
        _DSProc->max_run_time = _MaxRunTime;
    }
    else {
        status = dsproc_get_config_value("max_run_time", &config_value);

        if (status == 1) {
            _DSProc->max_run_time = (time_t)atoi(config_value);
            free(config_value);
        }
        else if (status == 0) {
            _DSProc->max_run_time = (time_t)0;
        }
        else {
            return(0);
        }
    }

    /************************************************************
    *  Get the data expectation interval
    *************************************************************/

    status = dsproc_get_config_value("data_interval", &config_value);

    if (status == 1) {
        _DSProc->data_interval = (time_t)atoi(config_value);
        free(config_value);
    }
    else if (status == 0) {
        _DSProc->data_interval = (time_t)0;
    }
    else {
        return(0);
    }

    /************************************************************
    *  Get minimum valid data time
    *************************************************************/

    status = dsproc_get_config_value("min_valid_time", &config_value);

    if (status == 1) {
        _DSProc->min_valid_time = (time_t)atoi(config_value);
        free(config_value);
    }
    else if (status == 0) {
        /* 694224000 = 1992-01-01 00:00:00 */
        _DSProc->min_valid_time = (time_t)694224000;
    }
    else {
        return(0);
    }

    /************************************************************
    * Get the output interval(s)
    *************************************************************/

    status = dsproc_get_config_value("output_interval", &config_value);

    if (status == 1) {
        status = _dsproc_parse_output_interval_string(config_value);
        free(config_value);
        if (status == 0) {
            return(0);
        }
    }
    else if (status < 0) {
        return(0);
    }

    return(1);
}

/**
 *  Static: Read the next processing interval begin time file.
 *
 *  @return
 *    -  1 if succesful
 *    -  0 if the file doesn't exist
 *    - -1 if an error occurred
 */
static int _read_next_begin_time_file(time_t *begin_time)
{
    LogFile    *log  = msngr_get_log_file();
    const char *path = log->path;
    const char *file = ".next_begin_time";
    char        full_path[PATH_MAX];
    FILE       *fp;
    char        timestamp[64];
    int         count;
    int         year, mon, day, hour, min, sec;

    /* Check to see if the next begin time file exists */

    snprintf(full_path, PATH_MAX, "%s/%s", path, file);

    if (access(full_path, F_OK) != 0) {

        if (errno != ENOENT) {

            ERROR( DSPROC_LIB_NAME,
                "Could not access file: %s\n"
                " -> %s\n", full_path, strerror(errno));

            dsproc_set_status(DSPROC_EACCESS);
            return(-1);
        }

        return(0);
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Getting processing period begin time from file: %s\n",
        full_path);

    /* Open the timestamp file */

    fp = fopen(full_path, "r");
    if (!fp) {

        ERROR( DSPROC_LIB_NAME,
            "Could not open file: %s\n"
            " -> %s\n",
            full_path, strerror(errno));
        dsproc_set_status(DSPROC_ELOGOPEN);
        return(-1);
    }

    /* Read in the timestamp */

    if (!fgets(timestamp, 64, fp)) {

        ERROR( DSPROC_LIB_NAME,
            "Could not read file: %s\n"
            " -> %s\n",
            full_path, strerror(errno));

        dsproc_set_status(DSPROC_EFILEREAD);
        fclose(fp);
        return(-1);
    }

    fclose(fp);

    /* Convert timestamp to seconds since 1970 */

    count = sscanf(timestamp,
        "%4d%2d%2d.%2d%2d%2d",
        &year, &mon, &day, &hour, &min, &sec);

    if (count != 6) {

        ERROR( DSPROC_LIB_NAME,
            "Invalid timestamp format '%s' in file: %s\n"
            " -> expected a string of the form YYYYMMDD.hhmmss'\n",
            timestamp, full_path);

        dsproc_set_status("Invalid Timestamp Format");
        return(-1);
    }

    *begin_time = get_secs1970(year, mon, day, hour, min, sec);

    return(1);
}

/**
 *  Static: Update the next processing interval begin time file.
 *
 *  @return
 *    -  1 if succesful
 *    -  0 if an error occurred
 */
static int _update_next_begin_time_file(time_t begin_time)
{
    LogFile    *log  = msngr_get_log_file();
    const char *path = log->path;
    const char *file = ".next_begin_time";
    char        full_path[PATH_MAX];
    FILE       *fp;
    char        timestamp[64];

    snprintf(full_path, PATH_MAX, "%s/%s", path, file);

    if (msngr_debug_level || msngr_provenance_level) {

        format_secs1970(begin_time, timestamp);

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Updating next processing period begin time file:\n"
            " -> file: %s\n"
            " -> time: %s\n",
            full_path, timestamp);
    }

    /* Convert timestamp to seconds since 1970 */

    if (!dsproc_create_timestamp(begin_time, timestamp)) {
        return(0);
    }

    /* Open the timestamp file */

    fp = fopen(full_path, "w");
    if (!fp) {

        ERROR( DSPROC_LIB_NAME,
            "Could not open file: %s\n"
            " -> %s\n",
            full_path, strerror(errno));
        dsproc_set_status(DSPROC_ELOGOPEN);
        return(0);
    }

    /* Update the timestamp */

    if (!fprintf(fp, "%s", timestamp)) {

        ERROR( DSPROC_LIB_NAME,
            "Could not write to file: %s\n"
            " -> %s\n",
            full_path, strerror(errno));

        dsproc_set_status(DSPROC_EFILEWRITE);
        fclose(fp);
        return(0);
    }

    fclose(fp);

    return(1);
}

/**
 *  Static: Check input datastreams for observation loop.
 *
 *  @return
 *    -  1 use observation loop
 *    -  0 not using observation loop
 *    - -1 memory error
 */
static void _check_for_obs_loop(void)
{
    int         dsid;
    DataStream *ds;

    for (dsid = 0; dsid < _DSProc->ndatastreams; dsid++) {

        ds = _DSProc->datastreams[dsid];

        if (ds->role == DSR_INPUT &&
            ds->flags & DS_OBS_LOOP) {

            _DSProc->use_obs_loop = 1;
            break;
        }
    }
}

/**
 *  Static: Set the next processing interval for observation loops.
 *
 *  @param  search_start - start time of search
 *
 *  @return
 *    -  1 if succesful
 *    -  0 if no new data was found
 *    - -1 if an error occurred
 */
int _set_next_obs_loop_interval(time_t search_start)
{
    int         dsid;
    DataStream *ds;

    DSFile     *dsfile;

    timeval_t   file_begin;
    timeval_t   file_end;

    timeval_t   search_begin = { search_start, 0 };
    timeval_t   begin        = { 0, 0 };
    timeval_t   end          = { 0, 0 };
    int         ti;

    int         status;

    const char *next_obs     = NULL;
    char        ts1[32];
    char        ts2[32];

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Searching for next observation after: %s\n",
        format_timeval(&search_begin, ts1));

    for (dsid = 0; dsid < _DSProc->ndatastreams; dsid++) {

        ds = _DSProc->datastreams[dsid];
        
        if (ds->role != DSR_INPUT) continue;

        if (!(ds->flags & DS_OBS_LOOP)) continue;

        status = _dsproc_find_next_dsfile(ds->dir, &search_begin, &dsfile);

        if (status < 0) {
            // error
            return(-1);
        }

        if (status == 0) {
            // no files found after specified search start time
            continue;
        }

        file_begin = dsfile->timevals[0];
        file_end   = dsfile->timevals[dsfile->ntimes - 1];

        /* Hack to get past files with corrupted time values that
         * resut in the end time being less that the begin time.
         * Without this hack we can get into an infinite loop. */

        if (TV_LTEQ(file_end, file_begin)) {

            /* Find the largest time value */
            file_end = file_begin;
            for (ti = 1; ti < dsfile->ntimes; ++ti) {
                if (TV_GT(dsfile->timevals[ti], file_end)) {
                    file_end = dsfile->timevals[ti];
                }
            }

            if (TV_EQ(file_end, file_begin)) {
                /* begin and end times are the same,
                 * so just add a minute. */
                file_end.tv_sec += 60;
            }
        }

        if (!begin.tv_sec || TV_LT(file_begin, begin)) {
            begin    = file_begin;
            end      = file_end;
            next_obs = dsfile->name;
        }
        else if (TV_EQ(file_begin, begin)) {
            if (TV_GT(file_end, end)) {
                end      = file_end;
                next_obs = dsfile->name;
            }
        }
    }

    if (!begin.tv_sec) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            " - none found\n");

        return(0);
    }

    _DSProc->interval_begin = begin.tv_sec;
    _DSProc->interval_end   = end.tv_sec + 1;

    DEBUG_LV1( DSPROC_LIB_NAME,
        " - found %s from %s to %s\n",
        next_obs,
        format_secs1970(_DSProc->interval_begin, ts1),
        format_secs1970(_DSProc->interval_end, ts2));

    return(1);
}

/**
 *  Static: Set the next processing period begin time for real time processes.
 *
 *  @return
 *    -  1 if succesful
 *    -  0 if the process needs to wait for new data
 *    - -1 if an error occurred
 */
static int _set_next_real_time_begin(void)
{
    int         ndsids;
    int        *dsids;
    int         dsid;
    int         i;

    time_t      now     = time(NULL);
    timeval_t   search  = { 0, 0 };
    timeval_t   found   = { 0, 0 };
    size_t      ntimes;

    time_t      begin;
    time_t      end;
    struct tm   gmt;
    int         status;

    char        ts1[32];

    /* Get the next begin time from the "next begin time" file if it exists. */

    begin  = 0;
    status = _read_next_begin_time_file(&begin);
    if (status < 0) {
        return(-1);
    }

    /* If the "next begin time" file does not exist, use the earliest end
     * time of all output datastreams to determine the next begin time. */

    if (status == 0) {

        begin = 0;
        end   = 0;

        /* Get the IDs of all output datastreams */

        ndsids = dsproc_get_output_datastream_ids(&dsids);
        if (ndsids < 0) return(-1);

        for (i = 0; i < ndsids; ++i) {

            dsid          = dsids[i];
            ntimes        = 1;
            search.tv_sec = now;

            if (!dsproc_fetch_timevals(
                dsid, NULL, &search, &ntimes, &found)) {

                if (ntimes != 0) {
                    free(dsids);
                    return(-1);
                }

                continue;
            }

            if (end == 0 || found.tv_sec < end) {
                end = found.tv_sec;
            }
        }

        free(dsids);

        if (end != 0) {

            DEBUG_LV1( DSPROC_LIB_NAME,
                "Getting processing period begin time from earliest output datastream end time\n");

            /* Set begin time to the start of the next processing interval
             * after the earliest output datastream end time. */

            begin = cds_get_midnight(end);
            while (begin < end) begin += _DSProc->proc_interval;
        }
    }

    /* If we still haven't been able to determine the begin time
     * we need to use the earliest available input data. */

    if (begin == 0) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Getting processing period begin time from earliest available input data\n");

        /* Get the IDs of all input datastreams */

        ndsids = dsproc_get_input_datastream_ids(&dsids);
        if (ndsids < 0) return(-1);

        for (i = 0; i < ndsids; ++i) {

            dsid          = dsids[i];
            ntimes        = 1;
            search.tv_sec = 1;

            if (!dsproc_fetch_timevals(
                dsid, &search, NULL, &ntimes, &found)) {

                if (ntimes != 0) {
                    free(dsids);
                    return(-1);
                }

                continue;
            }

            if (begin == 0 || found.tv_sec < begin) {
                begin = found.tv_sec;
            }
        }

        free(dsids);

        if (begin == 0) {

            LOG( DSPROC_LIB_NAME,
                "No data was found for any input datastreams.\n");

            dsproc_set_status(DSPROC_ENODATA);
            return(0);
        }

        /* Adjust begin time to either midnight or the start of the hour,
        *  depending on the processing interval.
        */

        memset(&gmt, 0, sizeof(struct tm));
        gmtime_r(&begin, &gmt);

        if (_DSProc->proc_interval != 3600) {
            gmt.tm_hour = 0;
        }

        gmt.tm_min = 0;
        gmt.tm_sec = 0;

        begin = timegm(&gmt);
    }

    if (msngr_debug_level || msngr_provenance_level) {

        format_secs1970(begin, ts1);

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Processing period begin time: %s\n",
            ts1);
    }

    _DSProc->period_begin = begin;

    return(1);
}

/**
 *  Static: Set the next processing period end time for real time processes.
 *
 *  @return
 *    -  1 if succesful
 *    -  0 if the process needs to wait for new data
 *    - -1 if an error occurred
 */
static int _set_next_real_time_end(void)
{
    int         ndsids;
    int        *dsids;
    DataStream *ds;
    int         dsid;
    int         i;

    time_t      now     = time(NULL);
    timeval_t   search  = { 0, 0 };
    timeval_t   found   = { 0, 0 };
    size_t      ntimes;

    time_t      end;
    time_t      max_end;
    time_t      delta_t;
    time_t      begin;
    int         count;

    char        ts1[32];

    /* Determine the end time of the processing period */

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Determining the processing period end time\n"
        " - using %d hours for maximum input data wait time",
        (int)(_MaxRealTimeWait/3600 + 0.5));

    end     = 0;
    max_end = 0;

    /* Get the IDs of all input datastreams */

    ndsids = dsproc_get_input_datastream_ids(&dsids);
    if (ndsids < 0) return(-1);

    for (i = 0; i < ndsids; ++i) {

        dsid          = dsids[i];
        ds            = _DSProc->datastreams[dsid];
        ntimes        = 1;
        search.tv_sec = now;

        if (!dsproc_fetch_timevals(
            dsid, NULL, &search, &ntimes, &found)) {

            if (ntimes != 0) {
                free(dsids);
                return(-1);
            }

            continue;
        }

        /* Adjust for the end time offset. */

        if (ds->ret_cache) {
            found.tv_sec -= ds->ret_cache->end_offset;
        }

        /* Keep track of the maximum end time found, we will try to use
         * this if no new data is found within the maximum wait time. */

        if (max_end == 0 ||
            max_end < found.tv_sec) {
            max_end = found.tv_sec;
        }

        /* We want the earliest end time found within the maximum wait time
         * window to ensure we have the most complete dataset possible. */

        delta_t = now - found.tv_sec;

        if (delta_t < _MaxRealTimeWait) {

            if (end == 0 ||
                end > found.tv_sec) {
                end = found.tv_sec;
            }
        }
    }

    free(dsids);

    if (end == 0) {

        if (max_end == 0) {

            LOG( DSPROC_LIB_NAME,
                "No new data was found for any input datastreams.\n");

            dsproc_set_status(DSPROC_ENODATA);
            return(0);
        }

        end = max_end;
    }

    _DSProc->period_end_max = end;

    /* Adjust end time so end - begin is an even multiple
     * of the processing interval.
     */

    begin = _DSProc->period_begin;

    if (end > begin) {
        count = (int)((end - begin) / _DSProc->proc_interval);
        end   = begin + (count * _DSProc->proc_interval);
    }

    /* Check if we have new data to process.
     */

    if (end <= begin) {

        LOG( DSPROC_LIB_NAME,
            "Missing input data for one or more datastreams.\n"
            " -> waiting for input data or the maximum wait time of %d hours is reached",
            (int)(_MaxRealTimeWait/3600 + 0.5));

        dsproc_set_status(DSPROC_ENODATA);
        return(0);
    }

    if (msngr_debug_level || msngr_provenance_level) {

        format_secs1970(end, ts1);

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Processing period end time: %s\n",
            ts1);
    }

    _DSProc->period_end = end;

    return(1);
}

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/**
 *  Free all memory used by the internal _DSProc structure.
 */
void _dsproc_destroy(void)
{
    int i;

    if (_LogsRoot) {
        free(_LogsRoot);
        _LogsRoot = (char *)NULL;
    }

    if (_LogsDir) {
        free(_LogsDir);
        _LogsDir = (char *)NULL;
    }

    if (_LogFile) {
        free(_LogFile);
        _LogFile = (char *)NULL;
    }

    if (_LogID) {
        free(_LogID);
        _LogID = (char *)NULL;
    }

    if (_DSProc) {

        if (_DSProc->lockfile_path && _DSProc->lockfile_name) {
            _unlock_process();
        }

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Freeing internal memory\n");

        if (_DSProc->site)      free((void *)_DSProc->site);
        if (_DSProc->facility)  free((void *)_DSProc->facility);
        if (_DSProc->name)      free((void *)_DSProc->name);
        if (_DSProc->type)      free((void *)_DSProc->type);
        if (_DSProc->version)   free((void *)_DSProc->version);
        if (_DSProc->db_alias)  free((void *)_DSProc->db_alias);
        if (_DSProc->site_desc) free((void *)_DSProc->site_desc);
        if (_DSProc->full_name) free((void *)_DSProc->full_name);

        if (_DSProc->retriever) {
            _dsproc_free_retriever();
        }

        if (_DSProc->ret_data) {
            cds_set_definition_lock(_DSProc->ret_data, 0);
            cds_delete_group(_DSProc->ret_data);
        }

        if (_DSProc->trans_data) {
            cds_set_definition_lock(_DSProc->trans_data, 0);
            cds_delete_group(_DSProc->trans_data);
        }

        if (_DSProc->trans_params) {
            cds_set_definition_lock(_DSProc->trans_params, 0);
            cds_delete_group(_DSProc->trans_params);
        }

        if (_DSProc->location)    dsdb_free_process_location(_DSProc->location);
        if (_DSProc->dsc_inputs)  dsdb_free_ds_classes(_DSProc->dsc_inputs);
        if (_DSProc->dsc_outputs) dsdb_free_ds_classes(_DSProc->dsc_outputs);
        if (_DSProc->dsdb)        dsdb_destroy(_DSProc->dsdb);
        if (_DSProc->dqrdb)       dqrdb_destroy(_DSProc->dqrdb);

        if (_DSProc->ndatastreams) {
            for (i = 0; i < _DSProc->ndatastreams; i++) {
                _dsproc_free_datastream(_DSProc->datastreams[i]);
            }
            free(_DSProc->datastreams);
        }

        if (_DSProc->output_intervals) _dsproc_free_output_intervals();

        free(_DSProc);

        _DSProc = (DSProc *)NULL;
    }

    cds_free_unit_system();
    _dsproc_free_exclude_atts();
    _dsproc_free_excluded_qc_vars();
    _dsproc_free_input_file_list();
    _dsproc_free_trans_qc_rollup_bit_descriptions();
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/**
 *  Abort the process and exit cleanly.
 *
 *  See convenience macro DSPROC_ABORT.
 *
 *  This function will:
 *
 *    - set the status of the process
 *    - append the error message to the log file and error mail message
 *    - call the users finish_process function (but only if dsproc_abort
 *      is not being called from there)
 *    - exit the process cleanly
 *
 *  The status string should be a brief one line error message that will
 *  be used as the process status in the database.  This is the message
 *  that will be displayed in DSView. If the status string is NULL the
 *  error message specified by the format string and args will be used.
 *
 *  The format string and args will be used to generate the complete and
 *  more detailed log and error mail messages. If the format string is
 *  NULL the status string will be used.
 *
 *  @param  func   - the name of the function sending the message (__func__)
 *  @param  file   - the source file the message came from (__FILE__)
 *  @param  line   - the line number in the source file (__LINE__)
 *  @param  status - brief error message to use for the process status.
 *  @param  format - format string for full error message (see printf)
 *  @param  ...    - arguments for the format string
 */
void dsproc_abort(
    const char *func,
    const char *file,
    int         line,
    const char *status,
    const char *format, ...)
{
    const char *sender = (_DSProc) ? _DSProc->full_name : "null";
    int         exit_value;
    va_list     args;

    if (!_DSProc) {
        fprintf(stderr,
            "dsproc_abort() called outside main processing framework\n");
        exit(1);
    }

    if (!func) func = "null";
    if (!file) file = "null";

    if (format || status) {

        if (!format) format = status;

        va_start(args, format);

        if (status) {
            dsproc_set_status(status);
        }
        else {
            status = msngr_format_va_list(format, args);
            if (status) {
                dsproc_set_status(status);
                free((void *)status);
            }
            else {
                dsproc_set_status(DSPROC_ENOMEM);
            }
        }

        msngr_vsend(sender, func, file, line, MSNGR_ERROR, format, args);

        va_end(args);
    }
    else {
        ERROR( DSPROC_LIB_NAME,
            "Error message not set in call to dsproc_abbort()\n");

        dsproc_set_status("Unknown Data Processing Error (check logs)");
    }

    if (!_InsideFinishProcessHook) {
        _dsproc_run_finish_process_hook();
    }

    exit_value = dsproc_finish();
    exit(exit_value);
}

/**
 *  Enable asynchronous processing mode.
 *
 *  Enabling asynchronous processing mode will allow multiple processes to be
 *  executed concurrently.  This will:
 *
 *    - disable the process lock file
 *    - disable check for chronological data processing
 *    - disable overlap checks with previously processed data
 *    - force a new file to be created for every output dataset
 */
void dsproc_enable_asynchronous_mode(void)
{
    DEBUG_LV1( DSPROC_LIB_NAME,
        "Enabling Asynchronous Processing Mode:\n"
        " - disabling the process lock file\n"
        " - disabling check for chronological data processing\n"
        " - disabling overlap checks with previously processed data\n"
        " - forcing a new file to be created for every output dataset\n");

    _DisableLockFile  = 1;
    _AsynchronousMode = 1;
}

/**
 *  Disable the datasystem process.
 *
 *  This function will set the status message, and cause the process to
 *  disable itself when it finishes if not running in force mode.
 *
 *  @param  message - disable process message
 */
void dsproc_disable(const char *message)
{
    int force_mode = dsproc_get_force_mode();

    if (!force_mode) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Setting disable process message: '%s'\n", message);

        strncpy((char *)_DSProc->disable, message, 511);
    }
    else {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Setting status to: '%s'\n", message);
    }

    strncpy((char *)_DSProc->status, message, 511);
}

/**
 *  Disable the database updates.
 *
 *  Disabling database updates will prevent the process from storing
 *  runtime status information in the database. This can be used to
 *  run processes that are connected to a read-only database.
 */
void dsproc_disable_db_updates(void)
{
    DEBUG_LV1( DSPROC_LIB_NAME, "Disabling database updates\n");

    _DisableDBUpdates = 1;
}

/**
 *  Disable the creation of the process lock file.
 *
 *  Warning: Disabling the lock file will allow multiple processes to run
 *  over the top of themselves and can lead to unpredictable behavior.
 */
void dsproc_disable_lock_file(void)
{
    DEBUG_LV1( DSPROC_LIB_NAME, "Disabling lock file\n");

    _DisableLockFile = 1;
}

/**
 *  Disable the mail messages.
 */
void dsproc_disable_mail_messages(void)
{
    DEBUG_LV1( DSPROC_LIB_NAME, "Disabling mail messages\n");

    _DisableMail = 1;
}

/**
 *  Get the asynchronous processing mode.
 *
 *  @return
 *    - 0 = disabled
 *    - 1 = enabled
 *
 *  @see dsproc_set_asynchronous_mode()
 */
int dsproc_get_asynchrounous_mode(void)
{
    return(_AsynchronousMode);
}

/**
 *  Get the expected data interval.
 *
 *  This is how often we expect to get data to process.
 *
 *  @return  expected data interval
 */
time_t dsproc_get_data_interval(void)
{
    return(_DSProc->data_interval);
}

/**
 *  Get the dynamic dods mode.
 *
 *  @return
 *    - 0 = disabled
 *    - 1 = enabled
 *    - 2 = enabled, but do not copy global attributes
 *          from input datasets to output datasets.
 *
 *  @see dsproc_set_dynamic_dods_mode()
 */
int dsproc_get_dynamic_dods_mode(void)
{
    return(_DynamicDODs);
}

/**
 *  Get the force mode.
 *
 *  The force mode can be enabled using the -F option on the command line.
 *  This mode can be used to force the process past all recoverable errors
 *  that would normally stop process execution.
 *
 *  @return
 *    - 0 = disabled
 *    - 1 = enabled
 *
 *  @see dsproc_set_force_mode()
 */
int dsproc_get_force_mode(void)
{
    return(_Force);
}

/**
 *  Get the input directory being used by an Ingest process.
 *
 *  @return
 *    - input directory
 *    - NULL if not set
 *
 *  @see dsproc_set_input_dir()
 */
const char *dsproc_get_input_dir(void)
{
    if (_InputDir[0]) return((const char *)_InputDir);
    return((const char *)NULL);
}

/**
 *  Get the current input file being processed by an Ingest.
 *
 *  @return
 *    - input file
 *    - NULL if not set
 *
 *  @see dsproc_set_input_source()
 */
const char *dsproc_get_input_file(void)
{
    if (_InputFile[0]) return((const char *)_InputFile);
    return((const char *)NULL);
}

/**
 *  Get the full path to the input file being processed by an Ingest.
 *
 *  @return
 *    - full path to the input file
 *    - NULL if not set
 *
 *  @see dsproc_set_input_source()
 */
const char *dsproc_get_input_source(void)
{
    if (_InputSource[0]) return((const char *)_InputSource);
    return((const char *)NULL);
}

/**
 *  Get the process max run time.
 *
 *  @return  max run time
 */
time_t dsproc_get_max_run_time(void)
{
    return(_DSProc->max_run_time);
}

/**
 *  Get the minimum valid data time for the process.
 *
 *  @return  minimum valid data time for the process
 */
time_t dsproc_get_min_valid_time(void)
{
    return(_DSProc->min_valid_time);
}

/**
 *  Get the begin and end times of the current processing interval.
 *
 *  @param  begin - output: processing interval begin time in seconds since 1970.
 *  @param  end   - output: processing interval end time in seconds since 1970.
 *
 *  @return  length of the data processing interval in seconds
 */
time_t dsproc_get_processing_interval(time_t *begin, time_t *end)
{
    if (begin) *begin = _DSProc->interval_begin;
    if (end)   *end   = _DSProc->interval_end;
    return(_DSProc->proc_interval);
}

/**
 *  Get the real time mode.
 *
 *  @return
 *    - 0 = disabled
 *    - 1 = enabled
 *
 *  @see dsproc_set_real_time_mode()
 */
int dsproc_get_real_time_mode(void)
{
    return(_RealTimeMode);
}

/**
 *  Get the reprocessing mode.
 *
 *  @return
 *    - 0 = disabled
 *    - 1 = enabled
 *
 *  @see dsproc_set_reprocessing_mode()
 */
int dsproc_get_reprocessing_mode(void)
{
    return(_Reprocessing);
}

/**
 *  Get the process start time.
 *
 *  @return  start time
 */
time_t dsproc_get_start_time(void)
{
    return(_DSProc->start_time);
}

/**
 *  Get the time remaining until the max run time is reached.
 *
 *  If the max run time has been exceeded a message will be added
 *  to the log file and the process status will be set appropriately.
 *
 *  @return
 *    -  time remaining until the max run time is reached
 *    -  0 if the max run time has been exceeded
 *    - -1 if the max run time has not been set
 */
time_t dsproc_get_time_remaining(void)
{
    time_t remaining;
    time_t exceeded;

    dsproc_reset_warning_count();

    /* All data processing loops should call this function, so we
     * add the logic to close all open datastream files that were
     * not accessed during the previous processing loop here: */

    dsproc_close_untouched_files();

    if (_DSProc->max_run_time == (time_t)0) {
        return(-1);
    }

    remaining = _DSProc->start_time
              + _DSProc->max_run_time
              - time(NULL);

    if (remaining <= 0) {

        exceeded = abs((int)remaining);

        LOG( DSPROC_LIB_NAME,
            "Exceeded max run time of %d seconds by %d seconds\n",
            (int)_DSProc->max_run_time, (int)exceeded);

        dsproc_set_status(DSPROC_ERUNTIME);
        return(0);
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Processing time remaining: %d seconds\n", (int)remaining);

    return(remaining);
}

/**
 *  Check if the last status was a fatal error.
 *
 *  This function is used to determine if the process should be
 *  forced to continue if the force_mode is enabled.
 *
 *  @param  last_errno  last errno value
 *
 *  @return
 *    - 1  if a fatal error occurred
 *         (i.e. memory allocation error, disk I/O error, etc...)
 *    - 0  if the process should attempt to continue.
 */
int dsproc_is_fatal(int last_errno)
{
    const char *status = dsproc_get_status();
    int         fi;

    if (!status) {
        status = "<null>";
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Checking for fatal system error:\n"
        " - dsproc_status: '%s'\n"
        " - errno:         %d = '%s'\n",
        status, last_errno, strerror(last_errno));

    const char *fatal[] = {
//        DSPROC_EBADDSID,        // "Invalid Datastream ID"
//        DSPROC_EBADINDSC,       // "Invalid Input Datastream Class"
//        DSPROC_EBADOUTDSC,      // "Invalid Output Datastream Class"
//        DSPROC_EBADOUTFORMAT,   // "Invalid Output Datastream Format"
        DSPROC_EBADRETRIEVER,   // "Invalid Retriever Definition"
//        DSPROC_EBASETIME,       // "Could Not Get Base Time For Time Variable"
//        DSPROC_EBOUNDSVAR,      // "Invalid Cell Boundary Variable or Definition"
        DSPROC_ECDSALLOCVAR,    // "Could Not Allocate Memory For Dataset Variable"
//        DSPROC_ECDSCHANGEATT,   // "Could Not Change Attribute Value In Dataset"
//        DSPROC_ECDSCOPY,        // "Could Not Copy Dataset Metadata"
//        DSPROC_ECDSDEFVAR,      // "Could Not Define Dataset Variable"
//        DSPROC_ECDSDELVAR,      // "Could Not Delete Dataset Variable"
//        DSPROC_ECDSGETTIME,     // "Could Not Get Time Values From Dataset"
//        DSPROC_ECDSSETATT,      // "Could Not Set Attribute Value In Dataset"
//        DSPROC_ECDSSETDATA,     // "Could Not Set Variable Data In Dataset"
//        DSPROC_ECDSSETDIM,      // "Could Not Set Dimension Length In Dataset"
//        DSPROC_ECDSSETTIME,     // "Could Not Set Time Values In Dataset"
//        DSPROC_ECLONEVAR,       // "Could Not Clone Dataset Variable"
//        DSPROC_EDATAATTTYPE,    // "Data Attribute Has Invalid Data Type"
        DSPROC_EDBCONNECT,      // "Database Connection Error"
        DSPROC_EDBERROR,        // "Database Error (see log file)"
        DSPROC_EDESTDIRMAKE,    // "Could Not Create Destination Directory"
        DSPROC_EDIRLIST,        // "Could Not Get Directory Listing"
        DSPROC_EDQRDBCONNECT,   // "DQR Database Connection Error"
        DSPROC_EDQRDBERROR,     // "DQR Database Error (see log file)"
        DSPROC_EDSPATH,         // "Could Not Determine Path To Datastream"
        DSPROC_EFILECOPY,       // "Could Not Copy File"
        DSPROC_EFILEMD5,        // "Could Not Get File MD5"
        DSPROC_EFILEMOVE,       // "Could Not Move File"
        DSPROC_EFILEOPEN,       // "Could Not Open File"
        DSPROC_EFILEREAD        // "Could Not Read From File"
        DSPROC_EFILEWRITE       // "Could Not Write To File"
        DSPROC_EFILESTATS,      // "Could Not Get File Stats"
        DSPROC_EFORCE,          // "Could Not Force Process To Continue"
        DSPROC_EFORK,           // "Could Not Create Fork For New Process"
//        DSPROC_EGLOBALATT,      // "Invalid Global Attribute Value"
//        DSPROC_EINITSIGS,       // "Could Not Initialize Signal Handlers"
        DSPROC_ELOGOPEN,        // "Could Not Open Log File"
        DSPROC_ELOGSPATH,       // "Could Not Determine Path To Logs Directory"
//        DSPROC_EMAILINIT,       // "Could Not Initialize Mail"
        DSPROC_EMD5CHECK,       // "Source And Destination File MD5s Do Not Match"
//        DSPROC_EMERGE,          // "Could Not Merge Datasets"
//        DSPROC_EMINTIME,        // "Found Data Time Before Minimum Valid Time"
//        DSPROC_ENCCLOSE,        // "Could Not Close NetCDF File"
        DSPROC_ENCCREATE,       // "Could Not Create NetCDF File"
        DSPROC_ENCOPEN,         // "Could Not Open NetCDF File"
        DSPROC_ENCREAD,         // "Could Not Read From NetCDF File"
        DSPROC_ENCSYNC,         // "Could Not Sync NetCDF File"
        DSPROC_ENCWRITE,        // "Could Not Write To NetCDF File"
//        DSPROC_ENODATA,         // "No Input Data Found"
//        DSPROC_ENOOUTDATA,      // "No Output Data Created"
        DSPROC_ENODOD,          // "DOD Not Defined In Database"
//        DSPROC_ENOFILETIME,     // "Could Not Determine File Time"
//        DSPROC_ENOINDSC,        // "Could Not Find Input Datastream Class In Database"
        DSPROC_ENOMEM,          // "Memory Allocation Error"
        DSPROC_ENORETRIEVER,    // "Could Not Find Retriever Definition In Database"
//        DSPROC_ENOSRCFILE,      // "Source File Does Not Exist"
//        DSPROC_EPROVOPEN,       // "Could Not Open Provenance Log"
//        DSPROC_EQCVARDIMS,      // "Invalid QC Variable Dimensions"
//        DSPROC_EQCVARTYPE,      // "Invalid Data Type For QC Variable"
//        DSPROC_EREGEX,          // "Regular Expression Error"
//        DSPROC_EREQVAR,         // "Required Variable Missing From Dataset"
//        DSPROC_EREQATT          // "Required Attribute Variable Missing From Variable or Dataset"
//        DSPROC_ERETRIEVER,      // "Could Not Retrieve Input Data"
//        DSPROC_ERUNTIME,        // "Maximum Run Time Limit Exceeded"
//        DSPROC_ESAMPLESIZE,     // "Invalid Variable Sample Size"
//        DSPROC_ETIMECALC,       // "Time Calculation Error"
//        DSPROC_ETIMEORDER,      // "Invalid Time Order"
//        DSPROC_ETIMEOVERLAP,    // "Found Overlapping Data Times"
        DSPROC_ETOOMANYINDSC,   // "Too Many Input Datastreams Defined In Database"
//        DSPROC_ETRANSFORM,      // "Could Not Transform Input Data"
        DSPROC_ETRANSPARAMLOAD, // "Could Not Load Transform Parameters File"
//        DSPROC_ETRANSQCVAR,     // "Could Not Create Consolidated Transformation QC Variable"
        DSPROC_EUNLINK,         // "Could Not Delete File"
//        DSPROC_EVARMAP,         // "Could Not Map Input Variable To Output Variable"
//        DSPROC_ECSVPARSER,      // "Could Not Parse CSV File"
        DSPROC_ECSVCONF,        // "Could Not Read CSV Ingest Configuration File"
//        DSPROC_ECSV2CDS,        // "Could Not Map Input CSV Data To Output Dataset"
        NULL
    };

    for (fi = 0; fatal[fi]; ++fi) {
        if (strcmp(status, fatal[fi]) == 0) {

            DEBUG_LV1( DSPROC_LIB_NAME,
                " - dsproc_status indicates a fatal system error\n");

            return(1);
        }
    }

    /* Check the last errno variable */

    switch (last_errno) {

        case EPERM:           /* Operation not permitted */
//        case ENOENT:        /* No such file or directory */
//        case ESRCH:         /* No such process */
        case EINTR:           /* Interrupted system call */
        case EIO:             /* I/O error */
        case ENXIO:           /* No such device or address */
//        case E2BIG:         /* Argument list too long */
//        case ENOEXEC:       /* Exec format error */
//        case EBADF:         /* Bad file number */
//        case ECHILD:        /* No child processes */
//        case EAGAIN:        /* Try again */
        case ENOMEM:          /* Out of memory */
        case EACCES:          /* Permission denied */
//        case EFAULT:        /* Bad address */
//        case ENOTBLK:       /* Block device required */
        case EBUSY:           /* Device or resource busy */
//        case EEXIST:        /* File exists */
//        case EXDEV:         /* Cross-device link */
        case ENODEV:          /* No such device */
//        case ENOTDIR:       /* Not a directory */
//        case EISDIR:        /* Is a directory */
//        case EINVAL:        /* Invalid argument */
//        case ENFILE:        /* File table overflow */
//        case EMFILE:        /* Too many open files */
//        case ENOTTY:        /* Not a typewriter */
        case ETXTBSY:         /* Text file busy */
//        case EFBIG:         /* File too large */
        case ENOSPC:          /* No space left on device */
//        case ESPIPE:        /* Illegal seek */
        case EROFS:           /* Read-only file system */
//        case EMLINK:        /* Too many links */
//        case EPIPE:         /* Broken pipe */
//        case EDOM:          /* Math argument out of domain of func */
//        case ERANGE:        /* Math result not representable */

//        case EDEADLK:       /* Resource deadlock would occur */
//        case ENAMETOOLONG:  /* File name too long */
//        case ENOLCK:        /* No record locks available */
//        case ENOSYS:        /* Function not implemented */
//        case ENOTEMPTY:     /* Directory not empty */
//        case ELOOP:         /* Too many symbolic links encountered */
//        case EWOULDBLOCK:   /* Operation would block */
//        case ENOMSG:        /* No message of desired type */
//        case EIDRM:         /* Identifier removed */
//        case ECHRNG:        /* Channel number out of range */
//        case EL2NSYNC:      /* Level 2 not synchronized */
//        case EL3HLT:        /* Level 3 halted */
//        case EL3RST:        /* Level 3 reset */
//        case ELNRNG:        /* Link number out of range */
//        case EUNATCH:       /* Protocol driver not attached */
//        case ENOCSI:        /* No CSI structure available */
//        case EL2HLT:        /* Level 2 halted */
//        case EBADE:         /* Invalid exchange */
//        case EBADR:         /* Invalid request descriptor */
//        case EXFULL:        /* Exchange full */
//        case ENOANO:        /* No anode */
//        case EBADRQC:       /* Invalid request code */
//        case EBADSLT:       /* Invalid slot */

//        case EBFONT:        /* Bad font file format */
//        case ENOSTR:        /* Device not a stream */
//        case ENODATA:       /* No data available */
//        case ETIME:         /* Timer expired */
        case ENOSR:           /* Out of streams resources */
// !OSX        case ENONET:          /* Machine is not on the network */
// !OSX        case ENOPKG:          /* Package not installed */
//        case EREMOTE:       /* Object is remote */
        case ENOLINK:         /* Link has been severed */
// !OSX        case EADV:            /* Advertise error */
// !OSX        case ESRMNT:          /* Srmount error */
// !OSX        case ECOMM:           /* Communication error on send */
//        case EPROTO:        /* Protocol error */
//        case EMULTIHOP:     /* Multihop attempted */
//        case EDOTDOT:       /* RFS specific error */
//        case EBADMSG:       /* Not a data message */
//        case EOVERFLOW:     /* Value too large for defined data type */
//        case ENOTUNIQ:      /* Name not unique on network */
// !OSX        case EBADFD:          /* File descriptor in bad state */
//        case EREMCHG:       /* Remote address changed */
// !OSX        case ELIBACC:         /* Can not access a needed shared library */
// !OSX        case ELIBBAD:         /* Accessing a corrupted shared library */
// !OSX        case ELIBSCN:         /* .lib section in a.out corrupted */
// !OSX        case ELIBMAX:         /* Attempting to link in too many shared libraries */
// !OSX        case ELIBEXEC:        /* Cannot exec a shared library directly */
        case EILSEQ:          /* Illegal byte sequence */
// !OSX        case ERESTART:        /* Interrupted system call should be restarted */
// !OSX        case ESTRPIPE:        /* Streams pipe error */
//        case EUSERS:        /* Too many users */
        case ENOTSOCK:        /* Socket operation on non-socket */
//        case EDESTADDRREQ:  /* Destination address required */
//        case EMSGSIZE:      /* Message too long */
        case EPROTOTYPE:      /* Protocol wrong type for socket */
        case ENOPROTOOPT:     /* Protocol not available */
        case EPROTONOSUPPORT: /* Protocol not supported */
        case ESOCKTNOSUPPORT: /* Socket type not supported */
        case EOPNOTSUPP:      /* Operation not supported on transport endpoint */
        case EPFNOSUPPORT:    /* Protocol family not supported */
        case EAFNOSUPPORT:    /* Address family not supported by protocol */
        case EADDRINUSE:      /* Address already in use */
        case EADDRNOTAVAIL:   /* Cannot assign requested address */
        case ENETDOWN:        /* Network is down */
        case ENETUNREACH:     /* Network is unreachable */
        case ENETRESET:       /* Network dropped connection because of reset */
        case ECONNABORTED:    /* Software caused connection abort */
        case ECONNRESET:      /* Connection reset by peer */
        case ENOBUFS:         /* No buffer space available */
        case EISCONN:         /* Transport endpoint is already connected */
        case ENOTCONN:        /* Transport endpoint is not connected */
        case ESHUTDOWN:       /* Cannot send after transport endpoint shutdown */
        case ETOOMANYREFS:    /* Too many references: cannot splice */
        case ETIMEDOUT:       /* Connection timed out */
        case ECONNREFUSED:    /* Connection refused */
        case EHOSTDOWN:       /* Host is down */
        case EHOSTUNREACH:    /* No route to host */
        case EALREADY:        /* Operation already in progress */
        case EINPROGRESS:     /* Operation now in progress */
        case ESTALE:          /* Stale NFS file handle */
//        case EUCLEAN:       /* Structure needs cleaning */
//        case ENOTNAM:       /* Not a XENIX named type file */
//        case ENAVAIL:       /* No XENIX semaphores available */
//        case EISNAM:        /* Is a named type file */
// !OSX        case EREMOTEIO:       /* Remote I/O error */
        case EDQUOT:          /* Quota exceeded */

//        case ENOMEDIUM:     /* No medium found */
//        case EMEDIUMTYPE:   /* Wrong medium type */
        case ECANCELED:       /* Operation Canceled */
// !OSX        case ENOKEY:          /* Required key not available */
// !OSX        case EKEYEXPIRED:     /* Key has expired */
// !OSX        case EKEYREVOKED:     /* Key has been revoked */
// !OSX        case EKEYREJECTED:    /* Key was rejected by service */

/* for robust mutexes */
        case EOWNERDEAD:      /* Owner died */
        case ENOTRECOVERABLE: /* State not recoverable */
// !OSX        case ERFKILL:         /* Operation not possible due to RF-kill */
// !OSX        case EHWPOISON:       /* Memory page has hardware error */

            DEBUG_LV1( DSPROC_LIB_NAME,
                " - last errno indicates a fatal system error\n");

            return(1);
        default:
            break;
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        " - not a fatal system error\n");

    return(0);
}

/**
 *  Set Dynamic DODs mode.
 *
 *  If the dynamic dods mode is enabled, the output DODs will be created
 *  and/or modified using all variables and associated attributes that
 *  are mapped to it.
 *
 *  @param  mode - dynamic dods mode:
 *                   - 0 = disabled
 *                   - 1 = enabled
 *                   - 2 = enabled, but do not copy global attributes
 *                         from input datasets to output datasets.
 *
 *
 *  @see dsproc_get_dynamic_dods_mode()
 */
void dsproc_set_dynamic_dods_mode(int mode)
{
    DEBUG_LV1( DSPROC_LIB_NAME,
        "Setting dynamic DODs mode to: %d\n", mode);

    _DynamicDODs = mode;
}

/**
 *  Set the force mode.
 *
 *  The force mode can be enabled using the -F option on the command line.
 *  This mode can be used to force the process past all recoverable errors
 *  that would normally stop process execution.
 *
 *  @param  mode - force mode (0 = disabled, 1 = enabled)
 *
 *  @see dsproc_get_force_mode()
 */
void dsproc_set_force_mode(int mode)
{
    DEBUG_LV1( DSPROC_LIB_NAME,
        "Setting force mode to: %d\n", mode);

    _Force = mode;
}

/**
 *  Set the input directory used to create the input_source attribute.
 *
 *  This function is called from the main Ingest files loop to set the
 *  current input directory being used by the Ingest. When new datasets
 *  are created this value will be used to populate the input_source
 *  global attribute value if it is defined in the DOD.
 *
 *  @param  input_dir - full path to the input directory
 */
void dsproc_set_input_dir(const char *input_dir)
{
    DEBUG_LV1( DSPROC_LIB_NAME,
        "Setting input directory: %s\n", input_dir);

    snprintf(_InputDir, PATH_MAX, "%s", input_dir);
}

/**
 *  Set the input file used to create the input_source attribute.
 *
 *  This function is called from the main Ingest files loop to set the
 *  current input file being used by the Ingest. When new datasets
 *  are created this value will be used to populate the input_source
 *  global attribute value if it is defined in the DOD.
 *
 *  @param  input_file - the name of the input file being processed
 */
void dsproc_set_input_source(const char *input_file)
{
    DEBUG_LV1( DSPROC_LIB_NAME,
        "Setting input source:    %s/%s\n", _InputDir, input_file);

    snprintf(_InputFile, PATH_MAX, "%s", input_file);
    snprintf(_InputSource, PATH_MAX*2, "%s/%s", _InputDir, input_file);
}

/**
 *  Set Log file directory.
 *
 *  @param  log_dir - full path to the log files directory
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int dsproc_set_log_dir(const char *log_dir)
{
    _LogsDir = strdup(log_dir);

    if (!_LogsDir) {

        ERROR( DSPROC_LIB_NAME,
            "Could not set log file directory: %s\n"
            " -> memory allocation error\n",
            log_dir);

        dsproc_set_status(DSPROC_ENOMEM);
        return(0);
    }

    return(1);
}

/**
 *  Set the name of the log file to use.
 *
 *  @param  log_file - name of the log file
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int dsproc_set_log_file(const char *log_file)
{
    _LogFile = strdup(log_file);

    if (!_LogFile) {

        ERROR( DSPROC_LIB_NAME,
            "Could not set log file name: %s\n"
            " -> memory allocation error\n",
            log_file);

        dsproc_set_status(DSPROC_ENOMEM);
        return(0);
    }

    return(1);
}

/**
 *  Replace timestamp in log file name with Log ID.
 *
 *  @param  log_id - log file ID
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int dsproc_set_log_id(const char *log_id)
{
    _LogID = strdup(log_id);

    if (!_LogID) {

        ERROR( DSPROC_LIB_NAME,
            "Could not set log file ID: %s\n"
            " -> memory allocation error\n",
            log_id);

        dsproc_set_status(DSPROC_ENOMEM);
        return(0);
    }

    return(1);
}

/**
 *  Set Log file interval.
 *
 *  @param  interval       - log file interval:
 *                             - LOG_MONTHLY = create monthly log files
 *                             - LOG_DAILY   = create daily log files
 *                             - LOG_RUN     = create one log file per run
 *
 *  @param  use_begin_time - VAP Only: flag indicating if the begin time
 *                           specified on the command line should be used
 *                           for the log file time.
 */
void dsproc_set_log_interval(LogInterval interval, int use_begin_time)
{
    _LogInterval  = interval;
    _LogDataTime  = use_begin_time;
}

/**
 *  Set the maximum runtime allowed for the process.
 *
 *  Calling this function will override the maximum runtime limit set
 *  in the database.
 *
 *  @param  max_runtime - max runtime allowed for the process.
 */
void dsproc_set_max_runtime(int max_runtime)
{
    DEBUG_LV1( DSPROC_LIB_NAME,
        "Setting maximum runtime to: %d\n", max_runtime);

    _MaxRunTime = max_runtime;
}

/**
 *  Set the begin and end times for the current processing interval.
 *
 *  This function can be used to override the begin and end times
 *  of the current processing interval and should be called from
 *  the pre-retrieval hook function.
 *
 *  @param  begin_time - begin time in seconds since 1970.
 *  @param  end_time   - end time in seconds since 1970.
 */
void dsproc_set_processing_interval(
    time_t begin_time,
    time_t end_time)
{
    char ts1[32], ts2[32];

    _DSProc->interval_begin = begin_time;
    _DSProc->interval_end   = end_time;
    _DSProc->proc_interval  = end_time - begin_time;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Setting processing interval:\n"
        " - begin time: %s\n"
        " - end time:   %s\n"
        " - interval:   %d seconds\n",
        format_secs1970(begin_time, ts1),
        format_secs1970(end_time,   ts2),
        _DSProc->proc_interval);
}

/**
 *  Set the offset to apply to the processing interval.
 *
 *  This function can be used to shift the processing interval and
 *  should be called from either the init-process or pre-retrieval
 *  hook function.
 *
 *  @param  offset - offset in seconds
 */
void dsproc_set_processing_interval_offset(time_t offset)
{
    DEBUG_LV1( DSPROC_LIB_NAME,
        "Setting processing interval offset to: %d seconds\n", (int)offset);

    _DSProc->interval_offset = offset;
}

/**
 *  Set the reprocessing mode.
 *
 *  If the reprocessing mode is enabled, the time validatation functions will
 *  not check if the data time is earlier than that of the latest processed
 *  data time.
 *
 *  @param  mode - reprocessing mode (0 = disabled, 1 = enabled)
 *
 *  @see dsproc_get_reprocessing_mode()
 */
void dsproc_set_reprocessing_mode(int mode)
{
    DEBUG_LV1( DSPROC_LIB_NAME,
        "Setting reprocessing mode to: %d\n", mode);

    _Reprocessing = mode;
}

/**
 *  Set the real time mode.
 *
 *  If the real time mode is enabled, the -b option will not be required
 *  on the command line. Instead the the end of the last processing interval
 *  will be tracked and used as the start of the next processing interval.
 *
 *  @param  mode     - real time mode (0 = disabled, 1 = enabled)
 *  @param  max_wait - maximum wait time for new data in hours
 *
 *  @see dsproc_get_reprocessing_mode()
 */
void dsproc_set_real_time_mode(int mode, float max_wait)
{
    DEBUG_LV1( DSPROC_LIB_NAME,
        "Setting real time mode to: %d\n"
        " -> max wait time = %g hours\n",
        mode, max_wait);

    _RealTimeMode = mode;

    if (max_wait >= 0) {
        _MaxRealTimeWait = (int)(max_wait * 3600.0);
    }
}

/**
 *  Initialize a data system process.
 *
 *  This function will:
 *    - Parse the command line
 *    - Connect to the database
 *    - Open the process log file
 *    - Initialize the mail messages
 *    - Update the process start time in the database
 *    - Initialize the signal handlers
 *    - Define non-standard unit symbols
 *    - Get process configuration information from database
 *    - Initialize input and output datastreams
 *    - Initialize the data retrival structures
 *
 *  The database connection will be left open when this function returns
 *  to allow the user's init_process() function to access the database without
 *  the need to reconnect to it. The database connection should be closed
 *  after the user's init_process() function returns.
 *
 *  The program will terminate inside this function if the -h (help) or
 *  -v (version) options are specified on the command line (exit value 0),
 *  or if an error occurs (exit value 1).
 *
 *  @param  argc         - command line argument count
 *  @param  argv         - command line argument vector
 *  @param  proc_model   - processing model to use
 *  @param  proc_version - process version
 *  @param  nproc_names  - number of valid process names
 *  @param  proc_names   - list of valid process names
 */
void dsproc_initialize(
    int          argc,
    char       **argv,
    ProcModel    proc_model,
    const char  *proc_version,
    int          nproc_names,
    const char **proc_names)
{
    const char *program_name = argv[0];
    time_t      start_time   = time(NULL);
    const char *site;
    const char *facility;
    const char *proc_name;
    const char *proc_type;
    int         db_attempts;
    FamProc    *fam_proc;
    char       *config_value;
    int         status;
    int         exit_value;

    /************************************************************
    *  Create the DSPROC structure
    *************************************************************/

    if (_DSProc) {
        dsproc_finish();
    }

    _DSProc = (DSProc *)calloc(1, sizeof(DSProc));
    if (!_DSProc) {

        fprintf(stderr,
            "%s: Memory allocation error initializing process\n",
            program_name);

        exit(1);
    }

    _DSProc->start_time = start_time;
    _DSProc->model      = proc_model;

    /* set version */

    if (proc_version) {
        if (!(_DSProc->version = strdup(proc_version))) {
            goto MEMORY_ERROR;
        }
        _dsproc_trim_version((char *)_DSProc->version);
    }
    else {
        if (!(_DSProc->version = strdup("Unknown"))) {
            goto MEMORY_ERROR;
        }
    }

    /* set process name if not from the command line */

    if (nproc_names == 1) {
        if (!(_DSProc->name = strdup(proc_names[0]))) {
            goto MEMORY_ERROR;
        }
    }

    /************************************************************
    *  Set process type and parse command line arguments
    *************************************************************/

    if (proc_model & DSP_INGEST) {

        if (!(_DSProc->type = strdup("Ingest"))) {
            goto MEMORY_ERROR;
        }

        if (proc_model & DSP_RETRIEVER ||
            proc_model & DSP_TRANSFORM) {

            /* Ingest/VAP Hybrid so set real-time mode
             * use VAP parse args */

            if (!dsproc_get_real_time_mode()) {
                dsproc_set_real_time_mode(1, 72);
            }

            _dsproc_vap_parse_args(argc, argv, nproc_names, proc_names);
        }
        else {
            _dsproc_ingest_parse_args(argc, argv, nproc_names, proc_names);
        }
    }
    else {
        if (!(_DSProc->type = strdup("VAP"))) {
            goto MEMORY_ERROR;
        }
        _dsproc_vap_parse_args(argc, argv, nproc_names, proc_names);
    }

    if (!(_DSProc->full_name = msngr_create_string(
        "%s-%s", _DSProc->name, _DSProc->type))) {

        goto MEMORY_ERROR;
    }

    /************************************************************
    *  Initialize the process
    *************************************************************/

    site      = _DSProc->site;
    facility  = _DSProc->facility;
    proc_name = _DSProc->name;
    proc_type = _DSProc->type;

    DEBUG_LV1_BANNER( DSPROC_LIB_NAME,
        "INITIALIZING PROCESS: %s%s-%s-%s\n",
        site, facility, proc_name, proc_type);

    if (!_DSProc->db_alias) {
        if (!(_DSProc->db_alias = strdup("dsdb_data"))) {
            goto MEMORY_ERROR;
        }
    }

    /************************************************************
    *  Create the lockfile for this process
    *************************************************************/

    if (!_DisableLockFile) {
        if (!_lock_process(site, facility, proc_name, proc_type)) {
            _dsproc_destroy();
            exit(1);
        }
    }

    /************************************************************
    *  Connect to the database
    *************************************************************/

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Initializing database connection: %s\n", _DSProc->db_alias);

    _DSProc->dsdb = dsdb_create(_DSProc->db_alias);
    if (!_DSProc->dsdb) {

        ERROR( DSPROC_LIB_NAME,
            "%s%s-%s-%s: Could not initialize database connection\n",
            site, facility, proc_name, proc_type);

        _dsproc_destroy();
        exit(1);
    }

    db_attempts = dsdb_connect(_DSProc->dsdb);

    if (db_attempts == 0) {

        ERROR( DSPROC_LIB_NAME,
            "%s%s-%s-%s: Could not connect to database\n",
            site, facility, proc_name, proc_type);

        _dsproc_destroy();
        exit(1);
    }

    if (msngr_debug_level) {

        if (_DSProc->dsdb->dbconn->db_host[0] != '\0') {
            DEBUG_LV1( DSPROC_LIB_NAME,
                " - db_host: %s\n", _DSProc->dsdb->dbconn->db_host);
        }

        if (_DSProc->dsdb->dbconn->db_name[0] != '\0') {
            DEBUG_LV1( DSPROC_LIB_NAME,
                " - db_name: %s\n", _DSProc->dsdb->dbconn->db_name);
        }

        if (_DSProc->dsdb->dbconn->db_user[0] != '\0') {
            DEBUG_LV1( DSPROC_LIB_NAME,
                " - db_user: %s\n", _DSProc->dsdb->dbconn->db_user);
        }
    }

    if (_DSProc->dsdb->dbconn->db_type == DB_WSPC) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            " - using read-only web service connection\n"
            " - disabled database updates\n"
            " - disabled mail messages\n");

        _DisableDBUpdates = 1;
        _DisableMail      = 1;
    }

    /************************************************************
    *  Make sure this is a valid datasystem process
    *************************************************************/

    status = dsdb_get_family_process(
        _DSProc->dsdb, site, facility, proc_type, proc_name, &fam_proc);

    if (status <= 0) {

        ERROR( DSPROC_LIB_NAME,
            "%s%s-%s-%s: Process not found in database\n",
            site, facility, proc_name, proc_type);

        _dsproc_destroy();
        exit(1);
    }

    dsdb_free_family_process(fam_proc);

    /************************************************************
    *  Open the provenance log
    *************************************************************/

    if (msngr_provenance_level) {

        if (!_init_provenance_log(site, facility, proc_name, proc_type)) {
            _dsproc_destroy();
            exit(1);
        }

        PROVENANCE_LV1( DSPROC_LIB_NAME,
            "Initializing process: %s%s-%s-%s\n",
            site, facility, proc_name, proc_type);
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Process version: %s\n", _DSProc->version);

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Library versions:\n"
        " - libdsproc3:  %s\n"
        " - libdsdb3:    %s\n"
        " - libtrans:    %s\n"
        " - libcds3:     %s\n"
        " - libncds3:    %s\n"
        " - libarmutils: %s\n"
        " - libdbconn:   %s\n"
        " - libmsngr:    %s\n",
        dsproc_lib_version(),
        dsdb_lib_version(),
        trans_lib_version(),
        cds_lib_version(),
        ncds_lib_version(),
        armutils_lib_version(),
        dbconn_lib_version(),
        msngr_lib_version());

    if (msngr_provenance_level) {

        if (_DSProc->lockfile_path && _DSProc->lockfile_name) {

            PROVENANCE_LV1( DSPROC_LIB_NAME,
                "Created process lockfile:\n"
                " - path: %s\n"
                " - name: %s\n",
                _DSProc->lockfile_path,
                _DSProc->lockfile_name);
        }

        PROVENANCE_LV1( DSPROC_LIB_NAME,
            "Using database connection:\n");

        if (_DSProc->dsdb->dbconn->db_host[0] != '\0') {
            PROVENANCE_LV1( DSPROC_LIB_NAME,
                " - db_host: %s\n", _DSProc->dsdb->dbconn->db_host);
        }

        if (_DSProc->dsdb->dbconn->db_name[0] != '\0') {
            PROVENANCE_LV1( DSPROC_LIB_NAME,
                " - db_name: %s\n", _DSProc->dsdb->dbconn->db_name);
        }

        if (_DSProc->dsdb->dbconn->db_user[0] != '\0') {
            PROVENANCE_LV1( DSPROC_LIB_NAME,
                " - db_user: %s\n", _DSProc->dsdb->dbconn->db_user);
        }

        if (_DSProc->dsdb->dbconn->db_type == DB_WSPC) {
            PROVENANCE_LV1( DSPROC_LIB_NAME,
                " - using read-only web service connection\n"
                " - disabled database updates\n"
                " - disabled mail messages\n");
        }
    }

    /************************************************************
    *  Open the log file
    *************************************************************/

    if (!_init_process_log(site, facility, proc_name, proc_type)) {
        _dsproc_destroy();
        exit(1);
    }

    /* Log the number of database connect attempts (if greater than 1) */

    if (db_attempts > 1) {

        LOG( DSPROC_LIB_NAME,
            "\nDB_ATTEMPTS: It took %d attempts to connect to the database.\n",
            db_attempts);
    }

    /************************************************************
    *  After this point the dsproc_finish() function should be
    *  used to cleanup before exiting.
    *************************************************************/

    if (!_dsproc_init()) {
        exit_value = dsproc_finish();
        exit(exit_value);
    }

    if (proc_model == PM_INGEST) {

        /************************************************************
        *  Initialize an Ingest process
        *************************************************************/

        /* Initialize the input datastreams */

        if (!_dsproc_init_input_datastreams()) {
            exit_value = dsproc_finish();
            exit(exit_value);
        }

        /* Initialize the output datastreams */

        if (!_dsproc_init_output_datastreams()) {
            exit_value = dsproc_finish();
            exit(exit_value);
        }
    }
    else {

        /************************************************************
        *  Initialize a VAP process
        *************************************************************/

        /* Initialize the output datastreams */

        if (!_dsproc_init_output_datastreams()) {
            exit_value = dsproc_finish();
            exit(exit_value);
        }

        /* Initialize the Retriever */

        if (!_dsproc_init_retriever()) {
            exit_value = dsproc_finish();
            exit(exit_value);
        }

        if ((proc_model & DSP_RETRIEVER_REQUIRED) &&
            !_DSProc->retriever) {

            ERROR( DSPROC_LIB_NAME,
                "Could not find retriever definition in database\n");

            dsproc_set_status(DSPROC_ENORETRIEVER);
            exit_value = dsproc_finish();
            exit(exit_value);
        }

        /* Get the data processing interval */

        status = dsproc_get_config_value("processing_interval", &config_value);

        if (status == 1) {
            _DSProc->proc_interval = (time_t)atoi(config_value);
            free(config_value);

            if (_DSProc->proc_interval <= 0) {
                status = 0;
            }
        }
        else if (status < 0) {
            exit_value = dsproc_finish();
            exit(exit_value);
        }

        if (status == 0) {

            if (_DSProc->cmd_line_end > _DSProc->cmd_line_begin) {

                _DSProc->proc_interval =
                    _DSProc->cmd_line_end - _DSProc->cmd_line_begin;

                DEBUG_LV1( DSPROC_LIB_NAME,
                    "Processing interval not defined or <= 0:\n"
                    " - using interval between begin and end times specified on command line: %d seconds\n",
                    _DSProc->proc_interval);
            }
            else {

                _DSProc->proc_interval = 86400;

                DEBUG_LV1( DSPROC_LIB_NAME,
                    "Processing interval not defined or <= 0:\n"
                    " - using default value: %d seconds\n",
                    _DSProc->proc_interval);
            }
        }

        /* Set the processing period. */

        if (_DSProc->cmd_line_begin) {
            _DSProc->period_begin = _DSProc->cmd_line_begin;
        }
        else {

            status = _set_next_real_time_begin();
            if (status <= 0) {
                exit_value = dsproc_finish();
                exit(exit_value);
            }
        }

        if (_DSProc->cmd_line_end) {
            _DSProc->period_end = _DSProc->cmd_line_end;
        }
        else if (_DSProc->cmd_line_begin) {
            _DSProc->period_end =
                _DSProc->cmd_line_begin + _DSProc->proc_interval;
        }
        else {

            status = _set_next_real_time_end();
            if (status <= 0) {
                exit_value = dsproc_finish();
                exit(exit_value);
            }
        }
    }

    return;

MEMORY_ERROR:

    fprintf(stderr,
        "%s: Memory allocation error initializing process\n",
        program_name);

    _dsproc_destroy();
    exit(1);
}

/**
 *  Start a processing interval loop.
 *
 *  This function will:
 *    - check if the process has (or will) exceed the maximum run time.
 *    - determine the begin and end times of the next processing interval.
 *
 *  @param  interval_begin - output: begin time of the processing interval
 *  @param  interval_end   - output: end time of the processing interval
 *
 *  @return
 *    - 1 if the next processing interval was returned
 *    - 0 if processing is complete
 */
int dsproc_start_processing_loop(
    time_t *interval_begin,
    time_t *interval_end)
{
    time_t time_remaining;
    char   begin_string[32];
    char   end_string[32];
    int    status;
    time_t next_begin_time;
    time_t last_begin_time;
    char   ts1[32], ts2[32];

    *interval_begin = 0;
    *interval_end   = 0;

    /* Determine the begin time of the next processing interval */

    if (_DSProc->interval_begin == 0) {

        _check_for_obs_loop(); // sets _DSProc->use_obs_loop

        if (!_DSProc->use_obs_loop) {

            /* Adjust processing period for the interval offset
             * that may have been set by the user. */

            _DSProc->period_begin += _DSProc->interval_offset;
            _DSProc->period_end   += _DSProc->interval_offset;

            if (_DSProc->period_end_max) {

                while (_DSProc->period_end > _DSProc->period_end_max) {
                    _DSProc->period_end -= _DSProc->proc_interval;
                }

                if (_DSProc->period_end <= _DSProc->period_begin) {

                    LOG( DSPROC_LIB_NAME,
                        "Missing input data for one or more datastreams.\n"
                        " -> waiting for input data or the maximum wait time of %d hours is reached",
                        (int)(_MaxRealTimeWait/3600 + 0.5));

                    dsproc_set_status(DSPROC_ENODATA);
                    return(0);
                }
            }
        }

        next_begin_time = _DSProc->period_begin;
    }
    else {
        next_begin_time = _DSProc->interval_end;
    }

    if (!_DSProc->cmd_line_begin) {

        // A begin time was not specified on the command line so
        // we are running in "real time" mode.

        if (!_update_next_begin_time_file(next_begin_time)) {
            return(0);
        }
    }
    else {

        // Check if a next_begin_time file exists and update it if the
        // current begin time is greater than the time in the file.

        status = _read_next_begin_time_file(&last_begin_time);
        if (status < 0) return(0);
        if (status > 0 && next_begin_time > last_begin_time) {

            if (!_update_next_begin_time_file(next_begin_time)) {
                return(0);
            }
        }
    }

    /* Set process interval begin and end times */

    if (_DSProc->use_obs_loop) {

        status = _set_next_obs_loop_interval(next_begin_time);
        if (status <= 0) {

            if (status == 0 && (next_begin_time == _DSProc->period_begin)) {

                format_secs1970(next_begin_time, ts1);

                LOG( DSPROC_LIB_NAME,
                    "\nNo data found after: %s\n",
                    ts1);

                dsproc_set_status(DSPROC_ENODATA);
            }

            return(0);
        }
        
        if (_DSProc->interval_begin > _DSProc->period_end) {

            if (next_begin_time == _DSProc->period_begin) {

                format_secs1970(_DSProc->period_begin, ts1);
                format_secs1970(_DSProc->period_end,   ts2);

                LOG( DSPROC_LIB_NAME,
                    "\nNo data found from '%s' to '%s'\n",
                    ts1, ts2);

                dsproc_set_status(DSPROC_ENODATA);
            }

            return(0);
        }
    }
    else {
        _DSProc->interval_begin = next_begin_time;

        /* Determine the end time of the next processing interval */

        _DSProc->interval_end = _DSProc->interval_begin
                              + _DSProc->proc_interval;

        if (_DSProc->interval_end > _DSProc->period_end) {

            if (_DSProc->interval_begin == _DSProc->period_begin) {
                _DSProc->interval_end    = _DSProc->period_end;
            }
            else {
                return(0);
            }
        }
    }

    *interval_begin = _DSProc->interval_begin;
    *interval_end   = _DSProc->interval_end;

    /* Check the run time */

    if (_DSProc->loop_begin != 0) {
        _DSProc->loop_end = time(NULL);
    }

    time_remaining = dsproc_get_time_remaining();

    if (time_remaining >= 0) {

        if (time_remaining == 0) {
            return(0);
        }
        else if ((_DSProc->loop_end - _DSProc->loop_begin) > time_remaining) {

            LOG( DSPROC_LIB_NAME,
                "\nStopping vap before max run time of %d seconds is exceeded\n",
                (int)dsproc_get_max_run_time());

            dsproc_set_status(DSPROC_ERUNTIME);
            return(0);
        }
    }

    _DSProc->loop_begin = time(NULL);

    /* Print debug and log messages */

    format_secs1970(*interval_begin, begin_string);
    format_secs1970(*interval_end, end_string);

    DEBUG_LV1_BANNER( DSPROC_LIB_NAME,
        "PROCESSING DATA:\n"
        " - from: %s\n"
        " - to:   %s\n",
        begin_string, end_string);

    LOG( DSPROC_LIB_NAME,
        "\nProcessing data: %s -> %s\n",
        begin_string, end_string);

    /* Update all datastream DODs for the current processing interval */

    if (!dsproc_update_datastream_dsdods(*interval_begin)) {
        return(0);
    }

    return(1);
}

/**
 *  Finish a data system process.
 *
 *  This function will:
 *
 *    - Update the process status in the database
 *    - Log all process stats that were recorded
 *    - Disconnect from the database
 *    - Mail all messages that were generated
 *    - Close the process log file
 *    - Free all memory used by the internal DSProc structure
 *
 *  @return  suggested program exit value (0 = success, 1 = failure)
 *
 *  @see dsproc_init()
 */
int dsproc_finish(void)
{
    ProcStatus *proc_status      = (ProcStatus *)NULL;
    char       *last_status_text = (char *)NULL;
    time_t      last_successful  = 0;
    time_t      last_completed   = 0;
    int         no_data_found    = 0;
    int         no_data_ok       = 0;
    int         total_in_records = 0;
    int         total_records    = 0;
    int         total_files      = 0;
    int         last_errno       = errno;
    int         successful;
    const char *status_name;
    const char *status_text;
    char       *status_message;
    char        status_note[128];
    time_t      delta_t;
    time_t      finish_time;
    char        finish_time_string[32];
    int         mail_error_status;
    DataStream *ds;
    int         dsi, fi, loop;
    char       *hostname;
    char        time_string1[32];
    char        time_string2[32];
    int         exit_value;

    dsproc_reset_warning_count();

    DEBUG_LV1_BANNER( DSPROC_LIB_NAME,
        "EXITING PROCESS\n");

    /************************************************************
    *  Log output data and file stats
    *************************************************************/

    for (dsi = 0; dsi < _DSProc->ndatastreams; dsi++) {

        ds = _DSProc->datastreams[dsi];

        if (ds->role == DSR_INPUT &&
            ds->total_records     &&
            _DSProc->retriever) {

            format_timeval(&(ds->begin_time), time_string1);

            if (ds->end_time.tv_sec) {
                format_timeval(&(ds->end_time), time_string2);
            }
            else {
                strcpy(time_string2, "none");
            }

            DEBUG_LV1( DSPROC_LIB_NAME,
                "\n"
                "Datastream Stats: %s\n"
                " - begin time:    %s\n"
                " - end time:      %s\n"
                " - total records: %d\n",
                ds->name, time_string1, time_string2,
                ds->total_records);

            total_in_records += ds->total_records;

            continue;
        }
    }

    for (loop = 0; loop < 2; ++loop) {

        for (dsi = 0; dsi < _DSProc->ndatastreams; dsi++) {

            ds = _DSProc->datastreams[dsi];

            if (loop == 0 && ds->dsc_level[0] != '0') {
                continue;
            }
            else if (loop == 1 && ds->dsc_level[0] == '0') {
                continue;
            }

            if (ds->role == DSR_OUTPUT &&
                ds->begin_time.tv_sec) {

                format_timeval(&(ds->begin_time), time_string1);

                if (ds->end_time.tv_sec) {
                    format_timeval(&(ds->end_time), time_string2);
                }
                else {
                    strcpy(time_string2, "none");
                }

                LOG( DSPROC_LIB_NAME,
                    "\n"
                    "Datastream Stats: %s\n"
                    " - begin time:    %s\n"
                    " - end time:      %s\n",
                    ds->name, time_string1, time_string2);

                if (ds->total_files) {

                    LOG( DSPROC_LIB_NAME,
                        " - total files:   %d\n"
                        " - total bytes:   %lu\n",
                        ds->total_files, (unsigned long)ds->total_bytes);

                    total_files += ds->total_files;
                }

                if (ds->total_records) {

                    LOG( DSPROC_LIB_NAME,
                        " - total records: %d\n",
                        ds->total_records);

                    total_records += ds->total_records;
                }

                if (ds->updated_files) {

                    LOG( DSPROC_LIB_NAME,
                        " - output files:\n");

                    for (fi = 0; ds->updated_files[fi]; ++fi) {
                        LOG( DSPROC_LIB_NAME,
                            "    - %s\n", ds->updated_files[fi]);
                    }
                }

                continue;
            }
        }
    }

    /************************************************************
    *  Set status_name and status_text values
    *************************************************************/

    status_text = _DSProc->status;
    if (status_text[0] == '\0') {

        if (total_files || total_records ||
            dsproc_get_quicklook_mode() == QUICKLOOK_ONLY) {

            status_text = DSPROC_SUCCESS;
        }
        else if (_DSProc->retriever && !total_in_records) {
            status_text = DSPROC_ENODATA;
        }
        else {
            status_text = DSPROC_ENOOUTDATA;
        }
    }

    if (strcmp(status_text, DSPROC_SUCCESS) == 0) {
        status_name = "Success";
        successful  = 1;
    }
    else if (strcmp(status_text, DSPROC_ENODATA) == 0) {
        status_name   = "NoDataFound";
        successful    = 0;
        no_data_found = 1;
    }
    else if (strcmp(status_text, DSPROC_ENOOUTDATA) == 0) {
        status_name   = "NoDataFound";
        successful    = 0;
        no_data_found = 1;
    }
    else if (strcmp(status_text, DSPROC_ERUNTIME) == 0) {
        status_name   = "MaxRuntimeExceeded";
        successful    = 0;
    }
    else {
        status_name = "Failure";
        successful  = 0;
    }

    status_note[0] = '\0';

    /************************************************************
    *  Set the process status in the database
    *************************************************************/

    finish_time = time(NULL);

    if (!_DisableDBUpdates) {

        if (dsproc_db_connect()) {

            DEBUG_LV1( DSPROC_LIB_NAME,
                "Updating process status in database\n");

            /* Check if we need to disable the process */

            if (_DSProc->disable[0] != '\0') {

                ERROR( DSPROC_LIB_NAME,
                    "Disabling Process: %s\n", _DSProc->disable);

                finish_time = time(NULL);

                dsdb_update_process_state(_DSProc->dsdb,
                    _DSProc->site, _DSProc->facility, _DSProc->type, _DSProc->name,
                    "AutoDisabled", _DSProc->disable, finish_time);
            }

            /* Get the status of the last run */

            dsdb_get_process_status(_DSProc->dsdb,
                _DSProc->site, _DSProc->facility, _DSProc->type, _DSProc->name,
                &proc_status);

            if (proc_status) {
                last_status_text = proc_status->text;
                last_successful  = proc_status->last_successful;
                last_completed   = proc_status->last_completed;
            }

            /*  Update the status in the database...
             *
             *  We do not want to update the status in the database if no
             *  input data was found and the data expectation interval is
             *  greater than the difference between the process start time
             *  and the last successful time.
             */

            if (no_data_found) {

                delta_t = _DSProc->start_time - last_successful;

                if (_DSProc->data_interval > delta_t) {

                    status_name = "Success";
                    status_text = DSPROC_SUCCESS;
                    no_data_ok  = 1;
                    successful  = 1;

                    snprintf(status_note, 128,
                        " -> No input data was found but we are within\n"
                        " -> the data expectation interval of %g hours.\n",
                        (double)_DSProc->data_interval/3600);
                }
            }

            finish_time = time(NULL);

            if (no_data_ok) {
                dsdb_update_process_completed(_DSProc->dsdb,
                    _DSProc->site, _DSProc->facility, _DSProc->type, _DSProc->name,
                    finish_time);
            }
            else {
                dsdb_update_process_status(_DSProc->dsdb,
                    _DSProc->site, _DSProc->facility, _DSProc->type, _DSProc->name,
                    status_name, status_text, finish_time);
            }

            /* Store any updated datastream times */

            _dsproc_store_output_datastream_times();

            /* Close database connection */

            dsproc_db_disconnect();
        }
        else {

            ERROR( DSPROC_LIB_NAME,
                "Could not update process status in database:\n"
                " -> database connect error\n");

            snprintf(status_note, 128,
                " -> Could not update status in database\n");
        }
    }

    /************************************************************
    *  Create the status message
    *************************************************************/

    hostname = dsenv_get_hostname();
    if (!hostname) {
        hostname = "unknown";
    }

    format_secs1970(finish_time, finish_time_string);

    status_message = msngr_create_string(
        "Current Status (%s):\n"
        "Process: %s%s-%s-%s\n"
        "Version: %s\n"
        "Host:    %s\n"
        "Status:  %s\n"
        "%s",
        finish_time_string,
        _DSProc->site, _DSProc->facility, _DSProc->name, _DSProc->type,
        _DSProc->version, hostname, status_text, status_note);

    if (!status_message) {

        ERROR( DSPROC_LIB_NAME,
            "Could not create status message\n"
            " -> memory allocation error\n");
    }

    /************************************************************
    *  Add process status to the mail messages
    *************************************************************/

    if (!_DisableMail) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Adding process status to mail messages\n");

        /* Error Mail */

        mail_error_status = 1;

        if (successful) {
            mail_error_status = 0;
        }
        else if (last_status_text) {
            if (strcmp(last_status_text, status_text) == 0) {
                mail_error_status = 0;
            }
        }

        _finish_mail(MSNGR_ERROR, mail_error_status, status_message,
            last_status_text, last_completed, last_successful, finish_time_string);

        /* Warning Mail */

        _finish_mail(MSNGR_WARNING, 0, status_message,
            last_status_text, last_completed, last_successful, finish_time_string);

        /* Maintainer Mail */

        _finish_mail(MSNGR_MAINTAINER, 0, status_message,
            last_status_text, last_completed, last_successful, finish_time_string);
    }

    /************************************************************
    *  Add process status to the log file
    *************************************************************/

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Adding process status to log file\n");

    if (status_message) {

        LOG( DSPROC_LIB_NAME,
            "\n%s", status_message);

        free(status_message);
    }

    /************************************************************
    *  Send the mail and close the log file
    *************************************************************/

    msngr_finish();

    /************************************************************
    *  Set suggested program exit value
    *************************************************************/

    if (successful) {
        exit_value = 0;
    }
    else if (dsproc_is_fatal(last_errno)) {
        exit_value = 2;
    }
    else {
        exit_value = 1;
    }

    /************************************************************
    *  Free the memory
    *************************************************************/

    if (proc_status) {
        dsdb_free_process_status(proc_status);
    }

    _dsproc_destroy();

    /************************************************************
    *  Return suggested exit value
    *************************************************************/

    if (msngr_debug_level || msngr_provenance_level) {

        if (successful) {
            DEBUG_LV1( DSPROC_LIB_NAME,
                "Suggested exit value: %d (successful)\n", exit_value);
        }
        else {
            DEBUG_LV1( DSPROC_LIB_NAME,
                "Suggested exit value: %d (failure)\n", exit_value);
        }
    }

    return(exit_value);
}

/************************************************************
 *  Print the contents of the internal DSProc structure.
 *
 *  @param  fp - pointer to the output file
 */
/*

BDE: None of this has been updated in a looooong time, and a lot has been
added. I leave it here in the off chance I will want/need it again...

void dsproc_print_info(FILE *fp)
{
    DSClass   *dsc;
    DSDOD     *dsdod;
    DataTimes *data_times;
    LogFile   *log;
    Mail      *mail;
    char       time_string1[32];
    char       time_string2[32];
    int        i;

    if (!_DSProc) {
        fprintf(fp,
            "Datasystem process has not been initialized.\n");
    }

    if (_DSProc->dsdb && _DSProc->dsdb->dbconn) {
        fprintf(fp,
        "\n"
        "Database Host:   %s\n"
        "Database Name:   %s\n"
        "Database User:   %s\n",
        _DSProc->dsdb->dbconn->db_host,
        _DSProc->dsdb->dbconn->db_name,
        _DSProc->dsdb->dbconn->db_user);
    }

    fprintf(fp,
        "\n"
        "Site:            %s\n"
        "Facility:        %s\n"
        "Process Name:    %s\n"
        "Process Type:    %s\n"
        "Process Version: %s\n",
        _DSProc->site, _DSProc->facility, _DSProc->name, _DSProc->type,
        _DSProc->version);

    format_secs1970(_DSProc->start_time, time_string1);
    format_secs1970(_DSProc->min_valid_time, time_string2);

    fprintf(fp,
        "\n"
        "Start Time:      %s\n"
        "Max Run Time:    %d seconds\n"
        "Min Valid Time:  %s\n",
        time_string1, (int)_DSProc->max_run_time, time_string2);

    if (_DSProc->location) {
        fprintf(fp,
        "\n"
        "Location:        %s\n"
        "Latitude:        %g N\n"
        "Longitude:       %g E\n"
        "Altitude:        %g MSL\n",
        _DSProc->location->name,
        _DSProc->location->lat,
        _DSProc->location->lon,
        _DSProc->location->alt);
    }

    log = msngr_get_log_file();

    if (log) {
        fprintf(fp,
        "\n"
        "Log File Path:   %s\n"
        "Log File Name:   %s\n",
        log->path, log->name);
    }

    mail = msngr_get_mail(MSNGR_ERROR);
    if (mail) {
        fprintf(fp,
        "\n"
        "Error Mail:      %s\n", mail->to);
    }

    mail = msngr_get_mail(MSNGR_WARNING);
    if (mail) {
        fprintf(fp,
        "Warning Mail:    %s\n", mail->to);
    }

    mail = msngr_get_mail(MSNGR_MAINTAINER);
    if (mail) {
        fprintf(fp,
        "Mentor Mail:     %s\n", mail->to);
    }

    if (_DSProc->data_interval) {
        fprintf(fp,
        "\n"
        "Expected Data Interval:   %d seconds\n", (int)_DSProc->data_interval);
    }

    if (_DSProc->proc_interval) {
        fprintf(fp,
        "\n"
        "Data Processing Interval: %d seconds\n", (int)_DSProc->proc_interval);
    }

    if (_DSProc->ndsc_inputs) {
        fprintf(fp,
        "\n"
        "Input Datastream Classes:\n"
        "\n");

        for (i = 0; i < _DSProc->ndsc_inputs; i++) {
            dsc = _DSProc->dsc_inputs[i];
            fprintf(fp, "    %s.%s\n", dsc->name, dsc->level);
        }
    }

    if (_DSProc->ndsc_outputs) {
        fprintf(fp,
        "\n"
        "Output Datastream Classes:\n"
        "\n");

        for (i = 0; i < _DSProc->ndsc_outputs; i++) {
            dsc = _DSProc->dsc_outputs[i];
            fprintf(fp, "    %s.%s\n", dsc->name, dsc->level);
        }
    }

    if (_DSProc->ndsdods) {
        fprintf(fp,
        "\n"
        "Loaded Datastream DODs:\n"
        "\n");

        for (i = 0; i < _DSProc->ndsdods; i++) {
            dsdod = _DSProc->dsdods[i];

            fprintf(fp,
            "    %s%s%s.%s-%s\n",
            _DSProc->site, dsdod->name, _DSProc->facility, dsdod->level,
            dsdod->version);
        }
    }

    if (_DSProc->ndatatimes) {
        fprintf(fp,
        "\n"
        "Previously Processed Data Times:\n");

        for (i = 0; i < _DSProc->ndatatimes; i++) {

            data_times = _DSProc->data_times[i];

            if (data_times->begin_time.tv_sec) {
                format_timeval(&data_times->begin_time, time_string1);
            }
            else {
                strcpy(time_string1, "N/A");
            }

            if (data_times->end_time.tv_sec) {
                format_timeval(&data_times->end_time, time_string2);
            }
            else {
                strcpy(time_string2, "N/A");
            }

            fprintf(fp,
            "\n"
            "    %s%s%s.%s\n"
            "     - begin time: %s\n"
            "     - end time:   %s\n",
            _DSProc->site, data_times->dsc_name, _DSProc->facility,
            data_times->dsc_level,
            time_string1,
            time_string2);
        }
    }

    if (_DSProc->input_dir) {
        fprintf(fp,
        "\n"
        "Input Directory: %s\n", _DSProc->input_dir);
    }

    if (_DSProc->file_patterns) {
        fprintf(fp,
        "\n"
        "File Patterns:\n"
        "\n");

        for (i = 0; i < _DSProc->npatterns; i++) {
            fprintf(fp,
            "    %s\n", _DSProc->file_patterns[i]);
        }
    }
}
*/

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Get the process status.
 *
 *  @return  process status message
 */
const char *dsproc_get_status(void)
{
    return(_DSProc->status);
}

/**
 *  Set the process status.
 *
 *  @param  status - process status message
 */
void dsproc_set_status(const char *status)
{
    if (status) {
        DEBUG_LV1( DSPROC_LIB_NAME,
            "Setting status to: '%s'\n", status);

        strncpy((char *)_DSProc->status, status, 511);
    }
    else {
        DEBUG_LV1( DSPROC_LIB_NAME,
            "Clearing last status string\n");

        strcpy((char *)_DSProc->status, "");
    }
}

/**
 *  Get the process site.
 *
 *  @return  site name
 */
const char *dsproc_get_site(void)
{
    return(_DSProc->site);
}

/**
 *  Get the process facility.
 *
 *  @return  facility
 */
const char *dsproc_get_facility(void)
{
    return(_DSProc->facility);
}

/**
 *  Get the process name.
 *
 *  @return  process name
 */
const char *dsproc_get_name(void)
{
    return(_DSProc->name);
}

/**
 *  Get the process type.
 *
 *  @return  process type
 */
const char *dsproc_get_type(void)
{
    return(_DSProc->type);
}

/**
 *  Get the process version.
 *
 *  @return  process version
 */
const char *dsproc_get_version(void)
{
    return(_DSProc->version);
}

/**
 *  Estimate timezone offset from longitude of process.
 *
 *  This function will generate a warning if the longitude of the
 *  process is specified as missing (-9999) in the database.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  tz_offset - output: timezone offset (in hours) 
 *                              estimated from the process location
 *
 *  @return
 *    -  1  Successful
 *    -  0  Longitude of the process is specified as missing (-9999) in the database.
 *    - -1  Memory allocation error
 */
int dsproc_estimate_timezone(int *tz_offset)
{
    ProcLoc *proc_loc;
    double   lon;

    *tz_offset = 0;

    if (dsproc_get_location(&proc_loc) <= 0) {
        ERROR( DSPROC_LIB_NAME,
            "Could not estimate timezone from process location\n"
            " -> memory allocation error getting process location\n");
        dsproc_set_status(DSPROC_ENOMEM);
        return(-1);
    }

    if (proc_loc->lon < -360) {
        WARNING( DSPROC_LIB_NAME,
            "Could not estimate timezone from process location\n"
            " -> process longitude in database is: %g\n", proc_loc->lon);
        return(0);
    }

    lon = proc_loc->lon;

    if (lon > 0) {
        lon -= 360.0;
    }

    *tz_offset = (int)(lon/15.0);

    if (lon < -180) {
        *tz_offset += 24;
    }

    return(1);
}
