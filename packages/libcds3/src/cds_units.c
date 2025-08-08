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

/** @file cds_units.c
 *  CDS Units Functions.
 */

#include "cds3.h"
#include "cds_private.h"
#include "../config.h"

#include <errno.h>
#include UDUNITS_INCLUDE
#include <math.h>
#include <ctype.h>

/*******************************************************************************
 *  Private Data and Functions
 */
/** @privatesection */

#include "cds_units_map.h"

/** Macro used to generate an error message from a UDUNITS-2 status value. */
#define UDUNITS_ERROR(sender, status, ...) \
    _cds_udunits_error(sender, __func__, __FILE__, __LINE__, status, __VA_ARGS__)

static ut_system *_UnitSystem;

typedef struct {
    char *symbol;
    char *name;
} SymbolMap;

static size_t     _NumMapSymbols = 0;
static SymbolMap *_MapSymbols    = (SymbolMap *)NULL;

/**
 *  Get the error message string for a UDUNITS-2 status value.
 *
 *  @param  sender   - the name of the process generating the error message
 *  @param  func     - the name of the function generating the error message
 *  @param  src_file - the source file the message came from
 *  @param  src_line - the line number in the source file
 *  @param  status   - UDUNITS-2 status value
 *  @param  format   - user message format string (see printf)
 *  @param  ...      - arguments for the user message format string
 */
void _cds_udunits_error(
    const char *sender,
    const char *func,
    const char *src_file,
    int         src_line,
    ut_status   status,
    const char *format, ...)
{
    const char *ut_string;
    char       *user_message;
    va_list     args;
    char       *chrp;

    switch (status) {
        case UT_BAD_ARG:
            ut_string = "An argument violates the the function's contract (e.g., it's NULL).";
            break;
        case UT_EXISTS:
            ut_string = "Unit, prefix, or identifier already exists";
            break;
        case UT_NO_UNIT:
            ut_string = "No such unit exists";
            break;
        case UT_OS:
            ut_string = strerror(errno);
            break;
        case UT_NOT_SAME_SYSTEM:
            ut_string = "The units belong to different unit-systems";
            break;
        case UT_MEANINGLESS:
            ut_string = "The operation on the unit or units is meaningless";
            break;
        case UT_NO_SECOND:
            ut_string = "The unit-system doesn't have a unit named 'second'";
            break;
        case UT_VISIT_ERROR:
            ut_string = "An error occurred while visiting a unit";
            break;
        case UT_CANT_FORMAT:
            ut_string = "A unit can't be formatted in the desired manner";
            break;
        case UT_SYNTAX:
            ut_string = "String unit representation contains syntax error";
            break;
        case UT_UNKNOWN:
            ut_string = "String unit representation contains unknown word";
            break;
        case UT_OPEN_ARG:
            ut_string = "Can't open argument-specified unit database";
            break;
        case UT_OPEN_ENV:
            ut_string = "Can't open environment-specified unit database";
            break;
        case UT_OPEN_DEFAULT:
            ut_string = "Can't open installed, default, unit database";
            break;
        case UT_PARSE:
            ut_string = "Error parsing unit database";
            break;
        case UT_SUCCESS:
        default:
            ut_string = NULL;
            break;
    }

    va_start(args, format);
    user_message = msngr_format_va_list(format, args);
    va_end(args);

    if (user_message) {

        chrp = user_message + strlen(user_message) - 1;
        if (*chrp == '\n') *chrp = '\0';

        if (ut_string) {
            msngr_send(sender, func, src_file, src_line, MSNGR_ERROR,
                "%s\n -> %s\n", user_message, ut_string);
        }
        else {
            msngr_send(sender, func, src_file, src_line, MSNGR_ERROR,
                "%s\n", user_message);
        }

        free(user_message);
    }
    else {

        msngr_send(sender, func, src_file, src_line, MSNGR_WARNING,
            "memory allocation error creating UDUNITS-2 error message.\n");
    }
}

static int _cds_add_symbol_to_map(const char *symbol, const char *name)
{
    SymbolMap *new_map;
    char      *new_name;
    char      *new_symbol;
    size_t     i;

    /* Check if we have already added this symbol */

    for (i = 0; i < _NumMapSymbols; i++) {
        if (strcmp(_MapSymbols[i].symbol, symbol) == 0) {
            break;
        }
    }

    if (i != _NumMapSymbols) {

        new_name = strdup(name);
        if (!new_name) {

            ERROR( CDS_LIB_NAME,
                "Could not map symbol '%s' to unit '%s'\n"
                " -> memory allocation error\n",
                symbol, name);

            return(0);
        }

        free(_MapSymbols[i].name);

        _MapSymbols[i].name = new_name;

        return(1);
    }

    /* Allocate memory for the new symbol map */

    new_map = (SymbolMap *)realloc(
        _MapSymbols, (_NumMapSymbols + 1) * sizeof(SymbolMap));

    if (!new_map) {

        ERROR( CDS_LIB_NAME,
            "Could not map symbol '%s' to unit '%s'\n"
            " -> memory allocation error\n",
            symbol, name);

        return(0);
    }

    _MapSymbols = new_map;

    new_symbol = strdup(symbol);
    new_name   = strdup(name);

    if (!new_symbol || !new_name) {

        ERROR( CDS_LIB_NAME,
            "Could not map symbol '%s' to unit '%s'\n"
            " -> memory allocation error\n",
            symbol, name);

        if (new_symbol) free(new_symbol);
        if (new_name)   free(new_name);

        return(0);
    }

    _MapSymbols[_NumMapSymbols].symbol = new_symbol;
    _MapSymbols[_NumMapSymbols].name   = new_name;

    _NumMapSymbols++;

    return(1);
}

static void _cds_free_symbols_map(void)
{
    size_t i;

    if (_MapSymbols) {

        for (i = 0; i < _NumMapSymbols; i++) {

            if (_MapSymbols[i].symbol) free(_MapSymbols[i].symbol);
            if (_MapSymbols[i].name)   free(_MapSymbols[i].name);
        }

        free(_MapSymbols);
    }

    _MapSymbols    = (SymbolMap *)NULL;
    _NumMapSymbols = 0;
}

static int _cds_map_symbols(void)
{
    size_t i;

    for (i = 0; i < _NumMapSymbols; i++) {

        if (!cds_map_symbol_to_unit(
            _MapSymbols[i].symbol, _MapSymbols[i].name)) {

            return(0);
        }
    }

    return(1);
}

