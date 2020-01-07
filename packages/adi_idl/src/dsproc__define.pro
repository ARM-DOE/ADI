;+
; @file_comments
;  Object class front end to dsproc library.
; @property status {type=int}
;  An integer status returned by the last executed dsproc library call.
; @property debug {type=int}
;  An integer debug level. Zero is default, meaning no debug info is printed.
; @property data_home {type=string}
;  Fully qualified path to the DATA_HOME directory used by the library.
; @property datastream_data {type=string}
;  Fully qualified path to the DATASTREAM_DATA directory.
; @property conf_data {type=string}
;  Fully qualified path to the CONF_DATA directory.
; @property logs_data {type=string}
;  Fully qualified path to the LOGS_DATA directory.
; @property site {type=string}
;  The site name for the current process.
; @property facility {type=string}
;  The facility name for the current process.
; @property name {type=string}
;  The name of the current process.
; @property userdata
;  A variable of any type, usage defined by the caller.
;-

;+
; Initializes the dsproc object.
; Access to libdsproc3.
; @returns
;  1=success <br> 0=failure
;-
function dsproc::Init
  compile_opt idl2, logical_predicate
  if !version.release lt '8.2' && !version.memory_bits eq 64 then begin
    message, 'Unsupported IDL version, try "idl -32"'
  endif
  self.userdata=ptr_new(!null)
  self.hook=hash()
  return, 1
end
;+
; Initialize a data system process.
;
; This function will:
; 
;  <br> - Parse the command line
;  <br> - Connect to the database
;  <br> - Open the process log file
;  <br> - Initialize the mail messages
;  <br> - Update the process start time in the database
;  <br> - Initialize the signal handlers
;  <br> - Define non-standard unit symbols
;  <br> - Get process configuration information from database
;  <br> - Initialize input and output datastreams
;  <br> - Initialize the data retrival structures
;<br> 
; The database connection will be left open when this
; function completes to allow the user's init_process() 
; function to access the database without the need to
; reconnect to it. The database connection should be
; closed after the user's init_process() function completes.
;<br> 
; The program will terminate inside this function if
; the -h (help) or -v (version) options are specified
; on the command line (exit value 0), or if an error
; occurs (exit value 1).
;
; @Param proc_model {in}{required}
;  Either an integer or one of the following strings:
;<br>    PM_GENERIC
;<br>    PM_RETRIEVER_VAP
;<br>    PM_TRANSFORM_VAP
;<br>    PM_INGEST
; @keyword alias {in}{optional}{type=string}{default='dsdb_data'}
;  Specify the .db_connect alias to use.
; @keyword proc_version {in}{required}{type=string}
;  A version string, or !null if unknown.
; @keyword proc_names {in}{required}{type=string}
;  A scalar or array of strings specifying the names of the procedures.
; @keyword begin_date {in}{optional}
;  A date string 'YYYYMMDD[HH]' or a double precicion
;  julian date
; @keyword end_date {in}{optional}
;  A date string 'YYYYMMDD[HH]' or a double precicion
;  julian date
; @Keyword site {in}{optional}{type=string}
;  A scalar string specifying the site name.
; @Keyword facility {in}{optional}{type=string}
;  A scalar string specifying the facility name.
; @Keyword debug {in}{optional}{type=int}
;  Integer specifying the debug level.
; @Keyword provenance {in}{optional}
;  Either string or integer specifying the provenance logging level.
; @keyword reprocess {in}{optional}{type=boolean}{default=false}
;  Set this keyword to enable reprocessing (overwrite output)
; @keyword argv {in}{optional}{type=string}
;  String array of command line options. For example:
;   argv=command_line_args()
;   ds=dsproc()
;   ds.initialize, proc_model, argv=argv
;  Note that if this keyword is specified, then it is the
;  caller's responsibility to avoid conflicts any of the
;  above keywords. Keywords specified in the argv style are
;  simply appended to the keywords specified above.
;-
pro dsproc::initialize, proc_model, $
    site=site, facility=facility, alias=alias, debug=debug, $
    provenance=provenance, reprocess=reprocess, $
    begin_date=begin_date, end_date=end_date, $
    proc_names=in_proc_names, proc_version=proc_version, $
    argv=in_argv
  compile_opt idl2, logical_predicate
  ; build argument list, executable name is expected first
  argv=['idl']
  if keyword_set(site) then argv=[argv, '-s', site]
  if keyword_set(facility) then argv=[argv, '-f', facility]
  if keyword_set(begin_date) then argv=[argv, '-b', begin_date]
  if keyword_set(end_date) then argv=[argv, '-e', end_date]
  if keyword_set(alias) then argv=[argv, '-a', alias]
  if keyword_set(debug) then argv=[argv, '-D', debug]
;  if n_elements(debug) then argv=[argv, '-D', strtrim(debug, 2)] 
;  else argv=[argv, '-D', strtrim(self.debug, 2)]
  if n_elements(provenance) then argv=[argv, '-P', strtrim(provenance, 2)]
  if keyword_set(reprocess) then argv=[argv, '-R']
  if n_elements(in_proc_names) eq 1 then argv=[argv, '-n', in_proc_names]
  if n_elements(in_proc_names) gt 1 then proc_names=in_proc_names
  if n_elements(in_proc_names) eq 0 then proc_names=!null

  if keyword_set(in_argv) then begin
    argv=[argv, in_argv]
  endif

  if self.debug then print, 'Calling dsproc_initialize '+strjoin(argv, ' ')
  dsproc_initialize, argv, proc_model, proc_version, proc_names
  ; make sure debug level is in sync
  self.debug=dsproc_get_debug_level()
  self.hasInit=1
end
;+
; Disconnect from the database.
;
; To insure the database connection is not held open longer
; than necessary it is important that every call to
; dsproc_db_connect() is followed by a call to dsproc_db_disconnect().
;-
pro dsproc::db_disconnect
  compile_opt idl2, logical_predicate
  if self.debug then print, 'Calling dsproc_db_disconnect'
  dsproc_db_disconnect
end
;+
; Start a processing interval loop.
;
; This function will:
;
;    check if the process has (or will) exceed the maximum run time.
;    determine the begin and end times of the next processing interval.
;
; Status:
;    error status will be updated to 0 if processing is complete
;    and to 1 otherwise
;
; @Returns
;    interval - output: begin and end time of the processing interval
;                       times are seconds since Jan 1, 1970, UTC.
;    !null if processing is complete.
;-
function dsproc::start_processing_loop
  compile_opt idl2, logical_predicate
  if self.debug then print, 'Caling dsproc_start_processing_loop'
  self.status=dsproc_start_processing_loop(interval)
  if self.status ne 1 then interval = !null
  return, interval
end
;+
; Get input data using retriever information.
;
; If an error occurs in this function it will
; be appended to the log and error mail messages,
; and the process status will be set appropriately.
;
; Status:
;    1 if successful or a retriever definition is not defined
;    0 if a required variable could not be found for the
;      requested processing interval.
;    -1 if an error occurred
; @Param interval {in}{required}
;  An array of 2 integers, [begin_time, end_time], in seconds
;  since Jan 1, 1970. Note that systime passes this time back as
;  floating point, but this function will truncate to nearest
;  integer if floating points are supplied.
; @returns
;  A CDSGroup object reference or !null.
;-
function dsproc::retrieve_data, interval
  compile_opt idl2, logical_predicate
  if self.debug then begin
    print, 'Caling dsproc_retrieve_data'
    print, 'Begin time:', systime(0, interval[0])
    print, 'End time:', systime(0, interval[1])
  endif
  self.status=dsproc_retrieve_data(interval, pCDSGroup)
  if self.debug then help, pCDSGroup
  if self.status eq 1 then begin
    CDSGroup=obj_new('CDSGroup', pCDSGroup)
  endif else CDSGroup=!null
  if self.contOnError eq 0 && self.status eq -1 then begin
    message, 'An error occurred during data retrieval'
  endif
  return, CDSGroup
end
;+
; Merge observations in the _DSProc->ret_data group.
;
; If an error occurs in this function it will be appended to the log and error mail messages, and the process status will be set appropriately.
;
; Status:
;     1 if successful
;     0 if an error occurred
;-
pro dsproc::merge_retrieved_data
  compile_opt idl2, logical_predicate
  if self.debug then print, 'Calling dsproc_merge_retrieved_data'
  self.status=dsproc_merge_retrieved_data()
  if self.contOnError eq 0 && self.status eq 0 then begin
    message, 'An error occurred in dsproc_merge_retrieved_data'
  endif
end
;+
; Run the data transformation logic.
;
; This function will transform the retrieved
; variable data to the coordinate systems
; specified in the retriever definition.
;
; If an error occurs in this function it will
; be appended to the log and error mail messages,
; and the process status will be set appropriately.
;
; Status:
;
;    1 if successful or there was no retrieved data to transform
;    0 if the transformation logic could not be run for the current processing interval.
;    -1 if an error occurred
;
; @Returns
;    A reference to the trans_data CDSGroup, or !null.
;-
function dsproc::transform_data
  compile_opt idl2, logical_predicate
  CDSGroup=!null
  if self.debug then print, 'Calling dsproc_transform_data'
  self.status=dsproc_transform_data(pCDSGroup)
  if self.status eq 1 then CDSGroup=obj_new('CDSGroup', pCDSGroup)
  if self.contOnError eq 0 && self.status eq -1 then begin
    message, 'An error occurred in dsproc_transform_data', /traceback
  endif
  return, CDSGroup
end

