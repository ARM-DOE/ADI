
/** @file idl_dsproc.c
 *  IDL bindings to dsproc_* functions.
 */

#include <stdio.h>
#include "dsproc3.h"
#include "idl_export.h"

// Define error messages for IDL users
#define PROC_MODEL_UNDEF 0
#define INVALID_ARGUMENT -1
#define INVALID_POINTER_TYPE -2
#define CDS_TYPE_UNDEF -3
#define TYPE_MISMATCH -4
#define TIME_T_SIZE -5

static IDL_MSG_DEF msg_arr[] = {
  { "PROC_MODEL_UNDEF", "%NUnsupported processing model %s." },
  { "INVALID_ARGUMENT", "%NInvalid argument: %s." },
  { "INVALID_POINTER_TYPE", "%NInvalid pointer type." },
  { "CDS_TYPE_UNDEF", "%NUnsupported CDSDataType: %d." },
  { "TYPE_MISMATCH", "%NIncorrect datatype: %s, expected %s." },
  { "TIME_T_SIZE", "%NUnsupported time_t datatype size." },
}; 
static IDL_MSG_BLOCK msg_block;

// A convenience macro to check if an IDL variable is
// either undefined or equal to !null.
#define IS_UNDEF(var) (((var)->flags & IDL_V_NULL) || \
    (((var)->type == IDL_TYP_UNDEF)))


/** @publicsection
  *
  * IDL does not support .h files directly, so
  * include a runtime function to lookup values.
  *
  * @param s - Pointer to IDL string structure
  */
ProcModel _proc_model(IDL_STRING *s)
{
  ProcModel proc_model;
  if (!strcmp(IDL_STRING_STR(s), "PM_GENERIC"))
    proc_model=PM_GENERIC;
  else if (!strcmp(IDL_STRING_STR(s), "PM_RETRIEVER_VAP"))
    proc_model=PM_RETRIEVER_VAP;
  else if (!strcmp(IDL_STRING_STR(s), "PM_TRANSFORM_VAP"))
    proc_model=PM_TRANSFORM_VAP;
  else if (!strcmp(IDL_STRING_STR(s), "PM_INGEST"))
    proc_model=PM_INGEST;
  else if (!strcmp(IDL_STRING_STR(s), "PM_RETRIEVER_INGEST"))
    proc_model=PM_RETRIEVER_INGEST;
  else if (!strcmp(IDL_STRING_STR(s), "PM_TRANSFORM_INGEST"))
    proc_model=PM_TRANSFORM_INGEST;
  else if (!strcmp(IDL_STRING_STR(s), "DSP_RETRIEVER"))
    proc_model=DSP_RETRIEVER;
  else if (!strcmp(IDL_STRING_STR(s), "DSP_TRANSFORM"))
    proc_model=DSP_TRANSFORM;
  else IDL_MessageFromBlock(msg_block, PROC_MODEL_UNDEF, IDL_MSG_LONGJMP,
      IDL_STRING_STR(s));
  return proc_model;
}

int _cds_to_idl_datatype(CDSDataType cds_type)
{
  if (cds_type == CDS_NAT) return IDL_TYP_UNDEF;
  if (cds_type == CDS_CHAR) return IDL_TYP_STRING;
  if (cds_type == CDS_BYTE) return IDL_TYP_BYTE;
  if (cds_type == CDS_SHORT) return IDL_TYP_INT;
  if (cds_type == CDS_INT) return IDL_TYP_LONG;
  if (cds_type == CDS_FLOAT) return IDL_TYP_FLOAT;
  if (cds_type == CDS_DOUBLE) return IDL_TYP_DOUBLE;
  IDL_MessageFromBlock(msg_block, CDS_TYPE_UNDEF, IDL_MSG_LONGJMP, (int) cds_type);
  // unreachable code:
  return IDL_TYP_UNDEF;
}

CDSDataType _idl_to_cds_datatype(int idl_type)
{
  if (idl_type == IDL_TYP_BYTE) return CDS_BYTE;
  if (idl_type == IDL_TYP_INT) return CDS_SHORT;
  if (idl_type == IDL_TYP_LONG) return CDS_INT;
  if (idl_type == IDL_TYP_FLOAT) return CDS_FLOAT;
  if (idl_type == IDL_TYP_DOUBLE) return CDS_DOUBLE;
  if (idl_type == IDL_TYP_STRING) return CDS_CHAR;
  return CDS_NAT;
}

IDL_VPTR _GettmpNull(void)
{
  // convenience function, returns an IDL !null
  IDL_VPTR res;
  res = IDL_Gettmp();
  res->value.l64 = 0;
  res->flags |= IDL_V_NULL;
  return res;
}

/** IDL Front end to dsproc_initialize
  * Example:
  * IDL> argv=['idl', '-s', 'sbs', '-f', 'S2', '-b', '20110401', $
  *      '-e', '20110402', '-D', '2', '-R', '-n', 'isde_example1']
  * IDL> dsproc_initialize, argv, 'PM_TRANSFORM_VAP', 'Version1'
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
void idl_dsproc_initialize(int argc, IDL_VPTR argv[])
{
  int i, n, nproc;
  char **par, **proc, *version;
  IDL_VPTR tmp=NULL, proc_tmp=NULL;
  IDL_STRING *s;
  ProcModel proc_model;

  IDL_ENSURE_STRING(argv[0]);
  IDL_ENSURE_STRING(argv[2]);
  IDL_ENSURE_SCALAR(argv[2]);
  IDL_ENSURE_SCALAR(argv[1]);
  IDL_ENSURE_ARRAY(argv[0]);

  if (argc == 4) {
    if (IS_UNDEF(argv[3])) {
      // support passing !null
      nproc=0;
      proc=NULL;
    } else if (argv[3]->flags & IDL_V_ARR) {
      // string array case
      IDL_ENSURE_STRING(argv[3]);
      nproc=argv[3]->value.arr->n_elts;
      proc=(char **) IDL_GetScratch(&proc_tmp, nproc, sizeof(char *));
      s=(IDL_STRING *) argv[3]->value.arr->data;
      for (i=0; i<nproc; i++) {
        proc[i]=IDL_STRING_STR(s+i);
      }
    } else {
      //scalar string case
      IDL_ENSURE_STRING(argv[3]);
      nproc=1;
      proc=(char **) IDL_GetScratch(&proc_tmp, nproc, sizeof(char *));
      proc[0]=IDL_STRING_STR(&argv[3]->value.str);
    }
  } else {
    // support omitting argument
    nproc=0;
    proc=NULL;
  }
      

  version=IDL_STRING_STR(&argv[2]->value.str);
  
  if (argv[1]->type == IDL_TYP_STRING) {
    proc_model=_proc_model(&argv[1]->value.str);
  } else {
    tmp=IDL_CvtLng(1, argv+1);
    proc_model=tmp->value.l;
  }


  n=argv[0]->value.arr->n_elts;
  par=(char **) IDL_GetScratch(&tmp, n, sizeof(char *));
  s=(IDL_STRING *) argv[0]->value.arr->data;
  for (i=0; i<n; i++) {
    par[i]=IDL_STRING_STR(s+i);
  }
  dsproc_initialize(n, par, proc_model, version, nproc, (const char **) proc);
  if (tmp) IDL_Deltmp(tmp);
  if (proc_tmp) IDL_Deltmp(proc_tmp);
}

/** IDL front end to dsproc_start_processing_loop
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
//static time_t begin_time, end_time;
IDL_VPTR idl_dsproc_start_processing_loop(int argc, IDL_VPTR argv[])
{
  int res;
  IDL_VPTR tmp;
  IDL_LONG64 *val;
  time_t begin_time=0, end_time=0;
  argc = argc; /* prevent unused parameter compiler warning  */

  val=(IDL_LONG64 *)
    IDL_MakeTempVector(IDL_TYP_LONG64, 2, IDL_ARR_INI_ZERO, &tmp);

  res=dsproc_start_processing_loop(&begin_time, &end_time);
  val[0]=(IDL_LONG64) begin_time;
  val[1]=(IDL_LONG64) end_time;
  IDL_VarCopy(tmp, argv[0]);

  return IDL_GettmpLong(res);
}

/** IDL front end to dsproc_retrieve_data
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_retrieve_data(int argc, IDL_VPTR argv[])
{
  int res;
  IDL_VPTR tmp, ret_val;
  IDL_LONG64 *val;
  CDSGroup *ret_data;
  time_t begin_time, end_time;
  argc = argc; /* prevent unused parameter compiler warning  */

  IDL_ENSURE_ARRAY(argv[0]);
  if (argv[0]->value.arr->n_elts != 2) {
    IDL_MessageFromBlock(msg_block, INVALID_ARGUMENT, IDL_MSG_LONGJMP,
        "INTERVAL, must be a 2 element array");
  }
  tmp=IDL_CvtLng64(1, argv);
  val=(IDL_LONG64 *) tmp->value.arr->data;
  begin_time=(time_t) val[0];
  end_time=(time_t) val[1];
  if (tmp != argv[0]) IDL_Deltmp(tmp);

  ret_data=(CDSGroup *) NULL;

  res=dsproc_retrieve_data(begin_time, end_time, &ret_data);

  ret_val=IDL_Gettmp();
  ret_val->type=IDL_TYP_UNDEF;
  if (res == 1) {
    ret_val->type=IDL_TYP_MEMINT;
    ret_val->value.memint=(IDL_MEMINT) ret_data;
  } else ret_val->flags |= IDL_V_NULL;
  IDL_VarCopy(ret_val, argv[1]);

  return IDL_GettmpLong(res);
}

/** IDL front end to dsproc_merge_retrieved_data
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_merge_retrieved_data(int argc, IDL_VPTR argv[])
{
  int res;
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  argv = argv;

  res=dsproc_merge_retrieved_data();
  return IDL_GettmpLong(res);
}

/** IDL front end to dsproc_transform_data
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_transform_data(int argc, IDL_VPTR argv[])
{
  int res;
  CDSGroup *trans_data=NULL;
  IDL_VPTR tmp;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  res=dsproc_transform_data(&trans_data);
  tmp=IDL_GettmpMEMINT((IDL_MEMINT) trans_data);
  IDL_VarCopy(tmp, argv[0]);
  return IDL_GettmpLong(res);
}

/** IDL front end to dsproc_create_output_datasets
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_create_output_datasets(int argc, IDL_VPTR argv)
{
  int res;
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  argv = argv;

  res=dsproc_create_output_datasets();
  return IDL_GettmpLong(res);
}

/** IDL front end to dsproc_create_output_dataset
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_create_output_dataset(int argc, IDL_VPTR argv[])
{
  CDSGroup *res;
  int      ds_id,set_location;
  time_t   data_time;

  /* prevent unused parameter compiler warning  */
  argc = argc;
  argv = argv;

  ds_id=IDL_LongScalar(argv[0]);
  data_time = IDL_Long64Scalar(argv[1]);
  set_location = IDL_LongScalar(argv[2]);

  res=dsproc_create_output_dataset(ds_id,data_time,set_location);
  return IDL_GettmpMEMINT((IDL_MEMINT) res);
}

/** IDL front end to dsproc_store_output_datasets
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_store_output_datasets(int argc, IDL_VPTR argv)
{
  int res;
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  argv = argv;

  res=dsproc_store_output_datasets();
  return IDL_GettmpLong(res);
}

/** IDL front end to dsproc_store_dataset
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_store_dataset(int argc, IDL_VPTR argv[])
{
  int      res,ds_id,newfile;

  /* prevent unused parameter compiler warning  */
  argc = argc;
  argv = argv;

  ds_id=IDL_LongScalar(argv[0]);
  newfile = IDL_LongScalar(argv[1]);

  res=dsproc_store_dataset(ds_id,newfile);
  return IDL_GettmpLong(res);
}

/** IDL front end to dsproc_finish
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_finish(int argc, IDL_VPTR argv)
{
  int res;
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  argv = argv;

  res=dsproc_finish();
  return IDL_GettmpLong(res);
}
/** IDL front end to dsproc_get_site
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_get_site(int argc, IDL_VPTR argv)
{
  return IDL_StrToSTRING(dsproc_get_site());
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  argv = argv;
}

/** IDL front end to dsproc_get_facility
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_get_facility(int argc, IDL_VPTR argv)
{
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  argv = argv;
  return IDL_StrToSTRING(dsproc_get_facility());
}
/** IDL front end to dsproc_get_name
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_get_name(int argc, IDL_VPTR argv)
{
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  argv = argv;
  return IDL_StrToSTRING(dsproc_get_name());
}
/** IDL helper routine to support enumerated ProcModel type
  * Example:
  *   IDL> res_int = dsproc_proc_model('PM_INGEST')
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_proc_model(int argc, IDL_VPTR argv[])
{
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  IDL_ENSURE_STRING(argv[0]);
  IDL_ENSURE_SCALAR(argv[0]);

  return IDL_GettmpLong((IDL_LONG) _proc_model(&argv[0]->value.str));
}
/** IDL front end to dsproc_get_datastream_id
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_get_datastream_id(int argc, IDL_VPTR argv[])
{
  int id;
  DSRole role;
  IDL_VPTR tmp;
  const char *site, *facility, *dsc_name, *dsc_level;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  if (argv[0]->flags & IDL_V_NULL) {
    site=NULL;
  } else {
    site=IDL_VarGetString(argv[0]);
  }
  if (argv[1]->flags & IDL_V_NULL) {
    facility=NULL;
  } else {
    facility=IDL_VarGetString(argv[1]);
  }
  dsc_name=IDL_VarGetString(argv[2]);
  dsc_level=IDL_VarGetString(argv[3]);
  IDL_ENSURE_SCALAR(argv[4]);

  if (argv[4]->type == IDL_TYP_STRING) {
    if (!strcmp("DSR_INPUT", IDL_STRING_STR(&argv[4]->value.str))) {
      role=DSR_INPUT;
    } else if (!strcmp("DSR_OUTPUT", IDL_STRING_STR(&argv[4]->value.str))) {
      role=DSR_OUTPUT;
    } else IDL_MessageFromBlock(msg_block, INVALID_ARGUMENT, IDL_MSG_LONGJMP,
        "ROLE");
  } else {
    tmp=IDL_CvtLng(1, argv+4);
    role=(DSRole) tmp->value.l;
    if (tmp != argv[4]) IDL_Deltmp(tmp);
  }

  id=dsproc_get_datastream_id(site, facility, dsc_name, dsc_level, role);

  return IDL_GettmpLong(id);
}
/** IDL front end to dsproc_get_input_datastream_id
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_get_input_datastream_id(int argc, IDL_VPTR argv[])
{
  int id;
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  IDL_ENSURE_STRING(argv[0]);
  IDL_ENSURE_STRING(argv[1]);
  IDL_ENSURE_SCALAR(argv[0]);
  IDL_ENSURE_SCALAR(argv[1]);

  id=dsproc_get_input_datastream_id(
      IDL_STRING_STR(&argv[0]->value.str),
      IDL_STRING_STR(&argv[1]->value.str));

  return IDL_GettmpLong(id);
}
/** IDL front end to dsproc_get_input_datastream_ids
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_get_input_datastream_ids(int argc, IDL_VPTR argv[])
{
  int res;
  int *ids, *i;
  IDL_VPTR tmp;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  res=dsproc_get_input_datastream_ids(&ids);
  if (res>0) {
    i=(int *) IDL_MakeTempVector(IDL_TYP_LONG, res, IDL_ARR_INI_NOP, &tmp);
    memcpy(i, ids, res*sizeof(int));
    free(ids);
    IDL_VarCopy(tmp, argv[0]);
  }

  return IDL_GettmpLong(res);
}
/** IDL front end to dsproc_get_output_datastream_id
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_get_output_datastream_id(int argc, IDL_VPTR argv[])
{
  int id;
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  IDL_ENSURE_STRING(argv[0]);
  IDL_ENSURE_STRING(argv[1]);
  IDL_ENSURE_SCALAR(argv[0]);
  IDL_ENSURE_SCALAR(argv[1]);

  id=dsproc_get_output_datastream_id(
      IDL_STRING_STR(&argv[0]->value.str),
      IDL_STRING_STR(&argv[1]->value.str));

  return IDL_GettmpLong(id);
}
/** IDL front end to dsproc_get_output_datastream_ids
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_get_output_datastream_ids(int argc, IDL_VPTR argv[])
{
  int res;
  int *ids, *i;
  IDL_VPTR tmp;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  res=dsproc_get_output_datastream_ids(&ids);
  if (res>0) {
    i=(int *) IDL_MakeTempVector(IDL_TYP_LONG, res, IDL_ARR_INI_NOP, &tmp);
    memcpy(i, ids, res*sizeof(int));
    free(ids);
    IDL_VarCopy(tmp, argv[0]);
  }

  return IDL_GettmpLong(res);
}
/** IDL front end to dsproc_find_datastream_files
 * 
 * @param argc - number of arguments
 * @param argv - array of pointers to IDL_VARIABLE structures
 */
