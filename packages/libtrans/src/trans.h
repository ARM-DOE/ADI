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
*    $Revision: 75893 $
*    $Author: shippert $
*    $Date: 2017-01-17 18:59:18 +0000 (Tue, 17 Jan 2017) $
*    $State: $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file trans.h
 *  typedefs for transform functions and structures.
 */

#ifndef _TRANS_H
#define _TRANS_H

#include "cds3.h"

#define TRANS_LIB_NAME "libtrans"


// Generic QC states: a superset of all the possible QC states from our
// transformations.  This probably will need some revision somewhere -
// possibly making it transform specific (although how to do that with
// different transforms in different directions, I don't know).  

// While debugging, it's nice to have them be variables rather than
// defines, but compiling with -Wall complains because we don't use them
// all in every function.  We'll decide how to do this for good later.

extern int QC_BAD;
extern int QC_INDETERMINATE;
extern int QC_INTERPOLATE;
extern int QC_EXTRAPOLATE;
extern int QC_NOT_USING_CLOSEST;
extern int QC_SOME_BAD_INPUTS;
extern int QC_ZERO_WEIGHT;
extern int QC_OUTSIDE_RANGE;
extern int QC_ALL_BAD_INPUTS;
extern int QC_ESTIMATED_INPUT_BIN;
extern int QC_ESTIMATED_OUTPUT_BIN;

extern int QC_BAD_STD;
extern int QC_INDETERMINATE_STD;
extern int QC_BAD_GOODFRAC;
extern int QC_INDETERMINATE_GOODFRAC;

// Generic qc operators - might need to move these out to another library,
// or rename them.  Or at least ifdef them.  I'm not sure all of them are
// important, but we use at least first two in the default trans functions,
// so here they are.

// I'm adding in the check for bit > 0, to disallow zero or negative bits.
// This allows us to turn off a bit by, for example, setting QC_BAD = 0
#ifndef qc_set
#  define qc_set(qc,bit) ((qc) |= (bit > 0 ? (1<<(bit-1)) : 0))
//#  define qc_set(qc,bit) ((qc) |= (1<<(bit-1)))
#endif 

#ifndef qc_check
#  define qc_check(qc,bit) ((qc) & (bit > 0 ? (1<<(bit-1)) : 0))
#endif 

#ifndef qc_clear
//   define qc_clear(qc,bit) ((qc) &= (~(1<<(bit-1))))
#   define qc_clear(qc,bit) ((qc) &= (~(bit > 0 ? (1<<(bit-1)) : 0)))
#endif

#ifndef qc_check_mask
#   define qc_check_mask(qc, mask) ((qc) & (mask))
#endif


// Now start prototyping the various functions

// Metric structure, which allows us to return metric information out of the
// interface with just one pointer.  Note that these will be set *inside* of
// the interface function, to tell the driver how to store these metrics
typedef struct _TRANSmetric {
  const char **metricnames;  //used as the tag to find sibling metric variables
  const char **metricunits;  //must define these in interface function
  int nmetrics;
  double **metrics;  // metric[nmetrics][*] - each return metric is 1D
  double *bad_max;  // To allow filtering - dimensioned by [m]
  double *bad_min;
  double *ind_max;  // To allow filtering - dimensioned by [m]
  double *ind_min;
} TRANSmetric;

// The designated argument struct for the interface functions.  This lets
// me easily modify the inputs and add stuff to or even modify them without
// having to rewrite everything in all the interface functions.  It even
// lets me use a #define to call each interface function with default
// values and a named argument list, so I don't have to remember the order
// of everything.  
typedef struct {
  double *input_data;
  double input_missing_value;
  int *input_qc;
  double *output_data;
  double output_missing_value;
  int *output_qc;
  CDSVar *invar;
  CDSVar *outvar;
  int d;
  int od;  // Output dim index - may be different than input
  TRANSmetric **met;
} interface_s;

// This is a structure that links an trans interface function to a
// character string name; I need it up here so that we can prototype the
// get_transform function for public use.  That's important if we want to
// make these functions available outside of the transform driver, which we
// might. 

// Adding space for dedicated 1D and 2D versions of the functions, which
// means we'll have to set them to NULL
// I'm trying something else, but this is how we might essentially overload
// the interface functions, by attaching specific 1D and 2D versions to them.
//typedef struct _TRANSfunc {
//  char *name;
//  int (*func)(double *, int*, double *, int*, CDSVar *, CDSVar*, int);
//  int (*func1D)(CDSVar *, CDSVar*, CDSVar*, CDSVar*);  
//  int (*func2D)(CDSVar *, CDSVar*, CDSVar*, CDSVar*);
//} TRANSfunc;

