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
*    $Revision: 6691 $
*    $Author: ermold $
*    $Date: 2011-05-16 19:55:00 +0000 (Mon, 16 May 2011) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
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

void *_cds_data_type_min(CDSDataType type)
{
    switch(type) {
        case CDS_BYTE:   return(&_Byte_Min);
        case CDS_CHAR:   return(&_Char_Min);
        case CDS_SHORT:  return(&_Short_Min);
        case CDS_INT:    return(&_Int_Min);
        case CDS_FLOAT:  return(&_Float_Min);
        case CDS_DOUBLE: return(&_Double_Min);
        default:         return(&_Double_Min);
    }
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
        default:         return(&_Double_Max);
    }
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
        default:         return(&_Double_Fill);
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
