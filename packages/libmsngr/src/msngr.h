/*******************************************************************************
*
*  COPYRIGHT (C) 2010 Battelle Memorial Institute.  All Rights Reserved.
*
********************************************************************************
*
*  Author:
*     name:  Brian Ermold
*     phone: (509) 375-2277
*     email: brian.ermold@pnl.gov
*
********************************************************************************
*
*  REPOSITORY INFORMATION:
*    $Revision: 13168 $
*    $Author: ermold $
*    $Date: 2012-03-24 01:49:45 +0000 (Sat, 24 Mar 2012) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file msngr.h
 *  Header file for the libmsngr library.
 */

#ifndef _MSNGR_H_
#define _MSNGR_H_ 1

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>

/** Messenger library name. */
#define MSNGR_LIB_NAME "libmsngr"

/** Define SOLARIS if this is a solaris system. */
#ifndef SOLARIS
#define SOLARIS (defined(sun) && (defined(__svr4__) || defined(__SVR4)))
#endif

/** Define LINUX if this is a linux system. */
#ifndef LINUX
#define LINUX (defined(linux))
#endif

/** External variable containing the debug level. */
extern int msngr_debug_level;

/** External variable containing the provenance level. */
extern int msngr_provenance_level;

/**
 *  @addtogroup MESSENGER
 */
/*@{*/

/*******************************************************************************
*  Message Types
*/

/**
 *  Message Types.
 */
typedef enum {
    MSNGR_LOG               = 1,   /**< Log Message                  */

    MSNGR_ERROR             = 10,  /**< Error Log and Mail Message   */
    MSNGR_WARNING           = 11,  /**< Warning Log and Mail Message */
    MSNGR_MAINTAINER        = 12,  /**< Maintainer Mail Message      */

    MSNGR_DEBUG_LV1         = 21,  /**< Level 1 Debug Message        */
    MSNGR_DEBUG_LV2         = 22,  /**< Level 2 Debug Message        */
    MSNGR_DEBUG_LV3         = 23,  /**< Level 3 Debug Message        */
    MSNGR_DEBUG_LV4         = 24,  /**< Level 4 Debug Message        */
    MSNGR_DEBUG_LV5         = 25,  /**< Level 5 Debug Message        */

    MSNGR_DEBUG_LV1_BANNER  = 31,  /**< Level 1 Debug Banner         */
    MSNGR_DEBUG_LV2_BANNER  = 32,  /**< Level 2 Debug Banner         */
    MSNGR_DEBUG_LV3_BANNER  = 33,  /**< Level 3 Debug Banner         */
    MSNGR_DEBUG_LV4_BANNER  = 34,  /**< Level 4 Debug Banner         */
    MSNGR_DEBUG_LV5_BANNER  = 35,  /**< Level 5 Debug Banner         */

    MSNGR_PROVENANCE_LV1    = 41,  /**< Level 1 Provenance Message   */
    MSNGR_PROVENANCE_LV2    = 42,  /**< Level 2 Provenance Message   */
    MSNGR_PROVENANCE_LV3    = 43,  /**< Level 3 Provenance Message   */
    MSNGR_PROVENANCE_LV4    = 44,  /**< Level 4 Provenance Message   */
    MSNGR_PROVENANCE_LV5    = 45   /**< Level 5 Provenance Message   */

} MessageType;

/*******************************************************************************
*  Macros
*/

/**
 *  Specify that the message is specified as an array of strimgs.
 */
#define MSNGR_MESSAGE_BLOCK "MSNGR_MESSAGE_BLOCK"

/**
 *  Log Message.
 */
#define LOG(sender, ...) \
msngr_send(sender, __func__, __FILE__, __LINE__, MSNGR_LOG, __VA_ARGS__)

/**
 *  Warning Log and Mail Message.
 */
#define WARNING(sender, ...) \
msngr_send(sender, __func__, __FILE__, __LINE__, MSNGR_WARNING, __VA_ARGS__)