;+
; Get an output dataset.
;<br>
; This function will return a reference to the output dataset for the specifed
; datastream and observation. The obs_index should always be zero unless
; observation based processing is being used. This is because all input
; observations should have been merged into a single observation in the output
; datasets.
;
; @param ds_id {in}{required}{type=int}
;  Output datastream ID.
; @param obs_index {in}{optional}{type=int}{default=0}
;  The index of the obervation to get the dataset for.
; @returns
;  dataset - object reference to the output dataset <br>
;  null object reference - if it does not exist
;-
function dsproc::get_output_dataset, ds_id, obs_index
  compile_opt idl2, logical_predicate

  oi = keyword_set(obs_index) ? obs_index : 0
  ptr = dsproc_get_output_dataset(ds_id, oi)
  res = ptr ? obj_new('CDSGroup', ptr) : obj_new()
  return, res
end
;+
; Get a retrieved dataset.
;<br>
; This function will return a pointer to the retrieved dataset for the
; specifed datastream and observation.
;<br>
; The obs_index is used to specify which observation to get the dataset
; for. This value will typically be zero unless this function is called
; from a post_retrieval_hook() function, or the process is using
; observation based processing. In either of these cases the retrieved
; data will contain one "observation" for every file the data was read
; from on disk.
;<br>
; It is also possible to have multiple observations in the retrieved data
; when a pre_transform_hook() is called if a dimensionality conflict
; prevented all observations from being merged.
;
; @param ds_id {in}{type=int}{required}
;  Input datastream ID.
; @param obs_index {in}{type=int}{optional}{default=0}
;  Observation index (0 based indexing)
; @returns
;  dataset - reference to the retrieved dataset <br>
;  NULL object reference, if it does not exist
;-
function dsproc::get_retrieved_dataset, ds_id, obs_index
  compile_opt idl2, logical_predicate

  oi = keyword_set(obs_index) ? obs_index : 0
  ptr = dsproc_get_retrieved_dataset(ds_id, oi)
  res = ptr ? obj_new('CDSGroup', ptr) : obj_new()
  return, res
end
;+
; Get a transformed dataset.
;<br>
; This function will return a pointer to the transformed dataset for the
; specifed coordinate system, datastream, and observation. The obs_index
; should always be zero unless observation based processing is being used.
; This is because all input observations should have been merged into a
; single observation in the transformed datasets.
;
; @param coordsys_name {in}{required}{type=string}
;  The name of the coordinate system as specified in the retriever definition
;  or !NULL if a coordinate system name was not specified.
; @param ds_id {in}{required}{type=int}
;  Input datastream ID.
; @param obs_index {in}{optional}{type=int}{default=0}
;  The index of the obervation to get the dataset for.
; @returns
;  Object reference to the output dataset. <br>
;  NULL object reference if it does not exist
;-
function dsproc::get_transformed_dataset, coordsys_name, ds_id, obs_index
  compile_opt idl2, logical_predicate

  oi = keyword_set(obs_index) ? obs_index : 0
  ptr = dsproc_get_transformed_dataset(coordsys_name, ds_id, oi)
  res = ptr ? obj_new('CDSGroup', ptr) : obj_new()
  return, res
end

;+
;Set user command line options.  this funciton must be called 
;before calling dsproc::main.
;<br>
; @param short_opt {in}{required}{type=string}
;  Short options are single letters and are prefixed by a single 
;  dash on the command line. Multiple short options can be grouped 
;  behind a single dash. Specify !NULL for this argument if a 
;  short option should not be used.
; @param long_opt {in}{required}{type=string}
;  Long options are prefixed by two consecutive dashes on the 
;  command line. Specify !NULL for this argument if a long option 
;  should not be used. 
; @param arg_name {in}{required}{type=string}
;  A single word description of the option argument to be used 
;  in the help message, or !NULL if this option does not take 
;  an argument on the command line. 
; @param opt_desc {in}{required}{type=string}
;  A brief description of the option to be used in the help message.
; @returns
;  1 if successful
;  0 if the option has already been used, or a memory error occurred
;-
function dsproc::setopt, short_opt, long_opt, arg_name, opt_desc
  compile_opt idl2, logical_predicate
  res = dsproc_setopt(short_opt, long_opt, arg_name, opt_desc)
  return, res
end

;+
;Get the value for the specified option.
;<br>
;This function will get the value for the specified long or short
;form command line option, setting the passed 'value' variable to
;the value of the option, or "" if the option has no value.
;
; @param option {in}{required}{type=string}
;  The long or short form option to get.  "s" corresponds to "-s"
;  on the command line, while "long-option" corresponds to
;  "--long-option" on the command line.
; @param value {in}{required}{type=string}
;  The variable to hold the value for the option.  'value' will
;  be set to the option's value, or to "" if the option did not
;  have a value specified (e.g. the command line string
;  "command -v --long -f filename" would have "" for the options "v"
;  and "long", and a value of "filename" for the option "f").
; @returns
;  1 if the option was specified on the command line
;  0 if the option was not specified on the command line
;-
function dsproc::getopt, option, value
  compile_opt idl2, logical_predicate
  res = dsproc_getopt(option, value)
  return, res
end

;+
; Free all internally stored command line options and their values.
;-
pro dsproc::freeopts
  compile_opt idl2, logical_predicate

  dsproc_freeopts
end



;+
; Create all output datasets.
;
; If an error occurs in this function it will be
; appended to the log and error mail messages,
; and the process status will be set appropriately.
;
; Status:
;
;    1 if successful
;    0 if an error occurred
;-
pro dsproc::create_output_datasets
  compile_opt idl2, logical_predicate
  if self.debug then print, 'Calling dsproc_create_output_datasets'
  self.status=dsproc_create_output_datasets()
  if self.contOnError eq 0 && self.status eq 0 then begin
    message, 'An error occurred in dsproc_create_output_datasets', /traceback
  endif
end

;+
; Create an output dataset.
;
; If an error occurs in this function it will be
; appended to the log and error mail messages,
; and the process status will be set appropriately.
;
; @param ds_id {in}{required}{type=int}
;  Output datastream ID.
; @param data_time {in}{required}{type=long64}
;  The time of the data being processed.  
;  This is used to determine the DOD version and set 
;  time varying attribute values.
; @keyword set_location {in}{required}{type=boolean}
;  Specifies if locaiton should be set using process 
;  location defined in database, (1 =  true, 0 = false)
; @returns
;  dataset - object reference to the output dataset <br>
;  null object reference - if it does not exist
;-
function dsproc::create_output_dataset, ds_id, data_time, set_location=set_location
  compile_opt idl2, logical_predicate

  ptr = dsproc_create_output_dataset(ds_id, data_time,keyword_set(set_location))
  res = ptr ? obj_new('CDSGroup', ptr) : obj_new()
  return, res
end

;+
; Store all output datasets.
;
; If an error occurs in this function
; it will be appended to the log and
; error mail messages, and the process
; status will be set appropriately.
;
; Status:
;
;        1 if successful
;        0 if an error occurred
;-
pro dsproc::store_output_datasets
  compile_opt idl2, logical_predicate
  self.status=dsproc_store_output_datasets()
  if self.debug then print, 'Calling dsproc_store_output_datasets'
  if self.contOnError eq 0 && self.status eq 0 then begin
    message, 'An error occurred in dsproc_store_output_datasets', /traceback
  endif
end

;+
; Store an output dataset.
;
; This function will:
;
;  - Filter out duplicate records and verify records are
;    in chronological order.
;  - Filter NaN and Inf values if variables have a missing value
;    attribute defined and the DS_FILTER_NANS flag is set.
;    This should only be used if the DS_STANDARD_QC flag is set,
;    or if the dataset has no QC variables defined.
;  - Apply standard missing value, min, max, and delta QC 
;    checks if the DS_STANDARD_QC flag is set.
;  - Verify that none of the record times are in the future.
;  - Merge datasets with existing files and split on defined 
;    intervals or when metadata values change.
;<br>
; If an error occurs in this function it will be
; appended to the log and error mail messages,
; and the process status will be set appropriately.
;
; @param ds_id {in}{required}{type=int}
;  Output datastream ID.
; @keyword newfile {in}{required}{type=boolean}
;  Specifies if a new file should be created.(1 =  true, 0 = false)
; @returns
;        The number of data samples stored
;<br>
;        0 if no data found in dataset or if all data
;          was duplicates of previously stored data
;<br>
;        -1 if an error occurred
;-
function dsproc::store_dataset, ds_id, newfile=newfile
  compile_opt idl2, logical_predicate

  return, dsproc_store_dataset(ds_id, keyword_set(newfile))

end
;+
; Map data from input to output datsaets.
;<br>
; This function will map all input data to all output datsets
; if an output dataset is not specified. In this case the output 
; datsets will be created as necessary.
;<br>
; If an error occurs in this function it will be appended to the log and error
; mail messages, and the process status will be set appropriately.
;
; @param in_dataset {in}{required}{type=CDSGroup}
;  A reference to the data group to map from.
; @param out_dataset {in}{required}{type=CDSGroup}
;  A reference to the data group to map to.
; @param flags {in}{required}{type=int}
;  Reserved for control flags.
; @returns
;  1 if successful <br>
;  0 if and error occurred <br>
;-
function dsproc::map_datasets, in_dataset, out_dataset, flags
  compile_opt idl2, logical_predicate

  ds_inptr = obj_valid(in_dataset) ? in_dataset.ptr : !null
  ds_outptr = obj_valid(out_dataset) ? out_dataset.ptr : !null

  return, dsproc_map_datasets(ds_inptr, ds_outptr, flags)
end

