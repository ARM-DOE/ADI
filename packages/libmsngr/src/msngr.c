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
*    $Revision: 63480 $
*    $Author: ermold $
*    $Date: 2015-08-26 23:09:36 +0000 (Wed, 26 Aug 2015) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file msngr.c
 *  Messanger Functions.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "msngr.h"

int msngr_debug_level      = 0;
int msngr_provenance_level = 0;

/**
 *  @defgroup MESSENGER Messenger
 */
/*@{*/

/*******************************************************************************
 *  Private Data and Functions
 */
/** @privatesection */

static LogFile *gLog;       /**< PRIVATE: Internal LogFile structure        */
static LogFile *gProvLog;   /**< PRIVATE: Internal Provenance Log File      */
static Mail    *gMail[3];   /**< PRIVATE: Internal array of Mail structures */

/**
 *  PRIVATE: Internal Debug structure.
 */
static struct
{
    int          fl_width;      /**< width of the $file:$line line prefix    */
    char         fl_format[16]; /**< format string used to print $file:$line */
    MessageType  prev_type;     /**< previous message type                   */
    const char  *footer;        /**< message footer                          */

} gDebug;

/**
 *  PRIVATE: Get the string name of a MessageType.
 *
 *  @param  type - mesage type
 *
 *  @return name of the message type
 */
static const char *_message_type_to_name(MessageType type)
{
    const char *name;

    switch (type) {

        case MSNGR_LOG:
            name = "Log";
            break;
        case MSNGR_WARNING:
            name = "Warning";
            break;
        case MSNGR_ERROR:
            name = "Error";
            break;
        case MSNGR_MAINTAINER:
            name = "Maintainer";
            break;
        case MSNGR_DEBUG_LV1:
            name = "Debug Level 1";
            break;
        case MSNGR_DEBUG_LV2:
            name = "Debug Level 2";
            break;
        case MSNGR_DEBUG_LV3:
            name = "Debug Level 3";
            break;
        case MSNGR_DEBUG_LV4:
            name = "Debug Level 4";
            break;
        case MSNGR_DEBUG_LV5:
            name = "Debug Level 5";
            break;
        case MSNGR_DEBUG_LV1_BANNER:
            name = "Debug Level 1 Banner";
            break;
        case MSNGR_DEBUG_LV2_BANNER:
            name = "Debug Level 2 Banner";
            break;
        case MSNGR_DEBUG_LV3_BANNER:
            name = "Debug Level 3 Banner";
            break;
        case MSNGR_DEBUG_LV4_BANNER:
            name = "Debug Level 4 Banner";
            break;
        case MSNGR_DEBUG_LV5_BANNER:
            name = "Debug Level 5 Banner";
            break;
        case MSNGR_PROVENANCE_LV1:
            name = "Provenance Level 1";
            break;
        case MSNGR_PROVENANCE_LV2:
            name = "Provenance Level 2";
            break;
        case MSNGR_PROVENANCE_LV3:
            name = "Provenance Level 3";
            break;
        case MSNGR_PROVENANCE_LV4:
            name = "Provenance Level 4";
            break;
        case MSNGR_PROVENANCE_LV5:
            name = "Provenance Level 5";
            break;
        default:
            name = "Invalid";
            break;
    }

    return(name);
}

static void _debug_print_message(char *message)
{
    char *linep;
    char *nl;
    int   indent;

    indent = 0;

    for (linep = message; (nl = strchr(linep, '\n')); linep = nl + 1) {

        *nl = '\0';

        if (indent) {
            fprintf(stdout, gDebug.fl_format, "");
        }

        fprintf(stdout, "%s\n", linep);

        indent = 1;
    }

    if (*linep != '\0') {

        if (indent) {
            fprintf(stdout, gDebug.fl_format, "");
        }

        fprintf(stdout, "%s\n", linep);
    }
}

