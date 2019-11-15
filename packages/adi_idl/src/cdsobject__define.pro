;+
; @file_comments
;  Implements the CDSObject base class.
;
; @property ptr {type=int}
;  A C pointer cast as an integer.
; @property parent {type=objref}
;  An object reference to the parent object.
; @property obj_path {type=string}
;  A string object path.
; @property def_lock {type=int}
;  Integer definition lock.
; @property name {type=string}
;  String object name.
;-

;+
; Initialization
;
; @param ptr {in}{required}
;  An opaque integer pointer.
; @returns
;  1 for success, and 0 for failure
;-
function CDSObject::Init, ptr
  compile_opt idl2, logical_predicate

  self.p = ptr_new(ptr)
  return, 1
end
;+
; Obtain properties
;-
pro CDSObject::GetProperty, parent=parent, ptr=ptr, name=name, $
    def_lock=def_lock, obj_path=obj_path
  compile_opt idl2, logical_predicate

  if arg_present(ptr) then ptr=*self.p
  if arg_present(parent) then begin
    pparent=cds_obj_parent(*self.p)
    if pparent then begin
      ; Cast to the correct type
      obj_type=cds_obj_type(pparent)
      ; convert to string classname
      obj_type=cds_obj_typename(obj_type)
      parent=obj_new(obj_type, pparent)
    endif
  endif
  if arg_present(name) then name = cds_obj_name(*self.p)
end
;+
; Overload "help, var" output.
;
; @param varname {in}{required}{type=string}
;  A string containing the name of the variable in the caller's scope
; @returns
;  A one line string description of the object.
;-
function CDSObject::_overloadHelp, varname
  compile_opt idl2, logical_predicate

  str = varname + '    <' + typename(self) + '> ' + cds_obj_name(*self.p) 
  return, str
end
;+
; Internal method for convenience, to convert from integers to objects
;-
function CDSObject::_toObject, ptr
  compile_opt idl2, logical_predicate
  if isa(ptr, /scalar) then $
    return, ptr ? obj_new(cds_obj_typename(ptr=ptr), ptr) : obj_new()
  np = n_elements(ptr)
  res = objarr(np)
  for i=0, np-1 do if ptr[i] then $
    res[i] = obj_new(cds_obj_typename(ptr=ptr[i]), ptr[i])
  return, res
end

;+
; Class definition of CDSObject base class
;-
pro CDSObject__define
  compile_opt idl2, logical_predicate
  s={ CDSObject $
    , inherits IDL_Object $
    , p: ptr_new() $
    }
end
