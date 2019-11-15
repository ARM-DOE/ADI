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
*    $Revision: 78546 $
*    $Author: ermold $
*    $Date: 2017-05-25 23:21:28 +0000 (Thu, 25 May 2017) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file cds_version.c
 *  libcds3 library version.
 */

#include "../config.h"

/** @privatesection */

static const char *_Version = PACKAGE_NAME"-"PACKAGE_VERSION;

/** @publicsection */

/**
 *  libcds3 library version.
 *
 *  @return libcds3 library version
 */
const char *cds_lib_version(void)
{
    return(_Version);
}