/**
 *  Error Log and Mail Message.
 */
#define ERROR(sender, ...) \
msngr_send(sender, __func__, __FILE__, __LINE__, MSNGR_ERROR, __VA_ARGS__)

/**
 *  Maintainer Mail Message.
 */
#define MAINTAINER_MAIL(sender, ...) \
msngr_send(sender, __func__, __FILE__, __LINE__, MSNGR_MAINTAINER, __VA_ARGS__)

/**
 *  Debug Level 1 Message.
 */
#define DEBUG_LV1(sender, ...) \
if (msngr_debug_level || msngr_provenance_level) msngr_send( \
    sender, __func__, __FILE__, __LINE__, MSNGR_DEBUG_LV1, __VA_ARGS__)

/**
 *  Debug Level 2 Message.
 */
#define DEBUG_LV2(sender, ...) \
if (msngr_debug_level > 1 || msngr_provenance_level > 1) msngr_send( \
    sender, __func__, __FILE__, __LINE__, MSNGR_DEBUG_LV2, __VA_ARGS__)

/**
 *  Debug Level 3 Message.
 */
#define DEBUG_LV3(sender, ...) \
if (msngr_debug_level > 2 || msngr_provenance_level > 2) msngr_send( \
    sender, __func__, __FILE__, __LINE__, MSNGR_DEBUG_LV3, __VA_ARGS__)

/**
 *  Debug Level 4 Message.
 */
#define DEBUG_LV4(sender, ...) \
if (msngr_debug_level > 3 || msngr_provenance_level > 3) msngr_send( \
    sender, __func__, __FILE__, __LINE__, MSNGR_DEBUG_LV4, __VA_ARGS__)

/**
 *  Debug Level 5 Message.
 */
#define DEBUG_LV5(sender, ...) \
if (msngr_debug_level > 4 || msngr_provenance_level > 4) msngr_send( \
    sender, __func__, __FILE__, __LINE__, MSNGR_DEBUG_LV5, __VA_ARGS__)

/**
 *  Debug Level 1 Banner.
 */
#define DEBUG_LV1_BANNER(sender, ...) \
if (msngr_debug_level || msngr_provenance_level) msngr_send( \
    sender, __func__, __FILE__, __LINE__, MSNGR_DEBUG_LV1_BANNER, __VA_ARGS__)

/**
 *  Debug Level 2 Banner.
 */
#define DEBUG_LV2_BANNER(sender, ...) \
if (msngr_debug_level > 1 || msngr_provenance_level > 1) msngr_send( \
    sender, __func__, __FILE__, __LINE__, MSNGR_DEBUG_LV2_BANNER, __VA_ARGS__)

/**
 *  Debug Level 3 Banner.
 */
#define DEBUG_LV3_BANNER(sender, ...) \
if (msngr_debug_level > 2 || msngr_provenance_level > 2) msngr_send( \
    sender, __func__, __FILE__, __LINE__, MSNGR_DEBUG_LV3_BANNER, __VA_ARGS__)

/**
 *  Debug Level 4 Banner.
 */
#define DEBUG_LV4_BANNER(sender, ...) \
if (msngr_debug_level > 3 || msngr_provenance_level > 3) msngr_send( \
    sender, __func__, __FILE__, __LINE__, MSNGR_DEBUG_LV4_BANNER, __VA_ARGS__)

/**
 *  Debug Level 5 Banner.
 */
#define DEBUG_LV5_BANNER(sender, ...) \
if (msngr_debug_level > 4 || msngr_provenance_level > 4) msngr_send( \
    sender, __func__, __FILE__, __LINE__, MSNGR_DEBUG_LV5_BANNER, __VA_ARGS__)

/**
 *  Provenance Level 1 Message.
 */
