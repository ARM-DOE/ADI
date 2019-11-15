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
*    $Revision: 80791 $
*    $Author: shippert $
*    $Date: 2017-09-27 16:05:17 +0000 (Wed, 27 Sep 2017) $
*    $State: $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file trans_caracena.c
 *  Station view to 2-D implementation of Caracena regridding
 */

// Check for 64 bit - required
//# include <limits.h>
//# if (__WORDSIZE == 64)

# include <stdio.h>
# include <math.h>
# include <string.h>
# include <time.h>
# include <getopt.h>
# include <regex.h>
# include <stdlib.h>
# include <stdint.h>
# include <cblas.h>

#include "../config.h"
#ifdef HAVE_LAPACKE_H
#include <lapacke.h>
#define clapack_dgetrf LAPACKE_dgetrf
#define clapack_dgetri LAPACKE_dgetri
#else
#include <clapack.h>
#endif

# include "cds3.h"
# include "trans.h"
# include "trans_private.h"

int caracena(double *data, double *deriv_lat, double *deriv_lon,
	     int ns, double *ilat, double *ilon,
	     double *out_data, int no, double *olat, double *olon,
	     int npass, double scale_factor);

static size_t _one=1;

/* We'll be doing a lot of dynamic allocation, so let's make it easier */
# define CALLOC(n,t)  (t*)calloc(n, sizeof(t))
# define REALLOC(p,n,t)  (t*) realloc((char *) p, (n)*sizeof(t));

// Stub in the derivative, since we might as well calculate it as we go.

//#define NUM_METRICS 2
static const char *metnames[] = {
  "nstat", 
  "deriv_lat",
  "deriv_lon"
};

static const char *metunits[] = {
  "unitless",
  "SAME",
  "SAME"
};


