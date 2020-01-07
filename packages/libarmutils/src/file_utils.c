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
*    $Revision: 16656 $
*    $Author: ermold $
*    $Date: 2013-01-24 01:51:31 +0000 (Thu, 24 Jan 2013) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file file_utils.c
 *  File Utilities
 */

#include "armutils.h"

#include <openssl/md5.h>
#define MD5Init   MD5_Init
#define MD5Update MD5_Update
#define MD5Final  MD5_Final

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Copy a file.
 *
 *  This function will prepend the destination file with a '.' and add a '.lck'
 *  extenstion to it while the file is being copied.  When the copy has been
 *  completed successfuly the rename function is used to remove the '.' prefix
 *  and '.lck' extension.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  src_file  - path to the source file
 *  @param  dest_file - path to the destination file
 *  @param  flags     - control flags
 *
 *  <b>Control Flags:</b>
 *
 *    - FC_CHECK_MD5 = Use MD5 validation.
 *
 *  @return
 *    - 1 if the file copy was successful
 *    - 0 if an error occurred
 */
int file_copy(
    const char *src_file,
    const char *dest_file,
    int         flags)
{
    size_t      dest_len;
    char        tmp_file[PATH_MAX];
    char       *chrp;
    int         src_fd;
    int         tmp_fd;
    struct stat src_stats;
    char       *buf;
    size_t      buf_size;
    ssize_t     nread;
    ssize_t     nwritten;
    char        src_md5[33];
    char        tmp_md5[33];

    /* Create the tmp file name */

    dest_len = strlen(dest_file);
    if ((dest_len == 0) || (dest_len > PATH_MAX - 6)) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not copy file:\n"
            " -> from: %s\n"
            " -> to:   %s\n"
            " -> invalid destination file length: %d\n",
            src_file, dest_file, dest_len);

        return(0);
    }

    strcpy(tmp_file, dest_file);

    for (chrp = tmp_file + dest_len; ; chrp--) {

        *(chrp+1) = *chrp;

        if (chrp == tmp_file)  break;
        if (*(chrp-1) == '/')  break;
    }
    *chrp = '.';

    strcat(tmp_file, ".lck");

    if (flags & FC_CHECK_MD5) {

        /* Get the src file MD5 */

        if (!file_get_md5(src_file, src_md5)) {

            ERROR( ARMUTILS_LIB_NAME,
                "Could not copy file:\n"
                " -> from: %s\n"
                " -> to:   %s\n"
                " -> could not get source file MD5\n",
                src_file, dest_file);

            return(0);
        }
    }

    /* Open the src file for reading */

    src_fd = open(src_file, O_RDONLY);
    if (src_fd < 0) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not copy file:\n"
            " -> from: %s\n"
            " -> to:   %s\n"
            " -> src file open error: %s\n",
            src_file, dest_file, strerror(errno));

        return(0);
    }

    /* Get the src file stats */

    if (fstat(src_fd, &src_stats) < 0) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not copy file:\n"
            " -> from: %s\n"
            " -> to:   %s\n"
            " -> src file stat error: %s\n",
            src_file, dest_file, strerror(errno));

        close(src_fd);
        return(0);
    }

    /* Open the tmp file for writing */

    tmp_fd = open(tmp_file, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (tmp_fd < 0) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not copy file:\n"
            " -> from: %s\n"
            " -> to:   %s\n"
            " -> tmp file open error: %s\n",
            src_file, tmp_file, strerror(errno));

        close(src_fd);
        return(0);
    }

    /* Copy the contents of the src file to the tmp file */

    buf_size = getpagesize();
    buf = (char *)malloc(buf_size * sizeof(char));

    while ((nread = read(src_fd, buf, buf_size)) > 0) {

        nwritten = write(tmp_fd, buf, nread);
        if (nwritten != nread) {
            if (nwritten < 0) {

                ERROR( ARMUTILS_LIB_NAME,
                    "Could not copy file:\n"
                    " -> from: %s\n"
                    " -> to:   %s\n"
                    " -> write error: %s\n",
                    src_file, tmp_file, strerror(errno));
            }
            else {

                ERROR( ARMUTILS_LIB_NAME,
                    "Could not copy file:\n"
                    " -> from: %s\n"
                    " -> to:   %s\n"
                    " -> bytes written (%d) does not match bytes read (%d)\n",
                    src_file, tmp_file, (int)nwritten, (int)nread);
            }

            free(buf);
            close(src_fd);
            close(tmp_fd);
            unlink(tmp_file);
            return(0);
        }
    }

    if (nread < 0) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not copy file:\n"
            " -> from: %s\n"
            " -> to:   %s\n"
            " -> read error: %s\n",
            src_file, tmp_file, strerror(errno));

        free(buf);
        close(src_fd);
        close(tmp_fd);
        unlink(tmp_file);
        return(0);
    }

    free(buf);
    close(src_fd);
    close(tmp_fd);

    /* Set the tmp file access permissions */

    chmod(tmp_file, src_stats.st_mode & 07777);

    if (flags & FC_CHECK_MD5) {

        /* Get the tmp file MD5 */

        if (!file_get_md5(tmp_file, tmp_md5)) {

            ERROR( ARMUTILS_LIB_NAME,
                "Could not copy file:\n"
                " -> from: %s\n"
                " -> to:   %s\n"
                " -> could not get destination file MD5\n",
                src_file, tmp_file);

            unlink(tmp_file);
            return(0);
        }

        /* Check MD5s */

        if (strcmp(src_md5, tmp_md5) != 0) {

            ERROR( ARMUTILS_LIB_NAME,
                "Could not copy file:\n"
                " -> from: %s\n"
                " -> to:   %s\n"
                " -> source and destination files have different MD5s\n",
                src_file, tmp_file);

            unlink(tmp_file);
            return(0);
        }
    }

    /* Rename the tmp file to the correct destination name */

    if (rename(tmp_file, dest_file) < 0) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not copy file:\n"
            " -> from: %s\n"
            " -> to:   %s\n"
            " -> tmp file rename error: %s\n",
            src_file, dest_file, strerror(errno));

        unlink(tmp_file);
        return(0);
    }

    return(1);
}

