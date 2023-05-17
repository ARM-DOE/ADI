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
*    $Revision: 6421 $
*    $Author: ermold $
*    $Date: 2011-04-26 01:23:44 +0000 (Tue, 26 Apr 2011) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file file_utils.h
 *  File Utilities
 */

#ifndef _FILEUTILS_H
#define _FILEUTILS_H

#include "armutils/time_utils.h"

/**
 *  @defgroup ARMUTILS_FILE_UTILS File Utils
 */
/*@{*/

#define FC_CHECK_MD5 0x1 /**< use md5 validation when copying a file */

int file_copy(
    const char *src_file,
    const char *dest_file,
    int         flags);

int file_exists(const char *file);

char *file_get_md5(const char *file, char *hexdigest);

int file_move(
    const char *src_file,
    const char *dest_file,
    int         use_md5_validation);

void *file_mmap(const char *file, size_t *map_size);
int   file_munmap(void *map_addr, size_t map_size);

int   file_mod_time(const char *full_path, timeval_t *mod_time);
int   get_nfs_time(const char *dir_path, timeval_t *nfs_time);

/*@}*/

#endif /* _FILEUTILS_H */
