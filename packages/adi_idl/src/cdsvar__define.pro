;+
; @file_comments
;  An object class that wraps the CDSVar structure pointer
; @property qc_var {type=CDSVar}
;  The companion QC variable, or null object if not found.
; @property time_var {type=CDSVar}
;  The corresponding time variable, or null.
; @property ptr {type=integer}
;  An integer containing the pointer to the underlying
;  C structure.
; @property sample_count {type=integer}
;  The number of samples in the variable's data array.
; @property data
;  The data array associated with the variable, or !null if uninitialized.
;  The init_var_data method can be used to initalize or allocate output
;  variables. Alternatively, data can be assigned directly via SetProperty,
;  such as:<br>   var.data = myarray <br>
;  Note that it is recommended to use explicit assigment after modifying
;  data. For example: <br>  myarray = var.data <br>  myarray = myarray + 2
;<br>  var.data = myarray
; @property missing_values
;  A scalar or array of missing values, or !null if no missing values are
;  defined for the variable.
; @property type {type=int}
;  An IDL typecode integer, see IDL's SIZE function for help on type codes.
; @property tname {type=string}
;  A human readable IDL type name for the variable.
; @property output_targets {type=struct}
;  Returns the output targets defined for the variable. Either
;  a structure array, or !null if no targets are defined.
; @property att_names {type=string}
;  Returns an array of string attribute names, or !null if no attributes
;  are defined.
; @property base_time {type=int}
;  Base time in seconds since 1970 UTC, or 0 if not found. <br>
;  The base time of a dataset or time variable. <br>
;  This property will convert the units attribute of a time variable to seconds
;  since 1970.
; @property time_range {type=double}
;  The start and end times given as double precision floating point numbers,
;  in units of seconds. This works on "time" or "time_offset" variables.
;  A scalar 0 is returned in case of failure.
; AB STEP 8
;  add documentation for ndims property
; @property ndims {type=int}
;  The number of dimensions in the variable.
; @property dim_names {type=string}
;  Returns an array of string dimension names, or !null if no dimensions
;  are defined.
; @property source_var_name {type=string}
;  The name of the source variable read in from the input file.
; @property source_ds_name {type=string}
;  The name of the input datastream the variable was retrieved from.
; @property source_ds_id {type=integer}
;  The datastream id of the input datastream the variable was retrieved from.
; @property _ref_extra
;  Note that all properties of the superclass CDSObject are also valid for
;  CDSVar.
;-

;+
; Overload help.
;
; @param varname {in}{required}{type=string}
;  A string containing the visible name of the argument to help
; @returns
;  A one line string description of the object content.
;-
function CDSVar::_overloadHelp, varName
  compile_opt idl2, logical_predicate

  str = varname + '    <' + typename(self) + '> ' + cds_obj_name(*self.p) + $
    ' ('+strtrim(cds_var_type(*self.p, /name), 2) + ')   [' + $
    strjoin(strtrim(cds_var_dims(*self.p), 2), ',') + ']'
  return, str
end
;+
; Initialize the data values for a dataset variable.
;
; This function will make sure enough memory is allocated
; for the specified samples and initializing the data values
; to either the variable's missing value (use_missing == 1),
; or 0 (use_missing == 0).
;<br>
; The search order for missing values is:
;<br>
;<br>    missing_value attribute
;<br>    _FillValue attribute
;<br>    variable's default missing value
;<br>
; If the variable does not have any missing or fill values
; defined the default fill value for the variable's data type
; will be used and the default fill value for the variable
; will be set.
;<br>
; If the specified start sample is greater than the variable's
; current sample count, the hole between the two will be filled
; with the first missing or fill value defined for the variable.
;<br>
; If an error occurs in this function it will be appended to the
; log and error mail messages, and the process status will be set
; appropriately.
;<br>
; @param sample_start {in}{required}{type=int}
;  start sample of the data to initialize (0 based indexing)
; @param sample_count {in}{required}{type=int}
;  number of samples to initialize
; @Keyword use_missing {in}{optional}{type=boolean}
;  flag indicating if the variables missing value should be used
;  (1 == TRUE, 0 == fill with zeros)
; @returns
;  Variable's data array, or !NULL if:
;<br> - the specified sample count is zero
;<br> - one of the variable's static dimensions has 0 length
;<br> - the variable has no dimensions, and sample_start is not
; equal to 0 or sample_count is not equal to 1.
;<br> - the first variable dimension is not unlimited, and
; sample_start + sample_count would exceed the dimension length.
;<br> - a memory allocation error occurred
;-
function CDSVar::init_var_data, sample_start, sample_count, $
    use_missing=use_missing
  compile_opt idl2, logical_predicate

  res=dsproc_init_var_data(*self.p, sample_start, sample_count, $
    keyword_set(use_missing))
  return, res
