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
*    $Revision: 67792 $
*    $Author: ermold $
*    $Date: 2016-02-29 23:43:36 +0000 (Mon, 29 Feb 2016) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file msngr_version.c
 *  libmsngr library version.
 */

#include "../config.h"

/**
 *  @defgroup MSNGR_VERSION Library Version
 */
/*@{*/

/** @privatesection */

static const char *_Version = PACKAGE_NAME"-"PACKAGE_VERSION;

/** @publicsection */

/**
 *  libmsngr library version.
 *
 *  @return libmsngr library version
 */
const char *msngr_lib_version(void)
{
    return(_Version);
}

/*@}*/
