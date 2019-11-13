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
*    $Revision: 84796 $
*    $Author: shippert $
*    $Date: 2018-12-07 21:45:38 +0000 (Fri, 07 Dec 2018) $
*    $State: $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file trans_interpolate.c
 *  1-D Interpolation transformation functions
 */
# include <stdio.h>
# include <math.h>
# include <string.h>
# include <time.h>
# include <getopt.h>
# include <regex.h>
# include "cds3.h"
# include "trans.h"
# include "trans_private.h"

static size_t _one=1;

/* We'll be doing a lot of dynamic allocation, so let's make it easier */
# define CALLOC(n,t)  (t*)calloc(n, sizeof(t))
# define REALLOC(p,n,t)  (t*) realloc((char *) p, (n)*sizeof(t));

#define NUM_METRICS 2
static const char *metnames[] = {
  "dist_1",
  "dist_2"
};

static const char *metunits[] = {
  "SAME",
  "SAME"
};

int trans_interpolate_interface(interface_s is)
//(double *data, int *qc_data, double *odata, int *qc_odata,
// CDSVar *invar, CDSVar *outvar, int d, TRANSmetric **met) 
{

  int ni, nt, status, m;
  double *index=NULL, *target=NULL, range, missing_value;
  unsigned int qc_mask=0;
  CDSVar *incoord, *outcoord;
  double **metrics=NULL;
  TRANSmetric *met1d;

  // Assign from interface struct - if I had written it this way to begin
  // with I would just is the is elements, but I didn't.
  double *data=is.input_data;
  int *qc_data=is.input_qc;
  double input_missing_value=is.input_missing_value;
  double *odata=is.output_data;
  int *qc_odata=is.output_qc;
  double output_missing_value=is.output_missing_value;
  CDSVar *invar=is.invar;
  CDSVar *outvar=is.outvar;
  int d=is.d;
  int od=is.od;
  TRANSmetric **met=is.met;

  // Right now, just null out our metrics, until we have some to set
  //free_metric(met);

  // Pull out the stuff we need from invar, outvar, the dimension index d,
  // and the right calls to the transfrom parameter functions
  ni=invar->dims[d]->length;
  nt=outvar->dims[od]->length;

  // Define and allocate our metric
  allocate_metric(met,metnames, metunits, NUM_METRICS, nt);
  met1d = (*met);

  // These mostly work straight up because coord vars are always 1D, so we
  // don't have to worry about indexing or casting correctly
  incoord = cds_get_coord_var(invar, d);
  index=(double *) cds_copy_array(incoord->type, ni,
				  incoord->data.vp,
				  CDS_DOUBLE, NULL,
				  0, NULL, NULL, NULL, NULL, NULL, NULL); 

  outcoord = cds_get_coord_var(outvar, od);
  target=(double *) cds_copy_array(outcoord->type, nt,
				  outcoord->data.vp,
				  CDS_DOUBLE, NULL,
				   0, NULL, NULL, NULL, NULL, NULL, NULL); 

  // Here's our lookup parameters
  if ((cds_get_transform_param_by_dim(invar, invar->dims[d],
				      "range", CDS_DOUBLE,
				      &_one, &range) == NULL) &&
      (cds_get_transform_param_by_dim(outvar, outvar->dims[od],
				     "range", CDS_DOUBLE,
				      &_one, &range) == NULL)) {
    range=CDS_MAX_DOUBLE;
    // Rather than store the max double, just say that range was not set
    trans_store_param("range","NONE", outvar->dims[od]->name, outvar->name);
  } else {
    // We have a valid range, so store it as a param
    trans_store_param_val("range", "%g",range,outvar->dims[od]->name, outvar->name);
  }

  // Override var missing values with transform params
  if (cds_get_transform_param_by_dim(invar, invar->dims[d],
				     "missing_value", CDS_DOUBLE,
				     &_one, &missing_value) != NULL) {
    input_missing_value=missing_value;

    //trans_store_param_text_by_dim(invar, invar->dims[d], "input_missing_value",
    //outvar->dims[od]->name, outvar->name);
    trans_store_param_val("input_missing_value", "%f",
			  missing_value, outvar->dims[od]->name, outvar->name);
  }

  if (cds_get_transform_param_by_dim(outvar, outvar->dims[od],
				     "missing_value", CDS_DOUBLE,
				     &_one, &missing_value) != NULL) {
    output_missing_value=missing_value;
  }

  CDSVar *qc_invar=get_qc_var(invar);
  if (qc_invar) {
    if (cds_get_transform_param_by_dim(qc_invar, qc_invar->dims[d],
				       "qc_mask", CDS_INT,
				       &_one, &qc_mask) == NULL) {
      qc_mask=get_qc_mask(invar);
    } else {
      trans_store_param_text_by_dim(qc_invar,  qc_invar->dims[d],
				    "qc_mask", outvar->dims[od]->name, outvar->name);
    }
  }

  // Translate our grid to midpoints for the interpolation.  
  double *index_mid = get_bin_midpoints(index, ni, invar, d);
  double *target_mid = get_bin_midpoints(target, nt, outvar, od);

  // Trap out if we failed to find any bin widths
  if (! index_mid) {
    ERROR(TRANS_LIB_NAME,
	  "Bin widths for input variable %s required but not provided.  Exiting...",
	  invar->name);
    return(-1);
  }

  if (! target_mid) {
    ERROR(TRANS_LIB_NAME,
	  "Bin widths for output variable %s required but not provided.  Exiting...",
	  outvar->name);
    return(-1);
  }

  // Now just call our core function
  //status=bilinear_interpolate(data, qc_data, qc_mask, index_mid, ni, range,
  //odata, qc_odata, target_mid, nt, missing_value,
  //&metrics);

  status=call_core_function(bilinear_interpolate,
			    .input_data=data,
			    .input_qc=qc_data,
			    .qc_mask=qc_mask,
			    .index=index_mid,
			    .nindex=ni,
			    .range=range,
			    .output_data=odata,
			    .output_qc=qc_odata,
			    .target=target_mid,
			    .ntarget=nt,
			    .input_missing_value=input_missing_value,
			    .output_missing_value=output_missing_value,
			    .metrics=&metrics);

  // Set the qc bits if we estimated the bin boundaries
  set_estimated_bin_qc(qc_odata, invar, d, outvar, od, nt); 

  // Pass our metrics back to the driver.
  for (m=0; m < NUM_METRICS ; m++) {
    memcpy(met1d->metrics[m], metrics[m], nt*sizeof(double));
  }

  // And now free them up
  for (m=0;m<NUM_METRICS;m++) {
    free(metrics[m]);
  }
  free(metrics);

  // Whew! we've run the transform, so we are done.
  if (index) free(index);
  if (target) free(target);
  if (index_mid) free(index_mid);
  if (target_mid) free(target_mid);


  return(status);
}