IDL_VPTR idl_dsproc_find_datastream_files(int argc, IDL_VPTR argv[])
{
  int res, dsid, i;
  time_t begin_time, end_time;
  char **files;
  IDL_STRING *s;
  IDL_VPTR retval;
  /* prevent unused parameter compiler warning  */
  argc = argc;
  res = 0;

  dsid = IDL_LongScalar(argv[0]);
  begin_time = (time_t) IDL_Long64Scalar(argv[1]);
  end_time = (time_t) IDL_Long64Scalar(argv[2]);

  res=dsproc_find_datastream_files(dsid, begin_time, end_time, &files);
  if (res>0) {

    s=(IDL_STRING *) IDL_MakeTempVector(IDL_TYP_STRING, res, IDL_ARR_INI_NOP, &retval);
    for (i=0; i < res; i++) {
      IDL_StrStore(s++, files[i]);
    }
    return retval;
  }
  else if (res<0) {
    return _GettmpNull();
  }
  else return  IDL_GettmpLong(0);

}
/** IDL front end to dsproc_get_datastream_files
 * 
 * @param argc - number of arguments
 * @param argv - array of pointers to IDL_VARIABLE structures
 */
IDL_VPTR idl_dsproc_get_datastream_files(int argc, IDL_VPTR argv[])
{
  int res, dsid, i;
  char **files;
  IDL_STRING *file_list;
  IDL_VPTR tmp_files;
  /* prevent unused parameter compiler warning  */
  argc = argc;
  res = 0;
    
  dsid = IDL_LongScalar(argv[0]);
  res=dsproc_get_datastream_files(dsid, &files); 
  if (res>0) {

    file_list=(IDL_STRING *) IDL_MakeTempVector(IDL_TYP_STRING, res, IDL_ARR_INI_NOP, &tmp_files); 
    for (i=0; i < res; i++) {
      IDL_StrStore(file_list++, files[i]); 
    } 
     IDL_VarCopy(tmp_files, argv[1]); 
  }

  return IDL_GettmpLong(res);
}
/** IDL front end to dsproc_set_datastream_path
 * 
 * @param argc - number of arguments
 * @param argv - array of pointers to IDL_VARIABLE structures
 */
IDL_VPTR idl_dsproc_set_datastream_path(int argc, IDL_VPTR argv[])
{
  int res, dsid;
  const char *path;
  /* prevent unused parameter compiler warning  */
  argc = argc;
  res = 0;

  dsid = IDL_LongScalar(argv[0]);
  path=IDL_VarGetString(argv[1]);

  res=dsproc_set_datastream_path(dsid, path);

  return IDL_GettmpLong(res);

}
/* IDL front end to dsproc_unset_datastream_flags
 * 
 * @param argc - number of arguments
 * @param argv - array of pointers to IDL_VARIABLE structures
 */
IDL_VPTR idl_dsproc_unset_datastream_flags(int argc, IDL_VPTR argv[])
{
  int ds_id, flag;
  const char *key;
  void *res;

  /* prevent unused parameter compiler warning  */
  argc = argc;

  ds_id=IDL_LongScalar(argv[0]);
  key=IDL_VarGetString(argv[1]);

  if (!strcmp(key, "DS_STANDARD_QC"))
    flag = DS_STANDARD_QC;
  if (!strcmp(key, "DS_FILTER_NANS"))
    flag = DS_FILTER_NANS;
  if (!strcmp(key, "DS_OVERLAP_CHECK"))
    flag = DS_OVERLAP_CHECK;
  if (!strcmp(key, "DS_PRESERVE_OBS"))
    flag = DS_PRESERVE_OBS;
  if (!strcmp(key, "DS_DISABLE_MERGE"))
    flag = DS_DISABLE_MERGE;
  if (!strcmp(key, "DS_SKIP_TRANSFORM"))
    flag = DS_SKIP_TRANSFORM;
  if (!strcmp(key, "DS_OBS_LOOP"))
    flag = DS_OBS_LOOP;
  if (!strcmp(key, "DS_SCAN_MODE"))
    flag = DS_SCAN_MODE;
  
  res = _GettmpNull();
  dsproc_unset_datastream_flags(ds_id, flag);

  return(res);

}
/** IDL front end to dsproc_datastream_name
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_datastream_name(int argc, IDL_VPTR argv[])
{
  IDL_VPTR tmp;
  const char *name;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  tmp=IDL_CvtLng(1, argv);
  name=dsproc_datastream_name(tmp->value.l);
  if (tmp != argv[0]) IDL_Deltmp(tmp);
  if (name == NULL) {
    tmp=IDL_Gettmp();
    tmp->value.l64=0;
    tmp->flags |= IDL_V_NULL;
    return tmp;
  }
  return IDL_StrToSTRING(name);
}
/** IDL front end to dsproc_datastream_path
 *   *
 *     * @param argc - number of arguments
 *       * @param argv - array of pointers to IDL_VARIABLE structures
 *         */
IDL_VPTR idl_dsproc_datastream_path(int argc, IDL_VPTR argv[])
{
  IDL_VPTR tmp;
  const char *path;
  /* prevent unused parameter compiler warning  */
  argc = argc;

  IDL_ENSURE_SCALAR(argv[0]);
  tmp=IDL_CvtLng(1, argv);
  path=dsproc_datastream_path(tmp->value.l);
  if (tmp != argv[0]) IDL_Deltmp(tmp);
  if (path == NULL) {
    tmp=IDL_Gettmp();
    tmp->value.l64=0;
    tmp->flags |= IDL_V_NULL;
    return tmp;
  }
  return IDL_StrToSTRING(path);
}
/** IDL front end to dsproc_datastream_site
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_datastream_site(int argc, IDL_VPTR argv[])
{
  IDL_VPTR tmp;
  const char *site;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  tmp=IDL_CvtLng(1, argv);
  site=dsproc_datastream_site(tmp->value.l);
  if (tmp != argv[0]) IDL_Deltmp(tmp);
  if (site == NULL) {
    tmp=IDL_Gettmp();
    tmp->value.l64=0;
    tmp->flags |= IDL_V_NULL;
    return tmp;
  }
  return IDL_StrToSTRING(site);
}
/** IDL front end to dsproc_datastream_facility
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_datastream_facility(int argc, IDL_VPTR argv[])
{
  IDL_VPTR tmp;
  const char *facility;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  tmp=IDL_CvtLng(1, argv);
  facility=dsproc_datastream_facility(tmp->value.l);
  if (tmp != argv[0]) IDL_Deltmp(tmp);
  if (facility == NULL) {
    tmp=IDL_Gettmp();
    tmp->value.l64=0;
    tmp->flags |= IDL_V_NULL;
    return tmp;
  }
  return IDL_StrToSTRING(facility);
}
/** IDL front end to dsproc_datastream_class_name
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_datastream_class_name(int argc, IDL_VPTR argv[])
{
  IDL_VPTR tmp;
  const char *class_name;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  tmp=IDL_CvtLng(1, argv);
  class_name=dsproc_datastream_class_name(tmp->value.l);
  if (tmp != argv[0]) IDL_Deltmp(tmp);
  if (class_name == NULL) {
    tmp=IDL_Gettmp();
    tmp->value.l64=0;
    tmp->flags |= IDL_V_NULL;
    return tmp;
  }
  return IDL_StrToSTRING(class_name);
}
/** IDL front end to dsproc_datastream_class_level
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_datastream_class_level(int argc, IDL_VPTR argv[])
{
  IDL_VPTR tmp;
  const char *class_level;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  tmp=IDL_CvtLng(1, argv);
  class_level=dsproc_datastream_class_level(tmp->value.l);
  if (tmp != argv[0]) IDL_Deltmp(tmp);
  if (class_level == NULL) {
    tmp=IDL_Gettmp();
    tmp->value.l64=0;
    tmp->flags |= IDL_V_NULL;
    return tmp;
  }
  return IDL_StrToSTRING(class_level);
}
/** IDL front end to dsproc_dataset_name
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_dataset_name(int argc, IDL_VPTR argv[])
{
  const char *name;
  CDSGroup *dataset;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  dataset=(CDSGroup *) IDL_MEMINTScalar(argv[0]);
  name=dsproc_dataset_name(dataset);
  if (name == NULL) _GettmpNull();
  return IDL_StrToSTRING(name);
}
/** IDL front end to dsproc_dump_dataset
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_dump_dataset(int argc, IDL_VPTR argv[])
{
  int res=0;
  CDSGroup *dataset;
  const char *outdir, *prefix, *suffix;
  time_t file_time;
  int flags;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  dataset = (CDSGroup *) IDL_MEMINTScalar(argv[0]);
  outdir = IDL_VarGetString(argv[1]);
  if (IS_UNDEF(argv[2])) {
    prefix = NULL;
  } else {
    prefix = IDL_VarGetString(argv[2]);
  }
  file_time = (time_t) IDL_Long64Scalar(argv[3]);
  suffix = IDL_VarGetString(argv[4]);
  flags = IDL_LongScalar(argv[5]);

  res = dsproc_dump_dataset(dataset, outdir, prefix, file_time, suffix, flags);
  return IDL_GettmpLong(res);
}
/** IDL front end to cds_delete_group
  * 
  * KLG:   I am intentionally not making this function 
   *       available to documentation
  */
IDL_VPTR idl_cds_delete_group(int argc, IDL_VPTR argv[])
{
  CDSGroup *dataset;
  /* prevent unused parameter compiler warning  */
  argc = argc;

  dataset = (CDSGroup *) IDL_MEMINTScalar(argv[0]);

  return IDL_GettmpLong( cds_delete_group(dataset) );
}
/** IDL front end to cds_trim_unlim_dim
  * 
  * KLG:   I am intentionally not making this function 
   *       available to documentation
  */
IDL_VPTR idl_cds_trim_unlim_dim(int argc, IDL_VPTR argv[])
{
  CDSGroup *dataset;
  const char *unlim_dim_name;
  size_t length;

  /* prevent unused parameter compiler warning  */
  argc = argc;

  if (IS_UNDEF(argv[0])) {
    dataset=NULL;
  } else {
    dataset=(CDSGroup *) IDL_MEMINTScalar(argv[0]);
  }

  if (IS_UNDEF(argv[1])) {
    unlim_dim_name=NULL;
  } else {
    unlim_dim_name=IDL_VarGetString(argv[1]);
  }

  length = (size_t) IDL_MEMINTScalar(argv[2]);

  cds_trim_unlim_dim(dataset, unlim_dim_name, length);

  return _GettmpNull();

}
/** IDL front end to dsproc_dump_output_datasets
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_dump_output_datasets(int argc, IDL_VPTR argv[])
{
  int res=0;
  const char *outdir, *suffix;
  IDL_VPTR tmp;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  outdir=IDL_VarGetString(argv[0]);
  suffix=IDL_VarGetString(argv[1]);
  IDL_ENSURE_SCALAR(argv[2]);
  tmp=IDL_CvtLng(1, argv+2);
  if (tmp != argv[2]) IDL_Deltmp(tmp);
  
  res=dsproc_dump_output_datasets(outdir, suffix, tmp->value.l);

  return IDL_GettmpLong(res);
}
/** IDL front end to dsproc_dump_retrieved_datasets
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_dump_retrieved_datasets(int argc, IDL_VPTR argv[])
{
  int res=0;
  const char *outdir, *suffix;
  int flags;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  outdir=IDL_VarGetString(argv[0]);
  suffix=IDL_VarGetString(argv[1]);
  flags = IDL_LongScalar(argv[2]);
  
  res=dsproc_dump_retrieved_datasets(outdir, suffix, flags);

  return IDL_GettmpLong(res);
}
/** IDL front end to dsproc_dump_transformed_datasets
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_dump_transformed_datasets(int argc, IDL_VPTR argv[])
{
  int res=0;
  const char *outdir, *suffix;
  int flags;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  outdir=IDL_VarGetString(argv[0]);
  suffix=IDL_VarGetString(argv[1]);
  flags = IDL_LongScalar(argv[2]);
  
  res=dsproc_dump_transformed_datasets(outdir, suffix, flags);

  return IDL_GettmpLong(res);
}
/** IDL front end to dsproc_db_disconnect
  * Example:
  *   IDL> dsproc_db_disconnect
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
void idl_dsproc_db_disconnect(int argc, IDL_VPTR argv[])
{
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  argv = argv; 
  dsproc_db_disconnect();
}

IDL_VPTR idl_dsproc_get_debug_level(int argc, IDL_VPTR argv[])
{
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  argv = argv; 
  return IDL_GettmpLong(dsproc_get_debug_level());
}
IDL_VPTR idl_dsproc_get_output_var(int argc, IDL_VPTR argv[])
{
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  argv = argv; 
  const char *var_name;
  int ds_id, obs_index;
  IDL_VPTR tmp;

  IDL_ENSURE_SCALAR(argv[0]);
  tmp=IDL_CvtLng(1, argv);
  ds_id=tmp->value.l;
  if (tmp != argv[0]) IDL_Deltmp(tmp);

  var_name=IDL_VarGetString(argv[1]);

  IDL_ENSURE_SCALAR(argv[2]);
  tmp=IDL_CvtLng(1, argv+2);
  obs_index=tmp->value.l;
  if (tmp != argv[2]) IDL_Deltmp(tmp);

  return IDL_GettmpMEMINT( (IDL_MEMINT)
      dsproc_get_output_var(ds_id, var_name, obs_index));
}

IDL_VPTR idl_dsproc_get_qc_var(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  var=(CDSVar *) argv[0]->value.memint;

  return IDL_GettmpMEMINT( (IDL_MEMINT)
      dsproc_get_qc_var(var) );
}

IDL_VPTR idl_dsproc_get_time_var(int argc, IDL_VPTR argv[])
{
  void *cds_object;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  cds_object=(void *) argv[0]->value.memint;

  return IDL_GettmpMEMINT( (IDL_MEMINT)
      dsproc_get_time_var(cds_object) );
}

IDL_VPTR idl_dsproc_var_name(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  var=(CDSVar *) argv[0]->value.memint;

  return IDL_StrToSTRING( dsproc_var_name(var) );
}

IDL_VPTR idl_dsproc_get_source_var_name(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  /* prevent unused parameter compiler warning  */
  argc = argc;

  IDL_ENSURE_SCALAR(argv[0]);
  var=(CDSVar *) argv[0]->value.memint;

  return IDL_StrToSTRING( dsproc_get_source_var_name(var) );
}

IDL_VPTR idl_dsproc_get_source_ds_name(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  /* prevent unused parameter compiler warning  */
  argc = argc;

  IDL_ENSURE_SCALAR(argv[0]);
  var=(CDSVar *) argv[0]->value.memint;

  return IDL_StrToSTRING( dsproc_get_source_ds_name(var) );
}

IDL_VPTR idl_dsproc_get_source_ds_id(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  /* prevent unused parameter compiler warning  */
  argc = argc;

  IDL_ENSURE_SCALAR(argv[0]);
  var=(CDSVar *) argv[0]->value.memint;

  return IDL_GettmpLong( dsproc_get_source_ds_id(var) );
}

IDL_VPTR idl_dsproc_var_sample_count(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  var=(CDSVar *) argv[0]->value.memint;

  return IDL_GettmpMEMINT( (IDL_MEMINT)
      dsproc_var_sample_count(var) );
}

IDL_VPTR idl_dsproc_var_sample_size(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  var=(CDSVar *) argv[0]->value.memint;

  return IDL_GettmpMEMINT( (IDL_MEMINT)
      dsproc_var_sample_size(var) );
}

IDL_VPTR idl_cds_var_data(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  IDL_VPTR ret;
  int idl_type, i, j;
  IDL_MEMINT dim[] = {1,1,1,1, 1,1,1,1};
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  var=(CDSVar *) argv[0]->value.memint;

  idl_type=_cds_to_idl_datatype(var->type);
  if (idl_type == IDL_TYP_STRING) {
    // string is a special case
    ret = IDL_StrToSTRING((char *) var->data.cp);
    if (var->ndims > 1) {
    // Array
      j=var->ndims-1;
      dim[j--]=var->sample_count;
      for (i=1; i < var->ndims; i++) {
        dim[j--]=var->dims[i]->length;
      }
      ret=IDL_ImportArray(var->ndims, dim, IDL_TYP_BYTE, var->data.cp, NULL, NULL);
    } else {
      // Scalar
      ret = IDL_StrToSTRING(var->data.cp);
    }
  } else if (var->ndims == 0 || idl_type == IDL_TYP_UNDEF) {
    // Scalar
    ret=IDL_Gettmp();
    ret->type=idl_type;
    if (ret->type == IDL_TYP_UNDEF) {
      ret->flags |= IDL_V_NULL;
    } else {
      memcpy(&ret->value.c, var->data.vp, IDL_TypeSizeFunc(ret->type));
    }
  } else {
    // Array
    j=var->ndims-1;
    dim[j--]=var->sample_count;
    for (i=1; i < var->ndims; i++) {
      dim[j--]=var->dims[i]->length;
    }

    ret=IDL_ImportArray(var->ndims, dim, idl_type, var->data.vp, NULL, NULL);
  }
  
  return ret;
}