static ut_unit *_cds_ut_parse_from_units(const char *from_units)
{
    ut_unit   *parsed_unit;
    ut_status  status;
    int        mi;

    /* Special hack for "Dobson units". For some reason UDUNITS thinks 'units'
     * is a valid unit so it successfully parses "Dobson units". Unfortunately,
     * this is not the same as Dobson or DU. */

    if (strcmp(from_units, "Dobson units") == 0) {
        parsed_unit = ut_parse(_UnitSystem, "DU", UT_ASCII);
        if (parsed_unit) {
            return(parsed_unit);
        }
    }

    /* Parse from_units string */

    parsed_unit = ut_parse(_UnitSystem, from_units, UT_ASCII);
    if (parsed_unit) {
        return(parsed_unit);
    }

    for (mi = 0; CDSBadUnitsMap[mi].bad; ++mi) {

        if (strcmp(from_units, CDSBadUnitsMap[mi].bad) == 0) {

            parsed_unit = ut_parse(_UnitSystem, CDSBadUnitsMap[mi].good, UT_ASCII);
            if (parsed_unit) {
                return(parsed_unit);
            }

            /* Bad unit in mapping table */

            status = ut_get_status();

            UDUNITS_ERROR( CDS_LIB_NAME, status,
                "Bad units in mapping table, mapping '%s' to '%s'\n",
                from_units, CDSBadUnitsMap[mi].good);

            return((ut_unit *)NULL);
        }
    }

    /* Bad unit not found in mapping table */

    status = ut_get_status();

    UDUNITS_ERROR( CDS_LIB_NAME, status,
        "Could not parse from_units string: '%s'\n", from_units);

    return((ut_unit *)NULL);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Check if two units are equal.
 *
 *  This function will load the default UDUNITS-2 unit system if a unit
 *  system has not been previously loaded (see cds_init_unit_system()).
 *  It is the responsibility of the calling process to free the memory
 *  used by the unit system by calling cds_free_unit_system() when no
 *  more unit conversions are necessary.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  from_units - units to convert from
 *  @param  to_units   - units to convert to
 *
 *  @return
 *    -  1 if the units are not equal
 *    -  0 if the units are equal
 *    - -1 if an error occurred
 */
int cds_compare_units(
    const char *from_units,
    const char *to_units)
{
    ut_unit    *from;
    ut_unit    *to;
    ut_status   status;

    /* Check if the unit strings are equal */

    if (strcmp(from_units, to_units) == 0) {
        return(0);
    }

    /* Load the default units system if one has not already been loaded */

    if (!_UnitSystem) {
        if (!cds_init_unit_system(NULL)) {
            return(-1);
        }
    }

    /* Parse from_units string */

    from = _cds_ut_parse_from_units(from_units);
    if (!from) {
        return(-1);
    }

    /* Parse to_units string */

    to = ut_parse(_UnitSystem, to_units, UT_ASCII);

    if (!to) {

        status = ut_get_status();

        UDUNITS_ERROR( CDS_LIB_NAME, status,
            "Could not parse to_units string: '%s'\n", to_units);

        ut_free(from);

        return(-1);
    }

    /* Check if the units are equal */

    status = ut_compare(from, to);

    ut_free(from);
    ut_free(to);

    if (status == 0) {
        return(0);
    }

    return(1);
}

/**
 *  Convert data values from one unit to another.
 *
 *  Memory will be allocated for the output data array if the out_data argument
 *  is NULL. In this case the calling process is responsible for freeing the
 *  allocated memory.
 *
 *  The input and output data arrays can be identical if the size of the input
 *  data type is greater than or equal to the size of the output data type.
 *
 *  The mapping variables can be used to copy values from the input array to
 *  the output array without performing the unit conversion. All values
 *  specified in the input map array will be replaced with the corresponding
 *  value in the output map array.
 *
 *  The range variables can be used to replace all values outside a specified
 *  range with a less-than-min or a greater-than-max value. If an out-of-range
 *  value is specified but the corresponding min/max value is not, the valid
 *  min/max value of the output data type will be used if necessary.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  converter - units converter
 *  @param  in_type   - data type of the input data
 *  @param  length    - number of values in the data arrays
 *  @param  in_data   - pointer to the input data array
 *  @param  out_type  - data type of the output data
 *  @param  out_data  - pointer to the output data array
 *                      (memory will allocated if this argument is NULL)
 *
 *  @param  nmap      - number of mapping values
 *  @param  in_map    - pointer to the array of input map values
 *  @param  out_map   - pointer to the array of output map values
 *
 *  @param  out_min   - pointer to the minimum value, or NULL to use the
 *                      minimum value of the output data type if this is
 *                      greater than the minimum value of the input data type.
 *
 *  @param  orv_min   - pointer to the value to use for values less than min,
 *                      or NULL to disable the min value check.
 *
 *  @param  out_max   - pointer to the maximum value, or NULL to use the
 *                      maximum value of the output data type if this is
 *                      less than the maximum value of the input data type.
 *
 *  @param  orv_max   - pointer to the value to use for values greater than max,
 *                      or NULL to disable the max value check.
 *
 *  @return
 *    - pointer to the output data array
 *    - NULL if attempting to convert between string and numbers,
 *           or attempting to convert units for string values,
 *           or a memory allocation error occurs (this can only happen
 *           if the specified out_data argument is NULL)
 */
void *cds_convert_units(
    CDSUnitConverter  converter,
    CDSDataType       in_type,
    size_t            length,
    void             *in_data,
    CDSDataType       out_type,
    void             *out_data,
    size_t            nmap,
    void             *in_map,
    void             *out_map,
    void             *out_min,
    void             *orv_min,
    void             *out_max,
    void             *orv_max)
{
    cv_converter *uc = (cv_converter *)converter;
    CDSData       in;
    CDSData       out;
    CDSData       imap;
    CDSData       omap;
    CDSData       min;
    CDSData       max;
    CDSData       ormin;
    CDSData       ormax;
    size_t        out_size;

    /* Check for CDS_STRING types */

    if (in_type == CDS_STRING) {

        if (out_type != CDS_STRING) {
            ERROR( CDS_LIB_NAME,
                "Attempt to convert between strings and numbers in cds_convert_units\n");
            return((void *)NULL);
        }

        if (converter) {
            ERROR( CDS_LIB_NAME,
                "Attempt to convert units for string values in cds_convert_units\n");
            return((void *)NULL);
        }
    }
    else if (out_type == CDS_STRING) {
        ERROR( CDS_LIB_NAME,
            "Attempt to convert between strings and numbers in cds_convert_units\n");
        return((void *)NULL);
    }

    /* Check if a converter was specified */

    if (!converter) {
        return(cds_copy_array(
            in_type, length, in_data, out_type, out_data,
            nmap, in_map, out_map, out_min, orv_min, out_max, orv_max));
    }

    /* Allocate memory for the output array if one was not specified */

    if (!out_data) {
        out_size = cds_data_type_size(out_type);
        out_data = malloc(length * out_size);
        if (!out_data) {
            ERROR( CDS_LIB_NAME,
                "Memory allocation error creating '%s' array of length %lu\n",
                cds_data_type_name(out_type), length);
            return((void *)NULL);
        }
    }

    /* Set CDSData pointers */

    in.vp    = in_data;
    out.vp   = out_data;
    imap.vp  = in_map;
    omap.vp  = out_map;
    min.vp   = out_min;
    ormin.vp = orv_min;
    max.vp   = out_max;
    ormax.vp = orv_max;

    /* Adjust range checking values */

    if (orv_min && !out_min) {
        min.vp = _cds_data_type_min(out_type);
    }

    if (orv_max && !out_max) {
        max.vp = _cds_data_type_max(out_type);
    }

    /* Do the unit conversion */

    switch (in_type) {
      case CDS_BYTE:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.bp, nmap, imap.bp, signed char, out.bp, omap.bp, min.bp, ormin.bp, max.bp, ormax.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.bp, nmap, imap.bp, char,        out.cp, omap.cp, min.cp, ormin.cp, max.cp, ormax.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.bp, nmap, imap.bp, short,       out.sp, omap.sp, min.sp, ormin.sp, max.sp, ormax.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_UNITS_DOUBLE(uc, length, in.bp, nmap, imap.bp, int,         out.ip, omap.ip, min.ip, ormin.ip, max.ip, ormax.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_UNITS_FLOAT (uc, length, in.bp, nmap, imap.bp, float,       out.fp, omap.fp, min.fp, ormin.fp, max.fp, ormax.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.bp, nmap, imap.bp, double,      out.dp, omap.dp, min.dp, ormin.dp, max.dp, ormax.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.bp, nmap, imap.bp, long long,   out.i64p, omap.i64p, min.i64p, ormin.i64p, max.i64p, ormax.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.bp, nmap, imap.bp, unsigned char,  out.ubp, omap.ubp, min.ubp, ormin.ubp, max.ubp, ormax.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.bp, nmap, imap.bp, unsigned short, out.usp, omap.usp, min.usp, ormin.usp, max.usp, ormax.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.bp, nmap, imap.bp, unsigned int,   out.uip, omap.uip, min.uip, ormin.uip, max.uip, ormax.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.bp, nmap, imap.bp, unsigned long long, out.ui64p, omap.ui64p, min.ui64p, ormin.ui64p, max.ui64p, ormax.ui64p, 1); break;
          default:
            break;
        }
        break;
      case CDS_CHAR:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.cp, nmap, imap.cp, signed char, out.bp, omap.bp, min.bp, ormin.bp, max.bp, ormax.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.cp, nmap, imap.cp, char,        out.cp, omap.cp, min.cp, ormin.cp, max.cp, ormax.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.cp, nmap, imap.cp, short,       out.sp, omap.sp, min.sp, ormin.sp, max.sp, ormax.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_UNITS_DOUBLE(uc, length, in.cp, nmap, imap.cp, int,         out.ip, omap.ip, min.ip, ormin.ip, max.ip, ormax.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_UNITS_FLOAT (uc, length, in.cp, nmap, imap.cp, float,       out.fp, omap.fp, min.fp, ormin.fp, max.fp, ormax.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.cp, nmap, imap.cp, double,      out.dp, omap.dp, min.dp, ormin.dp, max.dp, ormax.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.cp, nmap, imap.cp, long long,   out.i64p, omap.i64p, min.i64p, ormin.i64p, max.i64p, ormax.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.cp, nmap, imap.cp, unsigned char,  out.ubp, omap.ubp, min.ubp, ormin.ubp, max.ubp, ormax.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.cp, nmap, imap.cp, unsigned short, out.usp, omap.usp, min.usp, ormin.usp, max.usp, ormax.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.cp, nmap, imap.cp, unsigned int,   out.uip, omap.uip, min.uip, ormin.uip, max.uip, ormax.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.cp, nmap, imap.cp, unsigned long long, out.ui64p, omap.ui64p, min.ui64p, ormin.ui64p, max.ui64p, ormax.ui64p, 1); break;
          default:
            break;
        }
        break;
      case CDS_SHORT:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.sp, nmap, imap.sp, signed char, out.bp, omap.bp, min.bp, ormin.bp, max.bp, ormax.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.sp, nmap, imap.sp, char,        out.cp, omap.cp, min.cp, ormin.cp, max.cp, ormax.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.sp, nmap, imap.sp, short,       out.sp, omap.sp, min.sp, ormin.sp, max.sp, ormax.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_UNITS_DOUBLE(uc, length, in.sp, nmap, imap.sp, int,         out.ip, omap.ip, min.ip, ormin.ip, max.ip, ormax.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_UNITS_FLOAT (uc, length, in.sp, nmap, imap.sp, float,       out.fp, omap.fp, min.fp, ormin.fp, max.fp, ormax.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.sp, nmap, imap.sp, double,      out.dp, omap.dp, min.dp, ormin.dp, max.dp, ormax.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.sp, nmap, imap.sp, long long,   out.i64p, omap.i64p, min.i64p, ormin.i64p, max.i64p, ormax.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.sp, nmap, imap.sp, unsigned char,  out.ubp, omap.ubp, min.ubp, ormin.ubp, max.ubp, ormax.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.sp, nmap, imap.sp, unsigned short, out.usp, omap.usp, min.usp, ormin.usp, max.usp, ormax.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.sp, nmap, imap.sp, unsigned int,   out.uip, omap.uip, min.uip, ormin.uip, max.uip, ormax.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.sp, nmap, imap.sp, unsigned long long, out.ui64p, omap.ui64p, min.ui64p, ormin.ui64p, max.ui64p, ormax.ui64p, 1); break;
          default:
            break;
        }
        break;
      case CDS_INT:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ip, nmap, imap.ip, signed char, out.bp, omap.bp, min.bp, ormin.bp, max.bp, ormax.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ip, nmap, imap.ip, char,        out.cp, omap.cp, min.cp, ormin.cp, max.cp, ormax.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ip, nmap, imap.ip, short,       out.sp, omap.sp, min.sp, ormin.sp, max.sp, ormax.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ip, nmap, imap.ip, int,         out.ip, omap.ip, min.ip, ormin.ip, max.ip, ormax.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ip, nmap, imap.ip, float,       out.fp, omap.fp, min.fp, ormin.fp, max.fp, ormax.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ip, nmap, imap.ip, double,      out.dp, omap.dp, min.dp, ormin.dp, max.dp, ormax.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ip, nmap, imap.ip, long long,   out.i64p, omap.i64p, min.i64p, ormin.i64p, max.i64p, ormax.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ip, nmap, imap.ip, unsigned char,  out.ubp, omap.ubp, min.ubp, ormin.ubp, max.ubp, ormax.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ip, nmap, imap.ip, unsigned short, out.usp, omap.usp, min.usp, ormin.usp, max.usp, ormax.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ip, nmap, imap.ip, unsigned int,   out.uip, omap.uip, min.uip, ormin.uip, max.uip, ormax.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ip, nmap, imap.ip, unsigned long long, out.ui64p, omap.ui64p, min.ui64p, ormin.ui64p, max.ui64p, ormax.ui64p, 1); break;
          default:
            break;
        }
        break;
      case CDS_FLOAT:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.fp, nmap, imap.fp, signed char, out.bp, omap.bp, min.bp, ormin.bp, max.bp, ormax.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.fp, nmap, imap.fp, char,        out.cp, omap.cp, min.cp, ormin.cp, max.cp, ormax.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.fp, nmap, imap.fp, short,       out.sp, omap.sp, min.sp, ormin.sp, max.sp, ormax.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_UNITS_DOUBLE(uc, length, in.fp, nmap, imap.fp, int,         out.ip, omap.ip, min.ip, ormin.ip, max.ip, ormax.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_UNITS_FLOAT (uc, length, in.fp, nmap, imap.fp, float,       out.fp, omap.fp, min.fp, ormin.fp, max.fp, ormax.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.fp, nmap, imap.fp, double,      out.dp, omap.dp, min.dp, ormin.dp, max.dp, ormax.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.fp, nmap, imap.fp, long long,   out.i64p, omap.i64p, min.i64p, ormin.i64p, max.i64p, ormax.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.fp, nmap, imap.fp, unsigned char,  out.ubp, omap.ubp, min.ubp, ormin.ubp, max.ubp, ormax.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.fp, nmap, imap.fp, unsigned short, out.usp, omap.usp, min.usp, ormin.usp, max.usp, ormax.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.fp, nmap, imap.fp, unsigned int,   out.uip, omap.uip, min.uip, ormin.uip, max.uip, ormax.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.fp, nmap, imap.fp, unsigned long long, out.ui64p, omap.ui64p, min.ui64p, ormin.ui64p, max.ui64p, ormax.ui64p, 1); break;
          default:
            break;
        }
        break;
      case CDS_DOUBLE:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.dp, nmap, imap.dp, signed char, out.bp, omap.bp, min.bp, ormin.bp, max.bp, ormax.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.dp, nmap, imap.dp, char,        out.cp, omap.cp, min.cp, ormin.cp, max.cp, ormax.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.dp, nmap, imap.dp, short,       out.sp, omap.sp, min.sp, ormin.sp, max.sp, ormax.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_UNITS_DOUBLE(uc, length, in.dp, nmap, imap.dp, int,         out.ip, omap.ip, min.ip, ormin.ip, max.ip, ormax.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.dp, nmap, imap.dp, float,       out.fp, omap.fp, min.fp, ormin.fp, max.fp, ormax.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.dp, nmap, imap.dp, double,      out.dp, omap.dp, min.dp, ormin.dp, max.dp, ormax.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.dp, nmap, imap.dp, long long,   out.i64p, omap.i64p, min.i64p, ormin.i64p, max.i64p, ormax.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.dp, nmap, imap.dp, unsigned char,  out.ubp, omap.ubp, min.ubp, ormin.ubp, max.ubp, ormax.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.dp, nmap, imap.dp, unsigned short, out.usp, omap.usp, min.usp, ormin.usp, max.usp, ormax.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.dp, nmap, imap.dp, unsigned int,   out.uip, omap.uip, min.uip, ormin.uip, max.uip, ormax.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.dp, nmap, imap.dp, unsigned long long, out.ui64p, omap.ui64p, min.ui64p, ormin.ui64p, max.ui64p, ormax.ui64p, 1); break;
          default:
            break;
        }
        break;
      /* NetCDF4 extended data types */
      case CDS_INT64:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, signed char, out.bp, omap.bp, min.bp, ormin.bp, max.bp, ormax.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, char,        out.cp, omap.cp, min.cp, ormin.cp, max.cp, ormax.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, short,       out.sp, omap.sp, min.sp, ormin.sp, max.sp, ormax.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_UNITS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, int,         out.ip, omap.ip, min.ip, ormin.ip, max.ip, ormax.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, float,       out.fp, omap.fp, min.fp, ormin.fp, max.fp, ormax.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, double,      out.dp, omap.dp, min.dp, ormin.dp, max.dp, ormax.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, long long,   out.i64p, omap.i64p, min.i64p, ormin.i64p, max.i64p, ormax.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, unsigned char,  out.ubp, omap.ubp, min.ubp, ormin.ubp, max.ubp, ormax.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, unsigned short, out.usp, omap.usp, min.usp, ormin.usp, max.usp, ormax.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, unsigned int,   out.uip, omap.uip, min.uip, ormin.uip, max.uip, ormax.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, unsigned long long, out.ui64p, omap.ui64p, min.ui64p, ormin.ui64p, max.ui64p, ormax.ui64p, 1); break;
          default:
            break;
        }
        break;
      case CDS_UBYTE:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, signed char, out.bp, omap.bp, min.bp, ormin.bp, max.bp, ormax.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, char,        out.cp, omap.cp, min.cp, ormin.cp, max.cp, ormax.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, short,       out.sp, omap.sp, min.sp, ormin.sp, max.sp, ormax.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, int,         out.ip, omap.ip, min.ip, ormin.ip, max.ip, ormax.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_UNITS_FLOAT (uc, length, in.ubp, nmap, imap.ubp, float,       out.fp, omap.fp, min.fp, ormin.fp, max.fp, ormax.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, double,      out.dp, omap.dp, min.dp, ormin.dp, max.dp, ormax.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, long long,   out.i64p, omap.i64p, min.i64p, ormin.i64p, max.i64p, ormax.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, unsigned char,  out.ubp, omap.ubp, min.ubp, ormin.ubp, max.ubp, ormax.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, unsigned short, out.usp, omap.usp, min.usp, ormin.usp, max.usp, ormax.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, unsigned int,   out.uip, omap.uip, min.uip, ormin.uip, max.uip, ormax.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, unsigned long long, out.ui64p, omap.ui64p, min.ui64p, ormin.ui64p, max.ui64p, ormax.ui64p, 1); break;
          default:
            break;
        }
        break;
      case CDS_USHORT:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.usp, nmap, imap.usp, signed char, out.bp, omap.bp, min.bp, ormin.bp, max.bp, ormax.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.usp, nmap, imap.usp, char,        out.cp, omap.cp, min.cp, ormin.cp, max.cp, ormax.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.usp, nmap, imap.usp, short,       out.sp, omap.sp, min.sp, ormin.sp, max.sp, ormax.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_UNITS_DOUBLE(uc, length, in.usp, nmap, imap.usp, int,         out.ip, omap.ip, min.ip, ormin.ip, max.ip, ormax.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_UNITS_FLOAT (uc, length, in.usp, nmap, imap.usp, float,       out.fp, omap.fp, min.fp, ormin.fp, max.fp, ormax.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.usp, nmap, imap.usp, double,      out.dp, omap.dp, min.dp, ormin.dp, max.dp, ormax.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.usp, nmap, imap.usp, long long,   out.i64p, omap.i64p, min.i64p, ormin.i64p, max.i64p, ormax.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.usp, nmap, imap.usp, unsigned char,  out.ubp, omap.ubp, min.ubp, ormin.ubp, max.ubp, ormax.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.usp, nmap, imap.usp, unsigned short, out.usp, omap.usp, min.usp, ormin.usp, max.usp, ormax.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.usp, nmap, imap.usp, unsigned int,   out.uip, omap.uip, min.uip, ormin.uip, max.uip, ormax.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.usp, nmap, imap.usp, unsigned long long, out.ui64p, omap.ui64p, min.ui64p, ormin.ui64p, max.ui64p, ormax.ui64p, 1); break;
          default:
            break;
        }
        break;
      case CDS_UINT:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.uip, nmap, imap.uip, signed char, out.bp, omap.bp, min.bp, ormin.bp, max.bp, ormax.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.uip, nmap, imap.uip, char,        out.cp, omap.cp, min.cp, ormin.cp, max.cp, ormax.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.uip, nmap, imap.uip, short,       out.sp, omap.sp, min.sp, ormin.sp, max.sp, ormax.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_UNITS_DOUBLE(uc, length, in.uip, nmap, imap.uip, int,         out.ip, omap.ip, min.ip, ormin.ip, max.ip, ormax.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.uip, nmap, imap.uip, float,       out.fp, omap.fp, min.fp, ormin.fp, max.fp, ormax.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.uip, nmap, imap.uip, double,      out.dp, omap.dp, min.dp, ormin.dp, max.dp, ormax.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.uip, nmap, imap.uip, long long,   out.i64p, omap.i64p, min.i64p, ormin.i64p, max.i64p, ormax.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.uip, nmap, imap.uip, unsigned char,  out.ubp, omap.ubp, min.ubp, ormin.ubp, max.ubp, ormax.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.uip, nmap, imap.uip, unsigned short, out.usp, omap.usp, min.usp, ormin.usp, max.usp, ormax.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.uip, nmap, imap.uip, unsigned int,   out.uip, omap.uip, min.uip, ormin.uip, max.uip, ormax.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.uip, nmap, imap.uip, unsigned long long, out.ui64p, omap.ui64p, min.ui64p, ormin.ui64p, max.ui64p, ormax.ui64p, 1); break;
          default:
            break;
        }
        break;
      case CDS_UINT64:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, signed char, out.bp, omap.bp, min.bp, ormin.bp, max.bp, ormax.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, char,        out.cp, omap.cp, min.cp, ormin.cp, max.cp, ormax.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, short,       out.sp, omap.sp, min.sp, ormin.sp, max.sp, ormax.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, int,         out.ip, omap.ip, min.ip, ormin.ip, max.ip, ormax.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, float,       out.fp, omap.fp, min.fp, ormin.fp, max.fp, ormax.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, double,      out.dp, omap.dp, min.dp, ormin.dp, max.dp, ormax.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, long long,   out.i64p, omap.i64p, min.i64p, ormin.i64p, max.i64p, ormax.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, unsigned char,  out.ubp, omap.ubp, min.ubp, ormin.ubp, max.ubp, ormax.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, unsigned short, out.usp, omap.usp, min.usp, ormin.usp, max.usp, ormax.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, unsigned int,   out.uip, omap.uip, min.uip, ormin.uip, max.uip, ormax.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_UNITS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, unsigned long long, out.ui64p, omap.ui64p, min.ui64p, ormin.ui64p, max.ui64p, ormax.ui64p, 1); break;
          default:
            break;
        }
        break;

      default:
        break;
    }

    return(out_data);
}

