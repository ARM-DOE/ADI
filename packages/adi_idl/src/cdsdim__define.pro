;+
; Class definition for CDSDim class
;-
pro CDSDim__define
  compile_opt idl2, logical_predicate
  s={ CDSDim $
    , inherits CDSObject $
    }
end
