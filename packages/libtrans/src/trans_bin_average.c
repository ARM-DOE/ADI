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

/** @file trans_bin_average
 *  1-D Bin average transformation functions
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

// For some crazy reason, we don't get this or MAXFLOAT in math.h, and I'm
// tired of figurin it out, so here we go
# define HUGE		3.40282347e+38F

static size_t _one=1;

#define NUM_METRICS 2
static const char *metnames[] = {
  "std",
  "goodfraction"
};

static const char *metunits[] = {
  "SAME",
  "unitless"
};

/* We'll be doing a lot of dynamic allocation, so let's make it easier */
# define CALLOC(n,t)  (t*)calloc(n, sizeof(t))
# define REALLOC(p,n,t)  (t*) realloc((char *) p, (n)*sizeof(t));

// Average values into our bins - this is why we need the exact
// specification of our edge, so we can properly weight everything that
// goes into the average.

// Our bins will be given by _start and _end, which allows gaps
// in our bins.

/*
int trans_bin_average(float *array,
		int *qc_array,
		unsigned int qc_mask, 
		float *index_start,
		float *index_end,
		float *weights,
		int ni,
		float *output,
		int *qc_output,
		float *target_start,
		float *target_end,
		int nt,
		float missing_value) { 
*/

//int trans_bin_average_interface
//(double *data, int *qc_data, double *odata, int *qc_odata,
// CDSVar *invar, CDSVar *outvar, int d, TRANSmetric **met) 