/**
 *  Convert data values from one unit to another.
 *
 *  This function will apply the units conversion by subtracting the
 *  value converted to the new units from twice the value converted
 *  to the new units.
 *
 *  Memory will be allocated for the output data array if the out_data argument
 *  is NULL. In this case the calling process is responsible for freeing the
 *  allocated memory.
 *
 *  The input and output data arrays can be identical if the size of the input
 *  data type is greater than or equal to the size of the output data type.
 *
 *  The mapping variables can be used to copy values from the input array to
 *  the output array without performing the unit conversion. All values
 *  specified in the input map array will be replaced with the corresponding
 *  value in the output map array.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  converter - units converter
 *  @param  in_type   - data type of the input data
 *  @param  length    - number of values in the data arrays
 *  @param  in_data   - pointer to the input data array
 *  @param  out_type  - data type of the output data
 *  @param  out_data  - pointer to the output data array
 *                      (memory will allocated if this argument is NULL)
 *
 *  @param  nmap      - number of mapping values
 *  @param  in_map    - pointer to the array of input map values
 *  @param  out_map   - pointer to the array of output map values
 *
 *  @return
 *    - pointer to the output data array
 *    - NULL if attempting to convert between string and numbers,
 *           or attempting to convert units for string values,
 *           or a memory allocation error occurs (this can only happen
 *           if the specified out_data argument is NULL)
 */
