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
*    $Revision: 63481 $
*    $Author: ermold $
*    $Date: 2015-08-26 23:09:52 +0000 (Wed, 26 Aug 2015) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file msngr_mail.c
 *  Mail Functions.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "msngr.h"

/** Mail body growth size. */
#define MAIL_BODY_GROWTH_SIZE 1024

/** Amount of allocated space to reserve at the end of the mail body. */
#define MAIL_BODY_RESERVED_SIZE 80

/**
 *  @defgroup MAIL_MESSAGES Mail Messages
 */
/*@{*/

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  PRIVATE: Full path to the sendmail program.
 */
static char gSendMailPath[64];

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Create a new mail message.
 *
 *  The space pointed to by errstr should be large enough to
 *  hold MAX_MAIL_ERROR bytes. Any less than that and the error
 *  message could be truncated.
 *
 *  If errlen is 0 then no error message is written to errstr
 *  and errstr can be NULL.
 *
 *  If the sendmail program can not be found the error message
 *  will start with, "Could not find sendmail".
 *
 *  @param  from    - who the message is from
 *  @param  to      - comma delimited list of recipients
 *  @param  cc      - comma delimited carbon copy list
 *  @param  subject - message subject
 *  @param  flags   - control flags
 *  @param  errlen  - length of the error message buffer
 *  @param  errstr  - output: error message
 *
 *  Control Flags:
 *
 *    - MAIL_ADD_NEWLINE - Add a newline after every mail message added.
 *
 *  @return
 *    - pointer to the new Mail message
 *    - NULL if:
 *        - the sendmail program could not be found
 *        - a memory allocation error occurred
 */
Mail *mail_create(
    const char  *from,
    const char  *to,
    const char  *cc,
    const char  *subject,
    int          flags,
    size_t      errlen,
    char       *errstr)
{
    Mail *mail;
    int   alloc_error;

    /* Find the sendmail program if it has not already been found */

    if (gSendMailPath[0] == '\0') {

        if (access("/usr/sbin/sendmail", X_OK) == 0) {
            strcpy(gSendMailPath, "/usr/sbin/sendmail");
        }
        else if (access("/usr/lib/sendmail", X_OK) == 0) {
            strcpy(gSendMailPath, "/usr/lib/sendmail");
        }
        else {

            snprintf(errstr, errlen,
                "Could not find sendmail in /usr/sbin/sendmail\n");

            return((Mail *)NULL);
        }
    }

    /* Create the Mail structure */

    mail = (Mail *)calloc(1, sizeof(Mail));

    if (!mail) {

        snprintf(errstr, errlen,
            "Could not create mail message: '%s'\n"
            " -> memory allocation error\n", subject);

        return((Mail *)NULL);
    }

    alloc_error = 0;

    if (from) {

        mail->from = msngr_copy_string(from);
        if (!mail->from) {
            alloc_error = 1;
        }
    }

    if (to && !alloc_error) {

        mail->to = msngr_copy_string(to);
        if (!mail->to) {
            alloc_error = 1;
        }
    }

    if (cc && !alloc_error) {

        mail->cc = msngr_copy_string(cc);
        if (!mail->cc) {
            alloc_error = 1;
        }
    }

    if (subject && !alloc_error) {

        mail->subject = msngr_copy_string(subject);
        if (!mail->subject) {
            alloc_error = 1;
        }
    }

    if (!alloc_error) {

        mail->length   = 0;
        mail->nalloced = MAIL_BODY_GROWTH_SIZE;

        mail->body = (char *)calloc(mail->nalloced, sizeof(char));
        if (!mail->body) {
            alloc_error = 1;
        }
    }

    if (!alloc_error) {

        mail->errstr = (char *)calloc(MAX_MAIL_ERROR, sizeof(char));
        if (!mail->errstr) {
            alloc_error = 1;
        }
    }

    if (alloc_error) {

        mail_destroy(mail);

        snprintf(errstr, errlen,
            "Could not create mail message: '%s'\n"
            " -> memory allocation error\n", subject);

        return((Mail *)NULL);
    }

    mail->flags = flags;

    return(mail);
}

/**
 *  Free all memory used by a Mail message.
 *
 *  @param  mail - pointer to the Mail message
 */
void mail_destroy(Mail *mail)
{
    if (mail) {

        if (mail->from)    free(mail->from);
        if (mail->to)      free(mail->to);
        if (mail->cc)      free(mail->cc);
        if (mail->subject) free(mail->subject);
        if (mail->body)    free(mail->body);
        if (mail->errstr)  free(mail->errstr);

        free(mail);
     }
}

/**
 *  Clear the last error message for a Mail message.
 *
 *  @param  mail - pointer to the Mail message
 */
void mail_clear_error(Mail *mail)
{
    if (mail) {
        mail->errstr[0] = '\0';
    }
}

/**
 *  Get the last error message for a Mail message.
 *
 *  @param  mail - pointer to the Mail message
 *
 *  @return
 *    - the last error message that occurred
 *    - NULL if no errors have occurred
 */