IDL_VPTR idl_cds_var_type(int argc, IDL_VPTR argv[], char *argk)
{
  CDSVar *var;
  IDL_VPTR ret;
  int idl_type;
  typedef struct {
    IDL_KW_RESULT_FIRST_FIELD;
    int name;
  } KW_RESULT;
  static IDL_KW_PAR kw_pars[] = {
    { "NAME", IDL_TYP_LONG, 1, IDL_KW_ZERO, NULL, IDL_KW_OFFSETOF(name) },
    { NULL },
  };
  KW_RESULT kw;

  IDL_KWProcessByOffset(argc, argv, argk, kw_pars,
      (IDL_VPTR *) 0, 1, &kw);

  IDL_ENSURE_SCALAR(argv[0]);
  var=(CDSVar *) argv[0]->value.memint;

  idl_type=_cds_to_idl_datatype(var->type);
  if (kw.name) 
    ret=IDL_StrToSTRING(IDL_TypeNameFunc(idl_type));
  else
    ret=IDL_GettmpLong(idl_type);
  return ret;
}

IDL_VPTR idl_cds_var_dims(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  IDL_VPTR ret;
  int j, i;
  IDL_MEMINT *dim;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  var=(CDSVar *) argv[0]->value.memint;

  if (var->ndims == 0) {
    // Scalar
    ret=IDL_GettmpLong(0);
  } else {
    // Array
    dim=(IDL_MEMINT *) IDL_MakeTempVector(IDL_TYP_MEMINT, var->ndims,
        IDL_ARR_INI_NOP, &ret);
    j=var->ndims-1;
    dim[j--]=var->sample_count;
    for (i=1; i < var->ndims; i++) {
      dim[j--]=var->dims[i]->length;
    }
  }
  
  return ret;
}

IDL_VPTR idl_dsproc_var_dim_names(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  IDL_MEMINT i;
  IDL_STRING *s;
  IDL_VPTR retval;
  /* prevent unused parameter compiler warning  */
  argc = argc;

  var = (CDSVar *) IDL_MEMINTScalar(argv[0]);

  if (var->ndims == 0) return _GettmpNull();

  s = (IDL_STRING *) IDL_MakeTempVector(IDL_TYP_STRING, var->ndims,
      IDL_ARR_INI_NOP, &retval);

  for (i=0; i < var->ndims; i++) {
    IDL_StrStore(s++, var->dims[i]->name);
  }

  return retval;
}


IDL_VPTR idl_dsproc_get_var_missing_values(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  IDL_VPTR tmp;
  int idl_type, n;
  void *values, *idl_val;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  var=(CDSVar *) argv[0]->value.memint;

  n=dsproc_get_var_missing_values(var, &values);
  if (n > 0) {
    // make an IDL array to hold the missing values
    idl_type=_cds_to_idl_datatype(var->type);
    // don't support string, use byte
    if (idl_type == IDL_TYP_STRING) idl_type = IDL_TYP_BYTE;
    idl_val=IDL_MakeTempVector(idl_type, n, IDL_ARR_INI_NOP, &tmp);
    // copy the missing values
    memcpy(idl_val, values, tmp->value.arr->arr_len);
    // free up the missing values, works because of the shared LIBC
    free(values);
    // Replace the second argument with "tmp", output argument.
    IDL_VarCopy(tmp, argv[1]);
  }
  return IDL_GettmpLong(n);
}

IDL_VPTR idl_dsproc_init_var_data(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  IDL_VPTR ret;
  void *res;
  IDL_MEMINT sample_start, sample_count;
  char use_missing;

  // This is typically not exposed directly
  // to the IDL user i.e. *self.p should
  // always have the right type (cdsvar__define.pro)
  var=(CDSVar *) IDL_MEMINTScalar(argv[0]);

  // Type conversion is needed in case the user passed
  // a byte or 16-bit int instead of a memint.
  sample_start = IDL_MEMINTScalar(argv[1]);

  sample_count = IDL_MEMINTScalar(argv[2]);

  if (argc < 4) {
    // if only 3 args are passed, then call alloc.
    res=dsproc_alloc_var_data(var, sample_start, sample_count);
  } else {
    // when use_missing is passed in, call init.
    use_missing = (char) IDL_LongScalar(argv[3]);
    res=dsproc_init_var_data(var, sample_start, sample_count, use_missing);
  }

  if (!res) {
    // Return !null to the IDL user
    ret=_GettmpNull();
  } else {
    // Calling idl_cds_var_data sets up the IDL variable structure
    ret=idl_cds_var_data(1, argv);
  }

  return ret;
}
IDL_VPTR idl_dsproc_set_var_data(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  IDL_VPTR tmp;
  void *res, *missing_values;
  IDL_MEMINT sample_start;
  int last;
  int nvals;
  IDL_STRING *s;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  var=(CDSVar *) argv[0]->value.memint;

  IDL_ENSURE_SCALAR(argv[1]);
  tmp=IDL_CvtMEMINT(1, argv+1);
  sample_start = tmp->value.memint;
  if (tmp != argv[1]) IDL_Deltmp(tmp);

  // For simplicity only accept arrays
  // IDL wrapper code can promote scalars to 1-element arrays
  IDL_ENSURE_ARRAY(argv[2]);
  if (_cds_to_idl_datatype(var->type) != argv[2]->type) {
    IDL_MessageFromBlock(msg_block, TYPE_MISMATCH, IDL_MSG_LONGJMP,
        IDL_TypeNameFunc(argv[2]->type), _cds_to_idl_datatype(var->type));
  }
  if (dsproc_get_var_missing_values(var, &missing_values) <= 0)
    missing_values=NULL;

  // if the pointer is the same, considering sample_start, then
  // skip the call and avoid copying with src==dst.
  // Returning a non-null indicates success, the return value
  // itself is not useful to the IDL user
  if ((char *) var->data.bp + sample_start * argv[2]->value.arr->elt_len
      == (char *) argv[2]->value.arr->data)
    return IDL_GettmpMEMINT((IDL_MEMINT) argv[2]->value.arr->data);

  last = argv[2]->value.arr->n_dim - 1;
  if (var->type == 1) {
    if (argv[2]->value.arr->n_dim == 0) {
      nvals=1;
    } 
    else {
      nvals=argv[2]->value.arr->dim[last];
    }
    s=(IDL_STRING *) argv[2]->value.arr->data;
    int i;
    for (i=0; i<nvals; i++) {
      res=dsproc_set_var_data(
        var,
        CDS_CHAR,
        sample_start+i,
        1,
        NULL,
        IDL_STRING_STR(s));
      s++;
    }
  }
  else {
    res=dsproc_set_var_data(
        var,
        var->type,
        sample_start,
        argv[2]->value.arr->dim[last],
        missing_values,
        argv[2]->value.arr->data);
  }
  return IDL_GettmpMEMINT((IDL_MEMINT) res);
}

IDL_VPTR idl_dsproc_get_output_dataset(int argc, IDL_VPTR argv[])
{
  int ds_id, obs_index;
  CDSGroup *res;
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  argv = argv;

  ds_id=IDL_LongScalar(argv[0]);

  obs_index=IDL_LongScalar(argv[1]);

  res=dsproc_get_output_dataset(ds_id, obs_index);
  return IDL_GettmpMEMINT((IDL_MEMINT) res);
}

IDL_VPTR idl_dsproc_get_retrieved_dataset(int argc, IDL_VPTR argv[])
{
  int ds_id, obs_index;
  CDSGroup *res;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  ds_id=IDL_LongScalar(argv[0]);

  obs_index=IDL_LongScalar(argv[1]);

  res=dsproc_get_retrieved_dataset(ds_id, obs_index);
  return IDL_GettmpMEMINT((IDL_MEMINT) res);
}

IDL_VPTR idl_dsproc_get_transformed_dataset(int argc, IDL_VPTR argv[])
{
  int ds_id, obs_index;
  CDSGroup *res;
  const char *coordsys_name;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  if (IS_UNDEF(argv[0])) {
    coordsys_name = NULL;
  } else {
    coordsys_name=IDL_VarGetString(argv[0]);
  }

  ds_id=IDL_LongScalar(argv[1]);

  obs_index=IDL_LongScalar(argv[2]);

  res=dsproc_get_transformed_dataset(coordsys_name, ds_id, obs_index);
  return IDL_GettmpMEMINT((IDL_MEMINT) res);
}

IDL_VPTR idl_dsproc_get_dim(int argc, IDL_VPTR argv[])
{
  CDSDim *res;
  CDSGroup *dataset;
  const char *name;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  dataset=(CDSGroup *) argv[0]->value.memint;
  
  name=IDL_VarGetString(argv[1]);

  res=dsproc_get_dim(dataset, name);
  return IDL_GettmpMEMINT((IDL_MEMINT) res);
}

IDL_VPTR idl_dsproc_get_dim_length(int argc, IDL_VPTR argv[])
{
  size_t res;
  CDSGroup *dataset;
  const char *name;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  dataset=(CDSGroup *) argv[0]->value.memint;
  
  name=IDL_VarGetString(argv[1]);

  res=dsproc_get_dim_length(dataset, name);
  return IDL_GettmpMEMINT((IDL_MEMINT) res);
}

IDL_VPTR idl_dsproc_set_dim_length(int argc, IDL_VPTR argv[])
{
  CDSGroup *dataset;
  const char *name;
  size_t length;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  dataset=(CDSGroup *) argv[0]->value.memint;
  
  name=IDL_VarGetString(argv[1]);

  length=IDL_MEMINTScalar(argv[2]);

  return IDL_GettmpLong( dsproc_set_dim_length(dataset, name, length) );
}

IDL_VPTR idl_dsproc_change_att(int argc, IDL_VPTR argv[])
{
  int res=0, overwrite;
  void *parent;
  const char *name;
  CDSDataType type;
  size_t length;
  void *value;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  parent=(void *) argv[0]->value.memint;

  overwrite=IDL_LongScalar(argv[1]);

  name=IDL_VarGetString(argv[2]);

  type=_idl_to_cds_datatype(argv[3]->type);
  if (type == CDS_NAT)
    IDL_MessageFromBlock(msg_block, CDS_TYPE_UNDEF, IDL_MSG_LONGJMP, "CDS_NAT");

  if (argv[3]->type == IDL_TYP_STRING) {
    value = IDL_VarGetString(argv[3]);
    length = strlen(value);
  } else {
    IDL_VarGetData(argv[3], (IDL_MEMINT *) &length, (char **) &value, 0);
  }

  res=dsproc_change_att(parent, overwrite, name, type, length, value);

  return IDL_GettmpLong(res);
}

IDL_VPTR idl_dsproc_get_att(int argc, IDL_VPTR argv[])
{
  void *parent;
  const char *name;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  parent = (void *) IDL_MEMINTScalar(argv[0]);
  name = IDL_VarGetString(argv[1]);

  return IDL_GettmpMEMINT( (IDL_MEMINT) dsproc_get_att(parent, name) );
}

IDL_VPTR idl_dsproc_get_att_text(int argc, IDL_VPTR argv[])
{
  void *parent;
  const char *name;
  size_t length;
  char *res;
  IDL_VPTR retval;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  parent = (void *) IDL_MEMINTScalar(argv[0]);
  name = IDL_VarGetString(argv[1]);

  res = dsproc_get_att_text(parent, name, &length, NULL);
  if (res) {
    retval = IDL_StrToSTRING(res);
    free(res);
  } else {
    retval = IDL_GettmpLong(0);
  }
  return retval;
}

IDL_VPTR idl_dsproc_get_att_value(int argc, IDL_VPTR argv[])
{
  void *parent;
  const char *name;
  int idl_type;
  size_t length;
  void *data;
  char *dest;
  CDSAtt *att;
  IDL_VPTR retval;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  parent = (void *) IDL_MEMINTScalar(argv[0]);
  name = IDL_VarGetString(argv[1]);

  att = dsproc_get_att(parent, name);
  if (!att) return _GettmpNull();

  data = dsproc_get_att_value(parent, name, att->type, &length, NULL);
  // copy again, assuming this is small so performance is not critical
  // this avoids having to use a free callback
  idl_type = _cds_to_idl_datatype(att->type);
  if (idl_type == IDL_TYP_STRING) {
    // special case for string
    retval = IDL_StrToSTRING((char *) data);
  } else {

    if (length == 0) return _GettmpNull();

    dest = IDL_MakeTempVector(idl_type,
        (IDL_MEMINT) length,
        IDL_ARR_INI_NOP, &retval);
    memcpy(dest, data, length*IDL_TypeSizeFunc(idl_type));
  }
  free(data);

  return retval;
}

IDL_VPTR idl_dsproc_set_att(int argc, IDL_VPTR argv[])
{
  void *parent;
  int overwrite;
  const char *name;
  char *value;
  int res;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  parent = (void *) IDL_MEMINTScalar(argv[0]);
  overwrite = IDL_LongScalar(argv[1]);
  name = IDL_VarGetString(argv[2]);
  
  if (argv[3]->type == IDL_TYP_STRING) {
    value = IDL_VarGetString(argv[3]);
    res = dsproc_set_att(parent, overwrite, name, CDS_CHAR, 
        strlen(value), value);
  } else {
    IDL_ENSURE_ARRAY(argv[3]);
    res = dsproc_set_att(parent, overwrite, name,
        _idl_to_cds_datatype(argv[3]->type),
        (size_t) argv[3]->value.arr->n_elts,
        argv[3]->value.arr->data);
  }

  return IDL_GettmpLong(res);
}

IDL_VPTR idl_dsproc_set_att_text(int argc, IDL_VPTR argv[])
{
  void *parent;
  const char *name, *str;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  parent = (void *) IDL_MEMINTScalar(argv[0]);
  name = IDL_VarGetString(argv[1]);
  str = IDL_VarGetString(argv[2]);
  return IDL_GettmpLong( dsproc_set_att_text(parent, name, "%s", str) );
}

IDL_VPTR idl_dsproc_set_att_value(int argc, IDL_VPTR argv[])
{
  void *parent;
  const char *name, *value;
  int res;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  parent = (void *) IDL_MEMINTScalar(argv[0]);
  name = IDL_VarGetString(argv[1]);
  if (argv[2]->type == IDL_TYP_STRING) {
    value = IDL_VarGetString(argv[2]);
    res = dsproc_set_att_value(parent, name, CDS_CHAR, 
        strlen(value), (void *)value);
  } else {
    IDL_ENSURE_ARRAY(argv[2]);
    res = dsproc_set_att_value(parent, name,
        _idl_to_cds_datatype(argv[2]->type),
        (size_t) argv[2]->value.arr->n_elts,
        argv[2]->value.arr->data);
  }

  return IDL_GettmpLong(res);
}

IDL_VPTR idl_dsproc_get_retrieved_var(int argc, IDL_VPTR argv[])
{
  const char *var_name;
  int obs_index;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  var_name = IDL_VarGetString(argv[0]);
  obs_index = IDL_LongScalar(argv[1]);

  return IDL_GettmpMEMINT( (IDL_MEMINT)
      dsproc_get_retrieved_var(var_name, obs_index) );
}

IDL_VPTR idl_dsproc_get_transformed_var(int argc, IDL_VPTR argv[])
{
  const char *var_name;
  int obs_index=0;

  var_name = IDL_VarGetString(argv[0]);
  if (argc > 1) obs_index = IDL_LongScalar(argv[1]);

  return IDL_GettmpMEMINT( (IDL_MEMINT)
      dsproc_get_transformed_var(var_name, obs_index) );
}

