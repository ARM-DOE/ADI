# include <stdio.h>
# include <math.h>
# include <string.h>
# include <time.h>
# include <getopt.h>
# include <regex.h>
# include "cds3.h"
# include "trans.h"

// # include "netcdf.h"
#define NC_MAX_NAME 256

static size_t _one=1;

// This is infrastructure to tell us not to try and figure out default bin
// sizes but instead to crash.  That's the right behavior for output bins
// always anyway, so turning this off means we'll crash if we don't have
// input bins, either.  This lets me use it in here, when we call
// get_bin_edges(), rather than in each of the transformation files.
static int _use_default_edges=1;
void trans_turn_off_default_edges() {
  _use_default_edges=0;
  DEBUG_LV2("libtrans", "Turning default bin width calculations off - must be provided or process will exit");
}

# define CALLOC(n,t)  (t*)calloc(n, sizeof(t))

// Very ARM-ish.  Ah, well.
unsigned int get_qc_mask(CDSVar *var) {
  unsigned int b,mask=0;
  size_t length;
  char key[100], gkey[100], *qcstr;
  CDSAtt *att;
  CDSVar *get_qc_var(CDSVar *), *qcvar;

  // Assuming 32 bit integers - remember, we are 1-offset in our description
  for (b=1;b<=32;b++) {

    // I don't know if I still need this, but at some point we had somewhat
    // different assessment attribute names for field and global
    // attributes.  Crazy.
    sprintf(key, "bit_%d_assessment",b);
    sprintf(gkey, "qc_bit_%d_assessment",b);

    // Find an attribute attached to this var with this name - first in the
    // associated qc var and then in the parent group (i.e. field level
    // then global metadata).
    qcvar = get_qc_var(var);
    if ((qcvar == NULL || (att = cds_get_att(qcvar, key)) == NULL) &&
	(var->parent == NULL || (att = cds_get_att(var->parent, gkey)) == NULL)) {
	// No such attribute, so just keep looping
      continue;      
    }

    // Okay, we got an att, so grab the value
    qcstr = cds_get_att_text(att, &length, NULL);

    // Otherwise, we actually have an assessment here, so set the bit in
    // our mask if it is bad, or if level is set 
    if (strcmp(qcstr,"Bad") == 0) {
      //mask |= (1<<(b-1));
      qc_set(mask, b);
    }
    free(qcstr);
  }

  // Finally, if we have zero mask, then assume that we will do a mapping
  // to QC_BAD and QC_INDETERMINATE, and thus assume that our mask should
  // be QC_BAD.  This means that unmapped qc where 1 (or any odd number) is
  // good or indeterminate data will be improperly masked out.
  if (mask == 0) {
    // LOG(TRANS_LIB_NAME, "Using default qc mask (bit %d) for %s", QC_BAD, var->name);
    qc_set(mask,QC_BAD);
  }

  return(mask);
}

CDSVar *get_qc_var(CDSVar *var)
{
  char qc_var_name[NC_MAX_NAME];
  if (!var) return((CDSVar *)NULL);

  // If already a qc var, return yourself
  if (strncmp(var->name, "qc_", 3) == 0) return (var);

  snprintf(qc_var_name, NC_MAX_NAME, "qc_%s", var->name);
  return(cds_get_var((CDSGroup *)var->parent, qc_var_name));
}
 

/*
 * Find a transform parameter of form "dim_name:param_name" for a given CDS
 * object; if that doesn't exist, it will look for just "param_name" in the
 * object, and then, finally, look for "param_name" in the dimension
 * object.  This will allow you to specify different param values for each
 * combination of dim and var, but will also allow you to assign a single
 * param value for all dimensions in a var, or for all vars that use that
 * dimension. 
 *
 *  @param  object     - pointer to the CDS Object to get the parameter for
 *             (usually a CDSVar, because it connects to a dimension, too)
 *  @param dim - pointer to CDSDim (not actually required to be connected to
 *             the above object, but probably a mistake if you do)
 *  @param  param_name - name of the parameter
 *  @param  type       - data type of the output array
 *  @param  length     - pointer to the length of the output array
 *                         - input:  maximum length of the output array
 *                         - output: number of values written to the output array
 *  @param  value      - pointer to the output array
 *                       or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the output array
 *    - NULL if:
 *        - the parameter was not found or has zero length (length = 0)
 *        - a memory allocation error occurs (length = -1)
 */

void *cds_get_transform_param_by_dim
(void *object, 			     
 CDSDim      *dim,
 const char  *param_name,
 CDSDataType  type,
 size_t      *length,
 void        *value) {
  
  void *ret;
  char buf[1000];
  sprintf(buf,"%s:%s", dim->name, param_name);
  
  if ((ret = cds_get_transform_param(object, buf, type,length, value)) == NULL &&
      (ret = cds_get_transform_param(object, param_name, type,length, value)) == NULL) {
    ret = cds_get_transform_param(dim, param_name, type,length, value);
  }
  return(ret);
}

