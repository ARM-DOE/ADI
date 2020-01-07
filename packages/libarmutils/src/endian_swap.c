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
*    $Revision: 9450 $
*    $Author: ermold $
*    $Date: 2011-10-14 16:50:01 +0000 (Fri, 14 Oct 2011) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file endian_swap.c
 *  Endian Swapping Functions.
 */

#include "armutils.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Convert array of 16 bit big endian values to native byte order.
 *
 *  @param  data  - pointer to the array of data values
 *  @param  nvals - number of values in the data array
 *
 *  @return pointer to the array of data values
 */
void *bton_16(void *data, size_t nvals)
{
#ifndef _BIG_ENDIAN
    uint16_t *dp = (uint16_t *)data;
    size_t    i;

    for (i = 0; i < nvals; ++i) {
        dp[i] = SWAP_BYTES_16(dp[i]);
    }

#endif
    return(data);
}

/**
 *  Convert array of 32 bit big endian values to native byte order.
 *
 *  @param  data  - pointer to the array of data values
 *  @param  nvals - number of values in the data array
 *
 *  @return pointer to the array of data values
 */
void *bton_32(void *data, size_t nvals)
{
#ifndef _BIG_ENDIAN
    uint32_t *dp = (uint32_t *)data;
    size_t i;

    for (i = 0; i < nvals; ++i) {
        dp[i] = SWAP_BYTES_32(dp[i]);
    }

#endif
    return(data);
}

/**
 *  Convert array of 64 bit big endian values to native byte order.
 *
 *  @param  data  - pointer to the array of data values
 *  @param  nvals - number of values in the data array
 *
 *  @return pointer to the array of data values
 */
void *bton_64(void *data, size_t nvals)
{
#ifndef _BIG_ENDIAN
    uint64_t *dp = (uint64_t *)data;
    size_t i;

    for (i = 0; i < nvals; ++i) {
        dp[i] = SWAP_BYTES_64(dp[i]);
    }

#endif
    return(data);
}

/**
 *  Convert array of 16 bit little endian values to native byte order.
 *
 *  @param  data  - pointer to the array of data values
 *  @param  nvals - number of values in the data array
 *
 *  @return pointer to the array of data values
 */
void *lton_16(void *data, size_t nvals)
{
#ifdef _BIG_ENDIAN
    uint16_t *dp = (uint16_t *)data;
    size_t i;

    for (i = 0; i < nvals; ++i) {
        dp[i] = SWAP_BYTES_16(dp[i]);
    }

#endif
    return(data);
}

/**
 *  Convert array of 32 bit little endian values to native byte order.
 *
 *  @param  data  - pointer to the array of data values
 *  @param  nvals - number of values in the data array
 *
 *  @return pointer to the array of data values
 */
void *lton_32(void *data, size_t nvals)
{
#ifdef _BIG_ENDIAN
    uint32_t *dp = (uint32_t *)data;
    size_t i;

    for (i = 0; i < nvals; ++i) {
        dp[i] = SWAP_BYTES_32(dp[i]);
    }

#endif
    return(data);
}

/**
 *  Convert array of 64 bit little endian values to native byte order.
 *
 *  @param  data  - pointer to the array of data values
 *  @param  nvals - number of values in the data array
 *
 *  @return pointer to the array of data values
 */
void *lton_64(void *data, size_t nvals)
{
#ifdef _BIG_ENDIAN
    uint64_t *dp = (uint64_t *)data;
    size_t i;

    for (i = 0; i < nvals; ++i) {
        dp[i] = SWAP_BYTES_64(dp[i]);
    }

#endif
    return(data);
}

/**
 *  Read 16 bit values from a big endian binary file.
 *
 *  This function will read 16 bit values from a big endian binary file
 *  and convert them to the native byte order.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  fd    - file descriptor
 *  @param  data  - pointer to the output data array
 *  @param  nvals - number of data values to read
 *
 *  @return
 *    - number of data values successfully read
 *    - -1 if an error occurred
 */
int bton_read_16(int fd, void *data, size_t nvals)
{
    ssize_t bytes_read;
    int     vals_read;

    bytes_read = read(fd, data, 2*nvals);
    if (bytes_read < 0) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not read data from file: %s\n", strerror(errno));

        return(-1);
    }

    vals_read = bytes_read/2;

#ifndef _BIG_ENDIAN
    {
        uint16_t *dp = (uint16_t *)data;
        int i;

        for (i = 0; i < vals_read; ++i) {
            dp[i] = SWAP_BYTES_16(dp[i]);
        }
    }
#endif

    return(vals_read);
}

