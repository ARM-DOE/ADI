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

/** @file file_buffer.c
*  File Buffer Functions
*/

#include <errno.h>

#include "armutils.h"

/**
*  Free memory used by a FileBuffer structure.
*
*  @param   fbuf    pointer to the FileBuffer structure
*/
void file_buffer_destroy(FileBuffer *fbuf)
{
    if (fbuf) {
        if (fbuf->full_path) free((void *)fbuf->full_path);
        if (fbuf->stats)     free(fbuf->stats);
        if (fbuf->data)      free(fbuf->data);
        if (fbuf->lines)     free(fbuf->lines);
        free(fbuf);
    }
}

/**
*  Allocate memory for a FileBuffer structure.
*
*  Error messages from this function are sent to the message handler
*  (see msngr_init_log() and msngr_init_mail()).
*
*  @retval  fbuf  pointer to the FileBuffer structure
*  @retval  NULL  if a memory allocation error occurred
*/
FileBuffer *file_buffer_init(void)
{
    FileBuffer *fbuf = calloc(1, sizeof(FileBuffer));

    if (!fbuf) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not allocate memory for new FileBuffer structure\n");

        return((FileBuffer *)NULL);
    }

    fbuf->full_path = calloc(PATH_MAX, sizeof(char));
    if (!fbuf->full_path) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not allocate memory for new FileBuffer structure\n");

        free(fbuf);
        return((FileBuffer *)NULL);
    }

    fbuf->stats = calloc(1, sizeof(struct stat));
    if (!fbuf->stats) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not allocate memory for new FileBuffer structure\n");

        free((void *)fbuf->full_path);
        free(fbuf);
        return((FileBuffer *)NULL);
    }

    return(fbuf);
}

/**
*  Read a file into a FileBuffer.
*
*  The in memory copy of the file can be accessed via the 'data' member of
*  the FileBuffer structure.  This memory is managed by the FileBuffer
*  structure and should not be freed by the calling process.  It will be
*  freed when file_buffer_destroy() is called.
*
*  Using the same FileBuffer to read additional files will reused the
*  previously allocated memory, reallocating more as necessary.
*
*  \b Example:
*  \code
*
*    const char *full_path = "/full/path/to/file";
*    int         errcode   = 0;
*
*    FileBuffer *fbuf;
*    int         nlines;
*    char      **lines;
*    int         status;
*
*    char       *linep;
*    int         li;
*
*    if (!(fbuf = file_buffer_init())) {
*        return(errcode);
*    }
*
*    if (!file_buffer_read(fbuf, full_path, NULL)) {
*        return(errcode);
*    }
*
*    if (!file_buffer_split_lines(fbuf, &nlines, &lines)) {
*        return(errcode);
*    }
*
*    for (li = 0; li < nlines; ++li) {
*        linep = lines[li];
*        printf("%s\n", linep);
*    }
*
*  \endcode
*
*  Error messages from this function are sent to the message handler
*  (see msngr_init_log() and msngr_init_mail()).
*
*  @param   fbuf       pointer to the FileBuffer structure
*  @param   full_path  full path to the input file
*  @param   data       output: fbuf->data if not NULL
*
*  @retval  1  if successful (or zero length file)
*  @retval  0  if an errror occurred
*/
int file_buffer_read(
    FileBuffer  *fbuf,
    const char  *full_path,
    char       **data)
{
    char    *new_data;
    size_t   length;
    size_t   nread;
    FILE    *fp;

    if (data) *data = (char *)NULL;

    /* Clear structure data for new file read */

    fbuf->length = 0;
    fbuf->nlines = 0;

    if (fbuf->data)  fbuf->data[0]  = '\0';
    if (fbuf->lines) fbuf->lines[0] = (char *)NULL;

    /* Set the full_path in the FileBuffer */

    strncpy((char *)fbuf->full_path, full_path, PATH_MAX);

    /* Get file size */

    if (stat(full_path, fbuf->stats) != 0) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not read file: %s\n"
            "  -> %s\n",
            full_path, strerror(errno));

        return(0);
    }

    /* Resize buffer if necessary */

    length = fbuf->stats->st_size;
    if (length == 0) {
        return(1);
    }

    if (length >= fbuf->data_nalloced) {

        new_data = realloc(fbuf->data, (length + 1) * sizeof(char));
        if (!new_data) {

        ERROR( ARMUTILS_LIB_NAME,
                "Could not read file: %s\n"
                "  -> memory allocation error",
                full_path);

            return(0);
        }

        fbuf->data_nalloced = length + 1;
        fbuf->data          = new_data;
    }

    /* Read in the entire file */

    fp = fopen(full_path, "r");
    if (!fp) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not open file: %s\n"
            " -> %s\n",
            full_path, strerror(errno));

        return(0);
    }

    nread = fread(fbuf->data, 1, length, fp);

    if (ferror(fp)) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not read file: %s\n"
            " -> %s\n",
            full_path, strerror(errno));

        fclose(fp);
        return(0);
    }

    fclose(fp);

    if (nread != length) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not read file: %s\n"
            " -> number of bytes read (%d) != file size %d bytes\n",
            full_path, nread, length);

        return(0);
    }

    fbuf->length = length;
    fbuf->data[fbuf->length] = '\0';

    if (data) *data = fbuf->data;

    return(1);
}

