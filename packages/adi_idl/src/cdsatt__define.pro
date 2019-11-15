;+
; @file_comments
;  Implements an IDL wrapper to the CDSAtt C struct.
; @property value
;  The attribute value
;-

;+
; Overload "help, var" output.
;
; @param varname {in}{required}{type=string}
;  A string containing the name of the variable in the caller's scope
; @returns
;  A one line string description of the object.
;-
function CDSAtt::_overloadHelp, varname
  compile_opt idl2, logical_predicate

  self->GetProperty, value=val
  if val eq !null then begin
    short = '!NULL'
  endif else if isa(val, 'string') then begin
    if strlen(val) gt 30 then begin
      short = strmid(val, 0, 30) + ' ...'
    endif else short = val
  endif else if n_elements(val) gt 5 then begin
    short = strjoin(strtrim(val[0:4],2),' ') + ' ...'
  endif else begin
    short = strjoin(strtrim(val,2),' ')
  endelse

  str = varname + '    <' + typename(self) + '> ' + cds_obj_name(*self.p) + $
    ' ('+strtrim(cds_var_type(*self.p, /name), 2) + ') : ' + short
  return, str
end
;+
; Obtain public properties
;-
pro CDSAtt::GetProperty, value=value, _ref_extra=extra
  compile_opt idl2, logical_predicate

  if arg_present(value) then value = dsproc_get_att_value( $
    cds_obj_parent(*self.p), $
    cds_obj_name(*self.p) )
  ; pass additional properties to the superclass
  if n_elements(extra) then self->CDSObject::GetProperty, _STRICT_EXTRA=extra
end
;+
; Class definition for CDSAtt type
;-
pro CDSAtt__define
  compile_opt idl2, logical_predicate
  s={ CDSAtt $
    , inherits CDSObject $
    }
end
