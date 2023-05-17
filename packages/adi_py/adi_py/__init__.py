"""-----------------------------------------------------------------------
This module provides the new ADI Python bindings which incorporate full
XArray compatibility.
-----------------------------------------------------------------------"""

from .constants import (
    ADIAtts,
    ADIDatasetType,
    BitAssessment,
    SplitMode,
    SpecialXrAttributes,
    TransformAttributes
)

from .process import Process

from .utils import DatastreamIdentifier

from .xarray_accessors import ADIDatasetAccessor, ADIDataArrayAccessor

from .exception import SkipProcessingIntervalException

from .logger import ADILogger