// Interpolate a give array with a given index field onto the target index
//double *array,
//int *qc_array,
//unsigned int qc_mask, 
//double *index,
//int ni,
//double range, 
//double *output,
//int *qc_output,
//double *target,
//int nt,
//double missing_value, 
//double ***rmet){

int bilinear_interpolate(core_s cs) {
  int i,j,k, n1, n2;
  double u, x, y1, y2, x1, x2;
  int sign=1;
  int m;
  double **metrics;
  double *dist_1, *dist_2;

  // Convert core_s elements to interior elements.  AGain, if I'd written
  // this with core_s in mind I wouldn't need to do this.
  double *array = cs.input_data;
  int *qc_array = cs.input_qc;
  unsigned int qc_mask = cs.qc_mask;
  double *index = cs.index;
  int ni = cs.nindex;
  double *output = cs.output_data;
  int *qc_output = cs.output_qc;
  double *target = cs.target;
  int nt = cs.ntarget;
  double input_missing_value = cs.input_missing_value;
  double output_missing_value = cs.output_missing_value;
  double ***rmet = cs.metrics;

  double range = cs.range;

  // Just to keep things clear; rmet is the pointer to the 2D metrics
  // structure, and I prefer to work with that.
  metrics=*rmet;

  // Do some checking on our metrics
  if (metrics == NULL) {
    // We'll allocate so we can calculate stdev and coverage anyway, for
    // debugging and possibly for some other way of transmitting the
    // information.  But this allocation won't stick around after returning
    // out of this function.  If metrics has been allocated but
    // incorrectly (i.e. only one element), we'll get a core dump - how do
    // I check on that in C?
    metrics = CALLOC(NUM_METRICS, double *);
    *rmet=metrics;  // need to reassign our pointer so it carries back out
  }

  // These allocations, however, should stick around. 
  for (m=0;m<NUM_METRICS;m++) {
    if (metrics[m] == NULL) metrics[m]=CALLOC(nt, double);
  }

  // Okay, finally, assigning our metrics to something I understand
  // (I feel silly with just one metric, but whatever)
  dist_1=metrics[0];
  dist_2=metrics[1];

  // We obviously need at least two input points to interpolate.  Even if
  // we are going right on, all my logic below about finding monotonicity
  // and dealing with the edge bins works only if we have at least two
  // input points
  if (ni < 2) {
    WARNING(TRANS_LIB_NAME,
	    "Only %d input values: must have >= 2 input values to interpolate. Continuing...", ni);
    for (j=0;j<nt;j++) {
      output[j]=output_missing_value; 
      dist_1[j]=output_missing_value;
      dist_2[j]=output_missing_value;
      qc_set(qc_output[j], QC_OUTSIDE_RANGE);
      qc_set(qc_output[j], QC_BAD);
    }
    // Not fatal
    return(0);
  }

  // First, find whether we are monotonically increasing or decreasing;
  // this is important as we scan up to find what index is in our target
  // bin.  Take advantage of the fact that "a < b" is the same as "-a >
  // -b".
  // 5/30/17 - trap out cases where we have just one output point -
  // obviously we can't tell its monotonicity.  We have trapped out only
  // one input point above.
  if (nt > 1) {
    if (index[0] < index[1] && target[0] < target[1]) {
      sign = 1;
    } else if (index[0] > index[1] && target[0] > target[1]) {
      sign = -1;
    } else {
      ERROR(TRANS_LIB_NAME, "Target and index are not monotonically aligned");
      return(-5);
    }
  }

  // Now just run up the flag pole, interpolating as we go.  I don't know
  // what to do about edge effects yet.  i goes up the input index field, j
  // goes up the target field.

  i=0;
  for (j=0; j<nt; j++) {

    // we will modify this as conditions warrant
    qc_output[j]=0;

    // First, do not extrapolate beyond the actual range of inputs - that's
    // just silly.
    // ACtually, we are going to be fuzzy about this - extrapolate no more
    // than half an input bin beyond our input range.  This will allow us
    // to extrapolate from 318m to 316m, for example.
    if (sign*target[j] < sign*(index[0]-((index[1]-index[0])/2.0)) ||
	sign*target[j] > sign*(index[ni-1]+((index[ni-1]-index[ni-2])/2.0))) {
      output[j]=output_missing_value; // we used to set it to 0, but that was
			       // RIPBE specific
      dist_1[j]=dist_2[j]=output_missing_value;
      qc_set(qc_output[j], QC_OUTSIDE_RANGE);
      qc_set(qc_output[j], QC_BAD); // 11/26/12
      continue;
    }

    // run up until we find the next index that's just above this target
    // Note that this requires target values that are monotonically
    // increasing. 
    while (i < ni && sign*index[i] < sign*target[j]) { i++;}

    // This shortcircuits all the bracketting nonsense below for the case
    // where our coordinate target matches our coordinate index exactly.
    // In normal cases it doesn't matter, as one of our bracketing points
    // will have weight 1 and the other will have 0 - but if we have a
    // small range and the neighboring points are bad or missing, this can
    // lead to this particular point being set missing, even if array[i]
    // itself is good.
    // Need i<ni because we mmight run up to i=ni above
    if (i<ni && fabs(target[j]-index[i]) < 1e-8 &&
        fabs(array[i]-input_missing_value) > 1e-8 &&
        !(qc_array[i] & qc_mask) &&
        isfinite(array[i])) {
 
      output[j] = array[i];
      dist_1[j]=dist_2[j]=0;
      continue;
    }

    // All kinds of checking.  This, of course, is where I earn my money
    // This may seem like overkill, but there are a lot of exceptions I
    // need to deal with.  I need points (x1,y1) and (x2,y2), preferably
    // that bracket the value of x I'm interpolating to.  So, I do some
    // checks to find n1 and n2, which are the right indeces to use.
    // If we have bracketing points, then n1 and n2 are straightforward -
    // but if there are boundary issues then I set n1 and n2 to extrapolate
    // out.  *Then* I check to see if array[n1] and array[n2] are missing;
    // if they are, I need to iterate up and/or down until I find two good
    // points I can use.  Whew!
    if (i == ni) {
      // above the input range, so, extrapolate out from last two points
      // using the same formula
      n1=ni-2;
      n2=ni-1;
      // see below
      //qc_set(qc_output[j], QC_EXTRAPOLATE);
    } else if (i == 0) {
      // We can extrapolate down from the first two points, using the same
      // formula 
      n1=0;
      n2=1;
      // Not necessarily true, if our target point falls right on one of
      //our input points.  It's better to switch on the value of u below,
      //anyway 
      //qc_set(qc_output[j], QC_EXTRAPOLATE);
    } else {
      // we actually have input points that bracket our target, so whoopee
      n1=i-1;
      n2=i;
    }

    // If n1 is a missing value, then first scan down and then scan up to
    // find a good value, skipping the n1=n2 value.  This could lead to
    // n1>n2, if the first value is missing, but I don't think the
    // extrapolation actually cares

    // These conditionals are weird, because we need to limit check on n1
    // first, and then check all the ways that could indicate something is
    // wrong, such as n1==n2 or n1 < 0, as well as missing values or qc checks
    while ((n1 >= 0) &&
	   (fabs(array[n1]-input_missing_value) < 1e-8 ||
	    (qc_array[n1] & qc_mask) ||
	    ! isfinite(array[n1]))){
      qc_set(qc_output[j], QC_INTERPOLATE);
      n1--;
    }

    while ((n1 < ni) &&
	   (n1 < 0 || n1 == n2 ||
	    fabs(array[n1]-input_missing_value) < 1e-8 ||
	    (qc_array[n1] & qc_mask) ||
	    ! isfinite(array[n1]))) {
      qc_set(qc_output[j], QC_INTERPOLATE);
      n1++;
    }

    if (n1 >= ni) { 
      //msg_ELog(EF_PROBLEM, "bilinear_interpolate: All missing values in array");
      for (k=0;k<nt;k++) {
	output[k]=output_missing_value;
	dist_1[j]=dist_2[j]=output_missing_value;
	qc_set(qc_output[k], QC_ALL_BAD_INPUTS);
	qc_set(qc_output[k], QC_BAD);
      }
      return(2);
    }

    // Now if n2 is a missing value, first scan up and then scan down to
    // find a good value - careful, we can't have n1=n2, so we do an extra
    // check there.  Same thing as above - limit checks "anded" with all
    // the QC indicators "or'd" together
    while ((n2 < ni) &&
	   (n2 == n1 ||
	    fabs(array[n2]-input_missing_value) < 1e-8 ||
	    (qc_array[n2] & qc_mask) ||
	    ! isfinite(array[n2]))) {
      qc_set(qc_output[j], QC_INTERPOLATE);
      n2++;
    }
    while ((n2 > 0) &&
	   (n2 == n1 || n2 >= ni ||
	    fabs(array[n2]-input_missing_value) < 1e-8 ||
	    (qc_array[n2] & qc_mask) ||
	    ! isfinite(array[n2]))) {
      qc_set(qc_output[j], QC_INTERPOLATE);
      n2--;
    }

    if (n2 >= ni || n2 <= 0 || n2 == n1) { 
      //msg_ELog(EF_PROBLEM, "bilinear_interpolate: At most one good value in array");
      for (k=0;k<nt;k++) {
	output[k]=output_missing_value;
	dist_1[j]=dist_2[j]=output_missing_value;
	qc_set(qc_output[k], QC_ALL_BAD_INPUTS);
	qc_set(qc_output[k], QC_BAD);
      }
      return(2);
    }

    // Hopefully, we now have the input indeces to crank on
    x=target[j];

    x1=index[n1];
    x2=index[n2];

    y1=array[n1];
    y2=array[n2];

    // Final qc on range

    // First, do not extrapolate beyond the range of the actual input
    if (fabs(x-x1) > range || fabs(x-x2) > range) {
      output[j]=output_missing_value;
      qc_set(qc_output[j], QC_OUTSIDE_RANGE);
      qc_set(qc_output[j], QC_BAD); // 11/26/12
      continue;
    }

    // AT this point we are legally allowed to finish the interpolation,
    // with the appropriate qc flagging.

    // u is the fraction of the way up the bin to where our target value is
    u=(x-x1)/(x2-x1);

    // Now just do the weighted average
    output[j]=u*y2 + (1-u)*y1;

    // Keep signs and everything
    //dist_1[j]=index[n1]-target[j];
    //dist_2[j]=index[n2]-target[j];
    dist_1[j]=x1-x;
    dist_2[j]=x2-x;

    // This is how we tell if we extrapolated or not; if u is not in the
    // range [0,1], then it lies outside the bin of [x1,x2], and thus is an
    // extrapolation 
    if (u < 0 || u > 1) qc_set(qc_output[j], QC_EXTRAPOLATE);


    // Finally, merge in the input QC from the two points we use.  However,
    // if u == 1, don't use n1 and if u = 0 don't use n2, because our
    // weights on those two points will be 0 in those cases.

    if (fabs(u-1) > 1e-5 && (qc_array[n1] & ~qc_mask) != 0) {
      qc_set(qc_output[j],QC_INDETERMINATE);
    }

    if (fabs(u) > 1e-5 && (qc_array[n2] & ~qc_mask) != 0) {
      qc_set(qc_output[j],QC_INDETERMINATE);
    }

  }
  return(0);
}
    
