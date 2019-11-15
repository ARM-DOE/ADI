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
*    $Revision: 60948 $
*    $Author: ermold $
*    $Date: 2015-04-02 18:52:36 +0000 (Thu, 02 Apr 2015) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file armutils.h
 *  ARM Utilities Library Header.
 */

#ifndef _ARMUTILS_H
#define _ARMUTILS_H

#if defined(__GNUC__)
#define __need_timeval
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <regex.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>

#if defined(__GNUC__)
#include <stdint.h>
#endif

#include "msngr.h"

/** ARMUTILS library name. */
#define ARMUTILS_LIB_NAME "libarmutils"

#include "armutils/armutils_version.h"
#include "armutils/benchmark.h"
#include "armutils/dir_utils.h"
#include "armutils/dsenv.h"
#include "armutils/endian_swap.h"
#include "armutils/file_buffer.h"
#include "armutils/file_utils.h"
#include "armutils/regex_time.h"
#include "armutils/regex_utils.h"
#include "armutils/string_utils.h"
#include "armutils/time_utils.h"

#endif /* _ARMUTILS_H */