int trans_bin_average_interface(interface_s is)
{

  int ni, nt, i, status,m;
  double *index_start=NULL, *index_end=NULL, *target_start=NULL,
    *target_end=NULL, *weights=NULL;
  double missing_value;
  double **metrics=NULL;
  unsigned int qc_mask=0;
  size_t len;
  CDSVar *incoord, *outcoord;
  TRANSmetric *met1d;

  // Unlike other interfaces, these are just markers for the dimensions,
  // which we'll use to infer the bin edges.  
  double *index=NULL, *target=NULL;

  // Assign from interface struct - if I had written it this way to begin
  // with I would just use the is elements, but I didn't.
  double *data=is.input_data;
  int *qc_data=is.input_qc;
  double input_missing_value=is.input_missing_value;
  double *odata=is.output_data;
  int *qc_odata=is.output_qc;
  double output_missing_value=is.output_missing_value;
  CDSVar *invar=is.invar;
  CDSVar *outvar=is.outvar;
  int d=is.d;   // input and output dimensions may be different.
  int od=is.od;
  TRANSmetric **met=is.met;
  
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

  // The easy lookups - override missing_Value by transform params.
  if (cds_get_transform_param_by_dim(invar, invar->dims[d],
				     "missing_value", CDS_DOUBLE,
				     &_one, &missing_value) != NULL) {
    input_missing_value=missing_value;

    // Need to save anything that modifies input data: note, we are still
    // tagging with output dim and varname, because we store it in the
    // output field.
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

  ///////////////////////////////////////////////////////////////////////////
  // Weights - a little idiot checking
  if ((weights=cds_get_transform_param_by_dim(invar, invar->dims[d],
					     "weights", CDS_DOUBLE,
					      &len, weights)) == NULL) {
    weights=CALLOC(ni, double);
    for (i=0;i<ni;i++) {weights[i]=1.0;}
  } else if (len != (size_t) ni) {
    ERROR(TRANS_LIB_NAME, "Bin average weights array for %s (%s) different size then input data (%s, %s); setting weights=1.0", 
	  invar->name, invar->dims[d]->name, len, ni);
    if (weights) free(weights);
    weights=CALLOC(ni, double);
    for (i=0;i<ni;i++) {weights[i]=1.0;}
  }

  // Should I save the weights param here in cds_transform via
  // store_param()?  My answer is no, but only because it is probably a
  // very large array of values.

  /////////////////////////////////////////////
  // Limits on metrics for QC states
  // The order is bad std, indeterminate std, bad coverage, ind coverage
  // The first two are maxima, the latter are minima (on [0,1])
  double limits[] = {HUGE, HUGE, -1, -1};
  double std_bad_max, std_ind_max, goodfrac_bad_min, goodfrac_ind_min;

  if (cds_get_transform_param_by_dim(outvar, outvar->dims[od],
				     "std_bad_max", CDS_DOUBLE,
				     &_one, &std_bad_max) != NULL) {
    limits[0]=std_bad_max;

    trans_store_param_text_by_dim(outvar, outvar->dims[od],
				  "std_bad_max",outvar->dims[od]->name, outvar->name);
  }

  if (cds_get_transform_param_by_dim(outvar, outvar->dims[od],
				     "std_ind_max", CDS_DOUBLE,
				     &_one, &std_ind_max) != NULL) {
    limits[1]=std_ind_max;

    trans_store_param_text_by_dim(outvar, outvar->dims[od],
				  "std_ind_max", outvar->dims[od]->name, outvar->name);
  }

  if (cds_get_transform_param_by_dim(outvar, outvar->dims[od],
				     "goodfrac_bad_min", CDS_DOUBLE,
				     &_one, &goodfrac_bad_min) != NULL) {
    limits[2]=goodfrac_bad_min;

    trans_store_param_text_by_dim(outvar,  outvar->dims[od],
				  "goodfrac_bad_min",outvar->dims[od]->name, outvar->name);
  }

  if (cds_get_transform_param_by_dim(outvar, outvar->dims[od],
				     "goodfrac_ind_min", CDS_DOUBLE,
				     &_one, &goodfrac_ind_min) != NULL) {
    limits[3]=goodfrac_ind_min;

    trans_store_param_text_by_dim(outvar, outvar->dims[od],
				  "goodfrac_ind_min",outvar->dims[od]->name, outvar->name);
  }

  // To do the edges, we'll drop into a subfunction, because we have to do
  // it with both input and output edges.  We'll use Brian's trick of
  // passing in a pointer to our pointers, so we can set the addresses.

  if ((status = get_bin_edges(&index_start, &index_end, index, ni, invar, d)) < 0) {
    ERROR(TRANS_LIB_NAME,
	  "Bin widths for input variable %s required but not provided.  Exiting...",
	  invar->name);
    return(-1);
  }

  if ((status = get_bin_edges(&target_start, &target_end, target, nt, outvar, od)) < 0) {
    ERROR(TRANS_LIB_NAME,
	  "Bin widths for output variable %s required but not provided.  Exiting...",
	  outvar->name);
    return(-1);
  }

  // We now need to idiot-check our output grid - we need a non-zero width
  // for each output bin, or else averaging doesn't make sense.  We can
  // accept a zero width on input, but not on output. 
  for (i=0;i<nt;i++) {
    if (target_start[i]-target_end[i] == 0) {
      ERROR(TRANS_LIB_NAME, "Output bin %d for field %s dimension %s has zero width (%f) - must provide valid averaging interval",
	    i, outvar->name, outvar->dims[od]->name, target_start[i]);
      return(-1);
    }
  }

  // Now just call our core function

  //  status=bin_average(data, qc_data, qc_mask, index_start, index_end, weights, ni,
  //		     odata, qc_odata, target_start, target_end, nt,
  //		     input_missing_value, output_missing_value, 
  //		     &metrics);
  status = call_core_function(bin_average, 
			      .input_data=data,
			      .input_qc=qc_data,
			      .qc_mask=qc_mask,
			      .index_boundary_1=index_start,
			      .index_boundary_2=index_end,
			      .weights=weights,
			      .nindex=ni,
			      .output_data=odata,
			      .output_qc=qc_odata,
			      .target_boundary_1=target_start,
			      .target_boundary_2=target_end,
			      .ntarget=nt,
			      .input_missing_value=input_missing_value,
			      .output_missing_value=output_missing_value,
			      .metrics=&metrics,
			      .aux=limits);

  // Set the qc bits if we estimated the bin boundaries
  set_estimated_bin_qc(qc_odata, invar, d, outvar, od, nt); 

  if (index) free(index);
  if (target) free(target);
  if (weights) free(weights);
  if (index_start) free(index_start);
  if (index_end) free(index_end);
  if (target_start) free(target_start);
  if (target_end) free(target_end);

  // Pass our metrics back to the driver.
  for (m=0; m < NUM_METRICS ; m++) {
    memcpy(met1d->metrics[m], metrics[m], nt*sizeof(double));
  }

  // And now free them up
  for (i=0;i<NUM_METRICS;i++) {
    free(metrics[i]);
  }
  free(metrics);

  return(status);
}


//int bin_average(double *array,
//                int *qc_array,
//                unsigned int qc_mask, 
//		double *index_start,
//		double *index_end,
//		double *weights,
//		int ni,
//		double *output,
//		int *qc_output,
//		double *target_start,
//		double *target_end,
//		int nt,
//		double input_missing_value,
//		double output_missing_value,
//		double ***rmet) {;}

int bin_average(core_s cs) {

  int i,i0,j,qco,m;
  double u, v, w, bin;
  double sum_array, sum_weight, max_weight;
  double sum_array2,sum_weight2,total_span,good_span;
  double *stdev, *coverage;
  double **metrics;

  int sign;

  // Convert core_s elements to interior elements.  AGain, if I'd written
  // this with core_s in mind I wouldn't need to do this.
  double *array = cs.input_data;
  int *qc_array = cs.input_qc;
  unsigned int qc_mask = cs.qc_mask;
  double *index_start = cs.index_boundary_1;
  double *index_end = cs.index_boundary_2;
  double *weights = cs.weights;
  int ni = cs.nindex;
  double *output = cs.output_data;
  int *qc_output = cs.output_qc;
  double *target_start = cs.target_boundary_1;
  double *target_end = cs.target_boundary_2;
  int nt = cs.ntarget;
  double input_missing_value = cs.input_missing_value;
  double output_missing_value = cs.output_missing_value;
  double ***rmet = cs.metrics;

  double *limits = (double *) cs.aux;

  double std_bad_max = limits[0];
  double std_ind_max = limits[1];
  double goodfrac_bad_min = limits[2];
  double goodfrac_ind_min = limits[3];

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
    
  stdev=metrics[0];
  coverage=metrics[1];

  // Monotonically increasing or decreasing.  There are cleverer ways of
  // doing this, but this is at least clear

  // The nt==1 deals with the case where we have only one output.  Only one
  // input should probably have some kind of error somewhere.  Or not.
  // TRS 8/6/18 - I think allowing averaging of one input (at least, not
  // bombing out), is correct - how useful the output is is debateable, but
  // that's what metrics and QC are for.
  if ((ni == 1 || index_start[0] < index_start[1]) &&
      (nt == 1 || target_start[0] < target_start[1])) {
    sign = 1;
  } else if ((ni == 1 || index_start[0] > index_start[1]) &&
	     (nt == 1 || target_start[0] > target_start[1])) { 
    sign = -1;
  } else {
    ERROR(TRANS_LIB_NAME, "Target and index are not monotonically aligned");
    return(-5);
  }

  // Set our weights=1.0 if not given
  if (weights == NULL) {
    weights=CALLOC(ni,double);
    for(i=0;i<ni;i++) {
      weights[i]=1.0;
    }
  }

  // again, i indexes our input field, j indexes the target field
  i0=0;
  for (j=0; j<nt; j++) {
    // reinit
    sum_array=sum_weight=max_weight=0.0;
    sum_array2=sum_weight2=0.0;
    total_span=good_span=0.0;
    qc_output[j]=qco=0;
    i=i0;  // start with previous first input bin

    // run up i until we find an input bin that overlaps our target bin;
    // this means the upper index edge is at or above the lower target edge
    while (i < ni && sign*index_end[i] < sign*target_start[j]) { i++;}

    // Some idiot checking on i goes here

    // So now we know that input bin i has an end greater than the start of
    // output bin j.  Which is good - they (might) overlap.

    // Save this as i0 to start the next iteration from; this will allow
    // rolling averages and overlapping output bins, as long as the
    // target_start[j] are monotonically increasing (and, possibly, equal)
    i0=i;

    // Alright, so now we continue to run up i until the lower index edge
    // is at or above the upper target edge - this time, however, we also
    // start doing the average
    while (i < ni && sign*index_start[i] < sign*target_end[j]) {

      // Idiot check our input binning - we know our start is before the
      // end, now we need to know if our input end is after the target
      // start as well.  

      // Note that this is a rejection test, so the < means it passes if
      // the end >= target start, which is what we want.
      if (sign*index_end[i] < sign*target_start[j]) {
	// This is a weird situation, probably involving oddly shaped input
	// bins, so let's leave a log message just in case it's not expected.
	LOG(TRANS_LIB_NAME,
	    "Input bin %d [%f,%f] does not overlap output bin %d [%f,%f]; skipping...",
	    i, index_start[i], index_end[i], j, target_start[j], target_end[j]);
	i++;
	continue;
      }

      // need to check the qc of each bin i; if any of them fail, set
      // qcval=1 and don't use them in the average.  If we miss to many,
      // we'll hcange qcval=2 later on
      // ignore qc_array for now - we are revising the whole thing
	
      // Now we'll use the weights given on input, with u and v the
      // fraction of an input bin that's outside our output bin.
      //w=1.0;
      w=1.0;
 
      // This is tricky for monotonic decreasing, but I think the negatives
      // all cancel out correctly.  Even though negative bin size is weird.
      bin=index_end[i]-index_start[i];

      // Need to trap out zero bin widths - in that case, keep w = 1, and
      // our fractional bin coverages are zero as well.
      if (bin == 0.0) {  
	u=v=0;
      } else {
	if ((u=(target_start[j]-index_start[i])/bin) > 0) w-=u;
	if ((v=(index_end[i]-target_end[j])/bin) > 0) w-=v;
      }

      // These are weird circumstances if any of these happen, but I want to be
      // sure we don't use negative weights or whatever.

      if (u>1.0 || v>1.0 || u+v>1.0 || w < 0.0) {
	ERROR(TRANS_LIB_NAME,
	      "Problem with bin average: input bin %d [%f,%f], output bin %d [%f,%f]",
		 i, index_start[i], index_end[i], j, target_start[j], target_end[j]);
	return(-1);
      }

      // Before we trap out bad points, use the above w to calculate our
      // total input span, for determining coverage
      if (fabs(bin) > 0) { 
	//total_span += w*sign*(index_end[i]-index_start[i]);
	total_span += w*sign*bin;
      } else {
	total_span += 1;
      }

      // Set our maximum input weight down here, so we can make sure we are
      // using an overlapping bin (i.e. w>0).  If max_weight > 0 but 
      // the sum_weight is zero, we know it's because of bad data, rather
      // than zero weighting in this region.
      if (w>0 && weights[i] > max_weight) {max_weight=weights[i];}

      // Don't qc zero weighted points, because they don't matter.  They
      // are probably the result of a <= or >= and have zero bin overlap
      if (w>0 &&
	  (array[i] == input_missing_value || (qc_array[i] & qc_mask) || ! isfinite(array[i]))) {
	qc_set(qc_output[j], QC_SOME_BAD_INPUTS);
	i++;
	continue;
#ifdef notdef
      } else if (qc_array[i] != 0) {
	if (fabs(bin) > 0) { 
	  //yellow_span += w*sign*(index_end[i]-index_start[i]);
	  yellow_span += w*sign*bin;
	} else {
	  yellow_span += 1;
	}
#endif
      } else {
	// We are going to use this point, so add it to our good_span.
	if (fabs(bin) > 0) {
	  good_span += w*sign*bin;
	} else {
	  good_span += 1;
	}
      }


#ifdef notdef	
      // Don't think this is right - we want to figure out the fraction of
      // input bin that is within the output bin boundaries - it's hard to
      // do this generically, because we might have larger input bins than
      // output bins, or two huge input bins that are nevertheless
      // straddled by the output bin.  Weird.

      // DEFINITELY NOT RIGHT!
      //if ((u=(target_end[j]-index_end[i])/bin) < 1) w=u;
      // if ((v=(index_start[i]-target_start[j])/bin) < 1) w=v;
#endif
	
      // now mult by the actual weight of this bin
      w *= weights[i];
	
      sum_array += w*array[i];
      sum_weight += w;

      sum_array2 += w*array[i]*array[i];
      sum_weight2 += w*w;

      // Down here is where we might calculate the reduction metrics
      // (min,max).  Look up how to do a median without a sort, or insert
      // item into a sort to get median.  Maybe just put values into an
      // array, call qsort, and use that to get min,max,median.  qsort is
      // O(nlogn), right, which is bsaically nothing until n is millions.
      // Or, jeez, I could implement my own qsort right here - we are
      // already O(n) scanning over array values, just toss them into qsort
      // partitions and finish it up.

      // If we are actually weighting by this value, then we need to pass
      // through the qc as well.
      if (w > 0) {
	qco |= qc_array[i];
      }
	
      // Finally, don't forget to increment
      i++;

    }


    // So our average value is what it is
    if (max_weight==0 && i > i0) {
      // This means that (a) we had one or more overlapping points and (b)
      // all the weights were zero.  In this case we prescribe an output
      // value of zero.
      output[j]=0;
      stdev[j]=0;
      coverage[j]=0;
      qc_set(qc_output[j], QC_ZERO_WEIGHT);
    } else if (i == i0) {
      // This means that none of our inputs overlapped our output bin;
      // either the dimensions are wonky or the data is missing (NOT bad,
      // but actually missing).  Most likely this will happen when we have
      // a half-day of data in the file or something.  Anyway, set the
      // output to missing and set QC_OUTSIDE_RANGE
      output[j]=output_missing_value;
      stdev[j]=output_missing_value;
      coverage[j]=0;
      qc_set(qc_output[j], QC_OUTSIDE_RANGE);
      qc_set(qc_output[j], QC_BAD);
    } else if (sum_weight == 0) {
      // Now we know we had a positive weight, so this means all the data
      // failed QC, and should be crunched.
      //fprintf(stderr, "bin_average: target bin [%f,%f] has no good values to average\n",
      //target_start[j],target_end[j]);
      output[j]=output_missing_value;
      stdev[j]=output_missing_value;
      coverage[j]=0;
      qc_set(qc_output[j], QC_ALL_BAD_INPUTS);
      qc_set(qc_output[j], QC_BAD);
    } else {

      output[j]=sum_array/sum_weight;

      // Calculate our metrics

      // Check: 
      // s0 = sum(w)
      // s1 = sum(wx)
      // s2 = sum(wx^2)
      // sample stdev = x = sqrt((s0*s2 - s1*s1)/(s0*(s0-1)))

      stdev[j] = sum_weight*sum_array2 - sum_array*sum_array;

      // stdev[j] /= (sum_weight*sum_weight) - sum_weight;
      // This is a problem - the sample stdev divides by s0(s0-1).  But, if
      // s0 = sum_weight < 1, that goes negative, and we can get a nan when
      // we take the sqrt.  I'm seeing this with only two points, so it
      // might be only a problem when you have too few points to take a
      // proper sample stdev - with more points, perhaps s1*s1 will always
      // be > s0*s2 when s0 < 1.  I dunno, I'll have to read up on the
      // whole sample stdev issue.  But for now, I'm going to go to a
      // regular stdev instead.  I always have trouble getting my head
      // around this, but the sample stdev may not be what we want, anyway.

      stdev[j] /= sum_weight*sum_weight;  // s0*s0

      if (fabs(stdev[j]) < 1e-12) {
	stdev[j] = 0.0;  // roundoff error, so just set to zero
      } else if (stdev[j] < 0) {
	// This should no longer happen, unless the roundoff has gotten
	// really huge or something.
	LOG(TRANS_LIB_NAME,
	    "Standard deviation cannot be calculated: s0s2-s1s1 = %g (%e) \n",
	    stdev[j],stdev[j]);
	stdev[j] = output_missing_value;
      } else {
	stdev[j] = sqrt(stdev[j]);
      }
      coverage[j] = good_span/total_span;

      // If any of the input points were yellow (i.e. if they intersect
      // with the non-qc_masked bits), set output to indeterminate
      // We  used to just pass through the bits like this:
      // qc_output[j] |= qco
      if ((qco & ~qc_mask) != 0) {
	qc_set(qc_output[j], QC_INDETERMINATE);
      }
    }

    // After all this, set the the metric-based QC limits, assuming we have
    // a real data point - check for missing, first
    if (stdev[j] != output_missing_value) {
      if (stdev[j] > std_bad_max) {
	qc_set(qc_output[j], QC_BAD_STD);
      } else if (stdev[j] > std_ind_max) {
	qc_set(qc_output[j], QC_INDETERMINATE_STD);
      }
    }

    if (coverage[j] != output_missing_value) {
      if (coverage[j] < goodfrac_bad_min) {
	qc_set(qc_output[j], QC_BAD_GOODFRAC);
      } else if (coverage[j] < goodfrac_ind_min) {
	qc_set(qc_output[j], QC_INDETERMINATE_GOODFRAC);
      }
    }
  } // output samples: j

  return(0);
}

