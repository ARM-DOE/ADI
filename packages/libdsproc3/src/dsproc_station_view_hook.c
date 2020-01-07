/*******************************************************************************
 *
 *  Your copyright notice
 *
 ********************************************************************************
 *
 *  Author:
 *     name:  Tim Shippert
 *     phone: 509-375-5997
 *     email: tim.shippert@pnnl.gov
 *
 ********************************************************************************
 *
 *  REPOSITORY INFORMATION:
 *    $Revision: 80630 $
 *    $Author: shippert $
 *    $Date: 2017-09-18 17:27:51 +0000 (Mon, 18 Sep 2017) $
 *    $State: Exp $
 *
 ********************************************************************************
 *
 *  NOTE: DOXYGEN is used to generate documentation for this file.
 *
 *******************************************************************************/

/** @file dsproc_station_view_hook.c
 *  Post-transform hook used to convert fields to station view
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "cds3.h"
#include "trans.h"
#include <sys/wait.h>
#include <regex.h>

#include "dsproc3.h"

# define CALLOC(n,t)  (t*)calloc(n, sizeof(t))

// Simple list stuff, with an index, just for fun
typedef struct node {
  struct node *next;
  char *name;
  int index;
} node;

static int  _insert_node(node **head, char *name);

// not used
//static void _print_list(node *head);
//static void _destroy_list(node *head);

void _build_lists(node **field_list, node **station_list, CDSGroup *group);

static CDSVar *_get_station_var(CDSGroup *g, char *name);
static CDSVar *_get_trans_var(CDSGroup *g, char *name);

int dsproc_station_view_hook(
    void     *user_data,
    time_t    begin_date,
    time_t    end_date,
    CDSGroup *trans_data)
{
  // prevent "unused parameter" compiler warning

  if (! trans_data ||
      trans_data->ngroups == 0) {
    return(0);
  }

  user_data  = user_data;
  begin_date = begin_date;
  end_date   = end_date;
  
  // First of all, I need to find out what fields are going to be
  // converted.  The obvious solution is a trans parameter, but we might
  // also look for fields with a common structure like foo_C1, foo_E13,
  // etc.  Or maybe foo_sgpC1?  Or, maybe a special character: foo@E13 - is
  // that a valid field name?

  // Another wrinkle - if I'm letting the PCM set the fields and facs, what
  // happens if we have different fields merge on different stations?  My
  // solution: merge all fields on *all* stations extracted from the @
  // regexp: if we have foo@C1, foo@E13, bar@E27, we allocate and merge
  // both foo and bar on {C1,E13,E27}.  If one set of field@station is
  // missing from trans_data, fill it in with missing. 


  node *field_list=NULL;
  node *station_list=NULL;
  size_t len;

//  cds_print_transform_params(stdout, "  ", trans_data->groups[0], NULL);

  // Now I seed these with the transform parameters
  char *station_params =
    cds_get_transform_param(trans_data->groups[0], "station_view_stations",
				CDS_CHAR, &len, NULL);
  if (station_params) {
    // I'll need to strtok this to scan down
    char *scratch = (char *)NULL;
    char *st_name = (char *)NULL;
    char *p=station_params;
    while ((st_name = strtok_r(p," ", &scratch))) {
      p=NULL;
      _insert_node(&station_list, st_name);
    }
  }

  // Now list the fields
  char *field_params =
    cds_get_transform_param(trans_data->groups[0], "station_view_fields",
				CDS_CHAR, &len, NULL);
  if (field_params) {
    // I'll need to strtok this to scan down
    char *scratch = (char *)NULL;
    char *st_name = (char *)NULL;
    char *p=station_params;
    scratch = (char *)NULL;
    while ((st_name = strtok_r(p, " ", &scratch))) {
      p=NULL;
      _insert_node(&field_list, st_name);
    }
  }
  
  // Okay, now add in fields as listd from pcm
  _build_lists(&field_list, &station_list, trans_data);

  // Now we have to loop over all the input datasets, to find fields that
  // may not have been transformed (i.e. static location fields)
  int *dsids, ndsids;
  ndsids=dsproc_get_input_datastream_ids(&dsids);
  for (int g=0;g<ndsids;g++) {
    CDSGroup *inds = dsproc_get_retrieved_dataset(dsids[g],0);
    if (inds) {
      _build_lists(&field_list, &station_list, inds);
    }
  }
  free(dsids);

  // Just do some counting and indexing, now that everything is in order
  node *field=field_list;
  int nfields=0;

  while (field) { 
    field->index=nfields++;
    field=field->next;
  }

  node *station=station_list;
  int    nstations=0; 
  size_t maxlen=0;    // how much space to allocate for the station name
  while (station) {
    station->index=nstations++;
    if (strlen(station->name) > maxlen) {
      maxlen=strlen(station->name);
    }
    station=station->next;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Set up the output group, just below trans_data (sibling to the coord
  // system groups - the idea is that the station view is a new coord
  // system, which, it kinda is.
  CDSGroup *station_group=cds_define_group(trans_data, "station_view");

  // Now I have enough information to build the station dimension.
  CDSDim *station_dim=cds_define_dim(station_group, "station", (size_t) nstations, 0); 

  // So, now I need to build a coordinate variable?  Or not?  Anyway, I
  // would like to index this new dimension, if possible, so we need a
  // string variable, which means a string length dimension
  //CDSDim *strlen_dim=cds_define_dim(station_group, "strlen", (size_t)
  // maxlen+1, 0);
  cds_define_dim(station_group, "strlen", (size_t) maxlen+1, 0);

  // Check out the compound literal
  CDSVar *station_name=cds_define_var(station_group, "station_name", CDS_CHAR,
				      2, ((const char *[]) {"station", "strlen"}));

  // Stations is our sample variable...
  cds_alloc_var_data(station_name, 0, nstations);

  // Now, write it out
  station=station_list;
  int s=0;
  while (station) {
    cds_set_var_data(station_name, CDS_CHAR, s++, 1, NULL, station->name);
    station=station->next;
  }    

  ////////////////////////////////////////////////////////////////////////////////
  // Yay!  Now we have lists of fields and stations for which to combine
  // the data.  Now, figure out how to do that...
  field=field_list;
  while (field) {
    station=station_list;

    // Okay, here I need to generate a CDSVar that holds the data in 
    // foo[s][...] format.  That means getting the original dimensions,
    // which we'll do by pulling out the first input data var that has any
    // data. 
    CDSVar *ivar=NULL;

    // Wait!  I'm assuming I have field names of this foo@bar format, and
    // not some other format, which we might once we add in customizability
    // via transform parameters.  Maybe I should extract the delimeter or
    // something? 
    char buf[100];
    sprintf(buf,"%s@%s", field->name, station->name);

    // Loop over stations until we find a valid field@station
    while (station && (ivar=_get_station_var(trans_data,buf)) == NULL) {
      if ((station=station->next)) {
	  sprintf(buf,"%s@%s", field->name, station->name);
      }
    }

    if (ivar == NULL) {
      WARNING( DSPROC_LIB_NAME,
        "Cannot build station view of field %s - fields not found", field->name);
      field=field->next;
      continue;
    }

    // Now, pull out the shape, so I know how to dimension our new data.
    size_t ndims=ivar->ndims;
    size_t nsize=1;
    size_t nsamples = ndims > 0 ? ivar->dims[0]->length : 1;
    //size_t sample_size=cds_var_sample_size(ivar);

    size_t ondims=ndims+1;
    size_t *oshape=CALLOC(ondims, size_t);  // output shape
    CDSDim **odims=CALLOC(ondims, CDSDim *);  // output dim array
    char **odimnames=CALLOC(ondims, char*);

    for (size_t d=0;d<ndims;d++) {
      oshape[d]=ivar->dims[d]->length;
      nsize *= oshape[d];

      // Set output dims to match input dims.  I'm not sure this is
      // normally kosher, but in this case "input" and "output" vars and
      // are all in the same group (trans_data), so they should be able to
      // use the same CDSDims, with the same parent group etc.
      odims[d] = ivar->dims[d];
      odimnames[d] = ivar->dims[d]->name;

      // So, it's tricky, but we need to make sure that we have our
      // dimensions set correctly, and that the coordinate field is set. 
      // We have to do this for every template field, because different
      // ones might have different dimensions and come lexically first
      // (e.g. alt of dim 0 vs. temp of dim 1).  So: 
      
      CDSDim *dim;
      if ((dim=cds_get_dim(trans_data, ivar->dims[d]->name)) == NULL) {
	// Using cds_copy_var will also copy over the dimensions
	CDSVar *cvar = cds_get_coord_var(ivar, d);
	cds_copy_var(cvar, station_group, NULL, NULL, NULL, NULL, NULL,
		     0,0,cvar->sample_count, 0, NULL);
      }
    }

    // So, we are sticking [s] on the end (see my NOTES.transform, 7/21/15
    // and 7/10/15 for a discussion on this).  
    odims[ondims-1] = station_dim;
    oshape[ondims-1] = nstations;
    odimnames[ondims-1] = "station";

    // Wait! The number of samples is the size of first dimension, so
    // tacking a new dim on doesn't change anything.  Thus:
    // size_t onsamples = nsamples*nstations;
    size_t onsamples = nsamples;
    if (ondims == 1) {
      // This means we used to have zero-dim fields, so we need to change
      // our samples to nstations instead
      onsamples=nstations;
    }
      

    // This is still right, though - we are increasing our total size.
    size_t onsize = nsize*nstations;


    // Now create our output CDSVar
    CDSVar *ovar=cds_define_var(station_group, field->name, ivar->type,
				ondims, (const char **) odimnames);

    // We need to fill with missings, which means we need to set a missing
    // value.  We should, of course, match the missings from ivar, if they
    // exist - if not, we build one using one of Brian's utility functions,
    // because it was the first one I found that would do that for a
    // generic type
    void *ivar_missing;
    if (cds_get_var_missing_values(ivar, &ivar_missing) == 0) {

      // If it's a qc field, then we need to flip all the bits.  Or
      // someting
      if (strncmp(field->name,"qc_",3) == 0) {
	ivar_missing=CALLOC(1, int);
	// ((int *) ivar_missing)[0] = ~0;
	qc_set(((int *) ivar_missing)[0], QC_BAD);
      } else {
	ivar_missing=cds_string_to_array("-9999", ivar->type, 
					 (size_t[]){1},
					 NULL);
      }
    }

    // This is a kludge, and I might take it out, but if we are using the
    // default missing value, I want to replace with -9999.  Maybe govern
    // this with a transform param?  
    if (ivar->default_fill &&
	memcmp(ivar_missing, ivar->default_fill,
	       cds_data_type_size(ivar->type)) == 0) {
      ivar_missing=cds_string_to_array("-9999", ivar->type, 
				       (size_t[]){1},
				       NULL);
    }      

    // Now set it in ovar, using the first value from above and
    // missing_value as the name.  (Note we use size of 1, so additional
    // missings will be ignored)

    CDSAtt* ovar_matt=cds_set_att(ovar, 0, "missing_value", ovar->type,
				  (size_t)1, ivar_missing);

    if (! ovar_matt) {
      WARNING( DSPROC_LIB_NAME,
        "Problem setting missing value in varible %f", ovar->name);
    }

    // Now, finally, we can allocate and fill our data with missings.
    cds_init_var_data(ovar, 0, onsamples, 1);

    // Create a place to store our output data.  
    double *odata = CALLOC(onsize, double);

    // Fill with missing values, so I have to convert ivar_missing to
    // double. 
    CDSConverter mtod=cds_create_converter(ivar->type,NULL,CDS_DOUBLE,NULL);
    double omissing;
    cds_convert_array(mtod, 0, 1, ivar_missing, &omissing);
    for (size_t i=0;i<onsize;i++) {
      odata[i] = omissing;
    }

    // This only works with c99 or gnu99 or higher - VLAs.  But it means we
    // don't have to free things up, as vlas automatically free up when we
    // go out of scope.  But it uses stack rather than heap, so for really
    // big datasets it could fail.
    //double odata[onsize];

    // With the data array finally set up, now we loop over stations and
    // fill in our slices. 
    station=station_list;
    while(station) {

      // Its easier to do it this way, to handle exceptions
      int s = station->index;

      sprintf(buf,"%s@%s", field->name, station->name);
      CDSVar *var=_get_station_var(trans_data,buf);

      // As always, revisit what we should do when something fails.  Right,
      // now, I just continue on, and hope the missings are set right.
      if (var == NULL) {
	LOG("No input data for field %f, station %f", field->name, station->name);
	station=station->next;
	continue;
      }

      // Now get the row-majored data array, cast into doubles
      size_t insamples;
      double imiss;
      double *idata = cds_get_var_data(var, CDS_DOUBLE, 0, &insamples, &imiss, NULL);

      // Again, check for failure, although the var==NULL test above should
      // have done that.
      if (idata == NULL) {
	LOG("No input data for field %f, type 2 ", field->name);
	station=station->next;
	continue;
      }

      //So, this slab of foo[a][b][c] goes into foo[a][b][c][s], for a
      //given value of s.  It's probably easier just to do our own loop. 
      for (size_t k=0;k<nsize;k++) {
	// remap input missing to output missing, just in case input
	// missing is something weird like a fill value or CDS_MAX_FLOAT or
	// whatever
	if (idata[k] == imiss) {
	  odata[nstations*k+s]=omissing;
	} else {
	  odata[nstations*k+s]=idata[k];
	}
      }

      free(idata);

      station=station->next;
    } // station loop

    // Now, finally, stuff into the new field, cleanup, and we are done!
    cds_set_var_data(ovar, CDS_DOUBLE, 0, onsamples, ivar_missing, odata);

    // free up stuff
    free(oshape);
    free(odims);
    free(odimnames);
    free(ivar_missing);
    free(odata);  // Do not free if using VLA!

    // Next field
    field=field->next;
  } // field loop

  // This is required to write our new fields out to its own datastream
  // - we have to set the output target on these vars.  We should probably
  // either have a transform param to switch this on or to switch it off,
  // to allow for maximum flexibility.

  // BLERG!  I need to maybe scan down all groups, rather than take the
  // first one!
  char *output_param =
    cds_get_transform_param(trans_data->groups[0], "output_netcdf",
			    CDS_CHAR, &len, NULL);

  // The output is on by default; we have to turn it off via output_netcdf
  // = N,no,Nein,Nincompoop or something with N.
  if (output_param == NULL ||
      (strncmp(output_param,"n",1) != 0 &&
       strncmp(output_param,"N",1) != 0)) {

    // BLERG!  The 0 is the output dsid - I need to loop over all output
    // dsids, or allow a trans param for them!
    int *odsids=NULL;
    int nodsids=dsproc_get_output_datastream_ids(&odsids);

    for (int ds=0;ds<nodsids;ds++) {
      for (int i=0;i<station_group->nvars;i++) {
	dsproc_set_var_output_target(station_group->vars[i], odsids[ds],
				     station_group->vars[i]->name);
      }
      //dsproc_map_datasets(station_group, NULL,0);
    }
    free(odsids);
  }
      
  // We are done, so return a valid
  return(1);
}

// Utility functions, dealing with the field and station lists.
// I'm going to order first by size, then lexically.  This will put the
// shortest elements first, which is what we want for stations (sgpE9
// before sgpE12).  Hopefully it doesn't matter for fields.

int _insert_node(node **head, char *name) {
  if (! *head) {
    *head=(node *)malloc(sizeof(struct node));
    (*head)->name = strdup(name);
    (*head)->next = NULL;
    return(0);
  }

  // Scan down list
  node *list = *head;
  node *prev = NULL;
  while (list) {
    // Compare the name to the name of this element of the list

    if (strcmp(name, list->name) == 0) {
      // already in the list - do nothing
      return(0);
    }

    //if (strcmp(name, list->name) < 0) {

    // Tricky - only do lexical check if lengths are the same
    if (strlen(name) < strlen(list->name) ||
	(strlen(name) == strlen(list->name) && strcmp(name, list->name) < 0)) {
      // Insert a new node before this one
      node *new = (node *)malloc(sizeof(struct node));
      new->name = strdup(name);
      new->next = list;

      if (prev == NULL) {
	*head = new;
      } else {
	prev->next = new;
      }
      return(0);
    }

    // If we are at the end, stick it here. 
    if (list->next == NULL) {
      node *new = (node *)malloc(sizeof(struct node));
      new->name = strdup(name);
      new->next = NULL;
      list->next = new;
      return(0);
    }

    // Otherwise, go to the next element
    prev=list;
    list=list->next;
  }

  // If we are down here, somehow I've munged it all up
  return(-1);
}

// Debugging

void _print_list(node *head) {

  node *list = head;
  while (list) {
    printf("  %s\n", list->name);
    list=list->next;
  }
}

void _destroy_list(node *head) {
  node *list = head;
  node *n;
  while (list) {
    free(list->name);
    n=list;
    list=list->next;
    free(n);
  }
}
      

// Recursive function to scan down all the vars of all the subgroups to
// find fieldnames that might match

// Anyway, here is a regexp that finds @-delimited var names

// This doesn't let you have multiple @s, which might be a good thing -
// the second one just accepts everything after the first @ (including
// more @s) as the sitefac

// int status=regcomp(&at_re, "([^@]+)@([^@]+)", REG_EXTENDED);
// int status=regcomp(&at_re, "([^@]+)@(.+)", REG_EXTENDED);

void _build_lists(node **field_list, node **station_list, CDSGroup *group) {
  regex_t at_re;
  regmatch_t at_match[3];  // Make sure to allocate for the full match, too
  regcomp(&at_re, "([^@]+)@(.+)", REG_EXTENDED);

  // var loop first
  for (int i=0;i<group->nvars;i++) {
    if (regexec(&at_re, group->vars[i]->name, 3, at_match,0) == 0) {
      // So, we have a match.  Pull out our field and sitefac:
      int f=(int) at_match[1].rm_so;
      size_t nf = at_match[1].rm_eo - at_match[1].rm_so;

      int s=(int) at_match[2].rm_so;
      size_t ns = at_match[2].rm_eo - at_match[2].rm_so;

      // We need to strdup, because we need to null terminate, and I don't
      // want to overwrite the @ with '\0'.
      char *field=strndup(&(group->vars[i]->name)[f],nf);
      char *sitefac=strndup(&(group->vars[i]->name)[s],ns);

      // Now, add to our list of fields and facilities we need to merge
      _insert_node(field_list, field);
      _insert_node(station_list, sitefac);

      free(field);free(sitefac);
    }
  }

  // Now recurse over subgroups
  for (int g=0;g<group->ngroups;g++) {
    _build_lists(field_list, station_list, group->groups[g]);
  }
 }

// _get_trans_var will scan down all the subgroups of a group to find a
// field that matches the name.  At the top level we should call with
// trans_data, but, really, it will work with any group.  It is also
// recursive. 
CDSVar *_get_trans_var(CDSGroup *group, char *name) {
  CDSVar *var;

  if ((var = cds_get_var(group, name))) {
    return(var);
  } else {
    for (int g=0;g<group->ngroups;g++) {
      if ((var=_get_trans_var(group->groups[g],name))) {
	return(var);
      }
    }
  }
  return((CDSVar *)NULL);
}

// A quick wrapper to scan down any group (should be called with
// trans_data) to find a variable with name; if no such variable exists,
// look in the retrieved dataset.
CDSVar *_get_station_var(CDSGroup *group, char *name) {
  CDSVar *var;

  if ((var=_get_trans_var(group,name)) == NULL) {
    var=dsproc_get_retrieved_var(name,0);
  }
  return(var);
}