// Some tools for working with metric functions.
// This one allocates the space given a size and number of metrics, and
// also fills in the other elements, because why not.
int allocate_metric(TRANSmetric **met, const char *metnames[],
		    const char *metunits[], int nmet, int size) {
  int m;

  // If our met is not null, we have to free up the allocated memory
  if (*met) {
    free_metric(met);
  }
  
  // Have to actually allocate space for one struct
  *met=CALLOC(1, TRANSmetric);

  (*met)->nmetrics=nmet;

  // Should be extra careful, and allocate and copy, because we destroy
  // these metric elements willy nilly.  But, sooner or later, we should go
  // back to a static const array of character strings, and copying arrays
  // of strings is a mess.  Maybe I should have metnames be a const char?
  (*met)->metricnames=metnames; 
  (*met)->metricunits=metunits; 

  (*met)->metrics = CALLOC(nmet, double *);

  for (m=0;m<nmet;m++) {
    (*met)->metrics[m]=CALLOC(size, double);
  }

  // Someday, I'll add error checks and stuff.
  return(0);
}
  

int free_metric (TRANSmetric **met) {
  int m;

  if (met == NULL || *met == NULL) return(0);

  for (m=0;m < (*met)->nmetrics; m++) {
    if ((*met)->metrics[m]) {
      free((*met)->metrics[m]);
    }
  }

  // Don't forget to free the element pointer, as well as the final pointer.
  free((*met)->metrics);
  free(*met);
  *met=NULL;

  return(0);

}

// Function to return the sibling metric var for an input var.  We return
// directly out of cds_get_var, so return values are appropriate for that.
CDSVar *cds_get_metric_var(CDSVar *var, char *name) {
  char fname[300];

  sprintf(fname,"%s_%s",var->name, name);
  return(cds_get_var((CDSGroup*) var->parent, fname));
}


///////////////////////////////////////////////////////////////////////////
// Now figure out our edges, via a slurry of possible params.  Because of
// the different ways we have of specifying this information, we'll have
// to adopt a hierarchy.  So, we look, in this order:
// * Explicit declaration of front and back edges (== boundary_1 and boundary_2)
// * Width (single value or array) + alignment, working off index
// * Just alignment, assuming our bins cover completely our space,
//   without overlapping
// * With no information whatsoever, assume alignment=0.5 and do the
//   above
//
// Note: "front_edge" is now "boundary_1", but the code still calls
// associated variables "front" or tags it with an "f".  Same thing with
// "back" and "b" and "boundary_2".  I'm not searching and replacing to tag
// with 1s and 2s, because I'll just do it wrong and mess everything up.


