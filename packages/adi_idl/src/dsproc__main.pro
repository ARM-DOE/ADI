; Private method used by dsproc::main.
;
; @param proc_model {in}{required}
;  Either an integer or one of the following strings:
;<br>    PM_GENERIC
;<br>    PM_RETRIEVER_VAP
;<br>    PM_TRANSFORM_VAP
;-
pro dsproc::_dsproc_vap_main_loop, proc_model
  compile_opt idl2, logical_predicate

  status=1
  DSP_RETRIEVER=dsproc_proc_model('DSP_RETRIEVER')
  DSP_RETRIEVER=dsproc_proc_model('DSP_RETRIEVER')

  QuicklookMode = dsproc_get_quicklook_mode()

  while status do begin
    interval=self->start_processing_loop()
    if self.status eq 0 then break

    ; Run the pre_retrieval_hook function
    self->_run_pre_retrieval_hook, interval
    if self.status eq -1 then break;
    if self.status eq 0 then continue;

    if QuicklookMode ne "QUICKLOOK_ONLY" then begin
        ; Retrieve the data for the current processing interval
        if proc_model and dsproc_proc_model('DSP_RETRIEVER') then begin
          ret_data=self->retrieve_data(interval)
          if self.debug then begin
            print, 'Retrieve status: ', self.status
            help, ret_data
          endif
          if self.status eq -1 then break
          if self.status eq 0 then continue
        endif

        ; Run the post_retrieval_hook function
        self->_run_post_retrieval_hook, interval, ret_data
        if self.status eq -1 then break
        if self.status eq 0 then continue

        ; Merge the observations in the retrieved data
        self->merge_retrieved_data
        if self.status eq 0 then break

        ; Run the pre_transform_hook function
        self->_run_pre_transform_hook, interval, ret_data
        if self.status eq -1 then break
        if self.status eq 0 then continue

        ; Perform the data transformations for transform VAPs
        if proc_model and dsproc_proc_model('DSP_TRANSFORM') then begin
          trans_data=self->transform_data()
          if self.status eq -1 then break
          if self.status eq 0 then continue
        endif

        ; Run the post_transform_hook function
        self->_run_post_transform_hook, interval, trans_data
        if self.status eq -1 then break
        if self.status eq 0 then continue

        ; Create output datasets
        self->create_output_datasets
        if self.status eq 0 then break

        ; Run the user's data processing function
        if obj_valid(trans_data) then begin
          self->_run_process_data_hook, interval, trans_data
        endif else begin
          self->_run_process_data_hook, interval, ret_data
        endelse
        if self.status eq -1 then break
        if self.status eq 0 then continue

        ; Store output datasets
        self->store_output_datasets
        if self.status eq 0 then break

    endif ;end Quicklookmode ne QUICKLOOK_ONLY

    if QuicklookMode ne "QUICKLOOK_DISABLE" then begin
        ; Run the quicklook_hook function
        self->_run_quicklook_hook, interval
        if self.status eq -1 then break
        if self.status eq 0 then continue
    endif

  endwhile
end

;+
; Private method used by dsproc::main.
;
; @param proc_model {in}{required}
;  Either an integer or one of the following strings:
;<br>    PM_INGEST
;-
pro dsproc::_dsproc_ingest_main_loop, proc_model
  compile_opt idl2, logical_predicate

  ; Get the list of input datastream IDs
  dsids = self->get_input_datastream_ids()
  if dsids eq !null then return

  ndsids = n_elements(dsids)
  if n_elements(dsids) eq 0 then return

  ; Make sure that only one level 0 input datastream class
  ; was specified in the database.
  dsid = -1
  for dsi = 0, ndsids-1 do begin
    dsinfo = self->datastream_info(dsids[dsi]) 
    if dsinfo.class_level[0] eq '00' then begin
      if dsid eq -1 then begin
        dsid = dsids[dsi]
      endif else begin
        if self.debug then begin
          print, 'Too many level 0 input datastreams defined in database '
          print,  '-> ingests only support one level 0 input datastream'
        endif 
      endelse
    endif
  endfor

  ; Get the list of input files
  files = self->get_datastream_files(dsid)
  ; The type code for a string is 7
  if size(files[0], /TYPE) ne 7 then begin
    if self.debug then begin
      print, 'No data files found to process in: ',dsinfo.path[0] 
    endif 
  endif 

  input_dir = dsinfo.path[0]  

  if size(files[0], /TYPE) eq 7 then begin
    ; Loop over all input files
    loop_start = 0
    loop_end = 0
    for fi = 0, n_elements(files)-1 do begin
      time_remaining = self->get_time_remaining()
      if time_remaining ge 0 then begin
        if time_remaining eq 0 then break
        if loop_end - loop_start gt time_remaining then begin
          if self.debug then begin
            max_time = self->get_max_run_time()
            print, 'Stopping ingest before max run time of '
            print, max_time, ' seconds exceeded'
          endif 
          break
        endif
      endif 

      ; Process the file
      val = fi + 1
      string = 'PROCESSING_FILE ' + string(val) + ': ' + files[fi]
      self->debug, 1, ['', string, '']
      if self.debug then begin
        print, 'Processing: ' + input_dir + '/' + files[fi]
      endif
      loop_start = systime(1)
      self->set_input_dir, input_dir
      self->set_input_source, files[fi]
      ; Run the process_file_hook function
      self->_run_process_file_hook, input_dir, files[fi]
      if self.status eq -1 then break
      if self.status eq 0 then continue
      loop_end = systime(1)

    endfor ;end loop over files, fi
  
    self->free_file_list, files

  endif  ;end if fiels[0] ne 0

  

end


;+
; IDL entry point that closely follows the C implementation of dsproc_main.
;
; @param proc_model {in}{required}
;  An enumerated value: 
;   PM_GENERIC
;   PM_RETRIEVER_VAP
;   PM_TRANSFORM_VAP
;   PM_INGEST
;  Or the corresponding integer, see dsproc_proc_model
; @keyword _ref_extra
;  All keywords are passed along to ::Initialize, 
;  (see ::Initialize for actual keywords)
;-
pro dsproc::main, proc_model, _ref_extra=extra
  compile_opt idl2, logical_predicate

  if isa(proc_model, 'string') then begin
    pm=dsproc_proc_model(strupcase(proc_model))
  endif else pm=long(proc_model)

  self->initialize, pm, _strict_extra=extra

  ; run init process hook here
  self->_run_init_process_hook
  if self.status eq 0 then begin
    self->finish
    print, 'suggested exit status: ', self.status
    return
  endif

  self->db_disconnect

  if pm eq dsproc_proc_model('PM_INGEST') then begin
    self->_dsproc_ingest_main_loop
  endif else begin
    self->_dsproc_vap_main_loop, pm
  endelse

  ; run finish process hook here
  self->_run_finish_process_hook

  self->finish
  print, 'suggested exit status: ', self.status
end
