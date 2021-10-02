;+
; @file_comments
;  Implements the CDSGroup object class.
; @property name
;  The dataset name.
; @property att_names {type=string}
;  An array of string attribute names, or !null if no attributes are defined.
; @property dim_names {type=string}
;  An array of string dimension names, or !null if no dimensions are defined.
; @property var_names {type=string}
;  An array of string variable names, or !null if no variables are defined.
; @property group_names {type=string}
;  An array of string group names, of !null if no subgroups are present.
; @property base_time {type=int}
;  Base time in seconds since 1970 UTC, or 0 if not found. <br>
;  The base time of a dataset or time variable. <br>
;  This property will convert the units attribute of a time variable to seconds
;  since 1970. The specified dataset will be searched and then its parent
;  datasets will be searched until a "time" or "time_offset" variable is found.
; @property time_range {type=double}
;  The start and end times given as double precision floating point numbers,
;  in units of seconds. The dataset will be searched for a "time" or
;  "time_offset" variable is found. A scalar 0 is returned in case of failure.
; @property time_var {type=CDSVar}
;  The time variable used by the dataset, or null.
;-

;+
; Write to a file using "cds_print"
; The caller should make sure the file does not exist.
; @param file_name {in}{required}{type=string}
;  The output filename.
; @keyword skip_group_atts {in}{optional}{type=boolean}{default=false}
;  Do not print group attributes.
; @keyword skip_var_atts {in}{optional}{type=boolean}{default=false}
;  Do not print variable attributes.
; @keyword skip_data {in}{optional}{type=boolean}{default=false}
;  Do not print variable data.
; @keyword skip_subgroups {in}{optional}{type=boolean}{default=false}
;  Do not print subgroups.
;-
pro CDSGroup::Write, file_name, skip_group_atts=skip_group_atts, $
    skip_var_atts=skip_var_atts, skip_data=skip_data, $
    skip_subgroups=skip_subgroups
  compile_opt idl2, logical_predicate
  ; Assemble flags
  flags=0
  if keyword_set(skip_group_atts) then $
    flags or= dsproc_flags('CDS_SKIP_GROUP_ATTS')
  if keyword_set(skip_var_atts) then $
    flags or= dsproc_flags('CDS_SKIP_VAR_ATTS')
  if keyword_set(skip_data) then $
    flags or= dsproc_flags('CDS_SKIP_DATA')
  if keyword_set(skip_subgroups) then $
    flags or= dsproc_flags('CDS_SKIP_SUBGROUPS')

  cds_print, *self.p, file_name, flags
end
;+
; Get an attribute
;
; @param name {in}{required}{type=string}
;  name of the attribute
; @returns
;  Object reference to the attribute
;  or, NULL object reference if the attribute does not exist.
;  Use OBJ_VALID to check existence.
;-
function CDSGroup::get_att, name
  compile_opt idl2, logical_predicate

  p = dsproc_get_att(*self.p, name)
  res = p ? obj_new(cds_obj_typename(ptr=p), p) : obj_new()
  return, res
end
;+
; Set the value of an attribute in a dataset or variable.
;<br>
; This function will define the specified attribute if it
; does not exist. If the attribute does exist and the overwrite
; flag is set, the value will be set by casting the specified
; value into the data type of the attribute. The functions
; cds_string_to_array() and cds_array_to_string() are used to
; convert between text (CDS_CHAR) and numeric data types.
;<br>
; If an error occurs in this function it will be appended to
; the log and error mail messages, and the process status will
; be set appropriately.
;
; @param name {in}{required}{type=string}
;  Attribute name.
; @param value {in}{required}
;  A variable of any supported type containing the attribute
;  value.
; @keyword overwrite {in}{optional}{type=boolean}{default=false}
;  Overwrite flag (1 = TRUE, 0 = FALSE)
; @returns
;  1 if successful <br>
;  0 if: <br>
;   - the parent object is not a group or variable<br>
;   - the parent group or variable definition is locked<br>
;   - the attribute definition is locked<br>
;   - a memory allocation error occurred<br>
;-
function CDSGroup::set_att, name, value, overwrite=overwrite
  compile_opt idl2, logical_predicate
  if ~isa(value, 'string') && isa(value, /scalar) then begin
    ; this function does not accept scalars except string,
    ; so convert to a 1-element array.
    res = dsproc_set_att(*self.p, keyword_set(overwrite), name, [value])
  endif else begin
    ; either string, or array, pass directly
    res = dsproc_set_att(*self.p, keyword_set(overwrite), name, value)
  endelse

  return, res