int get_bin_edges(double **front_edge, double **back_edge, double *index,
		  int nbins, CDSVar *var, int d) {

  size_t front_len, back_len, width_len;
  int    i;
  void *retf, *retb;
  double alignment = 0.5, *width;

  // Let's dive in
  retf=cds_get_transform_param_by_dim(var, var->dims[d],
				      "boundary_1", CDS_DOUBLE,
				      &front_len, *front_edge);

  // grandfather
  if (!retf)
    retf=cds_get_transform_param_by_dim(var, var->dims[d],
					"front_edge", CDS_DOUBLE,
					&front_len, *front_edge);

  if (retf && front_len != (size_t) nbins) {
    ERROR(TRANS_LIB_NAME, 
	  "Front bin edge array for %s (%s) has incorrect number of values (%d, %d)",
	  var->name, var->dims[d]->name, front_len, nbins);
    retf=NULL;
  }

  retb=cds_get_transform_param_by_dim(var, var->dims[d],
				      "boundary_2", CDS_DOUBLE,
				      &back_len, *back_edge);

  // grandfather 
  if (!retb) 
    retb=cds_get_transform_param_by_dim(var, var->dims[d],
					"back_edge", CDS_DOUBLE,
					&back_len, *back_edge);

  if (retb && back_len != (size_t) nbins) {
    ERROR(TRANS_LIB_NAME, 
	  "Back bin edge array for %s (%s) has incorrect number of values (%d, %d)",
	  var->name, var->dims[d]->name, back_len, nbins);
    retb=NULL;
  }

  // Okay, now see if we have either or both of these things
  if (retb && retf) {
    // Hooray!
    *front_edge=retf;
    *back_edge=retb;
    return(0);
  }

  // We are missing one or the other or both, so now go down the list
  if (retf) free(retf);
  if (retb) free(retb);

  // No matter what, we'll need to build our edge arrays now
  // edges and fill them.  
  *front_edge=CALLOC(nbins, double);
  *back_edge=CALLOC(nbins, double);

  // We need a bin alignment for all of these choices, so set the default
  // value of 0.5 to indicate we assume we are in the middle of the bin
  if (cds_get_transform_param_by_dim(var, var->dims[d],
				     "alignment", CDS_DOUBLE,
				     &_one, &alignment) == NULL) {
    alignment = 0.5;
  }

  ///////////////////////////////////////////////
  // Okay, now look for a width 
  width=NULL;
  if ((width=cds_get_transform_param_by_dim(var, var->dims[d],
					   "width", CDS_DOUBLE,
					    &width_len, NULL))) {
    // Yay. Now idiot check the return size
    if (width_len != 1 && width_len != (size_t) nbins) {
      ERROR(TRANS_LIB_NAME, 
	    "Width array for %s (%s) has incorrect number of values (%d, %d)\nUsing first value only",
	    var->name, var->dims[d]->name, width_len, nbins);
      width_len=1;
    }

  } 

  // If we got a width, use it
  if (width) {
    // Simple matter to just build our arrays
    for (i=0;i<nbins;i++) {
      int w=(width_len == 1 ? 0 : i);
      (*front_edge)[i] = index[i] - alignment*width[w];
      (*back_edge)[i] = index[i] + (1.0-alignment)*width[w];
    }
    // Hooray!
    free(width);
    return(0);
  }

  // Right here we stop and return -1 if we have turned off default
  // calculation of bin widths
  if (_use_default_edges==0) {
    return(-1);
  }
  
  // For a time dimension without any bin widths, return a zero width
  // (instananeous) bin, with front and back edges equal to the index
  if (strcmp(var->dims[d]->name,"time") == 0) {
    for (i=0;i<nbins;i++) {
      (*front_edge)[i]=(*back_edge)[i]=index[i];
    }
    return(0);
  }

  // For non-time dimension, we infer our bin edges - 
  // Have to build our edges directly.  We can't just use a width,
  // because in an irregular grid (especially if time is our index and
  // data is missing), we end up messing up the boundaries.

  // Hard calc of the front edge of the first bin.
  (*front_edge)[0] = index[0] - alignment*(index[1]-index[0]);
  
  for (i=0;i<nbins;i++) {
    // Only need to calc the back edges of each bin, as the front edge of
    // the next will match it.  So wi is the difference between this and
    // the next index value, except for the last one, where we reuse the
    // previous width
    double wi = i<nbins-1 ? index[i+1]-index[i] : index[i]-index[i-1];
    
    if (i>0) (*front_edge)[i] = (*back_edge)[i-1];
    (*back_edge)[i] = index[i] + (1.0-alignment)*wi;
  }

  // Set a user tag so we know we had to fake the bin information
  char key[30];
  sprintf(key,"estimated_boundaries_%d",d);
  cds_set_user_data(var, key,"true",NULL);

  // Return a different status to indicate we had to guess our bin edges
  return(1);
}

// Return midpoint of array - uses get_bin_edges()
double *get_bin_midpoints(double *index, int nbins, CDSVar *var, int d) {
  int status, i;
  double *index_start=NULL, *index_end=NULL;
  double *index_mid;

  status=get_bin_edges(&index_start, &index_end, index, nbins, var, d);

  // Should check status, but right now it only returns 0, so bare bones it
  if (status < 0) {
    ERROR(TRANS_LIB_NAME,
	  "Bin widths for variable %s required but not provided.  Exiting...",
	  var->name);
    return(NULL);
  }

  index_mid=CALLOC(nbins, double);

  for (i=0;i<nbins;i++) {
    index_mid[i] = (index_start[i]+index_end[i])/2.0;
  }

  if (index_start) free(index_start);
  if (index_end) free(index_end);

  return(index_mid);
}


// This function will scan the user data of invar and outvar, and set the
// qc_odata bits if we had to estimate the bin boundaries from the data
// itself, rather than read it from metadata or trans params.
int set_estimated_bin_qc(int *qc_odata, CDSVar *invar, int d,
			 CDSVar *outvar, int od, int nt) {
  char *val;
  char key[30];
  int qc_bin=0;
  int i;

  sprintf(key,"estimated_boundaries_%d",d);

  if ((val=cds_get_user_data(invar, key)) &&
      (strcmp(val,"true")==0)) {
    qc_set(qc_bin,QC_ESTIMATED_INPUT_BIN);
  }

  sprintf(key,"estimated_boundaries_%d",od);

  if ((val=cds_get_user_data(outvar, key)) &&
      (strcmp(val,"true")==0)) {
    qc_set(qc_bin,QC_ESTIMATED_OUTPUT_BIN);
  }

  if (qc_bin) {
    for (i=0;i<nt;i++) {
      qc_odata[i] |= qc_bin;
    }
  }
  return(0);
}
