
cdsatt__define
cdsdim__define
cdsgroup__define
cdsobject__define
cds_obj_typename
cdsvararray__define
cdsvar__define
cdsvargroup__define
dsproc__define
dsproc__main


; New classes 
classes = ['cdsatt__define.pro', 'cdsdim__define.pro', 'cdsgroup__define.pro', 'cdsobject__define.pro', 'cds_obj_typename.pro', 'cdsvararray__define.pro', 'cdsvar__define.pro', 'cdsvargroup__define.pro', 'dsproc__define.pro', 'dsproc__main.pro']
FOREACH class, classes DO BEGIN
        RESOLVE_ROUTINE, class[0], /COMPILE_FULL_FILE
ENDFOREACH

PRO idl_dsproc
END; Procedure End