/**
 *  PRIVATE: Print a debug message to stdout.
 *
 *  This function prints debug information to stdout if the
 *  debug level of the messaage is less than or equal to the
 *  current debug level.
 *
 *  An array of strings can be passed into this function by
 *  specifying MSNGR_MESSAGE_BLOCK for the format argument.
 *  In this case the first argument in the args list must be
 *  a pointer to a NULL terminted array of strings (char **).
 *
 *  @param  file   - the source file the message came from
 *  @param  line   - the line number in the source file
 *  @param  level  - debug level of the message
 *  @param  type   - mesage type
 *  @param  format - format string (see printf)
 *  @param  args   - arguments for the format string
 */
static void _debug_vprintf(
    const char  *file,
    int          line,
    int          level,
    MessageType  type,
    const char  *format,
    va_list      args)
{
    char     file_line[64];
    char    *header;
    char    *message;
    size_t   length;
    int      print_newline;
    char   **msg_block;
    va_list  args_copy;
    int      i;

    if (level > msngr_debug_level) {
        return;
    }

    snprintf(file_line, 63, "%s:%d", file, line);

    length = strlen(file_line);

    if (gDebug.fl_width < (int)length) {

        gDebug.fl_width = length + 4;

        if (gDebug.fl_width < 32) {
            gDebug.fl_width = 32;
        }

        sprintf(gDebug.fl_format, "%%-%ds", gDebug.fl_width);
    }

    if (type != gDebug.prev_type) {

        print_newline = 0;

        if (gDebug.footer) {
            fprintf(stdout, gDebug.fl_format, "");
            fprintf(stdout, "%s", gDebug.footer);
            gDebug.footer = (char *)NULL;
            print_newline = 1;
        }

        switch (type) {

            case MSNGR_LOG:
                header        = "----- LOG MESSAGE ---------------------\n";
                gDebug.footer = "----- END LOG MESSAGE -----------------\n";
                break;
            case MSNGR_WARNING:
                header        = "----- WARNING MESSAGE -----------------\n";
                gDebug.footer = "----- END WARNING MESSAGE -------------\n";
                break;
            case MSNGR_ERROR:
                header        = "----- ERROR MESSAGE -------------------\n";
                gDebug.footer = "----- END ERROR MESSAGE ---------------\n";
                break;
            case MSNGR_MAINTAINER:
                header        = "----- MAINTAINER MESSAGE ---------------\n";
                gDebug.footer = "----- END MAINTAINER MESSAGE -----------\n";
                break;
            case MSNGR_PROVENANCE_LV1:
                header        = "----- PROVENANCE MESSAGE (LEVEL 1) -----\n";
                gDebug.footer = "----- END PROVENANCE MESSAGE -----------\n";
                break;
            case MSNGR_PROVENANCE_LV2:
                header        = "----- PROVENANCE MESSAGE (LEVEL 2) -----\n";
                gDebug.footer = "----- END PROVENANCE MESSAGE -----------\n";
                break;
            case MSNGR_PROVENANCE_LV3:
                header        = "----- PROVENANCE MESSAGE (LEVEL 3) -----\n";
                gDebug.footer = "----- END PROVENANCE MESSAGE -----------\n";
                break;
            case MSNGR_PROVENANCE_LV4:
                header        = "----- PROVENANCE MESSAGE (LEVEL 4) -----\n";
                gDebug.footer = "----- END PROVENANCE MESSAGE -----------\n";
                break;
            case MSNGR_PROVENANCE_LV5:
                header        = "----- PROVENANCE MESSAGE (LEVEL 5) -----\n";
                gDebug.footer = "----- END PROVENANCE MESSAGE -----------\n";
                break;
            default:
                header = (char *)NULL;
        }

        if (header) {
            fprintf(stdout, "\n");
            fprintf(stdout, gDebug.fl_format, "");
            fprintf(stdout, "%s", header);
        }
        else if (print_newline) {
            fprintf(stdout, "\n");
        }

        gDebug.prev_type = type;
    }

    fprintf(stdout, gDebug.fl_format, file_line);

    if (strcmp(format, "MSNGR_MESSAGE_BLOCK") == 0) {

        va_copy(args_copy, args);
        msg_block = va_arg(args_copy, char **);
        va_end(args_copy);

        for (i = 0; msg_block[i] != (char *)NULL; i++) {
            _debug_print_message(msg_block[i]);
        }
    }
    else {

        message = msngr_format_va_list(format, args);

        if (message) {
            _debug_print_message(message);
            free(message);
        }
        else {
            fprintf(stdout,
                "Memory allocation error formating debug message\n");
        }
    }

    fflush(stdout);
}