#define PROVENANCE_LV1(sender, ...) \
if (msngr_debug_level || msngr_provenance_level) msngr_send( \
    sender, __func__, __FILE__, __LINE__, MSNGR_PROVENANCE_LV1, __VA_ARGS__)

/**
 *  Provenance Level 2 Message.
 */
#define PROVENANCE_LV2(sender, ...) \
if (msngr_debug_level > 1 || msngr_provenance_level > 1) msngr_send( \
    sender, __func__, __FILE__, __LINE__, MSNGR_PROVENANCE_LV2, __VA_ARGS__)

/**
 *  Provenance Level 3 Message.
 */
#define PROVENANCE_LV3(sender, ...) \
if (msngr_debug_level > 2 || msngr_provenance_level > 2) msngr_send( \
    sender, __func__, __FILE__, __LINE__, MSNGR_PROVENANCE_LV3, __VA_ARGS__)

/**
 *  Provenance Level 4 Message.
 */
#define PROVENANCE_LV4(sender, ...) \
if (msngr_debug_level > 3 || msngr_provenance_level > 3) msngr_send( \
    sender, __func__, __FILE__, __LINE__, MSNGR_PROVENANCE_LV4, __VA_ARGS__)

/**
 *  Provenance Level 5 Message.
 */
#define PROVENANCE_LV5(sender, ...) \
if (msngr_debug_level > 4 || msngr_provenance_level > 4) msngr_send( \
    sender, __func__, __FILE__, __LINE__, MSNGR_PROVENANCE_LV5, __VA_ARGS__)

/*@}*/

/*******************************************************************************
*  Messenger Functions
*/

const char *msngr_lib_version(void);

int     msngr_init_log(
            const char *path,
            const char *name,
            int         flags,
            size_t      errlen,
            char       *errstr);

int     msngr_init_mail(
            MessageType  type,
            const char  *from,
            const char  *to,
            const char  *cc,
            const char  *subject,
            int          flags,
            size_t       errlen,
            char        *errstr);

int     msngr_init_provenance(
            const char *path,
            const char *name,
            int         flags,
            size_t      errlen,
            char       *errstr);

void    msngr_finish(void);

void    msngr_finish_log(void);
void    msngr_finish_mail(MessageType type);
void    msngr_finish_provenance(void);

void    msngr_flush_log_error(void);
void    msngr_flush_mail_errors(void);
void    msngr_flush_provenance_error(void);

void    msngr_send(
            const char  *sender,
            const char  *func,
            const char  *file,
            int          line,
            MessageType  type,
            const char  *format, ...);

void    msngr_vsend(
            const char  *sender,
            const char  *func,
            const char  *file,
            int          line,
            MessageType  type,
            const char  *format,
            va_list      args);

int    msngr_set_debug_level(int level);
int    msngr_set_provenance_level(int level);

struct LogFile *msngr_get_log_file(void);
struct Mail    *msngr_get_mail(MessageType type);

/*******************************************************************************
*  Lock Files
*/

/** maximum length of a lockfile error message */
#define MAX_LOCKFILE_ERROR  (PATH_MAX + 128)

int     lockfile_create(
            const char *path,
            const char *name,
            int         flags,
            size_t      errlen,
            char       *errstr);

int     lockfile_remove(
            const char *path,
            const char *name,
            size_t      errlen,
            char       *errstr);

/*******************************************************************************
*  Log Files
*/

#define LOG_TAGS  0x1 /**< print opened and closed log message tags      */
#define LOG_STATS 0x2 /**< log process stats before closing the log file */
#define LOG_LOCKF 0x4 /**< place advisory lock on log file using lockf() */

/** maximum length of a log error message */
#define MAX_LOG_ERROR  (PATH_MAX + 128)

/**
 *  Log File Structure.
 */