;+
; Set the time range to use in subsequent calls to dspro_map_datasets()
;
; @param begin_time {in}{required}{type=long64}
;  Only map data whose time is greater than or equal to begin_time.
; @param end_time {in}{required}{type=long64}
;  Only map data whose time is less than end_time.
;-
pro dsproc::set_map_time_range, begin_time, end_time
  compile_opt idl2, logical_predicate

  tmp = dsproc_set_map_time_range(begin_time,end_time)

end

;+
; Finish a data system process.
;
; This function will:
;
;  - Update the process status in the database
;  - Log all process stats that were recorded
;  - Disconnect from the database
;  - Mail all messages that were generated
;  - Close the process log file
;  - Free all memory used by the internal DSProc structure
;
; Status:
;    suggested program exit value (0 = success, 1 = failure)
;-
pro dsproc::finish
  compile_opt idl2, logical_predicate
  if self.debug then print, 'Calling dsproc_finish'
  self.status=dsproc_finish()
  self.hasInit=0
end
;+
; Get the ID of a datastream.
;
; @keyword site {in}{optional}{type=string}{default=!null}
;  Site name, or !null to find first match.
; @keyword facility {in}{optional}{type=string}{default=!null}
;  Facility name, or !null to find first match.
; @keyword dsc_name {in}{required}{type=string}
;  Datastream class name.
; @keyword dsc_level {in}{required}{type=string}
;  Datastream class level.
; @keyword input {in}{optional}{type=boolean}{default=false}
;  Specify /INPUT and/or /OUTPUT datastream role.
; @keyword output {in}{optional}{type=boolean}{default=false}
;  Specify /INPUT and/or /OUTPUT datastream role.
; @returns
;  The ID of a datastream, or -1 if an error occurred.
;-
function dsproc::get_datastream_id, site=site, facility=facility, $
    dsc_name=dsc_name, dsc_level=dsc_level, input=input, output=output
  compile_opt idl2, logical_predicate
  if n_elements(site) eq 0 then site=!null
  if n_elements(facility) eq 0 then facility=!null
  if ~isa(dsc_name, 'string') then message, 'DSC_NAME must be a string'
  if ~isa(dsc_level, 'string') then message, 'DSC_LEVEL must be a string'
  if (keyword_set(input) xor keyword_set(output)) eq 0 then begin
    message, 'Either /INPUT or /OUTPUT must be used.'
  endif
  role = 0
  role or= keyword_set(input)
  role or= keyword_set(output)*2
  return, dsproc_get_datastream_id(site, facility, dsc_name, dsc_level, role)
end
;+
; Get the ID of an input datastream.
;
; @keyword dsc_name {in}{required}{type=string}
;  Datastream class name.
; @keyword dsc_level {in}{required}{type=string}
;  Datastream class level.
; @returns
;  The ID of a datastream, or -1 if an error occurred.
;-
function dsproc::get_input_datastream_id, dsc_name=dsc_name, $
    dsc_level=dsc_level
  compile_opt idl2, logical_predicate
  if ~isa(dsc_name, 'string') then message, 'DSC_NAME must be a string'
  if ~isa(dsc_level, 'string') then message, 'DSC_LEVEL must be a string'
  return, dsproc_get_input_datastream_id(dsc_name, dsc_level)
end
;+
; Get the IDs of all input datastreams.
;
; @returns
;  An array of IDs, or !null if there was a memory allocation error.
;-
function dsproc::get_input_datastream_ids
  compile_opt idl2, logical_predicate
  ds_ids=!null
  res=dsproc_get_input_datastream_ids(ds_ids)
  return, ds_ids
end
;+
; Get the ID of an output datastream.
;
; @keyword dsc_name {in}{required}{type=string}
;  Datastream class name.
; @keyword dsc_level {in}{required}{type=string}
;  Datastream class level.
; @returns
;  The ID of the output datastream, or -1 if an error occurred.
;-
function dsproc::get_output_datastream_id, dsc_name=dsc_name, $
    dsc_level=dsc_level
  compile_opt idl2, logical_predicate
  if ~isa(dsc_name, 'string') then message, 'DSC_NAME must be a string'
  if ~isa(dsc_level, 'string') then message, 'DSC_LEVEL must be a string'
  return, dsproc_get_output_datastream_id(dsc_name, dsc_level)
end
;+
; Get the IDs of all outpu datastreams.
;
; @returns
;  An array of IDs, or !null if there was a memory allocation error.
;-
function dsproc::get_output_datastream_ids
  compile_opt idl2, logical_predicate
  ds_ids=!null
  res=dsproc_get_output_datastream_ids(ds_ids)
  return, ds_ids
end
;+
; Find all files in a datastream directory for a specified time range.
; 
; This function will return a list of all files in a datastream directory 
; containing data for the specified time range. This search wil
; include the begin_time but exclude the end_time. That is, it will find 
; files that include data greter than or equal to the begin time, and 
; less than the end time.
; 
; If the begin_time is not specified, the file containing data for the 
; time just prior to the end_time will be returned.
;
; If the end_time is not specified, the file containing data for the time  
; just after the begin_time will be returned.
;
; @param ds_id {in}{required}{type=int}
;   datastream ID
; @keyword begin_time {in}{optional}{type=long64}{default=0}
;   begin time in seconds since 1970.
; @keyword end_time {in}{optional}{type=long64}{default=0}
;   end time in seconds since 1970.
;
; @returns
;  An array of files, or !null if there was a memory allocation error.
;-
function dsproc::find_datastream_files, ds_id, $
  begin_time=begin_time, $
  end_time=end_time
  compile_opt idl2, logical_predicate
  ds_files=!null

  t_begin = keyword_set(begin_time) ? begin_time : 0
  t_end = keyword_set(end_time) ? end_time : 0

  res=dsproc_find_datastream_files(ds_id, t_begin, t_end, ds_files)

  return, res

end
;+
; Get the list of files in a datastream directory 
;
; @param ds_id {in}{required}{type=int}
;   datastream ID
;
; @returns
;  An array of files, or 0
;-
function dsproc::get_datastream_files, ds_id
  compile_opt idl2, logical_predicate
  ds_files=!null

  res=dsproc_get_datastream_files(ds_id, ds_files)

  if res le 0  then ds_files = 0

  return, ds_files
end
;+
; Set the path to the datastream directory
;
; Default datastream path set if path = NULL
;    dsenv_get_collection_dir() for level 0 input datastreams
;    dsenv_get_datastream_dir() for all other datastreams
;
; If an error occurs in this function it will be appended
; to the log and error mail messages, and the process
; status will be set appropriately.
;
; @param ds_id {in}{required}{type=int}
;   datastream ID
; @param path {in}{required}{type=string}
;   Path to the datastream directory
;
; @returns
;  1 if successful
;  0 if an error occurs
;-
function dsproc::set_datastream_path, ds_id, path
  compile_opt idl2, logical_predicate
  if ~isa(path, 'string') then message, 'PATH must be a string'

  res=dsproc_set_datastream_path(ds_id, path)
  return, res
end
;+
; Unset the control flags for a datastream.
;<br>
; @param ds_id {in}{required}{type=int}
;  Datastream ID.
; @keyword ds_standard_qc {in}{optional}{type=boolean}{default=false}
;  Apply standard QC before storing.
; @keyword ds_filter_nans {in}{optional}{type=boolean}{default=false}
;  Replace NaN and Inf values with missing values before storing.
; @keyword ds_overlap_check {in}{optional}{type=boolean}{default=false}
;  Check for overlap wiht previously processed data.
;  This flag is igored if run with -R reprocessing mode.
; @keyword ds_preserve_obs {in}{optional}{type=boolean}{default=false}
;  Preserve distinct observations when retrieving and storing data.
;  Only obs that start within the current processing interval are stored.
; @keyword ds_disable_merge {in}{optional}{type=boolean}{default=false}
;  Do not merge multiple observations in retrieved data.
;  Only data for the current processing interval are read in.
; @keyword ds_skip_transform {in}{optional}{type=boolean}{default=false}
;  Skip the transformation logic for all variables in this datastream.
; @keyword ds_obs_loop {in}{optional}{type=boolean}{default=false}
;  Loop over observations, or files, instead of time intervals.
; @keyword ds_scan_mode {in}{optional}{type=boolean}{default=false}
;  Enable scan mode for datastreams that are not expected to be continuous.
;-
pro dsproc::unset_datastream_flags, ds_id, $
  ds_standard_qc=ds_standard_qc, $
  ds_filter_nans=ds_filter_nans, $
  ds_overlap_check=ds_overlap_check, $
  ds_preserve_obs=ds_preserve_obs, $
  ds_disable_merge= ds_disable_merge, $
  ds_skip_transform= ds_skip_transform, $
  ds_obs_loop= ds_obs_loop, $
  ds_scan_mode= ds_scan_mode
  compile_opt idl2, logical_predicate

  flags = ''
  if keyword_set(ds_standard_qc) then $
    flags or= 'DS_STANDARD_QC'
  if keyword_set(ds_filter_nans) then $
    flags or= 'DS_FILTER_NANS'
  if keyword_set(ds_overlap_check) then $
    flags or= 'DS_OVERLAP_CHECK'
  if keyword_set(ds_preserve_obs) then $
    flags or= 'DS_PRESERVE_OBS'
  if keyword_set(ds_disable_merge) then $
    flags or= 'DS_DISABLE_MERGE'
  if keyword_set(ds_skip_transform) then $
    flags or= 'DS_SKIP_TRANSFORM'
  if keyword_set(ds_obs_loop) then $
    flags or= 'DS_OBS_LOOP'
  if keyword_set(ds_scan_mode) then $
    flags or= 'DS_SCAN_MODE'

  tmp = dsproc_unset_datastream_flags(ds_id,flags)