static void _provenance_print_message(
    const char  *sender,
    const char  *func,
    const char  *file,
    int          line,
    MessageType  type,
    const char  *message)
{
    const char *cp     = message;
    size_t      length = strlen(message);

    /* Ignore empty messages */

    for (; isspace(*cp) && *cp != '\0'; cp++);

    if (*cp == '\0') return;

    /* Print header line if the message doesn’t start with a space character */

    if (!isspace(*message)) {
        fprintf(gProvLog->fp, "\n%s->%s->%s:%d->'%s'\n",
            sender, func, file, line, _message_type_to_name(type));
    }

    /* Print message */

    fprintf(gProvLog->fp, "%s", message);

    if (message[length-1] != '\n') {
        fprintf(gProvLog->fp, "\n");
    }
}

/**
 *  PRIVATE: Print a message to the provenance log.
 *
 *  This function prints messages to the provenance log if the
 *  provenance level of the messaage is less than or equal to the
 *  current provenance level.
 *
 *  An array of strings can be passed into this function by
 *  specifying MSNGR_MESSAGE_BLOCK for the format argument.
 *  In this case the first argument in the args list must be
 *  a pointer to a NULL terminted array of strings (char **).
 *
 *  @param  sender - the name of the library or executable sending the message
 *  @param  func   - the name of the function sending the message
 *  @param  file   - the source file the message came from
 *  @param  line   - the line number in the source file
 *  @param  level  - debug level of the message
 *  @param  type   - mesage type
 *  @param  format - format string (see printf)
 *  @param  args   - arguments for the format string
 */
