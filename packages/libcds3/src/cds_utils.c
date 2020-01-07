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
*    $Revision: 63557 $
*    $Author: ermold $
*    $Date: 2015-09-02 02:40:13 +0000 (Wed, 02 Sep 2015) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file cds_utils.c
 *  CDS Utility Functions.
 */

#include <math.h>

#include "cds3.h"
#include "cds_private.h"

#include <ctype.h>

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  PRIVATE: Recursion function used by cds_create_data_index().
 *
 *  @param  data       - pointer to the data array
 *  @param  type_size  - size (in bytes) of the data type
 *  @param  ndims      - number of dimensions in the data array
 *  @param  lengths    - lengths of the dimensions in the data array
 *  @param  dimid      - dimension index
 *  @param  counter    - dimension index counter
 *  @param  stride     - dimension strides
 *
 *  @returns
 *    - array of index pointers
 *    - NULL if a memory allocation error occurs
 *
 *  @see cds_create_data_index()
 */
static void *_cds_create_data_index(
    void   *data,
    size_t  type_size,
    int     ndims,
    size_t *lengths,
    int     dimid,
    size_t *counter,
    size_t *stride)
{
    void **index;
    size_t offset;
    size_t dc;
    int    di;

    index = (void **)malloc(lengths[dimid] * sizeof(void *));
    if (!index) {
        return((void *)NULL);
    }

    if (dimid < ndims-1) {
        for (dc = 0; dc < lengths[dimid]; dc++) {

            index[dc] = _cds_create_data_index(
                data, type_size, ndims, lengths, dimid+1, counter, stride);

            if (!index[dc]) {
                if (dc > 0) {
                    while (dc--) free(index[dc]);
                }
                free(index);
                return((void *)NULL);
            }

            counter[dimid]++;
        }
    }
    else {
        offset = 0;
        for (di = 0; di < ndims-1; di++) {
            offset += (counter[di] * stride[di]);
        }
        offset *= type_size;

        for (dc = 0; dc < lengths[dimid]; dc++) {

            index[dc] = (unsigned char *)data + offset;

            offset += stride[ndims-1] * type_size;

            counter[dimid]++;
        }
    }

    counter[dimid] = 0;

    return((void *)index);
}

/**
 *  PRIVATE: recursion function used by cds_free_index_array()
 *
 *  @param  index   - pointer to the data index
 *  @param  ndims   - number of dimensions in the data array
 *  @param  lengths - lengths of the dimensions in the data array
 *  @param  dimid   - dimension index
 *
 *  @see cds_free_index_array()
 */
static void _cds_free_data_index(
    void   *index,
    int     ndims,
    size_t *lengths,
    int     dimid)
{
    void **vpp = (void **)index;
    size_t dc;

    if (dimid < ndims-1) {
        for (dc = 0; dc < lengths[dimid]; dc++) {
            _cds_free_data_index(
                vpp[dc], ndims, lengths, dimid+1);
        }
    }

    free(index);
}

/**
 *  Get the open and close brackets to use when printing a data array.
 *
 *  @param  type  - data type of the array
 *  @param  flags - control flags:
 *                    - 0x01: Print data type name for numeric arrays.
 *                    - 0x02: Print padded data type name for numeric arrays.
 *                    - 0x04: Print data type name at end of numeric arrays.
 *                    - 0x08: Do not print brackets around numeric arrays.
 *  @param  open  - output: pointer to the open bracket string.
 *  @param  close - output: pointer to the close bracket string.
 */
static void _cds_get_array_brackets(
    CDSDataType  type,
    int          flags,
    const char **open,
    const char **close)
{
    *open  = (const char *)NULL;
    *close = (const char *)NULL;

    if (type == CDS_CHAR) {
        *open  = "\"";
        *close = "\"";
    }
    else if (flags & 0x04) {
        switch (type) {
            case CDS_BYTE:   *close = "]:byte"; break;
            case CDS_SHORT:  *close = "]:short"; break;
            case CDS_INT:    *close = "]:int"; break;
            case CDS_FLOAT:  *close = "]:float"; break;
            case CDS_DOUBLE: *close = "]:double"; break;
            default: return;
        }
        *open = "[";
    }
    else if (flags & 0x02) {
        switch (type) {
            case CDS_BYTE:   *open = "byte:  ["; break;
            case CDS_SHORT:  *open = "short: ["; break;
            case CDS_INT:    *open = "int:   ["; break;
            case CDS_FLOAT:  *open = "float: ["; break;
            case CDS_DOUBLE: *open = "double:["; break;
            default: return;
        }
        *close = "]";
    }
    else if (flags & 0x01) {
        switch (type) {
            case CDS_BYTE:   *open = "byte:["; break;
            case CDS_SHORT:  *open = "short:["; break;
            case CDS_INT:    *open = "int:["; break;
            case CDS_FLOAT:  *open = "float:["; break;
            case CDS_DOUBLE: *open = "double:["; break;
            default: return;
        }
        *close = "]";
    }
    else if (!(flags & 0x08)) {
        *open  = "[";
        *close = "]";
    }
}

/**
 *  Print an array of values to the specified buffer.
 *
 *  Note: bufsize must be greater than 31 for numeric data,
 *  and greater than maxline + 3 for character data.
 *
 *  @param  bufsize  - maximum size of the output buffer
 *  @param  buffer   - pointer to the output buffer
 *  @param  indexp   - input/output: current array index
 *  @param  type     - data type of the array
 *  @param  length   - length of the array
 *  @param  array    - pointer to the array of values
 *  @param  maxline  - maximum number of characters to print per line,
 *                     or 0 for no line breaks in numeric arrays and to only
 *                     split character arrays on newlines.
 *  @param  lineposp - input/output: current line position.
 *  @param  indent   - pointer to the indent string.
 *
 *  @return number of characters written to the buffer
 */