end
;+
; Obtain information about datastream.
;
; @param ds_id {in}{required}{type=long}
;  An IDL long integer containing the datastream ID.
; @returns
;  A structure containing:
;    name: the fully qualified datastream name
;    site: the datastream site
;    facility: the datastream facility
;    class_name: the datastream class name
;    class_level: the datastream class level
;-
function dsproc::datastream_info, ds_id
  compile_opt idl2, logical_predicate
  res={name: '', path: '',site: '', facility: '', class_name: '', class_level: ''}
  s=list()
  s->Add, dsproc_datastream_name(ds_id)
  s->Add, dsproc_datastream_path(ds_id)
  s->Add, dsproc_datastream_site(ds_id)
  s->Add, dsproc_datastream_facility(ds_id)
  s->Add, dsproc_datastream_class_name(ds_id)
  s->Add, dsproc_datastream_class_level(ds_id)
  for i=0, n_elements(s)-1 do if s[i] ne !null then res.(i) = s[i]
  return, res
end
;+
; Set the control flags for a variable.
;<br>
; @param ds_id {in}{required}{type=int}
;  Datastream ID.
; @keyword ds_standard_qc {in}{optional}{type=boolean}{default=false}
;  Apply standard QC before storing.
; @keyword ds_filter_nans {in}{optional}{type=boolean}{default=false}
;  Replace NaN and Inf values with missing values before storing.
; @keyword ds_overlap_check {in}{optional}{type=boolean}{default=false}
;  Check for overlap wiht previously processed data.
;  This flag is igored if run with -R reprocessing mode.
; @keyword ds_preserve_obs {in}{optional}{type=boolean}{default=false}
;  Preserve distinct observations when retrieving and storing data.
;  Only obs that start within the current processing interval are stored.
; @keyword ds_disable_merge {in}{optional}{type=boolean}{default=false}
;  Do not merge multiple observations in retrieved data.
;  Only data for the current processing interval are read in.
; @keyword ds_skip_transform {in}{optional}{type=boolean}{default=false}
;  Skip the transformation logic for all variables in this datastream.
; @keyword ds_obs_loop {in}{optional}{type=boolean}{default=false}
;  Loop over observations, or files, instead of time intervals.
; @keyword ds_scan_mode {in}{optional}{type=boolean}{default=false}
;  Enable scan mode for datastreams that are not expected to be continuous.
;-
pro dsproc::set_datastream_flags, ds_id, $
  ds_standard_qc=ds_standard_qc, $
  ds_filter_nans=ds_filter_nans, $
  ds_overlap_check=ds_overlap_check, $
  ds_preserve_obs=ds_preserve_obs, $
  ds_disable_merge= ds_disable_merge, $
  ds_skip_transform= ds_skip_transform, $
  ds_obs_loop= ds_obs_loop, $
  ds_scan_mode= ds_scan_mode
  compile_opt idl2, logical_predicate

  flags = ''
  if keyword_set(ds_standard_qc) then $
    flags or= 'DS_STANDARD_QC'
  if keyword_set(ds_filter_nans) then $
    flags or= 'DS_FILTER_NANS'
  if keyword_set(ds_overlap_check) then $
    flags or= 'DS_OVERLAP_CHECK'
  if keyword_set(ds_preserve_obs) then $
    flags or= 'DS_PRESERVE_OBS'
  if keyword_set(ds_disable_merge) then $
    flags or= 'DS_DISABLE_MERGE'
  if keyword_set(ds_skip_transform) then $
    flags or= 'DS_SKIP_TRANSFORM'
  if keyword_set(ds_obs_loop) then $
    flags or= 'DS_OBS_LOOP'
  if keyword_set(ds_scan_mode) then $
    flags or= 'DS_SCAN_MODE'

  tmp = dsproc_set_datastream_flags(ds_id,flags)

end
;+
; Add datastream file patterns.
;<br>
; This function adds file patterns to look for when creating
; the list of files n the datastream directory. By default all
; files in the directory will be listed.  
; 
; @param ds_id {in}{required}{type=int}
; Output datastream ID.
; @param npatterns {in}{required}{type=int}
; Number of file patterns.
; @param patterns {in}{optional}{type=string}
; String array of file patterns.
; @param ignore_case {in}{required}{type=int}
; Ignore case in file patterns
; @returns
; 1 if successful
; 0 if an error occured
;-
function dsproc::add_datastream_file_patterns, ds_id, $
     npatterns, patterns, ignore_case
  compile_opt idl2, logical_predicate

  res = dsproc_add_datastream_file_patterns(ds_id, npatterns, $
        patterns, ignore_case)
  return, res
end 
;+
; Set the datastream file extension.
;
; @param ds_id {in}{required}{type=int}
; Output datastream ID.
; @param extension {in}{required}{type=string}
; File extension
;-
pro dsproc::set_datastream_file_extension, ds_id, extension
  compile_opt idl2, logical_predicate
  if ~isa(extension, 'string') then message, 'EXTENSION must be a string'

  tmp = dsproc_set_datastream_file_extension(ds_id,extension)

end
;+
; Set the file splitting mode for output files.
; If split_mode is set to SPLIT_ON_STORE the 
; split_start and split_interval values should 
; be set to zero.
; Default for VAPs: always create a new file when data is stored
;    split_mode = SPLIT_ON_STORE
;    split_start = ignored
;    split_interval = ignored
; Default for Ingests: daily files that split at midnight 
;    split_mode = SPLIT_ON_HOURS
;    split_start = 0
;    split_interval = 24
;
; @param ds_id {in}{required}{type=int}
; Output datastream ID.
; @param split_start {in}{required}{type=double}
; the start of the split interval (see SplitMode)
; @param split_interval {in}{required}{type=double}
; the split interval (see SplitMode)
; @keyword split_on_store {in}{optional}{type=boolean}{default=false}
;  Specify /STORE, /HOURS, /DAYS, or /MONTHS  split mode.
; @keyword split_on_hours {in}{optional}{type=boolean}{default=false}
;  Specify /STORE, /HOURS, /DAYS, or /MONTHS  split mode.
; @keyword split_on_days {in}{optional}{type=boolean}{default=false}
;  Specify /STORE, /HOURS, /DAYS, or /MONTHS  split mode.
; @keyword split_on_months {in}{optional}{type=boolean}{default=false}
;  Specify /STORE, /HOURS, /DAYS, or /MONTHS  split mode.
;-
pro dsproc::set_datastream_split_mode, ds_id, $
     split_on_store=split_on_store, $ 
     split_on_hours=split_on_hours, $ 
     split_on_days=split_on_days, $ 
     split_on_months=split_on_months, $ 
     split_start, split_interval
  compile_opt idl2, logical_predicate

  split_mode = ''
  if keyword_set(split_on_store) then $
    split_mode or= 'SPLIT_ON_STORE'
  if keyword_set(split_on_hours) then $
    split_mode or= 'SPLIT_ON_HOURS'
  if keyword_set(split_on_days) then $
    split_mode or= 'SPLIT_ON_DAYS'
  if keyword_set(split_on_months) then $
    split_mode or= 'SPLIT_ON_MONTHS'

  tmp = dsproc_set_datastream_split_mode(ds_id,split_mode, split_start, split_interval)

end
;+
; Get a variable from an output dataset.
;
; The obs_index should always be zero unless
; observation based processing is being used.
; This is because all input observations should
; have been merged into a single observation in
; the output datasets.
;
; @param ds_id {in}{required}{type=int}
;  Output datastream ID.
; @param var_name {in}{required}{type=string}
;  Variable name.
; @param obs_index {in}{required}{type=int}
;  The index of the observation to get the dataset for.
; @returns
;  A valid object reference to a CDSVar object or a null object
;  reference if it does not exist.
;-
function dsproc::get_output_var, ds_id, var_name, obs_index
  compile_opt idl2, logical_predicate

  ptr=dsproc_get_output_var(ds_id, var_name, obs_index)
  if ptr eq 0 then return, obj_new()
  return, obj_new('CDSVar', ptr)
end
;+
; Get a primary variable from the retrieved data.
;
; This function will find a variable in the retrieved data
; that was explicitly requested by the user in the retriever definition.
;<br>
; The obs_index is used to specify which observation to pull the variable
; from. This value will typically be zero unless this function is called
; from a post_retrieval_hook() function, or the process is using observation
; based processing. In either of these cases the retrieved data will contain 
; one "observation" for every file the data was read from on disk.
;<br>
; It is also possible to have multiple observations in the retrieved data
; when a pre_transform_hook() is called if a dimensionality conflict
; prevented all observations from being merged.
; @param var_name {in}{required}{type=string}
;  Variable name.
; @param obs_index {in}{required}{type=int}
;  The index of the observation to get the dataset for.
; @returns
;  A valid object reference to a CDSVar object or a null object
;  reference if not found.
;-
function dsproc::get_retrieved_var, var_name, obs_index
  compile_opt idl2, logical_predicate

  ptr=dsproc_get_retrieved_var(var_name, obs_index)
  if ptr eq 0 then return, obj_new()
  return, obj_new('CDSVar', ptr)
