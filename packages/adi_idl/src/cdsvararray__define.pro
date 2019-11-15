;+
; Class definition for CDSVarArray
;-
pro CDSVarArray__define
  compile_opt idl2, logical_predicate
  s={ CDSVarArray $
    , inherits CDSObject $
    }
end