end
;+
; Change an attribute for a dataset or variable.
;<br>
; This function will define the specified attribute if it does not exist.
; If the attribute does exist and the overwrite flag is set, the data type
; and value will be changed.
;<br>
; If an error occurs in this function it will be appended to the log and
; error mail messages, and the process status will be set appropriately.
;
; @param name {in}{required}{type=string}
;  Attribute name.
; @param value {in}{required}{type=string}
;  The attribute value.
; @keyword overwrite {in}{optional}{type=boolean}{default=false}
;  Overwrite flag (1 = TRUE, 0 = FALSE)
; @returns
;  1 if successful <br>
;  0 if: <br>
;      the parent object is not a group or variable<br>
;      the parent group or variable definition is locked<br>
;      the attribute definition is locked<br>
;-
function CDSGroup::change_att, name, value, overwrite=overwrite
  compile_opt idl2, logical_predicate

  return, dsproc_change_att(*self.p, keyword_set(overwrite), name, value)
end
;+
; Set the value of an attribute in a dataset or variable.
;<br>
; This function will set the value of an attribute by casting the
; specified value into the data type of the attribute. The functions
; cds_string_to_array() and cds_array_to_string() are used to convert
; between text (CDS_CHAR) and numeric data types.
;<br>
; If an error occurs in this function it will be appended to the log and
; error mail messages, and the process status will be set appropriately.
;
; @param name {in}{type=string}{required}
;  Name of the attribute.
; @param value {in}{required}
;  The new attribute value, will be converted to the attribute type if needed.
; @returns
;  1 if successful <br>
;  0 if: <br>
;   the attribute does not exist
;   the attribute definition is locked
;   a memory allocation error occurred
;-
function CDSGroup::set_att_value, name, value
  compile_opt idl2, logical_predicate

  return, dsproc_set_att_value(*self.p, name, value)
end

;+
; Set the value of an attribute in a dataset or variable.
;<br>
; The cds_string_to_array() function will be used to set the attribute value
; if the data type of the attribute is not CDS_CHAR.
;<br>
; If an error occurs in this function it will be appended to the log and error
; mail messages, and the process status will be set appropriately.
;
; @param name {in}{type=string}{required}
;  Name of the attribute.
; @param str {in}{type=string}{required}
;  The new attribute value.
; @returns
;  1 if successful <br>
;  0 if:  <br>
;      the attribute does not exist
;      the attribute definition is locked
;      a memory allocation error occurred
;-
function CDSGroup::set_att_text, name, str
  compile_opt idl2, logical_predicate

  return, dsproc_set_att_text(*self.p, name, str)
end