typedef struct _TRANSfunc {
  char *name;
  //int (*func)(double *, int*, double *, int*, CDSVar *, CDSVar*, int,
  //TRANSmetric **);
  int (*func)(interface_s);
} TRANSfunc;

// I have no idea how to prototype the second argument here; it's a pointer
// to a function that takes those six arguments.
int assign_transform_function(char *, int(*)());
void assign_qc_mapping_function(int(*)());
int default_qc_mapping_function(CDSVar *, double , int);

TRANSfunc *get_transform(char*);
int cds_transform_driver(CDSVar *, CDSVar *, CDSVar *, CDSVar *);

// Default transform interface functions - they all need to be prototyped
// this way.
//int trans_interpolate_interface(double*, int*,double*,int*,CDSVar*,CDSVar*,int, TRANSmetric **); 
//int trans_subsample_interface(double*, int*,double*,int*,CDSVar*,CDSVar*,int, TRANSmetric **);
//int trans_bin_average_interface(double*, int*,double*,int*,CDSVar*,CDSVar*,int, TRANSmetric **);
//int trans_passthrough_interface(double*, int*,double*,int*,CDSVar*,CDSVar*,int, TRANSmetric **);

// Now we just use the interface struct to hold our input arguments
int trans_interpolate_interface(interface_s);
int trans_subsample_interface(interface_s);
int trans_bin_average_interface(interface_s);
int trans_passthrough_interface(interface_s);
int trans_caracena_interface(interface_s);

// This is a working version of a core function calling structure
// My idea here is to hold a superset of all the elements we might call in
// our core functions.  Of course, we can't assume we know everything into
// the future, so I'll stick a void *aux in there as well, which can hold
// an ad-hoc auxilliary structure.  Hurm.  Is this really worth it?  It
// would let me add things to every core function (like, for example, error
// codes and stuff like that).  But it may not actually add any clarity or
// functionality. 
typedef struct {
  // input elements
  double *input_data;
  int *input_qc;
  unsigned int qc_mask;
  double *index;  // for single value indeces
  double *index_boundary_1;  // for bins
  double *index_boundary_2;  // ...
  double **index_n;  // for multiple input coordinate dims
  double input_missing_value;
  int nindex;

  // output, or transformed, elements
  double *output_data;
  int *output_qc;
  double *target;  
  double *target_boundary_1;
  double *target_boundary_2;
  int ntarget;
  double output_missing_value;
  double **target_n;  // for multiple output coordinate dims

  double ***metrics;

  // Here are some possibly transform specific values.  Maybe put them in
  // the aux?
  double *weights;
  double range;
  
  // And an auxilliary pointer, just in case
  void *aux;
} core_s;

// Macro to call the transformation?
// So:  call_transform(binlinear_interpolate, .input_data=data, .input_qc=qc_data,....)
#define call_core_function(func,...) (func((core_s){.weights=NULL, .range=1800,__VA_ARGS__}))

// Default transform core functions.
// It is not clear to me that this is the best place to prototype these,
// but I would like to make them available to anyone, especially people
// writing custom transforms that may default back to one of these or
// something. 

// These two have no metrics calls yet, but I should stub one in.

int bilinear_interpolate(core_s);
int subsample(core_s);
int bin_average(core_s);

#ifdef notdef

// Old prototypes
int bilinear_interpolate(double*,int*,unsigned int,double*,int,double,
			 double*,int*,double*,int,double, double ***);

int subsample(double*,int*,unsigned int,double*,int,double,
	      double*,int*,double*,int,double, double ***);

int bin_average(double *,int *,unsigned int, double *,double *,double *,int,
		double *,int *,double *,double *,int,double, double, double ***); 

#endif

// util functions - may be moved.
void *cds_get_transform_param_by_dim(void *, CDSDim*, const char *,
				     CDSDataType, size_t*, void*);
CDSVar *cds_get_metric_var(CDSVar *, char *);
unsigned int get_qc_mask(CDSVar*);
int allocate_metric(TRANSmetric **, const char **, const char **, int , int);
int free_metric (TRANSmetric **);
const char *trans_lib_version(void);
int get_bin_edges(double **, double **, double *, int, CDSVar *, int);
double *get_bin_midpoints(double *index, int nbins, CDSVar *var, int d);
int set_estimated_bin_qc(int *qc_odata, CDSVar *invar, int d,
			 CDSVar *outvar, int od, int nt);
CDSVar *get_qc_var(CDSVar *);

void trans_turn_off_default_edges();
#endif
