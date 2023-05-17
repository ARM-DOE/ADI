"""---------------------------------------------------------------------------
This module provides a variety of symbolic constants that can be used to
find/select allowable values without hardcoding strings.
---------------------------------------------------------------------------"""
from enum import Enum
import dsproc3 as dsproc


class SplitMode(Enum):
    """-----------------------------------------------------------------------
    Enumerates the split mode which is used to define the output file size
    used when storing values.  See dsproc.set_datastream_split_mode().
    -----------------------------------------------------------------------"""
    SPLIT_ON_STORE = dsproc.SPLIT_ON_STORE
    SPLIT_ON_HOURS = dsproc.SPLIT_ON_HOURS
    SPLIT_ON_DAYS = dsproc.SPLIT_ON_DAYS
    SPLIT_ON_MONTHS = dsproc.SPLIT_ON_MONTHS


class SpecialXrAttributes:
    """-----------------------------------------------------------------------
    Enumerates the special XArray variable attributes that are assigned
    temporarily to help sync data between xarray and adi.
    -----------------------------------------------------------------------"""
    SOURCE_DS_NAME = '__source_ds_name'
    SOURCE_VAR_NAME = '__source_var_name'
    COORDINATE_SYSTEM = '__coordsys_name'
    OUTPUT_TARGETS = '__output_targets'
    DATASTREAM_DSID = '__datastream_dsid'
    DATASET_TYPE = '__dataset_type'
    OBS_INDEX = '__obs_index'


class ADIAtts:
    MISSING_VALUE = 'missing_value'
    LONG_NAME = 'long_name'
    STANDARD_NAME = 'standard_name'
    UNITS = 'units'
    VALID_MIN = 'valid_min'
    VALID_MAX = 'valid_max'
    FILL_VALUE = '_FillValue',
    DESCRIPTION = 'description'
    ANCILLARY_VARIABLES = 'ancillary_variables'


class BitAssessment(Enum):
    """-----------------------------------------------------------------------
    Used to easily reference bit assessment values used in ADI QC
    -----------------------------------------------------------------------"""
    BAD = 'Bad'
    INDETERMINATE = 'Indeterminate'


class TransformAttributes:
    """-----------------------------------------------------------------------
    Used to easily reference transformation metadata attrs used in ADI QC
    -----------------------------------------------------------------------"""
    CELL_TRANSFORM = 'cell_transform'


class ADIDatasetType(Enum):
    """-----------------------------------------------------------------------
    Used to easily reference different types of ADI datasets.
    -----------------------------------------------------------------------"""
    RETRIEVED = 1
    TRANSFORMED = 2
    OUTPUT = 3