static void _provenance_vprintf(
    const char  *sender,
    const char  *func,
    const char  *file,
    int          line,
    int          level,
    MessageType  type,
    const char  *format,
    va_list      args)
{
    char   **msg_block;
    char    *message;
    va_list  args_copy;
    int      i;

    if (level > msngr_provenance_level || !gProvLog) {
        return;
    }

    if (strcmp(format, "MSNGR_MESSAGE_BLOCK") == 0) {

        va_copy(args_copy, args);
        msg_block = va_arg(args_copy, char **);
        va_end(args_copy);

        for (i = 0; msg_block[i] != (char *)NULL; i++) {
            _provenance_print_message(
                sender, func, file, line, type, msg_block[i]);
        }
    }
    else {

        message = msngr_format_va_list(format, args);

        if (message) {
            _provenance_print_message(
                sender, func, file, line, type, message);
            free(message);
        }
        else {
            fprintf(stdout,
                "Memory allocation error formating provenance message\n");
        }
    }

    fflush(gProvLog->fp);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Initialize the log file.
 *
 *  If a log file is already open, it will be closed and the new
 *  log file will take its place.
 *
 *  See log_open() for details.
 *
 *  @param  path   - path to the directory to open the log file in
 *  @param  name   - log file name
 *  @param  flags  - control flags
 *  @param  errlen - length of the error message buffer
 *  @param  errstr - output: error message
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int msngr_init_log(
    const char *path,
    const char *name,
    int         flags,
    size_t      errlen,
    char       *errstr)
{
    if (gLog) {
        msngr_finish_log();
    }

    gLog = log_open(path, name, flags, errlen, errstr);

    return((gLog) ? 1 : 0 );
}

/**
 *  Initialize a mail message.
 *
 *  See mail_create() for details.
 *
 *  @param  type    - message type
 *  @param  from    - who the message is from
 *  @param  to      - comma delimited list of recipients
 *  @param  cc      - comma delimited carbon copy list
 *  @param  subject - message subject
 *  @param  flags   - control flags
 *  @param  errlen  - length of the error message buffer
 *  @param  errstr  - output: error message
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int msngr_init_mail(
    MessageType  type,
    const char  *from,
    const char  *to,
    const char  *cc,
    const char  *subject,
    int          flags,
    size_t       errlen,
    char        *errstr)
{
    int index;

    index = type - MSNGR_ERROR;

    if (index < 0 || index > 2) {

        snprintf(errstr, errlen,
            "Could not initialize %s Mail message\n"
            " -> invalid mail message type\n",
            _message_type_to_name(type));

        return(0);
    }

    if (gMail[index]) {
        msngr_finish_mail(type);
    }

    gMail[index] = mail_create(from, to, cc, subject, flags, errlen, errstr);

    return((gMail[index]) ? 1 : 0 );
}

/**
 *  Initialize the provenance log.
 *
 *  If a provenance log file is already open, it will be
 *  closed and the new log file will take its place.
 *
 *  See log_open() for details.
 *
 *  @param  path   - path to the directory to open the log file in
 *  @param  name   - log file name
 *  @param  flags  - control flags
 *  @param  errlen - length of the error message buffer
 *  @param  errstr - output: error message
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int msngr_init_provenance(
    const char *path,
    const char *name,
    int         flags,
    size_t      errlen,
    char       *errstr)
{
    if (gProvLog) {
        msngr_finish_provenance();
    }

    gProvLog = log_open(path, name, flags, errlen, errstr);

    return((gProvLog) ? 1 : 0 );
}

/**
 *  Finish all messenger processes.
 *
 *  This function will:
 *
 *    - send all mail messages
 *    - close the log file
 *    - cleanup all allocated memory
 */
void msngr_finish(void)
{
    MessageType type;
    int         i;

    /* Send the mail messages */

    for (i = 2; i > -1; i--) {
        type = MSNGR_ERROR + i;
        msngr_finish_mail(type);
    }

    /* Close the provenance and log files */

    msngr_finish_log();
    msngr_finish_provenance();
}

/**
 *  Finish and close the log file.
 */
void msngr_finish_log(void)
{
    char errstr[MAX_LOG_ERROR];

    /* Flush any mail and/or log errors */

    msngr_flush_mail_errors();
    msngr_flush_log_error();
    msngr_flush_provenance_error();

    /* Close the log file */

    if (gLog) {
        if (!log_close(gLog, MAX_LOG_ERROR, errstr)) {
            ERROR( MSNGR_LIB_NAME, "%s", errstr);
        }

        gLog = (LogFile *)NULL;
    }
}

/**
 *  Finish and send a mail message.
 *
 *  @param  type - message type
 */
void msngr_finish_mail(
    MessageType  type)
{
    int index;

    index = type - MSNGR_ERROR;

    if (index < 0 || index > 2) {
        return;
    }

    if (gMail[index]) {

        if (type == MSNGR_ERROR) {
            msngr_flush_mail_errors();
            msngr_flush_log_error();
        }

        /* Send the mail messages */

        if (!mail_send(gMail[index])) {
            ERROR( MSNGR_LIB_NAME, "%s", mail_get_error(gMail[index]));
        }

        /* Cleanup Mail structure */

        mail_destroy(gMail[index]);

        gMail[index] = (Mail *)NULL;
    }
}

/**
 *  Finish and close the provenance log file.
 */
void msngr_finish_provenance(void)
{
    char errstr[MAX_LOG_ERROR];

    /* Flush any mail and/or log errors */

    msngr_flush_mail_errors();
    msngr_flush_log_error();
    msngr_flush_provenance_error();

    /* Close the log file */

    if (gProvLog) {
        if (!log_close(gProvLog, MAX_LOG_ERROR, errstr)) {
            ERROR( MSNGR_LIB_NAME, "%s", errstr);
        }

        gProvLog = (LogFile *)NULL;
    }
}

/**
 *  Flush the log error message.
 */
void msngr_flush_log_error(void)
{
    const char *last_error;

    /* Check for log errors */

    if (gLog) {
        last_error = log_get_error(gLog);
        if (last_error) {
            ERROR( MSNGR_LIB_NAME, "%s", last_error);
            log_clear_error(gLog);
        }
    }
}

/**
 *  Flush all mail error messages.
 */
void msngr_flush_mail_errors(void)
{
    const char *last_error;
    int         i;

    for (i = 2; i > -1; i--) {

        if (gMail[i]) {

            last_error = mail_get_error(gMail[i]);
            if (last_error) {
                ERROR( MSNGR_LIB_NAME, "%s", last_error);
                mail_clear_error(gMail[i]);
            }
        }
    }
}

/**
 *  Flush the provenance log error message.
 */
void msngr_flush_provenance_error(void)
{
    const char *last_error;

    /* Check for provenance log errors */

    if (gProvLog) {
        last_error = log_get_error(gProvLog);
        if (last_error) {
            ERROR( MSNGR_LIB_NAME, "%s", last_error);
            log_clear_error(gProvLog);
        }
    }
}

/**
 *  Message handling function.
 *
 *  This is the function used by the messenger macros.
 *
 *  An array of strings can be passed into this function by
 *  specifying MSNGR_MESSAGE_BLOCK for the format argument.
 *  In this case the next argument after format must be a
 *  pointer to a NULL terminted array of strings (char **).
 *
 *  @param  sender - the name of the library or executable sending the message
 *  @param  func   - the name of the function sending the message
 *  @param  file   - the source file the message came from
 *  @param  line   - the line number in the source file
 *  @param  type   - mesage type
 *  @param  format - format string (see printf)
 *  @param  ...    - arguments for the format string
 */
void msngr_send(
    const char  *sender,
    const char  *func,
    const char  *file,
    int          line,
    MessageType  type,
    const char  *format, ...)
{
    va_list args;

    va_start(args, format);
    msngr_vsend(sender, func, file, line, type, format, args);
    va_end(args);
}

/**
 *  Message handling function.
 *
 *  An array of strings can be passed into this function by
 *  specifying MSNGR_MESSAGE_BLOCK for the format argument.
 *  In this case the first argument in the args list must be
 *  a pointer to a NULL terminted array of strings (char **).
 *
 *  @param  sender - the name of the library or executable sending the message
 *  @param  func   - the name of the function sending the message
 *  @param  file   - the source file the message came from
 *  @param  line   - the line number in the source file
 *  @param  type   - mesage type
 *  @param  format - format string (see printf)
 *  @param  args   - arguments for the format string
 */
void msngr_vsend(
    const char  *sender,
    const char  *func,
    const char  *file,
    int          line,
    MessageType  type,
    const char  *format,
    va_list      args)
{
    int debug_banner;
    int msg_debug_level;
    int msg_prov_level;

    if (!sender) sender = "null";
    if (!func)   func   = "null";
    if (!file)   file   = "null";

    /* Log and Mail messages */

    switch (type) {

        case MSNGR_LOG:

            if (gLog) {
                log_vprintf(gLog, NULL, format, args);
            }
            else {
                msngr_vfprintf(stdout, format, args);
            }

            break;

        case MSNGR_ERROR:

            if (gLog) {
                log_vprintf(gLog, "ERROR: ", format, args);
            }
            else {
                fprintf(stderr, "ERROR: ");
                msngr_vfprintf(stderr, format, args);
            }

            if (gMail[0]) {
                mail_vprintf(gMail[0], format, args);
            }

            break;

        case MSNGR_WARNING:

            if (gLog) {
                log_vprintf(gLog, "WARNING: ", format, args);
            }
            else {
                fprintf(stdout, "WARNING: ");
                msngr_vfprintf(stdout, format, args);
            }

            if (gMail[1]) {
                mail_vprintf(gMail[1], format, args);
            }

            break;

        case MSNGR_MAINTAINER:

            if (gMail[2]) {
                mail_vprintf(gMail[2], format, args);
            }

            break;

        default:
            break;
    }

    /* Debug messages */

    if (msngr_debug_level) {

        debug_banner = 0;

        switch (type) {

            case MSNGR_DEBUG_LV1:
            case MSNGR_DEBUG_LV2:
            case MSNGR_DEBUG_LV3:
            case MSNGR_DEBUG_LV4:
            case MSNGR_DEBUG_LV5:
                msg_debug_level = type - MSNGR_DEBUG_LV1 + 1;
                break;

            case MSNGR_DEBUG_LV1_BANNER:
            case MSNGR_DEBUG_LV2_BANNER:
            case MSNGR_DEBUG_LV3_BANNER:
            case MSNGR_DEBUG_LV4_BANNER:
            case MSNGR_DEBUG_LV5_BANNER:
                msg_debug_level = type - MSNGR_DEBUG_LV1_BANNER + 1;
                debug_banner    = 1;
                break;

            case MSNGR_PROVENANCE_LV1:
            case MSNGR_PROVENANCE_LV2:
            case MSNGR_PROVENANCE_LV3:
            case MSNGR_PROVENANCE_LV4:
            case MSNGR_PROVENANCE_LV5:
                msg_debug_level = type - MSNGR_PROVENANCE_LV1 + 1;
                break;

            default:
                msg_debug_level = 1;
        }

        if (debug_banner) {

            if (gDebug.footer) {
                fprintf(stdout, gDebug.fl_format, "");
                fprintf(stdout, "%s", gDebug.footer);
                gDebug.footer = (char *)NULL;
                fprintf(stdout, "\n");
            }

            fprintf(stdout,
                "\n================================================================================\n");
        }

        if (msg_debug_level <= msngr_debug_level) {
            _debug_vprintf(file, line, msg_debug_level, type, format, args);
        }

        if (debug_banner) {
            fprintf(stdout,
                "================================================================================\n\n");
            fflush(stdout);
        }
    }

    /* Provenance messages */

    if (gProvLog && msngr_provenance_level) {

        switch (type) {

            case MSNGR_DEBUG_LV1:
            case MSNGR_DEBUG_LV2:
            case MSNGR_DEBUG_LV3:
            case MSNGR_DEBUG_LV4:
            case MSNGR_DEBUG_LV5:
                msg_prov_level = type - MSNGR_DEBUG_LV1 + 1;
                break;

            case MSNGR_PROVENANCE_LV1:
            case MSNGR_PROVENANCE_LV2:
            case MSNGR_PROVENANCE_LV3:
            case MSNGR_PROVENANCE_LV4:
            case MSNGR_PROVENANCE_LV5:
                msg_prov_level = type - MSNGR_PROVENANCE_LV1 + 1;
                break;

            default:
                msg_prov_level = 1;
        }

        if (msg_prov_level <= msngr_provenance_level) {

            _provenance_vprintf(
                sender, func, file, line, msg_prov_level, type, format, args);
        }
    }
}

/**
 *  Set the debug level.
 *
 *  @param  level - debug level
 *
 *  @return  previous debug level
 */
int msngr_set_debug_level(int level)
{
    int prev_debug_level = msngr_debug_level;

    if (msngr_debug_level != level) {

        if (!msngr_debug_level) {
            msngr_debug_level = level;
        }

        DEBUG_LV1( MSNGR_LIB_NAME,
            "Changing debug level from %d to %d\n",
            prev_debug_level, level);

        msngr_debug_level = level;
    }

    return(prev_debug_level);
}

/**
 *  Set the provenance level.
 *
 *  @param  level - provenance message level
 *
 *  @return  previous provenance level
 */
int msngr_set_provenance_level(int level)
{
    int prev_prov_level = msngr_provenance_level;

    if (msngr_provenance_level != level) {

        if (!msngr_provenance_level) {
            msngr_provenance_level = level;
        }

        PROVENANCE_LV1( MSNGR_LIB_NAME,
            "Changing provenance level from %d to %d\n",
            prev_prov_level, level);

        msngr_provenance_level = level;
    }

    return(prev_prov_level);
}

/**
 *  Get the LogFile structure used by the messengr functions.
 *
 *  @return
 *    - pointer to the internal log file structure
 *    - NULL if the log file has not been initialized
 */
LogFile *msngr_get_log_file(void)
{
    return(gLog);
}

/**
 *  Get a Mail structure used by the messengr functions.
 *
 *  @param  type - mesage type
 *
 *  @return
 *    - pointer to the internal mail structure
 *    - NULL if the mail message has not been initialized
 */
Mail *msngr_get_mail(MessageType type)
{
    switch (type) {

        case MSNGR_ERROR:

            return(gMail[0]);

        case MSNGR_WARNING:

            return(gMail[1]);

        case MSNGR_MAINTAINER:

            return(gMail[2]);

        default:
            break;
    }

    return((Mail *)NULL);
}

/*@}*/