;+
; Define a new variable in an existing dataset.
;<br>
; This function will define a new variable with all standard attributes.
; Any of the attribute values can be NULL to indicate that the attribute
; should not be created.
;<br>
; Description of Attributes:<br>
;  long_name: This is a one line description of the variable and should
;  be suitable to use as a plot title for the variable.<br>
;  standard_name: This is defined in the CF Convention and describes the
;  physical quantities being represented by the variable. Please refer to
;  the "CF Standard Names" section of the CF Convention for the table of
;  standard names.<br>
;  units: This is the units string to use for the variable and must be
;  recognized by the UDUNITS-2 libary.<br>
;  valid_min: The smallest value that should be considered to be a valid
;  data value. The specified value must be the same data type as the variable.
;<br>
;  valid_max: The largest value that should be considered to be a valid data
;  value. The specified value must be the same data type as the variable.
;<br>
;  missing_value: This comes from an older NetCDF convention and has been used
;  by ARM for almost 2 decades. The specified value must be the same data type
;  as the variable.
;<br>
;  fill_value: Most newer conventions specify the use of _FillValue over
;  missing_value. The value of this attribute is also recognized by the NetCDF
;  library and will be used to initialize the data values on disk when the
;  variable is created. Tools like ncdump will also display fill values as
;  _ so they can be easily identified in a text dump. The libdsproc3 library
;  allows you to use both missing_value and _FillValue and they do not need
;  to be the same. The specified value must be the same data type as the
;  variable.<br>
; If an error occurs in this function it will be appended to the log and error
; mail messages, and the process status will be set appropriately.
;
; @param name {in}{required}{type=string}
;  Name of the variable.
; @param type {in}{required}{type=int}
;  IDL data type code of the variable, see IDL's SIZE function.
; @param dim_names {in}{required}{type=string}
;  Either a scalar or array of string names for the dimensions.
; @keyword long_name {in}{optional}{type=string}{default=!null}
;  String to use for the long_name attribute.
; @keyword standard_name {in}{optional}{type=string}{default=!null}
;  String to use for the standard_name attribute.
; @keyword units {in}{optional}{type=string}{default=!null}
;  String to use for the units attribute.
; @keyword valid_min {in}{optional}{default=!null}
;  Minimum value attribute.
; @keyword valid_max {in}{optional}{default=!null}
;  Maximum value attribute.
; @keyword missing_value {in}{optional}{default=!null}
;  The missing_value attribute.
; @keyword fill_value {in}{optional}{default=!null}
;  The _FillValue attribute.
; @returns
;  Object reference to the new variable,<br>
;  or, NULL object if an error occurred
;-
function CDSGroup::define_var, name, type, dim_names, long_name=long_name, $
    standard_name=standard_name, units=units, valid_min=valid_min, $
    valid_max=valid_max, missing_value=missing_value, fill_value=fill_value
  compile_opt idl2, logical_predicate

  if ~n_elements(long_name) then long_name = !null
  if ~n_elements(standard_name) then standard_name = !null
  if ~n_elements(units) then units = !null
  if ~n_elements(valid_min) then valid_min = !null
  if ~n_elements(valid_max) then valid_max = !null
  if ~n_elements(missing_value) then missing_value = !null
  if ~n_elements(fill_value) then fill_value = !null

  in_dim_names = isa(dim_names, /scalar) ? [dim_names] : dim_names

  ptr = dsproc_define_var(*self.p, name, type, in_dim_names, long_name, $
    standard_name, units, valid_min, valid_max, missing_value, fill_value)

  res = ptr ? obj_new('CDSVar', ptr) : obj_new()
  return, res
end

;+
; Get a dimension from a dataset.
;
; @param name {in}{required}{type=string}
;  Name of the dimension.
; @returns
;  Object reference to the dimension
;<br>  or, NULL object if the dimension does not exist.
;-
function CDSGroup::get_dim, name
  compile_opt idl2, logical_predicate
  
  ptr = dsproc_get_dim(*self.p, name)
  res = ptr ? obj_new('CDSDim', ptr) : obj_new()
  return, res
end
;+
; Get the length of a dimension from a dataset.
;
; @param name {in}{required}{type=string}
;  Name of the dimension.
; @returns
;  The length of the dimension.
;-
function CDSGroup::get_dim_length, name
  compile_opt idl2, logical_predicate
  
  len = dsproc_get_dim_length(*self.p, name)
  return, len
end
;+
; Set the length of a dimension from a dataset.
;<br>
; If an error occurs in this function it will be appended to the log and error
; mail messages, and the process status will be set appropriately.
;
; @param name {in}{required}{type=string}
;  Name of the dimension.
; @param length {in}{required}{type=int}
;  New length of the dimension.
; @returns
;  1 if successful <br>
;  0 if: <br>
;    the dimension does not exist <br>
;    the dimension definition is locked <br>
;    data has already been added to a variable using this dimension
;-
function CDSGroup::set_dim_length, name, length
  compile_opt idl2, logical_predicate
  
  r = dsproc_set_dim_length(*self.p, name, length)
  return, r
end