end
;+
; Get a primary variable from the transformed data.
;
; This function will find a variable in the transformed data that was
; explicitly requested by the user in the retriever definition.
;<br>
; The obs_index is used to specify which observation to pull the variable
; from. This value will typically be zero unless the process is using
; observation based processing. If this is the case the transformed data
; will contain one "observation" for every file the data was read from on disk.
;
; @param var_name {in}{required}{type=string}
;  Variable name.
; @param obs_index {in}{optional}{type=long}{default=0}
;  The index of the obervation to get the variable from.
; @returns 
;  Object reference to the transformed variable
;  or NULL object reference if not found
;-
function dsproc::get_transformed_var, var_name, obs_index
  compile_opt idl2, logical_predicate

  if n_params() lt 2 || obs_index eq !null then begin
    ptr=dsproc_get_transformed_var(var_name)
  endif else begin
    ptr=dsproc_get_transformed_var(var_name, obs_index)
  endelse
  if ptr eq 0 then return, obj_new()
  return, obj_new('CDSVar', ptr)
end

;+
; Get a variable from a transformation coordinate system.
;<br>
; Unlike the dsproc_get_transformed_var() function, this function will find
; any variable in the specified transformation coordinate system.
;<br>
; The obs_index is used to specify which observation to pull the variable from.
; This value will typically be zero unless the process is using observation
; based processing. If this is the case the transformed data will contain one
; "observation" for every file the data was read from on disk.
;
; @param coordsys_name {in}{required}{type=string}
;  Coordinate system name.
; @param var_name {in}{required}{type=string}
;  Variable name.
; @param obs_index {in}{optional}{type=int}{default=0}
;  The index of the obervation to get the variable from.
; @returns
;  Object reference to the transformed variable, <br>
;  or, NULL object reference if not found
;-
function dsproc::get_trans_coordsys_var, coordsys_name, var_name, obs_index
  compile_opt idl2, logical_predicate

  if n_params() lt 3 || obs_index eq !null then begin
    ptr = dsproc_get_trans_coordsys_var(coordsys_name, var_name)
  endif else begin
    ptr = dsproc_get_trans_coordsys_var(coordsys_name, var_name, obs_index)
  endelse
  if ptr eq 0 then return, obj_new()
  return, obj_new('CDSVar', ptr)
end

;+
; Generate an error message.
;<br>
; This function will set the status of the process and append an Error message
; to the log file and error mail message.
;<br>
; The status string should be a brief one line error message that will be used
; as the process status in the database. This is the message that will be
; displayed in DSView. If the status string is NULL the error message specified
; by the format string and args will be used.
;<br>
; The msg string will be used to generate the complete and more detailed log
; and error mail messages. If the msg string is NULL the status string will
; be used.
; @param status {in}{required}{type=string}
;  A brief one line error message.
; @keyword msg {in}{optional}{type=string}{default=status}
;  A complete and more detailed error message, the default is to use the
;  status string if this string is not passed in. If present, this should
;  be either a single string, or a string array. In the case of a string
;  array, it is first converted to a multi-line scalar string.
;-
pro dsproc::error, status, msg=msg
  compile_opt idl2, logical_predicate
  if n_elements(status) ne 1 then message, 'STATUS argument must be a scalar'
  if ~keyword_set(msg) then begin
    m=!null
  endif else if n_elements(msg) gt 1 then begin
    m=strjoin(msg, string(10b))
  endif else begin
    m=msg
  endelse

  tr=scope_traceback(/structure)
  if  m eq !null then begin
      dsproc_error, tr[-2].routine, tr[-2].filename, $
      tr[-2].line, status, 'Unknown Data Processing Error'
  endif else begin
      dsproc_error, tr[-2].routine, tr[-2].filename, $
      tr[-2].line, status, m
  endelse

end

;+
; Abort the process and exit cleanly.
;<br>
; This function will set the status of the process, append an Error message 
; to the log file and error mail message, call the user's finish_process function
; (but only if dsproc_abort is not being called from there), and exit the
; process cleanly.
;<br>
; The status string should be a brief one line error message that will be used
; as the process status in the database. This is the message that will be
; displayed in DSView. If the status string is NULL the error message specified
; by the format string and args will be used.
;<br>
; The msg string will be used to generate the complete and more detailed log
; and error mail messages. If the msg string is NULL the status string will
; be used.
; @param status {in}{required}{type=string}
;  A brief one line error message.
; @keyword msg {in}{optional}{type=string}{default=status}
;  A complete and more detailed error message, the default is to use the
;  status string if this string is not passed in. If present, this should
;  be either a single string, or a string array. In the case of a string
;  array, it is first converted to a multi-line scalar string.
;-
pro dsproc::abort, status, msg=msg
  compile_opt idl2, logical_predicate
  if n_elements(status) ne 1 then message, 'STATUS argument must be a scalar'
  if ~keyword_set(msg) then begin
    m=!null
  endif else if n_elements(msg) gt 1 then begin
    m=strjoin(msg, string(10b))
  endif else begin
    m=msg
  endelse

  tr=scope_traceback(/structure)
  if  m eq !null then begin
      dsproc_abort, tr[-2].routine, tr[-2].filename, $
      tr[-2].line, status, 'Unknown Data Processing Error'
  endif else begin
      dsproc_abort, tr[-2].routine, tr[-2].filename, $
      tr[-2].line, status, m
  endelse

end

;+
;  Append a 'Bad File' message to the log file and warnning mail 
;  message.
; @param file {in}{required}{type=string}
;  The name of the bad file
; @keyword msg {in}{optional}{type=string}{default=status}
;  A description of why the file was found to be bad. 
;  If present, this should be either a single string, or a string
;  array. In the case of a string array, it is first converted 
;  to a multi-line scalar string.
;-
pro dsproc::bad_file_warning, file, msg=msg
  compile_opt idl2, logical_predicate
  if ~keyword_set(msg) then begin
    m=!null
  endif else if n_elements(msg) gt 1 then begin
    m=strjoin(msg, string(10b))
  endif else begin
    m=msg
  endelse

  tr=scope_traceback(/structure)
  dsproc_bad_file_warning, tr[-2].routine, tr[-2].filename, tr[-2].line, file, m
end

;+
;  Append a 'Bad Line' message to the log file and warnning mail 
;  message.
; @param file {in}{required}{type=string}
;  The name of the bad file
; @param line_num {in}{required}{type=int}
;  The line number of the bad line
; @keyword msg {in}{optional}{type=string}{default=status}
;  A description of why the line was found to be bad. 
;  If present, this should be either a single string, or a string
;  array. In the case of a string array, it is first converted 
;  to a multi-line scalar string.
;-
pro dsproc::bad_line_warning, file, line_num, msg=msg
  compile_opt idl2, logical_predicate
  if ~keyword_set(msg) then begin
    m=!null
  endif else if n_elements(msg) gt 1 then begin
    m=strjoin(msg, string(10b))
  endif else begin
    m=msg
  endelse

  tr=scope_traceback(/structure)
  dsproc_bad_line_warning, tr[-2].routine, tr[-2].filename, $
       tr[-2].line, file, line_num, m

end

;+
; Generate a debug message.
;<br>
; This function will generate the specified message if debug mode is enabled
; and at a level greater than or equal to the specified debug level of the
; message. For example, if the debug level specifed on the command line is 2
; all debug level 1 and 2 messages will be generated, but the debug
; level 3, 4, and 5 messages will not be.
;
; @param level {in}{required}{type=int}
;  Debug message level [1-5]
; @param msg {in}{required}{type=string}
;  A scalar or array of strings. If an array is passed, newline characters
;  are automatically inserted between elements.
;-
pro dsproc::debug, level, msg
  compile_opt idl2, logical_predicate
  if n_params() lt 2 then message, 'Incorrect number of arguments', /TRACEBACK
  if n_elements(msg) gt 1 then begin
    m=strjoin(msg, string(10b))
  endif else begin
    m=msg
  endelse

  tr=scope_traceback(/structure)
  dsproc_debug, tr[-2].routine, tr[-2].filename, tr[-2].line, level, m
end
;+
; Append a message to the log file.
;
; @param msg {in}{type=string}{required}
;  The string message to be added, if an array is passed, newline
;  characters are appended between elements.
;-
pro dsproc::log, msg
  compile_opt idl2, logical_predicate
  if n_elements(msg) gt 1 then begin
    m=strjoin(msg, string(10b))
  endif else begin
    m=msg
  endelse

  tr=scope_traceback(/structure)
  dsproc_log, tr[-2].routine, tr[-2].filename, tr[-2].line, m
end

;+
; Send a mail message to the mentor of the software or data.
;
; @param msg {in}{type=string}{required}
;  The string message to be sent, if an array is passed, newline
;  characters are appended between elements.
;-
pro dsproc::mentor_mail, msg
  compile_opt idl2, logical_predicate
  if n_elements(msg) gt 1 then begin
    m=strjoin(msg, string(10b))
  endif else begin
    m=msg
  endelse

  tr=scope_traceback(/structure)
  dsproc_mentor_mail, tr[-2].routine, tr[-2].filename, tr[-2].line, m
end
;+
; Dump all output datasets to text files.
;<br>
; This function will dump all output datasets to a text file with
; the following names:
;<br><br>
;   datastream.YYYYMMDD.hhmmss.suffix
;<br><br>
; where:<br>
;  datastream = the output datastream name<br>
;  YYYYMMDD = the date of the first sample in the dataset<br>
;  hhmmss = the time of the first sample in the dataset<br>
;  suffix = the user specified suffix in the function call<br>
;<br>
; If an error occurs in this function it will be appended to the log and error
; mail messages, and the process status will be set appropriately.
;
; @param outdir {in}{required}{type=string}
;  The output directory.
; @param suffix {in}{optional}{type=string}{default=''}
;  The suffix portion of the file name.
; @keyword flags {in}{optional}{type=int}{default=0}
;  Reserved for control flags.
; @returns
;  1 if successful <br>
;  0 if and error occurred <br>
;-
function dsproc::dump_output_datasets, outdir, suffix, flags=flags
  compile_opt idl2, logical_predicate
  
  fl = (n_elements(flags) eq 0) ? 0 : flags
  sf = (n_elements(suffix) eq 0) ? '' : suffix
  return, dsproc_dump_output_datasets(outdir, sf, fl)