IDL_VPTR idl_dsproc_clone_var(int argc, IDL_VPTR argv[])
{
  CDSVar *src_var, *res;
  CDSGroup *dataset;
  const char *var_name;
  CDSDataType data_type;
  const char **dim_names;
  int copy_data;
  IDL_STRING *s;
  IDL_MEMINT i;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  IDL_ENSURE_SCALAR(argv[0]);
  src_var=(CDSVar *) argv[0]->value.memint;

  if (IS_UNDEF(argv[1])) {
    dataset=NULL;
  } else {
    dataset=(CDSGroup *) IDL_MEMINTScalar(argv[1]);
  }

  if (IS_UNDEF(argv[2])) {
    var_name=NULL;
  } else {
    var_name=IDL_VarGetString(argv[2]);
  }

  if (IS_UNDEF(argv[3])) {
    data_type=CDS_NAT;
  } else {
    data_type=_idl_to_cds_datatype( IDL_LongScalar(argv[3]) );
  }
  
  copy_data=IDL_LongScalar(argv[5]);

  if (IS_UNDEF(argv[4])) {
    dim_names=NULL;
  } else {
    IDL_ENSURE_ARRAY(argv[4]);
    IDL_ENSURE_STRING(argv[4]);
    dim_names = malloc(argv[4]->value.arr->n_elts * sizeof(const char *));
    s=(IDL_STRING *) argv[4]->value.arr->data;
    for (i=0; i<argv[4]->value.arr->n_elts; i++) {
      dim_names[i]=IDL_STRING_STR(s++);
    }
  }

  res=dsproc_clone_var(src_var, dataset, var_name, data_type,
      dim_names, copy_data);
  
  if (dim_names) free(dim_names);

  return IDL_GettmpMEMINT( (IDL_MEMINT) res );
}

IDL_VPTR idl_dsproc_copy_var_tag(int argc, IDL_VPTR argv[])
{
  CDSVar *src_var, *dest_var;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  src_var = (CDSVar *) IDL_MEMINTScalar(argv[0]);
  dest_var = (CDSVar *) IDL_MEMINTScalar(argv[1]);

  return IDL_GettmpLong( dsproc_copy_var_tag(src_var, dest_var) );
}

IDL_VPTR idl_dsproc_flags(int argc, IDL_VPTR argv[])
{
  const char *key;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  key=IDL_VarGetString(argv[0]);

  if (!strcmp(key, "VAR_SKIP_TRANSFORM"))
    return IDL_GettmpLong( VAR_SKIP_TRANSFORM );
  if (!strcmp(key, "CDS_SKIP_GROUP_ATTS"))
    return IDL_GettmpLong( CDS_SKIP_GROUP_ATTS );
  if (!strcmp(key, "CDS_SKIP_VAR_ATTS"))
    return IDL_GettmpLong( CDS_SKIP_VAR_ATTS );
  if (!strcmp(key, "CDS_SKIP_DATA"))
    return IDL_GettmpLong( CDS_SKIP_DATA );
  if (!strcmp(key, "CDS_SKIP_SUBGROUPS"))
    return IDL_GettmpLong( CDS_SKIP_SUBGROUPS );

  return IDL_GettmpLong(0);
}

IDL_VPTR idl_dsproc_set_var_flags(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  int flags;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  // Using IDL_MEMINTScalar will perform all error checking
  // necessary and return a memint (memory pointer sized integer)
  var=(CDSVar *) IDL_MEMINTScalar(argv[0]);

  flags=IDL_LongScalar(argv[1]);

  return IDL_GettmpLong( dsproc_set_var_flags(var, flags) );
}

IDL_VPTR idl_dsproc_set_datastream_flags(int argc, IDL_VPTR argv[])
{
  int ds_id, flag;
  const char *key;
  void *res;
  /* prevent unused parameter compiler warning  */
  argc = argc;


  ds_id=IDL_LongScalar(argv[0]);
  key=IDL_VarGetString(argv[1]);

  if (!strcmp(key, "DS_STANDARD_QC"))
    flag = DS_STANDARD_QC;
  if (!strcmp(key, "DS_FILTER_NANS"))
    flag = DS_FILTER_NANS;
  if (!strcmp(key, "DS_OVERLAP_CHECK"))
    flag = DS_OVERLAP_CHECK;
  if (!strcmp(key, "DS_PRESERVE_OBS"))
    flag = DS_PRESERVE_OBS;
  if (!strcmp(key, "DS_DISABLE_MERGE"))
    flag = DS_DISABLE_MERGE;
  if (!strcmp(key, "DS_SKIP_TRANSFORM"))
    flag = DS_SKIP_TRANSFORM;
  if (!strcmp(key, "DS_OBS_LOOP"))
    flag = DS_OBS_LOOP;
  if (!strcmp(key, "DS_SCAN_MODE"))
    flag = DS_SCAN_MODE;

  res = _GettmpNull(); 
  dsproc_set_datastream_flags(ds_id, flag);

  return (res); 

}
IDL_VPTR idl_dsproc_add_datastream_file_patterns(int argc, IDL_VPTR argv[])
{
  /* prevent unused parameter compiler warning  */
  argc = argc;
  argv = argv;
  int res, ds_id, npatterns, ignore_case;
  const char **patterns;
  IDL_MEMINT i, n, *arr;
  IDL_STRING *s;

  ds_id=IDL_LongScalar(argv[0]);
  npatterns=IDL_LongScalar(argv[1]);
  ignore_case=IDL_LongScalar(argv[3]);

  /* Make sure patterns are a string */
  IDL_ENSURE_STRING(argv[2]);
  if (argv[2]->flags & IDL_V_ARR) {
    n = argv[2]->value.arr->n_elts;
    s = (IDL_STRING *) argv[2]->value.arr->data;
  } else {
    n=1;
    s = &argv[2]->value.str;
  }
  patterns = malloc((n+1) * sizeof(const char *));
  for (i=0; i<n; i++) {
    patterns[i] = IDL_STRING_STR(s);
    s++;
  }
  patterns[n] = NULL;

  res = dsproc_add_datastream_file_patterns(ds_id, npatterns, patterns, ignore_case);

  if (patterns) {
    for (i=0; patterns[i] ;i++) printf("%s\n", patterns[i]);
    free(patterns);
  }

  return IDL_GettmpLong(res);
}
/** IDL front end to dsproc_set_datastream_file_extension
 * 
 * @param argc - number of arguments
 * @param argv - array of pointers to IDL_VARIABLE structures
 */
IDL_VPTR idl_dsproc_set_datastream_file_extension(int argc, IDL_VPTR argv[])
{
  int ds_id;
  const char *extension;
  void *res;

  /* prevent unused parameter compiler warning  */
  argc = argc;

  ds_id=IDL_LongScalar(argv[0]);
  extension=IDL_VarGetString(argv[1]);

  dsproc_set_datastream_file_extension(ds_id, extension);

  return(res);

}
IDL_VPTR idl_dsproc_set_datastream_split_mode(int argc, IDL_VPTR argv[])
{
  int ds_id;
  SplitMode split_mode;
  IDL_VPTR tmp;
  double split_start, split_interval;

  /* prevent unused parameter compiler warning  */
  argc = argc;

  ds_id=IDL_LongScalar(argv[0]);
  split_start=IDL_DoubleScalar(argv[2]);
  split_interval=IDL_DoubleScalar(argv[3]);
  IDL_ENSURE_SCALAR(argv[1]);

  if (argv[1]->type == IDL_TYP_STRING) {
    if (!strcmp("SPLIT_ON_STORE", IDL_STRING_STR(&argv[1]->value.str))) {
      split_mode=SPLIT_ON_STORE;
    } else if (!strcmp("SPLIT_ON_HOURS", IDL_STRING_STR(&argv[1]->value.str))) {
      split_mode=SPLIT_ON_HOURS;
    } else if (!strcmp("SPLIT_ON_DAYS", IDL_STRING_STR(&argv[1]->value.str))) {
      split_mode=SPLIT_ON_DAYS;
    } else if (!strcmp("SPLIT_ON_MONTHS", IDL_STRING_STR(&argv[1]->value.str))) {
      split_mode=SPLIT_ON_MONTHS;
    } else IDL_MessageFromBlock(msg_block, INVALID_ARGUMENT, IDL_MSG_LONGJMP,
        "SPLIT_MODE");
  } else {
    tmp=IDL_CvtLng(1, argv+1);
    split_mode=(SplitMode) tmp->value.l;
    if (tmp != argv[1]) IDL_Deltmp(tmp);
  }

  dsproc_set_datastream_split_mode(ds_id, split_mode, split_start, split_interval);

  return _GettmpNull();


}

void idl_cds_print(int argc, IDL_VPTR argv[])
{
  CDSGroup *dataset;
  const char *file_name;
  int flags;
  FILE *fp;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  dataset=(CDSGroup *) IDL_MEMINTScalar(argv[0]);

  file_name=IDL_VarGetString(argv[1]);

  flags=IDL_LongScalar(argv[2]);

  fp=fopen(file_name, "w");
  cds_print(fp, dataset, flags);
  fclose(fp);
}

IDL_VPTR idl_cds_obj_parent(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  var=(CDSVar *) IDL_MEMINTScalar(argv[0]);

  return IDL_GettmpMEMINT( (IDL_MEMINT) var->parent );
}

IDL_VPTR idl_cds_obj_type(int argc, IDL_VPTR argv[])
{
  CDSObject *obj;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  obj=(CDSObject *) IDL_MEMINTScalar(argv[0]);

  return IDL_GettmpLong( (int) obj->obj_type );
}

void idl_dsproc_error(int argc, IDL_VPTR argv[])
{
  const char *func, *file, *status, *msg;
  int line;

  func=IDL_VarGetString(argv[0]);
  file=IDL_VarGetString(argv[1]);
  line=IDL_LongScalar(argv[2]);
  status=IDL_VarGetString(argv[3]);
  if (argc == 4) {
    dsproc_error(func, file, line, status, NULL);
  } else {
    // 5 args
    msg=IDL_VarGetString(argv[4]);
    dsproc_error(func, file, line, status, "%s\n", msg);
  }
}

void idl_dsproc_abort(int argc, IDL_VPTR argv[])
{
  const char *func, *file, *status, *msg;
  int line;

  func=IDL_VarGetString(argv[0]);
  file=IDL_VarGetString(argv[1]);
  line=IDL_LongScalar(argv[2]);
  status=IDL_VarGetString(argv[3]);
  if (argc == 4) {
    dsproc_abort(func, file, line, status, NULL);
  } else {
    // 5 args
    msg=IDL_VarGetString(argv[4]);
    dsproc_abort(func, file, line, status, "%s\n", msg);
  }
}

void idl_dsproc_bad_file_warning(int argc, IDL_VPTR argv[])
{
  const char *func, *src_file, *file_name, *msg;
  int src_line;

  func=IDL_VarGetString(argv[0]);
  src_file=IDL_VarGetString(argv[1]);
  src_line=IDL_LongScalar(argv[2]);
  file_name=IDL_VarGetString(argv[3]);
  if (argc == 4) {
    dsproc_bad_file_warning(func, src_file, src_line, file_name, NULL);
  } else {
    // 5 args
    msg=IDL_VarGetString(argv[4]);
    dsproc_bad_file_warning(func, src_file, src_line, file_name, "%s\n", msg);
  }
}

void idl_dsproc_bad_line_warning(int argc, IDL_VPTR argv[])
{
  const char *func, *src_file, *file_name, *msg;
  int src_line, line_num;

  func=IDL_VarGetString(argv[0]);
  src_file=IDL_VarGetString(argv[1]);
  src_line=IDL_LongScalar(argv[2]);
  file_name=IDL_VarGetString(argv[3]);
  line_num=IDL_LongScalar(argv[4]);
  if (argc == 5) {
    dsproc_bad_line_warning(func, src_file, src_line, file_name, line_num, NULL);
  } else {
    // 6 args
    msg=IDL_VarGetString(argv[5]);
    dsproc_bad_line_warning(func, src_file, src_line, file_name, line_num, "%s\n", msg);
  }
}

void idl_dsproc_set_status(int argc, IDL_VPTR argv[])
{
  const char *status;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  status=IDL_VarGetString(argv[0]);

  dsproc_set_status(status);
}

void idl_dsproc_debug(int argc, IDL_VPTR argv[])
{
  const char *func, *file, *msg;
  int line, level;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  func=IDL_VarGetString(argv[0]);
  file=IDL_VarGetString(argv[1]);
  line=IDL_LongScalar(argv[2]);
  level=IDL_LongScalar(argv[3]);
  msg=IDL_VarGetString(argv[4]);
  dsproc_debug(func, file, line, level, "%s\n", msg);
}

void idl_dsproc_delete_var_tag(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  var=(CDSVar *) IDL_MEMINTScalar(argv[0]);

  dsproc_delete_var_tag(var);
}

IDL_VPTR idl_cds_obj_name(int argc, IDL_VPTR argv[])
{
  CDSObject *obj;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  obj = (CDSObject *) IDL_MEMINTScalar(argv[0]);
  return IDL_StrToSTRING(obj->name);
}

IDL_VPTR idl_cds_att_names(int argc, IDL_VPTR argv[])
{
  CDSObject *obj; // either CDSVar or CDSGroup
  CDSVar *var;
  CDSGroup *group;
  IDL_MEMINT i;
  IDL_STRING *s;
  IDL_VPTR retval;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  obj = (CDSObject *) IDL_MEMINTScalar(argv[0]);
  if (obj->obj_type == CDS_VAR) {
    var = (CDSVar *) obj;

    if (var->natts == 0) return _GettmpNull();

    s = (IDL_STRING *) IDL_MakeTempVector(IDL_TYP_STRING, var->natts,
        IDL_ARR_INI_NOP, &retval);

    for (i=0; i < var->natts; i++) {
      IDL_StrStore(s++, var->atts[i]->name);
    }
  } else if (obj->obj_type == CDS_GROUP) {

    group = (CDSGroup *) obj;

    if (group->natts == 0) return _GettmpNull();

    s = (IDL_STRING *) IDL_MakeTempVector(IDL_TYP_STRING, group->natts,
        IDL_ARR_INI_NOP, &retval);

    for (i=0; i < group->natts; i++) {
      IDL_StrStore(s++, group->atts[i]->name);
    }
  } else retval = _GettmpNull();

  return retval;
}

IDL_VPTR idl_cds_var_names(int argc, IDL_VPTR argv[])
{
  CDSObject *obj;
  CDSGroup *group;
  IDL_MEMINT i;
  IDL_STRING *s;
  IDL_VPTR retval;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  obj = (CDSObject *) IDL_MEMINTScalar(argv[0]);
  if (obj->obj_type == CDS_GROUP) {

    group = (CDSGroup *) obj;

    if (group->nvars == 0) return _GettmpNull();

    s = (IDL_STRING *) IDL_MakeTempVector(IDL_TYP_STRING, group->nvars,
        IDL_ARR_INI_NOP, &retval);

    for (i=0; i < group->nvars; i++) {
      IDL_StrStore(s++, group->vars[i]->name);
    }
  } else retval = _GettmpNull();

  return retval;
}

IDL_VPTR idl_cds_group_names(int argc, IDL_VPTR argv[])
{
  CDSObject *obj;
  CDSGroup *group;
  IDL_MEMINT i;
  IDL_STRING *s;
  IDL_VPTR retval;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  obj = (CDSObject *) IDL_MEMINTScalar(argv[0]);
  if (obj->obj_type == CDS_GROUP) {

    group = (CDSGroup *) obj;

    if (group->ngroups == 0) return _GettmpNull();

    s = (IDL_STRING *) IDL_MakeTempVector(IDL_TYP_STRING, group->ngroups,
        IDL_ARR_INI_NOP, &retval);

    for (i=0; i < group->ngroups; i++) {
      IDL_StrStore(s++, group->groups[i]->name);
    }
  } else retval = _GettmpNull();

  return retval;
}

IDL_VPTR idl_dsproc_set_var_output_target(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  int ds_id;
  const char *var_name;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  var = (CDSVar *) IDL_MEMINTScalar(argv[0]);
  ds_id = IDL_LongScalar(argv[1]);
  var_name = IDL_VarGetString(argv[2]);

  return IDL_GettmpLong( dsproc_set_var_output_target(var, ds_id, var_name) );
}

IDL_VPTR idl_dsproc_get_var_output_targets(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  VarTarget **targets;
  int n, i;
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  IDL_STRUCT_TAG_DEF tags[] = {
    { "DS_ID", NULL, (void *) IDL_TYP_LONG, 0 },
    { "VAR_NAME", NULL, (void *) IDL_TYP_STRING, 0 },
    { 0 }
  };
  void *s;
  char *p;
  IDL_STRING *str;
  IDL_LONG *ds_id;
  int offs[2];
  IDL_VPTR retval;

  var = (CDSVar *) IDL_MEMINTScalar(argv[0]);

  n = dsproc_get_var_output_targets(var, &targets);
  if (n == 0) return _GettmpNull();

  // Set up an IDL structure array
  s = IDL_MakeStruct(0, tags);
  p = IDL_MakeTempStructVector(s, n, &retval, IDL_ARR_INI_NOP);
  
  // Get the byte offsets to the tags
  offs[0] = IDL_StructTagInfoByIndex(s, 0, IDL_MSG_LONGJMP, NULL);
  offs[1] = IDL_StructTagInfoByIndex(s, 1, IDL_MSG_LONGJMP, NULL);

  // Populate the IDL structure array
  for (i=0; i<n; i++) {
    ds_id=(IDL_LONG *) (p+offs[0]);
    str=(IDL_STRING *) (p+offs[1]);

    *ds_id = targets[i]->ds_id;
    IDL_StrStore(str, targets[i]->var_name);

    p += retval->value.s.arr->elt_len;
  }
  return retval;
}

