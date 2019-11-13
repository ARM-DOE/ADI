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
*    $Revision: 6704 $
*    $Author: ermold $
*    $Date: 2011-05-16 22:47:23 +0000 (Mon, 16 May 2011) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file ncds_data_types.c
 *  NCDS Data Type Functions.
 */

#include "ncds3.h"
#include "ncds_private.h"

/*******************************************************************************
 *  Private Data and Functions
 */
/** @privatesection */

static signed char _Byte_Min    = NC_MIN_BYTE;
static signed char _Byte_Max    = NC_MAX_BYTE;
static signed char _Byte_Fill   = NC_FILL_BYTE;

static char        _Char_Min    = 0;
static char        _Char_Max    = NC_MAX_CHAR;
static char        _Char_Fill   = NC_FILL_CHAR;

static short       _Short_Min   = NC_MIN_SHORT;
static short       _Short_Max   = NC_MAX_SHORT;
static short       _Short_Fill  = NC_FILL_SHORT;

static int         _Int_Min     = NC_MIN_INT;
static int         _Int_Max     = NC_MAX_INT;
static int         _Int_Fill    = NC_FILL_INT;

static float       _Float_Min   = NC_MIN_FLOAT;
static float       _Float_Max   = NC_MAX_FLOAT;
static float       _Float_Fill  = NC_FILL_FLOAT;

static double      _Double_Min  = NC_MIN_DOUBLE;
static double      _Double_Max  = NC_MAX_DOUBLE;
static double      _Double_Fill = NC_FILL_DOUBLE;

void *_ncds_data_type_min(nc_type type)
{
    switch(type) {
        case NC_BYTE:   return(&_Byte_Min);
        case NC_CHAR:   return(&_Char_Min);
        case NC_SHORT:  return(&_Short_Min);
        case NC_INT:    return(&_Int_Min);
        case NC_FLOAT:  return(&_Float_Min);
        case NC_DOUBLE: return(&_Double_Min);
        default:        return((void *)NULL);
    }
}

void *_ncds_data_type_max(nc_type type)
{
    switch(type) {
        case NC_BYTE:   return(&_Byte_Max);
        case NC_CHAR:   return(&_Char_Max);
        case NC_SHORT:  return(&_Short_Max);
        case NC_INT:    return(&_Int_Max);
        case NC_FLOAT:  return(&_Float_Max);
        case NC_DOUBLE: return(&_Double_Max);
        default:        return((void *)NULL);
    }
}

void *_ncds_default_fill_value(nc_type type)
{
    switch(type) {
        case NC_BYTE:   return(&_Byte_Fill);
        case NC_CHAR:   return(&_Char_Fill);
        case NC_SHORT:  return(&_Short_Fill);
        case NC_INT:    return(&_Int_Fill);
        case NC_FLOAT:  return(&_Float_Fill);
        case NC_DOUBLE: return(&_Double_Fill);
        default:        return((void *)NULL);
    }
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Convert a NetCDF data type to a CDS data type.
 *
 *  @param  nctype - NetCDF data type
 *
 *  @return
 *    - CDS data type
 *    - CDS_NAT if an invalid NetCDF data type is specified
 */
CDSDataType ncds_cds_type(nc_type nctype)
{
    CDSDataType cds_type;

    switch (nctype) {
        case NC_BYTE:   cds_type = CDS_BYTE;   break;
        case NC_CHAR:   cds_type = CDS_CHAR;   break;
        case NC_SHORT:  cds_type = CDS_SHORT;  break;
        case NC_INT:    cds_type = CDS_INT;    break;
        case NC_FLOAT:  cds_type = CDS_FLOAT;  break;
        case NC_DOUBLE: cds_type = CDS_DOUBLE; break;
        default:        cds_type = CDS_NAT;
    }

    return(cds_type);
}

/**
 *  Convert a CDS data type type to a NetCDF data.
 *
 *  @param  cds_type - CDS data type
 *
 *  @return
 *      - NetCDF data type
 *      - NC_NAT if an invalid CDSDataType is specified
 */
nc_type ncds_nc_type(CDSDataType cds_type)
{
    nc_type nctype;

    switch (cds_type) {
        case CDS_BYTE:   nctype = NC_BYTE;   break;
        case CDS_CHAR:   nctype = NC_CHAR;   break;
        case CDS_SHORT:  nctype = NC_SHORT;  break;
        case CDS_INT:    nctype = NC_INT;    break;
        case CDS_FLOAT:  nctype = NC_FLOAT;  break;
        case CDS_DOUBLE: nctype = NC_DOUBLE; break;
        default:         nctype = NC_NAT;
    }

    return(nctype);
}

/**
 *  Get the default fill value used by the NetCDF library.
 *
 *  @param  type  - the data type
 *  @param  value - output: the default fill value
 */
void ncds_get_default_fill_value(nc_type type, void *value)
{
    switch(type) {
        case NC_BYTE:   memcpy(value, &_Byte_Fill,   sizeof(char));   break;
        case NC_CHAR:   memcpy(value, &_Char_Fill,   sizeof(char));   break;
        case NC_SHORT:  memcpy(value, &_Short_Fill,  sizeof(short));  break;
        case NC_INT:    memcpy(value, &_Int_Fill,    sizeof(int));    break;
        case NC_FLOAT:  memcpy(value, &_Float_Fill,  sizeof(float));  break;
        case NC_DOUBLE: memcpy(value, &_Double_Fill, sizeof(double)); break;
        default:  break;
    }
}
