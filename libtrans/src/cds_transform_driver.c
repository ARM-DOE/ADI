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
*    $Revision: 80506 $
*    $Author: shippert $
*    $Date: 2017-09-12 20:37:40 +0000 (Tue, 12 Sep 2017) $
*    $State: $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file transform.c
 *  transform one cds group into another, by somehow pulling out the
 *  transformation details from the aether.
 */
# include <stdio.h>
# include <math.h>
# include <string.h>
# include <time.h>
# include <getopt.h>
# include <regex.h>
# include <ctype.h>
# include "trans.h"
# include "trans_private.h"
# include "timing.h" 

int QC_BAD=1;
int QC_INDETERMINATE=2;
int QC_INTERPOLATE=3;
int QC_EXTRAPOLATE=4;
int QC_NOT_USING_CLOSEST=5;
int QC_SOME_BAD_INPUTS=6;
int QC_ZERO_WEIGHT=7;
int QC_OUTSIDE_RANGE=8;
int QC_ALL_BAD_INPUTS=9;
int QC_BAD_STD=10;
int QC_INDETERMINATE_STD=11;
int QC_BAD_GOODFRAC=12;
int QC_INDETERMINATE_GOODFRAC=13;

// Deprecated
int QC_ESTIMATED_INPUT_BIN=0;
int QC_ESTIMATED_OUTPUT_BIN=0;


static size_t _one=1;

/* We'll be doing a lot of dynamic allocation, so let's make it easier */
# define CALLOC(n,t)  (t*)calloc(n, sizeof(t))
# define REALLOC(p,n,t)  (t*) realloc((char *) p, (n)*sizeof(t))


// This depends on the interface_s literal struct, defined in trans.h so
// the interface functions can understand it.  This lets me hide the
// horrendous syntax, and al
// I'm putting some defaults in here, just for fun.
#define do_transform(trans,...) ((*(trans->func))((interface_s){ \
	.input_missing_value=-9999, .output_missing_value=-9999, __VA_ARGS__}))
  
// This is how we map the tranform parameters that hold the name of the
// transform to actual functions.  Uses the TRANSf1unc structure defined in
// trans.h 

// Now these are the default transforms - we'll always search these to
// match the text given in the params.
// TRANS_AUTO is handled separately.
static TRANSfunc DefaultTransFuncs[] = {
  {"TRANS_INTERPOLATE", trans_interpolate_interface}, 
  {"TRANS_SUBSAMPLE", trans_subsample_interface}, 
  {"TRANS_BIN_AVERAGE", trans_bin_average_interface},
  {"TRANS_PASSTHROUGH", trans_passthrough_interface},
  {"TRANS_CARACENA", trans_caracena_interface},
};

// Apparently, this works
static int NumDefaultTransFuncs=sizeof(DefaultTransFuncs)/sizeof(TRANSfunc);

// This holds the user defined ones, which we'll set by dedicated functions
// that the user will have to call, probably right up near the main
// function. 
static TRANSfunc *UserTransFuncs;
static int NumUserTransFuncs=0;

// Allows for a user defined qc mapping from non-standard aqc
static int (*qc_mapping_function)() = NULL;
static int *qc_bad_values=NULL;
static size_t num_qc_bad_values=0;

// Structure to hold the dimension groups
struct dim_group {
  int ninput;                  // number input dims in group
  //char *input_dimnames;        // comma delimited list of names
  int noutput;                 // number output dims in group
  //char *output_dimnames;       // comma delimited list of names
  int input_d;                 // index in input space
  int output_d;                // index in output space
  int ilen;                    // size of input group: na*nb*nc...
  int olen;                    // size of output group: na*nb*nc...
  char **input_d_names;        // List of input dimension names
  int *input_d_length;         // Size of each input subdim in a group
  char **output_d_names;        // List of output dimension names
  int *output_d_length;        // Size of each output subdim in a group
  int order;                   // order in which we should transform (as
			       // listed by param)
};

struct dim_group *parse_dim_grouping(char *dim_grouping,
				     CDSVar *invar,
				     CDSVar *outvar,
				     int *Ndimgroups);

void free_dim_group(struct dim_group *g);
  
// So, here is the dedicated function to allocate space for and assign the
// user functions.  Incidentally, I have no idea how to prototype this...
//int assign_transform_function(char *name,
//			      int (*fptr)(double *, int*, double*,
//					  int*, CDSVar*, CDSVar*, int, TRANSmetric **)) {

int assign_transform_function(char *name,int (*fptr)(interface_s)) {

  int i;
  static int nalloc=0; // Keep track of how much we've allocated, to see if
		       // we need to realloc

  // Allocation shabadoo; do it in blocks of 10, because that's how I roll
  if (NumUserTransFuncs == 0) {
    nalloc=10;
    if (! (UserTransFuncs = CALLOC(nalloc, TRANSfunc))) {
      ERROR(TRANS_LIB_NAME,
	    "Initial alloc of UserTransFuncs failed (%d)\n", nalloc);
      return(-1);
    } 
  } else {
    // Check to see if this tag is already used, and replace if so
    for (i=0;i<NumUserTransFuncs;i++) {
      if (strcmp(name, UserTransFuncs[i].name) == 0) {
	LOG(TRANS_LIB_NAME,
	    "Warning: replacing user-defined function %s\n",
	    name);
	UserTransFuncs[i].func=fptr;
	return(0);
      }
    }
  }
      
      
  // Now, we have to check to see if we have enough space allocated.  I
  // expect never to have to run this code, so it may not actually work,
  // because it's probably never been tested.
  if (NumUserTransFuncs >= nalloc) {
    nalloc +=10;
    if (! REALLOC(UserTransFuncs, nalloc, TRANSfunc)) {
      ERROR(TRANS_LIB_NAME,
	    "Realloc of UserTransFuncs failed (%d)", nalloc);
      return(-1);
    }
  }

  // Finally, just assign stuff
  UserTransFuncs[NumUserTransFuncs].name = name;
  UserTransFuncs[NumUserTransFuncs].func = fptr;
  NumUserTransFuncs++;

  return(0);
}

// Assignment function for qc mapping, which either the user or we can use,
// depending on the flat file.
void assign_qc_mapping_function(int (*fptr)(CDSVar *, double , int)) {
  qc_mapping_function=(void *) fptr;
}

// Default qc mapping func, for use when we list bad values in the qc_bad
// transform param
int default_qc_mapping_function(__attribute__ ((unused)) CDSVar *qc_var,
				__attribute__ ((unused)) double val,
				int qc_val) {
  unsigned int k, qc=0;

  // I'm not sure I like the use of globals here.  Oh well.

  // Scan up our list of bad values, set via transform params before this
  // call.  If any match, set the QC_BAD bit and return.
  for (k=0;k<num_qc_bad_values; k++) {
    if (qc_val == qc_bad_values[k]) {
      qc_set(qc, QC_BAD);
      return(qc);
    }
  }

  if (qc_val != 0) {
    qc_set(qc, QC_INDETERMINATE);
  }
  return(qc);
}
  

// Now, here's the function to find and return the transform function.  I'm
// returning the whole TRANSfunc structure, so I don't have to prototype
// things the hard way.  Also, it will help with debugging to keep the name
// tagged along.
TRANSfunc *get_transform(char *name) {
  int i;

  // Look through the user defined ones first, so that they can overide the
  // defaults 
  for (i=0;i<NumUserTransFuncs;i++) {
    // Should I use strcmp, or some kind of regex? Nah, we'll be hardcore
    // and force complete compliance
    if (strcmp(name, UserTransFuncs[i].name) == 0) {
      return(&UserTransFuncs[i]);
    }
  }

  for (i=0;i<NumDefaultTransFuncs;i++) {
    if (strcmp(name, DefaultTransFuncs[i].name) == 0) {
      return(&DefaultTransFuncs[i]);
    }
  }
  // Okay, we've whiffed.  So, let them know.

  ERROR(TRANS_LIB_NAME,
	"Transform function %s not known; check spelling and documentation\n", name);
  return(NULL);
}


