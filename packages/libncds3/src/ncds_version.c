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
*    $Revision: 72316 $
*    $Author: ermold $
*    $Date: 2016-07-11 22:07:18 +0000 (Mon, 11 Jul 2016) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file ncds_version.c
 *  libncds3 library version.
 */

#include "../config.h"

/** @privatesection */

static const char *_Version = PACKAGE_NAME"-"PACKAGE_VERSION;

/** @publicsection */

/**
 *  libncds3 library version.
 *
 *  @return libncds3 library version
 */
const char *ncds_lib_version(void)
{
    return(_Version);
}
