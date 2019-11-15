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
*    $Revision: 77046 $
*    $Author: ermold $
*    $Date: 2017-03-07 18:52:30 +0000 (Tue, 07 Mar 2017) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file trans_version.c
 *  libtrans library version.
 */

#include "../config.h"

/** @privatesection */

static const char *_Version = PACKAGE_NAME"-"PACKAGE_VERSION;

/** @publicsection */

/**
 *  libtrans library version.
 *
 *  @return libtrans library version
 */
const char *trans_lib_version(void)
{
    return(_Version);
}