/**
*  Run the transform engine on an input variable, given input QC and an
*  allocated and dimensioned output variable (and QC) structure.
*
*  @param  invar - pointer to input CDSVar
*  @param  qc_invar - pointer to input QC CDSVar
*  @param  outvar - pointer to output CDSVar; must have dimensions and data
*            spaces allocated, and the dimensions must have coordinate
*            variables already created and attached (we use this
*            information to build the output grid to transform to)
*  @param  qc_outvar - pointer to output QC CDSVar; must be dimensioned and
*            allocated as above for outvar
*
*  @return
*    - 0 if successful
*    - -1 if something failed, usually deeper in CDS
*
*  Upon successful output, outvar and qc_outvar will contain the
*  transformed data and QC. 
*/

int cds_transform_driver(CDSVar *invar, CDSVar *qc_invar, CDSVar *outvar, CDSVar *qc_outvar) {
  int i, d, k, z, s, z0, m, Ndims, oNdims;
  TRANSfunc *trans=NULL;
  double *data, *odata=NULL,*tdata;
  int *qc_data, *qc_odata=NULL, *qc_tdata, *qc_temp;
  int size;

  int Ntot, *D, *len;
  int tNtot, *tD, *tlen;
  int oNtot, *oD, *olen;
  double input_interval, output_interval;
  double input_missing_value, output_missing_value;

  // To help freeing temp data without freeing original data.
  int iter_count=0;

  char *transform_type;

  double trans_calculate_interval(CDSVar *, int);

  // Setup some regexps for later on - creating metric fields for station
  // view fields with @s in them.
  regex_t at_re;
  regmatch_t at_match[3];  // Make sure to allocate for the full match, too
  regcomp(&at_re, "([^@]+)@(.+)", REG_EXTENDED);

  call_getrusage("*** Start of transform driver");

  // First, let's make sure that we want a serial 1D transform; if not,
  // well, we'll have to deal with that later.
  if ((transform_type = cds_get_transform_param(outvar, "transform_type",
						      CDS_CHAR, &_one, NULL)) != NULL &&
      (strcmp(transform_type, "Multi_Dimensional") == 0)) {
    LOG(TRANS_LIB_NAME, "Multi D transforms not implemented yet\n");
    return(-3);
  }

  // Now, let's check for qc mapping in the flat files, and set the
  // qc_mapping function to the default integer map, if it's not already
  // set 
  qc_bad_values=NULL;  // reset from last time
  if (qc_mapping_function == NULL &&
      qc_invar != NULL &&
      (qc_bad_values=cds_get_transform_param(qc_invar,"qc_bad", CDS_INT, &num_qc_bad_values,NULL))) {
    LOG(TRANS_LIB_NAME, "Using specified qc value mapping\n");
    assign_qc_mapping_function(default_qc_mapping_function);

    // store the param - so we have to reflurp it into a string
    trans_store_param_text(qc_invar, "qc_bad", "NODIM", outvar->name);
  }

  // Okay, proceed with serial 1D transform.  This means looping over dims
  // and building strides and stuff.  This will be fun.
  Ndims=invar->ndims;
  oNdims=outvar->ndims;

  // First, we build our stride array based solely on the input data.
  // We'll move it into groups later.  That's why these are prefixed with
  // i. 
  int *iD=CALLOC(Ndims, int);
  int *ilen=CALLOC(Ndims, int);

  // I'm going to 
  // Initial stride and length; it's easier if we go backwards and use
  // D[i]=D[i+1]*len[i+1] 
  iD[Ndims-1]=1;  // Always true - unit stride on fastest dimension
  ilen[Ndims-1] = Ntot = invar->dims[Ndims-1]->length;

  for (d=Ndims-2;d>=0;d--) {
    ilen[d]=invar->dims[d]->length;
    Ntot *= ilen[d];
    iD[d]=iD[d+1]*ilen[d+1];
  }

  // Now, to allocate arrays, we need to consider grouping, so before we
  // can do anything we have to pull out and analyze our dimensional
  // groups.  Fortunately, our dimensional groups have to be one to one; if
  // we have extra dimensions either on input or output, they have to be
  // grouped together to match the other groups.
  char *dim_grouping=NULL;
  struct dim_group *dim_groups=NULL;
  int *group_order=NULL;

  // Default: groups equals dimensions
  int Ndimgroups=invar->ndims;

  // Return's NULL if dim_grouping isn't set
  dim_grouping = cds_get_transform_param(outvar, "dim_grouping",
					 CDS_CHAR, &_one, NULL);

  // Store the param, if we've set it
  if (dim_grouping) {
    trans_store_param("dim_grouping", dim_grouping, "NODIM", outvar->name);
  }

  // If dim_grouping is NULL, 
  dim_groups=parse_dim_grouping(dim_grouping, invar, outvar, &Ndimgroups);

  // This looks weird, but I need to keep the groups in dimension-similar
  // order, so that the recursive build on our stride array D works.  But I
  // want to be able to transform in arbitrary order, so I need to now scan our
  // groups and determine the order in which to transform.

  // I build group_order[n] such that g=group_order[n] is the group index
  // to use for transform number n.  We loop over [n], and transform [g]
  // accordingly.   
  group_order=CALLOC(Ndimgroups, int);
  for (int n=0;n<Ndimgroups;n++) {
    group_order[n]=-1;  // Idiot proofing in case I muck up the ordering
    for (int g=0;g<Ndimgroups;g++) {
      if (dim_groups[g].order == n) {
	group_order[n]=g;
	break;
      }
    }
  }

  // Now I have to build our stride and len arrays, based on our groups.
  D=CALLOC(Ndimgroups, int);
  len=CALLOC(Ndimgroups, int);
  // Allocate our working arrays of sizes and strides
  tD=CALLOC(Ndimgroups, int);
  oD=CALLOC(Ndimgroups, int);

  tlen=CALLOC(Ndimgroups, int);
  olen=CALLOC(Ndimgroups, int);

  //  if (! dim_groups) {
  //  // By default, dims equals groups, so just copy our input dims into our
  //  // working group arrays
  //  memmove(len, ilen, Ndimgroups*sizeof(int));
  //  memmove(D, iD, Ndimgroups*sizeof(int));
  //} else {

  // Okay, so now we have to build our group based strides.  As we've
  // kept groups in dim-similar order, we can build it with the same
  // recursive relation we always use.
  D[Ndimgroups-1] = 1;
  len[Ndimgroups-1]=dim_groups[Ndimgroups-1].ilen;
  for (int g=Ndimgroups-2;g>=0;g--) {
    len[g]=dim_groups[g].ilen;
    D[g]=D[g+1]*len[g+1];
  }

  // These are transitional - the lengths will change with each iteration
  // over g, as we transform g->g'.  So, as we loop over g, the lenghts are
  // mixtures of dims we have transformed and those we havent.  Thus, to
  // start with, fill in the lengths of the input groups.
  memmove(tlen, len, Ndimgroups*sizeof(int));
  memmove(olen, len, Ndimgroups*sizeof(int));
	      
  // For comments below this, I might talk about "number of dimensions" and
  // stuff like that - I will usually mean "number of groups", especially
  // for older comments.

  // Now, pull out our row-major ordered array, and allocate our output
  // array. 

  // There's some fancy stuff with missing values that I'll have to figure
  // out later, having to do with all those NULLS.
  size=Ntot;

  //data=cds_copy_array(invar->type, size, invar->data.vp,
  //CDS_DOUBLE, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
  size_t nsamples;
  //data=dsproc_get_var_data(invar, CDS_DOUBLE, 0, &nsamples,
  //&input_missing_value, NULL);
  data=cds_get_var_data(invar, CDS_DOUBLE, 0, &nsamples,
			   &input_missing_value, NULL);

  // Plus, QC data.  Fazoom
  qc_data=qc_temp=NULL;  // Just in case

  // Figure out output missing value - if it's not in outvar, use the input
  // value we just got.
  // This is more complicated than it oughta be, which probably means I'm
  // doing it wrong.
  void *missing_array;
  double *missing_double;

  //dsproc_get_var_missing_values(outvar, &missing_array);
  cds_get_var_missing_values(outvar, &missing_array);

  if (missing_array) {
    missing_double =
      (double *) cds_get_missing_values_map(outvar->type, 1,
					    missing_array, CDS_DOUBLE, NULL);
    output_missing_value=missing_double[0];
    free(missing_array);
    free(missing_double);
  } else {
    LOG(TRANS_LIB_NAME,
	"No missing value for transformed field %s; using input field value=%g\n",
	outvar->name, input_missing_value);
    output_missing_value=input_missing_value;
  }

  // Set up some infrastructure to allow QC a mapping function, to map
  // straight integer states and/or actual data values to bit-packed QC
  // states.  Also, if qc_invar is NULL (no input qc) then don't bother.
  if (qc_invar) {
    // Okay, right here we have to do something different if we are missing
    // one or more dimensions in our qc array.
    // BLORP
    if (qc_invar->ndims < invar->ndims) {
      // First, figure out which Dims are missing, and build a Dq[p]
      // stride coefficient for the qc var.
      int *mu=CALLOC(Ndims, int);  // 1 if in qc, 0 if not
      int *Dq=CALLOC(Ndims, int);  // stride coeffs for qc var
      int prev_d=INT_MAX;

      for(d=Ndims-1; d>=0;d--) {
	// loop over [dq] in {Nq} to see if [d] is in {Nq}
	// (That's my math-y way of saying if qc_data has [d] as a dim
	for (int dq=0;dq<qc_invar->ndims;dq++) {
	  mu[d]=0;
	  if (strcmp(qc_invar->dims[dq]->name,
		     invar->dims[d]->name) == 0) {
	    mu[d]=1;
	    break;
	  }
	}

	// Now calculate Dq, if [d] is in {Nq}
	if (mu[d]==1) {
	  if (prev_d > Ndims-1) {
	    Dq[d]=1;
	  } else {
	    Dq[d]=len[prev_d]*Dq[prev_d];
	  }
	  prev_d=d;
	}
      }

      // Hooray!  Now we just need to align the flattened k in invar space
      // with the flattened kp in qc_invar space, which will naturally
      // duplicate QC values over missing dimensions.
      // Note - we've already calculated our stride coefficients D[d], above.

      qc_temp=CALLOC(Ntot, int);
      //int *qc_foo=dsproc_get_var_data(qc_invar, CDS_INT, 0, &nsamples,
      //NULL, NULL);
      int *qc_foo=cds_get_var_data(qc_invar, CDS_INT, 0, &nsamples, NULL, NULL);

      for(k=0;k<Ntot;k++) {
	int kq=mu[0]*(k/D[0])*Dq[0]; // d=0, special form (k%D[-1]=k, by
				     // def and inspection)

	for(d=1;d<Ndims;d++) {
	  kq += mu[d]*((k % D[d-1])/D[d])*Dq[d];
	}
	qc_temp[k]=qc_foo[kq];
      }

      // Finally, free stuff up
      free(qc_foo);free(mu);free(Dq);
    } else if (qc_invar->ndims == invar->ndims) {
      //qc_temp=dsproc_get_var_data(qc_invar, CDS_INT, 0, &nsamples, NULL, NULL);
      qc_temp=cds_get_var_data(qc_invar, CDS_INT, 0, &nsamples, NULL, NULL);

      //qc_temp=cds_copy_array(qc_invar->type, size, qc_invar->data.vp,
      //CDS_INT, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    } else {
      // I don't know what to do if we have *more* QC dimensions than data
      // dimensions.  It's probably an error.  I should probably also free
      // up some stuff. Whatever
      ERROR(TRANS_LIB_NAME,
	    "Qc field %s has %d dimensions, while data field %s has %d dimensions\n", 
	    qc_invar->name, qc_invar->ndims, invar->name, invar->ndims);
      ERROR(TRANS_LIB_NAME,
	    "This literally makes no sense, so I'm exiting...\n");
      return(-1);
    }

    // If we have a mapping function, we have to apply it here
    if (qc_mapping_function) {
      qc_data=CALLOC(size, int);
      // Passing both data and qc values in, if they exist, just in
      // case there is some combo of both of these things that we need
      // to map qc to.
      for (k=0;k<size;k++) {
	qc_data[k]=(*qc_mapping_function)(qc_invar, data[k],qc_temp[k]);
      }
      free(qc_temp);
      qc_temp=NULL;
    } else {
      qc_data=qc_temp;
    }
  }

  // Okay, we have our data, so now we start looping over dims to do our
  // Serial 1D.  In RIPBE we start with the slowest (lowest) index
  // first, which may not be completely logical, but I'm going for
  // consistency now.  We can probably implement a transform param that
  // changes this without too much trouble.

  // The prefixes get a little confusing: no prefix means original input
  // data; 't' means temp data, the input for this dimension's transform,
  // and 'o' means output data, the output of this dimension's transform.
  // When we iterate over d, we set 't' variables to 'o' variables - the
  // output of each iteration is the input for the next one.  That's what
  // serial 1D means.

  // Perhaps I don't need tdata, and we can just reset data, D, len, and
  // Ntot each iteration, but I don't want to overwrite anything orignal
  // until the code is fully debugged, and after that I won't want to mess
  // with something that works.  So there - we got some memmoves and
  // pointer reassigns in our future.

  // Note that we rebuild oD and olen each iteration, so we copy values, we
  // don't pointer assign.
  tdata=data; 
  qc_tdata=qc_data;  // Right here - if qc_data is not of the same
		     // dimension as data, we need to remap it onto the
		     // same grid.
  //memmove(tD,D,Ndims*sizeof(int));
  //memmove(tlen,len,Ndims*sizeof(int));
  memmove(tD,D,Ndimgroups*sizeof(int));
  memmove(tlen,len,Ndimgroups*sizeof(int));
  tNtot=Ntot;

  // This will help us with metrics later on - it's the shape of the output
  // of this particular transformation.  So, we start with the shape of the
  // input (in dimension space), and change each dim as we transform it.

  // Okay - we need not the number of input dims, but the number of output
  // dims in our shape.  I believe shape
  //int *shape=CALLOC(Ndims, int);
  int *shape=CALLOC(oNdims, int);
  for (d=0;d<Ndims;d++) {
    shape[d]=invar->dims[d]->length;
  }

  // Okay, instead of the dim shape stuff above, I'm going to pre-analyze the
  // transformations, walking them back from the final output, and seeing
  // if the group shapes stay the same.  The tricky question, which I still
  // don't have an answer to, is whether group shape is a valid substitute
  // for dimensional shape - if we have the same number of groups, with the
  // same sizes, is that the same thing as same number of dimensions with
  // same sizes?  I'm worried about something like a 2D->2D asymmetrical
  // rotation - (x,y) -> (x',y'), where nx*ny = nx'*ny', but nx != nx' and
  // ny != ny'.  But!  I think that's okay.  If we take [t][x][y] ->
  // [t'][x][y], and nx'*ny' = nx*ny, then we can store our metrics on our
  // output grid without loss of information.  We may need to *reform* it
  // to find out exactly which [x][y] led to metric[t'][x][y], but we could
  // still, for example, use the metric[t'][x'][y'] form to count instances
  // and whatever.

  // Then again, if we have the same metric for a later transform:
  // [t][x][y][z] -> [t'][s'][z'], where we average t and z, then we really
  // want the last stdev (on z) rather than the first one (on t).  Gah!

  // Anyway, the key idea here is that we stop as soon as we find a
  // reverse-transformation that doesn't match output.  Then we know only
  // metrics after than transformation get saved.

  // Starting with groups, it's easy.  okshape[g]=1 means after this
  // transformation we can save metrics.  So, of course, initialze to 0.
  int *okshape=CALLOC(Ndimgroups,int);

  // Always output metrics for final transformation
  okshape[group_order[Ndimgroups-1]]=1;

  // We are assigning the okshape for n-1, so stop the loop at n==1
  for (int n=Ndimgroups-1;n>0;n--) {
    int g = group_order[n];
    int gp = group_order[n-1]; // previous transformation

    if (dim_groups[g].olen == dim_groups[g].ilen &&
	dim_groups[g].ninput == dim_groups[g].noutput) {
      // Okay, same group size, with same number of dims in each group.
      // So, now we gotta check them one by one
      int dd;
      for (dd=0;dd<dim_groups[g].ninput;dd++) {
	if (dim_groups[g].input_d_length[dd] != dim_groups[g].output_d_length[dd]) {
	  break;
	}
      }

      // If we got through all subdims, then this transform didn't break
      // shape
      if (dd == dim_groups[g].ninput) {
	okshape[gp]=1;
      } else {
	// Okay, we have a mismatch, so break out
	break;
      }
    } else {
      // This one did, so break out - no longer check any previous transforms
      break;
    }
  }

  // Inside this loop, the prefix "t" means the current *input* data, and
  // the prefix "o" means the current *output* data.  At the end of each
  // loop, we copy the o variables into the t variables, as the output of
  // each loop is the input to the next.  Then the o variables are reformed
  // into the new (group-based) output shape.

  // I use "t" here to mean "transitional", to allow "i" prefix to indicate
  // the *original* input stuff.  We need to keep original inputs around
  // for various checks and stuff.
  
  //   for (d=0; d<Ndims;d++) {
  for (int n=0; n<Ndimgroups;n++) {

    // This lets us transform groups in our custom order
    //g = dim_groups? group_order[n] : n;
    int g = group_order[n];

    // We still need to keep track of our dimensions, so keep a d and an od
    // By default, dims <=> groups, so only if we've remapped our groups do
    // we need to do something different.
    int d=dim_groups[g].input_d;
    int od=dim_groups[g].output_d;

    //if (dim_groups) {
    //d=
    //od=dim_groups[g].output_d;
    //}

    // First, find the size of our output data in this dimension.  This
    // keeps the other dims what they were, which is what we want - we
    // transform one d at a time.

    // For groups, we need to store this as the length of the dim group.  
    //olen[g] = dim_group ? dim_group[g].olen : outvar->dims[od]->length;
    olen[g] = dim_groups[g].olen;

    // This is the shape of the arrays coming out of this transformation
    // Shape is dimension based, so we have to loop over all the dims
    // represented by this group and reset the shape to match this
    // transformation.  I need shape for deciding whether to store the
    // metrics or not - we save metrics to output after the first
    // transformation in which they have the same shape as the output
    // data. 

    if (dim_groups) {
      for (int sd=od;sd < od+dim_groups[g].noutput; sd++) {
	shape[sd]=outvar->dims[sd]->length;
      }
    } else {
      shape[od] = outvar->dims[od]->length;
    }

    // Now, we have to build *output* stride arrays.  It's easier if I just
    // do it from scratch each time, just in case we change the order of
    // dims or something.

    // For groups, this is harder, as we don't have the same recursive
    // relation for group-ordered strides as we do for dim-ordered.  So it
    // seems like we have to rebuild the dimensional-ordered stride of our
    // transitional data first, then reorder into groups.  Arg.

    // Maybe I should keep [g] in dim-ordering, and use a lookup table to
    // decide which group to do next?

    oD[Ndimgroups-1]=1;
    oNtot=olen[Ndimgroups-1];
    for (i=Ndimgroups-2;i>=0;i--) {
      oD[i] = oD[i+1]*olen[i+1];
      oNtot *= olen[i];
    }

    // Now, allocate our output arrays for this iter.  The old odata
    // pointers are still in use as the new tdata, so we never end up
    // freeing these pointers; we'll have to do that when we reassign tdata
    // at the end.
    odata=CALLOC(oNtot, double);
    qc_odata=CALLOC(oNtot, int);

    // Okay: we have reformed our "o" variables to be in the shape of our
    // output after this group's transformation.

    // This next part is hinky - we pull out the dims associated with [d].
    // When we have dim_groups, that means we pull out the dim associated
    // with the *first* dimension in the group.  WE do this *solely*
    // because we need to find the transform parameters for this
    // dimension.  That means OUR TRANSFORM PARAMETERS FOR GROUPS *MUST* BE
    // ASSOCIATED WITH THE __FIRST__ DIMENSION OF THE GROUP!  That seems
    // like an undesigned requirement that falls out of the way the code is
    // written.  Sorry.  If this is not good, then I'll have to work out a
    // solution, but for now I'm keeping it.
			     
    // get the CDSdims associated with this dimension, because sometimes we
    // use index, and sometimes not.  This is used mainly to get tranform
    // params, but who knows.
    CDSDim *dim=invar->dims[d]; //apparently, we don't use this yet, but we
				//probably will as we add more transform params
    CDSDim *odim=outvar->dims[od];

    // Now get the transform function for this var for this dimension.  It
    // needs to be by outvar and dimension name, like:
    // temp:time:transform = "TRANS_INTERPOLATE";
    char *transform_name = cds_get_transform_param_by_dim(outvar, odim, "transform",
							  CDS_CHAR, &_one, NULL); 

    // TRANS_AUTO means automatically calculate which transform to use,
    // just like if there is no transform_name
    if (transform_name != NULL &&
	strcmp(transform_name,"TRANS_AUTO") != 0) {
      trans=get_transform(transform_name);
    } else {

      // Without a transform name, we have to figure out which one to use.
      // Our only default transformations are 1D and based on interval -
      // thus, we have to check to see if this "group" is 1D or not before
      // continuing. 

      if (dim_groups &&
	  (dim_groups[g].ninput != 1 ||
	   dim_groups[g].noutput != 1)) {
	    // We expect non-1D-to-1D transforms, which we cannot do by
	    // default, so bail:
	ERROR(TRANS_LIB_NAME,
	      "No transform given for field %s, dimensions %s -> %s; no defaults available for non-1D dimensional groups\n",
	      invar->name, dim_groups[g].input_d_names[0], dim_groups[g].output_d_names[0]);
	return(-1);
      }

      // Okay, so our dim groups are 1D, and the rest of this makes sense.

      // Okay, figure out the intervals to choose; this is stupid, but we
      // gotta do something like this.

      CDSVar *incoord = cds_get_coord_var(invar, d);
      CDSVar *outcoord = cds_get_coord_var(outvar, od);

      // Check to see if the coord fields exist, first
      if (incoord && outcoord) {

	if (! cds_get_transform_param(incoord, "interval", CDS_DOUBLE,
				      &_one, &input_interval)) {
	  // have to calculate it myself...
	  input_interval = trans_calculate_interval(invar, d);
	}

	if (! cds_get_transform_param(outcoord, "interval", CDS_DOUBLE,
				      &_one, &output_interval)) {
	  output_interval = trans_calculate_interval(outvar, od);
	}
	
	// So, our default is to average if output interval is larger, or
	// interpolate if input is larger
	
	if (output_interval > input_interval) {
	  trans=get_transform("TRANS_BIN_AVERAGE");
	  transform_name=strdup("TRANS_BIN_AVERAGE");
	  LOG(TRANS_LIB_NAME, "Using bin averaging for dim %s in field %s\n",
	      odim->name, outvar->name);
	} else {
	  trans=get_transform("TRANS_INTERPOLATE");
	  transform_name=strdup("TRANS_INTERPOLATE");
	  LOG(TRANS_LIB_NAME, "Using interpolation for dim %s in field %s\n",
	      odim->name, outvar->name);
	}
      } else if (dim->length == odim->length) {
	// At least one of the coord field does not exist, but input and
	// output dims are the same length, so set it as a passthrough
	trans=get_transform("TRANS_PASSTHROUGH");
	transform_name=strdup("TRANS_PASSTHROUGH");
	LOG(TRANS_LIB_NAME, "Using passthrough for dim %s in field %s\n",
	    odim->name, outvar->name);
      } else {
	// Just record the log, and let the next step dump
	LOG(TRANS_LIB_NAME,
	    "Coordinate field missing and dimensions do not match; no transformation possible\n");
      }
    }
      
    // Make sure we have an actual transform
    if (trans == NULL) {
      ERROR(TRANS_LIB_NAME,"No valid transform for dim %s in field %s; exiting transform code...\n",
	    dim->name, invar->name); 
      return(-1);
    }

    // Now store the transform name
    trans_store_param("transform", transform_name, odim->name, outvar->name);
    
    // Allocate space to hold our input and output slices.  We have to have
    // holding arrays, because our N-dimensional flattened array won't map
    // pointers or anything.

    // These will get overwritten with each iteration, which means the
    // following loop is not thread safe.  We'll have to allocate inside
    // the loop to make it that way.
    double *data1d=CALLOC(tlen[g], double);  
    int *qc1d=CALLOC(tlen[g], int);   // If no qc_invar, this will stay 0s,
				     // so it's cool.
    double *odata1d=CALLOC(olen[g], double);
    int *oqc1d=CALLOC(olen[g], int);  

    // Now, loop over all the slices, and pull out the input data and qc
    // for that slice.  This is fun.
    int nslice=tNtot/tlen[g];  
    int oz0=z0=0;  // First slice *always* starts at origin in our
		   // flattened array

    // Metric holders
    TRANSmetric *met1d=NULL, *metNd=NULL;

    for (s=0;s<nslice;s++) {

      // (re)initialize our output fields for this slice
      for (k=0;k<olen[g];k++) {
	oqc1d[k]=0;
	odata1d[k]=output_missing_value;
      }

      // Now build up our input slice, which is k strides up from z0
      for (k=0;k<tlen[g];k++) {
	z=z0+k*tD[g];  // striding
	data1d[k]=tdata[z];
	if (qc_tdata) qc1d[k]=qc_tdata[z];

      }
      // Great.  We now have our input and output arrays, so it's just a
      // matter of calling the transform interface function.  This takes a
      // mixture of 1D data arrays and CDS pointers; the latter are used to
      // track down the various parameters used in the transformation,
      // including such basic information as the size of the arrays and
      // what not.
      //int status =
      // (*(trans->func))(data1d,qc1d,odata1d,oqc1d,invar,outvar,d,
      // &met1d);
      DEBUG_LV4("libtrans",
		"Analyzing slice %d for %s, dim %d...", s, invar->name, d);
      //printf(	"Analyzing slice %d for %s, dim %d...", s, invar->name, d);
      int status = do_transform(trans,
				.input_data=data1d,
				.input_qc=qc1d,
				.input_missing_value=input_missing_value,
				.output_data=odata1d,
				.output_qc=oqc1d,
				.output_missing_value=output_missing_value,
				.invar=invar,
				.outvar=outvar,
				//.d=g,  ! Wrong, I think
				.d=d,  // input and output ds may be different
				.od=od,
				.met=&met1d); 

      // Bomb out if status is bad - I think we exit the whole problem if
      // this happens, so we don't have to worry about memory allocation or anything.
      if (status < 0) {
	ERROR(TRANS_LIB_NAME,
	      "Problem transforming variable %s, dimension %d, slice %d; exiting...", invar->name, d,s);
	return(status);
      }

      // We don't know how many metrics to fill until right here, so:
      // Make sure to trap out met1==NULL, because that means this
      // transform doesn't make metrics.
      if (s==0 && met1d) { // again, not thread safe 
	allocate_metric(&metNd, met1d->metricnames,met1d->metricunits,met1d->nmetrics, oNtot);
      }

      // Now that we've filled odata1d with our transformed data, fill in
      // our output slice, using the exact process above.  Make sure
      // we use output strides, z0s, and lens.
      for (k=0;k<olen[g];k++) {
	z=oz0+k*oD[g];
	odata[z]=odata1d[k];
	qc_odata[z]=oqc1d[k];

	// Fill our metrics array, if we've got one
	if (met1d && met1d->metrics) {
	  for (m=0;m<met1d->nmetrics;m++) {
	    metNd->metrics[m][z]=met1d->metrics[m][k];
	  }
	}
      }

      // We need to free up met1d and point it back to null, so it will get
      // properly allocated and not leave hangers.
      free_metric(&met1d);

      // Now, find our next slice, and reset our pointers.  The math behind
      // this is non-trivial, but sound.  Qualitatively, the idea is that
      // when we increment our faster indeces than d, we 
      // increment z0 by just 1 (since the stride of our fastest dimension
      // is always 1).  We can do that D[g] times, until we have to
      // increment the slower dimensions than d; to do that, we stride in
      // d-1, i.e. increment z0 by D[g-1].  We then subtract D[g]-1 to
      // reset our faster indeces to 0, and begin the cycle again.  Hence,
      // if z0+1 is a multiple of D[g], we set z0 += D[g-1] - (D[g] - 1);
      // otherwise, z0 += 1.

      // (Note the hidden +1 by doing ++z0 first).
      if ((++z0 % tD[g]) == 0 && g > 0) {
	z0 +=  tD[g-1] - tD[g];  
      }

      // I don't know if oz0 will always be a multiple of oD whenever z0 is
      // a multiple of D, so repeat the check just to be sure.
      if ((++oz0 % oD[g]) == 0 && g > 0) {
	oz0 +=  oD[g-1] - oD[g];  
      }

      // That's it - ready for the next stride.  Our s index just counts,
      // nothing else.
    }

    // Hokay.  At this point we've gone through all the slices in this
    // dimension, so it's time to see if we need to store our metrics in
    // sibling variables.  The decision tree is this: 
    // (a) does there exist a sibling variable to outvar with the proper
    // metric name?
    // (b) Does that sibling variable have the same shape as the metrics we
    // have right now in metNd.

    // If both of these hold true, save the metric in that sibling
    // variable.
    if (metNd) {
      // Obviously, we need some metrics or this doesn't make any sense

#ifdef notdef
      // Replaced with group-based calculations

      // Way up here, we first check to see if this iteration has the same
      // shape as the output var.  If so, we can continue on with the
      // transformation.  The shape of this iteration is in shape, which
      // has dimensionality of output for dm<=d and input for dm>d.

      // We have kept "shape" on the dimensional grid

      int okshape=1;
      int msize=1;
      int msample=1;
      int dm;
      for (dm=0;dm<outvar->ndims;dm++) {
	if (shape[dm] == outvar->dims[dm]->length) {
	  // I think we should know the total size of our metric (outvar) array
	  // already, but I can't figure out which variable holds it, so
	  // I'm recalculating it here.
	  msize *= outvar->dims[dm]->length;  

	  // Find the number of samples - i.e. size of time dimension
	  if (strcmp(outvar->dims[dm]->name, "time") == 0) {
	    msample=outvar->dims[dm]->length;
	  }

	} else {
	  okshape=0;
	}
      }
#endif

      if (okshape[g]) {
	// THis means we can store metrics - the shape of our metrics is
	// the same as the final shape of our output dimensions, so it
	// makes sense to store the value here.

	// Should merge with above the metNd check, actually, but I want to
	// leave in the commented out stuff above until I'm sure things
	// work right..

	// msample is the length of the first dimension
	int msample=outvar->dims[0]->length;

	// Now loop over our metrics.  For each one, we first look for an
	// existing CDSVar; if it exists, as a sibling to outvar, we'll try
	// to use it, but it means we have to double check the shape and
	// dimensions.  If it does not exist, we create one as a clone to
	// outvar, to get the same shape and type.
	for (m=0;m<metNd->nmetrics;m++) {

	  char sibname[300];

	  // Station view fix: we want to put the metric name *before* any
	  // @ in the field name - temp_std@sgpE13, rather than
	  // temp@sgpE13_std.  Maybe I'm in a rut, but the way I think to
	  // do stuff like this is via regexps
	  if (regexec(&at_re, outvar->name, 3, at_match,0) == 0) {

	    // Make a copy, which we can mess with
	    char *buf=strdup(outvar->name);
	    int ff=(int) at_match[1].rm_so;
	    int ss=(int) at_match[2].rm_so;

	    // Replace the @ with \0
	    buf[at_match[1].rm_eo]='\0';

	    // Put the station after the @ delimeter, and the metric before
	    sprintf(sibname, "%s_%s@%s", &buf[ff], metNd->metricnames[m], &buf[ss]);
	    free(buf);

	  } else {
	    sprintf(sibname,"%s_%s",outvar->name, metNd->metricnames[m]);
	  }

	  // Check to see if this sibling variable exists; if so, log that
	  // fact, and escape.  This will trap out cases where the user has
	  // already defined the field name in the PCM - the user wins over
	  // default behavior when there is a  name collision.

	  // But!  This is a little tricky - it means that we
	  // will store the metric only for the very first dimension which
	  // we transform that gives us the same shape as the outvar.
	  // Thus, if we average time and then average onto a grid of the
	  // same size (maybe a five-rolling mean), we'll only get metrics
	  // for the time average.  I will probably add transform
	  // parameters to control this behavior: avoid metrics when
	  // transforming certain dimensions, or to modify the metric names
	  // so we don't have a name collision.
	  CDSVar *mvar = cds_get_var((CDSGroup *) (outvar->parent), sibname);

	  if (mvar) {
	    LOG(TRANS_LIB_NAME, 
		"Metric field %s already exists; no metrics stored while transforming dimension %d (%s)\n", 
		sibname, od, outvar->dims[od]->name);
	    continue;
	  }

	  // Okay, mvar doesn't exist, so we have to create and fill it

	  // I really think the metric is a clone - same type, same
	  // dimensions, same dataset.  Variable mitosis.
	  // The 0 means we don't copy data, which is good.
	  
	  // Fine, I'll clone it myself.  The only hassle is dim_names
	  char **dim_names=CALLOC(outvar->ndims, char*);
	  for (int dm=0;dm<outvar->ndims;dm++) {
	    dim_names[dm]=outvar->dims[dm]->name;
	  }
	  
	  if ((mvar = cds_define_var((CDSGroup *) outvar->parent, sibname, outvar->type,
				     outvar->ndims, (const char **) dim_names))) {
	    // Very base attributes
	    cds_define_att_text(mvar, "long_name", "Metric %s for field %s",
				metNd->metricnames[m], outvar->name);

	    // Now do units.  If set to SAME, then use same units as outvar
	    if (strcmp(metNd->metricunits[m],"SAME") == 0) {
	      size_t lu;
	      CDSAtt *unit_att=cds_get_att(outvar,"units");
	      char *outvar_units = unit_att ? cds_get_att_text(unit_att,&lu,NULL) : NULL;
	      if (outvar_units == NULL) {
		ERROR(TRANS_LIB_NAME,
		      "Transformed variable %s does not have valid units attribute\n",
		      outvar->name);
		cds_define_att_text(mvar, "units", "unknown");
	      } else {
		cds_define_att_text(mvar, "units", "%s", outvar_units);
		free(outvar_units);
	      }
	    } else {
	      cds_define_att_text(mvar, "units", metNd->metricunits[m]); 
	    }

	    // Missing
	    void *missing_value=NULL;
	    int nms = cds_get_var_missing_values(outvar, &missing_value);

	    if (nms > 0 && missing_value) {
	      cds_define_att(mvar, "missing_value", outvar->type,
			     nms, missing_value); 
	    } else {
	      // Great.  Now I have to make up my own missing value
	      // Make sure this holds enough bytes for all data types
	      if (! (missing_value = malloc(cds_data_type_size(outvar->type)))) {
		ERROR(TRANS_LIB_NAME,
		      "Cannot allocate %d bytes for missing value\n",
		      cds_data_type_size(outvar->type));
		return(-1);
	      }

	      cds_get_default_fill_value(mvar->type, missing_value);
	      cds_define_att(mvar, "missing_value", mvar->type, 1,missing_value); 	      
	    }
	    
	    // Either way, have to free up 
	    free(missing_value);
	  } else {
	    LOG(TRANS_LIB_NAME,
		"Warning: Cannot create metric field %s; continuing...\n", sibname);
	  }
	  
	  free(dim_names);


#ifdef notdef	  
	  // This stuff, I think, is redundant, as I no longer need to
	  // do checks for if mvar previously existed.  If it exists, then
	  // we don't fill it, because it means the user wants it for
	  // something else.  But I'm keeping this code in here in case we
	  // change our mind and have upstream code create our metric vars
	  // at some later date.

	  // Now we idiot check everything.  A Null mvar or dimension
	  // mismatch is an easy no go.  Should I log this?
	  if (! mvar || mvar->ndims != Ndims) continue;

	  // Double check mvar's shape, in case it pre-existed
	  okshape=1;
	  for (dm=0;dm<mvar->ndims;dm++) {
	    if (shape[dm] != mvar->dims[dm]->length) {
	      okshape=0;
	      break;
	    }
	  }
	  if (!okshape)  continue;
#endif

	  // Hooray!  We can finally store the metric
	  if (! cds_set_var_data(mvar, CDS_DOUBLE, 0, msample,NULL, 
				    metNd->metrics[m])) {
	    LOG(TRANS_LIB_NAME,
		"Warning: Could not write data to metric field %s\n",
		mvar->name);
	  }
	  
	  // Okay, now tag this variable in outvars user data, to make it
	  // easier to find.
	  if (!cds_set_user_data(outvar, metNd->metricnames[m], (void *) mvar, NULL)) {
	  //if (!cds_set_user_data(outvar, metNd->metricnames[m], (void *) mvar, free)) {
	    LOG(TRANS_LIB_NAME, "Warning: vould not attach metric field %s to user data for %s\n",
		metNd->metricnames[m], outvar->name);
	  }
	} // m loop over metrics
      } // okshape
    } // if metNd

    // Either way, we need to free metNd for the next iteration of our
    // transform code
    free_metric(&metNd);

    // Now that we've transformed all the slices, time to move on to the
    // next dimension.  The output of this transform is the input to the
    // next, so reset our pointers and values.  This gets tricky.

    // First, free up - But! I don't want to free data and qc_data, the
    // original input yet.  So'll I kludge a free only if it's not our
    // first iteration.  And, I'll do it with a seperate counting variable,
    // so we can change the order of dims later.

    if (iter_count++ > 0) {
      if (tdata) free(tdata);
      if (qc_tdata) free(qc_tdata);
    }

    // This assign should work.
    tdata=odata;
    qc_tdata=qc_odata;
    memmove(tD,oD,Ndims*sizeof(int));
    memmove(tlen,olen,Ndims*sizeof(int));
    tNtot=oNtot;

    // do some freeing of our working arrays
    free(data1d);free(qc1d);free(odata1d);free(oqc1d);

    if (transform_name) free(transform_name);
  }

  // Great - we've transformed all the dimensions.  Our new data should be

  // in odata and qc_odata, with dims set correctly.  So now we just need
  // to copy to outvar and return happy.
  if (cds_set_var_data(outvar, CDS_DOUBLE, 0, olen[0],
			  &output_missing_value, odata) == NULL ||
      cds_set_var_data(qc_outvar, CDS_INT, 0, olen[0],
			  NULL, qc_odata) == NULL) {
    ERROR(TRANS_LIB_NAME, "Problem writing output data for %s or %s\n",
	  outvar->name, qc_outvar->name);
    return(-1);
  }

  // Now, finally, add in cell_transforms att
  char *cell_transform;
  if ((cell_transform=trans_build_param_att(outvar->name)) != NULL) {
    cds_define_att_text(outvar, "cell_transform", "%s", cell_transform);
    free(cell_transform);
    trans_destroy_param_list();
  }

  // Old way - probably exactly the same
  //if (cds_put_var_data(outvar, 0, olen[0], CDS_DOUBLE, odata) == NULL ||
  //    cds_put_var_data(qc_outvar, 0, olen[0], CDS_INT, qc_odata) == NULL) {
  //  ERROR(TRANS_LIB_NAME, "Problem writing output data for %s or %s",
  //	  outvar->name, qc_outvar->name);
  //  return(-1);
  //  }

  // I'll free stuff up down here, only if things go well.  Brian will
  // probably hate that.
  if (data) free(data);
  if (qc_data) free(qc_data);
  if (odata) free(odata);
  if (qc_odata) free(qc_odata);
  // if (qc_temp) free(qc_temp);

  // Free our dimensional arrays
  free(D);free(tD);free(oD);free(iD);
  free(len);free(tlen);free(olen);free(ilen);
  free(okshape); free(shape); free(group_order);

  // Free the dim groups
  int g;
  for (g=0;g<Ndimgroups;g++) {
    free_dim_group(&dim_groups[g]);
  };
  free(dim_groups);

  // Free regexp
  regfree(&at_re);

  // Free up our default qc_mapping_function.  Note that the way I've coded
  // this, you can only have one qc_mapping function for all input fields.
  // That's no good - I need a lookup table kind of thing; foo.
  if (qc_mapping_function == default_qc_mapping_function) {
    qc_mapping_function=NULL;
  }

  call_getrusage("*** End of transform driver");

  return(0);
}
  