const char *mail_get_error(Mail *mail)
{
    if (mail->errstr[0] == '\0') {
        return((const char *)NULL);
    }
    else {
        return((const char *)mail->errstr);
    }
}

/**
 *  Print a message to a mail message.
 *
 *  This function will print a message to a mail body under the
 *  control of the format argument. Alternatively, an array of
 *  strings can be passed into this function by specifying
 *  MSNGR_MESSAGE_BLOCK for the format argument. In this case
 *  the next argument after format must be a pointer to a NULL
 *  terminted array of strings (char **).
 *
 *  This function will print a message to a mail body
 *  under the control of the format argument.
 *
 *  @param  mail   - pointer to the Mail message
 *  @param  format - format string (see printf)
 *  @param  ...    - arguments for the format string
 */
void mail_printf(
    Mail       *mail,
    const char *format, ...)
{
    va_list args;

    va_start(args, format);
    mail_vprintf(mail, format, args);
    va_end(args);
}

/**
 *  Print a message to a mail message.
 *
 *  This function will print a message to a mail body under the
 *  control of the format argument. Alternatively, an array of
 *  strings can be passed into this function by specifying
 *  MSNGR_MESSAGE_BLOCK for the format argument. In this case
 *  the first argument in the args list must be a pointer to
 *  a NULL terminted array of strings (char **).
 *
 *  @param  mail   - pointer to the Mail message
 *  @param  format - format string (see printf)
 *  @param  args   - arguments for the format string
 */
void mail_vprintf(
    Mail       *mail,
    const char *format,
    va_list     args)
{
    int      space_left;
    char    *msg_start;
    char    *msg_end;
    int      msg_length;
    size_t   min_size;
    size_t   new_size;
    char    *new_body;
    char   **msg_block;
    va_list  args_copy;
    int      i;

    /* Calculate the space left in the buffer, the start position
     * in the buffer, and print the message to the buffer if there
     * is enough space left. */

    space_left = mail->nalloced - mail->length - MAIL_BODY_RESERVED_SIZE;
    msg_start  = mail->body + mail->length;

    if (space_left < 0) space_left = 0;

    if (strcmp(format, "MSNGR_MESSAGE_BLOCK") == 0) {

        va_copy(args_copy, args);
        msg_block = va_arg(args_copy, char **);
        va_end(args_copy);

        msg_length = 0;

        for (i = 0; msg_block[i] != (char *)NULL; i++) {
            msg_length += strlen(msg_block[i]);
        }
    }
    else {
        msg_block = (char **)NULL;
        msg_length = msngr_vsnprintf(msg_start, space_left, format, args);
    }

    /* Check if there was enough space left in the buffer */

    if (msg_length >= space_left) {

        /* Increase the size of the mail buffer */

        min_size = mail->length + msg_length + MAIL_BODY_RESERVED_SIZE;
        new_size = mail->nalloced + MAIL_BODY_GROWTH_SIZE;

        while (new_size < min_size) {
            new_size += MAIL_BODY_GROWTH_SIZE;
        }

        new_body = (char *)realloc(mail->body, new_size * sizeof(char));

        /* Check if there was enough memory to resize the buffer */

        if (new_body) {

            /* Print message to the resized buffer */

            mail->nalloced = new_size;
            mail->body     = new_body;

            msg_start  = mail->body + mail->length;

            if (!msg_block) {
                msg_length = msngr_vsprintf(msg_start, format, args);
            }
        }
        else { /* not enough memory to resize buffer */

            /* Flush the mail buffer */

            sprintf(msg_start,
                "Could not increase mail buffer size, "
                "sending mail and flushing buffer.\n\n");

            mail_send(mail);

            /* Recalculate the space left in the buffer
             * and the start position in the buffer */

            space_left = mail->nalloced
                       - mail->length
                       - MAIL_BODY_RESERVED_SIZE;

            msg_start  = mail->body + mail->length;

            if (msg_length < space_left) {
                if (!msg_block) {
                    msg_length = msngr_vsprintf(msg_start, format, args);
                }
            }
            else { /* mail message is lost */
                msg_length = 0;
            }
        }
    }

    if (msg_length && msg_block) {
        for (i = 0; msg_block[i] != (char *)NULL; i++) {
            msg_start += sprintf(msg_start, "%s", msg_block[i]);
        }
    }

    mail->length += msg_length;

    /* Make sure the message is terminated with a new line character */

    msg_end = mail->body + mail->length - 1;

    if (*msg_end != '\n') {
        *(++msg_end) = '\n';
        *(++msg_end) = '\0';
        mail->length++;
    }

    /* Check if we need to add a newline to the mail body */

    if (mail->flags & MAIL_ADD_NEWLINE) {
         mail->body[mail->length] = '\n';
         mail->length++;
         mail->body[mail->length] = '\0';
    }
}

/**
 *  Send a mail message.
 *
 *  If the mail message does not have a recipient or mail body,
 *  this function will do nothing and return successfully.
 *
 *  @param  mail - pointer to the Mail message
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 *
 *  @see mail_get_error()
 */