void idl_dsproc_unset_var_flags(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  int flags;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  var = (CDSVar *) IDL_MEMINTScalar(argv[0]);
  flags = IDL_LongScalar(argv[1]);

  dsproc_unset_var_flags(var, flags);
}


IDL_VPTR idl_dsproc_set_var_coordsys_name(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  const char *coordsys_name;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  var = (CDSVar *) IDL_MEMINTScalar(argv[0]);
  coordsys_name = IDL_VarGetString(argv[1]);

  return IDL_GettmpLong( dsproc_set_var_coordsys_name(var, coordsys_name) );
}

void idl_dsproc_freeopts(int argc, IDL_VPTR argv[])
{
  /* prevent unused parameter compiler warning  */
  argc = argc;
  argv = argv;
  dsproc_freeopts();
}

IDL_VPTR idl_dsproc_getopt(int argc, IDL_VPTR argv[])
{
  char * option;
  const char * value;
  int res;
  IDL_VPTR temp;

  /* prevent unused parameter compiler warning  */
  argc = argc;

  option = IDL_VarGetString(argv[0]);
  value = IDL_VarGetString(argv[1]);
  res = dsproc_getopt(option, &value);
  temp = IDL_StrToSTRING(value);
  IDL_VarCopy(temp, argv[1]);
  
  return IDL_GettmpLong( res );
}

IDL_VPTR idl_dsproc_setopt(int argc, IDL_VPTR argv[])
{
  char * short_opt;
  char * long_opt;
  char * arg_name;
  char * opt_desc;
  char * dirty_short_opt;
  
  /* prevent unused parameter compiler warning  */
  argc = argc;

  /* Workaround to help get a character from a 1 character IDL string */
  if (IS_UNDEF(argv[0])) {
    short_opt = NULL;
  } else {
      dirty_short_opt = IDL_VarGetString(argv[0]);
      short_opt = dirty_short_opt[0];
  }

  if (IS_UNDEF(argv[1])) {
    long_opt = NULL;
  } else {
    long_opt = IDL_VarGetString(argv[1]);
  }

  if (IS_UNDEF(argv[2])) {
    arg_name = NULL;
  } else {
    arg_name = IDL_VarGetString(argv[2]);
  }

  if (IS_UNDEF(argv[3])) {
    opt_desc = NULL;
  } else {
    opt_desc = IDL_VarGetString(argv[3]);
  }

  return IDL_GettmpLong( dsproc_setopt(short_opt, long_opt, arg_name, opt_desc) );
}

IDL_VPTR idl_dsproc_add_var_output_target(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  int ds_id;
  const char *var_name;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  var = (CDSVar *) IDL_MEMINTScalar(argv[0]);
  ds_id = IDL_LongScalar(argv[1]);
  var_name = IDL_VarGetString(argv[2]);

  return IDL_GettmpLong( dsproc_add_var_output_target(var, ds_id, var_name) );
}

IDL_VPTR idl_dsproc_define_var(int argc, IDL_VPTR argv[])
{
  CDSGroup *dataset;
  const char *name;
  CDSDataType type;
  int ndims;
  const char **dim_names;
  const char *long_name, *standard_name, *units;
  void *valid_min, *valid_max, *missing_value, *fill_value;
  IDL_STRING *str;
  int i, idl_type;
  IDL_VPTR tmp_valid_min, tmp_valid_max, tmp_missing_value, tmp_fill_value;
  CDSVar *res;

  dataset = (CDSGroup *) IDL_MEMINTScalar(argv[0]);
  name = IDL_VarGetString(argv[1]);
  idl_type = IDL_LongScalar(argv[2]);
  type = _idl_to_cds_datatype( idl_type );
  IDL_ENSURE_ARRAY(argv[3]);
  ndims = argv[3]->value.arr->n_elts;
  dim_names = malloc(ndims * sizeof(const char *));
  str = (IDL_STRING *) argv[3]->value.arr->data;
  for (i=0; i<ndims; i++) {
    dim_names[i] = IDL_STRING_STR(str);
    str++;
  }
  if (argc < 5 || IS_UNDEF(argv[4])) {
    long_name = NULL;
  } else {
    long_name = IDL_VarGetString(argv[4]);
  }
  if (argc < 6 || IS_UNDEF(argv[5])) {
    standard_name = NULL;
  } else {
    standard_name = IDL_VarGetString(argv[5]);
  }
  if (argc < 7 || IS_UNDEF(argv[6])) {
    units = NULL;
  } else {
    units = IDL_VarGetString(argv[6]);
  }
  if (argc < 8 || IS_UNDEF(argv[7])) {
    valid_min = NULL;
    tmp_valid_min = NULL;
  } else {
    IDL_ENSURE_SCALAR(argv[7]);
    tmp_valid_min = IDL_BasicTypeConversion(1, argv+7, idl_type);
    valid_min = &tmp_valid_min->value.c;
  }
  if (argc < 9 || IS_UNDEF(argv[8])) {
    valid_max = NULL;
    tmp_valid_max = NULL;
  } else {
    IDL_ENSURE_SCALAR(argv[8]);
    tmp_valid_max = IDL_BasicTypeConversion(1, argv+8, idl_type);
    valid_max = &tmp_valid_max->value.c;
  }
  if (argc < 10 || IS_UNDEF(argv[9])) {
    missing_value = NULL;
    tmp_missing_value = NULL;
  } else {
    IDL_ENSURE_SCALAR(argv[9]);
    tmp_missing_value = IDL_BasicTypeConversion(1, argv+9, idl_type);
    missing_value = &tmp_missing_value->value.c;
  }
  if (argc < 11 || IS_UNDEF(argv[10])) {
    fill_value = NULL;
    tmp_fill_value = NULL;
  } else {
    IDL_ENSURE_SCALAR(argv[10]);
    tmp_fill_value = IDL_BasicTypeConversion(1, argv+10, idl_type);
    fill_value = &tmp_fill_value->value.c;
  }

  res = dsproc_define_var(dataset, name, type, ndims,
      dim_names, long_name, standard_name, units, valid_min,
      valid_max, missing_value, fill_value);

  if (tmp_valid_min && tmp_valid_min != argv[7])
    IDL_Deltmp(tmp_valid_min);
  if (tmp_valid_max && tmp_valid_max != argv[8])
    IDL_Deltmp(tmp_valid_max);
  if (tmp_missing_value && tmp_missing_value != argv[9])
    IDL_Deltmp(tmp_missing_value);
  if (tmp_fill_value && tmp_fill_value != argv[10])
    IDL_Deltmp(tmp_fill_value);

  return IDL_GettmpMEMINT( (IDL_MEMINT) res );
}

IDL_VPTR idl_cds_dim_names(int argc, IDL_VPTR argv[])
{
  CDSGroup *dataset;
  IDL_MEMINT i;
  IDL_STRING *s;
  IDL_VPTR retval;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  dataset = (CDSGroup *) IDL_MEMINTScalar(argv[0]);

  if (dataset->ndims == 0) return _GettmpNull();

  s = (IDL_STRING *) IDL_MakeTempVector(IDL_TYP_STRING, dataset->ndims,
      IDL_ARR_INI_NOP, &retval);

  for (i=0; i < dataset->ndims; i++) {
    IDL_StrStore(s++, dataset->dims[i]->name);
  }

  return retval;
}

IDL_VPTR idl_dsproc_delete_var(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  var = (CDSVar *) IDL_MEMINTScalar(argv[0]);

  return IDL_GettmpLong( dsproc_delete_var(var) );
}

IDL_VPTR idl_dsproc_get_coord_var(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  int dim_index;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  var = (CDSVar *) IDL_MEMINTScalar(argv[0]);
  dim_index = IDL_LongScalar(argv[1]);

  return IDL_GettmpMEMINT( (IDL_MEMINT) dsproc_get_coord_var(var, dim_index) );
}

IDL_VPTR idl_dsproc_get_dataset_vars(int argc, IDL_VPTR argv[])
{
  CDSGroup *dataset;
  const char **var_names;
  int required;
  CDSVar **vars, **qc_vars, **acq_vars, ***p_qc, ***p_acq;
  IDL_MEMINT i, n, *arr;
  IDL_STRING *s;
  IDL_VPTR tmp_vars, tmp_qc, tmp_acq;

  IDL_EXCLUDE_EXPR(argv[3]);
  dataset = (CDSGroup *) IDL_MEMINTScalar(argv[0]);
  // Allow !null for var_names
  if (IS_UNDEF(argv[1])) {
    var_names = NULL;
  } else {
    // if not !null, it must be string
    IDL_ENSURE_STRING(argv[1]);
    if (argv[1]->flags & IDL_V_ARR) {
      n = argv[1]->value.arr->n_elts;
      s = (IDL_STRING *) argv[1]->value.arr->data;
    } else {
      n=1;
      s = &argv[1]->value.str;
    }
    var_names = malloc((n+1) * sizeof(const char *));
    for (i=0; i<n; i++) {
      var_names[i] = IDL_STRING_STR(s);
      s++;
    }
    var_names[n] = NULL;
  }

  required = IDL_LongScalar(argv[2]);

  if (argc > 4) {
    // Calling VarCopy below, will exclude expressions
    // but we need to do this before the library call
    IDL_EXCLUDE_EXPR(argv[4]);
    p_qc = &qc_vars;
  } else p_qc = NULL;

  if (argc > 5) {
    IDL_EXCLUDE_EXPR(argv[5]);
    p_acq = &acq_vars;
  } else p_acq = NULL;

  n = dsproc_get_dataset_vars(dataset, var_names, required, &vars, p_qc, p_acq);

  if (var_names) {
    for (i=0; var_names[i] ;i++) printf("%s\n", var_names[i]);
    free(var_names);
  }
  if (n <= 0) return IDL_GettmpLong( n );
  
  arr = (IDL_MEMINT *)
    IDL_MakeTempVector(IDL_TYP_MEMINT, n, IDL_ARR_INI_NOP, &tmp_vars);
  memcpy(arr, vars, tmp_vars->value.arr->arr_len);
  IDL_VarCopy(tmp_vars, argv[3]);

  if (p_qc) {
    arr = (IDL_MEMINT *)
      IDL_MakeTempVector(IDL_TYP_MEMINT, n, IDL_ARR_INI_NOP, &tmp_qc);
    memcpy(arr, *p_qc, tmp_qc->value.arr->arr_len);
    IDL_VarCopy(tmp_qc, argv[4]);
  }

  if (p_acq) {
    arr = (IDL_MEMINT *)
      IDL_MakeTempVector(IDL_TYP_MEMINT, n, IDL_ARR_INI_NOP, &tmp_acq);
    memcpy(arr, *p_acq, tmp_acq->value.arr->arr_len);
    IDL_VarCopy(tmp_acq, argv[5]);
  }

  return IDL_GettmpLong( n );
}

IDL_VPTR idl_cds_def_lock(int argc, IDL_VPTR argv[])
{
  CDSObject *obj;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  obj = (CDSObject *) IDL_MEMINTScalar(argv[0]);

  return IDL_GettmpLong( obj->def_lock );
}

IDL_VPTR idl_cds_obj_path(int argc, IDL_VPTR argv[])
{
  CDSObject *obj;
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  obj = (CDSObject *) IDL_MEMINTScalar(argv[0]);

  return IDL_StrToSTRING(obj->obj_path);
}

IDL_VPTR idl_dsproc_get_var(int argc, IDL_VPTR argv[])
{
  CDSGroup *dataset;
  const char *name;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  dataset = (CDSGroup *) IDL_MEMINTScalar(argv[0]);
  name = IDL_VarGetString(argv[1]);

  return IDL_GettmpMEMINT( (IDL_MEMINT) dsproc_get_var(dataset, name) );
}

IDL_VPTR idl_cds_get_group(int argc, IDL_VPTR argv[])
{
  CDSGroup *dataset;
  const char *name;
  int i;
  CDSGroup *arr;
  IDL_VPTR res;

  dataset = (CDSGroup *) IDL_MEMINTScalar(argv[0]);
  if (dataset->ngroups == 0) return _GettmpNull();
  if (argc > 1) {
    name = IDL_VarGetString(argv[1]);
    for (i=0; i < dataset->ngroups; i++) {
      if (!strcmp(name, dataset->groups[i]->name)) {
        return IDL_GettmpMEMINT( (IDL_MEMINT) dataset->groups[i] );
      }
    }
    return _GettmpNull();
  }
  // If only one argument is passed in, return all
  arr = (CDSGroup *) IDL_MakeTempVector(IDL_TYP_MEMINT, dataset->ngroups,
      IDL_ARR_INI_NOP, &res);
  memcpy(arr, dataset->groups, dataset->ngroups * sizeof(CDSGroup *));
  return res;
}

IDL_VPTR idl_dsproc_get_metric_var(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  const char *metric;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  var = (CDSVar *) IDL_MEMINTScalar(argv[0]);
  metric = IDL_VarGetString(argv[1]);

  return IDL_GettmpMEMINT( (IDL_MEMINT) dsproc_get_metric_var(var, metric));
}

IDL_VPTR idl_dsproc_get_trans_coordsys_var(int argc, IDL_VPTR argv[])
{
  const char *coordsys_name, *var_name;
  int obs_index=0;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  coordsys_name = IDL_VarGetString(argv[0]);
  var_name = IDL_VarGetString(argv[1]);
  if (argc > 2) obs_index = IDL_LongScalar(argv[2]);

  return IDL_GettmpMEMINT( (IDL_MEMINT) 
      dsproc_get_trans_coordsys_var(coordsys_name, var_name, obs_index) );
}

IDL_VPTR idl_dsproc_get_base_time(int argc, IDL_VPTR argv[])
{
  void *cds_object;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  cds_object = (void *) IDL_MEMINTScalar(argv[0]);
  
  return IDL_GettmpLong64( (IDL_LONG64) dsproc_get_base_time(cds_object) );
}

void idl_dsproc_log(int argc, IDL_VPTR argv[])
{
  const char *func, *file, *str;
  int line;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  func = IDL_VarGetString(argv[0]);
  file = IDL_VarGetString(argv[1]);
  line = IDL_LongScalar(argv[2]);
  str = IDL_VarGetString(argv[3]);

  dsproc_log(func, file, line, "%s\n", str);
}
void idl_dsproc_mentor_mail(int argc, IDL_VPTR argv[])
{
  const char *func, *file, *str;
  int line;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  func = IDL_VarGetString(argv[0]);
  file = IDL_VarGetString(argv[1]);
  line = IDL_LongScalar(argv[2]);
  str = IDL_VarGetString(argv[3]);

  dsproc_mentor_mail(func, file, line, "%s\n", str);
}

void idl_dsproc_warning(int argc, IDL_VPTR argv[])
{
  const char *func, *file, *str;
  int line;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  func = IDL_VarGetString(argv[0]);
  file = IDL_VarGetString(argv[1]);
  line = IDL_LongScalar(argv[2]);
  str = IDL_VarGetString(argv[3]);

  dsproc_warning(func, file, line, "%s\n", str);
}

IDL_VPTR idl_dsproc_get_time_range(int argc, IDL_VPTR argv[])
{
  void *cds_object;
  timeval_t start_time, end_time;
  size_t res;
  IDL_VPTR tmp;
  int *arr;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  cds_object = (void *) IDL_MEMINTScalar(argv[0]);
  res = dsproc_get_time_range(cds_object, &start_time, &end_time);

  if (res) {
    // Use an array of 2 signed 32-bit integers to hold sec, usec
    arr = (int *) IDL_MakeTempVector(IDL_TYP_LONG, 2, IDL_ARR_INI_NOP, &tmp);
    arr[0] = start_time.tv_sec;
    arr[1] = start_time.tv_usec;
    IDL_VarCopy(tmp, argv[1]);
    arr = (int *) IDL_MakeTempVector(IDL_TYP_LONG, 2, IDL_ARR_INI_NOP, &tmp);
    arr[0] = end_time.tv_sec;
    arr[1] = end_time.tv_usec;
    IDL_VarCopy(tmp, argv[2]);
  }

  return IDL_GettmpMEMINT( (IDL_MEMINT) res );
}