// calculate the average interval for a given dim of a given var
// If this isn't regular, I don't know what this will tell you, but there
// it is.
double trans_calculate_interval(CDSVar *var, int dim) {
  int i;
  int cmpdbl(const void *, const void *);
  double val;

  int nv=var->dims[dim]->length;
  CDSVar *coord = cds_get_coord_var(var, dim);
  double *index=(double *) cds_copy_array(coord->type, nv,
					  coord->data.vp,
					  CDS_DOUBLE, NULL,
					  0, NULL, NULL, NULL,
					  NULL, NULL, NULL); 

  // Take median diff, which is more robust to slighly irregular grids

  // First, convert index to a diff array, in place.
  for (i=0;i<nv-1;i++) {
    index[i]=fabs(index[i+1]-index[i]);
  }

  // inefficient and even slightly wrong for even nvs; should use some
  // implementation of quickselect, possibly folded with the above diff-ing 
  // Still, O(N ln N) vs. O(N) isn't going to kill us in less we have
  // millions of elements.

  qsort(index,nv-1, sizeof(double), cmpdbl);
  val=index[(nv-1)/2];

  free(index);
  return(val);

  //double input_interval = fabs(index[nv-1]-index[0])/(float) (nv-1);
  //return(input_interval);
}

int cmpdbl(const void *v1, const void *v2) {
  double x1, x2;
  x1= *(double *)v1;
  x2= *(double *)v2;

  if (x1 < x2) return (-1);
  if (x1 > x2) return (1);
  return(0);
}

