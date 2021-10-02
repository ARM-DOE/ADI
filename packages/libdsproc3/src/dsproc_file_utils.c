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

/** @file dsproc_file_utils.c
 *  File Functions.
 */

#include "dsproc3.h"

/**
 *  Copy a file.
 *
 *  This function will copy a file and use MD5 checking to validate the
 *  file copy was successful. It will also add a "copying file" message
 *  to the log file.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  src_file  - full path to the source file
 *  @param  dest_file - full path to the destination file
 *
 *  @return
 *    - 1 if successfull
 *    - 0 if an error occurred
 */
int dsproc_copy_file(const char *src_file, const char *dest_file)
{
    LOG( DSPROC_LIB_NAME,
        "Copying:  %s\n"
        " -> to:   %s\n", src_file, dest_file);

    if (!file_copy(src_file, dest_file, FC_CHECK_MD5)) {
        dsproc_set_status(DSPROC_EFILECOPY);
        return(0);
    }

    return(1);
}

/**
 *  Move a file.
 *
 *  This function will first attempt to rename the file. If the rename
 *  fails because the file is being moved across file systems, it will
 *  be copied to the destination file and the source file deleted. MD5
 *  checking will be used if the file needs to be copied across file
 *  systems.
 *
 *  This function will also add a "moving file" message to the log file.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  src_file  - full path to the source file
 *  @param  dest_file - full path to the destination file
 *
 *  @return
 *    - 1 if the file was moved successfully
 *    - 0 if an error occurred
 */
int dsproc_move_file(const char *src_file, const char *dest_file)
{
    LOG( DSPROC_LIB_NAME,
        "Moving:   %s\n"
        " -> to:   %s\n", src_file, dest_file);

    if (!file_move(src_file, dest_file, FC_CHECK_MD5)) {
        dsproc_set_status(DSPROC_EFILEMOVE);
        return(0);
    }

    return(1);
}

/**
 *  Open a file for reading.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param file - full path to the file
 *
 *  @return
 *    - pointer to the open file
 *    - NULL if an error occurred
 */
FILE *dsproc_open_file(const char *file)
{
    FILE *fp;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Opening file: %s\n", file);

    fp = fopen(file, "r");

    if (!fp) {

        ERROR( DSPROC_LIB_NAME,
            "Could not open file: %s\n"
            " -> %s\n",
            file, strerror(errno));

        dsproc_set_status(DSPROC_EFILEOPEN);
        return((FILE *)NULL);
    }

    return(fp);
}