;+
; Get variables and companion QC variables from a dataset.
;<br>
; If var_names is not specified or !null, the output vars array will contain the
; object references to the variables that are not companion QC variables.
; In this case the variables in the vars array will be in the same order they
; appear in the dataset. The following time and location variables will be
; excluded from this array: <br>
;<br>
; - base_time <br>
; - time_offset <br>
; - time <br>
; - lat <br>
; - lon <br>
; - alt <br>
;<br>
; If var_names are specified, the output vars array will contain an
; entry for every variable in the var_names array, and will be in the specified
; order. Variables that are not found in the dataset will have a NULL object if
; the required flag is set to 0. If the required flag is set to 1 and a
; variable does not exist, an error will be generated.<br>
;
; If the qc_vars keyword is set to a named variable it will contain the
; references to the companion qc_ variables. Likewise, if the aqc_vars
; keyword is set to a named variable it will return the references to the
; companion aqc_ variables. If a companion QC variable does not exist for
; a variable, the corresponding entry in the QC array will be a NULL object
; reference. <br>
;
; If an error occurs in this function it will be appended to the log and
; error mail messages, and the process status will be set appropriately.
;
; @param var_names {in}{optional}{type=string}{default=!null}
;  String array of variable names, or !null as described above.
; @keyword required {in}{optional}{type=boolean}{default=false}
;  Specifies if all variables in the names list are required.
; @keyword qc_vars {out}{optional}{type=objref}
;  Optionally, returns an array of the companion qc_ variables.
; @keyword aqc_vars {out}{optional}{type=objref}
;  Optionally, returns an array of companion aqc_ variables.
; @returns
;  Array of variables, (may contain null objects, see "required") <br>
;  or, !null if no variables were found.
;-
function CDSGroup::get_dataset_vars, var_names, required=required, $
    qc_vars=qc_vars, aqc_vars=aqc_vars
  compile_opt idl2, logical_predicate

  ; flag whether to return
  ret_qc = arg_present(qc_vars)
  ret_aqc = arg_present(aqc_vars)

  ; call the library and return the requested variable pointers
  if ret_aqc then begin
    n = dsproc_get_dataset_vars(*self.p, var_names, keyword_set(required), $
      ptrs, qc_ptrs, aqc_ptrs)
  endif else if ret_qc then begin
    n = dsproc_get_dataset_vars(*self.p, var_names, keyword_set(required), $
      ptrs, qc_ptrs)
  endif else begin
    n = dsproc_get_dataset_vars(*self.p, var_names, keyword_set(required), $
      ptrs)
  endelse

  if ret_qc then qc_vars = (n gt 0) ? objarr(n) : !null
  if ret_aqc then aqc_vars = (n gt 0) ? objarr(n) : !null

  ; Handle special case
  if n le 0 then return, !null

  ; convert pointers to objects
  vars = (n gt 0) ? objarr(n) : !null
  for i=0, n-1 do begin
    if ptrs[i] then vars[i] = self->_toObject(ptrs[i])
    if ret_qc then begin
      if qc_ptrs[i] then qc_vars[i] = self->_toObject(qc_ptrs[i])
    endif
    if ret_aqc then begin
      if aqc_ptrs[i] then aqc_vars[i] = self->_toObject(aqc_ptrs[i])
    endif
  endfor

  return, vars
end
;+
; Get a variable from a dataset.
;<br>
; The syntax: <br>
;  var = dataset->get_var('varname') <br>
; and <br>
;  var = dataset['varname'] <br>
; will accomplish the same result, although the latter syntax will
; first search for a group named 'varname', and if that does not
; exist, then a variable named 'varname' is searched for.
;
; @param name {in}{optional}{type=string}{default=all}
;  The name of the variable (case sensitive). If this is a scalar,
;  then a scalar object reference is returned, otherwise an array
;  of object references is returned. If this argument is omitted,
;  then all known variables are returned.
; @returns
;  An scalar or array object reference, to the variable(s), <br>
;  a null object reference if it does not exist.
;-
function CDSGroup::get_var, name
  compile_opt idl2, logical_predicate

  if n_elements(name) eq 0 then begin
    ; default is to return all
    n = cds_var_names(*self.p)
  endif else n = name
  nn = n_elements(n)
  if nn eq 0 then return, obj_new()
  res = objarr(nn)
  for i=0, nn-1 do begin
    ptr = dsproc_get_var(*self.p, n[i])
    res[i] = self->_toObject(ptr)
  endfor
  if isa(n, /scalar) then res = res[0]
  return, res
