/*******************************************************************************
*
*  Your copyright notice
*
********************************************************************************
*
*  Author:
*     name:  Tim Shippert
*     phone: 5093755997
*     email: tim.shippert@pnl.gov
*
********************************************************************************
*
*  REPOSITORY INFORMATION:
*    $Revision: 57306 $
*    $Author: shippert $
*    $Date: 2014-10-07 00:05:10 +0000 (Tue, 07 Oct 2014) $
*    $State: $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file trans_subsample.c
 *  Null functions, just set input to output.
 */
# include <stdio.h>
# include <math.h>
# include <string.h>
# include <time.h>
# include <getopt.h>
# include <regex.h>
# include <cds3.h>
# include "trans.h"

/* We'll be doing a lot of dynamic allocation, so let's make it easier */
# define CALLOC(n,t)  (t*)calloc(n, sizeof(t))
# define REALLOC(p,n,t)  (t*) realloc((char *) p, (n)*sizeof(t));

int trans_passthrough_interface(interface_s is)
//(double *data, int *qc_data, double *odata, int *qc_odata,
// CDSVar *invar, CDSVar *outvar, int d, TRANSmetric **met) 
{
  int ni, nt;

  // Don't want any metrics at all
  free_metric(is.met);

  // Pull out the stuff we need from invar, outvar, the dimension index d,
  // and the right calls to the transfrom parameter functions
  ni=is.invar->dims[is.d]->length;
  nt=is.outvar->dims[is.d]->length;

  if (ni != nt) {
    ERROR(TRANS_LIB_NAME, "Output variable not same size or shape as input for passthrough (%d vs. %d)",
	  ni, nt);
    return(-1);
  }

  //memcpy(odata,data,ni*sizeof(double));
  //memcpy(qc_odata,qc_data,ni*sizeof(int));

  memcpy(is.output_data,is.input_data,ni*sizeof(double));
  memcpy(is.output_qc, is.input_qc,ni*sizeof(int));

  return(0);
}