IDL_VPTR idl_dsproc_get_sample_times(int argc, IDL_VPTR argv[])
{
  void *cds_object;
  size_t sample_start;
  size_t sample_count, i;
  time_t *sample_times;
  IDL_VPTR res, tmp;
  IDL_LONG64 *arr;

  cds_object = (void *) IDL_MEMINTScalar(argv[0]);
  sample_start = (size_t) IDL_MEMINTScalar(argv[1]);

  sample_times = dsproc_get_sample_times(cds_object, sample_start,
      &sample_count, NULL);

  if (argc > 2) {
    tmp = IDL_GettmpMEMINT((IDL_MEMINT) sample_count);
    IDL_VarCopy(tmp, argv[2]);
  }
  if (sample_times) {
    arr = (IDL_LONG64 *) IDL_MakeTempVector(IDL_TYP_LONG64, sample_count,
        IDL_ARR_INI_NOP, &res);
    for (i=0; i<sample_count; i++) arr[i] = sample_times[i];
    free(sample_times);
  } else {
    res = _GettmpNull();
  }
  return res;
}

IDL_VPTR idl_dsproc_get_sample_timevals(int argc, IDL_VPTR argv[])
{
  void *cds_object;
  size_t sample_start;
  size_t sample_count, i;
  timeval_t *sample_times;
  IDL_VPTR res, tmp;
  double *arr;

  cds_object = (void *) IDL_MEMINTScalar(argv[0]);
  sample_start = (size_t) IDL_MEMINTScalar(argv[1]);

  sample_times = dsproc_get_sample_timevals(cds_object, sample_start,
      &sample_count, NULL);

  if (argc > 2) {
    tmp = IDL_GettmpMEMINT((IDL_MEMINT) sample_count);
    IDL_VarCopy(tmp, argv[2]);
  }
  if (sample_times) {
    arr = (double *) IDL_MakeTempVector(IDL_TYP_DOUBLE, sample_count,
        IDL_ARR_INI_NOP, &res);
    for (i=0; i<sample_count; i++) arr[i] = sample_times[i].tv_sec +
      sample_times[i].tv_usec * 1e-6;
    free(sample_times);
  } else {
    res = _GettmpNull();
  }
  return res;
}

IDL_VPTR idl_dsproc_set_base_time(int argc, IDL_VPTR argv[])
{
  void *cds_object;
  const char *long_name=NULL;
  time_t base_time;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  cds_object = (void *) IDL_MEMINTScalar(argv[0]);
  if (IS_UNDEF(argv[1])) {
    long_name = IDL_VarGetString(argv[1]);
  }
  base_time = (time_t) IDL_Long64Scalar(argv[2]);
  return IDL_GettmpLong(
      dsproc_set_base_time(cds_object, long_name, base_time) );
}

IDL_VPTR idl_dsproc_set_sample_times(int argc, IDL_VPTR argv[])
{
  void *cds_object;
  size_t sample_start;
  IDL_VPTR tmp;
  int res;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  cds_object = (void *) IDL_MEMINTScalar(argv[0]);
  sample_start = (size_t) IDL_MEMINTScalar(argv[1]);
  IDL_ENSURE_ARRAY(argv[2]);
  if (sizeof(time_t) == 4) tmp = IDL_CvtLng(1, argv+2);
  else if (sizeof(time_t) == 8) tmp = IDL_CvtLng64(1, argv+2);
  else IDL_MessageFromBlock(msg_block, TIME_T_SIZE, IDL_MSG_LONGJMP);

  res = dsproc_set_sample_times(cds_object, sample_start,
      tmp->value.arr->n_elts, (time_t *) tmp->value.arr->data);

  if (tmp != argv[2]) IDL_Deltmp(tmp);

  return IDL_GettmpLong(res);
}

IDL_VPTR idl_dsproc_set_sample_timevals(int argc, IDL_VPTR argv[])
{
  void *cds_object;
  size_t sample_start, sample_count, i;
  timeval_t *sample_times;
  IDL_VPTR tmp;
  double *v;
  int res;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  cds_object = (void *) IDL_MEMINTScalar(argv[0]);
  sample_start = (size_t) IDL_MEMINTScalar(argv[1]);
  IDL_ENSURE_ARRAY(argv[2]);

  tmp = IDL_CvtDbl(1, argv+2);

  sample_count = tmp->value.arr->n_elts;
  sample_times = malloc(sample_count * sizeof(timeval_t));
  v = (double *) tmp->value.arr->data;
  for (i=0; i<sample_count; i++) {
    sample_times[i].tv_sec = (IDL_LONG64) v[i];
    sample_times[i].tv_usec = (IDL_LONG64) (1e6 * (v[i] - (IDL_LONG64) v[i]));
  }
  res = dsproc_set_sample_timevals(cds_object, sample_start, sample_count,
      sample_times);
  free(sample_times);
  return IDL_GettmpLong(res);
}

IDL_VPTR idl_dsproc_create_timestamp(int argc, IDL_VPTR argv[])
{
  time_t secs1970;
  char *timestamp;
  int res;
  IDL_VPTR retval;
  /* prevent unused parameter compiler warning  */
  argc = argc;

  secs1970 = (time_t) IDL_Long64Scalar(argv[0]);

  timestamp = (char *)calloc(16, sizeof(char));
  res = dsproc_create_timestamp(secs1970, timestamp);
  if (res == 1) {
    retval =  IDL_StrToSTRING(timestamp);
  } else {
    retval = IDL_GettmpLong(0);
  } 

  return retval;

}

IDL_VPTR idl_dsproc_get_var_dqrs(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  VarDQR **var_dqrs;
  IDL_STRUCT_TAG_DEF tags[] = {
    { "ID", NULL, (void *) IDL_TYP_STRING, 0 },
    { "DESC", NULL, (void *) IDL_TYP_STRING, 0 },
    { "DS_NAME", NULL, (void *) IDL_TYP_STRING, 0 },
    { "VAR_NAME", NULL, (void *) IDL_TYP_STRING, 0 },
    { "CODE", NULL, (void *) IDL_TYP_LONG, 0 },
    { "COLOR", NULL, (void *) IDL_TYP_STRING, 0 },
    { "CODE_DESC", NULL, (void *) IDL_TYP_STRING, 0 },
    { "START_TIME", NULL, (void *) IDL_TYP_LONG64, 0 },
    { "END_TIME", NULL, (void *) IDL_TYP_LONG64, 0 },
    { "START_INDEX", NULL, (void *) IDL_TYP_MEMINT, 0 },
    { "END_INDEX", NULL, (void *) IDL_TYP_MEMINT, 0 },
    { 0 }
  };
  int offs[11];
  char *p;
  void *s;
  IDL_STRING *str;
  IDL_LONG *code;
  IDL_LONG64 *time;
  IDL_MEMINT *index;
  IDL_VPTR retval;
  int res;
  int i;

  var = (CDSVar *) IDL_MEMINTScalar(argv[0]);
  res = dsproc_get_var_dqrs(var, &var_dqrs);
  // Populate struct
  if (res <= 0) return IDL_GettmpLong(res);

  if (argc < 2) return IDL_GettmpLong(res);

  IDL_EXCLUDE_EXPR(argv[1]);

  s = IDL_MakeStruct(0, tags);
  p = IDL_MakeTempStructVector(s, res, &retval, IDL_ARR_INI_NOP);
  
  for (i=0; tags[i].name; i++) {
    offs[i] = IDL_StructTagInfoByIndex(s, i, IDL_MSG_LONGJMP, NULL);
  }
  for (i=0; i<res; i++) {
    str = (IDL_STRING *) (p+offs[0]);
    IDL_StrStore(str, var_dqrs[i]->id);
    str = (IDL_STRING *) (p+offs[1]);
    IDL_StrStore(str, var_dqrs[i]->desc);
    str = (IDL_STRING *) (p+offs[2]);
    IDL_StrStore(str, var_dqrs[i]->ds_name);
    str = (IDL_STRING *) (p+offs[3]);
    IDL_StrStore(str, var_dqrs[i]->var_name);
    code = (IDL_LONG *) (p+offs[4]);
    *code = var_dqrs[i]->code;
    str = (IDL_STRING *) (p+offs[5]);
    IDL_StrStore(str, var_dqrs[i]->color);
    str = (IDL_STRING *) (p+offs[6]);
    IDL_StrStore(str, var_dqrs[i]->code_desc);
    time = (IDL_LONG64 *) (p+offs[7]);
    *time = var_dqrs[i]->start_time;
    time = (IDL_LONG64 *) (p+offs[8]);
    *time = var_dqrs[i]->end_time;
    index = (IDL_MEMINT *) (p+offs[9]);
    *index = var_dqrs[i]->start_index;
    index = (IDL_MEMINT *) (p+offs[10]);
    *index = var_dqrs[i]->end_index;

    p += retval->value.s.arr->elt_len;
  }

  IDL_VarCopy(retval, argv[1]);
  return IDL_GettmpLong(res);
}

/** IDL front end to dsproc_get_bad_qc_mask
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_get_bad_qc_mask(int argc, IDL_VPTR argv[])
{
  CDSVar *var;
  /* prevent unused parameter compiler warning  */
  argc = argc;

  var = (CDSVar *) IDL_MEMINTScalar(argv[0]);

  return IDL_GettmpLong( dsproc_get_bad_qc_mask(var) );
}

/** IDL front end to dsproc_map_datasets
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_map_datasets(int argc, IDL_VPTR argv[])
{

/* KLG update because output dataset is not required input?? */
  CDSGroup *in_dataset, *out_dataset;
/*  CDSGroup *parent; */
  int res=0, flags;
/*  IDL_VPTR tmp;  */
  /* prevent unused parameter compiler warning  */
  argc = argc;

  in_dataset=(CDSGroup *) IDL_MEMINTScalar(argv[0]);
  out_dataset=(CDSGroup *) IDL_MEMINTScalar(argv[1]);

  flags = IDL_LongScalar(argv[2]);
  res=dsproc_map_datasets(in_dataset, out_dataset, flags);


  return IDL_GettmpLong(res);
}

/** IDL front end to dsproc_set_map_time_range
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_set_map_time_range(int argc, IDL_VPTR argv[])
{

  time_t begin_time, end_time;
  void *res;

  /* prevent unused parameter compiler warning  */
  argc = argc;
  argv = argv;

  begin_time = IDL_Long64Scalar(argv[0]);
  end_time = IDL_Long64Scalar(argv[1]);

  res = _GettmpNull();
  dsproc_set_map_time_range(begin_time, end_time);

  return (res);

}

/** IDL front end to dsproc_get_time_remaining
  
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_get_time_remaining(int argc, IDL_VPTR *argv)
{
  /* prevent unused parameter compiler warning  */
  argc = argc;
  argv = argv;

 return IDL_GettmpLong64( (IDL_LONG64) dsproc_get_time_remaining() );
}

/** IDL front end to dsproc_get_tmax_run_time
 *   
 *     * @param argc - number of arguments
 *       * @param argv - array of pointers to IDL_VARIABLE structures
 *         */
IDL_VPTR idl_dsproc_get_max_run_time(int argc, IDL_VPTR *argv)
{
  /* prevent unused parameter compiler warning  */
  argc = argc;
  argv = argv;

 return IDL_GettmpLong64( (IDL_LONG64) dsproc_get_max_run_time() );
}


/** IDL front end to dsproc_set_input_dir
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_set_input_dir(int argc, IDL_VPTR argv[])
{
  const char *input_dir;
  void *res;
  
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  res = _GettmpNull(); 

  input_dir = IDL_VarGetString(argv[0]);
  dsproc_set_input_dir(input_dir);
  
  return(res);
  
}

/** IDL front end to dsproc_set_input_source
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_set_input_source(int argc, IDL_VPTR argv[])
{
  const char *input_source;
  void *res;
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  res = _GettmpNull(); 

  input_source = IDL_VarGetString(argv[0]);
  dsproc_set_input_source(input_source);
  
  return(res);
}

/** IDL front end to dsproc_free_file_list
  *
  * @param argc - number of arguments
  * @param argv - array of pointers to IDL_VARIABLE structures
  */
IDL_VPTR idl_dsproc_free_file_list(int argc, IDL_VPTR argv[])
{
  int i, nfiles;
  char **files;
  void *res;
  IDL_STRING *s;
  argc = argc; 

  if (argv[0]->flags & IDL_V_ARR) {
    if(IS_UNDEF(argv[0])) {
      files = NULL;
    }
    else {
      IDL_ENSURE_ARRAY(argv[0]);
      IDL_ENSURE_STRING(argv[0]);
      nfiles = argv[0]->value.arr->n_elts;
      files = malloc(nfiles * sizeof(char *));
      s=(IDL_STRING *) argv[0]->value.arr->data;
      for (i=0; i<nfiles; i++) {
        files[i]=IDL_STRING_STR(s+i);
      }
      free(files);
    }
  }
  else {
   IDL_ENSURE_STRING(argv[0]);
    nfiles=1;
    files = malloc(nfiles * sizeof(char *));
    free(files);
  }

  res = _GettmpNull();
  return(res);

}

/** IDL front end to dsproc_set_processing_interval
 *
 * @param argc - number of arguments
 * @param argv - array of pointers to IDL_VARIABLE structures
 */
IDL_VPTR idl_dsproc_set_processing_interval(int argc, IDL_VPTR argv[])
{
  time_t begin_time, end_time;
  void *res;

  /* prevent unused parameter compiler warning  */
  argc = argc;
  argv = argv;

  /* KLG May2018, I updated the next line to reference argv[1] */
  /*     and updated to cast begin and end time to time_t types */
  /*end_time = IDL_Long64Scalar(argv[0]); */
  begin_time = (time_t) IDL_Long64Scalar(argv[0]);
  end_time = (time_t) IDL_Long64Scalar(argv[1]);
  res = _GettmpNull();
  dsproc_set_processing_interval(begin_time, end_time);
  return (res);

}
/** IDL front end to dsproc_set_processing_interval_offset
 *
 * @param argc - number of arguments
 * @param argv - array of pointers to IDL_VARIABLE structures
 */
IDL_VPTR idl_dsproc_set_processing_interval_offset(int argc, IDL_VPTR argv[])
{
  time_t   offset;
  void *res;

  /* prevent unused parameter compiler warning  */
  argc = argc;
  argv = argv;

  offset = IDL_Long64Scalar(argv[0]);
  res = _GettmpNull();
  dsproc_set_processing_interval_offset(offset);
  return (res);

}

/** IDL front end to dsproc_set_trans_qc_rollup_flag
 *  *
 *   * @param argc - number of arguments
 *    * @param argv - array of pointers to IDL_VARIABLE structures
 *     */
IDL_VPTR idl_dsproc_set_trans_qc_rollup_flag(int argc, IDL_VPTR argv[])
{
  int   flag;
  void *res;

  /* prevent unused parameter compiler warning  */
  argc = argc;
  argv = argv;

  flag = IDL_LongScalar(argv[0]);
  res = _GettmpNull();
  dsproc_set_trans_qc_rollup_flag(flag);
  return (res);

}

/* IDL front end to dsproc_get_force_mode
 * 
 * @param argc - number of arguments
 * @param argv - array of pointers to IDL_VARIABLE structures
 */
IDL_VPTR idl_dsproc_get_force_mode(int argc, IDL_VPTR argv[])
{ 
  int   res;
  
  /* prevent unused parameter compiler warning  */
  argc = argc;
  
  res = dsproc_get_force_mode();
  return IDL_GettmpLong(res);

}

/* IDL front end to dsproc_get_quicklook_mode
 * 
 * @param argc - number of arguments
 * @param argv - array of pointers to IDL_VARIABLE structures
 */
IDL_VPTR idl_dsproc_get_quicklook_mode(int argc, IDL_VPTR argv[])
{
  int   res;

  /* prevent unused parameter compiler warning  */
  argc = argc;

  res = dsproc_get_quicklook_mode();
  if (res == QUICKLOOK_NORMAL)
      return IDL_StrToSTRING("QUICKLOOK_NORMAL");
  else if (res == QUICKLOOK_ONLY)
      return IDL_StrToSTRING("QUICKLOOK_ONLY");
  else if (res == QUICKLOOK_DISABLE)
      return IDL_StrToSTRING("QUICKLOOK_DISABLE");

}




/*IDL front end to dsproc_rename
 *
 * @param argc - number of arguments
 * @param argv - array of pointers to IDL_VARIABLE structures
 */