end
;+
; Allocate memory for a variable's data array.
;<br>
; This function will allocate memory as necessary to ensure that the
; variable's data array is large enough to store another sample_count
; samples starting from sample_start.
;<br>
; The data type of the returned array will be the same as the variable’s
; data type. It is the responsibility of the calling process to cast the
; returned array into the proper data type. If the calling process does
; not know the data type of the variable, it can store the data in an array
; of a known type and then use the dsproc_set_var_data() function to cast
; this data into the variables data array. In this case the memory does not
; need to be preallocated and this function is not needed.
;<br>
; The data array returned by this function belongs to the variable and will
; be freed when the variable is destroyed. The calling process must not attempt
; to free this memory.
;<br>
; If an error occurs in this function it will be appended to the log and error
; mail messages, and the process status will be set appropriately.
;<br>
; @param sample_start {in}{required}{type=int}
;  start sample of the data to initialize (0 based indexing)
; @param sample_count {in}{required}{type=int}
;  number of samples to initialize
; @returns
;  Variable's data array, or !NULL if:
;<br> - the specified sample count is zero
;<br> - one of the variable's static dimensions has 0 length
;<br> - the variable has no dimensions, and sample_start is not
; equal to 0 or sample_count is not equal to 1.
;<br> - the first variable dimension is not unlimited, and
; sample_start + sample_count would exceed the dimension length.
;<br> - a memory allocation error occurred
;-
function CDSVar::alloc_var_data, sample_start, sample_count
  compile_opt idl2, logical_predicate

  res=dsproc_init_var_data(*self.p, sample_start, sample_count)
  return, res
end

;+
; Create a clone of an existing variable.
;<br>
; If an error occurs in this function it will be appended to the log
; and error mail messages, and the process status will be set appropriately.
;
; @keyword dataset {in}{optional}{default=!null}
;  Dataset to create the new variable in, or !NULL to create the variable in
;  the same dataset the source variable belongs to.
; @keyword var_name {in}{optional}{default=!null}
;  Name to use for the new variable, or !NULL to use the source variable name.
; @keyword data_type {in}{optional}{default=!null}
;  IDL data type to use for the new variable, or !NULL to use the same data
;  type as the source variable. Valid datatypes are:
;<br> 1 = BYTE (8-bit)
;<br> 2 = INT (16-bit)
;<br> 3 = LONG (32-bit)
;<br> 4 = FLOAT (32-bit)
;<br> 5 = DOUBLE (64-bit)
;<br>  all other values will be treated as !NULL.
; @keyword dim_names {in}{optional}{default=!null}
;  Array of strings corresponding to dimension names in the dataset the new
;  variable will be created in, or NULL if the dimension names are the same.
; @keyword copy_data {in}{type=boolean}{default=false}
;  Flag indicating if the data should be copied, (1 == TRUE, 0 == FALSE)
; @returns
;  Object reference to the new variable,
;  or NULL object reference if:
;<br> - the variable already exists in the dataset
;<br> - a memory allocation error occurred
;-
function CDSVar::clone_var, dataset=dataset, var_name=var_name, $
    data_type=data_type, dim_names=dim_names, copy_data=copy_data
  compile_opt idl2, logical_predicate

  ds_ptr = obj_valid(dataset) ? dataset.ptr : !null

  ptr=dsproc_clone_var(*self.p, ds_ptr, var_name, data_type, dim_names, $
    keyword_set(copy_data))
  return, ptr ne 0 ? obj_new('CDSVar', ptr) : obj_new()
