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

/** @file cds_data_types.c
 *  CDS Data Types.
 */

#include "cds3.h"

/*******************************************************************************
 *  Private Data and Functions
 */
/** @privatesection */

static signed char _Byte_Min    = CDS_MIN_BYTE;
static signed char _Byte_Max    = CDS_MAX_BYTE;
static signed char _Byte_Fill   = CDS_FILL_BYTE;

static char        _Char_Min    = CDS_MIN_CHAR;
static char        _Char_Max    = CDS_MAX_CHAR;
static char        _Char_Fill   = CDS_FILL_CHAR;

static short       _Short_Min   = CDS_MIN_SHORT;
static short       _Short_Max   = CDS_MAX_SHORT;
static short       _Short_Fill  = CDS_FILL_SHORT;

static int         _Int_Min     = CDS_MIN_INT;
static int         _Int_Max     = CDS_MAX_INT;
static int         _Int_Fill    = CDS_FILL_INT;

static float       _Float_Min   = CDS_MIN_FLOAT;
static float       _Float_Max   = CDS_MAX_FLOAT;
static float       _Float_Fill  = CDS_FILL_FLOAT;

static double      _Double_Min  = CDS_MIN_DOUBLE;
static double      _Double_Max  = CDS_MAX_DOUBLE;
static double      _Double_Fill = CDS_FILL_DOUBLE;

/* NetCDF4 extended data types */

static unsigned char       _UByte_Min   = CDS_MIN_UBYTE;
static unsigned char       _UByte_Max   = CDS_MAX_UBYTE;
static unsigned char       _UByte_Fill  = CDS_FILL_UBYTE;

static unsigned short      _UShort_Min  = CDS_MIN_USHORT;
static unsigned short      _UShort_Max  = CDS_MAX_USHORT;
static unsigned short      _UShort_Fill = CDS_FILL_USHORT;

static unsigned int        _UInt_Min    = CDS_MIN_UINT;
static unsigned int        _UInt_Max    = CDS_MAX_UINT;
static unsigned int        _UInt_Fill   = CDS_FILL_UINT;

static long long           _Int64_Min   = CDS_MIN_INT64;
static long long           _Int64_Max   = CDS_MAX_INT64;
static long long           _Int64_Fill  = CDS_FILL_INT64;

static unsigned long long  _UInt64_Min  = CDS_MIN_UINT64;
static unsigned long long  _UInt64_Max  = CDS_MAX_UINT64;
static unsigned long long  _UInt64_Fill = CDS_FILL_UINT64;

static char *              _String_Fill = CDS_FILL_STRING;

void *_cds_data_type_min(CDSDataType type)
{
    switch(type) {
        case CDS_BYTE:   return(&_Byte_Min);
        case CDS_CHAR:   return(&_Char_Min);
        case CDS_SHORT:  return(&_Short_Min);
        case CDS_INT:    return(&_Int_Min);
        case CDS_FLOAT:  return(&_Float_Min);
        case CDS_DOUBLE: return(&_Double_Min);
        /* NetCDF4 extended data types */
        case CDS_UBYTE:  return(&_UByte_Min);
        case CDS_USHORT: return(&_UShort_Min);
        case CDS_UINT:   return(&_UInt_Min);
        case CDS_INT64:  return(&_Int64_Min);
        case CDS_UINT64: return(&_UInt64_Min);
        default:         return((void *)NULL);
    }
}

double _cds_data_type_min_double(CDSDataType type)
{
    switch(type) {
        case CDS_BYTE:   return((double)_Byte_Min);
        case CDS_CHAR:   return((double)_Char_Min);
        case CDS_SHORT:  return((double)_Short_Min);
        case CDS_INT:    return((double)_Int_Min);
        case CDS_FLOAT:  return((double)_Float_Min);
        case CDS_DOUBLE: return(_Double_Min);
        /* NetCDF4 extended data types */
        case CDS_UBYTE:  return((double)_UByte_Min);
        case CDS_USHORT: return((double)_UShort_Min);
        case CDS_UINT:   return((double)_UInt_Min);
        case CDS_INT64:  return((double)_Int64_Min);
        case CDS_UINT64: return((double)_UInt64_Min);
        default:         return(_Double_Min);
    }
}

int _cds_data_type_mincmp(CDSDataType type1, CDSDataType type2)
{
    double dbl1 = _cds_data_type_min_double(type1);
    double dbl2 = _cds_data_type_min_double(type2);

    if (dbl1 < dbl2) return(-1);
    if (dbl1 > dbl2) return(1);
    return(0);
}

void *_cds_data_type_max(CDSDataType type)
{
    switch(type) {
        case CDS_BYTE:   return(&_Byte_Max);
        case CDS_CHAR:   return(&_Char_Max);
        case CDS_SHORT:  return(&_Short_Max);
        case CDS_INT:    return(&_Int_Max);
        case CDS_FLOAT:  return(&_Float_Max);
        case CDS_DOUBLE: return(&_Double_Max);
        /* NetCDF4 extended data types */
        case CDS_UBYTE:  return(&_UByte_Max);
        case CDS_USHORT: return(&_UShort_Max);
        case CDS_UINT:   return(&_UInt_Max);
        case CDS_INT64:  return(&_Int64_Max);
        case CDS_UINT64: return(&_UInt64_Max);
        default:         return((void *)NULL);
    }
}