// Function to parse out the dimensional grouping trans parameter and
// return a dim_groups array, in group order.  For example:
// {time}, {station:lat,lon}, {height}

// Warning: this is complicated and impenetrable, because of the regular
// expression stuff.  I honestly thought it was easier to do this in C, but
// it's done now and it works, so I don't want to have to rewrite it.
struct dim_group *parse_dim_grouping(char *dim_grouping,
				      CDSVar *invar,
				      CDSVar *outvar,
				      int *Ndimgroups) {
  // If dim_grouping isn't turned on, then I just need to build my groups
  // directly from the invar -> outvar dimensions.
  if (! dim_grouping) {
    if (invar->ndims != outvar->ndims) {
	ERROR(TRANS_LIB_NAME,
	      "Number of input and output dimension for field %s don't match and no grouping: %d, %d\n",
	      invar->name, invar->ndims, outvar->ndims);
	return(NULL);
    }
    *Ndimgroups = invar->ndims;
  } else {
    // This seems silly, but I'm scanning the string to see how many groups we
    // actually have, which we need before we can allocate.  As groups are {}
    // delimited, counting either start or end braces should give me what I want.
    *Ndimgroups=0;
    for (char *s=dim_grouping;*s; s++) {
      if (*s == '{') (*Ndimgroups)++;
    }
  }

  // Output array
  struct dim_group *groups = CALLOC(*Ndimgroups, struct dim_group);

  if (! dim_grouping) {
    // Just fill in using invar and outvar
    for (int ng=0;ng<*Ndimgroups;ng++) {
      struct dim_group *g = &groups[ng];

      // Size 1 dim per group
      g->ninput=1;
      g->noutput=1;

      // Store the order each group is listed, because we sort to
      // dimension-similar order later on.
      g->order = ng;

      // We have to alloc and copy - we don't want anything pointing back
      // to invar or outvar
      //g->input_dimnames=strdup(invar->dims[ng]->name);
      //g->output_dimnames=strdup(outvar->dims[ng]->name);

      g->input_d_names=CALLOC(1,char *);
      g->output_d_names=CALLOC(1,char *);

      g->input_d_names[0]=strdup(invar->dims[ng]->name);
      g->output_d_names[0]=strdup(outvar->dims[ng]->name);

      g->input_d_length = CALLOC(1, int);
      g->output_d_length = CALLOC(1, int);

      // yadda yadda
      g->ilen = g->input_d_length[0] = invar->dims[ng]->length;
      g->olen = g->output_d_length[0] = outvar->dims[ng]->length;

      g->input_d=ng;
      g->output_d=ng;
    }
  } else {
    // Below here is all for parsing the dim_groups and building the group
    // arrays
    // It seems like we should do this prior to calling this function, as the
    // regexps will be the same no matter how many vars we are transforming.
    // But, whatever.
    regex_t re, re2;
    regcomp(&re, "\\s*\\{([^}]+)\\}", REG_EXTENDED);
    regcomp(&re2,"\\s*(.+)\\s*:\\s*(.+)", REG_EXTENDED);

    // The trick to extracting from a regexp is (a) you need two matches, one
    // for the whole match and one for the submatch, and (b) you need to
    // iterate by setting your string to the end of the current match, in
    // order to find the next match.
    regmatch_t match[2], match2[3];
    char *str=dim_grouping;
    int ng=0;

    while (regexec(&re, str, 2, match, REG_EXTENDED) == 0) {
      if (match[1].rm_so < 0) break;
      
      char *m=str+match[1].rm_so;
      int nchar=match[1].rm_eo - match[1].rm_so;
      char *buf=strndup(m, nchar);

      // Now, a trick to remove all the whitespace
      char buf2[strlen(buf)];

      char *s=buf, *e=buf2;
      do {
	while(isspace(*s)) s++;
      } while((*e++ = *s++));
      free(buf);

      char *input_groups=buf2;
      char *output_groups=buf2;

      // Now do the interior look for mappings.  In other words, if this
      // matches, we have a colon in this group, which means we have
      // different input dims and output dims, and possibly different
      // *numbers* of dimensions on either end.  I.e.: {station: lat,lon}
      // means we take the station dim on input and expect the lat and lon
      // dims (in row-major contiguous order) upon transformation.
      if (regexec(&re2, buf2, 3, match2, REG_EXTENDED) == 0) {
	input_groups=&buf2[match2[1].rm_so];
	output_groups=&buf2[match2[2].rm_so];
	buf2[match2[1].rm_eo]='\0';
      }

      str+=match[1].rm_eo;

      // Okay, as you can see by this testing line:
      // printf("Group %d: %s -> %s\n", ng, input_groups, output_groups);
      // The group index is now ng, input_groups is the list of input dimensions,
      // and output_groups is the list of output dimensions; both maps have had the
      // whitespace removed, so they are straight up comma-delimited
      // strings. So, now we need to stuff everything into the group
      // structure. 

      struct dim_group *g = &groups[ng];

      // Now I need to find out the number of dims I have, which is the
      // same as the 1+number of commas, since it's a comma delimited list

      // With the strtok, we start at zero and count each token
      g->ninput=0;
      g->noutput=0;
      
      // Store the order each group is listed, because we sort to
      // dimension-similar order later on.
      g->order = ng;

      // We have to alloc and copy, because buf2 (as a VLA) only exists in
      // this function's scope.

      // Change this to make array of dim names.  It's just cleaner, and
      // easy enough to construct the comma delimited name later
      //g->input_dimnames=strdup(input_groups);
      //g->output_dimnames=strdup(output_groups);

      // Size of group set to something easy to flag if never gets set
      g->ilen = 1;
      g->olen = 1;

      // Now strtok through our groups, verify and count each dim
      char *saveptr;
      char *wdimname=NULL;
      char *str2=NULL;
      int d;
      
      str2=input_groups;
      
      while ((wdimname=strtok_r(str2, " ,};", &saveptr)) != NULL) {
	// wdimname now has the name of this dim, so, look it up.
	for (d=0;d<invar->ndims;d++) {
	  if (strcmp(invar->dims[d]->name,wdimname) == 0) {
	    // A valid dim!
	    g->ninput++;
	    if (str2) {
	      g->input_d=d;
	      g->ilen=1;  // so the mult below will work
	    };

	    // len of this group
	    g->ilen *= invar->dims[d]->length;
	    
	    break;
	  }
	}

	if (d == invar->ndims) {
	  // This means dim not found, or else we'd break out of the for
	  // loop above
	  ERROR(TRANS_LIB_NAME,
		"Input dimension(s) in group %d missing: %s (%s)",
		ng, wdimname, g->input_d_names[0]);
	  free(groups);
	  return(NULL);
	}

	str2=NULL;  // weird strtok thing - set this to null after first
		   // token is found.  But, as it turns out, it's useful
		   // for telling if we are on the first token or not,
		   // i.e. first dim in group
      }

      // Now do the same thing for the output groups
      str2=output_groups;
      while ((wdimname=strtok_r(str2, " ,};", &saveptr)) != NULL) {
	// wdimname now has the name of this dim, so, look it up.
	for (d=0;d<outvar->ndims;d++) {
	  if (strcmp(outvar->dims[d]->name,wdimname) == 0) {
	    // A valid dim!
	    g->noutput++;
	    if (str2) {
	      g->output_d=d;
	      g->olen = 1;
	    };
	    // len of this group
	    g->olen *= outvar->dims[d]->length;
	    break;
	  }
	}

	if (d == outvar->ndims) {
	  // This means dim not found, or else we'd break out of the for
	  // loop above
	  ERROR(TRANS_LIB_NAME,
		"Output dimension(s) in group %d missing: %s (%s)",
		ng, wdimname, g->output_d_names[0]);
	  free(groups);
	  return(NULL);
	}

	str2=NULL;  // weird strtok thing - set this to null after first
		   // token is found.  But, as it turns out, it's useful
		   // for telling if we are on the first token or not,
		   // i.e. first dim in group
      }

      // Store the lengths and names of the subdimensions of this group
      // We stored the index of the first dim of the group in input_d, so
      // we can iterate off of that
      g->input_d_length = CALLOC(g->ninput, int);
      g->input_d_names = CALLOC(g->ninput, char *);
      for (d=0;d<g->ninput; d++) {
	g->input_d_length[d]=invar->dims[d+g->input_d]->length;
	g->input_d_names[d] = strdup(invar->dims[d+g->input_d]->name);
      }

      g->output_d_length = CALLOC(g->noutput, int);
      g->output_d_names = CALLOC(g->noutput, char *);
      for (d=0;d<g->noutput; d++) {
	g->output_d_length[d]=outvar->dims[d+g->output_d]->length;
	g->output_d_names[d] = strdup(outvar->dims[d+g->output_d]->name);
      }

      // Now, increment ng
      ng++;

    }

    // Free my regular expressions
    regfree(&re);
    regfree(&re2);

    // Now, before I return the groups, I need to sort them by their input
    // dimensions.  Does using qsort make sense?  It actually might, not for
    // speed, but just for rigorousness.  Also, it's easy to do, and probably
    // takes less coding time than writing my own.
    int cmpdimgroup(const void *v1, const void *v2);
    qsort(groups, *Ndimgroups, sizeof(struct dim_group), cmpdimgroup);

  } // if dim_groups
  return(groups);
}