void *cds_convert_unit_deltas(
    CDSUnitConverter  converter,
    CDSDataType       in_type,
    size_t            length,
    void             *in_data,
    CDSDataType       out_type,
    void             *out_data,
    size_t            nmap,
    void             *in_map,
    void             *out_map)
{
    cv_converter *uc = (cv_converter *)converter;
    CDSData in;
    CDSData out;
    CDSData imap;
    CDSData omap;
    size_t  out_size;

    /* Check for CDS_STRING types */

    if (in_type == CDS_STRING) {

        if (out_type != CDS_STRING) {
            ERROR( CDS_LIB_NAME,
                "Attempt to convert between strings and numbers in cds_convert_unit_deltas\n");
            return((void *)NULL);
        }

        if (converter) {
            ERROR( CDS_LIB_NAME,
                "Attempt to convert units for string values in cds_convert_unit_deltas\n");
            return((void *)NULL);
        }
    }
    else if (out_type == CDS_STRING) {
        ERROR( CDS_LIB_NAME,
            "Attempt to convert between strings and numbers in cds_convert_unit_deltas\n");
        return((void *)NULL);
    }

    /* Check if a converter was specified */

    if (!converter) {
        return(cds_copy_array(
            in_type, length, in_data, out_type, out_data,
            nmap, in_map, out_map, NULL, NULL, NULL, NULL));
    }

    /* Allocate memory for the output array if one was not specified */

    if (!out_data) {
        out_size = cds_data_type_size(out_type);
        out_data = malloc(length * out_size);
        if (!out_data) {
            return((void *)NULL);
        }
    }

    /* Do the unit conversion */

    in.vp   = in_data;
    out.vp  = out_data;
    imap.vp = in_map;
    omap.vp = out_map;

    switch (in_type) {
      case CDS_BYTE:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.bp, nmap, imap.bp, signed char, out.bp, omap.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.bp, nmap, imap.bp, char,        out.cp, omap.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.bp, nmap, imap.bp, short,       out.sp, omap.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.bp, nmap, imap.bp, int,         out.ip, omap.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_DELTAS_FLOAT (uc, length, in.bp, nmap, imap.bp, float,       out.fp, omap.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.bp, nmap, imap.bp, double,      out.dp, omap.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.bp, nmap, imap.bp, long long,   out.i64p, omap.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.bp, nmap, imap.bp, unsigned char,  out.ubp, omap.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.bp, nmap, imap.bp, unsigned short, out.usp, omap.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.bp, nmap, imap.bp, unsigned int,   out.uip, omap.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.bp, nmap, imap.bp, unsigned long long, out.ui64p, omap.ui64p, 1); break;
          default:
            break;
        }
        break;
      case CDS_CHAR:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.cp, nmap, imap.cp, signed char, out.bp, omap.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.cp, nmap, imap.cp, char,        out.cp, omap.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.cp, nmap, imap.cp, short,       out.sp, omap.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.cp, nmap, imap.cp, int,         out.ip, omap.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_DELTAS_FLOAT (uc, length, in.cp, nmap, imap.cp, float,       out.fp, omap.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.cp, nmap, imap.cp, double,      out.dp, omap.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.cp, nmap, imap.cp, long long,   out.i64p, omap.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.cp, nmap, imap.cp, unsigned char,  out.ubp, omap.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.cp, nmap, imap.cp, unsigned short, out.usp, omap.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.cp, nmap, imap.cp, unsigned int,   out.uip, omap.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.cp, nmap, imap.cp, unsigned long long, out.ui64p, omap.ui64p, 1); break;
          default:
            break;
        }
        break;
      case CDS_SHORT:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.sp, nmap, imap.sp, signed char, out.bp, omap.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.sp, nmap, imap.sp, char,        out.cp, omap.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.sp, nmap, imap.sp, short,       out.sp, omap.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.sp, nmap, imap.sp, int,         out.ip, omap.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_DELTAS_FLOAT (uc, length, in.sp, nmap, imap.sp, float,       out.fp, omap.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.sp, nmap, imap.sp, double,      out.dp, omap.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.sp, nmap, imap.sp, long long,   out.i64p, omap.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.sp, nmap, imap.sp, unsigned char,  out.ubp, omap.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.sp, nmap, imap.sp, unsigned short, out.usp, omap.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.sp, nmap, imap.sp, unsigned int,   out.uip, omap.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.sp, nmap, imap.sp, unsigned long long, out.ui64p, omap.ui64p, 1); break;
          default:
            break;
        }
        break;
      case CDS_INT:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ip, nmap, imap.ip, signed char, out.bp, omap.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ip, nmap, imap.ip, char,        out.cp, omap.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ip, nmap, imap.ip, short,       out.sp, omap.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ip, nmap, imap.ip, int,         out.ip, omap.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_DELTAS_FLOAT (uc, length, in.ip, nmap, imap.ip, float,       out.fp, omap.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ip, nmap, imap.ip, double,      out.dp, omap.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ip, nmap, imap.ip, long long,   out.i64p, omap.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ip, nmap, imap.ip, unsigned char,  out.ubp, omap.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ip, nmap, imap.ip, unsigned short, out.usp, omap.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ip, nmap, imap.ip, unsigned int,   out.uip, omap.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ip, nmap, imap.ip, unsigned long long, out.ui64p, omap.ui64p, 1); break;
          default:
            break;
        }
        break;
      case CDS_FLOAT:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.fp, nmap, imap.fp, signed char, out.bp, omap.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.fp, nmap, imap.fp, char,        out.cp, omap.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.fp, nmap, imap.fp, short,       out.sp, omap.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.fp, nmap, imap.fp, int,         out.ip, omap.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_DELTAS_FLOAT (uc, length, in.fp, nmap, imap.fp, float,       out.fp, omap.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.fp, nmap, imap.fp, double,      out.dp, omap.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.fp, nmap, imap.fp, long long,   out.i64p, omap.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.fp, nmap, imap.fp, unsigned char,  out.ubp, omap.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.fp, nmap, imap.fp, unsigned short, out.usp, omap.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.fp, nmap, imap.fp, unsigned int,   out.uip, omap.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.fp, nmap, imap.fp, unsigned long long, out.ui64p, omap.ui64p, 1); break;
          default:
            break;
        }
        break;
      case CDS_DOUBLE:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.dp, nmap, imap.dp, signed char, out.bp, omap.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.dp, nmap, imap.dp, char,        out.cp, omap.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.dp, nmap, imap.dp, short,       out.sp, omap.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.dp, nmap, imap.dp, int,         out.ip, omap.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.dp, nmap, imap.dp, float,       out.fp, omap.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.dp, nmap, imap.dp, double,      out.dp, omap.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.dp, nmap, imap.dp, long long,   out.i64p, omap.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.dp, nmap, imap.dp, unsigned char,  out.ubp, omap.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.dp, nmap, imap.dp, unsigned short, out.usp, omap.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.dp, nmap, imap.dp, unsigned int,   out.uip, omap.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.dp, nmap, imap.dp, unsigned long long, out.ui64p, omap.ui64p, 1); break;
          default:
            break;
        }
        break;
      /* NetCDF4 extended data types */
      case CDS_INT64:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, signed char, out.bp, omap.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, char,        out.cp, omap.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, short,       out.sp, omap.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, int,         out.ip, omap.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_DELTAS_FLOAT (uc, length, in.i64p, nmap, imap.i64p, float,       out.fp, omap.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, double,      out.dp, omap.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, long long,   out.i64p, omap.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, unsigned char,  out.ubp, omap.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, unsigned short, out.usp, omap.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, unsigned int,   out.uip, omap.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.i64p, nmap, imap.i64p, unsigned long long, out.ui64p, omap.ui64p, 1); break;
          default:
            break;
        }
        break;
      case CDS_UBYTE:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, signed char, out.bp, omap.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, char,        out.cp, omap.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, short,       out.sp, omap.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, int,         out.ip, omap.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_DELTAS_FLOAT (uc, length, in.ubp, nmap, imap.ubp, float,       out.fp, omap.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, double,      out.dp, omap.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, long long,   out.i64p, omap.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, unsigned char,  out.ubp, omap.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, unsigned short, out.usp, omap.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, unsigned int,   out.uip, omap.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ubp, nmap, imap.ubp, unsigned long long, out.ui64p, omap.ui64p, 1); break;
          default:
            break;
        }
        break;
      case CDS_USHORT:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.usp, nmap, imap.usp, signed char, out.bp, omap.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.usp, nmap, imap.usp, char,        out.cp, omap.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.usp, nmap, imap.usp, short,       out.sp, omap.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.usp, nmap, imap.usp, int,         out.ip, omap.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_DELTAS_FLOAT (uc, length, in.usp, nmap, imap.usp, float,       out.fp, omap.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.usp, nmap, imap.usp, double,      out.dp, omap.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.usp, nmap, imap.usp, long long,   out.i64p, omap.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.usp, nmap, imap.usp, unsigned char,  out.ubp, omap.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.usp, nmap, imap.usp, unsigned short, out.usp, omap.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.usp, nmap, imap.usp, unsigned int,   out.uip, omap.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.usp, nmap, imap.usp, unsigned long long, out.ui64p, omap.ui64p, 1); break;
          default:
            break;
        }
        break;
      case CDS_UINT:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.uip, nmap, imap.uip, signed char, out.bp, omap.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.uip, nmap, imap.uip, char,        out.cp, omap.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.uip, nmap, imap.uip, short,       out.sp, omap.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.uip, nmap, imap.uip, int,         out.ip, omap.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_DELTAS_FLOAT (uc, length, in.uip, nmap, imap.uip, float,       out.fp, omap.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.uip, nmap, imap.uip, double,      out.dp, omap.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.uip, nmap, imap.uip, long long,   out.i64p, omap.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.uip, nmap, imap.uip, unsigned char,  out.ubp, omap.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.uip, nmap, imap.uip, unsigned short, out.usp, omap.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.uip, nmap, imap.uip, unsigned int,   out.uip, omap.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.uip, nmap, imap.uip, unsigned long long, out.ui64p, omap.ui64p, 1); break;
          default:
            break;
        }
        break;
      case CDS_UINT64:
        switch (out_type) {
          case CDS_BYTE:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, signed char, out.bp, omap.bp, 1); break;
          case CDS_CHAR:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, char,        out.cp, omap.cp, 1); break;
          case CDS_SHORT:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, short,       out.sp, omap.sp, 1); break;
          case CDS_INT:    CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, int,         out.ip, omap.ip, 1); break;
          case CDS_FLOAT:  CDS_CONVERT_DELTAS_FLOAT (uc, length, in.ui64p, nmap, imap.ui64p, float,       out.fp, omap.fp, 0); break;
          case CDS_DOUBLE: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, double,      out.dp, omap.dp, 0); break;
          /* NetCDF4 extended data types */
          case CDS_INT64:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, long long,   out.i64p, omap.i64p, 1); break;
          case CDS_UBYTE:  CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, unsigned char,  out.ubp, omap.ubp, 1); break;
          case CDS_USHORT: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, unsigned short, out.usp, omap.usp, 1); break;
          case CDS_UINT:   CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, unsigned int,   out.uip, omap.uip, 1); break;
          case CDS_UINT64: CDS_CONVERT_DELTAS_DOUBLE(uc, length, in.ui64p, nmap, imap.ui64p, unsigned long long, out.ui64p, omap.ui64p, 1); break;
          default:
            break;
        }
        break;
      default:
        break;
    }

    return(out_data);
}