end
;+
; Copy a variable tag from one variable to another.
;<br>
; If an error occurs in this function it will be appended to the log and
; error mail messages, and the process status will be set appropriately.
;
; @keyword src_var {in}{optional}{type=CDSVar}{default=self}
;  Variable to copy the tag from, if this is left
;  blank (or !null}, then DEST_VAR is required,
;  and the self object is used as the SRC_VAR.
; @keyword dest_var {in}{optional}{type=CDSVar}{default=self}
;  Variable to copy into. If left blank, then "self" is assumed.
; @Returns
;  1 if successful, or if source and dest are the same<br>
;  0 if an error occurred
;-
function CDSVar::copy_var_tag, src_var=src_var, dest_var=dest_var
  compile_opt idl2, logical_predicate

  src = obj_valid(src_var) ? src_var.ptr : *self.p
  dest = obj_Valid(dest_var) ? dest_var.ptr : *self.p

  return, src eq dest ? 1 : dsproc_copy_var_tag(src, dest)
end

;+
; Set the control flags for a variable.
;<br>
; @keyword VAR_SKIP_TRANSFORM {in}{optional}{type=boolean}{default=false}
;  Instruct the transform logic to ignore this variable.
; @returns
;  1 if successful<br>
;  0 if an error occurred
;-
function CDSVar::set_var_flags, var_skip_transform=var_skip_transform
  compile_opt idl2, logical_predicate

  flags = 0
  if keyword_set(var_skip_transform) then begin
    flags or= dsproc_flags('VAR_SKIP_TRANSFORM')
  endif

  return, dsproc_set_var_flags(*self.p, flags)
end

;+
; Delete a variable tag.
;-
pro CDSVar::delete_var_tag
  compile_opt idl2, logical_predicate
  dsproc_delete_var_tag, *self.p
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
function CDSVar::get_att, name
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
function CDSVar::set_att, name, value, overwrite=overwrite
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
function CDSVar::change_att, name, value, overwrite=overwrite
  compile_opt idl2, logical_predicate

  return, dsproc_change_att(*self.p, keyword_set(overwrite), name, value)
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
function CDSVar::set_att_text, name, str
  compile_opt idl2, logical_predicate

  return, dsproc_set_att_text(*self.p, name, str)
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
function CDSVar::set_att_value, name, value
  compile_opt idl2, logical_predicate

  if isa(value, /scalar) then begin
    ; convert to array because the C code is written to
    ; accept arrays
    return, dsproc_set_att_value(*self.p, name, [value])
  endif
  return, dsproc_set_att_value(*self.p, name, value)
end
;+
; Set the output target for a variable.
;<br>
; This function will replace any previously specified output targets already
; set for the variable.
;<br>
; If an error occurs in this function it will be appended to the log and error
; mail messages, and the process status will be set appropriately.
;
; @param ds_id {in}{required}{type=int}
;  Output datastream ID.
; @param var_name {in}{required}{type=string}
;  Name of the variable in the output datastream.
; @returns
;  1 if successful<br>
;  0 if an error occurred
;-
function CDSVar::set_var_output_target, ds_id, var_name
  compile_opt idl2, logical_predicate
  
  return, dsproc_set_var_output_target(*self.p, ds_id, var_name)
end

;+
; Add an output target for a variable.
;<br>
; This function will add an output target for the variable.
;<br>
; If an error occurs in this function it will be appended to the log and
; error mail messages, and the process status will be set appropriately.
;
; @param ds_id {in}{required}{type=int}
;  Output datastream ID.
; @param var_name {in}{required}{type=string}
;  Name of the variable in the output datastream.
; @Returns
;  1 if successful<br>
;  0 if an error occurred
;-
function CDSVar::add_var_output_target, ds_id, var_name
  compile_opt idl2, logical_predicate

  return, dsproc_add_var_output_target(*self.p, ds_id, var_name)
end