/**
 *  Read 32 bit values from a big endian binary file.
 *
 *  This function will read 32 bit values from a big endian binary file
 *  and convert them to the native byte order.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  fd    - file descriptor
 *  @param  data  - pointer to the output data array
 *  @param  nvals - number of data values to read
 *
 *  @return
 *    - number of data values successfully read
 *    - -1 if an error occurred
 */
int bton_read_32(int fd, void *data, size_t nvals)
{
    ssize_t bytes_read;
    int     vals_read;

    bytes_read = read(fd, data, 4*nvals);
    if (bytes_read < 0) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not read data from file: %s\n", strerror(errno));

        return(-1);
    }

    vals_read = bytes_read/4;

#ifndef _BIG_ENDIAN
    {
        uint32_t *dp = (uint32_t *)data;
        int i;

        for (i = 0; i < vals_read; ++i) {
            dp[i] = SWAP_BYTES_32(dp[i]);
        }
    }
#endif

    return(vals_read);
}

/**
 *  Read 64 bit values from a big endian binary file.
 *
 *  This function will read 64 bit values from a big endian binary file
 *  and convert them to the native byte order.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  fd    - file descriptor
 *  @param  data  - pointer to the output data array
 *  @param  nvals - number of data values to read
 *
 *  @return
 *    - number of data values successfully read
 *    - -1 if an error occurred
 */
int bton_read_64(int fd, void *data, size_t nvals)
{
    ssize_t bytes_read;
    int     vals_read;

    bytes_read = read(fd, data, 8*nvals);
    if (bytes_read < 0) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not read data from file: %s\n", strerror(errno));

        return(-1);
    }

    vals_read = bytes_read/8;

#ifndef _BIG_ENDIAN
    {
        uint64_t *dp = (uint64_t *)data;
        int i;

        for (i = 0; i < vals_read; ++i) {
            dp[i] = SWAP_BYTES_64(dp[i]);
        }
    }
#endif

    return(vals_read);
}

/**
 *  Read 16 bit values from a little endian binary file.
 *
 *  This function will read 16 bit values from a little endian binary file
 *  and convert them to the native byte order.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  fd    - file descriptor
 *  @param  data  - pointer to the output data array
 *  @param  nvals - number of data values to read
 *
 *  @return
 *    - number of data values successfully read
 *    - -1 if an error occurred
 */
int lton_read_16(int fd, void *data, size_t nvals)
{
    ssize_t bytes_read;
    int     vals_read;

    bytes_read = read(fd, data, 2*nvals);
    if (bytes_read < 0) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not read data from file: %s\n", strerror(errno));

        return(-1);
    }

    vals_read = bytes_read/2;

#ifdef _BIG_ENDIAN
    {
        uint16_t *dp = (uint16_t *)data;
        int i;

        for (i = 0; i < vals_read; ++i) {
            dp[i] = SWAP_BYTES_16(dp[i]);
        }
    }
#endif

    return(vals_read);
}

/**
 *  Read 32 bit values from a little endian binary file.
 *
 *  This function will read 32 bit values from a little endian binary file
 *  and convert them to the native byte order.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  fd    - file descriptor
 *  @param  data  - pointer to the output data array
 *  @param  nvals - number of data values to read
 *
 *  @return
 *    - number of data values successfully read
 *    - -1 if an error occurred
 */
int lton_read_32(int fd, void *data, size_t nvals)
{
    ssize_t bytes_read;
    int     vals_read;

    bytes_read = read(fd, data, 4*nvals);
    if (bytes_read < 0) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not read data from file: %s\n", strerror(errno));

        return(-1);
    }

    vals_read = bytes_read/4;

#ifdef _BIG_ENDIAN
    {
        uint32_t *dp = (uint32_t *)data;
        int i;

        for (i = 0; i < vals_read; ++i) {
            dp[i] = SWAP_BYTES_32(dp[i]);
        }
    }
#endif

    return(vals_read);
}

/**
 *  Read 64 bit values from a little endian binary file.
 *
 *  This function will read 64 bit values from a little endian binary file
 *  and convert them to the native byte order.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  fd    - file descriptor
 *  @param  data  - pointer to the output data array
 *  @param  nvals - number of data values to read
 *
 *  @return
 *    - number of data values successfully read
 *    - -1 if an error occurred
 */
int lton_read_64(int fd, void *data, size_t nvals)
{
    ssize_t bytes_read;
    int     vals_read;

    bytes_read = read(fd, data, 8*nvals);
    if (bytes_read < 0) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not read data from file: %s\n", strerror(errno));

        return(-1);
    }

    vals_read = bytes_read/8;

#ifdef _BIG_ENDIAN
    {
        uint64_t *dp = (uint64_t *)data;
        int i;

        for (i = 0; i < vals_read; ++i) {
            dp[i] = SWAP_BYTES_64(dp[i]);
        }
    }
#endif

    return(vals_read);
}