/**
 *  Free a UDUNITS-2 unit converter.
 *
 *  @param  unit_converter - pointer to the units converter
 */
void cds_free_unit_converter(CDSUnitConverter unit_converter)
{
    if (unit_converter) {
        cv_free((cv_converter *)unit_converter);
    }
}

/**
 *  Free the UDUNITS-2 unit system.
 *
 *  This function will free the memory used by the UDUNITS-2 unit system.
 */
void cds_free_unit_system(void)
{
    _cds_free_symbols_map();

    if (_UnitSystem) {
        ut_free_system(_UnitSystem);
    }

    _UnitSystem = (ut_system *)NULL;
}

/**
 *  Get a UDUNITS-2 unit converter.
 *
 *  This function will load the default UDUNITS-2 unit system if a unit
 *  system has not been previously loaded (see cds_init_unit_system()).
 *  It is the responsibility of the calling process to free the memory
 *  used by the unit system by calling cds_free_unit_system() when no
 *  more unit conversions are necessary.
 *
 *  The memory used by the returned unit converter must also be freed
 *  by calling cds_free_unit_converter().
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  from_units     - units to convert from
 *  @param  to_units       - units to convert to
 *  @param  unit_converter - output: units converter
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the units are equal
 *    - -1 if an error occurred
 */
