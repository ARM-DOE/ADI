/*******************************************************************************
*
*  Copyright © 2014, Battelle Memorial Institute
*  All rights reserved.
*
********************************************************************************
*
*  Author:
*     name:  Brian Ermold
*     phone: (509) 375-2277
*     email: brian.ermold@pnl.gov
*
*******************************************************************************/

/** @file dsproc_version.c
 *  libdsproc3 library version.
 */

#include "../config.h"

#include <string.h>
#include "dsproc3.h"
#include "trans.h"

/** @privatesection */

static const char *_Version = PACKAGE_NAME"-"PACKAGE_VERSION;

static char  _ADI_Version[256];

/**
 *  Private: Trim repository tag from version string if necessary.
 *
 *  @param version - version string
 */
void _dsproc_trim_version(char *version)
{
    char *vp;
    char *cp;

    if (!version) return;

    /* find colon */

    if ((*version != '$') || !(cp = strchr(version, ':'))) {
        return;
    }

    /* skip spaces */

    for (cp++; *cp == ' '; cp++);

    if (*cp == '\0') {
        *version = '\0';
        return;
    }

    /* trim leading portion of repository tag */

    vp = version;
    while (*cp != '\0') *vp++ = *cp++;

    /* trim trailing portion of repository tag */

    for (vp--; (*vp == ' ') || (*vp == '$'); vp--) {
        if (vp == version) break;
    }

    *++vp = '\0';
}

/** @publicsection */

/**
 *  libdsproc3 library version.
 *
 *  @return libdsproc3 library version
 */
const char *dsproc_lib_version(void)
{
    return((const char *)_Version);
}

/**
 *  ADI version.
 *
 *  @return Full ADI version string
 */
const char *adi_version(void)
{
    const char *string;
    char        version[16];
    char       *strp;
    int         nfound, major, minor, micro, i;

    struct {
        const char *name;
        const char *(*version)(void);
    } libs[] = {
        { "dsproc3",  dsproc_lib_version   },
        { "dsdb3",    dsdb_lib_version     },
        { "trans",    trans_lib_version    },
        { "ncds3",    ncds_lib_version     },
        { "cds3",     cds_lib_version      },
        { "dbconn",   dbconn_lib_version   },
        { "armutils", armutils_lib_version },
        { "msngr",    msngr_lib_version    },
        { NULL,       NULL                 }
    };

    if (*_ADI_Version == '\0') {
    
        strp = _ADI_Version;

        for (i = 0; libs[i].name; ++i) {

            micro  = 0;
            string = libs[i].version();
            nfound = parse_version_string(string, &major, &minor, &micro);

            if (nfound < 2) {
                strcpy(version, "x.x");
            }
            else {
                sprintf(version, "%d.%d", major, minor);
            }
        
            if (i == 0) {
                strp += sprintf(strp, "%s-%s", libs[i].name, version);
            }
            else {
                strp += sprintf(strp, ":%s-%s", libs[i].name, version);
            }
        }
    }

    return((const char *)_ADI_Version);
}
