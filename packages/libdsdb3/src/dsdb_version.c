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
*    $Revision: 54547 $
*    $Author: ermold $
*    $Date: 2014-05-27 19:37:27 +0000 (Tue, 27 May 2014) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsdb_version.c
 *  libdsdb3 library version.
 */

#include "../config.h"

/**
 *  @defgroup DSDB_VERSION Library Version
 */
/*@{*/

/** @privatesection */

static const char *_Version = PACKAGE_NAME"-"PACKAGE_VERSION;

/** @publicsection */

/**
 *  libdsdb3 library version.
 *
 *  @return libdsdb3 library version
 */
const char *dsdb_lib_version(void)
{
    return((const char *)_Version);
}

/*@}*/
