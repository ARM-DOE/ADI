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
*     email: brian.ermold@pnnl.gov
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

/* NetCDF4 extended data types */

static long long   _Int64_Min   = NC_MIN_INT64;
static long long   _Int64_Max   = NC_MAX_INT64;
static long long   _Int64_Fill  = NC_FILL_INT64;

static unsigned char      _UByte_Min   = 0;
static unsigned char      _UByte_Max   = NC_MAX_UBYTE;
static unsigned char      _UByte_Fill  = NC_FILL_UBYTE;

static unsigned short     _UShort_Min  = 0;
static unsigned short     _UShort_Max  = NC_MAX_USHORT;
static unsigned short     _UShort_Fill = NC_FILL_USHORT;

static unsigned int       _UInt_Min    = 0;
static unsigned int       _UInt_Max    = NC_MAX_UINT;
static unsigned int       _UInt_Fill   = NC_FILL_UINT;

static unsigned long long _UInt64_Min  = 0;
static unsigned long long _UInt64_Max  = NC_MAX_UINT64;
static unsigned long long _UInt64_Fill = NC_FILL_UINT64;

static char *             _String_Fill = NC_FILL_STRING;

void *_ncds_data_type_min(nc_type type)
{
    switch(type) {
        case NC_BYTE:   return(&_Byte_Min);
        case NC_CHAR:   return(&_Char_Min);
        case NC_SHORT:  return(&_Short_Min);
        case NC_INT:    return(&_Int_Min);
        case NC_FLOAT:  return(&_Float_Min);
        case NC_DOUBLE: return(&_Double_Min);
        /* NetCDF4 extended data types */
        case NC_INT64:  return(&_Int64_Min);
        case NC_UBYTE:  return(&_UByte_Min);
        case NC_USHORT: return(&_UShort_Min);
        case NC_UINT:   return(&_UInt_Min);
        case NC_UINT64: return(&_UInt64_Min);
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
        /* NetCDF4 extended data types */
        case NC_INT64:  return(&_Int64_Max);
        case NC_UBYTE:  return(&_UByte_Max);
        case NC_USHORT: return(&_UShort_Max);
        case NC_UINT:   return(&_UInt_Max);
        case NC_UINT64: return(&_UInt64_Max);
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
        /* NetCDF4 extended data types */
        case NC_INT64:  return(&_Int64_Fill);
        case NC_UBYTE:  return(&_UByte_Fill);
        case NC_USHORT: return(&_UShort_Fill);
        case NC_UINT:   return(&_UInt_Fill);
        case NC_UINT64: return(&_UInt64_Fill);
        case NC_STRING: return(_String_Fill);
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
        /* NetCDF4 extended data types */
        case NC_INT64:  cds_type = CDS_INT64;  break;
        case NC_UBYTE:  cds_type = CDS_UBYTE;  break;
        case NC_USHORT: cds_type = CDS_USHORT; break;
        case NC_UINT:   cds_type = CDS_UINT;   break;
        case NC_UINT64: cds_type = CDS_UINT64; break;
        case NC_STRING: cds_type = CDS_STRING; break;
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
        /* NetCDF4 extended data types */
        case CDS_INT64:  nctype = NC_INT64;  break;
        case CDS_UBYTE:  nctype = NC_UBYTE;   break;
        case CDS_USHORT: nctype = NC_USHORT;  break;
        case CDS_UINT:   nctype = NC_UINT;    break;
        case CDS_UINT64: nctype = NC_UINT64;  break;
        case CDS_STRING: nctype = NC_STRING;  break;
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
        case NC_BYTE:   memcpy(value, &_Byte_Fill,   sizeof(signed char)); break;
        case NC_CHAR:   memcpy(value, &_Char_Fill,   sizeof(char));        break;
        case NC_SHORT:  memcpy(value, &_Short_Fill,  sizeof(short));       break;
        case NC_INT:    memcpy(value, &_Int_Fill,    sizeof(int));         break;
        case NC_FLOAT:  memcpy(value, &_Float_Fill,  sizeof(float));       break;
        case NC_DOUBLE: memcpy(value, &_Double_Fill, sizeof(double));      break;
        /* NetCDF4 extended data types */
        case NC_INT64:  memcpy(value, &_Int64_Fill,  sizeof(long long));   break;
        case NC_UBYTE:  memcpy(value, &_UByte_Fill,  sizeof(unsigned char));  break;
        case NC_USHORT: memcpy(value, &_UShort_Fill, sizeof(unsigned short)); break;
        case NC_UINT:   memcpy(value, &_UInt_Fill,   sizeof(unsigned int));   break;
        case NC_UINT64: memcpy(value, &_UInt64_Fill, sizeof(unsigned long long)); break;
        case NC_STRING: *(char **)value = strdup(_String_Fill); break;
        default:  break;
    }
}