int cds_get_unit_converter(
    const char       *from_units,
    const char       *to_units,
    CDSUnitConverter *unit_converter)
{
    cv_converter *converter;
    ut_unit      *from;
    ut_unit      *to;
    ut_status     status;

    *unit_converter = (CDSUnitConverter)NULL;

    /* Check if the units are equal */

    if (strcmp(from_units, to_units) == 0) {
        return(0);
    }

    /* Load the default units system if one has not already been loaded */

    if (!_UnitSystem) {
        if (!cds_init_unit_system(NULL)) {
            return(-1);
        }
    }

    /* Parse from_units string */

    from = _cds_ut_parse_from_units(from_units);
    if (!from) {
        return(-1);
    }

    /* Parse to_units string */

    to = ut_parse(_UnitSystem, to_units, UT_ASCII);

    if (!to) {

        status = ut_get_status();

        UDUNITS_ERROR( CDS_LIB_NAME, status,
            "Could not parse to_units string: '%s'\n", to_units);

        ut_free(from);

        return(-1);
    }

    /* Check if the units are equal */

    if (ut_compare(from, to) == 0) {

        ut_free(from);
        ut_free(to);

        return(0);
    }

    /* Get units converter */

    converter = ut_get_converter(from, to);

    if (!converter) {

        status = ut_get_status();

        UDUNITS_ERROR( CDS_LIB_NAME, status,
            "Could not get units converter for: '%s' to '%s'\n",
            from_units, to_units);

        ut_free(from);
        ut_free(to);

        return(-1);
    }

    *unit_converter = (CDSUnitConverter)converter;

    ut_free(from);
    ut_free(to);

    return(1);
}

