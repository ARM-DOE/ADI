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
*    $Revision: 65910 $
*    $Author: ermold $
*    $Date: 2015-11-17 00:12:02 +0000 (Tue, 17 Nov 2015) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dbconn_version.c
 *  libdbconn library version.
 */

#include "../config.h"

/**
 *  @defgroup DBCONN_VERSION Library Version
 */
/*@{*/

/** @privatesection */

static const char *_Version = PACKAGE_NAME"-"PACKAGE_VERSION;

/** @publicsection */

/**
 *  libdbconn library version.
 *
 *  @return libdbconn library version
 */
const char *dbconn_lib_version(void)
{
    return(_Version);
}

/*@}*/
