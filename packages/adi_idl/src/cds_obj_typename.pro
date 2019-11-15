;+
; Equivalent to #define header definition for CDSObjectType
; @param enum {in}{optional}{type=int}
;  The integer to be converted to a string. Required if PTR is not specified.
; @keyword ptr {in}{optional}{type=int}
;  An integer pointer to the C struct.
; @returns
;  A valid class name that can consequently be used
;  to create an IDL object.
;-
function cds_obj_typename, enum, ptr=ptr
  compile_opt idl2, logical_predicate

  if n_params() eq 0 then enum = cds_obj_type(ptr)

  case enum of
  1: return, 'CDSGroup'
  2: return, 'CDSDim'
  3: return, 'CDSAtt'
  4: return, 'CDSVar'
  5: return, 'CDSVarGroup'
  6: return, 'CDSVarArray'
  else:
  endcase
  return, 'CDSObject'
end
