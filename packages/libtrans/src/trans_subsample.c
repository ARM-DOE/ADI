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
*    $Revision: 75855 $
*    $Author: shippert $
*    $Date: 2017-01-13 17:08:40 +0000 (Fri, 13 Jan 2017) $
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
# include <cds3.h>
# include "trans.h"
# include "trans_private.h"
# include "timing.h"

static size_t _one=1;

/* We'll be doing a lot of dynamic allocation, so let's make it easier */
# define CALLOC(n,t)  (t*)calloc(n, sizeof(t))
# define REALLOC(p,n,t)  (t*) realloc((char *) p, (n)*sizeof(t));

#define NUM_METRICS 1
static const char *metnames[] = {
  "dist"
};

static const char *metunits[] = {
  "SAME"
};

int trans_subsample_interface(interface_s is)
//(double *data, int *qc_data, double *odata, int *qc_odata,
// CDSVar *invar, CDSVar *outvar, int d, TRANSmetric **met) 
{

  double *index=NULL, range, *target=NULL, missing_value;
  unsigned int qc_mask=0;
  int ni, nt, status, m;
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

  outcoord = cds_get_coord_var(outvar, d);
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

  if (cds_get_transform_param_by_dim(invar, invar->dims[d],
				     "missing_value", CDS_DOUBLE,
				     &_one, &missing_value) != NULL) {
    input_missing_value=missing_value;

    //trans_store_param_text_by_dim(invar, invar->dims[d], "input_missing_value",
    //outvar->dims[od]->name, outvar->name);
    trans_store_param_val("input_missing_value", "%f",
			  missing_value, outvar->dims[od]->name, outvar->name);
  }

  if (cds_get_transform_param_by_dim(outvar, invar->dims[d],
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

  // TRS - 5/28/14
  // Subsample from bin midpoints.
  // The beauty of this is that the odata arrays will still be
  // indexed correctly by the target array, so we don't even need to
  // translate back.  But, instead of subsampling from index to target, we
  // subsample from index_mid to target_mid

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

  //status=subsample(data, qc_data, qc_mask, index_mid, ni, range,
  //		   odata, qc_odata, target_mid, nt, missing_value,
  //		   &metrics);

  TIMER_LOG(
	    status=call_core_function(subsample,
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
	    );

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

  if (index) free(index);
  if (target) free(target);
  if (index_mid) free(index_mid);
  if (target_mid) free(target_mid);

  return(status);
}

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
//double ***rmet

int subsample (core_s cs) {

  int i,j,iold, it,m;
  double d, smallest_d, dist;
  int status=0;
  double **metrics;
  double *distance;

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

  int smallest_d_last_good_value, first_iteration_of_while_loop;

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
  distance=metrics[0];

  // Great - now I can simply calculate dist[j] and everything will be cool.

  // again, i indexes our input field, j indexes the target field
  iold=0;
  smallest_d_last_good_value = 0;
  for (j=0; j<nt; j++) {
    qc_output[j]=0;

    // Set our scanning input value to the edge of the last window
    i=iold;

    // So, we need to find the nearest point, so start by setting
    // distance big
    dist=HUGE_VAL;
    smallest_d=dist;
    it = -1;

    // Scan up until we are at least within dist on the left side
    while (index[i] < target[j]-range && i < ni) i++;

    // This above worked only with increasing coords, and its weird with
    // decreasing ones, so recast to just measure the distance and compare
    // with range.
    // NOPE! This, below, does not work if we generate a missing output,
    // because index[i] will have already run past the range of target[j],
    // so it will just scan up the array until i=ni, skipping all the other
    // js.
    //while (i < ni && fabs(index[i]-target[j]) > range) i++;

    // So, we'll have to fix this for decreasing coordinates. - Maybe this?

    if (i==ni) { 
      //msg_ELog(EF_PROBLEM, "subsample: no input values within range of target %f",
      // target[j]);
      status=1;

      // We know that all our j's after this will be outside range, because
      // we are at the top of our input array.  So:

      while (j < nt) {
	// Need to zero this out, because we are jumping the j loop
	qc_output[j]=0;
	qc_set(qc_output[j], QC_OUTSIDE_RANGE);
	qc_set(qc_output[j], QC_BAD); // 11/26/12
	output[j]=output_missing_value;
	distance[j]=output_missing_value;
	j++;
      }
      break;  // out of the j loop
    }

    // Save this i for later - we know i's less than this cannot be within
    // dist for j's greater than this, so we'll start our later scans from
    // this point.
    // iold=i;
    
    // Now scan up i's until we find the one that's closest, absolutely, to
    // our target index
    //while (index[i] <= target[j]+range && i < ni) {
    first_iteration_of_while_loop = 1; 
    while (i < ni && (d=fabs(index[i]-target[j])) <= range) {

      // keep track of smallest absolute distance, regardless of whether
      // that point is decent or not...
      if (d < smallest_d) { 
	smallest_d=d;
      }

      //KLG new logic to possible address range issue.
      //    if index[i] > target[j] from the start of this while loop then we want to reset
      //    smallest_d to the smallest_d used when we set the last good value. 
      if ( (j != 0) && first_iteration_of_while_loop == 1) {
          if (index[i] > target[j]) {
              smallest_d = smallest_d_last_good_value;
          }
      }

      // ...but we only want to set this as our target distance if it
      // passes qc and whatnot
      if (d < dist && array[i] != input_missing_value
	  && (! (qc_array[i] & qc_mask))
	  && isfinite(array[i])) {
	dist=d; it=i;
      }

      // If we are growing in distance and we already have a good value,
      // then we can break out of this loop.
      if (d > dist && it > 0) break;

      first_iteration_of_while_loop = 0; 
      i++;
    }

    if (it < 0) {
      // THis is not actually a big deal - it could simply be at night.  So
      // don't overwhelm the logger with messages.
      // msg_ELog(EF_PROBLEM, "subsample: no good input values within range of target %f",
      //     target[j]);

      // Of course, level five is what overwhelming messages is all about
      DEBUG_LV5("libtrans",
		"No good input values for output bin %d, index value %f",
		j, target[j]);

      output[j]=output_missing_value;
      distance[j]=output_missing_value;
      status=1;
      qc_set(qc_output[j], QC_ALL_BAD_INPUTS);
      qc_set(qc_output[j], QC_BAD);

      // If we are at the top of our input array, then we know all the
      // remaining output points also have either bad input or are outside
      // our range.  So, do that now:
      if (i == ni) {
	while (++j < nt) {
	  output[j]=output_missing_value;
	  distance[j]=output_missing_value;

	  // Need to zero this out, because we are jumping the j loop
	  qc_output[j]=0;
	  qc_set(qc_output[j], QC_BAD);
	  
	  // If our target is beyond the range of the final index point, set
	  // QC_OUTSIDE_RANGE.  Otherwise, QC_ALL_BAD_INPUTS
	  if (target[j] < index[ni-1]+range) {
	    qc_set(qc_output[j], QC_ALL_BAD_INPUTS);
	  } else {
	    qc_set(qc_output[j], QC_OUTSIDE_RANGE);
	  }
	}
	break;  // out of the j loop
      }

      // I believe, at this point, we can rule out anything less than the
      // current i as a valid start point for the next j.  The only way we
      // have it < 0 is if all the inputs are missing for all i's in this
      // loop.  Therefore, we can start with this i and continue.
      // Otherwise, if the data is full of missings, we end up O(n^2).
      iold=i;
      continue;
    } // end logic if no good points within range

    // Otherwise, use our stored it
    output[j]=array[it];

    //KLG new logic saving smallest_d of last good value
    smallest_d_last_good_value = smallest_d;

    // target[j+1] can be closest to no value of i < it, so save it as the
    // start part.

    // Actually, if index is monotonically opposed to target (one is
    // positive, the other is negative) then that above statement about i
    // >= it is no longer true.  Hum.
    iold=it;

    // I can't just use dist, as that's absolute distance and I want a
    // signed distance.  Negative will mean our input index was before the
    // target, positive will mean our index was after.  i.e. Add the distance to
    // the target to find the index.
    distance[j]=index[it]-target[j];

    // If the input point is yellow, set QC_INDETERMINATE on output 
    if ((qc_array[it] & ~qc_mask) != 0) {
      	qc_set(qc_output[j], QC_INDETERMINATE);
    }

    // Set qc=1 if we skipped over the actual nearest point, though
    if (dist > smallest_d) {
      qc_set(qc_output[j], QC_NOT_USING_CLOSEST);
    }
  }
   
  return(status);
}