int mail_send(Mail *mail)
{
    struct iovec  iov[32];
    int           iovcnt;
    int           mail_pipe[2];
    pid_t         mail_pid;
    int           mail_exit_value;
    int           retval;
    int           i;

    /* Make sure we have a recipient and something to send */

    if (!mail->to || !strlen(mail->to) || !mail->length) {
        return(1);
    }

    /* Create the iov array containing the complete mail message */

    iovcnt = 0;

/*
    if (mail->from) {
        iov[iovcnt++].iov_base = "From: ";
        iov[iovcnt++].iov_base = mail->from;
        iov[iovcnt++].iov_base = "\n";
    }
*/

    iov[iovcnt++].iov_base = "To: ";
    iov[iovcnt++].iov_base = mail->to;
    iov[iovcnt++].iov_base = "\n";

    if (mail->cc) {
        iov[iovcnt++].iov_base = "Cc: ";
        iov[iovcnt++].iov_base = mail->cc;
        iov[iovcnt++].iov_base = "\n";
    }

    if (mail->subject) {
        iov[iovcnt++].iov_base = "Subject: ";
        iov[iovcnt++].iov_base = mail->subject;
        iov[iovcnt++].iov_base = "\n";
        iov[iovcnt++].iov_base = "\n";
    }

    iov[iovcnt++].iov_base = mail->body;

    for (i = 0; i < iovcnt; i++) {
        iov[i].iov_len = strlen(iov[i].iov_base);
    }

//printf("%s -F '%s' -t '%s'\n", gSendMailPath, mail->from, mail->to);
//for (i = 0; i < iovcnt; i++) {
//  printf("%s", iov[i].iov_base);
//}

    /* Create the pipe */

    if (pipe(mail_pipe) == -1) {

        snprintf(mail->errstr, MAX_MAIL_ERROR,
            "Could not create pipe to send mail message: '%s'\n"
            " -> %s\n", mail->subject, strerror(errno));

        mail->length  = 0;
        mail->body[0] = '\0';

        return(0);
    }

    /* Create the fork */

    mail_pid = fork();

    if (mail_pid == (pid_t)-1) {

        snprintf(mail->errstr, MAX_MAIL_ERROR,
            "Could not create fork to send mail message: '%s'\n"
            " -> %s\n", mail->subject, strerror(errno));

        mail->length  = 0;
        mail->body[0] = '\0';

        return(0);
    }

    /* Child Process */

    if (mail_pid == 0) {

        dup2(mail_pipe[0], STDIN_FILENO);
        close(mail_pipe[0]);
        close(mail_pipe[1]);

        if (mail->from) {
            execl(gSendMailPath, gSendMailPath,
                "-F", mail->from, "-t", mail->to, NULL);
        }
        else {
            execl(gSendMailPath, gSendMailPath,
                "-t", mail->to, NULL);
        }

        exit(errno);
    }

    /* Parent Process */

    close(mail_pipe[0]);

    retval = 1;

    if (writev(mail_pipe[1], iov, iovcnt) == -1) {

        snprintf(mail->errstr, MAX_MAIL_ERROR,
            "Could not write mail message: '%s'\n"
            " -> %s\n", mail->subject, strerror(errno));

        retval = 0;
    }

    /* Cleanup and return */

    close(mail_pipe[1]);
    wait(&mail_exit_value);

    if (mail_exit_value) {

        int  exit_value    = mail_exit_value >> 8;  /* upper 8 bits */
        int  signal_number = mail_exit_value & 127; /* lower 7 bits */
        int  core_dumped   = mail_exit_value & 128; /* bit 8        */

        if (core_dumped) {

            snprintf(mail->errstr, MAX_MAIL_ERROR,
                "Could not execute sendmail command: '%s'\n"
                " -> core dumped with signal #%d\n",
                gSendMailPath, signal_number);
        }
        else if (signal_number) {

            snprintf(mail->errstr, MAX_MAIL_ERROR,
                "Could not execute sendmail command: '%s'\n"
                " -> exited with signal #%d\n",
                gSendMailPath, signal_number);
        }
        else {

            snprintf(mail->errstr, MAX_MAIL_ERROR,
                "Could not execute sendmail command: '%s'\n"
                " -> Non-zero exit value: %d\n",
                gSendMailPath, exit_value);
        }

        retval = 0;
    }

    mail->length  = 0;
    mail->body[0] = '\0';

    return(retval);
}

/**
 *  Set control flags for a mail message.
 *
 *  @param  mail  - pointer to the Mail message
 *  @param  flags - flags to set
 *
 *  Control Flags:
 *
 *    - MAIL_ADD_NEWLINE - Add an extra newline character after every
 *                         message added with dsmail_append().
 */
void mail_set_flags(Mail *mail, int flags)
{
    mail->flags |= flags;
}

/**
 *  Unset control flags for a mail message.
 *
 *  @param  mail   - pointer to the Mail message
 *  @param  flags  - flags to unset
 *
 *  @see mail_set_flags()
 */
void mail_unset_flags(Mail *mail, int flags)
{
    mail->flags &= (0xffff ^ flags);
}

/*@}*/