end
;+
; Dump all retrieved datasets to a text file.
;<br>
; This function will dump all retrieved data to a text file with the following
; name:
;<br><br>
; {site}{process}{facility}.YYYYMMDD.hhmmss.suffix
;<br><br>
; where: <br>
; site = the site name as specified on the command line <br>
; process = the name of the process as defined in the database <br>
; facility = the facility name as specified on the command line <br>
; YYYYMMDD = the start date of the current processing interval <br>
; hhmmss = the start time of the current processing interval <br>
; suffix = the user specified suffix in the function call <br>
;<br>
; If an error occurs in this function it will be appended to the log and error
; mail messages, and the process status will be set appropriately.
;
; @param outdir {in}{required}{type=string}
;  The output directory.
; @param suffix {in}{optional}{type=string}{default=''}
;  The suffix portion of the file name.
; @keyword flags {in}{optional}{type=int}{default=0}
;  Reserved for control flags.
; @returns
;  1 if successful <br>
;  0 if and error occurred <br>
;-
function dsproc::dump_retrieved_datasets, outdir, suffix, flags=flags
  compile_opt idl2, logical_predicate

  fl = (n_elements(flags) eq 0) ? 0 : flags
  sf = (n_elements(suffix) eq 0) ? '' : suffix
  return, dsproc_dump_retrieved_datasets(outdir, sf, fl)
end

;+
; Dump all transformed datasets to a text file.
;<br>
; This function will dump all transformed data to a text file with the
; following name:
;<br><br>
; {site}{process}{facility}.YYYYMMDD.hhmmss.suffix
;<br><br>
; where: <br>
; site = the site name as specified on the command line <br>
; process = the name of the process as defined in the database <br>
; facility = the facility name as specified on the command line <br>
; YYYYMMDD = the start date of the current processing interval <br>
; hhmmss = the start time of the current processing interval <br>
; suffix = the user specified suffix in the function call <br>
;<br>
; If an error occurs in this function it will be appended to the log and
; error mail messages, and the process status will be set appropriately.
;
; @param outdir {in}{required}{type=string}
;  The output directory.
; @param suffix {in}{optional}{type=string}{default=''}
;  The suffix portion of the file name.
; @keyword flags {in}{optional}{type=int}{default=0}
;  Reserved for control flags.
; @returns
; 1 if successful <br>
; 0 if and error occurred
;-
function dsproc::dump_transformed_datasets, outdir, suffix, flags=flags
  compile_opt idl2, logical_predicate

  sf = (n_elements(suffix) eq 0) ? '' : suffix
  fl = (n_elements(flags) eq 0) ? 0 : flags

  return, dsproc_dump_transformed_datasets(outdir, sf, fl)
end
;+
; Get the time remaining until the max run time is reached.
;
; @returns
;   time remaining until the max run time is reached
;   0 if the max run time has been exceeded
;   -1 if the max run time has not been set
;
;-
function dsproc::get_time_remaining
  compile_opt idl2, logical_predicate

  return, dsproc_get_time_remaining()
end
;+
; Get the process max run time.
;
; @returns
;   max run time
;
;-
function dsproc::get_max_run_time
  compile_opt idl2, logical_predicate

  return, dsproc_get_max_run_time()
end


;+
; Set the input directory used to create the input_source attribute
; 
; This function will set the full path to the input directory used 
; to create the input_source global attribute value when new 
; datasets are created. This value will only be set in datasets
; that have the input_source attribute defined.
;
; @param input_dir {in}{required}{type=string}
;   The full path to the input directory.
;
;-
pro dsproc::set_input_dir, input_dir
  compile_opt idl2, logical_predicate

  tmp = dsproc_set_input_dir(input_dir)
end

;+
; Set the input source string to use when new datasets are created.
; 
; This function will set the string to use for the input_source 
; global attribute value when new datasets are created.
; This value will only be set in datasets that have the 
; input_source attribute defined.
;
; @param input_file {in}{required}{type=string}
;   The full path to and name of the input file being processed
;-
pro dsproc::set_input_source, input_file
  compile_opt idl2, logical_predicate

  tmp = dsproc_set_input_source(input_file)
end

;+
; Free a null terminated list of filee names.
;
; @param file_list {in}{required}{type=string}
;  A scalar or array of strings specifying the names of the files.
; 
;-
pro dsproc::free_file_list, file_list
  compile_opt idl2, logical_predicate

  tmp = dsproc_free_file_list(file_list)
end

;+
; Set the begin and end times for the current processing interval.
; 
; This function can be used to override the begin and end times
; of the current processing interval and should be called from the
; pre-retrieval hook function.
;
; @param begin_time {in}{required}{type=long64}
;   begin time in seconds since 1970.
; @param end_time {in}{required}{type=long64}
;   end time in seconds since 1970.
; 
;-
pro dsproc::set_processing_interval, begin_time, end_time
  compile_opt idl2, logical_predicate

  tmp = dsproc_set_processing_interval(begin_time, end_time)

end
;+
; Set the offset to apply to the processing interval.
; 
; This function can be used to shift the processing interval 
; and should be called from either the init-process or 
; pre-retrieval hook function.
;
; @param offset {in}{required}{type=long64}
;  offset in seconds
; 
;-
pro dsproc::set_processing_interval_offset, offset
  compile_opt idl2, logical_predicate

  tmp = dsproc_set_processing_interval_offset(offset)

end

;+
; Set the global transformation QC rollup flag.
; 
; This function should typically be called from the users init_process function, 
; but must be called before the post-transform hook completes.
;
; Setting this flag to true specifies that all bad and indeterminate bits
; in transformation QC variables should be consolidated into a single
; bad or indeterminate bit when they are mapped to the output datasets. 
; This bit consolidation will only be done if the input and output QC
; variables have the appropriate bit descriptions:
;    - The input transformation QC variables will be determined by 
;      checking the tag names in the bit description attributes. 
;      These must be in same order as the transformation would define them.
;     - The output QC variables must contain two bit descriptions 
;       for the bad and indeterminate bits to use, and these bit 
;       descriptions must begin with the following text:
;          "Transformation could not finish"'
;          "Transformation resulted in an indeterminate outcome"
; An alternative to calling this function is to set the "ROLLUP TRANS QC"
; flags for the output datastreams and/or retrieved variables.
; See dsproc.set_datastream_flags() and cdsvar.set_var_flags(). 
; These options should not typically be needed, however, because the
; internal mapping logic will determine when it is appropriate 
; to do the bit consolidation.
;
; @keyword flag {in}{type=boolean}{default=false}
;  flag - transformation QC rollup flat (1=TRUE, 0=FALSE)
; 
;-
pro dsproc::set_trans_qc_rollup_flag, flag=flag
  compile_opt idl2, logical_predicate

  tmp = dsproc_set_trans_qc_rollup_flag(keyword_set(flag))

end

;+
; Get the force mode
; 
; @returns 
;  1 if enabled <br>
;  0 if disabled
;-
; The force mode can be enabled using the -F option on the command line.
; This mode can be used to force the process past allrecoverable errors
; that would normally stop process execution.
; but must be called before the post-transform hook completes.
;
function dsproc::get_force_mode
  compile_opt idl2, logical_predicate
  if self.debug then print, 'Calling dsproc_get_force_mode'

  return, dsproc_get_force_mode()

end

;+
; Rename a data file.
; This function will rename a raw data file into the datastream
; directory using the datastream name and begin_time to give it a
; fully qualified ARM name.  
; 
; If the end_time is specified, this function will verify that it 
; is greater than the begin_time and is not in the future. If only 
; one record was found in the raw file, the end_time argument must be 
; set to NULL.
;
; If the output file exists and has the same MD5 as the input file, 
; the input file will be removed and a warning message will be generated.
; 
; If the output file exists and has a different MD5 than the input file, 
; the rename will fail.
; 
; @param ds_id {in}{required}{type=int}
;  datastream ID.
; @param file_path {in}{required}{type=string}
; path to the directory the file is in
; @param file_name {in}{required}{type=string}
; name of the file to rename
; @param begin_time {in}{type=int}{required}
;  The time of the first record in the file
; @param begin_time {in}{type=int}{optional}
;  The time of the last record in the file 
;
; @returns 
;  1 if successful <br>
;  0 if and error occurred
;-
function dsproc::rename, ds_id, file_path, file_name, begin_time, end_time
  compile_opt idl2, logical_predicate

  if n_elements(end_time) eq 0 then ent_time = !null

  return, dsproc_rename(ds_id, file_path, file_name, begin_time, endtime)
end

;+
; Rename a bad data file.
; This function works the same as dsproc_rename() except that the 
; extension "bad" will be used in the output file name.
; 
; @param ds_id {in}{required}{type=int}
;  datastream ID.
; @param file_path {in}{required}{type=string}
; path to the directory the file is in
; @param file_name {in}{required}{type=string}
; name of the file to rename
; @param file_time {in}{type=int}{required}
;  The time used to rename the file
;
; @returns 
;  1 if successful <br>
;  0 if and error occurred
;-
function dsproc::rename_bad, ds_id, file_path, file_name, file_time
  compile_opt idl2, logical_predicate

  return, dsproc_rename_bad(ds_id, file_path, file_name, file_time)