/**
 *  Initialize the UDUNITS-2 unit system.
 *
 *  This function will initialize the UDUNITS-2 unit system that will be
 *  used to do all unit conversions. The cds_free_unit_system() function
 *  should be called to free the memory used by the unit system when no
 *  more unit conversions are needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  xml_db_path - path to the XML database to use
 *                        or NULL to use the default UDUNITS-2 database.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int cds_init_unit_system(const char *xml_db_path)
{
    ut_status status;

    /* Free the current unit system if one has been initialized */

    if (_UnitSystem) {
        cds_free_unit_system();
    }

    /* Turn off warning and error messages from the UDUNITS-2 library */

    ut_set_error_message_handler(ut_ignore);

    /* Initialize the unit system */

    _UnitSystem = ut_read_xml(xml_db_path);

    if (!_UnitSystem) {

        status = ut_get_status();

        UDUNITS_ERROR( CDS_LIB_NAME, status,
            "Could not initialize udunits unit system\n");

        return(0);
    }

    /* Check if we need to map any symbols */

    if (_NumMapSymbols) {
        if (!_cds_map_symbols()) {
            cds_free_unit_system();
            return(0);
        }
    }

    return(1);
}

/**
 *  Map a symbol to a UDUNITS-2 unit.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  symbol - unit symbol
 *  @param  name   - unit name
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int cds_map_symbol_to_unit(const char *symbol, const char *name)
{
    ut_status  status;
    ut_unit   *unit;

    if (!_UnitSystem) {
        if (!_cds_add_symbol_to_map(symbol, name)) {
            return(0);
        }
        return(1);
    }

    /* Get the specified unit */

    unit = ut_parse(_UnitSystem, name, UT_ASCII);

    if (!unit) {

        status = ut_get_status();

        UDUNITS_ERROR( CDS_LIB_NAME, status,
            "Could not map symbol '%s' to unit: '%s'\n",
            symbol, name);

        return(0);
    }

    /* Unmap the symbol if it exists */

    status = ut_unmap_symbol_to_unit(_UnitSystem, symbol, UT_ASCII);
    if (status != UT_SUCCESS) {

        UDUNITS_ERROR( CDS_LIB_NAME, status,
            "Could not unmap symbol '%s' from unit system\n", symbol);

        return(0);
    }

    status = ut_map_symbol_to_unit(symbol, UT_ASCII, unit);

    ut_free(unit);

    if (status != UT_SUCCESS) {

        UDUNITS_ERROR( CDS_LIB_NAME, status,
            "Could not map symbol '%s' to unit: '%s'\n",
            symbol, name);

        return(0);
    }

    return(1);
}