;+
; Set the coordinate system for a variable.
;<br>
; If an error occurs in this function it will be appended to the log and
; error mail messages, and the process status will be set appropriately.
;
; @param coordsys_name {in}{required}{type=string}
;  Name of the coordinate system.
; @returns
;  1 if successful<br>
;  0 if an error occurred
;-
function CDSVar::set_var_coordsys_name, coordsys_name
  compile_opt idl2, logical_predicate

  return, dsproc_set_var_coordsys_name(*self.p, coordsys_name)
end

;+
; Unset the control flags for a variable.
;<br>
; @keyword VAR_SKIP_TRANSFORM {in}{optional}{type=boolean}{default=false}
;  Unset the flag that instructs the transform logic to ignore this variable.
;-
function CDSVar::unset_var_flags, var_skip_transform=var_skip_transform
  compile_opt idl2, logical_predicate

  flags = 0
  if keyword_set(var_skip_transform) then begin
    flags or= dsproc_flags('VAR_SKIP_TRANSFORM')
  endif

  dsproc_unset_var_flags, *self.p, flags
end

;+
; Delete a variable from a dataset.
;<br>
; If an error occurs in this function it will be appended to the log and
; error mail messages, and the process status will be set appropriately.
;<br>
; Note, the caller should not attempt to use this object reference after
; the "delete_var" method has been called.
;
; @returns
;  1 if the variable was deleted (or the input var was NULL) <br>
;  0 if: <br>
;     the variable is locked <br>
;     the group is locked
;-
function CDSVar::delete_var
  compile_opt idl2, logical_predicate

  return, dsproc_delete_var(*self.p)
end

;+
; Get the coordinate variable for a variable's dimension.
;
; @param dim_index {in}{optional}{type=int}{default=0}
;   Index of the dimension.
; @returns
;  Object reference to the coordinate variable. <br>
;  NULL object reference if not found.
;-
function CDSVar::get_coord_var, dim_index
  compile_opt idl2, logical_predicate

  ptr = dsproc_get_coord_var(*self.p, dim_index)
  res = self->_toObject(ptr)
  return, res
end

;+
; Get a companion metric variable for a variable.
;<br>
; Known metrics at the time of this writing (so there may be others):
;<br>
;  "frac": the fraction of available input values used <br>
;  "std": the standard deviation of the calculated value
;
; @param metric {in}{required}{type=string}
;  Name of the metric.
; @returns
;  Object reference to the metric variable, <br>
;  NULL object reference if not found
;-
function CDSVar::get_metric_var, metric
  compile_opt idl2, logical_predicate

  ptr = dsproc_get_metric_var(*self.p, metric)
  res = self->_toObject(ptr)
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
function CDSVar::get_sample_times, sample_start, count=sample_count, $
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
function CDSVar::set_base_time, base_time, long_name=long_name
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
function CDSVar::set_sample_times, sample_times, sample_start=sample_start, $
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
; Get all available DQRs for the data stored in the specified variable.
;<br>
; The memory used by the output array belongs to the internal variable tag and
; must not be freed by the calling process.
;
; @keyword count {out}{optional}{type=int}
;  Optionally returns the number of DQRs, or 0 if none were found, or -1 if an
;  error occurred.
; @returns
;  The array of VarDQR structures.<br>
;  Or a scalar 0 if no DQRs where found for the relevant time range, or the
;  variable was not explicitely requested by the user in the retriever
;  definition.<br>
;  Or -1 if an error occurred.
;-
function CDSVar::get_var_dqrs, count=count
  compile_opt idl2, logical_predicate

  count = dsproc_get_var_dqrs(*self.p, dqrs)
  if count le 0 then dqrs = count
  return, dqrs
end

;+
; Get the QC mask used to determine bad QC valurs.
; This function will use the bit assessment attributes to create
; a mask with all bits set for bad assessment values. 
; It will first check for field level bit assessment attributes,
;  and then for the global attributes if they are not found.
; @returns
; The QC mask with all bad bits set.
;-
function CDSVar::get_bad_qc_mask
  compile_opt idl2, logical_predicate

  bad_mask = dsproc_get_bad_qc_mask(*self.p)
  return, bad_mask

end

