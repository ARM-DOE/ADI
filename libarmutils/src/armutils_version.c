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
*    $Revision: 75245 $
*    $Author: ermold $
*    $Date: 2016-12-06 01:25:47 +0000 (Tue, 06 Dec 2016) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file armutils_version.c
 *  libarmutils library version.
 */

#include "../config.h"

/** @privatesection */

static const char *_Version = PACKAGE_NAME"-"PACKAGE_VERSION;

/** @publicsection */

/**
 *  libarmutils library version.
 *
 *  @return libarmutils library version
 */
const char *armutils_lib_version(void)
{
    return(_Version);
}