end

;+
; Set the portion of file names to preserve when they are renamed.
; This is the number of dots from the end of the file name and
; specifies the portion of the original file name to preserve when 
; it is renamed.
; 
; @param ds_id {in}{required}{type=int}
;  datastream ID.
; @param preserve_dots {in}{required}{type=int}
;  portion of original file name to preserve
;
;-
function dsproc::set_rename_preserve_dots, ds_id, preserve_dots
  compile_opt idl2, logical_predicate

  return, dsproc_set_rename_preserve_dots(ds_id, preserve_dots)
end

;+
; Fetch a dataset from previously stored data.
; This function will retrieve a dataset from the previously stored
; data for the specified datastream and time range.
;
; If the begin_timeval is not specified, data for the time just prior
; to the end_timeval will be retrieved.
;
; If the end_timeval is not specified, data for the time just after
; the begin_timeval will be retrieved.
;
; If both the begin and end times are not specified, the data
; previously retrieved by this function will be returned.
;
; 
; @param ds_id {in}{required}{type=int}
;  datastream ID.
; @keyword begin_timeval {in}{optional}{type=int}{default=0}
;   beginning of the time range to search
; @keyword end_timeval {in}{optional}{type=int}{default=0}
;   end of the time range to search
; @param merge_obs {in}{required}{type=int}
;   flag specifying if multiple observations should be merged when
;   possible (0 == false, 1 == true) 
; @param var_names {in}{optional}{type=string}{default=!null}
;  String array of variable names, or !null as described above.
; @returns
;  A CDSGroup object reference or !null.
;
;-
function dsproc::fetch_dataset, ds_id, begin_timeval=begin_timeval, $
     end_timeval=end_timeval, merge_obs, var_names
     
  compile_opt idl2, logical_predicate

  nvars = n_elements(var_names)
  if nvars eq 0 then var_names=!null

  t_begin = keyword_set(begin_timeval) ? begin_timeval : 0
  t_end = keyword_set(end_timeval) ? end_timeval : 0

  CDSGroup=!null

  self.status = dsproc_fetch_dataset(ds_id, t_begin, t_end, $
     merge_obs, nvars, var_names, pCDSGroup)

  if self.status ge 1 then begin
    CDSGroup=obj_new('CDSGroup', pCDSGroup)
  endif else CDSGroup=!null

  if self.contOnError eq 0 && self.status eq -1 then begin
    message, 'An error occurred while fetching previous dataset'
  endif

  return, CDSGroup

end

;+
; Set the value of a coordinate system transformation parameter.
; 
; Error messages from this function are sent to the message handler
; 
; @param coordsys_name {in}{required}{type=string}
;  The name of the coordinate system as specified in the retriever definition
; @param field_name {in}{required}{type=string}
;  The name of the field
; @param param_name {in}{required}{type=string}
;  The name of the parameter
; @keyword data_type {in}{optional}{default=!null}
;  IDL data type to use for the new variable, or !NULL to use the same data
;  type as the source variable. Valid datatypes are:
;<br> 1 = BYTE (8-bit)
;<br> 2 = INT (16-bit)
;<br> 3 = LONG (32-bit)
;<br> 4 = FLOAT (32-bit)
;<br> 5 = DOUBLE (64-bit)
;<br> 7 = STRING (64-bit)
;<br>  all other values will be treated as !NULL.
; @param data {in}{required}
;  pointer  to the parameter data
; @returns
;  1 if successful <br>
;  0 if a memory allocation error occurred
;-
function dsproc::set_coordsys_trans_param, coordsys_name, $
    field_name, param_name, data_type=data_type, data
  compile_opt idl2, logical_predicate

  ; KLG when we go to IDL 8.4 I want update this to 
  ; support users providing data_type as either the type
  ; (1, 2, 3, etc) or typename (STRING, FLOAT, DOUBLE)
  ; I need 8.4 so I can use ISA function 
  ; I will check to see if data_type is a string or char
  ; and if so I will assume they passed in typename and 
  ; have code to convert that to type ...
  ; if data_type eq "STRING" then data_type = 7
  ; I should update the cdsvar data propertely that calls
  ; set_var_data also have similar logic.

  if n_elements(data) gt 0 then begin

    ; If the data is type scalar convert to an 
    ; array because dsproc_set_coordsys_trans_param only
    ; accepts arrays.
    if isa(data, /scalar) then begin
        type = size(data, /TNAME)
        data = [data]
        ; If its a scalar and a string then length is 
        ; string length plus 1.
        if type eq 'STRING' then begin
            ; then length to pass in is length of string + 1
            length = strlen(data[0]) + 1
        endif else begin
        ; otherwise the length is the number of elements in the array
            length = n_elements(data)
        endelse
    endif else begin
    ; otherwise make sure its a 1dim array
        ndim = size(data, /N_DIMENSIONS)
        if ndim gt 1 then begin
            status = 'Error in set_coordsys_trans_param'
            debug_string  = string(format ='A,A', $
              'set_coordsys_trans_param does not support ', $
              'mult-dimensional arrays.') 
              ds.error, status_string, msg = debug_string
              return, 0
        endif 
        length = n_elements(data)
    endelse
   
    ; convert to target type
    ndata  = fix(data, type=data_type)

    res=dsproc_set_coordsys_trans_param(coordsys_name, $
      field_name, param_name, data_type, length, ndata)

  return, res
  endif
end

;+
; Get process location.
;<br>
; The memory used by the output location structure belongs to the 
; internal structures and must not be freed or modified by the 
; calling process.
; If an error occurs in this function it will be appended to the 
; log and error mail messages, and the process status will be 
; set appropriately.
;
; @keyword err {out}{required}{type=int}
;  1 if successful, 0 if the database returned a NULL
;  result, or -1 if an error occurred.
; @returns
;  The procloc structure.<br>
;-
function dsproc::get_location, err=err
  compile_opt idl2, logical_predicate

  err = -2
  err = dsproc_get_location(procloc)
  return, procloc
end

;+
; Set the default NetCDF file extension to 'nc' for output files.
;
; The NetCDF file extension used by ARM has historically been "cdf".
; This function can be used to change it to the new prefered of "nc",
; and must be called before calling dsproc_main().
;
;-
pro dsproc::use_nc_extension
  compile_opt idl2, logical_predicate
  if self.debug then print, 'Calling dsproc_use_nc_extension'
  tmp=dsproc_use_nc_extension()

end

;+
; Disable the creation of the process lock file.
;
; Warning: Disabling the lock file will allow multiple
; processes to run over the top of themselves 
; and can lead to unpredictable behavior.
;
;-
pro dsproc::disable_lock_file
  compile_opt idl2, logical_predicate
  if self.debug then print, 'Calling dsproc_disable_lock_file'
  tmp=dsproc_disable_lock_file()

end

;+
; Convert seconds since 1970 to a timestamp.
; This function will create a timestamp of the form:
; 'YYYYMMDD.hhmmss'
;<br>
; The timestamp argument must be large enough to hold at least 16 characters 
; (15 plus the null terminator).
;<br>
;If an error occurs in this function it will be appended to the 
; log and error mail 
; @param secs1970 {in}{required}{type=int}
;  seconds since 1970.
; @returns
;  timestamp if successful<br>
;  0 if an error occurred
function dsproc::create_timestamp, secs1970
  compile_opt idl2, logical_predicate

  res = dsproc_create_timestamp(secs1970)
  return, res

end

;+
; Set the offset to use when retrieving data.
; 
; This function can be used to override the begin and end time
; offsets specified in the retriever definition and should be
; called from the pre-retrieval hook function.
;
; @param ds_id {in}{required}{type=int}
;  datastream ID.
; @param begin_offset {in}{required}{type=long64}
;  offset in seconds
; @param end_offset {in}{required}{type=long64}
;  offset in seconds
; 
;-
pro dsproc::set_retriever_time_offsets, ds_id, begin_offset, $
   end_offset
  compile_opt idl2, logical_predicate

  tmp = dsproc_set_retriever_time_offsets(ds_id, begin_offset, $
    end_offset)

end
; AB Step 6
;  This step is to write a small IDL wrapper on top of the
;  C binding. This may not strictly be necessary in all cases
;  but in the interest of encapsulating the functionality into
;  IDL object classes I have chosen to take this extra step.
;  Otherwise, only the global functions/procedures would be available.
;
;  There are a couple of advantages to this approach. One is that more
;  error handling can be coded into the IDL code, and convenience steps
;  can be added here. But the most important one is that the header
;  can provide IDL doc style documentation. Unfortunately, there isn't
;  a way to directly create IDL doc style documentation from the C wrapper.
;
;+ Get the process status.
;
; @returns
;  Process status message.
;-
function dsproc::get_status
  compile_opt idl2, logical_predicate
; The compile_opt statement above causes IDL to do the following for this function
;  1. treat integer constants as 32-bit by default instead of the standard 16-bit
;  2. treats parenthesis a(b) as a function call as opposed to array index
;     and enforces array indexing to use brackets a[b]
;  3. makes the idl "if" statement evaluate more like C in that
;     non-zero is true and zero is false. (the old default for IDL was to only look
;     at the first bit, so for integers, even would become false and odd would
;     become true). I know, that is hard to believe for a C programmer.
;
; To get more help on IDL code, start IDL and type ?
; If you have an "ssh -X" connection that should work.
  return, dsproc_get_status()