end
;+
; Get a group from a dataset.
;<br>
; The syntax: <br>
;  group = dataset->get_group('group') <br>
;
; @param name {in}{optional}{type=string}{default=all}
;  The name of the group (case sensitive). If this is a scalar,
;  then a scalar object reference is returned, otherwise an array
;  of object references is returned. If this argument is omitted,
;  then all known groups are returned.
; @returns
;  An scalar or array object reference, to the group(s), <br>
;  a null object reference if it does not exist.
;-
function CDSGroup::get_group, name
  compile_opt idl2, logical_predicate

  if n_elements(name) eq 0 then begin
    ; default is to return all
    n = cds_group_names(*self.p)
  endif else n = name
  nn = n_elements(n)
  if nn eq 0 then return, obj_new()
  res = objarr(nn)
  for i=0, nn-1 do begin
    ptr = cds_get_group(*self.p, n[i])
    res[i] = self->_toObject(ptr)
  endfor
  if isa(n, /scalar) then res = res[0]
  return, res
end
;+
; Get the sample times for a dataset or time variable.
;<br>
; This function will convert the data values of a time variable to seconds
; since 1970. If the input object is a CDSGroup, the specified dataset
; and then its parent datasets will be searched until a "time" or
; "time_offset" variable is found.
;<br>
; Note: If the sample times can have fractional seconds the
; dsproc_get_sample_timevals() function should be used instead.
;<br>
; If an error occurs in this function it will be appended to the log and
; error mail messages, and the process status will be set appropriately.
;
; @param sample_start {in}{type=int}{optional}{default=0}
;  Start sample (0 based indexing).
; @keyword count {out}{optional}{type=int}
;  The number of samples returned. This will be 0 if no samples were found,
;  and -1 in case of an error.
; @keyword double {in}{optional}{type=boolean}{default=0}
;  Set this keyword to return double precision numbers instead of integer
;  seconds. The fractional seconds are obtained using the
;  dsproc_get_sample_timevals function and converting tv_sec, tv_usec
;  into a double precision floating point number.
; @returns
;  An array of sample times in seconds since 1970, of !null if no samples
;  were found, or an error happened.
;-
function CDSGroup::get_sample_times, sample_start, count=sample_count, $
    double=dbl
  compile_opt idl2, logical_predicate

  s = n_elements(sample_start) ne 1 ? 0 : sample_start
  if keyword_set(dbl) then begin
    res = dsproc_get_sample_timevals(*self.p, s, sample_count)
  endif else begin
    res = dsproc_get_sample_times(*self.p, s, sample_count)
  endelse
  return, res
end
;+
; Set the base time of a dataset or time variable.
;<br>
; This function will set the base time for a time variable and adjust all
; attributes and data values as necessary. If the input object is one of
; the standard time variables ("time", "time_offset", or "base_time"), all
; standard time variables that exist in its parent dataset will also be
; updated. If the input object is a CDSGroup, the specified dataset and then
; its parent datasets will be searched until a "time" or "time_offset"
; variable is found. All standard time variables that exist in this dataset
; will then be updated.
;<br>
; For the base_time variable the data value will be set and the "string"
; attribute will be set to the string representation of the base_time value.
; The "long_name" and "units" attributes will also be set to "Base time in
; Epoch" and "seconds since 1970-1-1 0:00:00 0:00" respectively.
;<br>
; For the time_offset variable the "units" attribute will set to the string
; representation of the base_time value, and the "long_name" attribute will
; be set to "Time offset from base_time".
;<br>
; For all other time variables the "units" attribute will be set to the string
; representation of the base_time value, and the "long_name" attribute will be
; set to the specified value. If a long_name attribute is not specified, the
; string "Time offset from midnight" will be used for base times of midnight,
; and "Sample times" will be used for all other base times.
;<br>
; Any existing data in a time variable will also be adjusted for the new
; base_time value.
;<br>
; If an error occurs in this function it will be appended to the log and error
; mail messages, and the process status will be set appropriately.
;
; @param base_time {in}{required}{type=int}
;  Base time in seconds since 1970.
; @keyword long_name {in}{optional}{type=string}{default=!null}
;  String to use for the long_name attribute.
; @returns
;  1 if successful <br>
;  0 if an error occurred
;-
function CDSGroup::set_base_time, base_time, long_name=long_name
  compile_opt idl2, logical_predicate

  lon = n_elements(long_name) eq 1 ? long_time : !null
  res = dsproc_set_base_time(*self.p, lon, base_name)
  return, res