IDL_VPTR idl_dsproc_rename(int argc, IDL_VPTR argv[])
{
  int res, dsid;
  const char *file_path, *file_name;
  time_t begin_time, end_time;
  /* prevent unused parameter compiler warning  */
  argc = argc;
  res = 0;

  dsid = IDL_LongScalar(argv[0]);
  file_path=IDL_VarGetString(argv[1]);
  file_name=IDL_VarGetString(argv[2]);
  begin_time = (time_t) IDL_Long64Scalar(argv[3]);
  if (IS_UNDEF(argv[4])) {
    end_time= (time_t)NULL;
  } else {
  end_time = (time_t) IDL_Long64Scalar(argv[4]);
  }

  res=dsproc_rename(dsid, file_path, file_name, begin_time, end_time);

  return IDL_GettmpLong(res);

}
/*IDL front end to dsproc_rename_bad
 *
 * @param argc - number of arguments
 * @param argv - array of pointers to IDL_VARIABLE structures
 */
IDL_VPTR idl_dsproc_rename_bad(int argc, IDL_VPTR argv[])
{
  int res, dsid;
  const char *file_path, *file_name;
  time_t file_time;
  /* prevent unused parameter compiler warning  */
  argc = argc;
  res = 0;

  dsid = IDL_LongScalar(argv[0]);
  file_path=IDL_VarGetString(argv[1]);
  file_name=IDL_VarGetString(argv[2]);
  file_time = (time_t) IDL_Long64Scalar(argv[3]);

  res=dsproc_rename_bad(dsid, file_path, file_name, file_time);

  return IDL_GettmpLong(res);

}
/*IDL front end to dsproc_set_rename_preserve_dots
 *
 * @param argc - number of arguments
 * @param argv - array of pointers to IDL_VARIABLE structures
 */
IDL_VPTR idl_dsproc_set_rename_preserve_dots(int argc, IDL_VPTR argv[])
{
  int dsid, preserve_dots;
  void *res;

  /* prevent unused parameter compiler warning  */
  argc = argc;

  dsid = IDL_LongScalar(argv[0]);
  preserve_dots = IDL_LongScalar(argv[1]);
  res = _GettmpNull();

  dsproc_set_rename_preserve_dots(dsid, preserve_dots);

  return (res);

}
/*IDL front end to dsproc_fetch_dataset
 *
 * @param argc - number of arguments
 * @param argv - array of pointers to IDL_VARIABLE structures
 */
IDL_VPTR idl_dsproc_fetch_dataset(int argc, IDL_VPTR argv[])
{
  int nobs,dsid, merge_obs;
  timeval_t begin_timeval, end_timeval; 
  size_t nvars, tmptime;
  const char **var_names;
  CDSGroup *dataset;
  IDL_MEMINT i, n;
  IDL_VPTR var_names_tmp=NULL;
  IDL_VPTR tmp_begin, tmp_end, ret_val;
  IDL_STRING *s;
  double v_begin, v_end;

  /* prevent unused parameter compiler warning  */
  argc = argc;

  dsid = IDL_LongScalar(argv[0]);

  tmp_begin = IDL_CvtDbl(1, argv+1);
  tmp_end = IDL_CvtDbl(1, argv+2);
  v_begin = (double) tmp_begin->value.d;
  v_end = (double) tmp_end->value.d;
  begin_timeval.tv_sec = (IDL_LONG64) v_begin;
  begin_timeval.tv_usec = (IDL_LONG64) (1e6 * (v_begin - (IDL_LONG64) v_begin));
  end_timeval.tv_sec = (IDL_LONG64) v_end;
  end_timeval.tv_usec = (IDL_LONG64) (1e6 * (v_end - (IDL_LONG64) v_end));

  tmptime = IDL_LongScalar(argv[2]);

  merge_obs = IDL_LongScalar(argv[3]);

  nvars = (size_t) IDL_MEMINTScalar(argv[4]);

  if (IS_UNDEF(argv[5])) {
    var_names = NULL;
  } 
  else {
   IDL_ENSURE_STRING(argv[5]);
    if (argv[5]->flags & IDL_V_ARR) {
      n = argv[5]->value.arr->n_elts;
      s = (IDL_STRING *) argv[5]->value.arr->data;
    } else {
      n=1;
      s = &argv[5]->value.str;
    }
    var_names = malloc((n+1) * sizeof(const char *));
    for (i=0; i<n; i++) {
      var_names[i] = IDL_STRING_STR(s);
      s++;
    }
    var_names[n] = NULL;
  }


  dataset=(CDSGroup *) NULL;

  nobs = dsproc_fetch_dataset(dsid, &begin_timeval, &end_timeval, nvars,
        var_names, merge_obs, &dataset);

  if (var_names) {
    free(var_names);
  }

  ret_val=IDL_Gettmp();
  ret_val->type=IDL_TYP_UNDEF;
  if (nobs > 0) {
    ret_val->type=IDL_TYP_MEMINT;
    ret_val->value.memint=(IDL_MEMINT) dataset;
  } else ret_val->flags |= IDL_V_NULL;
  IDL_VarCopy(ret_val, argv[6]);

  return IDL_GettmpLong(nobs);

}

/*IDL front end to dsproc_use_nc_extension
 *
 * @param argc - number of arguments
 * @param argv - array of pointers to IDL_VARIABLE structures
 */
IDL_VPTR idl_dsproc_set_coordsys_trans_param(int argc, IDL_VPTR argv[])
{
  const char *coordsys_name;
  const char *field_name;
  const char *param_name;
  CDSDataType data_type;
  size_t length;
  int res;
  int last;
  int nvals;
  char *parr;
  IDL_STRING *s;
  /* prevent unused parameter compiler warning  */
  argc = argc;

  coordsys_name = IDL_VarGetString(argv[0]);
  field_name = IDL_VarGetString(argv[1]);
  param_name = IDL_VarGetString(argv[2]);

  if (IS_UNDEF(argv[3])) {
    data_type=CDS_NAT;
  } else {
    data_type=_idl_to_cds_datatype( IDL_LongScalar(argv[3]) );
  }

  length=IDL_MEMINTScalar(argv[4]);

  // For simplicity only accept arrays
  IDL_ENSURE_ARRAY(argv[5]);

  // Always 1 dim
  nvals = argv[5]->value.arr->dim[0];
  if (data_type == 1) {
    if (argv[5]->value.arr->n_dim == 0) {
      nvals=1;
    }
    s=(IDL_STRING *) argv[5]->value.arr->data;
    int i;
    if (nvals == 1) {
      res=dsproc_set_coordsys_trans_param(
        coordsys_name,
        field_name,
        param_name,
        CDS_CHAR,
        (size_t) length,
        IDL_STRING_STR(s));
    }
    else {
      for (i=0; i<nvals; i++) {
        res=dsproc_set_coordsys_trans_param(
          coordsys_name,
          field_name,
          param_name,
          CDS_CHAR,
          1,
          IDL_STRING_STR(s));
        s++;
      }
    }
  }
  else {
    res=dsproc_set_coordsys_trans_param(
        coordsys_name,
        field_name,
        param_name,
        data_type,
         (size_t) argv[5]->value.arr->n_elts,
        argv[5]->value.arr->data);
  }
  return IDL_GettmpLong(res);
}

/*IDL front end to dsproc_use_nc_extension
 *
 * @param argc - number of arguments
 * @param argv - array of pointers to IDL_VARIABLE structures
 */
IDL_VPTR idl_dsproc_use_nc_extension(int argc, IDL_VPTR argv[]) 
{
  /* prevent unused parameter compiler warning  */
  argc = argc;

  dsproc_use_nc_extension();
  return _GettmpNull();
}

/*IDL front end to dsproc_disable_lock_file
 *  *
 *   * @param argc - number of arguments
 *    * @param argv - array of pointers to IDL_VARIABLE structures
 *     */
IDL_VPTR idl_dsproc_disable_lock_file(int argc, IDL_VPTR argv[])
{
  /* prevent unused parameter compiler warning  */
  argc = argc;

  dsproc_disable_lock_file();
  return _GettmpNull();
}

/** IDL front end to dsproc_set_retriever_time_offsets
 *
 * @param argc - number of arguments
 * @param argv - array of pointers to IDL_VARIABLE structures
 */
IDL_VPTR idl_dsproc_set_retriever_time_offsets(int argc, IDL_VPTR argv[])
{
  int ds_id;
  time_t begin_offset, end_offset;

  /* prevent unused parameter compiler warning  */
  argc = argc;
  argv = argv;

  ds_id=IDL_LongScalar(argv[0]);
  begin_offset = IDL_Long64Scalar(argv[1]);
  end_offset = IDL_Long64Scalar(argv[2]);
  dsproc_set_retriever_time_offsets(ds_id, begin_offset, end_offset);
  return _GettmpNull();
}

IDL_VPTR idl_dsproc_get_location(int argc, IDL_VPTR argv[])
{
  ProcLoc *proc_loc;
  IDL_STRUCT_TAG_DEF tags[] = {
    { "NAME", NULL, (void *) IDL_TYP_STRING, 0 },
    { "LAT", NULL, (void *) IDL_TYP_FLOAT, 0 },
    { "LON", NULL, (void *) IDL_TYP_FLOAT, 0 },
    { "ALT", NULL, (void *) IDL_TYP_FLOAT, 0 },
    { 0 }
  };
  int offs[11];
  char *p;
  void *s;
  IDL_STRING *str;
  float *lat;
  float *lon;
  float *alt;
  IDL_VPTR retval;
  int res;
  int i;

  res = dsproc_get_location(&proc_loc);
  // Populate struct
  if (res <= 0) return IDL_GettmpLong(res);

  if (argc < 1) return IDL_GettmpLong(res);

  // Don't allow structure to be an IDL temp var or constant.
  IDL_EXCLUDE_EXPR(argv[0]);

  s = IDL_MakeStruct(0, tags);
  p = IDL_MakeTempStructVector(s, 1, &retval, IDL_ARR_INI_NOP);

  /* There will be only one, but reusing logic from 
   * get_var_dqrs function 
  for (i=0; tags[i].name; i++) {
    offs[i] = IDL_StructTagInfoByIndex(s, i, IDL_MSG_LONGJMP, NULL);
  } */

  offs[0] = IDL_StructTagInfoByIndex(s, 0, IDL_MSG_LONGJMP, NULL);
  offs[1] = IDL_StructTagInfoByIndex(s, 1, IDL_MSG_LONGJMP, NULL);
  offs[2] = IDL_StructTagInfoByIndex(s, 2, IDL_MSG_LONGJMP, NULL);
  offs[3] = IDL_StructTagInfoByIndex(s, 3, IDL_MSG_LONGJMP, NULL);


  str = (IDL_STRING *) (p+offs[0]);
  lat = (float *) (p+offs[1]);
  lon = (float *) (p+offs[2]);
  alt = (float *) (p+offs[3]);

  IDL_StrStore(str, proc_loc->name);
  *lat = proc_loc->lat;
  *lon = proc_loc->lon;
  *alt = proc_loc->alt;

  /* KLG May2018, added cast to get rid of warning, but didn't
         test as currently no funcitonal vaps use this function */
  p = (char *)retval->value.s.arr->elt_len;

  IDL_VarCopy(retval, argv[0]);
  return IDL_GettmpLong(res);
}

// AB Step 3
// Write the functions whose entry point was linked to an IDL routine
// name in step 1 and 2. The entry point name can be anything.
// Functions always return an IDL_VPTR, and procedures always return
// void. The argc, argv parameters are required to be present, and
// if the routine accepts keywords in addition to positional parameters
// then "char *argk" must be added as a third parameter.
//
// For mode details, see: /apps/base/rsi/idl82/help/pdf/edg.pdf
//
IDL_VPTR idl_dsproc_get_status(int argc, IDL_VPTR *argv)
{
  /* prevent unused parameter compiler warning  */
  argc = argc; 
  argv = argv; 
  // The IDL_StrToSTRING function converts a standard
  // C const char *, or char * to an IDL string variable
  // it also allocates temporary memory needed for the IDL variable.
  // In order to avoid memory leaks, such a temporary IDL variable
  // must either be returned to IDL or deleted with IDL_Deltmp.
 return IDL_StrToSTRING( dsproc_get_status() );
}

// AB Step 6
// The function to return ndims to IDL
IDL_VPTR idl_cds_var_ndims(int argc, IDL_VPTR *argv)
{
  CDSVar *var;
  /* prevent unused parameter compiler warning  */
  argc = argc; 

  // The IDL_MEMINTScalar converts the IDL variable into a scalar of
  // type IDL_MEMINT (this is basically a signed size_t) it is signed
  // to allow subtraction and checking differences.
  // Note that it the user passes in an IDL variable that cannot be
  // converted to a scalar memint (such as an array or a string, ...)
  // then, this function (IDL_MEMINTScalar) will report an error back
  // to the IDL user
  var = (CDSVar *)IDL_MEMINTScalar(argv[0]);
  
  // The IDL_GettmpLong function will convert a 32-bit integer (int)
  // into a scalar IDL variable (a temporary IDL variable will be used).
  // Type casting is not needed since var->ndims is also a 32-bit integer
  //
  // For mode info on IDL types, see: /apps/base/rsi/idl82/help/pdf/edg.pdf
  return IDL_GettmpLong( var->ndims );
}
// Note that the next step is in dsproc__define.pro

/** IDL entry point for DLM. Defines all function and procedure info.
  * Also define error messages and such.
  */