end
;+
; Set hook function
;
; @param name {in}{type=string}{required}
;  The name of the hook to be set. Valid names are:
;  init_process, finish_process, pre_retrieval, post_retrieval, 
;  pre_transform, post_transform, process_file, process_data.
; @param value {in}{required}
;  Either a string defining a global scope function, or
;  an object reference whose corresponding hook method
;  will be called.
;-
pro dsproc::set_hook, name, value
  compile_opt idl2, logical_predicate
  h=self.hook
  h[strlowcase(name)]=value
end
;+
; Check init state, for internal use.
; @private
;-
pro dsproc::_check_init
  on_error, 2
  if self.hasInit eq 0 then begin
    message, 'Error, initialize must be called first'
  endif
end
;+
; Init hook
;-
pro dsproc::_run_init_process_hook
  compile_opt idl2, logical_predicate
  h=self.hook
  if h->HasKey('init_process') then begin
    cb=h['init_process']
    if isa(cb, 'string') then begin
      self.status=call_function(cb, self)
    endif else if obj_valid(cb) then begin
      self.status=cb->init_process_hook(self)
    endif
  endif else self.status=1
end
;+
; Finish hook
;-
pro dsproc::_run_finish_process_hook
  compile_opt idl2, logical_predicate
  h=self.hook
  if h->HasKey('finish_process') then begin
    cb=h['finish_process']
    if isa(cb, 'string') then begin
      self.status=call_function(cb, self)
    endif else if obj_valid(cb) then begin
      self.status=cb->finish_process_hook(self)
    endif
  endif else self.status=1
end
;+
; Pre-retrieval hook
;
; @param interval {in}{required}{type=long64}
;  An array on the form [ begin_time, end_time ]. Where
;  times are in seconds since 1-Jan-1970.
;-
pro dsproc::_run_pre_retrieval_hook, interval
  compile_opt idl2, logical_predicate
  h=self.hook
  if h->HasKey('pre_retrieval') then begin
    cb=h['pre_retrieval']
    if isa(cb, 'string') then begin
      self.status=call_function(cb, self, interval)
    endif else if obj_valid(cb) then begin
      self.status=cb->pre_retrieval_hook(self, interval)
    endif
  endif else self.status=1
end
;+
; Post-retrieval hook
;
; @param interval {in}{required}{type=long64}
;  An array on the form [ begin_time, end_time ]. Where
;  times are in seconds since 1-Jan-1970.
; @param ret_data {in}{required}{type=CDSGroup}
;  A reference to the retrieved data group.
;-
pro dsproc::_run_post_retrieval_hook, interval, ret_data
  compile_opt idl2, logical_predicate
  h=self.hook
  if h->HasKey('post_retrieval') then begin
    cb=h['post_retrieval']
    if isa(cb, 'string') then begin
      self.status=call_function(cb, self, interval, ret_data)
    endif else if obj_valid(cb) then begin
      self.status=cb->post_retrieval_hook(self, interval, ret_data)
    endif
  endif else self.status=1
end
;+
; Pre-transform hook
;
; @param interval {in}{required}{type=long64}
;  An array on the form [ begin_time, end_time ]. Where
;  times are in seconds since 1-Jan-1970.
; @param ret_data {in}{required}{type=CDSGroup}
;  A reference to the retrieved data group.
;-
pro dsproc::_run_pre_transform_hook, interval, ret_data
  compile_opt idl2, logical_predicate
  h=self.hook
  if h->HasKey('pre_transform') then begin
    cb=h['pre_transform']
    if isa(cb, 'string') then begin
      self.status=call_function(cb, self, interval, ret_data)
    endif else if obj_valid(cb) then begin
      self.status=cb->pre_transform_hook(self, interval, ret_data)
    endif
  endif else self.status=1
end
;+
; Post-transform hook
;
; @param interval {in}{required}{type=long64}
;  An array on the form [ begin_time, end_time ]. Where
;  times are in seconds since 1-Jan-1970.
; @param trans_data {in}{required}{type=CDSGroup}
;  A reference to the transformed data group.
;-
pro dsproc::_run_post_transform_hook, interval, trans_data
  compile_opt idl2, logical_predicate
  h=self.hook
  if h->HasKey('post_transform') then begin
    cb=h['post_transform']
    if isa(cb, 'string') then begin
      self.status=call_function(cb, self, interval, trans_data)
    endif else if obj_valid(cb) then begin
      self.status=cb->post_transform_hook(self, interval, trans_data)
    endif
  endif else self.status=1
end

;+
; Quicklook hook
;
; @param interval {in}{required}{type=long64}
;  An array on the form [ begin_time, end_time ]. Where
;  times are in seconds since 1-Jan-1970.
;-
pro dsproc::_run_quicklook_hook, interval
  compile_opt idl2, logical_predicate
  h=self.hook
  if h->HasKey('quicklook') then begin
    self->debug, 1, ['', '----- ENTERING QUICKLOOK HOOK ------']
    cb=h['quicklook']
    if isa(cb, 'string') then begin
      self.status=call_function(cb, self, interval)
    endif else if obj_valid(cb) then begin
      self.status=cb->quicklook_hook(self, interval)
    endif
    self->debug, 1, ['', '----- EXITING QUICKLOOK HOOK ------']
  endif else self.status=1
end

;+
; Process file hook
;
; @param input_dir {in}{required}{type=string}
;  The directory in which the file is located
; @param file_name {in}{required}{type=string}
;  The name of the file to process
;-
pro dsproc::_run_process_file_hook, input_dir, file_name
  compile_opt idl2, logical_predicate
  h=self.hook
  if h->HasKey('process_file') then begin
    cb=h['process_file']
    if isa(cb, 'string') then begin
      self.status=call_function(cb, self, input_dir, file_name)
    endif else if obj_valid(cb) then begin
      self.status=cb->process_file_hook(self, input_dir, file_name)
    endif
  endif else self.status=1
end


;+
; Process data hook
;
; @param interval {in}{required}{type=long64}
;  An array on the form [ begin_time, end_time ]. Where
;  times are in seconds since 1-Jan-1970.
; @param input_data {in}{required}{type=CDSGroup}
;  A reference to the input data group.
;-
pro dsproc::_run_process_data_hook, interval, input_data
  compile_opt idl2, logical_predicate
  h=self.hook
  if h->HasKey('process_data') then begin
    ; AB Step 7b modify to better match C code.
    self->debug, 1, ['', '----- ENTERING PROCESS DATA HOOK ------']
    cb=h['process_data']
    if isa(cb, 'string') then begin
      self.status=call_function(cb, self, interval, input_data)
    endif else if obj_valid(cb) then begin
      self.status=cb->process_data_hook(self, interval, input_data)
    endif
    ; AB Step 7
    ; Modify the code to match _dsproc_run_process_data_hook in dsproc_hooks.c
    ; This is more or less a direct translation.
    if self.status lt 0 then begin
      status_text = dsproc_get_status()
      if strlen(status_text) eq 0 then begin
        self->error, "Error message not set by process_data_hook function"
        dsproc_set_status, "Unknown Data Processing Error (check logs)"
      endif
    endif
    self->debug, 1, ['----- EXITING PROCESS DATA HOOK -------', '']
    ; KLG lines below are from Atle, I should decide whether to do this...
    ; Note that the next step is in cdsvar__define.pro to add ndims.
    ; Also note that similar modifications need to be added to the other
    ; hooks.

  endif else self.status=1
end
;+
; Public properties (get)
;-
pro dsproc::GetProperty, status=status, debug=debug, data_home=data_home, $
    datastream_data=datastream_data, conf_data=conf_data, $
    logs_data=logs_data, site=site, facility=facility, name=name, $
    userdata=userdata
  compile_opt idl2, logical_predicate
  if arg_present(status) then status=self.status
  if arg_present(debug) then debug=self.debug
  if arg_present(data_home) then data_home=getenv('DATA_HOME')
  if arg_present(datastream_data) then datastream_data=getenv('DATASTREAM_DATA')
  if arg_present(conf_data) then conf_data=getenv('CONF_DATA')
  if arg_present(logs_data) then logs_data=getenv('LOGS_DATA')
  if arg_present(site) then site=dsproc_get_site()
  if arg_present(facility) then facility=dsproc_get_facility()
  if arg_present(name) then name=dsproc_get_name()
  if arg_present(userdata) then userdata=*self.userdata
end
;+
; Public properties (set)
;-
pro dsproc::SetProperty, debug=debug, data_home=data_home, $
    datastream_data=datastream_data, conf_data=conf_data, $
    logs_data=logs_data, userdata=userdata
  compile_opt idl2, logical_predicate
  if n_elements(debug) then self.debug=debug
  if n_elements(data_home) then setenv, 'DATA_HOME='+data_home
  if n_elements(datastream_data) then setenv, 'DATASTREAM_DATA='+datastream_data
  if n_elements(conf_data) then setenv, 'CONF_DATA='+conf_data
  if n_elements(logs_data) then setenv, 'LOGS_DATA='+logs_data
  if userdata ne !null then *self.userdata=userdata
end
;+
; Life cycle method
;-
pro dsproc::Cleanup
  compile_opt idl2, logical_predicate
  if self.hasInit then begin
    ; need to finish gracefully
    print, 'Calling dsproc::finish.'
    self->finish
  endif
end
;+
; Class definition routine.
;-
pro dsproc__define
  compile_opt idl2, logical_predicate
  s={ dsproc $
    , inherits IDL_Object $
    , CDSData: obj_new() $
    , hook: obj_new() $
    , hasInit: 0 $
    , status: 0 $
    , contOnError: 0 $
    , debug: 0 $
    , userdata: ptr_new() $
    }
end