/**
 *  Check if a file exists.
 *
 *  @param  file - path to the file
 *
 *  @return
 *    - 1 if the file exists
 *    - 0 if the file does not exist
 */
int file_exists(const char *file)
{
    if (access(file, F_OK) == 0) {
        return(1);
    }

    return(0);
}

/**
 *  Get the MD5 of a file.
 *
 *  The hexdigest buffer must be large enough to hold 33 characters
 *  (32 for the hex value plus one for the string terminator).
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  file      - path to the file
 *  @param  hexdigest - buffer to store the MD5 hexdigext value
 *
 *  @return
 *    - pointer to the hexdigest buffer
 *    - NULL if an error occurred
 */
char *file_get_md5(const char *file, char *hexdigest)
{
    int            fd;
    char          *buf;
    size_t         buf_size;
    ssize_t        nread;
    MD5_CTX        md5_context;
    unsigned char  md5_digest[16];
    int            i;

    fd = open(file, O_RDONLY);
    if (fd < 0) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not get MD5 for file: %s\n"
            " -> open error: %s\n", file, strerror(errno));

        return((char *)NULL);
    }

    MD5Init(&md5_context);

    buf_size = getpagesize();
    buf = (char *)malloc(buf_size * sizeof(char));

    while ((nread = read(fd, buf, buf_size)) > 0) {
        MD5Update(&md5_context, buf, nread);
    }

    if (nread == -1) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not get MD5 for file: %s\n"
            " -> read error: %s\n", file, strerror(errno));

        free(buf);
        close(fd);
        return((char *)NULL);
    }

    free(buf);
    close(fd);

    MD5Final(md5_digest, &md5_context);

    for (i = 0; i < 16; i++) {
        sprintf(hexdigest + 2 * i, "%02x", md5_digest[i]);
    }

    return(hexdigest);
}

/**
 *  Move a file.
 *
 *  This function will first attempt to simply rename the file.
 *  If the rename fails because the file is being moved across
 *  file systems, the file_copy() function will be used and the
 *  source file deleted.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  src_file  - path to the old file name
 *  @param  dest_file - path to the new file name
 *  @param  flags     - control flags
 *
 *  <b>Control Flags:</b>
 *
 *  - FC_CHECK_MD5 = Use MD5 validation. This flag will be ignored
 *                   unless it is necessary to copy and delete the
 *                   file in order to move it.
 *
 *  @return
 *    - 1 if the file was moved
 *    - 0 if an error occurred
 */
int file_move(
    const char *src_file,
    const char *dest_file,
    int         flags)
{
    struct stat    old_stats;
    struct utimbuf ut;

    /* First try using rename */

    if (rename(src_file, dest_file) == 0) {
        return(1);
    }

    /* Check if we are moving the file across file systems */

    if (errno != EXDEV) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not move file:\n"
            " -> from: %s\n"
            " -> to:   %s\n"
            " -> rename error: %s\n",
            src_file, dest_file, strerror(errno));

        return(0);
    }

    /* We are moving the file across file systems */

    /* Get the old file stats */

    if (stat(src_file, &old_stats) < 0) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not move file:\n"
            " -> from: %s\n"
            " -> to:   %s\n"
            " -> stat error: %s\n",
            src_file, dest_file, strerror(errno));

        return(0);
    }

    /* Copy the old file to the new file */

    if (!file_copy(src_file, dest_file, flags)) {
        return(0);
    }

    /* Set the new file access and modification times */

    ut.actime  = old_stats.st_atime;
    ut.modtime = old_stats.st_mtime;
    utime(dest_file, &ut);

    /* Unlink the old file */

    if (unlink(src_file) < 0) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not unlink file: %s\n"
            " -> %s\n", src_file, strerror(errno));

        return(0);
    }

    return(1);
}

/**
 *  Create a memory map of a file for reading.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  file     - path to the file
 *  @param  map_size - returns the size of the memory mapped file
 *
 *  @return
 *    - address of the memory map
 *    - (void *)-1 if an error occurred
 */
void *file_mmap(const char *file, size_t *map_size)
{
    int         fd;
    struct stat file_stats;
    void       *map_addr;

    fd = open(file, O_RDONLY);
    if (fd < 0) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not create memory map for file: %s\n"
            " -> open error: %s\n", file, strerror(errno));

        return(MAP_FAILED);
    }

    if (fstat(fd, &file_stats) == -1) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not create memory map for file: %s\n"
            " -> fstat error: %s\n", file, strerror(errno));

        close(fd);
        return(MAP_FAILED);
    }

    *map_size = file_stats.st_size;

    map_addr = mmap((void *)NULL, *map_size, PROT_READ, MAP_SHARED, fd, 0);
    if (map_addr == MAP_FAILED) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not create memory map for file: %s\n"
            " -> mmap error: %s\n", file, strerror(errno));

        close(fd);
        return(MAP_FAILED);
    }

    close(fd);

    return(map_addr);
}

/**
 *  Remove a memory map created to read a file.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  map_addr  - address of the memory map
 *  @param  map_size  - size of the memory mapped file
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int file_munmap(void *map_addr, size_t map_size)
{
    if (munmap(map_addr, map_size) == -1) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not remove memory map\n"
            " -> munmap error: %s\n", strerror(errno));

        return(0);
    }

    return(1);
}