double _cds_data_type_max_double(CDSDataType type)
{
    switch(type) {
        case CDS_BYTE:   return((double)_Byte_Max);
        case CDS_CHAR:   return((double)_Char_Max);
        case CDS_SHORT:  return((double)_Short_Max);
        case CDS_INT:    return((double)_Int_Max);
        case CDS_FLOAT:  return((double)_Float_Max);
        case CDS_DOUBLE: return(_Double_Max);
        /* NetCDF4 extended data types */
        case CDS_UBYTE:  return((double)_UByte_Max);
        case CDS_USHORT: return((double)_UShort_Max);
        case CDS_UINT:   return((double)_UInt_Max);
        case CDS_INT64:  return((double)_Int64_Max);
        case CDS_UINT64: return((double)_UInt64_Max);
        default:         return(_Double_Max);
    }
}

int _cds_data_type_maxcmp(CDSDataType type1, CDSDataType type2)
{
    double dbl1 = _cds_data_type_max_double(type1);
    double dbl2 = _cds_data_type_max_double(type2);

    if (dbl1 < dbl2) return(-1);
    if (dbl1 > dbl2) return(1);
    return(0);
}

void *_cds_default_fill_value(CDSDataType type)
{
    switch(type) {
        case CDS_BYTE:   return(&_Byte_Fill);
        case CDS_CHAR:   return(&_Char_Fill);
        case CDS_SHORT:  return(&_Short_Fill);
        case CDS_INT:    return(&_Int_Fill);
        case CDS_FLOAT:  return(&_Float_Fill);
        case CDS_DOUBLE: return(&_Double_Fill);
        /* NetCDF4 extended data types */
        case CDS_UBYTE:  return(&_UByte_Fill);
        case CDS_USHORT: return(&_UShort_Fill);
        case CDS_UINT:   return(&_UInt_Fill);
        case CDS_INT64:  return(&_Int64_Fill);
        case CDS_UINT64: return(&_UInt64_Fill);
        case CDS_STRING: return(_String_Fill);
        default:         return((void *)NULL);
    }
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Get the data type for the specified name.
 *
 *  Valid names:
 *    - char
 *    - byte
 *    - short
 *    - int
 *    - float
 *    - double
 *
 *    NetCDF4 extended data types
 *
 *    - ubyte
 *    - ushort
 *    - uint
 *    - int64
 *    - uint64
 *
 *  @param  name - the name of the data type
 *
 *  @return
 *    - the data type for the specified name
 *    - CDS_NAT if an invalid name is specified
 */
CDSDataType cds_data_type(const char *name)
{
    CDSDataType type;

    if      (strcmp(name, "char")   == 0) type = CDS_CHAR;
    else if (strcmp(name, "byte")   == 0) type = CDS_BYTE;
    else if (strcmp(name, "short")  == 0) type = CDS_SHORT;
    else if (strcmp(name, "int")    == 0) type = CDS_INT;
    else if (strcmp(name, "float")  == 0) type = CDS_FLOAT;
    else if (strcmp(name, "double") == 0) type = CDS_DOUBLE;
    /* NetCDF4 extended data types */
    else if (strcmp(name, "ubyte")  == 0) type = CDS_UBYTE;
    else if (strcmp(name, "ushort") == 0) type = CDS_USHORT;
    else if (strcmp(name, "uint")   == 0) type = CDS_UINT;
    else if (strcmp(name, "int64")  == 0) type = CDS_INT64;
    else if (strcmp(name, "uint64") == 0) type = CDS_UINT64;
    else if (strcmp(name, "string") == 0) type = CDS_STRING;
    else                                  type = CDS_NAT;

    return(type);
}

/**
 *  Get the name of the specified data type.
 *
 *  @param  type - the data type
 *
 *  @return
 *    - the data type name
 *    - NULL if an invalid data type is specified
 */
const char *cds_data_type_name(CDSDataType type)
{
    const char *name;

    switch (type) {
        case CDS_CHAR:   name = "char";   break;
        case CDS_BYTE:   name = "byte";   break;
        case CDS_SHORT:  name = "short";  break;
        case CDS_INT:    name = "int";    break;
        case CDS_FLOAT:  name = "float";  break;
        case CDS_DOUBLE: name = "double"; break;
        /* NetCDF4 extended data types */
        case CDS_UBYTE:  name = "ubyte";  break;
        case CDS_USHORT: name = "ushort"; break;
        case CDS_UINT:   name = "uint";   break;
        case CDS_INT64:  name = "int64";  break;
        case CDS_UINT64: name = "uint64"; break;
        case CDS_STRING: name = "string"; break;
        default:         name = (const char *)NULL;
    }

    return(name);
}

/**
 *  Get the size (in bytes) of a data type.
 *
 *  @param  type - the data type
 *
 *  @return
 *    - size (in bytes) of the data type
 *    - 0 if an invalid data type is specified
 */
size_t cds_data_type_size(CDSDataType type)
{
    size_t size;

    switch(type) {
        case CDS_BYTE:   size = sizeof(char);   break;
        case CDS_CHAR:   size = sizeof(char);   break;
        case CDS_SHORT:  size = sizeof(short);  break;
        case CDS_INT:    size = sizeof(int);    break;
        case CDS_FLOAT:  size = sizeof(float);  break;
        case CDS_DOUBLE: size = sizeof(double); break;
        /* NetCDF4 extended data types */
        case CDS_UBYTE:  size = sizeof(unsigned char);      break;
        case CDS_USHORT: size = sizeof(unsigned short);     break;
        case CDS_UINT:   size = sizeof(unsigned int);       break;
        case CDS_INT64:  size = sizeof(long long);          break;
        case CDS_UINT64: size = sizeof(unsigned long long); break;
        case CDS_STRING: size = sizeof(char *); break;
        default:         size = 0;
    }

    return(size);
}

/**
 *  Get the valid range of a data type.
 *
 *  @param  type  - the data type
 *  @param  min   - output: the minimum value the data type can hold
 *  @param  max   - output: the maximum value the data type can hold
 */
void cds_get_data_type_range(CDSDataType type, void *min, void *max)
{
    switch(type) {
        case CDS_BYTE:
            memcpy(min, &_Byte_Min,  sizeof(char));
            memcpy(max, &_Byte_Max,  sizeof(char));
            break;
        case CDS_CHAR:
            memcpy(min, &_Char_Min,  sizeof(char));
            memcpy(max, &_Char_Max,  sizeof(char));
            break;
        case CDS_SHORT:
            memcpy(min, &_Short_Min, sizeof(short));
            memcpy(max, &_Short_Max, sizeof(short));
            break;
        case CDS_INT:
            memcpy(min, &_Int_Min,   sizeof(int));
            memcpy(max, &_Int_Max,   sizeof(int));
            break;
        case CDS_FLOAT:
            memcpy(min, &_Float_Min, sizeof(float));
            memcpy(max, &_Float_Max, sizeof(float));
            break;
        case CDS_DOUBLE:
            memcpy(min, &_Double_Min, sizeof(double));
            memcpy(max, &_Double_Max, sizeof(double));
            break;
        /* NetCDF4 extended data types */
        case CDS_UBYTE:
            memcpy(min, &_UByte_Min,  sizeof(unsigned char));
            memcpy(max, &_UByte_Max,  sizeof(unsigned char));
            break;
        case CDS_USHORT:
            memcpy(min, &_UShort_Min, sizeof(unsigned short));
            memcpy(max, &_UShort_Max, sizeof(unsigned short));
            break;
        case CDS_UINT:
            memcpy(min, &_UInt_Min,   sizeof(unsigned int));
            memcpy(max, &_UInt_Max,   sizeof(unsigned int));
            break;
        case CDS_INT64:
            memcpy(min, &_Int64_Min,  sizeof(long long));
            memcpy(max, &_Int64_Max,  sizeof(long long));
            break;
        case CDS_UINT64:
            memcpy(min, &_UInt64_Min, sizeof(unsigned long long));
            memcpy(max, &_UInt64_Max, sizeof(unsigned long long));
            break;
        default:
            break;
    }
}

/**
 *  Get the default fill value used by the NetCDF library.
 *
 *  @param  type  - the data type
 *  @param  value - output: the default fill value
 */
void cds_get_default_fill_value(CDSDataType type, void *value)
{
    switch(type) {
        case CDS_BYTE:   memcpy(value, &_Byte_Fill,   sizeof(char));   break;
        case CDS_CHAR:   memcpy(value, &_Char_Fill,   sizeof(char));   break;
        case CDS_SHORT:  memcpy(value, &_Short_Fill,  sizeof(short));  break;
        case CDS_INT:    memcpy(value, &_Int_Fill,    sizeof(int));    break;
        case CDS_FLOAT:  memcpy(value, &_Float_Fill,  sizeof(float));  break;
        case CDS_DOUBLE: memcpy(value, &_Double_Fill, sizeof(double)); break;
        /* NetCDF4 extended data types */
        case CDS_UBYTE:  memcpy(value, &_UByte_Fill,  sizeof(unsigned char));  break;
        case CDS_USHORT: memcpy(value, &_UShort_Fill, sizeof(unsigned short)); break;
        case CDS_UINT:   memcpy(value, &_UInt_Fill,   sizeof(unsigned int));   break;
        case CDS_INT64:  memcpy(value, &_Int64_Fill,  sizeof(long long));      break;
        case CDS_UINT64: memcpy(value, &_UInt64_Fill, sizeof(unsigned long long)); break;
//        case CDS_STRING: memcpy(value, &_String_Fill, sizeof(char *)); break;
        case CDS_STRING: *(char **)value = strdup(_String_Fill); break;
        default:  break;
    }
}

/**
 *  Get the size (in bytes) of the largest possible data type.
 *
 *  @return
 *    - size (in bytes) of the largest possible data type
 */
size_t cds_max_type_size()
{
    return(CDS_MAX_TYPE_SIZE);
}