int trans_caracena_interface(interface_s is)
{
  //CDSVar *incoord, *outcoord;
  TRANSmetric *met1d;

  // Unlike other interfaces, these are just markers for the dimensions,
  // which we'll use to infer the bin edges.  
  ///double *index=NULL, *target=NULL;

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
  int d=is.d;
  int od=is.od;
  TRANSmetric **met=is.met;

  // Length of input data
  int ni=invar->dims[d]->length;

  // Unfortunately, we can only assume that d is the (first) input
  // dimension.  We now have to track down the 

  // This is a 2D caracena, so that means our output dimension is a
  // row-major ordered 2D array.  The way I've structure this is that we
  // pass in index of the first dim in, so nt = len(od)*len(od+1).
  // We have to keep everything in 1D arrays,
  // even inside the core function, so either we work in 1D arrays always
  // or we map from 2D->1D inside the core function.  Either way, we input
  // a 1D odata array, so we need to figure out how big it is.
  if (outvar->ndims <= od+1) {
    ERROR(TRANS_LIB_NAME, "Not enough dimensions on output: need index %d (ndims=%d)",
	  od+1, outvar->ndims);
    return(-5);
  }


  //int nt = outvar->dims[od]->length * outvar->dims[od+1]->length;
  int nolat = outvar->dims[od]->length;
  int nolon = outvar->dims[od+1]->length;
  int no = nolat * nolon;

  // Define and allocate our metric - in this case the derivative
  // Note - this is for returning the output
  int nmetrics = (sizeof metnames)/(sizeof metnames[0]);
  allocate_metric(met,metnames, metunits, nmetrics, no);
  met1d = (*met);

  // The input coordinate is just the station index - still, we need to
  // make it, so:
  //index=CALLOC(ni,double);
  //for (i=0;i<ni;i++) index[i]=(double) i;
  // Actually, we don't need to make it, if we aren't using the core
  // function input structure.

  // The output coordinates are in lat, lon, so we need to both find them
  // and stuff them into the target_n structure.
  CDSVar *lat_coord = cds_get_coord_var(outvar, od);
  double *olat_1D=(double *) cds_copy_array(lat_coord->type, nolat,
					    lat_coord->data.vp,
					    CDS_DOUBLE, NULL,
					    0, NULL, NULL, NULL, NULL, NULL, NULL); 

  CDSVar *lon_coord = cds_get_coord_var(outvar, od+1);
  double *olon_1D = (double *) cds_copy_array(lon_coord->type, nolon, 
					      lon_coord->data.vp,
					      CDS_DOUBLE, NULL,
					      0, NULL, NULL, NULL, NULL, NULL, NULL); 

  // Now we need to construct a 1D array, so that we have olat[o],olon[o]
  // describe the location of odata[o], for o=[0,nt-1].  We need this
  // because Caracena needs a 1D vector describing the output points -
  // i.e. a virtual "station" view, even though the "stations" are points
  // on a regular grid. 

  // The best way to do this is to loop over i in lat and j in lon, and
  // construct each o from that.

  double *olat=CALLOC(no, double);
  double *olon=CALLOC(no, double);
  for (int i=0;i<nolat;i++) {
    for (int j=0;j<nolon;j++) {
      int o = nolon*i + j;
      olat[o]=olat_1D[i];
      olon[o]=olon_1D[j];
    }
  }

  // Okay, now we need station-based location information, which means
  // finding sibling variables to invar.  We will use transform parameters
  // to find them, and use a default name of "glat" and "glon"
  char *lat_field_name, *lon_field_name;
  
  if ((lat_field_name = cds_get_transform_param(invar,"lat_field", CDS_CHAR,
						&_one, NULL)) == NULL) {
    // use strdup so we can free
    lat_field_name=strdup("lat");
  }

  if ((lon_field_name = cds_get_transform_param(invar, "lon_field",CDS_CHAR, 
						&_one, NULL)) == NULL) {
    // use strdup so we can free
    lon_field_name=strdup("lon");
  }

  CDSVar *lat_var=cds_get_var((CDSGroup *)(invar->parent), lat_field_name);
  CDSVar *lon_var=cds_get_var((CDSGroup *)(invar->parent), lon_field_name);

  free(lat_field_name);
  free(lon_field_name);
  
  if (! lat_var || ! lon_var) {
    ERROR(TRANS_LIB_NAME,"Missing lat and/or lon field in input dataset: %s, %s\n",
	  lat_field_name, lon_field_name);
    return(-1);
  }

  // Now extract ilat and ilon, the station based arrays of location
  size_t nilat, nilon;
  double dummy_missing_value;
  double *ilat = cds_get_var_data(lat_var, CDS_DOUBLE, 0, &nilat,
				  &dummy_missing_value, NULL);

  double *ilon = cds_get_var_data(lon_var, CDS_DOUBLE, 0, &nilon,
				  &dummy_missing_value, NULL);

  if (nilat != (size_t) ni || nilon != (size_t) ni) {
    ERROR(TRANS_LIB_NAME,
	  "Input lat and lon are not dimensioned correctly by station: %d %d %d\n",
	  ni, nilat,nilon);
    return(-1);
  }

  /////////////////////////////////////////////////////////////////////////
  // The easy lookups - override missing_Value by transform params.
  double missing_value;
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

  unsigned int qc_mask=0;
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

  int npass;
  if (cds_get_transform_param_by_dim(outvar, outvar->dims[od], "number_of_passes",
				     CDS_INT, &_one, &npass) == NULL) {
    npass=16;
  }

  trans_store_param_val("number_of_passes", "%d", npass, 
			outvar->dims[od]->name, outvar->name);

  double scale_factor;
  if (cds_get_transform_param_by_dim(outvar, outvar->dims[od], "scale_factor",
				     CDS_DOUBLE, &_one, &scale_factor) == NULL) {
    scale_factor=100.0;
  }

  trans_store_param_val("scale_factor", "%f", scale_factor,
			outvar->dims[od]->name, outvar->name);


  // minimum number of stations to use - 15 by default
  // Bugfix 5/30/17 to actually assign min_stations, rather than
  // scale_factor.
  // Bugfix 9/25/17 again to make it an int, rather than a double
  int min_stations;
  if (cds_get_transform_param_by_dim(outvar, outvar->dims[od], "min_stations",
				     CDS_INT, &_one, &min_stations) == NULL) {
    min_stations=15;
  }

  // Just in case - this will cause problems later in clapack_getrf to
  // invert a matrix if we don't set min_stations to at least one.
  if (min_stations <= 0) {
    ERROR(TRANS_LIB_NAME,
	  "Min stations is %d: must be >=0",
	  min_stations);
    return(-1);
  }


  trans_store_param_val("min_stations", "%d", min_stations, 
			outvar->dims[od]->name, outvar->name);


  //////////////////////////////////////////////////////////////////////////////////
  // Develop data to handle QC and missing values and the like.
  // We eliminate stations that have bad or missing data, so this means we
  // need to copy our data and location information into new arrays.
  double *kdata=CALLOC(ni,double);
  double *klat=CALLOC(ni,double);
  double *klon=CALLOC(ni,double);
  int nk=0;

  for (int i=0;i<ni;i++) {
    if (data[i] == input_missing_value ||
	data[i] >= CDS_MAX_FLOAT-1 ||
	(qc_data[i] & qc_mask) != 0) {
      // This is bad data, so just continue on with the loop without
      // copying into idata or anything
      continue;
    }

    // Okay, now just copy stuff over
    kdata[nk]=data[i];
    klat[nk]=ilat[i];
    klon[nk]=ilon[i];
    nk++;
  }

  //////////////////////////////////////////////////////////////////
  // Allocate derivative arrays
  //////////////////////////////////////////////////////////////////
  double *deriv_lat = CALLOC(no, double);
  double *deriv_lon = CALLOC(no, double);

  //////////////////////////////////////////////////////////////////
  // Set everything to missing if not enough stations
  int status;
  if (nk < min_stations) {
    for (int o=0;o<no;o++) {
      odata[o]=deriv_lat[o]=deriv_lon[o]=output_missing_value;
      // I should set a QC flag here, too.  Some bad inputs?  I think that
      // makes it yellow, but all bad inputs isn't necessarily accurate.
      // So, set some but also set bad.  Maybe I need some new flags just
      // for caracena?
      qc_set(qc_odata[o], QC_BAD);

      if (nk == 0) {
	qc_set(qc_odata[o], QC_ALL_BAD_INPUTS);
      } else {
	qc_set(qc_odata[o], QC_SOME_BAD_INPUTS);
      }
    }
    // Also, the status
    status=0;
  } else {
    //////////////////////////////////////////////////////////////////////////////////
    // call core function
    status = caracena(kdata, deriv_lat, deriv_lon,
		      nk, klat, klon,
		      odata, no, olat, olon,
		      npass, scale_factor);

    // I should also set qc_odata to "some bad" without setting bad if we don't
    // have all the stations but do have enough to run.
    if (nk < ni) {
      for (int o=0;o<no;o++) {
	qc_set(qc_odata[o], QC_SOME_BAD_INPUTS);
      }
    }

    // Check the status - negative status means we had a linear algebra
    // problem and couldn't do the transformation
    if (status < 0) {
      for (int o=0;o<no;o++) {
	odata[o]=deriv_lat[o]=deriv_lon[o]=output_missing_value;
	qc_set(qc_odata[o], QC_BAD);

	// I probably need another QC flag to indicate a failure from the
	// blas libraries.  But I don't have one yet.
	//qc_set(qc_odata[o], QC_BLAS);
      }
    }
  }

  // Pass our metrics (derivs) back to the driver.
  memcpy(met1d->metrics[1], deriv_lat, no*sizeof(double));
  memcpy(met1d->metrics[2], deriv_lon, no*sizeof(double));

  // And our number of stations.  I need to think about collapsing down
  // metrics, since we don't need the lat/lon dimensions for nstat.  But
  // right now metrics have the same dimensions as the data, so we just
  // keep repeating everything.
  for (int o=0;o<no;o++) {
    met1d->metrics[0][o] = nk;
  }

  // Free stuff up
  free(deriv_lat);
  free(deriv_lon);
  free(olat_1D);
  free(olon_1D);
  free(olat);
  free(olon);
  free(ilat);
  free(ilon);
  return(status);
}