int IDL_Load(void)
{
  static IDL_SYSFUN_DEF2 func_addr[] = {
    { {idl_dsproc_start_processing_loop}, "DSPROC_START_PROCESSING_LOOP", 1, 1, 0, 0 },
    { {idl_dsproc_retrieve_data}, "DSPROC_RETRIEVE_DATA", 2, 2, 0, 0 },
    { {idl_dsproc_merge_retrieved_data}, "DSPROC_MERGE_RETRIEVED_DATA", 0, 0, 0, 0 },
    { {idl_dsproc_transform_data}, "DSPROC_TRANSFORM_DATA", 1, 1, 0, 0 },
    { {idl_dsproc_create_output_datasets}, "DSPROC_CREATE_OUTPUT_DATASETS", 0, 0, 0, 0 },
    { {idl_dsproc_create_output_dataset}, "DSPROC_CREATE_OUTPUT_DATASET", 3, 3, 0, 0 },
    { {idl_dsproc_store_output_datasets}, "DSPROC_STORE_OUTPUT_DATASETS", 0, 0, 0, 0 },
    { {idl_dsproc_store_dataset}, "DSPROC_STORE_DATASET", 2, 2, 0, 0 },
    { {idl_dsproc_finish}, "DSPROC_FINISH", 0, 0, 0, 0 },
    { {idl_dsproc_get_site}, "DSPROC_GET_SITE", 0, 0, 0, 0 },
    { {idl_dsproc_get_facility}, "DSPROC_GET_FACILITY", 0, 0, 0, 0 },
    { {idl_dsproc_get_name}, "DSPROC_GET_NAME", 0, 0, 0, 0 },
    { {idl_dsproc_proc_model}, "DSPROC_PROC_MODEL", 1, 1, 0, 0 },
    { {idl_dsproc_get_datastream_id}, "DSPROC_GET_DATASTREAM_ID", 5, 5, 0, 0 },
    { {idl_dsproc_get_input_datastream_id}, "DSPROC_GET_INPUT_DATASTREAM_ID", 2, 2, 0, 0 },
    { {idl_dsproc_get_input_datastream_ids}, "DSPROC_GET_INPUT_DATASTREAM_IDS", 1, 1, 0, 0 },
    { {idl_dsproc_get_output_datastream_id}, "DSPROC_GET_OUTPUT_DATASTREAM_ID", 2, 2, 0, 0 },
    { {idl_dsproc_get_output_datastream_ids}, "DSPROC_GET_OUTPUT_DATASTREAM_IDS", 1, 1, 0, 0 },
    { {idl_dsproc_find_datastream_files}, "DSPROC_FIND_DATASTREAM_FILES", 3, 4, 0, 0 },
    { {idl_dsproc_get_datastream_files}, "DSPROC_GET_DATASTREAM_FILES", 2, 2, 0, 0 },
    { {idl_dsproc_set_datastream_path}, "DSPROC_SET_DATASTREAM_PATH", 2, 2, 0, 0 },
    { {idl_dsproc_unset_datastream_flags}, "DSPROC_UNSET_DATASTREAM_FLAGS", 2, 2, 0, 0 },
    { {idl_dsproc_datastream_name}, "DSPROC_DATASTREAM_NAME", 1, 1, 0, 0 },
    { {idl_dsproc_datastream_path}, "DSPROC_DATASTREAM_PATH", 1, 1, 0, 0 },
    { {idl_dsproc_datastream_site}, "DSPROC_DATASTREAM_SITE", 1, 1, 0, 0 },
    { {idl_dsproc_datastream_facility}, "DSPROC_DATASTREAM_FACILITY", 1, 1, 0, 0 },
    { {idl_dsproc_datastream_class_name}, "DSPROC_DATASTREAM_CLASS_NAME", 1, 1, 0, 0 },
    { {idl_dsproc_datastream_class_level}, "DSPROC_DATASTREAM_CLASS_LEVEL", 1, 1, 0, 0 },
    { {idl_dsproc_set_datastream_flags}, "DSPROC_SET_DATASTREAM_FLAGS", 2, 2, 0, 0},
    { {idl_dsproc_add_datastream_file_patterns}, "DSPROC_ADD_DATASTREAM_FILE_PATTERNS", 4, 4, 0, 0},
    { {idl_dsproc_set_datastream_file_extension}, "DSPROC_SET_DATASTREAM_FILE_EXTENSION", 2, 2, 0, 0},
    { {idl_dsproc_set_datastream_split_mode}, "DSPROC_SET_DATASTREAM_SPLIT_MODE", 4, 4, 0, 0},
    { {idl_dsproc_dataset_name}, "DSPROC_DATASET_NAME", 1, 1, 0, 0 },
    { {idl_dsproc_dump_dataset}, "DSPROC_DUMP_DATASET", 6, 6, 0, 0 },
    { {idl_cds_delete_group}, "CDS_DELETE_GROUP", 1, 1, 0, 0 },
    { {idl_cds_trim_unlim_dim}, "CDS_TRIM_UNLIM_DIM", 3, 3, 0, 0 },
    { {idl_dsproc_dump_output_datasets}, "DSPROC_DUMP_OUTPUT_DATASETS", 3, 3, 0, 0 },
    { {idl_dsproc_dump_retrieved_datasets}, "DSPROC_DUMP_RETRIEVED_DATASETS", 3, 3, 0, 0 },
    { {idl_dsproc_dump_transformed_datasets}, "DSPROC_DUMP_TRANSFORMED_DATASETS", 3, 3, 0, 0 },
    { {idl_dsproc_get_debug_level}, "DSPROC_GET_DEBUG_LEVEL", 0, 0, 0, 0 },
    { {idl_dsproc_get_output_var}, "DSPROC_GET_OUTPUT_VAR", 3, 3, 0, 0 },
    { {idl_dsproc_get_qc_var}, "DSPROC_GET_QC_VAR", 1, 1, 0, 0 },
    { {idl_dsproc_get_time_var}, "DSPROC_GET_TIME_VAR", 1, 1, 0, 0 },
    { {idl_dsproc_var_name}, "DSPROC_VAR_NAME", 1, 1, 0, 0 },
    { {idl_dsproc_get_source_var_name}, "DSPROC_GET_SOURCE_VAR_NAME", 1, 1, 0, 0 },
    { {idl_dsproc_get_source_ds_name}, "DSPROC_GET_SOURCE_DS_NAME", 1, 1, 0, 0 },
    { {idl_dsproc_get_source_ds_id}, "DSPROC_GET_SOURCE_DS_ID", 1, 1, 0, 0 },
    { {idl_dsproc_var_sample_count}, "DSPROC_VAR_SAMPLE_COUNT", 1, 1, 0, 0 },
    { {idl_dsproc_var_sample_size}, "DSPROC_VAR_SAMPLE_SIZE", 1, 1, 0, 0 },
    { {idl_cds_var_data}, "CDS_VAR_DATA", 1, 1, 0, 0 },
    { {idl_cds_var_type}, "CDS_VAR_TYPE", 1, 1, IDL_SYSFUN_DEF_F_KEYWORDS, 0 },
    { {idl_cds_var_dims}, "CDS_VAR_DIMS", 1, 1, 0, 0 },
    { {idl_dsproc_var_dim_names}, "DSPROC_VAR_DIM_NAMES", 1, 1, 0, 0 }, 
    { {idl_dsproc_get_var_missing_values}, "DSPROC_GET_VAR_MISSING_VALUES", 2, 2, 0, 0 },
    { {idl_dsproc_init_var_data}, "DSPROC_INIT_VAR_DATA", 3, 4, 0, 0 },
    { {idl_dsproc_set_var_data}, "DSPROC_SET_VAR_DATA", 3, 3, 0, 0 },
    { {idl_dsproc_get_output_dataset}, "DSPROC_GET_OUTPUT_DATASET", 2, 2, 0, 0 },
    { {idl_dsproc_get_retrieved_dataset}, "DSPROC_GET_RETRIEVED_DATASET", 2, 2, 0, 0 },
    { {idl_dsproc_get_transformed_dataset}, "DSPROC_GET_TRANSFORMED_DATASET", 3, 3, 0, 0 },
    { {idl_dsproc_get_dim}, "DSPROC_GET_DIM", 2, 2, 0, 0 },
    { {idl_dsproc_get_dim_length}, "DSPROC_GET_DIM_LENGTH", 2, 2, 0, 0 },
    { {idl_dsproc_set_dim_length}, "DSPROC_SET_DIM_LENGTH", 3, 3, 0, 0 },
    { {idl_dsproc_change_att}, "DSPROC_CHANGE_ATT", 4, 4, 0, 0},
    { {idl_dsproc_get_att}, "DSPROC_GET_ATT", 2, 2, 0, 0},
    { {idl_dsproc_get_att_text}, "DSPROC_GET_ATT_TEXT", 2, 2, 0, 0},
    { {idl_dsproc_get_att_value}, "DSPROC_GET_ATT_VALUE", 2, 2, 0, 0},
    { {idl_dsproc_set_att}, "DSPROC_SET_ATT", 4, 4, 0, 0},
    { {idl_dsproc_set_att_text}, "DSPROC_SET_ATT_TEXT", 3, 3, 0, 0},
    { {idl_dsproc_set_att_value}, "DSPROC_SET_ATT_VALUE", 3, 3, 0, 0},
    { {idl_dsproc_get_retrieved_var}, "DSPROC_GET_RETRIEVED_VAR", 2, 2, 0, 0},
    { {idl_dsproc_get_transformed_var}, "DSPROC_GET_TRANSFORMED_VAR", 1, 2, 0, 0},
    { {idl_dsproc_clone_var}, "DSPROC_CLONE_VAR", 6, 6, 0, 0},
    { {idl_dsproc_copy_var_tag}, "DSPROC_COPY_VAR_TAG", 2, 2, 0, 0},
    { {idl_dsproc_flags}, "DSPROC_FLAGS", 1, 1, 0, 0},
    { {idl_dsproc_set_var_flags}, "DSPROC_SET_VAR_FLAGS", 2, 2, 0, 0},
    { {idl_cds_obj_parent}, "CDS_OBJ_PARENT", 1, 1, 0, 0},
    { {idl_cds_obj_type}, "CDS_OBJ_TYPE", 1, 1, 0, 0},
    { {idl_cds_obj_name}, "CDS_OBJ_NAME", 1, 1, 0, 0},
    { {idl_cds_att_names}, "CDS_ATT_NAMES", 1, 1, 0, 0},
    { {idl_cds_var_names}, "CDS_VAR_NAMES", 1, 1, 0, 0},
    { {idl_cds_group_names}, "CDS_GROUP_NAMES", 1, 1, 0, 0},
    { {idl_cds_get_group}, "CDS_GET_GROUP", 1, 2, 0, 0},
    { {idl_cds_dim_names}, "CDS_DIM_NAMES", 1, 1, 0, 0},
    { {idl_cds_def_lock}, "CDS_DEF_LOCK", 1, 1, 0, 0},
    { {idl_cds_obj_path}, "CDS_OBJ_PATH", 1, 1, 0, 0},
    { {idl_dsproc_set_var_output_target}, "DSPROC_SET_VAR_OUTPUT_TARGET", 3, 3, 0, 0},
    { {idl_dsproc_get_var_output_targets}, "DSPROC_GET_VAR_OUTPUT_TARGETS", 1, 1, 0, 0},
    { {idl_dsproc_set_var_coordsys_name}, "DSPROC_SET_VAR_COORDSYS_NAME", 2, 2, 0, 0},
    { {idl_dsproc_add_var_output_target}, "DSPROC_ADD_VAR_OUTPUT_TARGET", 3, 3, 0, 0},
    { {idl_dsproc_define_var}, "DSPROC_DEFINE_VAR", 7, 11, 0, 0},
    { {idl_dsproc_delete_var}, "DSPROC_DELETE_VAR", 1, 1, 0, 0},
    { {idl_dsproc_get_coord_var}, "DSPROC_GET_COORD_VAR", 2, 2, 0, 0},
    { {idl_dsproc_get_dataset_vars}, "DSPROC_GET_DATASET_VARS", 4, 6, 0, 0},
    { {idl_dsproc_get_var}, "DSPROC_GET_VAR", 2, 2, 0, 0},
    { {idl_dsproc_get_metric_var}, "DSPROC_GET_METRIC_VAR", 2, 2, 0, 0},
    { {idl_dsproc_get_trans_coordsys_var}, "DSPROC_GET_TRANS_COORDSYS_VAR", 2, 3, 0, 0},
    { {idl_dsproc_get_base_time}, "DSPROC_GET_BASE_TIME", 1, 1, 0, 0},
    { {idl_dsproc_get_time_range}, "DSPROC_GET_TIME_RANGE", 3, 3, 0, 0},
    { {idl_dsproc_get_sample_times}, "DSPROC_GET_SAMPLE_TIMES", 2, 3, 0, 0},
    { {idl_dsproc_get_sample_timevals}, "DSPROC_GET_SAMPLE_TIMEVALS", 2, 3, 0, 0},
    { {idl_dsproc_set_base_time}, "DSPROC_SET_BASE_TIME", 3, 3, 0, 0},
    { {idl_dsproc_set_sample_times}, "DSPROC_SET_SAMPLE_TIMES", 3, 3, 0, 0},
    { {idl_dsproc_set_sample_timevals}, "DSPROC_SET_SAMPLE_TIMEVALS", 3, 3, 0, 0},
    { {idl_dsproc_create_timestamp}, "DSPROC_CREATE_TIMESTAMP", 1, 1, 0, 0},
    { {idl_dsproc_get_var_dqrs}, "DSPROC_GET_VAR_DQRS", 1, 2, 0, 0},
    { {idl_dsproc_get_bad_qc_mask}, "DSPROC_GET_BAD_QC_MASK", 1, 1, 0, 0},
    { {idl_dsproc_map_datasets}, "DSPROC_MAP_DATASETS", 3, 3, 0, 0 },
    { {idl_dsproc_set_map_time_range}, "DSPROC_SET_MAP_TIME_RANGE", 2, 2, 0, 0 },
    { {idl_dsproc_get_time_remaining}, "DSPROC_GET_TIME_REMAINING", 0, 0, 0, 0},
    { {idl_dsproc_get_max_run_time}, "DSPROC_GET_MAX_RUN_TIME", 0, 0, 0, 0 },
    { {idl_dsproc_set_input_dir}, "DSPROC_SET_INPUT_DIR", 1, 1, 0, 0},
    { {idl_dsproc_set_input_source}, "DSPROC_SET_INPUT_SOURCE", 1, 1, 0, 0},
    { {idl_dsproc_free_file_list}, "DSPROC_FREE_FILE_LIST", 1, 1, 0, 0 },
    { {idl_dsproc_set_processing_interval}, "DSPROC_SET_PROCESSING_INTERVAL", 2, 2, 0, 0 },
    { {idl_dsproc_set_processing_interval_offset}, "DSPROC_SET_PROCESSING_INTERVAL_OFFSET", 1, 1, 0, 0 },
    { {idl_dsproc_set_trans_qc_rollup_flag}, "DSPROC_SET_TRANS_QC_ROLLUP_FLAG", 1, 1, 0, 0 },
    { {idl_dsproc_get_force_mode}, "DSPROC_GET_FORCE_MODE", 0, 0, 0, 0 },
    { {idl_dsproc_get_quicklook_mode}, "DSPROC_GET_QUICKLOOK_MODE", 0, 0, 0, 0 },
    { {idl_dsproc_rename}, "DSPROC_RENAME", 4, 5, 0, 0 },
    { {idl_dsproc_rename_bad}, "DSPROC_RENAME_BAD", 4, 4, 0, 0 },
    { {idl_dsproc_set_rename_preserve_dots}, "DSPROC_SET_RENAME_PRESERVE_DOTS", 2, 2, 0, 0 },
    { {idl_dsproc_fetch_dataset}, "DSPROC_FETCH_DATASET", 5, 7, 0, 0 },
    { {idl_dsproc_set_coordsys_trans_param}, "DSPROC_SET_COORDSYS_TRANS_PARAM", 6, 6, 0, 0 },
    { {idl_dsproc_use_nc_extension}, "DSPROC_USE_NC_EXTENSION", 0, 0, 0, 0 },
    { {idl_dsproc_disable_lock_file}, "DSPROC_DISABLE_LOCK_FILE", 0, 0, 0, 0 },
    { {idl_dsproc_set_retriever_time_offsets}, "DSPROC_SET_RETRIEVER_TIME_OFFSETS", 3, 3, 0, 0 },
    { {idl_dsproc_get_location}, "DSPROC_GET_LOCATION", 1, 1, 0, 0},
    { {idl_dsproc_setopt}, "DSPROC_SETOPT", 4, 4, 0, 0},
    { {idl_dsproc_getopt}, "DSPROC_GETOPT", 2, 2, 0, 0},
    // AB STEP 1
    // Registers a definition of how the function will be visible in IDL
    // struct consists of local entry point, IDL all caps function name,
    // minimum number of arguments, max number of arguments, flags for 
    // whether it accepts keywords or is an obsolete function, reserved flag
    // Note: the next step is in idl_dsproc.dlm
    { {idl_dsproc_get_status}, "DSPROC_GET_STATUS", 0, 0, 0, 0},
    // AB STEP 4
    // Register another function to return ndims
    // Note: Next step is in idl_dsproc.dlm
    { {idl_cds_var_ndims}, "CDS_VAR_NDIMS", 1, 1, 0, 0},

  };
  static IDL_SYSFUN_DEF2 proc_addr[] = {
    { {(IDL_SYSRTN_GENERIC) idl_dsproc_db_disconnect}, "DSPROC_DB_DISCONNECT", 0, 0, 0, 0 },
    { {(IDL_SYSRTN_GENERIC) idl_dsproc_initialize}, "DSPROC_INITIALIZE", 3, 4, 0, 0 },
    { {(IDL_SYSRTN_GENERIC) idl_cds_print}, "CDS_PRINT", 3, 3, 0, 0},
    { {(IDL_SYSRTN_GENERIC) idl_dsproc_error}, "DSPROC_ERROR", 4, 5, 0, 0},
    { {(IDL_SYSRTN_GENERIC) idl_dsproc_abort}, "DSPROC_ABORT", 4, 5, 0, 0},
    { {(IDL_SYSRTN_GENERIC) idl_dsproc_bad_file_warning}, "DSPROC_BAD_FILE_WARNING", 4, 5, 0, 0},
    { {(IDL_SYSRTN_GENERIC) idl_dsproc_bad_line_warning}, "DSPROC_BAD_LINE_WARNING", 5, 6, 0, 0},
    { {(IDL_SYSRTN_GENERIC) idl_dsproc_set_status}, "DSPROC_SET_STATUS", 1, 1, 0, 0},
    { {(IDL_SYSRTN_GENERIC) idl_dsproc_debug}, "DSPROC_DEBUG", 5, 5, 0, 0},
    { {(IDL_SYSRTN_GENERIC) idl_dsproc_delete_var_tag}, "DSPROC_DELETE_VAR_TAG", 1, 1, 0, 0},
    { {(IDL_SYSRTN_GENERIC) idl_dsproc_unset_var_flags}, "DSPROC_UNSET_VAR_FLAGS", 2, 2, 0, 0},
    { {(IDL_SYSRTN_GENERIC) idl_dsproc_log}, "DSPROC_LOG", 4, 4, 0, 0},
    { {(IDL_SYSRTN_GENERIC) idl_dsproc_mentor_mail}, "DSPROC_MENTOR_MAIL", 4, 4, 0, 0},
    { {(IDL_SYSRTN_GENERIC) idl_dsproc_warning}, "DSPROC_WARNING", 4, 4, 0, 0},
    { {(IDL_SYSRTN_GENERIC) idl_dsproc_freeopts}, "DSPROC_FREEOPTS", 0, 0, 0, 0},
  };

  // Load the message block (return if it fails)
  if (!(msg_block=IDL_MessageDefineBlock("DSPROC", IDL_CARRAY_ELTS(msg_arr),
        msg_arr))) return 0;

  return IDL_SysRtnAdd(func_addr, IDL_TRUE, IDL_CARRAY_ELTS(func_addr))
    && IDL_SysRtnAdd(proc_addr, IDL_FALSE, IDL_CARRAY_ELTS(proc_addr));
}

