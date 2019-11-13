/*******************************************************************************
*
*  COPYRIGHT (C) 2017 Battelle Memorial Institute.  All Rights Reserved.
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
*    $Revision:$
*    $Author:$
*    $Date:$
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file file_buffer.h
*  File Buffer Header.
*/

#ifndef _FILE_BUFFER_H
#define _FILE_BUFFER_H

#include <sys/stat.h>

/**
 *  @defgroup ARMUTILS_FILE_BUFFER File Buffer
 */
/*@{*/

/** FileBuffer Structure. */
typedef struct FileBuffer {

    const char  *full_path;      /**< full path to the file             */
    struct stat *stats;          /**< file stats                        */
    size_t       length;         /**< size of the file in bytes         */
    char        *data;           /**< in memory copy of the file        */
    size_t       data_nalloced;  /**< allocated length of data buffer   */

    /* used by file_buffer_split_lines() */

    char       **lines;          /**< array of line pointers created by
                                      file_buffer_split_lines()          */
    size_t       nlines;         /**< number of lines                    */
    size_t       lines_nalloced; /**< allocated length of lines array    */
    
} FileBuffer;

void        file_buffer_destroy(FileBuffer *fbuf);
FileBuffer *file_buffer_init(void);
int         file_buffer_read(
                FileBuffer  *fbuf,
                const char  *full_path,
                char       **data);

int         file_buffer_split_lines(
                FileBuffer *fbuf,
                int        *nlines,
                char     ***lines);

/*@}*/

#endif /* _FILE_BUFFER_H */