// Some linear algebra functions

// row-major value for nxn array
#define M_val(matrix,n,i,j) (matrix[n*i+j])

#define M_mult(M1,M2,Mout,n) \
  (cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, \
	       n,n,n,1,M1,n,M2,n,0,Mout,n))

// Invert row_major flattened array M in place.  Better as a function,
// because it's two lapack calls and needs a pivot array.
int M_invert(double *M, int n) {
  int pivot[n];
  int rval;
  if ((rval=clapack_dgetrf(CblasRowMajor,n,n,(double *)M,n,pivot)) !=0) {
    ERROR(TRANS_LIB_NAME,
	  "DGETRF returns %d: singular weight matrix (check for collocated stations, which are not allowed)",
	  rval);
    return(-1);
  }

  if ((rval=clapack_dgetri(CblasRowMajor,n,(double *)M,n,pivot)) !=0 ){
    ERROR(TRANS_LIB_NAME,
	  "DGETRI returns %d: weight matrix cannot be inverted",
	  rval);
    return(-1);
  }
  return(0);
}


// Return the distance in meters between two lats and lons, plus the
// x and y components 
double mdist(double lat1, double lat2, double lon1, double lon2,
	     double *dlat_m, double *dlon_m) {
  
  // idiot proof
  if (fabs(lat1-lat2) < 0.001 &&
      fabs(lon1-lon2) < 0.001) {
    *dlat_m=0;
    *dlon_m=0;
    return(0.0);
  }

  // Otherwise, we need the cosine of the mean lat
  double mlat=(lat1+lat2)/2.0;
  double cosm=cos(mlat*0.017453292);

  //*dlat_m = fabs(lat1-lat2)*111190.;
  //*dlon_m = fabs(lon1-lon2)*111190.*cosm;

  // I think I want to keep the sign, so we can keep the direction for use
  // with the position vectors in the derivatives.  Note that this requres
  // our origin (the lat_mean and lon_mean) to be the *second* position
  // entered, i.e. lat2=lat_mean, lon2=lon_mean
  *dlat_m = (lat1-lat2)*111190.;
  *dlon_m = (lon1-lon2)*111190.*cosm;

  double dist=sqrt((*dlat_m)*(*dlat_m) + (*dlon_m)*(*dlon_m));
  return(dist);
}