end

;+
; Set the sample times for a dataset or time variable.
;<br>
; This function will set the data values for a time variable by subtracting
; the base time (as defined by the units attribute) and converting the
; remainder to the data type of the variable.
;<br>
; If the input object is one of the standard time variables:
;<br> - time
;<br> - time_offset
;<br> - base_time
;<br>
; All standard time variables that exist in its parent dataset will also be
; updated.
;<br>
; If the input object is a CDSGroup, the specified dataset and then its parent
; datasets will be searched until a "time" or "time_offset" variable is found.
; All standard time variables that exist in this dataset will then be updated.
;<br>
; If the specified sample_start value is 0 and a base time value has not
; already been set, the base time will be set using the time of midnight just
; prior to the first sample time.
;<br>
; The data type of the time variable(s) must be CDS_SHORT, CDS_INT, CDS_FLOAT
; or CDS_DOUBLE. However, CDS_DOUBLE is usually recommended.
; Note that if the /FLOAT keyword is set, then only CDS_FLOAT or CDS_DOUBLE
; are permitted.
;<br>
; If an error occurs in this function it will be appended to the log and error
; mail messages, and the process status will be set appropriately.
;
; @param sample_times {in}{required}
;  The array of sample times in seconds since 1970 UTC. This can be either
;  floating point or integer type, see FLOAT keyword.
; @keyword sample_start {in}{optional}{type=int}{default=0}
;  Start sample (0 based indexing).
; @keyword float {in}{optional}{type=boolean}{default=0}
;  Set this keyword to support fractional seconds. By default, the
;  "dsproc_set_sample_times" function is used, which causes input times to be
;  treated as integer seconds (truncated if needed). But, if /FLOAT is set,
;  then the "dsproc_set_sample_timevals" function is used instead which
;  supports microsecond resolution. In this case, the time variable must be
;  either CDS_FLOAT or CDS_DOUBLE type.
; @returns
;  1 if successful<br>
;  0 if an error occurred
;-
function CDSGroup::set_sample_times, sample_times, sample_start=sample_start, $
    float=flt
  compile_opt idl2, logical_predicate

  s_start = keyword_set(sample_start) ? sample_start : 0
  if keyword_set(flt) then begin
    res = dsproc_set_sample_timevals(*self.p, s_start, sample_times)
  endif else begin
    res = dsproc_set_sample_times(*self.p, s_start, sample_times)
  endelse
  return, res
end
;+
; Dump the contents of a dataset to a text file.
;<br>
; This function will dump the contents of a dataset to a text file with the
; following name:
;<br><br>
;   prefix.YYYYMMDD.hhmmss.suffix
;<br><br>
; If an error occurs in this function it will be appended to the log and error
; mail messages, and the process status will be set appropriately.
;
; @param outdir {in}{type=string}{required}
;  The output directory.
; @param prefix {in}{type=string}{optional}
;  The prefix to use for the file name, or !NULL to use the name of the dataset.
; @param file_time {in}{type=int}{optional}
;  The time to use to create the file timestamp, or 0 to use the first sample time in the dataset.
; @param suffix {in}{type=string}{optional}{default=''}
;  The suffix portion of the file name.
; @keyword flags {in}{optional}{type=int}{default=0}
;  Reserved for control flags.
; @returns
;  1 if successful <br>
;  0 if and error occurred
;-
function CDSGroup::dump_dataset, outdir, prefix, file_time, suffix, $
    flags=flags
  compile_opt idl2, logical_predicate

  pf = (n_elements(prefix) eq 0) ? !null : prefix
  ft = (n_elements(file_time) eq 0) ? 0 : file_time
  sf = (n_elements(suffix) eq 0) ? '' : suffix
  fl = (n_elements(flags) eq 0) ? 0 : flags
  
  return, dsproc_dump_dataset(*self.p, outdir, pf, ft, sf, fl)
end
function CDSGroup::delete_group
  compile_opt idl2, logical_predicate

  return, cds_delete_group(*self.p)
