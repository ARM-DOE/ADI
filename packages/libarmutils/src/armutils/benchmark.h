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

/** @file benchmark.h
*  Benchmark Functions
*/

#ifndef _BENCHMARK_H
#define _BENCHMARK_H

/**
*  @defgroup ARMUTILS_BENCHMARK_UTILS Benchmark Utils
*/
/*@{*/

void benchmark_init(void);
void benchmark(FILE *fp, char *message);

/*@}*/

#endif /* _BENCHMARK_H */