typedef struct LogFile
{
    char   *path;       /**< path to the directory the log file is in  */
    char   *name;       /**< log file name                             */
    char   *full_path;  /**< full path to the log file                 */
    FILE   *fp;         /**< pointer to the open log file              */
    int     flags;      /**< control flags                             */
    time_t  open_time;  /**< The time the log file was opened          */
    char   *errstr;     /**< buffer used for error messages            */

} LogFile;

LogFile *log_open(
            const char *path,
            const char *name,
            int         flags,
            size_t      errlen,
            char       *errstr);

int     log_close(
            LogFile    *log,
            size_t      errlen,
            char       *errstr);

int     log_printf(
            LogFile    *log,
            const char *line_tag,
            const char *format, ...);

int     log_vprintf(
            LogFile    *log,
            const char *line_tag,
            const char *format,
            va_list     args);

void        log_clear_error(LogFile *log);
const char *log_get_error(LogFile *log);

/*******************************************************************************
*  Mail Messages Files
*/

#define MAIL_ADD_NEWLINE 0x1 /**< flag to add an extra newline after messages */

/** maximum length of a mail error message */
#define MAX_MAIL_ERROR  256

/**
 *  Mail Message Structure.
 */
typedef struct Mail
{
    char   *from;        /**< who the message is from            */
    char   *to;          /**< comma delimited list of recipients */
    char   *cc;          /**< comma delimited carbon copy list   */
    char   *subject;     /**< message subject                    */
    int     flags;       /**< control flags                      */
    size_t  length;      /**< length of the mail message         */
    char   *body;        /**< body of the mail message           */
    size_t  nalloced;    /**< allocated length of the mail body  */
    char   *errstr;      /**< buffer used for error messages     */

} Mail;

Mail   *mail_create(
            const char  *from,
            const char  *to,
            const char  *cc,
            const char  *subject,
            int          flags,
            size_t       errlen,
            char        *errstr);

void    mail_destroy(Mail *mail);

void    mail_printf(
            Mail       *mail,
            const char *format, ...);

void    mail_vprintf(
            Mail       *mail,
            const char *format,
            va_list     args);

int     mail_send(Mail *mail);

void    mail_set_flags(Mail *mail, int flags);
void    mail_unset_flags(Mail *mail, int flags);


void        mail_clear_error(Mail *mail);
const char *mail_get_error(Mail *mail);

/*******************************************************************************
*  Process Stats
*/

#define MAX_STATS_ERROR 256

/**
 *  Process Stats Structure.
 */
typedef struct ProcStats {

    char          exe_name[128];  /**< executable file name                 */
    unsigned int  image_size;     /**< process image size        (Kbytes)   */
    unsigned int  rss_size;       /**< process resident set size (Kbytes)   */
    unsigned long total_rw_io;    /**< total read/write IO       (bytes)    */
    double        cpu_time;       /**< total CPU usage           (seconds)  */
    double        run_time;       /**< process run time          (seconds)  */

    char errstr[MAX_STATS_ERROR]; /**< buffer used for error messages       */

} ProcStats;

ProcStats  *procstats_get(void);
void        procstats_print(FILE *fp);

/*******************************************************************************
*  Utility Functions
*/

char   *msngr_copy_string(
            const char *string);

char   *msngr_create_string(
            const char *format, ...);

char   *msngr_format_time(
            time_t  secs1970,
            char   *string);

char   *msngr_format_va_list(
            const char *format,
            va_list     args);

time_t  msngr_get_process_start_time(
            pid_t pid);

int     msngr_make_path(
            const char *path,
            mode_t      mode,
            size_t      errlen,
            char       *errstr);

int     msngr_vprintf(
            const char *format,
            va_list     args);

int     msngr_vfprintf(
            FILE       *stream,
            const char *format,
            va_list     args);

int     msngr_vsprintf(
            char       *string,
            const char *format,
            va_list     args);

int     msngr_vsnprintf(
            char       *string,
            size_t      nbytes,
            const char *format,
            va_list     args);

#endif /* _MSNGR_H_ */
