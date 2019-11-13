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
*    $Revision: 6421 $
*    $Author: ermold $
*    $Date: 2011-04-26 01:23:44 +0000 (Tue, 26 Apr 2011) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file endian_swap.h
 *  Endian Swapping Functions
 */

#ifndef _ENDIAN_SWAP_H
#define _ENDIAN_SWAP_H

/**
 *  @defgroup ARMUTILS_ENDIAN_SWAP Endian Swapping
 */
/*@{*/

/** Swap byes in 16 bit value. */
#define SWAP_BYTES_16(x) \
    ( ( ((x) & 0xff00) >> 8) | \
      ( ((x) & 0x00ff) << 8) )

/** Swap byes in 32 bit value. */
#define SWAP_BYTES_32(x) \
    ( ( ((x) & 0xff000000) >> 24) | \
      ( ((x) & 0x00ff0000) >> 8)  | \
      ( ((x) & 0x0000ff00) << 8)  | \
      ( ((x) & 0x000000ff) << 24) )

/** Swap byes in 64 bit value. */
#define SWAP_BYTES_64(x) \
    ( ( ((x) & 0xff00000000000000LL) >> 56) | \
      ( ((x) & 0x00ff000000000000LL) >> 40) | \
      ( ((x) & 0x0000ff0000000000LL) >> 24) | \
      ( ((x) & 0x000000ff00000000LL) >> 8)  | \
      ( ((x) & 0x00000000ff000000LL) << 8)  | \
      ( ((x) & 0x0000000000ff0000LL) << 24) | \
      ( ((x) & 0x000000000000ff00LL) << 40) | \
      ( ((x) & 0x00000000000000ffLL) << 56) )

#ifdef _BIG_ENDIAN
  #define BTON_16(x) (x)
  #define BTON_32(x) (x)
  #define BTON_64(x) (x)
  #define LTON_16(x) SWAP_BYTES_16(x)
  #define LTON_32(x) SWAP_BYTES_32(x)
  #define LTON_64(x) SWAP_BYTES_64(x)
#else
  #define BTON_16(x) SWAP_BYTES_16(x)
  #define BTON_32(x) SWAP_BYTES_32(x)
  #define BTON_64(x) SWAP_BYTES_64(x)
  #define LTON_16(x) (x)
  #define LTON_32(x) (x)
  #define LTON_64(x) (x)
#endif

void *bton_16(void *data, size_t nvals);
void *bton_32(void *data, size_t nvals);
void *bton_64(void *data, size_t nvals);

void *lton_16(void *data, size_t nvals);
void *lton_32(void *data, size_t nvals);
void *lton_64(void *data, size_t nvals);

int bton_read_16(int fd, void *data, size_t nvals);
int bton_read_32(int fd, void *data, size_t nvals);
int bton_read_64(int fd, void *data, size_t nvals);

int lton_read_16(int fd, void *data, size_t nvals);
int lton_read_32(int fd, void *data, size_t nvals);
int lton_read_64(int fd, void *data, size_t nvals);

/*@}*/

#endif /* _ENDIAN_SWAP_H */
