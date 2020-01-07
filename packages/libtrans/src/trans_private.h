
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

/** @file trans_private.h
 *  typedefs for internal transform functions and structures.
 *  You should NOT include this unless developing libtrans
 */

#ifndef _TRANS_PRIVATE_H
#define _TRANS_PRIVATE_H

// prototyping for private functions - storing param atts for one
int trans_store_param(char *name, char *val, char *dim, char *field);
int trans_destroy_param_list();
char *trans_build_param_att(char *field);
void trans_print_param_list();

// Macro for reading and setting attribute with text
#define trans_store_param_text(var,param,dim,field)			\
  {char *__xbuf=cds_get_transform_param(var,param,CDS_CHAR,NULL,NULL);	\
    if (__xbuf) {trans_store_param(param,__xbuf,dim,field); free(__xbuf);}};

#define trans_store_param_val(param,format,val,dim,field)	\
  {char __xbuf[100];sprintf(__xbuf,format,val);\
    trans_store_param(param,__xbuf,dim,field);};

#define trans_store_param_text_by_dim(var,indim, param,dim,field)       \
  {char *__xbuf=cds_get_transform_param_by_dim(var,indim,param,CDS_CHAR,NULL,NULL); \
    if (__xbuf) {trans_store_param(param,__xbuf,dim,field); free(__xbuf);}};

#endif //_TRANS_PRIVATE_H