/**
 *  Verify (and fix if possible) the format of a time units string.
 *
 *  This function will ensure that the time_units string can be converted
 *  to seconds since 1970-01-01 00:00:00 0:00 by the UDUNITS2 library. If
 *  UDUNITS2 does not support the specified time_units string but it does
 *  have a know format, the time_unit string will be updated as necessary.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  time_units - pointer to the time units string
 *
 *  @return
 *    -  time in seconds since 1970
 *    - -1 if the time units string is not valid and could not be fixed
 *    - -2 if an error occurred
 */
time_t cds_validate_time_units(char *time_units)
{
    const char   *secs1970_string = "seconds since 1970-01-01 00:00:00 0:00";
    ut_unit      *from;
    ut_unit      *to;
    cv_converter *converter;
    double        secs1970;
    ut_status     status;
    struct tm     gmt;
    char         *strp;
    char         *cp1;
    char         *cp2;
    int           length;
    int           found_space;

    /* Load the default units system if one has not already been loaded */

    if (!_UnitSystem) {
        if (!cds_init_unit_system(NULL)) {
            return(-2);
        }
    }

    /* Parse the seconds since 1970 string */

    to = ut_parse(_UnitSystem, secs1970_string, UT_ASCII);

    if (!to) {

        status = ut_get_status();

        UDUNITS_ERROR( CDS_LIB_NAME, status,
            "Could not parse seconds since 1970 string: '%s'\n",
            secs1970_string);

        return(-2);
    }

    /* Parse from_units string */

    from = ut_parse(_UnitSystem, time_units, UT_ASCII);

    if (!from ||
        !ut_are_convertible(from, to)) {

        if (from) ut_free(from);

        /* First check for white space at the end of the string */

        length = strlen(time_units);
        if (length == 0) {
            ut_free(to);
            return(-1);
        }

        found_space = 0;
        cp1 = &time_units[length-1];
        if (isspace(*cp1)) {
            found_space = 1;
            while (isspace(*cp1) && cp1 != time_units) *cp1-- = '\0';
        }

        /* Check for known formats that can be fixed */

        if ( (strp = strstr(time_units, "0:00 UTC")) ) {

            /* remove the redundant UTC at the end */

            strp += 4;
            *strp = '\0';
        }
        else if (sscanf(time_units,
            "seconds since %d-%d-%d, %d:%d:%d",
            &gmt.tm_year, &gmt.tm_mon, &gmt.tm_mday,
            &gmt.tm_hour, &gmt.tm_min, &gmt.tm_sec) == 6) {

            /* remove the comma */

            cp1 = strchr(time_units, ',');
            cp2 = cp1 + 1;
            while (*cp1 != '\0') *cp1++ = *cp2++;
        }
        else if (sscanf(time_units,
            "seconds since %d-%d-%dT%d:%d:%d",
            &gmt.tm_year, &gmt.tm_mon, &gmt.tm_mday,
            &gmt.tm_hour, &gmt.tm_min, &gmt.tm_sec) == 6) {

            /* replace the T and optional Z with a space */

            cp1  = strchr(time_units, 'T');
            *cp1 = ' ';
            cp2  = strchr(cp1, 'Z');
            if (cp2) *cp2 = ' ';
        }
        else if (sscanf(time_units,
            "seconds since %d/%d/%d",
            &gmt.tm_year, &gmt.tm_mon, &gmt.tm_mday) == 3) {

            /* replace slashes with dashes */

            cp1 = time_units;
            while ( (cp1 = strchr(cp1, '/')) ) *cp1++ = '-';
        }
        else if (!found_space) {
            ut_free(to);
            return(-1);
        }

        from = ut_parse(_UnitSystem, time_units, UT_ASCII);
        if (!from) {
            ut_free(to);
            return(-1);
        }
    }

    /* Get units converter */

    converter = ut_get_converter(from, to);

    if (!converter) {

        status = ut_get_status();

        UDUNITS_ERROR( CDS_LIB_NAME, status,
            "Could not get units converter:\n"
            "  - from: '%s'\n"
            "  - to:   '%s'\n",
            time_units, secs1970_string);

        ut_free(from);
        ut_free(to);

        return(-2);
    }

    secs1970 = cv_convert_double(converter, 0.0);

    cv_free(converter);
    ut_free(from);
    ut_free(to);

    return((time_t)secs1970);
}