int caracena(double *data, double *deriv_lat, double *deriv_lon,
	     int ns, double *ilat, double *ilon,
	     double *out_data, int no, double *olat, double *olon,
	     int npass, double scale_factor) {

  int i,j,o;
  double dlat_m, dlon_m;

  // First, set up our weight array
  double *W=CALLOC(ns*ns,double);
  double *I_W=CALLOC(ns*ns,double);
  double *W_1=CALLOC(ns*ns,double);

  // Location array, for deriv calculation (indexed by station)
  double *rlat=CALLOC(ns,double);
  double *rlon=CALLOC(ns,double);

  // Scale factor in km, L2 in m2, so mult by 1e6
  double L2=scale_factor*scale_factor*1e6;

  // Want to get the mean lat/lon on our output grid, for use as an origin
  // for the location vectors (needed for derivatives).  I think this is
  // mainly to make our location grid somewhat more cartesian, and to not
  // use distances from the equator/prime meridian as our vectors.  Anyway,
  // this is what Chuck does, so I suspect it's a good idea.
  double lat_mean=0.0, lon_mean=0.0;
  for (o=0;o<no;o++) {
    lat_mean += olat[o]/no;
    lon_mean += olon[o]/no;
  }

  for (i=0;i<ns;i++) {
    double W_sum=0.0;
    for(j=0;j<ns;j++) {
      double dist = mdist(ilat[i], ilat[j], ilon[i], ilon[j],&dlat_m, &dlon_m);
      M_val(W,ns,i,j) = exp(-dist*dist/L2);
      W_sum += M_val(W,ns,i,j);
    }

    // Normalize - need to loop over j again, becuase we are normalizing
    // our columns.  Or maybe our rows.  Whatever.
    for(j=0;j<ns;j++) {
      M_val(W,ns,i,j) /= W_sum;
    }

    // Calculate the position vector (in meters) for each station.  Always
    // have the man values second in the list of args, to keep the signs on
    // rlat and rlon correct.
    // double dist = mdist(ilat[i], lat_mean, ilon[i], lon_mean, &rlat[i], &rlon[i]);
  }

  // Now, we gotta create I_W, so we do the loops again
  for (i=0;i<ns;i++) {
    for(j=0;j<ns;j++) {
      // I - W
      M_val(I_W,ns,i,j) = -M_val(W,ns,i,j);
      if (i==j) {
	M_val(I_W,ns,i,j) += 1.0;
      }
    }
  }

  // Now, invert W
  memcpy(W_1,W,ns*ns*sizeof(double));
  int rval=M_invert(W_1,ns);

  if (rval < 0) {
    // Already had an error message inside of the invert

    // Note that a negative return value percolates up to
    // cds_transform_driver, which then causes an early exit of the code.
    // I think it might be best to instead return(0), which (with the
    // latest changes concurrent with this comment) will fill this
    // particular transformation with missings and set some qc bits, but
    // allow the next sample time to be transformed, rather than dumping
    // out before we write a netCDF file or antyhing.
    //return(-1);
    return(0);
  }

  // Now, build our correction 
  double *Mwork = CALLOC(ns*ns,double);
  double *Mout = CALLOC(ns*ns,double);

  // This builds up (I_W)^npass, as long as npass is a mult of 2
  // Actually, it does (I_W)^m, where m is the power of 2 closest to but
  // not greater than npass.
  memcpy(Mwork, I_W, ns*ns*sizeof(double));
  for (i=2;i<=npass;i*=2) {
    M_mult(Mwork,Mwork,Mout,ns);
    memcpy(Mwork,Mout,ns*ns*sizeof(double));
  }

  // Right here is where we add in more passes, I guess
  // We need npass - i/2 more passes to get to npass.
  for (j=0; j < npass-i/2; j++) {
    M_mult(Mwork,I_W,Mout,ns);
    memcpy(Mwork,Mout,ns*ns*sizeof(double));
  }

  // Finally!  MWork is now (I_W)^npass.  Now build our correction.  First,
  // I-(I_W)^npass
  for (i=0;i<ns;i++) {
    for (j=0;j<ns;j++) {
      M_val(Mwork,ns,i,j) = -M_val(Mwork,ns,i,j);
      if (i==j) M_val(Mwork,ns,i,j) +=1;
    }
  }

  // Finally, mult by the inverse
  M_mult(W_1,Mwork,Mout,ns);

  //...and Mout is our correction matrix C.  So, now we apply C to our
  // station data fs to get a corrected data array fcorr = C*fs
  double *c_data=CALLOC(ns,double);

  // i is the index of the row, j is the index of the column, so 
  // c_data[i] = sum over j (C[i][j]*fs[j])
  for (i=0;i<ns;i++) {
    for (j=0;j<ns;j++) {
      c_data[i] += M_val(Mout,ns,i,j)*data[j];
    }
  }

  // Yay!  Now, to get the value at a given lat, lon, we need to build a
  // weight array of distances from that lat and lon to each station point,
  // and then take the dot product from that weight array and our corrected
  // data.

  //double *out_data=CALLOC(no,double);

  // Okay, to get the matrix the right way, we want: odata = Wr*data
  // data = data[s], odata=odata[o], so Wr = Wr[o][s]
  // i.e. o is the index of the row, s is the index of the column

  double *Wr=CALLOC(no*ns,double);

  // This is the right order of things - Nr is designed to normalize over
  // all stations, so each row (i.e. each value of o) is a normal vector.
  // Thus, for each [o], we sum Nr over s, and *then* we can calculate
  // odata[o] += Wr[o][s]*c_data[s]/Nr for each value of s.

  // Thus, we need two loops over s for each value of o.
  for (int o=0;o<no;o++) {
    double Nr=0.0;
    out_data[o]=0.0;  // init to zero, just in case

    double fRlat=0, fRlon=0, Rlat=0, Rlon=0;
    
    for (int s=0;s<ns;s++) {
      double dist=mdist(ilat[s], olat[o], ilon[s], olon[o], &dlat_m, &dlon_m);
      M_val(Wr,ns,o,s)=exp(-dist*dist/L2);
      Nr+=M_val(Wr,ns,o,s);
    }

    for (int s=0;s<ns;s++) {
      // This simultaneously normalizes Wr and multiples Wr*idata
      out_data[o] += M_val(Wr,ns,o,s) * c_data[s] / Nr;

      // Now generate our <FR> and <R> elements for the derivatives.  Here
      // are how the terms in eqns (9) and (10) in the Caracena map to what
      // we got:
      // fk = c_data[k]  (the "corrected" data from multiple passes
      // rk = r{lat,lon}[k]
      // wk(r) = Wr[o][k]/Nr
      fRlat += c_data[s]*rlat[s]*M_val(Wr,ns,o,s)*c_data[s]/Nr;
      fRlon += c_data[s]*rlon[s]*M_val(Wr,ns,o,s)*c_data[s]/Nr;

      Rlat += rlat[s]*M_val(Wr,ns,o,s)*c_data[s]/Nr;
      Rlon += rlon[s]*M_val(Wr,ns,o,s)*c_data[s]/Nr;

      // So now we have all the elements we need to calcuate the
      // derivatives
      deriv_lat[o] = 2*(fRlat - out_data[o]*Rlat)/L2;
      deriv_lon[o] = 2*(fRlon - out_data[o]*Rlon)/L2;

    }
  }

  // Yay.  We've calculated all our outputs, so we just need to free
  // everything up(!) and exit happily.
  free(W);
  free(I_W);
  free(W_1);
  free(rlat);
  free(rlon);
  free(Mwork);
  free(Mout);
  free(c_data);
  free(Wr);

  return(0);
}


// Quick function to validate our row major arrays
void print_M(double *M, int ni, int nj) {
  for(int i=0;i<ni;i++) {
    printf("\n");
    for(int j=0;j<nj;j++) {
      printf(" %8.5f ", M[ni*i+j]);
    }
  }
  printf("\n\n");
}


//# endif //__WORDSIZE != 64
