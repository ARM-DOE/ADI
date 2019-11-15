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
*    $Revision: 76984 $
*    $Author: shippert $
*    $Date: 2017-02-28 20:41:14 +0000 (Tue, 28 Feb 2017) $
*    $State: $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file trans_param_atts.c
 *  Functions and structures to capture and output transform parameters
 *  into field attributes.
 */

# include <string.h>
# include "trans.h"
# include "trans_private.h"

struct param_node {
  char *name;  // name of parameter
  char *val;   // value of parameter, as a string
  char *dim;   // dimension name, or global
  char *field; // Field name, primarily for concurrancy in the future 
  struct param_node *next;   // for global attribute chain
  struct param_node *prev;   // for global attribute chain
};

// A global, hopefully static pointer, which I will tack new params onto as
// I go.  In the end, I'll have a list of all the params I want to output.
static struct param_node *_Param_List=NULL;
static struct param_node *_Last_Param=NULL;
static int _Nparams=0;

// This is kludgy, but static allocations are just easier
#define _MAXDIMS 20
#define _MAXBUF 4096

int trans_store_param(char *name, char *val, char *dim, char *field) {

  // First, scan down entire list and see if we have an identical attribute
  // already.  IF so, then dump out, because we don't need to store it.

  struct param_node *n=_Param_List;
  while (n) {
    if (strcmp(n->name, name) == 0 &&
	strcmp(n->val, val) == 0 &&
	strcmp(n->dim, dim) == 0 &&
	strcmp(n->field, field) == 0) {
      return(0);
    }
    n=n->next;
  }

  // Okay, it's a new param, so now tack it on the end
  // Allocate current node
  struct param_node *current=malloc(sizeof(struct param_node));

  current->name = strdup(name);
  current->val = strdup(val);
  current->dim = strdup(dim);
  current->field = strdup(field);
  current->prev=NULL;
  current->next=NULL;

  // Now tack into our list
  if (_Nparams == 0) {
    _Param_List=current;
  } else {
    _Last_Param->next=current;
    current->prev=_Last_Param;
  }

  _Last_Param=current;
  _Nparams++;
    
  return(0);
}

int trans_destroy_param_list() {
  struct param_node *n=_Param_List, *m;
  
  while(n) {
    free(n->name);
    free(n->val);
    free(n->dim);
    free(n->field);

    // Tricky - point to next node before freeing allocation for this one
    m=n->next;
    free(n);
    n=m;
  }

  // Reset the param list
  _Param_List=NULL;
  _Last_Param=NULL;
  _Nparams=0;

  return(0);
}


// Diagnostic function
void trans_print_param_list() {
  struct param_node *n=_Param_List;
  while (n) {
    printf("%s %s %s %s\n", n->name, n->val, n->dim, n->field);
    n=n->next;
  }
}

// This will build the attribute string we need to store.  There are
// several elements to this: looping over all dimensions, finding the
// transformation names, stringing things together.  This involves running
// down my param list a lot, but I suspect there is no point in optimizing
// it 
char *trans_build_param_att(char *field) {
  struct param_node *n;
  char *dim_list[_MAXDIMS]={NULL};
  int ndims,d,nbuf=0;

  char buf[_MAXBUF];

  // First, count the number of different dimensions we have to list 
  n=_Param_List;
  ndims=0;
  while(n) {
    for (d=0;d<ndims;d++) {
      if (strcmp(dim_list[d],n->dim) == 0) {
	break;
      }
    }
    if (d==ndims) {
      // We need a new elements of dimlist
      dim_list[d]=strdup(n->dim);
      ndims++;
    }
    n=n->next;
  }

  // Now, loop over our dims, excluding NODIM, and do the same kind of
  // thing.
  for (d=0;d<ndims;d++) {
    // skip globals, which we've moved to the front
    if (strcmp(dim_list[d],"NODIM") == 0) continue;

    // First, scan down and find the transform name. Inefficient, but
    // whatever. 
    n=_Param_List;
    while (n) {
      if (strcmp(n->field,field) == 0 &&
	  strcmp(n->dim,dim_list[d]) == 0 &&
	  strcmp(n->name,"transform") == 0) {

	// note: this is of form dim: TRANS_NAME (param: val), so n->dim is
	// correct here, and n->name is correct in the next section
	if (nbuf) {
	  nbuf += sprintf(buf+nbuf," %s: %s (", n->dim,n->val);
	} else {
	  // I just can't figure out how to do spaces elegantly
	  nbuf += sprintf(buf+nbuf,"%s: %s (", n->dim,n->val);
	}
	break;
      }
      n=n->next;
    }

    // If no "transform" param is set, then n==NULL; otherwise we would
    // have broken out of the loop.  So stub in a default of TRANS_UNKOWN
    if (! n) {
	nbuf += sprintf(buf+nbuf," %s: %s (", dim_list[d],"TRANS_UNKNOWN");
    }      

    // Now, rescan down and find all the params
    n=_Param_List;
    while (n) {
      if (strcmp(n->field,field) == 0 &&
	  strcmp(n->dim,dim_list[d]) == 0 &&
	  strcmp(n->name,"transform") != 0) {
	nbuf += sprintf(buf+nbuf,"%s: %s ", n->name,n->val);
      }
      n=n->next;
    }
    // Check for an extraneous space, so:
    if (buf[nbuf-1] == ' ') {
      nbuf--;
    }

    // Now close parens, or remove parens if they are empty
    if (buf[nbuf-1] == '(') {
      nbuf -= 2;  // There is an extra space before the open paren
    } else {
      nbuf += sprintf(buf+nbuf,")");
    }
  }

  // Next, build up the global string
  n=_Param_List;
  while (n) {
    if (strcmp(n->field,field) == 0 &&
	strcmp(n->dim,"NODIM") == 0) {

      // key: value
      nbuf += sprintf(buf+nbuf," %s: %s", n->name, n->val);
    }
    n=n->next;
  }
  // no End paren
  // nbuf += sprintf(buf+nbuf,")");

  // Great. Now, null-terminate, and copy into a pointer
  //nbuf+=sprintf(buf+nbuf,"\0");
  // Apparently, sprintf always null terminates.  Also, strndup also null
  // terminates, so it's possible the above would add three nulls on the end
  // of the string. If not, and I really do need a null, do this instead to
  // make -Wall stop complaining:
  // nbuf+=sprintf(buf+nbuf,"%c", '\0');

  char *ret = NULL;
  if (nbuf > 0) {
    ret=strndup(buf,nbuf);
  }

  // Memory cleanup
  for (d=0;d<ndims;d++) {
    free(dim_list[d]);
  }

  // return
  return(ret);
}
      
      
  