static size_t _cds_print_array_to_buffer(
    size_t       bufsize,
    char        *buffer,
    size_t      *indexp,
    CDSDataType  type,
    size_t       length,
    void        *array,
    size_t       maxline,
    size_t      *lineposp,
    const char  *indent)
{
    char   *bufp;
    CDSData data;
    size_t  index;
    size_t  linepos;

    if (*indexp >= length) {
        return(0);
    }

    bufp    = buffer;
    data.vp = array;
    index   = *indexp;
    linepos = (lineposp) ? *lineposp : 0;

    /* Print values */

    switch (type) {
        case CDS_CHAR:
        {
            size_t  count         = length - index + 1;
            size_t  indlen        = (indent) ? strlen(indent) : 0 ;
            char   *bufend        = bufp + bufsize - indlen - 8;
            char   *end_bufp      = (char *)NULL;
            size_t  end_count     = 0;
            size_t  end_linepos   = 0;
            char   *break_bufp    = (char *)NULL;
            char   *break_datap   = (char *)NULL;
            size_t  break_count   = 0;
            size_t  break_linepos = 0;
            int     found_newline = 0;
            size_t  nchars;
            char    uc;

            if (bufsize < maxline + 4) return(0);

            data.cp += index;

            if (maxline) {

                if (maxline < indlen + 3) {
                    maxline += indlen + 1;
                }
                else {
                    maxline -= 1;
                }

                while (--count && bufp < bufend) {

                    switch (uc = *data.cp++ & 0377) {
                        case '\0': memcpy(bufp, "\\0",  2); bufp += 2; linepos += 2; break;
                        case '\b': memcpy(bufp, "\\b",  2); bufp += 2; linepos += 2; break;
                        case '\f': memcpy(bufp, "\\f",  2); bufp += 2; linepos += 2; break;
                        case '\r': memcpy(bufp, "\\r",  2); bufp += 2; linepos += 2; break;
                        case '\v': memcpy(bufp, "\\v",  2); bufp += 2; linepos += 2; break;
                        case '\t': memcpy(bufp, "\\t",  2); bufp += 2; linepos += 2; break;
                        case '\\': memcpy(bufp, "\\\\", 2); bufp += 2; linepos += 2; break;
                        case '"':  memcpy(bufp, "\\\"", 2); bufp += 2; linepos += 2; break;
                        case '\n': memcpy(bufp, "\\n",  2); bufp += 2; linepos += 2;
                            found_newline = 1;
                            break;
                        case ' ':
                            if (linepos + 1 <= maxline) {
                                break_bufp    = bufp + 1;
                                break_datap   = data.cp;
                                break_count   = count;
                                break_linepos = linepos + 1;
                            }
                        default:
                            *bufp++ = uc;
                            linepos += 1;
                            break;
                    }

                    if (found_newline || linepos > maxline) {

                        if (linepos > maxline) {

                            if (break_bufp) {
                                bufp    = break_bufp;
                                count   = break_count;
                                data.cp = break_datap;
                                linepos = break_linepos;
                            }
                            else {
                                while (linepos > maxline) {
                                    nchars = (*(bufp-2) == '\\') ? 2 : 1;
                                    if (linepos - nchars < indlen + 2) break;
                                    count   += 1;
                                    data.cp -= 1;
                                    bufp    -= nchars;
                                    linepos -= nchars;
                                }
                            }
                        }

                        if (count > 1) {
                            memcpy(bufp, "\"\n", 2);
                            bufp += 2;
                            if (indlen) {
                                memcpy(bufp, indent, indlen);
                                bufp += indlen;
                            }
                            *bufp++ = '\"';
                            linepos = indlen + 1;

                            end_bufp    = bufp;
                            end_count   = count;
                            end_linepos = linepos;

                            break_bufp  = (char *)NULL;
                        }

                        found_newline = 0;
                    }
                }

                if (count && end_bufp) {
                    bufp    = end_bufp;
                    count   = end_count - 1;
                    linepos = end_linepos;
                }
            }
            else {
                while (--count && bufp < bufend) {
                    switch (uc = *data.cp++ & 0377) {
                        case '\0': memcpy(bufp, "\\0",  2); bufp += 2; break;
                        case '\b': memcpy(bufp, "\\b",  2); bufp += 2; break;
                        case '\f': memcpy(bufp, "\\f",  2); bufp += 2; break;
                        case '\r': memcpy(bufp, "\\r",  2); bufp += 2; break;
                        case '\v': memcpy(bufp, "\\v",  2); bufp += 2; break;
                        case '\t': memcpy(bufp, "\\t",  2); bufp += 2; break;
                        case '\\': memcpy(bufp, "\\\\", 2); bufp += 2; break;
                        case '"':  memcpy(bufp, "\\\"", 2); bufp += 2; break;
                        case '\n': memcpy(bufp, "\\n",  2); bufp += 2;
                            if (count > 1) {
                                memcpy(bufp, "\"\n", 2);
                                bufp += 2;
                                if (indlen) {
                                    memcpy(bufp, indent, indlen);
                                    bufp += indlen;
                                }
                                *bufp++ = '\"';
                            }
                            break;
                        default:
                            *bufp++ = uc;
                            break;
                    }
                }
            }

            index = length - count;
            *bufp = '\0';
        }
            break;
        case CDS_BYTE:    CDS_PRINT_TO_BUFFER(index, length, "%hhd",  data.bp, bufp, bufsize, maxline, linepos, indent); break;
        case CDS_SHORT:   CDS_PRINT_TO_BUFFER(index, length, "%hd",   data.sp, bufp, bufsize, maxline, linepos, indent); break;
        case CDS_INT:     CDS_PRINT_TO_BUFFER(index, length, "%d",    data.ip, bufp, bufsize, maxline, linepos, indent); break;
        case CDS_FLOAT:   CDS_PRINT_TO_BUFFER(index, length, "%.7g",  data.fp, bufp, bufsize, maxline, linepos, indent); break;
        case CDS_DOUBLE:  CDS_PRINT_TO_BUFFER(index, length, "%.15g", data.dp, bufp, bufsize, maxline, linepos, indent); break;
        default:
            return(0);
    }

    *indexp = index;
    if (lineposp) *lineposp = linepos;

    return(bufp - buffer);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Compare the values in two arrays.
 *
 *  If a plus/minus threshold value is specified it will be applied to the
 *  value in array two, and must be a positive value having the same type as
 *  array two.
 *
 *  If the diff_index argument is not NULL it will return the index of the
 *  first unequal values.
 *
 *  @param  length      - number of values to compare
 *  @param  array1_type - data type of array one
 *  @param  array1      - pointer to array one
 *  @param  array2_type - data type of array two
 *  @param  array2      - pointer to array two
 *  @param  threshold   - pointer to the plus/minus threshold value,
 *                        or NULL for no threshold value.
 *  @param  diff_index  - output: index of the first unequal values.
 *
 *  @return
 *    - -1 if the fist unequal value in array one is less than the value in array 2
 *    -  0 if the two arrays are equal
 *    -  1 if the first unequal value in array one is greater than the value in array 2
 */
int cds_compare_arrays(
    size_t       length,
    CDSDataType  array1_type,
    void        *array1,
    CDSDataType  array2_type,
    void        *array2,
    void        *threshold,
    size_t      *diff_index)
{
    CDSData a1;
    CDSData a2;
    CDSData thresh;
    int     result;
    size_t  count;

    if (array1_type == array2_type && !threshold) {
        length *= cds_data_type_size(array1_type);
        return(memcmp(array1, array2, length));
    }

    a1.vp     = array1;
    a2.vp     = array2;
    thresh.vp = threshold;
    count     = length;
    result    = 0;

    switch (array1_type) {
      case CDS_BYTE:
        switch (array2_type) {
          case CDS_BYTE:   CDS_COMPARE_ARRAYS(result, count, a1.bp, a2.bp, thresh.bp); break;
          case CDS_CHAR:   CDS_COMPARE_ARRAYS(result, count, a1.bp, a2.cp, thresh.cp); break;
          case CDS_SHORT:  CDS_COMPARE_ARRAYS(result, count, a1.bp, a2.sp, thresh.sp); break;
          case CDS_INT:    CDS_COMPARE_ARRAYS(result, count, a1.bp, a2.ip, thresh.ip); break;
          case CDS_FLOAT:  CDS_COMPARE_ARRAYS(result, count, a1.bp, a2.fp, thresh.fp); break;
          case CDS_DOUBLE: CDS_COMPARE_ARRAYS(result, count, a1.bp, a2.dp, thresh.dp); break;
          default:
            break;
        }
        break;
      case CDS_CHAR:
        switch (array2_type) {
          case CDS_BYTE:   CDS_COMPARE_ARRAYS(result, count, a1.cp, a2.bp, thresh.bp); break;
          case CDS_CHAR:   CDS_COMPARE_ARRAYS(result, count, a1.cp, a2.cp, thresh.cp); break;
          case CDS_SHORT:  CDS_COMPARE_ARRAYS(result, count, a1.cp, a2.sp, thresh.sp); break;
          case CDS_INT:    CDS_COMPARE_ARRAYS(result, count, a1.cp, a2.ip, thresh.ip); break;
          case CDS_FLOAT:  CDS_COMPARE_ARRAYS(result, count, a1.cp, a2.fp, thresh.fp); break;
          case CDS_DOUBLE: CDS_COMPARE_ARRAYS(result, count, a1.cp, a2.dp, thresh.dp); break;
          default:
            break;
        }
        break;
      case CDS_SHORT:
        switch (array2_type) {
          case CDS_BYTE:   CDS_COMPARE_ARRAYS(result, count, a1.sp, a2.bp, thresh.bp); break;
          case CDS_CHAR:   CDS_COMPARE_ARRAYS(result, count, a1.sp, a2.cp, thresh.cp); break;
          case CDS_SHORT:  CDS_COMPARE_ARRAYS(result, count, a1.sp, a2.sp, thresh.sp); break;
          case CDS_INT:    CDS_COMPARE_ARRAYS(result, count, a1.sp, a2.ip, thresh.ip); break;
          case CDS_FLOAT:  CDS_COMPARE_ARRAYS(result, count, a1.sp, a2.fp, thresh.fp); break;
          case CDS_DOUBLE: CDS_COMPARE_ARRAYS(result, count, a1.sp, a2.dp, thresh.dp); break;
          default:
            break;
        }
        break;
      case CDS_INT:
        switch (array2_type) {
          case CDS_BYTE:   CDS_COMPARE_ARRAYS(result, count, a1.ip, a2.bp, thresh.bp); break;
          case CDS_CHAR:   CDS_COMPARE_ARRAYS(result, count, a1.ip, a2.cp, thresh.cp); break;
          case CDS_SHORT:  CDS_COMPARE_ARRAYS(result, count, a1.ip, a2.sp, thresh.sp); break;
          case CDS_INT:    CDS_COMPARE_ARRAYS(result, count, a1.ip, a2.ip, thresh.ip); break;
          case CDS_FLOAT:  CDS_COMPARE_ARRAYS(result, count, a1.ip, a2.fp, thresh.fp); break;
          case CDS_DOUBLE: CDS_COMPARE_ARRAYS(result, count, a1.ip, a2.dp, thresh.dp); break;
          default:
            break;
        }
        break;
      case CDS_FLOAT:
        switch (array2_type) {
          case CDS_BYTE:   CDS_COMPARE_ARRAYS(result, count, a1.fp, a2.bp, thresh.bp); break;
          case CDS_CHAR:   CDS_COMPARE_ARRAYS(result, count, a1.fp, a2.cp, thresh.cp); break;
          case CDS_SHORT:  CDS_COMPARE_ARRAYS(result, count, a1.fp, a2.sp, thresh.sp); break;
          case CDS_INT:    CDS_COMPARE_ARRAYS(result, count, a1.fp, a2.ip, thresh.ip); break;
          case CDS_FLOAT:  CDS_COMPARE_ARRAYS(result, count, a1.fp, a2.fp, thresh.fp); break;
          case CDS_DOUBLE: CDS_COMPARE_ARRAYS(result, count, a1.fp, a2.dp, thresh.dp); break;
          default:
            break;
        }
        break;
      case CDS_DOUBLE:
        switch (array2_type) {
          case CDS_BYTE:   CDS_COMPARE_ARRAYS(result, count, a1.dp, a2.bp, thresh.bp); break;
          case CDS_CHAR:   CDS_COMPARE_ARRAYS(result, count, a1.dp, a2.cp, thresh.cp); break;
          case CDS_SHORT:  CDS_COMPARE_ARRAYS(result, count, a1.dp, a2.sp, thresh.sp); break;
          case CDS_INT:    CDS_COMPARE_ARRAYS(result, count, a1.dp, a2.ip, thresh.ip); break;
          case CDS_FLOAT:  CDS_COMPARE_ARRAYS(result, count, a1.dp, a2.fp, thresh.fp); break;
          case CDS_DOUBLE: CDS_COMPARE_ARRAYS(result, count, a1.dp, a2.dp, thresh.dp); break;
          default:
            break;
        }
        break;
      default:
        break;
    }

    if (diff_index && result != 0) {
        *diff_index = length - count;
    }

    return(result);
}

/**
 *  Create a copy of an array of data.
 *
 *  Memory will be allocated for the output data array if the out_data argument
 *  is NULL. In this case the calling process is responsible for freeing the
 *  allocated memory.
 *
 *  The input and output data arrays can be identical if the size of the input
 *  data type is greater than or equal to the size of the output data type.
 *
 *  The mapping variables can be used to change data values when they are copied
 *  to the output array. All values specified in the input map array will be
 *  replaced with the corresponding value in the output map array.
 *
 *  The range variables can be used to replace all values outside a specified
 *  range with a less-than-min or a greater-than-max value. If an out-of-range
 *  value is specified but the corresponding min/max value is not, the valid
 *  min/max value of the output data type will be used if necessary.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  in_type   - data type of the input data
 *  @param  length    - number of values to copy
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
 *    - NULL if a memory allocation error occurs
 *      (this can only happen if the specified out_data argument is NULL)
 */
void *cds_copy_array(
    CDSDataType  in_type,
    size_t       length,
    void        *in_data,
    CDSDataType  out_type,
    void        *out_data,
    size_t       nmap,
    void        *in_map,
    void        *out_map,
    void        *out_min,
    void        *orv_min,
    void        *out_max,
    void        *orv_max)
{
    CDSData in;
    CDSData out;
    CDSData imap;
    CDSData omap;
    CDSData min;
    CDSData max;
    CDSData ormin;
    CDSData ormax;
    size_t  out_size;

    /* Allocate memory for the output array if one was not specified */

    if (!out_data) {
        out_size = cds_data_type_size(out_type);
        out_data = malloc(length * out_size);
        if (!out_data) {
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
        if (out_type < in_type) {
            min.vp = _cds_data_type_min(out_type);
        }
        else {
            ormin.vp = (void *)NULL;
        }
    }

    if (orv_max && !out_max) {
        if (out_type < in_type) {
            max.vp = _cds_data_type_max(out_type);
        }
        else {
            ormax.vp = (void *)NULL;
        }
    }

    /* Copy data from the input data array to the output data array
     * if the input data type is the same as the output data type,
     * and we are not updating missing values.
     */

    if (in_type == out_type && !nmap && !ormin.vp && !ormax.vp) {
        out_size = cds_data_type_size(out_type);
        memcpy(out_data, in_data, length * out_size);
        return(out_data);
    }

    /* Cast data from the input data array to the output data array */

    switch (in_type) {
      case CDS_BYTE:
        switch (out_type) {
          case CDS_BYTE:   CDS_COPY_ARRAY(length, in.bp, nmap, imap.bp, signed char, out.bp, omap.bp, min.bp, ormin.bp, max.bp, ormax.bp, 0); break;
          case CDS_CHAR:   CDS_COPY_ARRAY(length, in.bp, nmap, imap.bp, char,        out.cp, omap.cp, min.cp, ormin.cp, max.cp, ormax.cp, 0); break;
          case CDS_SHORT:  CDS_COPY_ARRAY(length, in.bp, nmap, imap.bp, short,       out.sp, omap.sp, min.sp, ormin.sp, max.sp, ormax.sp, 0); break;
          case CDS_INT:    CDS_COPY_ARRAY(length, in.bp, nmap, imap.bp, int,         out.ip, omap.ip, min.ip, ormin.ip, max.ip, ormax.ip, 0); break;
          case CDS_FLOAT:  CDS_COPY_ARRAY(length, in.bp, nmap, imap.bp, float,       out.fp, omap.fp, min.fp, ormin.fp, max.fp, ormax.fp, 0); break;
          case CDS_DOUBLE: CDS_COPY_ARRAY(length, in.bp, nmap, imap.bp, double,      out.dp, omap.dp, min.dp, ormin.dp, max.dp, ormax.dp, 0); break;
          default:
            break;
        }
        break;
      case CDS_CHAR:
        switch (out_type) {
          case CDS_BYTE:   CDS_COPY_ARRAY(length, in.cp, nmap, imap.cp, signed char, out.bp, omap.bp, min.bp, ormin.bp, max.bp, ormax.bp, 0); break;
          case CDS_CHAR:   CDS_COPY_ARRAY(length, in.cp, nmap, imap.cp, char,        out.cp, omap.cp, min.cp, ormin.cp, max.cp, ormax.cp, 0); break;
          case CDS_SHORT:  CDS_COPY_ARRAY(length, in.cp, nmap, imap.cp, short,       out.sp, omap.sp, min.sp, ormin.sp, max.sp, ormax.sp, 0); break;
          case CDS_INT:    CDS_COPY_ARRAY(length, in.cp, nmap, imap.cp, int,         out.ip, omap.ip, min.ip, ormin.ip, max.ip, ormax.ip, 0); break;
          case CDS_FLOAT:  CDS_COPY_ARRAY(length, in.cp, nmap, imap.cp, float,       out.fp, omap.fp, min.fp, ormin.fp, max.fp, ormax.fp, 0); break;
          case CDS_DOUBLE: CDS_COPY_ARRAY(length, in.cp, nmap, imap.cp, double,      out.dp, omap.dp, min.dp, ormin.dp, max.dp, ormax.dp, 0); break;
          default:
            break;
        }
        break;
      case CDS_SHORT:
        switch (out_type) {
          case CDS_BYTE:   CDS_COPY_ARRAY(length, in.sp, nmap, imap.sp, signed char, out.bp, omap.bp, min.bp, ormin.bp, max.bp, ormax.bp, 0); break;
          case CDS_CHAR:   CDS_COPY_ARRAY(length, in.sp, nmap, imap.sp, char,        out.cp, omap.cp, min.cp, ormin.cp, max.cp, ormax.cp, 0); break;
          case CDS_SHORT:  CDS_COPY_ARRAY(length, in.sp, nmap, imap.sp, short,       out.sp, omap.sp, min.sp, ormin.sp, max.sp, ormax.sp, 0); break;
          case CDS_INT:    CDS_COPY_ARRAY(length, in.sp, nmap, imap.sp, int,         out.ip, omap.ip, min.ip, ormin.ip, max.ip, ormax.ip, 0); break;
          case CDS_FLOAT:  CDS_COPY_ARRAY(length, in.sp, nmap, imap.sp, float,       out.fp, omap.fp, min.fp, ormin.fp, max.fp, ormax.fp, 0); break;
          case CDS_DOUBLE: CDS_COPY_ARRAY(length, in.sp, nmap, imap.sp, double,      out.dp, omap.dp, min.dp, ormin.dp, max.dp, ormax.dp, 0); break;
          default:
            break;
        }
        break;
      case CDS_INT:
        switch (out_type) {
          case CDS_BYTE:   CDS_COPY_ARRAY(length, in.ip, nmap, imap.ip, signed char, out.bp, omap.bp, min.bp, ormin.bp, max.bp, ormax.bp, 0); break;
          case CDS_CHAR:   CDS_COPY_ARRAY(length, in.ip, nmap, imap.ip, char,        out.cp, omap.cp, min.cp, ormin.cp, max.cp, ormax.cp, 0); break;
          case CDS_SHORT:  CDS_COPY_ARRAY(length, in.ip, nmap, imap.ip, short,       out.sp, omap.sp, min.sp, ormin.sp, max.sp, ormax.sp, 0); break;
          case CDS_INT:    CDS_COPY_ARRAY(length, in.ip, nmap, imap.ip, int,         out.ip, omap.ip, min.ip, ormin.ip, max.ip, ormax.ip, 0); break;
          case CDS_FLOAT:  CDS_COPY_ARRAY(length, in.ip, nmap, imap.ip, float,       out.fp, omap.fp, min.fp, ormin.fp, max.fp, ormax.fp, 0); break;
          case CDS_DOUBLE: CDS_COPY_ARRAY(length, in.ip, nmap, imap.ip, double,      out.dp, omap.dp, min.dp, ormin.dp, max.dp, ormax.dp, 0); break;
          default:
            break;
        }
        break;
      case CDS_FLOAT:
        switch (out_type) {
          case CDS_BYTE:   CDS_COPY_ARRAY(length, in.fp, nmap, imap.fp, signed char, out.bp, omap.bp, min.bp, ormin.bp, max.bp, ormax.bp, 1); break;
          case CDS_CHAR:   CDS_COPY_ARRAY(length, in.fp, nmap, imap.fp, char,        out.cp, omap.cp, min.cp, ormin.cp, max.cp, ormax.cp, 1); break;
          case CDS_SHORT:  CDS_COPY_ARRAY(length, in.fp, nmap, imap.fp, short,       out.sp, omap.sp, min.sp, ormin.sp, max.sp, ormax.sp, 1); break;
          case CDS_INT:    CDS_COPY_ARRAY(length, in.fp, nmap, imap.fp, int,         out.ip, omap.ip, min.ip, ormin.ip, max.ip, ormax.ip, 1); break;
          case CDS_FLOAT:  CDS_COPY_ARRAY(length, in.fp, nmap, imap.fp, float,       out.fp, omap.fp, min.fp, ormin.fp, max.fp, ormax.fp, 0); break;
          case CDS_DOUBLE: CDS_COPY_ARRAY(length, in.fp, nmap, imap.fp, double,      out.dp, omap.dp, min.dp, ormin.dp, max.dp, ormax.dp, 0); break;
          default:
            break;
        }
        break;
      case CDS_DOUBLE:
        switch (out_type) {
          case CDS_BYTE:   CDS_COPY_ARRAY(length, in.dp, nmap, imap.dp, signed char, out.bp, omap.bp, min.bp, ormin.bp, max.bp, ormax.bp, 1); break;
          case CDS_CHAR:   CDS_COPY_ARRAY(length, in.dp, nmap, imap.dp, char,        out.cp, omap.cp, min.cp, ormin.cp, max.cp, ormax.cp, 1); break;
          case CDS_SHORT:  CDS_COPY_ARRAY(length, in.dp, nmap, imap.dp, short,       out.sp, omap.sp, min.sp, ormin.sp, max.sp, ormax.sp, 1); break;
          case CDS_INT:    CDS_COPY_ARRAY(length, in.dp, nmap, imap.dp, int,         out.ip, omap.ip, min.ip, ormin.ip, max.ip, ormax.ip, 1); break;
          case CDS_FLOAT:  CDS_COPY_ARRAY(length, in.dp, nmap, imap.dp, float,       out.fp, omap.fp, min.fp, ormin.fp, max.fp, ormax.fp, 0); break;
          case CDS_DOUBLE: CDS_COPY_ARRAY(length, in.dp, nmap, imap.dp, double,      out.dp, omap.dp, min.dp, ormin.dp, max.dp, ormax.dp, 0); break;
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
 *  Create a data index for an n-dimensional array of data.
 *
 *  This function creates an data index for an n-dimensional array of
 *  data stored linearly in memory with the last dimension varying the
 *  fastest. That is, it allows the data to be accessed using the
 *  traditional x[i][j], x[i][j][k], etc... syntax.
 *
 *  The data index returned by this function is dynamically allocated
 *  and must be freed using the cds_free_index_array() function.
 *
 *  @param  data    - pointer to the data array
 *  @param  type    - data type of the data array
 *  @param  ndims   - number of dimensions in the data array
 *  @param  lengths - lengths of the dimensions in the data array
 *
 *  @return
 *    - pointer to the data index
 *    - NULL if:
 *       - ndims is less than 2
 *       - the type is not a valid CDSDataType
 *       - a memory allocation error occurs
 *
 *  @see cds_free_index_array()
 */
void *cds_create_data_index(
    void        *data,
    CDSDataType  type,
    int          ndims,
    size_t      *lengths)
{
    void   *index;
    size_t  type_size;
    size_t *counter;
    size_t *stride;
    int     di;

    /* Make sure the number of dims is >= 2 */

    if (ndims < 2) {
        return((void *)NULL);
    }

    /* Get the size of the data type */

    type_size = cds_data_type_size(type);
    if (!type_size) {
        return((void *)NULL);
    }

    /* Calculate the dimension strides */

    stride = (size_t *)malloc((ndims-1) * sizeof(size_t));
    if (!stride) {
        return((void *)NULL);
    }

    stride[0] = lengths[ndims-1];
    for (di = ndims-2; di > 0; di--) {
        stride[di] = stride[0];
        stride[0] *= lengths[di];
    }

    /* Create dimension index counter */

    counter = (size_t *)calloc(ndims-1, sizeof(size_t));
    if (!counter) {
        free(stride);
        return((void *)NULL);
    }

    /* Create the data index */

    index = _cds_create_data_index(
        data, type_size, ndims-1, lengths, 0, counter, stride);

    free(stride);
    free(counter);

    return(index);
}

/**
 *  Free the data index created for an n-dimensional array of data.
 *
 *  This function will free the memory allocated for the index
 *  array created by the cds_create_data_index() function.
 *
 *  @param  index   - pointer to the data index
 *  @param  ndims   - number of dimensions in the data array
 *  @param  lengths - lengths of the dimensions in the data array
 *
 *  @see cds_create_data_index()
 */
void cds_free_data_index(
    void   *index,
    int     ndims,
    size_t *lengths)
{
    if (ndims < 2) {
        return;
    }

    _cds_free_data_index(index, ndims-1, lengths, 0);
}

/**
 *  Get the missing values map from one data type to another.
 *
 *  Memory will be allocated for the output missing values array if the
 *  out_missing argument is NULL. In this case the calling process is
 *  responsible for freeing the allocated memory.
 *
 *  This function will find the first value in the input missing values array
 *  that is within the range of the output data type. This value will then be
 *  used for all other values that are outside the range of the output data
 *  type. If no values can be found within the range of the output data type,
 *  the default fill value for the output data type will be used.
 *
 *  This function will also map default fill values for the input data type
 *  to the default fill value for the output data type.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  in_type     - data type of the input data
 *  @param  nmissing    - number of missing values
 *  @param  in_missing  - array of missing values in the input data
 *  @param  out_type    - data type of the output data
 *  @param  out_missing - pointer to the output missing values array
 *                        (memory will allocated if this argument is NULL)
 *
 *  @return
 *    - pointer to the out_missing array
 *    - NULL if a memory allocation error occurs
 *      (this can only happen if the specified out_missing argument is NULL)
 */
void *cds_get_missing_values_map(
    CDSDataType  in_type,
    int          nmissing,
    void        *in_missing,
    CDSDataType  out_type,
    void        *out_missing)
{
    CDSData imv;
    CDSData omin;
    CDSData omax;
    CDSData orv;
    CDSData ifill;
    CDSData ofill;
    char    orv_buffer[CDS_MAX_TYPE_SIZE];
    int     mi;

    imv.vp   = in_missing;
    orv.cp   = orv_buffer;
    omin.vp  = _cds_data_type_min(out_type);
    omax.vp  = _cds_data_type_max(out_type);
    ifill.vp = _cds_default_fill_value(in_type);
    ofill.vp = _cds_default_fill_value(out_type);

    /* Get the out-of-range value to use */

    switch (in_type) {
      case CDS_BYTE:
        switch (out_type) {
          case CDS_BYTE:   CDS_FIND_ORV(nmissing, imv.bp, signed char, omin.bp, omax.bp, orv.bp, ofill.bp); break;
          case CDS_CHAR:   CDS_FIND_ORV(nmissing, imv.bp, char,        omin.cp, omax.cp, orv.cp, ofill.cp); break;
          case CDS_SHORT:  CDS_FIND_ORV(nmissing, imv.bp, short,       omin.sp, omax.sp, orv.sp, ofill.sp); break;
          case CDS_INT:    CDS_FIND_ORV(nmissing, imv.bp, int,         omin.ip, omax.ip, orv.ip, ofill.ip); break;
          case CDS_FLOAT:  CDS_FIND_ORV(nmissing, imv.bp, float,       omin.fp, omax.fp, orv.fp, ofill.fp); break;
          case CDS_DOUBLE: CDS_FIND_ORV(nmissing, imv.bp, double,      omin.dp, omax.dp, orv.dp, ofill.dp); break;
          default:
            break;
        }
        break;
      case CDS_CHAR:
        switch (out_type) {
          case CDS_BYTE:   CDS_FIND_ORV(nmissing, imv.cp, signed char, omin.bp, omax.bp, orv.bp, ofill.bp); break;
          case CDS_CHAR:   CDS_FIND_ORV(nmissing, imv.cp, char,        omin.cp, omax.cp, orv.cp, ofill.cp); break;
          case CDS_SHORT:  CDS_FIND_ORV(nmissing, imv.cp, short,       omin.sp, omax.sp, orv.sp, ofill.sp); break;
          case CDS_INT:    CDS_FIND_ORV(nmissing, imv.cp, int,         omin.ip, omax.ip, orv.ip, ofill.ip); break;
          case CDS_FLOAT:  CDS_FIND_ORV(nmissing, imv.cp, float,       omin.fp, omax.fp, orv.fp, ofill.fp); break;
          case CDS_DOUBLE: CDS_FIND_ORV(nmissing, imv.cp, double,      omin.dp, omax.dp, orv.dp, ofill.dp); break;
          default:
            break;
        }
        break;
      case CDS_SHORT:
        switch (out_type) {
          case CDS_BYTE:   CDS_FIND_ORV(nmissing, imv.sp, signed char, omin.bp, omax.bp, orv.bp, ofill.bp); break;
          case CDS_CHAR:   CDS_FIND_ORV(nmissing, imv.sp, char,        omin.cp, omax.cp, orv.cp, ofill.cp); break;
          case CDS_SHORT:  CDS_FIND_ORV(nmissing, imv.sp, short,       omin.sp, omax.sp, orv.sp, ofill.sp); break;
          case CDS_INT:    CDS_FIND_ORV(nmissing, imv.sp, int,         omin.ip, omax.ip, orv.ip, ofill.ip); break;
          case CDS_FLOAT:  CDS_FIND_ORV(nmissing, imv.sp, float,       omin.fp, omax.fp, orv.fp, ofill.fp); break;
          case CDS_DOUBLE: CDS_FIND_ORV(nmissing, imv.sp, double,      omin.dp, omax.dp, orv.dp, ofill.dp); break;
          default:
            break;
        }
        break;
      case CDS_INT:
        switch (out_type) {
          case CDS_BYTE:   CDS_FIND_ORV(nmissing, imv.ip, signed char, omin.bp, omax.bp, orv.bp, ofill.bp); break;
          case CDS_CHAR:   CDS_FIND_ORV(nmissing, imv.ip, char,        omin.cp, omax.cp, orv.cp, ofill.cp); break;
          case CDS_SHORT:  CDS_FIND_ORV(nmissing, imv.ip, short,       omin.sp, omax.sp, orv.sp, ofill.sp); break;
          case CDS_INT:    CDS_FIND_ORV(nmissing, imv.ip, int,         omin.ip, omax.ip, orv.ip, ofill.ip); break;
          case CDS_FLOAT:  CDS_FIND_ORV(nmissing, imv.ip, float,       omin.fp, omax.fp, orv.fp, ofill.fp); break;
          case CDS_DOUBLE: CDS_FIND_ORV(nmissing, imv.ip, double,      omin.dp, omax.dp, orv.dp, ofill.dp); break;
          default:
            break;
        }
        break;
      case CDS_FLOAT:
        switch (out_type) {
          case CDS_BYTE:   CDS_FIND_ORV(nmissing, imv.fp, signed char, omin.bp, omax.bp, orv.bp, ofill.bp); break;
          case CDS_CHAR:   CDS_FIND_ORV(nmissing, imv.fp, char,        omin.cp, omax.cp, orv.cp, ofill.cp); break;
          case CDS_SHORT:  CDS_FIND_ORV(nmissing, imv.fp, short,       omin.sp, omax.sp, orv.sp, ofill.sp); break;
          case CDS_INT:    CDS_FIND_ORV(nmissing, imv.fp, int,         omin.ip, omax.ip, orv.ip, ofill.ip); break;
          case CDS_FLOAT:  CDS_FIND_ORV(nmissing, imv.fp, float,       omin.fp, omax.fp, orv.fp, ofill.fp); break;
          case CDS_DOUBLE: CDS_FIND_ORV(nmissing, imv.fp, double,      omin.dp, omax.dp, orv.dp, ofill.dp); break;
          default:
            break;
        }
        break;
      case CDS_DOUBLE:
        switch (out_type) {
          case CDS_BYTE:   CDS_FIND_ORV(nmissing, imv.dp, signed char, omin.bp, omax.bp, orv.bp, ofill.bp); break;
          case CDS_CHAR:   CDS_FIND_ORV(nmissing, imv.dp, char,        omin.cp, omax.cp, orv.cp, ofill.cp); break;
          case CDS_SHORT:  CDS_FIND_ORV(nmissing, imv.dp, short,       omin.sp, omax.sp, orv.sp, ofill.sp); break;
          case CDS_INT:    CDS_FIND_ORV(nmissing, imv.dp, int,         omin.ip, omax.ip, orv.ip, ofill.ip); break;
          case CDS_FLOAT:  CDS_FIND_ORV(nmissing, imv.dp, float,       omin.fp, omax.fp, orv.fp, ofill.fp); break;
          case CDS_DOUBLE: CDS_FIND_ORV(nmissing, imv.dp, double,      omin.dp, omax.dp, orv.dp, ofill.dp); break;
          default:
            break;
        }
        break;
      default:
        break;
    }

    out_missing = cds_copy_array(
        in_type, nmissing, in_missing, out_type, out_missing,
        1, ifill.vp, ofill.vp,
        NULL, orv.vp, NULL, orv.vp);

    return(out_missing);
}

/**
 *  Initialize the values in a data array.
 *
 *  This function can be used to initialize the values of a data array to a
 *  specified fill value. The default fill value for the data type will be used
 *  if the fill value argument os NULL.
 *
 *  Memory will be allocated for the returned array if the specified data array
 *  is NULL. In this case the calling process is responsible for freeing the
 *  allocated memory.
 *
 *  @param  type       - data type of the fill value and data array
 *  @param  length     - number of values
 *  @param  fill_value - pointer to the fill value
 *  @param  array      - pointer to the data array
 *
 *  @return
 *    - pointer to the output data array
 *    - NULL if a memory allocation error occurs
 *      (this can only happen if the specified array argument is NULL)
 */
void *cds_init_array(
    CDSDataType  type,
    size_t       length,
    void        *fill_value,
    void        *array)
{
    size_t  type_size;
    CDSData da;
    CDSData fv;

    if (!fill_value) {
        fill_value = _cds_default_fill_value(type);
    }

    if (!array) {
        type_size = cds_data_type_size(type);
        array     = malloc(length * type_size);
        if (!array) {
            return((void *)NULL);
        }
    }

    length++;
    da.vp = array;
    fv.vp = fill_value;

    switch (type) {
      case CDS_BYTE:   while (--length) *da.bp++ = *fv.bp; break;
      case CDS_CHAR:   while (--length) *da.cp++ = *fv.cp; break;
      case CDS_SHORT:  while (--length) *da.sp++ = *fv.sp; break;
      case CDS_INT:    while (--length) *da.ip++ = *fv.ip; break;
      case CDS_FLOAT:  while (--length) *da.fp++ = *fv.fp; break;
      case CDS_DOUBLE: while (--length) *da.dp++ = *fv.dp; break;
      default:
        break;
    }

    return(array);
}

/**
 *  Create a dynamically allocated copy of an array of memory.
 *
 *  @param  nbytes - number of bytes to copy
 *  @param  memp   - pointer to the memory to copy
 *
 *  @return
 *    - pointer to the dynamically alloced copy
 *    - NULL if a memory allocation error occurred
 */
void *cds_memdup(size_t nbytes, void *memp)
{
    void *dup = (void *)NULL;

    if (memp && (dup = malloc(nbytes))) {
        memcpy(dup, memp, nbytes);
    }

    return(dup);
}

/**
 *  Convert base time/time offset values to time_t values.
 *
 *  Memory will be allocated for the returned array of times if the
 *  output array is NULL. In this case the calling process is responsible
 *  for freeing the allocated memory.
 *
 *  @param  type      - data type of the offsets array
 *  @param  ntimes    - number of times in the offsets array
 *  @param  base_time - base time value in seconds since 1970
 *  @param  offsets   - array of offsets from the base time in seconds
 *  @param  times     - pointer to the output array
 *                      or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the array of times in seconds since 1970
 *    - NULL if a memory allocation error occurred
 */
time_t *cds_offsets_to_times(
    CDSDataType  type,
    size_t       ntimes,
    time_t       base_time,
    void        *offsets,
    time_t      *times)
{
    int        count;
    double     offset;
    time_t    *tp;
    CDSData    off;

    /* Allocate memory for the output array if necessary */

    if (!times) {
        times = (time_t *)malloc(ntimes * sizeof(time_t));
        if (!times) {
            return(NULL);
        }
    }

    /* Convert offsets to times */

    off.vp = offsets;
    tp     = times;
    count  = ntimes + 1;

    switch (type) {
      case CDS_DOUBLE:
        while (--count) {
            offset = *off.dp++;
            if (offset < 0) *tp++ = base_time + (time_t)(offset - 0.5);
            else            *tp++ = base_time + (time_t)(offset + 0.5);
        }
        break;
      case CDS_FLOAT:
        while (--count) {
            offset = (double)*off.fp++;
            if (offset < 0) *tp++ = base_time + (time_t)(offset - 0.5);
            else            *tp++ = base_time + (time_t)(offset + 0.5);
        }
        break;
      case CDS_INT:
        while (--count) *tp++ = base_time + (time_t)(*off.ip++);
        break;
      case CDS_SHORT:
        while (--count) *tp++ = base_time + (time_t)(*off.sp++);
        break;
      case CDS_BYTE:
        while (--count) *tp++ = base_time + (time_t)(*off.bp++);
        break;
      case CDS_CHAR:
        while (--count) *tp++ = base_time + (time_t)(*off.cp++);
        break;
      default:
        while (--count) *tp++ = 0;
    }

    return(times);
}

/**
 *  Convert base time/time offset values to timeval_t values.
 *
 *  Memory will be allocated for the returned array of timevals if the
 *  output array is NULL. In this case the calling process is responsible
 *  for freeing the allocated memory.
 *
 *  @param  type      - data type of the offsets array
 *  @param  ntimes    - number of times in the offsets array
 *  @param  base_time - base time value in seconds since 1970
 *  @param  offsets   - array of offsets from the base time in seconds
 *  @param  timevals  - pointer to the output array
 *                      or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the array of timeval_t values in seconds since 1970
 *    - NULL if a memory allocation error occurred
 */
timeval_t *cds_offsets_to_timevals(
    CDSDataType  type,
    size_t       ntimes,
    time_t       base_time,
    void        *offsets,
    timeval_t   *timevals)
{
    int        count;
    double     offset;
    timeval_t *tvp;
    CDSData    off;

    /* Allocate memory for the output array if necessary */

    if (!timevals) {
        timevals = (timeval_t *)calloc(ntimes, sizeof(timeval_t));
        if (!timevals) {
            return(NULL);
        }
    }

    /* Convert offsets to timevals */

    off.vp = offsets;
    tvp    = timevals;
    count  = ntimes + 1;

    switch (type) {
      case CDS_DOUBLE:
        while (--count) {

            offset      = *off.dp++;
            tvp->tv_sec = offset;

            if (offset < 0) {
                tvp->tv_usec = (offset - tvp->tv_sec) * 1E6 - 0.5;
                if (tvp->tv_usec < 0) {
                    tvp->tv_sec  -= 1;
                    tvp->tv_usec += 1E6;
                }
            }
            else {
                tvp->tv_usec = (offset - tvp->tv_sec) * 1E6 + 0.5;
                if (tvp->tv_usec > 999999) {
                    tvp->tv_sec  += 1;
                    tvp->tv_usec -= 1E6;
                }
            }

            tvp->tv_sec += base_time;

            ++tvp;
        }
        break;
      case CDS_FLOAT:
        while (--count) {

            offset      = (double)*off.fp++;
            tvp->tv_sec = offset;

            if (offset < 0) {
                tvp->tv_usec = (offset - tvp->tv_sec) * 1E6 - 0.5;
                if (tvp->tv_usec < 0) {
                    tvp->tv_sec  -= 1;
                    tvp->tv_usec += 1E6;
                }
            }
            else {
                tvp->tv_usec = (offset - tvp->tv_sec) * 1E6 + 0.5;
                if (tvp->tv_usec > 999999) {
                    tvp->tv_sec  += 1;
                    tvp->tv_usec -= 1E6;
                }
            }

            tvp->tv_sec += base_time;

            ++tvp;
        }
        break;
      case CDS_INT:
        while (--count) {
            tvp->tv_sec  = base_time + (time_t)(*off.ip++);
            tvp->tv_usec = 0;
            ++tvp;
        }
        break;
      case CDS_SHORT:
        while (--count) {
            tvp->tv_sec  = base_time + (time_t)(*off.sp++);
            tvp->tv_usec = 0;
            ++tvp;
        }
        break;
      case CDS_BYTE:
        while (--count) {
            tvp->tv_sec  = base_time + (time_t)(*off.bp++);
            tvp->tv_usec = 0;
            ++tvp;
        }
      case CDS_CHAR:
        while (--count) {
            tvp->tv_sec  = base_time + (time_t)(*off.cp++);
            tvp->tv_usec = 0;
            ++tvp;
        }
        break;
      default:
        while (--count) {
            tvp->tv_sec  = 0;
            tvp->tv_usec = 0;
            ++tvp;
        }
    }

    return(timevals);
}

/**
 *  Perform QC delta checks on an array of data values.
 *
 *  Memory will be allocated for the returned array of qc_flags if the output
 *  array is NULL. In this case the calling process is responsible for freeing
 *  the allocated memory.
 *
 *  @param  data_type      - data type of the data values
 *  @param  ndims          - number of dimensions
 *  @param  dim_lengths    - dimension lengths
 *  @param  data_vp        - void pointer to the array of data values stored
 *                           linearly in memory with the last dimension varying
 *                           the fastest.
 *
 *  @param  ndeltas        - the number of delta values (must be <= ndims)
 *  @param  deltas_vp      - void pointer to the array of delta values for each
 *                           dimension to perform delta checks on (specify delta
 *                           values of 0 to skip dimensions).
 *  @param  delta_flags    - array of corresponding QC flags to use for failed
 *                           delta checks.
 *
 *  @param  prev_sample_vp - void pointer to the data values from the previous
 *                           sample or NULL if not available.
 *  @param  prev_qc_flags  - array of QC values from the previous sample used to
 *                           determine if the delta check should be preformed.
 *
 *  @param  bad_flags      - QC flags used to determine bad or missing values
 *                           that should not be used for delta checks.
 *
 *  @param  qc_flags       - array of previously initialized qc_flags,
 *                           or NULL to dynamically allocate memory for
 *                           the returned array.
 *
 *  @return
 *    - pointer to the array of qc_flags
 *    - NULL if a memory allocation error occurred
 */
int *cds_qc_delta_checks(
    CDSDataType  data_type,
    size_t       ndims,
    size_t      *dim_lengths,
    void        *data_vp,
    size_t       ndeltas,
    void        *deltas_vp,
    int         *delta_flags,
    void        *prev_sample_vp,
    int         *prev_qc_flags,
    int          bad_flags,
    int         *qc_flags)
{
    size_t  sample_count = 1;
    size_t  sample_size  = 1;
    size_t  nvalues      = 1;
    size_t *index        = (size_t *)NULL;
    size_t *strides      = (size_t *)NULL;
    size_t  di;

    /* Determine the sample size and count. */

    if (ndims && dim_lengths) {
        sample_count = dim_lengths[0];
        for (di = 1; di < ndims; ++di) {
            sample_size *= dim_lengths[di];
        }
    }

    nvalues = sample_count * sample_size;

    /* Allocate memory for the qc_flags array if necessary */

    if (!qc_flags) {
        qc_flags = (int *)calloc(nvalues, sizeof(int));
        if (!qc_flags) {
            return((int *)NULL);
        }
    }

    /* Perform the QC checks across the sample dimension */

    if (sample_size == 1) {

        switch (data_type) {
            case CDS_DOUBLE: CDS_QC_DELTA_CHECKS_1D_1(double,        double,  fabs);  break;
            case CDS_FLOAT:  CDS_QC_DELTA_CHECKS_1D_1(float,         float,   fabsf); break;
            case CDS_INT:    CDS_QC_DELTA_CHECKS_1D_1(int,           int,     abs);   break;
            case CDS_SHORT:  CDS_QC_DELTA_CHECKS_1D_1(short,         int,     abs);   break;
            case CDS_BYTE:   CDS_QC_DELTA_CHECKS_1D_1(signed char,   int,     abs);   break;
            case CDS_CHAR:   CDS_QC_DELTA_CHECKS_1D_1(unsigned char, int,     abs);   break;
            default:
                break;
        }
    }
    else { /* sample_size > 1 */

        switch (data_type) {
            case CDS_DOUBLE: CDS_QC_DELTA_CHECKS_1D_N(double,        double,  fabs);  break;
            case CDS_FLOAT:  CDS_QC_DELTA_CHECKS_1D_N(float,         float,   fabsf); break;
            case CDS_INT:    CDS_QC_DELTA_CHECKS_1D_N(int,           int,     abs);   break;
            case CDS_SHORT:  CDS_QC_DELTA_CHECKS_1D_N(short,         int,     abs);   break;
            case CDS_BYTE:   CDS_QC_DELTA_CHECKS_1D_N(signed char,   int,     abs);   break;
            case CDS_CHAR:   CDS_QC_DELTA_CHECKS_1D_N(unsigned char, int,     abs);   break;
            default:
                break;
        }
    }

    /* Check if we are doing delta checks across any of the other dimensions  */

    if (ndims   > 1 &&
        ndeltas > 1) {

        /* Allocate memory for the strides and index arrays */

        strides = malloc(ndims * sizeof(size_t));
        if (!strides) {
            return((int *)NULL);
        }

        index = malloc(ndims * sizeof(size_t));
        if (!index) {
            free(strides);
            return((int *)NULL);
        }

        /* Do the QC checks */

        switch (data_type) {
            case CDS_DOUBLE: CDS_QC_DELTA_CHECKS_ND(double,        double,  fabs);  break;
            case CDS_FLOAT:  CDS_QC_DELTA_CHECKS_ND(float,         float,   fabsf); break;
            case CDS_INT:    CDS_QC_DELTA_CHECKS_ND(int,           int,     abs);   break;
            case CDS_SHORT:  CDS_QC_DELTA_CHECKS_ND(short,         int,     abs);   break;
            case CDS_BYTE:   CDS_QC_DELTA_CHECKS_ND(signed char,   int,     abs);   break;
            case CDS_CHAR:   CDS_QC_DELTA_CHECKS_ND(unsigned char, int,     abs);   break;
            default:
                break;
        }

        free(strides);
        free(index);
    }

    return(qc_flags);
}

/**
 *  Perform QC limit checks on an array of data values.
 *
 *  Memory will be allocated for the returned array of qc_flags if the output
 *  array is NULL. In this case the calling process is responsible for freeing
 *  the allocated memory.
 *
 *  @param  data_type     - data type of the data values
 *  @param  nvalues       - number of data values
 *  @param  data_vp       - void pointer to the array of data values
 *
 *  @param  nmissings     - number of missing values
 *  @param  missings_vp   - void pointer to the array of missing values,
 *                          or NULL if there are no missing values
 *  @param  missing_flags - array of corresponding QC flags to use for failed
 *                          missing value checks.
 *
 *  @param  min_vp        - void pointer to the minimum value
 *                          or NULL if there is no minimum value
 *  @param  min_flag      - QC flag to use for values below the minimum
 *
 *  @param  max_vp        - void pointer to the maximum value
 *                          or NULL if there is no maximum value
 *  @param  max_flag      - QC flag to use for values above the maximum
 *
 *  @param  qc_flags      - array of previously initialized qc_flags,
 *                          or NULL to dynamically allocate memory for
 *                          the returned array.
 *
 *  @return
 *    - pointer to the array of qc_flags
 *    - NULL if a memory allocation error occurred
 */
int *cds_qc_limit_checks(
    CDSDataType  data_type,
    size_t       nvalues,
    void        *data_vp,
    size_t       nmissings,
    void        *missings_vp,
    int         *missing_flags,
    void        *min_vp,
    int          min_flag,
    void        *max_vp,
    int          max_flag,
    int         *qc_flags)
{
    size_t  mi;
    int    *flagsp;

    /* Allocate memory for the qc_flags array if necessary */

    if (!qc_flags) {
        qc_flags = (int *)calloc(nvalues, sizeof(int));
        if (!qc_flags) {
            return((int *)NULL);
        }
    }

    flagsp = qc_flags;

    /* Perform the QC checks */

    switch (data_type) {
        case CDS_DOUBLE: CDS_QC_LIMITS_CHECK(double);        break;
        case CDS_FLOAT:  CDS_QC_LIMITS_CHECK(float);         break;
        case CDS_INT:    CDS_QC_LIMITS_CHECK(int);           break;
        case CDS_SHORT:  CDS_QC_LIMITS_CHECK(short);         break;
        case CDS_BYTE:   CDS_QC_LIMITS_CHECK(signed char);   break;
        case CDS_CHAR:   CDS_QC_LIMITS_CHECK(unsigned char); break;
        default:
            break;
    }

    return(qc_flags);
}

/**
 *  Perform QC checks on an array of time offsets.
 *
 *  Memory will be allocated for the returned array of qc_flags if the output
 *  array is NULL. In this case the calling process is responsible for freeing
 *  the allocated memory.
 *
 *  @param  data_type      - data type of the time offset values
 *  @param  noffsets       - number of time offset values
 *  @param  offsets_vp     - void pointer to the array of time offset values
 *  @param  prev_offset_vp - void pointer to the previous offset value to use
 *                           for the delta check on the first value, or NULL
 *                           for no delta check on the first time offset
 *
 *  @param  lteq_zero_flag - QC flag to use for time deltas <= zero
 *
 *  @param  min_delta_vp   - void pointer to the minimum time delta value,
 *                           or NULL if there is no minimum time delta value
 *  @param  min_delta_flag - QC flag to use for time deltas below the minimum
 *
 *  @param  max_delta_vp   - void pointer to the maximum time delta value,
 *                           or NULL if there is no maximum time delta value
 *  @param  max_delta_flag - QC flag to use for time deltas above the maximum
 *
 *  @param  qc_flags       - array of previously initialized qc_flags, or
 *                           NULL to dynamically memory for the returned array.
 *
 *  @return
 *    - pointer to the array of qc_flags
 *    - NULL if a memory allocation error occurred
 */
int *cds_qc_time_offset_checks(
    CDSDataType  data_type,
    size_t       noffsets,
    void        *offsets_vp,
    void        *prev_offset_vp,
    int          lteq_zero_flag,
    void        *min_delta_vp,
    int          min_delta_flag,
    void        *max_delta_vp,
    int          max_delta_flag,
    int         *qc_flags)
{
    int *flagsp;

    /* Allocate memory for the qc_flags array if necessary */

    if (!qc_flags) {
        qc_flags = (int *)calloc(noffsets, sizeof(int));
        if (!qc_flags) {
            return((int *)NULL);
        }
    }

    flagsp = qc_flags;

    /* Perform the QC checks */

    switch (data_type) {
        case CDS_DOUBLE: CDS_QC_TIME_OFFSETS_CHECK(double);        break;
        case CDS_FLOAT:  CDS_QC_TIME_OFFSETS_CHECK(float);         break;
        case CDS_INT:    CDS_QC_TIME_OFFSETS_CHECK(int);           break;
        case CDS_SHORT:  CDS_QC_TIME_OFFSETS_CHECK(short);         break;
        case CDS_BYTE:   CDS_QC_TIME_OFFSETS_CHECK(signed char);   break;
        case CDS_CHAR:   CDS_QC_TIME_OFFSETS_CHECK(unsigned char); break;
        default:
            break;
    }

    return(qc_flags);
}

/**
 *  Print an array of data values.
 *
 *  By default data arrays will be beigin and end with open and close brackets,
 *  and character arrays will begin and end with a quote.
 *
 *  @param  fp      - pointer to the output stream to write to
 *  @param  type    - data type of the array
 *  @param  length  - number of values to print
 *  @param  array   - pointer to the array of values
 *  @param  indent  - line indent string to use for new lines
 *  @param  maxline - maximum number of characters to print per line,
 *                    or 0 for no line breaks in numeric arrays and to only
 *                    split character arrays on newlines.
 *  @param  linepos - starting line position when this function was called,
 *                    ignored if maxline == 0
 *  @param  flags   - control flags:
 *                      - 0x01: Print data type name for numeric arrays.
 *                      - 0x02: Print padded data type name for numeric arrays.
 *                      - 0x04: Print data type name at end of numeric arrays.
 *                      - 0x08: Do not print brackets around numeric arrays.
 *                      - 0x10: Trim trailing NULLs from the end of strings.
 *
 *  @return
 *    - number of bytes printed
 *    - (size_t)-1 if an error occurs
 */
size_t cds_print_array(
    FILE        *fp,
    CDSDataType  type,
    size_t       length,
    void        *array,
    const char  *indent,
    size_t       maxline,
    size_t       linepos,
    int          flags)
{
    size_t      bufsize = 4096;
    char        buffer[bufsize];
    size_t      index;
    size_t      nbytes;
    size_t      tbytes;
    const char *open_bracket;
    const char *close_bracket;

    /* Used to test buffer boundries
     *
     * bufsize = (maxline) ? maxline + 9 : 33;
     * if (type != CDS_CHAR) {
     *     bufsize = 33;
     * }
     */

    if (!fp || !length || !array) {
        return(0);
    }

    tbytes = 0;

    /* Trim trailing NULLs from character strings */

    if ((type == CDS_CHAR) && (flags & 0x10)) {
        while (length && ((char *)array)[length-1] == '\0') --length;
    }

    /* Get the open and close brackets or quotes */

    _cds_get_array_brackets(type, flags, &open_bracket, &close_bracket);

    if (open_bracket) {
        nbytes = strlen(open_bracket);
        if (fwrite(open_bracket, nbytes, 1, fp) != 1) {
            return((size_t)-1);
        }
        tbytes  += nbytes;
        linepos += nbytes;
    }

    /* Print values */

    index = 0;

    while (index < length) {

        nbytes = _cds_print_array_to_buffer(
            bufsize, buffer, &index, type, length, array,
            maxline, &linepos, indent);

        if (!nbytes) {
            break;
        }

        if (fwrite(buffer, nbytes, 1, fp) != 1) {
            return((size_t)-1);
        }

        tbytes += nbytes;
    }

    /* Print close bracket or quote */

    if (close_bracket) {
        nbytes = strlen(close_bracket);
        if (fwrite(close_bracket, 1, nbytes, fp) != nbytes) {
            return((size_t)-1);
        }
        tbytes += nbytes;
    }

    return(tbytes);
}

/**
 *  Print an array of data values to a string.
 *
 *  By default data arrays will be beigin and end with open and close brackets,
 *  and character arrays will begin and end with a quote.
 *
 *  Memory will be allocated for the returned string if the output string
 *  is NULL. In this case the calling process is responsible for freeing
 *  the allocated memory.
 *
 *  @param  type          - data type of the array
 *  @param  array_length  - the length of the input array
 *  @param  array         - pointer to the input array
 *  @param  string_length - pointer to the length of the output string
 *                           - input:
 *                              - maximum length of the output string
 *                              - ignored if the output string is NULL
 *                           - output:
 *                              - length of the output string
 *                              - 0 if array_length is zero
 *                              - (size_t)-1 if a memory allocation error occurs
 *  @param  string        - pointer to the output string
 *                          or NULL to dynamically allocate the memory needed.
 *
 *  @param  indent        - line indent string to use for new lines
 *  @param  maxline       - maximum number of characters to print per line,
 *                          or 0 for no line breaks in numeric arrays and to
 *                          only split character arrays on newlines.
 *  @param  linepos       - starting line position when this function was called,
 *                          ignored if maxline == 0
 *  @param  flags         - control flags:
 *                          - 0x01: Print data type name for numeric arrays.
 *                          - 0x02: Print padded data type name for numeric arrays.
 *                          - 0x04: Print data type name at end of numeric arrays.
 *                          - 0x08: Do not print brackets around numeric arrays.
 *                          - 0x10: Trim trailing NULLs from the end of strings.
 *
 *  @return
 *    - pointer to the output string
 *    - NULL if:
 *        - array_length is zero (string_length == 0)
 *        - a memory allocation error occured (string_length == (size_t)-1)
 */
char *cds_sprint_array(
    CDSDataType  type,
    size_t       array_length,
    void        *array,
    size_t      *string_length,
    char        *string,
    const char  *indent,
    size_t       maxline,
    size_t       linepos,
    int          flags)
{
    size_t      max_length;
    size_t      space_left;
    size_t      str_length;
    size_t      index;
    size_t      nbytes;
    char       *out_string;
    char       *new_string;
    char       *strp;
    const char *open_bracket;
    const char *close_bracket;
    size_t      ob_len;
    size_t      cb_len;

    if (!array || !array_length) {
        if (string_length) *string_length = 0;
        if (string) *string = '\0';
        return((char *)NULL);
    }

    /* Trim trailing NULLs from character strings */

    if ((type == CDS_CHAR) && (flags & 0x10)) {
        while (array_length && ((char *)array)[array_length-1] == '\0') {
            --array_length;
        }
    }

    /* Get the open and close brackets or quotes */

    _cds_get_array_brackets(type, flags, &open_bracket, &close_bracket);

    ob_len = (open_bracket)  ? strlen(open_bracket)  : 0;
    cb_len = (close_bracket) ? strlen(close_bracket) : 0;

    /* Allocate memory for the output string */

    if (string) {
        if (!string_length || *string_length == 0) {
            *string = '\0';
            return((char *)NULL);
        }

        max_length = *string_length;
    }
    else {
        max_length = (type == CDS_CHAR) ? array_length : array_length * 3;
    }

    max_length += maxline + ob_len + cb_len + 32;
    out_string  = (char *)malloc(max_length * sizeof(char));

    if (!out_string) {
        if (string_length) *string_length = (size_t)-1;
        return((char *)NULL);
    }

    strp = out_string;

    /* Print the open bracket or quote */

    if (open_bracket) {
        memcpy(strp, open_bracket, ob_len);
        strp    += ob_len;
        linepos += ob_len;
    }

    /* Print values */

    index = 0;

    for (;;) {

        /* Print the values to the string */

        space_left = max_length - (strp - out_string);

        nbytes = _cds_print_array_to_buffer(
            space_left, strp, &index, type, array_length, array,
            maxline, &linepos, indent);

        strp += nbytes;

        /* Check if we are done */

        if (nbytes == 0    ||
            string != NULL ||
            index  >= array_length) {

            break;
        }

        /* Allocate more memory */

        str_length  = strp - out_string;
        max_length += (array_length - index) * (str_length/index + 1);
        max_length += maxline + cb_len + 32;

        new_string  = (char *)realloc(
            out_string, max_length * sizeof(char) + 32);

        if (!new_string) {
            free(out_string);
            if (string_length) *string_length = (size_t)-1;
            return((char *)NULL);
        }

        out_string = new_string;
        strp       = out_string + str_length;
    }

    /* Print the close bracket or quote */

    if (close_bracket) {
        memcpy(strp, close_bracket, cb_len);
        strp    += cb_len;
        linepos += cb_len;
    }

    *strp = '\0';

    /* Copy the string to the output buffer if one was specified */

    str_length = strp - out_string;

    if (string) {

        if (str_length > *string_length - 1) {
            str_length = *string_length - 1;
        }

        memcpy(string, out_string, str_length);
        string[str_length] = '\0';
        free(out_string);
        out_string = string;
    }

    if (string_length) *string_length = str_length;

    return(out_string);
}

/**
 *  Convert a text string to an array of values.
 *
 *  This function will convert a text string containing an array of
 *  values into an array of values with the specified data type.
 *  Values that are less than the minimum value for the data type
 *  will be converted to the minimum value, and values that are
 *  greater than the maximum value for the data type will be converted
 *  to the maximum value.
 *
 *  Memory will be allocated for the returned array if the output array
 *  is NULL. In this case the calling process is responsible for freeing
 *  the allocated memory.
 *
 *  @param  string - pointer to the input string
 *  @param  type   - data type of the output array
 *  @param  length - pointer to the length of the output array
 *                     - input:
 *                         - maximum length of the output array
 *                         - ignored if the output array is NULL
 *                     - output:
 *                         - number of values written to the output array
 *                         - 0 if the string did not contain any numeric values
 *                         - (size_t)-1 if a memory allocation error occurs
 *  @param  array  - pointer to the output array
 *                   or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the output array
 *    - NULL if:
 *        - the string did not contain any numeric values (length == 0)
 *        - a memory allocation error occured (length == (size_t)-1)
 */
void *cds_string_to_array(
    const char  *string,
    CDSDataType  type,
    size_t      *length,
    void        *array)
{
    size_t  type_size = cds_data_type_size(type);
    char   *strp;
    char   *endp;
    size_t  nvals;
    size_t  nalloced;
    void   *new_data;
    CDSData data;
    double  dval;

    if (!string) {
        if (length) *length = 0;
        return((void *)NULL);
    }

    data.vp  = array;
    nvals    = 0;
    nalloced = 0;

    for (strp = (char *)string; *strp != '\0'; strp++) {

        /* Skip white-space */

        while (isspace(*strp)) strp++;

        if (*strp == '\0') break;

        /* Allocate more memory if necessary */

        if (!array && (nvals == nalloced)) {

            if (nalloced) {
                nalloced *= 2;
            }
            else {
                nalloced = 1;
            }

            new_data = realloc(data.vp, nalloced * type_size);
            if (!new_data) {
                if (length) *length = -1;
                free(data.vp);
                return((void *)NULL);
            }

            data.vp = new_data;
        }

        /* Get the next array element from the string */

        switch (type) {

            case CDS_BYTE:

                dval = strtod(strp, &endp);

                if (dval < CDS_MIN_BYTE) {
                    data.bp[nvals] = CDS_MIN_BYTE;
                }
                else if (dval > CDS_MAX_BYTE) {
                    data.bp[nvals] = CDS_MAX_BYTE;
                }
                else if (dval < 0) {
                    data.bp[nvals] = (signed char)(dval - 0.5);
                }
                else {
                    data.bp[nvals] = (signed char)(dval + 0.5);
                }

                break;

            case CDS_SHORT:

                dval = strtod(strp, &endp);

                if (dval < CDS_MIN_SHORT) {
                    data.sp[nvals] = CDS_MIN_SHORT;
                }
                else if (dval > CDS_MAX_SHORT) {
                    data.sp[nvals] = CDS_MAX_SHORT;
                }
                else if (dval < 0) {
                    data.sp[nvals] = (short)(dval - 0.5);
                }
                else {
                    data.sp[nvals] = (short)(dval + 0.5);
                }

                break;

            case CDS_INT:

                dval = strtod(strp, &endp);

                if (dval < CDS_MIN_INT) {
                    data.ip[nvals] = CDS_MIN_INT;
                }
                else if (dval > CDS_MAX_INT) {
                    data.ip[nvals] = CDS_MAX_INT;
                }
                else if (dval < 0) {
                    data.ip[nvals] = (int)(dval - 0.5);
                }
                else {
                    data.ip[nvals] = (int)(dval + 0.5);
                }

                break;

            case CDS_FLOAT:

                dval = strtod(strp, &endp);

                if (dval < CDS_MIN_FLOAT) {
                    data.fp[nvals] = CDS_MIN_FLOAT;
                }
                else if (dval > CDS_MAX_FLOAT) {
                    data.fp[nvals] = CDS_MAX_FLOAT;
                }
                else {
                    data.fp[nvals] = (float)dval;
                }

                break;

            case CDS_DOUBLE:

                dval = strtod(strp, &endp);

                if (dval < CDS_MIN_DOUBLE) {
                    data.dp[nvals] = CDS_MIN_DOUBLE;
                }
                else if (dval > CDS_MAX_DOUBLE) {
                    data.dp[nvals] = CDS_MAX_DOUBLE;
                }
                else {
                    data.dp[nvals] = dval;
                }

                break;

            case CDS_CHAR:
                data.cp[nvals] = *strp;
                endp = strp + 1;
                break;

            default:

                if (length) *length = 0;
                if (!array) {
                    free(data.vp);
                    return((void *)NULL);
                }

                break;
        }

        if (endp != strp) {

            nvals++;
            strp = endp;

            if (*strp == '\0') break;

            if (length) {
                if (array && (nvals == *length)) break;
            }
        }
    }

    if (length) *length = nvals;

    if (!array && !nvals) {
        free(data.vp);
        return((void *)NULL);
    }

    return(data.vp);
}

/**
 *  Convert a text string to an array of values.
 *
 *  This function is identical to cds_string_to_array() except that
 *  it will convert values that are out of range for the specified
 *  data type to the default fill value for the data type.
 *
 *  Memory will be allocated for the returned array if the output array
 *  is NULL. In this case the calling process is responsible for freeing
 *  the allocated memory.
 *
 *  @param  string - pointer to the input string
 *  @param  type   - data type of the output array
 *  @param  length - pointer to the length of the output array
 *                     - input:
 *                         - maximum length of the output array
 *                         - ignored if the output array is NULL
 *                     - output:
 *                         - number of values written to the output array
 *                         - 0 if the string did not contain any numeric values
 *                         - (size_t)-1 if a memory allocation error occurs
 *  @param  array  - pointer to the output array
 *                   or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the output array
 *    - NULL if:
 *        - the string did not contain any numeric values (length == 0)
 *        - a memory allocation error occured (length == (size_t)-1)
 */
void *cds_string_to_array_use_fill(
    const char  *string,
    CDSDataType  type,
    size_t      *length,
    void        *array)
{
    size_t  type_size = cds_data_type_size(type);
    char   *strp;
    char   *endp;
    size_t  nvals;
    size_t  nalloced;
    void   *new_data;
    CDSData data;
    double  dval;

    if (!string) {
        if (length) *length = 0;
        return((void *)NULL);
    }

    data.vp  = array;
    nvals    = 0;
    nalloced = 0;

    for (strp = (char *)string; *strp != '\0'; strp++) {

        /* Skip white-space */

        while (isspace(*strp)) strp++;

        if (*strp == '\0') break;

        /* Allocate more memory if necessary */

        if (!array && (nvals == nalloced)) {

            if (nalloced) {
                nalloced *= 2;
            }
            else {
                nalloced = 1;
            }

            new_data = realloc(data.vp, nalloced * type_size);
            if (!new_data) {
                if (length) *length = -1;
                free(data.vp);
                return((void *)NULL);
            }

            data.vp = new_data;
        }

        /* Get the next array element from the string */

        switch (type) {

            case CDS_BYTE:

                dval = strtod(strp, &endp);

                if (dval < CDS_MIN_BYTE) {
                    data.bp[nvals] = CDS_FILL_BYTE;
                }
                else if (dval > CDS_MAX_BYTE) {
                    data.bp[nvals] = CDS_FILL_BYTE;
                }
                else if (dval < 0) {
                    data.bp[nvals] = (signed char)(dval - 0.5);
                }
                else {
                    data.bp[nvals] = (signed char)(dval + 0.5);
                }

                break;

            case CDS_SHORT:

                dval = strtod(strp, &endp);

                if (dval < CDS_MIN_SHORT) {
                    data.sp[nvals] = CDS_FILL_SHORT;
                }
                else if (dval > CDS_MAX_SHORT) {
                    data.sp[nvals] = CDS_FILL_SHORT;
                }
                else if (dval < 0) {
                    data.sp[nvals] = (short)(dval - 0.5);
                }
                else {
                    data.sp[nvals] = (short)(dval + 0.5);
                }

                break;

            case CDS_INT:

                dval = strtod(strp, &endp);

                if (dval < CDS_MIN_INT) {
                    data.ip[nvals] = CDS_FILL_INT;
                }
                else if (dval > CDS_MAX_INT) {
                    data.ip[nvals] = CDS_FILL_INT;
                }
                else if (dval < 0) {
                    data.ip[nvals] = (int)(dval - 0.5);
                }
                else {
                    data.ip[nvals] = (int)(dval + 0.5);
                }

                break;

            case CDS_FLOAT:

                dval = strtod(strp, &endp);

                if (dval < CDS_MIN_FLOAT) {
                    data.fp[nvals] = CDS_FILL_FLOAT;
                }
                else if (dval > CDS_MAX_FLOAT) {
                    data.fp[nvals] = CDS_FILL_FLOAT;
                }
                else {
                    data.fp[nvals] = (float)dval;
                }

                break;

            case CDS_DOUBLE:

                dval = strtod(strp, &endp);

                if (dval < CDS_MIN_DOUBLE) {
                    data.dp[nvals] = CDS_FILL_DOUBLE;
                }
                else if (dval > CDS_MAX_DOUBLE) {
                    data.dp[nvals] = CDS_FILL_DOUBLE;
                }
                else {
                    data.dp[nvals] = dval;
                }

                break;

            case CDS_CHAR:
                data.cp[nvals] = *strp;
                endp = strp + 1;
                break;

            default:

                if (length) *length = 0;
                if (!array) {
                    free(data.vp);
                    return((void *)NULL);
                }

                break;
        }

        if (endp != strp) {

            nvals++;
            strp = endp;

            if (*strp == '\0') break;

            if (length) {
                if (array && (nvals == *length)) break;
            }
        }
    }

    if (length) *length = nvals;

    if (!array && !nvals) {
        free(data.vp);
        return((void *)NULL);
    }

    return(data.vp);
}

/**
 *  Convert an array of values to a text string.
 *
 *  This is a wrapper function to cds_sprint_array() with:
 *
 *    - indent  = NULL
 *    - maxline = 0
 *    - linepos = 0
 *    - flags   = 0x08 | 0x10
 *
 *  Memory will be allocated for the returned string if the output string
 *  is NULL. In this case the calling process is responsible for freeing
 *  the allocated memory.
 *
 *  @param  type          - data type of the array
 *  @param  array_length  - the length of the input array
 *  @param  array         - pointer to the input array
 *  @param  string_length - pointer to the length of the output string
 *                            - input:
 *                                - maximum length of the output string
 *                                - ignored if the output string is NULL
 *                            - output:
 *                                - length of the output string
 *                                - 0 if array_length is zero
 *                                - (size_t)-1 if a memory allocation error occurs
 *  @param  string        - pointer to the output string
 *                          or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the output string
 *    - NULL if:
 *        - array_length is zero (string_length == 0)
 *        - a memory allocation error occured (string_length == (size_t)-1)
 */
char *cds_array_to_string(
    CDSDataType  type,
    size_t       array_length,
    void        *array,
    size_t      *string_length,
    char        *string)
{
    return(cds_sprint_array(
        type, array_length, array, string_length, string,
        NULL, 0, 0, 0x08 | 0x10));
}