/**
*  Create array of line pointers into a FileBuffer.
*
*  This function will replace newline characters '\n' with string terminator
*  characters '\0' and create an array of pointers to each line in the buffer.
*  The output fbuf->lines array will be null terminated.
*
*  The memory used by the returned array of line pointers is managed by the
*  FileBuffer structure and should not be freed by the calling process.
*
*  The number of lines and line pointers can also be accessed using the
*  'nlines' and 'lines' members of the FileBuffer structure.
*
*  \b Example:
*  \code
*  \endcode
*
*  Error messages from this function are sent to the message handler
*  (see msngr_init_log() and msngr_init_mail()).
*
*  @param   fbuf       pointer to the FileBuffer structure
*  @param   nlines     output: fbuf->nlines if not NULL
*  @param   lines      output: fbuf->lines if not NULL
*
*  @retval  1    if successful
*  @retval  0    if a memory allocation error occurred
*/
int file_buffer_split_lines(
    FileBuffer *fbuf,
    int        *nlines,
    char     ***lines)
{
    char   **new_lines;
    char    *chrp;
    size_t   count;

    if (fbuf->nlines) {

        if (nlines) *nlines = fbuf->nlines;
        if (lines)  *lines  = fbuf->lines;

        return(1);
    }

    if (nlines) *nlines = 0;
    if (lines)  *lines  = (char **)NULL;

    /* Count the number of new line characters */

    chrp  = fbuf->data;
    count = 1;

    while( (chrp = strchr(chrp, '\n')) ) {
        count += 1;
        chrp  += 1;
    }

    /* Allocate memory for the array of line pointesr */

    if (count >= fbuf->lines_nalloced) {

        new_lines = realloc(fbuf->lines, (count+1) * sizeof(char *));
        if (!new_lines) {

        ERROR( ARMUTILS_LIB_NAME,
                "Could not allocate memory for FileBuffer line pointers\n");

            return(0);
        }

        fbuf->lines          = new_lines;
        fbuf->lines_nalloced = count + 1;
    }

    /* Create array of line pointers */

    fbuf->lines[0] = chrp = fbuf->data;
    fbuf->nlines   = 1;

    while( (chrp = strchr(chrp, '\n')) ) {
        *chrp = '\0';
        fbuf->lines[fbuf->nlines++] = ++chrp;
    }
    
    fbuf->lines[fbuf->nlines] = (char *)NULL;

    if (nlines) *nlines = fbuf->nlines;
    if (lines)  *lines  = fbuf->lines;

    return(1);
}