// Free up the allocated portions of a dim group
void free_dim_group(struct dim_group *g) {

  for (int d=0;d<g->ninput; d++) {
    free(g->input_d_names[d]);
  }

  for (int d=0;d<g->noutput; d++) {
    free(g->output_d_names[d]);
  }

  free(g->input_d_length);
  free(g->input_d_names);
  free(g->output_d_length);
  free(g->output_d_names);
}

int cmpdimgroup(const void *v1, const void *v2) {

  int d1,d2;

  d1 = ((struct dim_group *) v1)->input_d;
  d2 = ((struct dim_group *) v2)->input_d;

  if (d1 < d2) return (-1);
  if (d1 > d2) return (1);
  return(0);
}

void call_getrusage(const char *msg) {
  struct rusage rusage;
  getrusage(RUSAGE_SELF, &rusage);

  DEBUG_LV4("libtrans",msg);

  DEBUG_LV4("libtrans","ru_utime = %f\n",
	 (double) rusage.ru_utime.tv_sec + (double) rusage.ru_utime.tv_usec/1e6);

  DEBUG_LV4("libtrans","ru_stime = %f\n",
	 (double) rusage.ru_stime.tv_sec + (double) rusage.ru_stime.tv_usec/1e6);

  DEBUG_LV4("libtrans","ru_maxrss = %ld\n", rusage.ru_maxrss);
  DEBUG_LV4("libtrans","ru_idrss = %ld\n", rusage.ru_idrss);
  DEBUG_LV4("libtrans","ru_isrss = %ld\n", rusage.ru_isrss);
  DEBUG_LV4("libtrans","ru_minflt = %ld\n", rusage.ru_minflt);
  DEBUG_LV4("libtrans","ru_majflt = %ld\n", rusage.ru_majflt);
  DEBUG_LV4("libtrans","ru_nswap = %ld\n", rusage.ru_nswap);
  DEBUG_LV4("libtrans","ru_inblock = %ld\n", rusage.ru_inblock);
  DEBUG_LV4("libtrans","ru_oublock = %ld\n", rusage.ru_oublock);
  DEBUG_LV4("libtrans","ru_msgsnd = %ld\n", rusage.ru_msgsnd);
  DEBUG_LV4("libtrans","ru_msgrcv = %ld\n", rusage.ru_msgrcv);
  DEBUG_LV4("libtrans","ru_nsignals = %ld\n", rusage.ru_nsignals);
  DEBUG_LV4("libtrans","ru_nvcsw = %ld\n", rusage.ru_nvcsw);
  DEBUG_LV4("libtrans","ru_nivcsw = %ld\n", rusage.ru_nivcsw);
}