;+
; Public properties (get)
;-
pro CDSVar::GetProperty, qc_var=qc_var, time_var=time_var, $
    sample_count=sample_count, data=data, $
    missing_values=missing_values, type=type, tname=tname, $
    output_targets=output_targets, att_names=att_names, $
    base_time=base_time, time_range=time_range, $
    ndims=ndims, $ ; AB STEP 9
    dim_names=dim_names, $  
    source_var_name=source_var_name, $
    source_ds_name=source_ds_name, $
    source_ds_id=source_ds_id, $
    _ref_extra=extra
  compile_opt idl2, logical_predicate

  ; AB Step 10 add code to retrieve the number of dimensions, if
  ; requested. self.p is an IDL pointer to an integer that holds
  ; the C pointer to the CDSVar structure. The reason for using
  ; an IDL pointer instead of a hard coded integer type in the
  ; IDL self structure definition is that it works better across
  ; 32-bit and 64-bit platforms. Hard coded structures have a
  ; specific integer size coded in, here, we just use whatever
  ; comes back from the C wrapper. self.p is actually defined in
  ; the superclass CDSObject.
  if arg_present(ndims) then ndims = cds_var_ndims(*self.p)
  if arg_present(source_var_name) then source_var_name = dsproc_get_source_var_name(*self.p)
  if arg_present(source_ds_name) then source_ds_name = dsproc_get_source_ds_name(*self.p)
  if arg_present(source_ds_id) then source_ds_id = dsproc_get_source_ds_id(*self.p)
  if arg_present(qc_var) then begin
    ptr_qc_var=dsproc_get_qc_var(*self.p)
    qc_var = ptr_qc_var eq 0 ? obj_new() : obj_new('CDSVar', ptr_qc_var)
  endif
  if arg_present(time_var) then begin
    time_var = self->_toObject(dsproc_get_time_var(*self.p))
  endif
  if arg_present(sample_count) then begin
    sample_count=dsproc_var_sample_count(*self.p)
  endif
  if arg_present(data) then begin
    data=cds_var_data(*self.p)
    if strtrim(cds_var_type(*self.p, /name), 2) eq 'STRING' then data = string(data)
  endif
  if arg_present(missing_values) then begin
    missing_values=!null
    n_missing=dsproc_get_var_missing_values(*self.p, missing_values)
    ; return a scalar instead of 1 element array
    if n_missing eq 1 then missing_values=missing_values[0]
  endif
  if arg_present(type) then begin
    type=cds_var_type(*self.p)
  endif
  if arg_present(tname) then begin
    tname=strtrim(cds_var_type(*self.p, /name), 2)
  endif
  if arg_present(output_targets) then begin
    output_targets = dsproc_get_var_output_targets(*self.p)
  endif
  if arg_present(att_names) then begin
    att_names = cds_att_names(*self.p)
  endif
  if arg_present(dim_names) then begin
    dim_names = dsproc_var_dim_names(*self.p)
  endif
  if arg_present(base_time) then begin
    base_time = dsproc_get_base_time(*self.p)
  endif
  if arg_present(time_range) then begin
    if dsproc_get_time_range(*self.p, start_time, end_time) then begin
      time_range = [total(start_time*[1, 1d-6]), total(end_time*[1, 1d-6])]
    endif else time_range = 0
  endif
  if n_elements(extra) then begin
    ; let the superclass handle extra
    self->CDSObject::GetProperty, _strict_extra=extra
  endif
end
;+
; Public properties (set)
;-
pro CDSVar::SetProperty, data=data
  compile_opt idl2, logical_predicate

  if n_elements(data) gt 0 then begin
    vartype=cds_var_type(*self.p)
    ; If the data is type scalar convert to an 
    ; array because dsproc_set_var_data only
    ; accepts arrays.
    if isa(data, /scalar) then begin
        data = [data]
    endif 
    if size(data, /type) ne vartype then begin
      ; convert to target type, if possible
      res=dsproc_set_var_data(*self.p, 0, fix(data, type=vartype))
    endif else begin
      res=dsproc_set_var_data(*self.p, 0, data)
    endelse
  endif
end

;+
; Class definition routine
;-
pro CDSVar__define
  compile_opt idl2, logical_predicate

  s={ CDSVar $
    , inherits CDSObject $
    }
end