end
pro CDSGroup::trim_unlim_dim, unlim_dim_name, length
  compile_opt idl2, logical_predicate

  tmp = cds_trim_unlim_dim(*self.p, unlim_dim_name, length)
end
;+
; Overload brackets to return child objects
;<br>
; @param isRange {in}{required}
;  Flag indicating whether the subscript is a range, for each dimension.
; @param sub1 {in}{required}
;  Either integer or string or range, as in:
;   t = group['ambient_temp']
;   t = group[0]
;   all = group[*]
;   some = group[2:5]
; @returns
;  Object reference, or array of object references, or !null
;-
function CDSGroup::_overloadBracketsRightSide, isRange, sub1
  compile_opt idl2, logical_predicate
  if isa(sub1, 'string') then begin
    ; look for a name
    group_names=cds_group_names(*self.p)
    if max(sub1 eq group_names) then begin
      return, self->_toObject(cds_get_group(*self.p, sub1))
    endif
    var_names=cds_var_names(*self.p)
    if max(sub1 eq var_names) then begin
      return, self->_toObject(dsproc_get_var(*self.p, sub1))
    endif
    att_names=cds_att_names(*self.p)
    if max(sub1 eq att_names) then begin
      return, self->_toObject(dsproc_get_att(*self.p, sub1))
    endif
    dim_names=cds_dim_names(*self.p)
    if max(sub1 eq dim_names) then begin
      return, self->_toObject(dsproc_get_dim(*self.p, sub1))
    endif
    return, !null
  endif
  ; integer or range
  all = []
  groups = cds_get_group(*self.p)
  if n_elements(groups) then all = [all, groups]
  if ~isRange && min(sub1) ge 0 && max(sub1) lt n_elements(all) then begin
    return, self->_toObject(all[sub1])
  endif
  var_names=cds_var_names(*self.p)
  for i=0, n_elements(var_names)-1 do begin
    all = [all, dsproc_get_var(*self.p, var_names[i])]
  endfor
  if ~isRange && min(sub1) ge 0 && max(sub1) lt n_elements(all) then begin
    return, self->_toObject(all[sub1])
  endif
  att_names=cds_att_names(*self.p)
  for i=0, n_elements(att_names)-1 do begin
    all = [all, dsproc_get_att(*self.p, att_names[i])]
  endfor
  if ~isRange && min(sub1) ge 0 && max(sub1) lt n_elements(all) then begin
    return, self->_toObject(all[sub1])
  endif
  dim_names=cds_dim_names(*self.p)
  for i=0, n_elements(dim_names)-1 do begin
    all = [all, dsproc_get_dim(*self.p, dim_names[i])]
  endfor
  if ~isRange then begin
    return, self->_toObject(all[sub1])
  endif
  return, self->_toObject(all[sub1[0]:sub1[1]:sub1[2]])
end

;+
; Define public properties
;-
pro CDSGroup::GetProperty, name=name, att_names=att_names, time_var=time_var, $
    dim_names=dim_names, var_names=var_names, group_names=group_names, $
    base_time=base_time, time_range=time_range, $
    _ref_extra=extra
  compile_opt idl2, logical_predicate
  if arg_present(name) then name=dsproc_dataset_name(*self.p)
  if arg_present(att_names) then att_names=cds_att_names(*self.p)
  if arg_present(dim_names) then dim_names=cds_dim_names(*self.p)
  if arg_present(var_names) then var_names=cds_var_names(*self.p)
  if arg_present(group_names) then group_names=cds_group_names(*self.p)
  if arg_present(base_time) then base_time=dsproc_get_base_time(*self.p)
  if arg_present(time_range) then begin
    if dsproc_get_time_range(*self.p, start_time, end_time) then begin
      time_range = [total(start_time*[1, 1d-6]), total(end_time*[1, 1d-6])]
    endif else time_range = 0
  endif
  if arg_present(time_var) then begin
    time_var = self->_toObject(dsproc_get_time_var(*self.p))
  endif
  if n_elements(extra) then self->CDSObject::GetProperty, _strict_extra=extra
end
;+
; Class definition routine
;-
pro CDSGroup__define
  compile_opt idl2, logical_predicate
  s={ CDSGroup $
    , inherits CDSObject $
    }
end
