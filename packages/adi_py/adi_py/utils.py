"""---------------------------------------------------------------------------
This module provides a variety of functions that are used by the Process
class to serialize/deserialize between ADI and XArray.  Most of these utility
functions are used within Process class methods and are generally not intended
to be called directly by developers when implementing a specific Process subclass.
---------------------------------------------------------------------------"""
import itertools
import os
from typing import Any, Callable, Dict, List, Optional, Union, NamedTuple

import numpy as np
import pandas as pd
import xarray as xr

import cds3
import dsproc3 as dsproc
from .constants import SpecialXrAttributes, ADIAtts, ADIDatasetType
from .exception import SkipProcessingIntervalException, DatasetConversionException
from .logger import ADILogger, LogLevel


class DatastreamIdentifier(NamedTuple):
    """-----------------------------------------------------------------------
    NamedTuple class that holds various information used to identify a specific
    ADI dataset.
    -----------------------------------------------------------------------"""
    datastream_name: str
    site: str
    facility: str
    dsid: int


def is_empty_function(func: Callable) -> bool:
    """-----------------------------------------------------------------------
    Evaluates a given function to see if the code contains anything more than
    doctrings and 'pass'.  If not, it is considered an 'empty' function.

    Args:
        func (Callable):

    Returns:
        bool: True if the function is empty, otherwise False.

    -----------------------------------------------------------------------"""

    def empty_func():
        pass

    def empty_func_with_doc():
        """Empty function with docstring."""
        pass

    return func.__code__.co_code == empty_func.__code__.co_code or \
           func.__code__.co_code == empty_func_with_doc.__code__.co_code


def adi_hook_exception_handler(hook_func: Callable,
                               pre_hook_func: Callable = None,
                               post_hook_func: Callable = None) -> Callable:
    """-----------------------------------------------------------------------
    Python function decorator used to consistently handle exceptions in hooks
    so that they return the proper integer value to ADI core.  Also used to
    ensure that consistent logging and debug dumps happen for hook methods.

    Args:
        hook_func (Callable): The original hook function implemented by
            the developer.

        pre_hook_func (Callable): An optional function to be invoked right
            before the hook function (i.e., to do debug dumps)

        post_hook_func (Callable): An optional function to be invoked right
            after the hook function (i.e., to do debug dumps

    Returns:
        Callable: Decorator function that wraps the original hook function to
        provide built-in, ADI-compliant logging and exception handling.
    -----------------------------------------------------------------------"""
    hook_name = hook_func.__name__

    def wrapper_function(*args, **kwargs):
        ret_val = 1

        # Only run the hook if it is not empty!!
        if not is_empty_function(hook_func):
            try:
                if pre_hook_func is not None:
                    pre_hook_func()

                ADILogger.debug(f"**************** Starting {hook_name} ****************\n")
                hook_func(*args, **kwargs)

                if post_hook_func is not None:
                    post_hook_func()

            except SkipProcessingIntervalException as spe:
                if spe.log_level == LogLevel.ERROR:
                    ADILogger.error(spe.msg)
                elif spe.log_level == LogLevel.WARNING:
                    ADILogger.warning(spe.msg)
                else:
                    ADILogger.info(spe.msg)

                ret_val = 0

            except Exception as e:
                # Any other exception we treat as a fatal error
                # TODO: should we catch other exceptions and then set specific statuses??
                # e.g., dsproc.set_status("Required Variable(s) Not Found In Retrieved Data")
                # e.g., dsproc.set_status("Required Variable(s) Not Found In Transformed Data")
                ret_val = -1

                # Make sure to log the stack trace to the log
                ADILogger.exception(f"Hook {hook_name} failed.")

                # If we are in debug mode, then raise the exception back so that it can be better
                # utilized by debuggers.
                mode = os.environ.get('ADI_PY_MODE', 'production').lower()
                if mode == 'development':
                    raise e

            finally:
                ADILogger.debug(f"**************** Finished {hook_name} ****************\n")

        return ret_val

    return wrapper_function


def get_datastream_id(datastream_name: str, site: str = None, facility: str = None,
                   dataset_type: ADIDatasetType = None) -> Optional[int]:
    """-----------------------------------------------------------------------
    Gets the corresponding dataset id for the given datastream (input or output)

    Args:
        datastream_name (str):  The name of the datastream to find

        site (str):
            Optional parameter used only to find some input datasets (RETRIEVED or TRANSFORMED).
            Site is only required if the retrieval rules in the PCM specify two different
            rules for the same datastream that differ by site.

        facility (str):
            Optional parameter used only to find some input datasets (RETRIEVED or TRANSFORMED).
            Facility is only required if the retrieval rules in the PCM specify two different
            rules for the same datastream that differ by facility.

        dataset_type (ADIDatasetType):
            The type of the dataset to convert (RETRIEVED, TRANSFORMED, OUTPUT)

    Returns:
        Optional[int]: The dataset id or None if not found
    -----------------------------------------------------------------------"""

    def find_datastream_dsid(dsids):

        for id in dsids:
            level = dsproc.datastream_class_level(id)
            cls = dsproc.datastream_class_name(id)

            if datastream_name == f"{cls}.{level}":
                match = True

                if site:
                    datastream_site = dsproc.datastream_site(id)
                    if datastream_site != site.lower():
                        match = False

                if facility:
                    datastream_fac = dsproc.datastream_facility(id)
                    if datastream_fac != facility.upper():
                        match = False

                if match:
                    return int(id)

        return None

    if dataset_type == ADIDatasetType.RETRIEVED or dataset_type == ADIDatasetType.TRANSFORMED:
        dsid = find_datastream_dsid(dsproc.get_input_datastream_ids())

    elif dataset_type == ADIDatasetType.OUTPUT:
        dsid = find_datastream_dsid(dsproc.get_output_datastream_ids())

    else:
        dsid = find_datastream_dsid(dsproc.get_input_datastream_ids())

        if dsid is None:
            dsid = find_datastream_dsid(dsproc.get_output_datastream_ids())

    return dsid


def add_vartag_attributes(xr_var_attrs: Dict, adi_var: cds3.Var):
    """-----------------------------------------------------------------------
    For the given ADI Variable, extract the source_ds_name and source_var_name
    from the ADI var tags and add them to the attributes Dict for to be used
    for the Xarray variable.

    Note:
        Currently we are not including the coordinate system and output
        targets as part of the XArray variable's attributes since these
        are unlikely to be changed.  If a user creates a new variable, then
        they should call the corresponding Process methods
        assign_coordinate_system_to_variable or assign_output_datastream_to_variable
        to add the new variable to the designated coordinate system or
        output datastream, respectively.

    Args:
        xr_var_attrs (Dict): A Dictionary of attributes to be assigned to the
            XArray variable.

        adi_var (cds3.Var): The original ADI variable object.

    -----------------------------------------------------------------------"""
    source_ds_name = dsproc.get_source_ds_name(adi_var)
    if source_ds_name:
        xr_var_attrs[SpecialXrAttributes.SOURCE_DS_NAME] = source_ds_name

    source_var_name = dsproc.get_source_var_name(adi_var)
    if source_var_name:
        xr_var_attrs[SpecialXrAttributes.SOURCE_VAR_NAME] = source_var_name

    # TODO: I decided it's confusing to have the output dataset and coord sys
    # copied automatically, so user has to explicitly set this if they create
    # a new variable.
    # Add the output targets in case the user creates a new variable and it
    # does not have output datastream defined
    # output_targets = dsproc.get_var_output_targets(adi_var)
    # if output_targets:
    #     xr_output_targets = {}
    #     for output_target in output_targets:
    #         dsid = output_target.ds_id
    #         ds_var_name = output_target.var_name
    #         xr_output_targets[dsid] = ds_var_name
    #
    #     xr_var_attrs[SpecialXrAttributes.OUTPUT_TARGETS] = xr_output_targets


def get_empty_ndarray_for_var(adi_var: cds3.Var, attrs: Dict = None) -> np.ndarray:
    """-----------------------------------------------------------------------
    For the given ADI variable object, initialize an empty numpy ndarray data
    array with the correct shape and data type.  All values will be filled
    with the appropriate fill value.  The rules for selecting a fill value
    are as follows:

        - If this is a qc variable, the missing value bit flag will be used.  If no missing value bit, then the failed
          transformation bit flag will be used.  If no transformation failed bit, then use _FillValue.  If no _FillValue,
          then use the netcdf default fill value for integer data type.
        - Else if a missing_value attribute is available, missing_value will be used
        - Else if a _FillValue attribute is available, _FillValue will be used
        - Else use the netcdf default fill value for the variable's data type

    Args:
        adi_var (cds3.Var): The ADI variable object
        attrs (Dict):  A Dictionary of attributes that will be assigned to the
            variable when it is converted to XArray.  If not provided, it
            will be created from the ADI variable's attrs.

    Returns:
        np.ndarray: An empty ndarray of the same shape as the variable.
    -----------------------------------------------------------------------"""

    if attrs is None:
        adi_atts: List[cds3.Att] = adi_var.get_atts()
        attrs = {att.get_name(): dsproc.get_att_value(adi_var, att.get_name(), att.get_type()) for att in adi_atts}

    # Create a data array for this variable with empty values.
    fill_value = None

    # Look up the _FillValue attribute
    _fill_value = attrs.get(ADIAtts.FILL_VALUE)

    # Figure out the fill value to use based upon the var's metadata
    missing_values = dsproc.get_var_missing_values(adi_var)

    if adi_var.get_name().startswith('qc_'):
        # If this is a qc_ var, then first call dsproc_get_missing_value_bit_flag to see if there is a missing value
        # bit set.
        # dsproc.get_missing_value_bit_flag() will find a match if the bit description matches the value used in the
        # current DOD ingest template as well as common permutations.
        bit_descriptions = dsproc.get_qc_bit_descriptions(adi_var)
        missing_flag = dsproc.get_missing_value_bit_flag(bit_descriptions)
        if missing_flag > 0:
            fill_value = missing_flag

        else:
            # If not, then we check to see if the bit 1 description contains missing value descriptions found in other
            # DOD templates
            if len(bit_descriptions) > 0 and 'Transformation could not finish' in bit_descriptions[0]:
                # "Transformation could not finish" is found in VAP DOD template
                fill_value = 1

            elif len(bit_descriptions) > 0 and 'Data value not available' in bit_descriptions[0]:
                # "Datat value not available" was used in old Ingest DOD template
                fill_value = 1

            elif _fill_value:
                # If there is no missing value bit or transformation failed bit, then we will use the fill value attr
                # if it exists.
                fill_value = _fill_value

            else:
                # Get the default NetCDF4 fill value for this data type
                fill_value = adi_var.get_default_fill_value()

    elif missing_values and len(missing_values) > 0:
        fill_value = missing_values[0]

    elif _fill_value:
        fill_value = _fill_value

    else:
        # Get the default NetCDF4 fill value for this data type
        fill_value = adi_var.get_default_fill_value()

    # Get the np dtype for the variable
    dtype = dsproc.cds_type_to_dtype_obj(adi_var.get_type())

    # Get the shape of the data
    shape = []
    for dim in adi_var.get_dims():
        shape.append(dim.get_length())

    # Create a np.ndarray from the shape using np.full()
    # https://numpy.org/doc/stable/reference/generated/numpy.full.html
    data = np.full(shape, fill_value, dtype=dtype)
    return data


def get_time_data_as_datetime64(time_var: cds3.Var) -> np.ndarray:
    """-----------------------------------------------------------------------
    Get the time values from dsproc as seconds since 1970, then convert those
    values to datetime64 with microsecond precision.

    Args:
        time_var (cds3.Var): An ADI time variable object

    Returns:
        np.ndarray: An ndarray of the same shape as the variable with time
        values converted to the np.datetime64 data type with microsecond
        precision.
    -----------------------------------------------------------------------"""
    # If this dataset has no time data, then throw a readable exception and let developer
    # deal with it (since each process may want to handle it differently).
    if time_var.sample_count == 0:
        raise DatasetConversionException(
            'XArray dataset could not be created because the ADI dataset has no time data!')

    microsecond_times = np.asarray(dsproc.get_sample_timevals(time_var, 0)) * 1000000
    datetime64_times =  np.array(pd.to_datetime(microsecond_times, unit='us'), np.datetime64)
    return datetime64_times


def get_adi_var_as_dict(adi_var: cds3.Var) -> Dict:
    """-----------------------------------------------------------------------
    Convert the given adi variable to a dictionary that can be used to create
    an xarray dataarray.

    Args:
        adi_var (cds3.Var): An ADI variable object

    Returns:
        Dict: A Dictionary representation of the variable that can be used
        in the XArray.DataArray constructor to initialize a corresponding
        XArray variable.
    -----------------------------------------------------------------------"""
    # Get the variable's dimensions
    dims = [dim.get_name() for dim in adi_var.get_dims()]

    # Get the variable's attributes
    adi_atts: List[cds3.Att] = adi_var.get_atts()
    attrs = {att.get_name(): dsproc.get_att_value(adi_var, att.get_name(), att.get_type()) for att in adi_atts}

    # Now add special attributes for the variable tags
    add_vartag_attributes(attrs, adi_var)

    # If the variable is 'time' then we will convert values to datetime64 data types with microsecond precision
    if adi_var.get_name() == 'time':
        data = get_time_data_as_datetime64(adi_var)

    else:
        # This method uses np.PyArray_SimpleNewFromData to convert adi data array to np.ndarray
        # =====> It will return None if the variable has no data
        data = adi_var.get_datap()

        if data is None:
            # We need to initialize the DataArray with empty values.
            data = get_empty_ndarray_for_var(adi_var, attrs)

    return {
        'dims': dims,
        'attrs': attrs,
        'data': data
    }


def get_dataset_dims(adi_dataset: cds3.Group) -> List[cds3.Dim]:
    # Loop through parent groups to pull dims, and vars that may be associated with parent coord system group
    adi_dims = []
    group: cds3.Group = adi_dataset
    while group:
        group_dims = group.get_dims()
        for dim in group_dims:
            adi_dims.append(dim)
        group = group.get_parent()

    return adi_dims


def get_dataset_vars(adi_dataset: cds3.Group) -> List[cds3.Var]:
    # Loop through parent groups to pull dims, and vars that may be associated with parent coord system group
    adi_vars = []
    group: cds3.Group = adi_dataset
    while group:
        group_vars = group.get_vars()
        for var in group_vars:
            adi_vars.append(var)

        group = group.get_parent()
    return adi_vars


def to_xarray(adi_dataset: cds3.Group) -> xr.Dataset:
    """-----------------------------------------------------------------------
    Convert the specified CDS.Group into an XArray dataset.
    Attributes will be copied, but the DataArrays for each variable
    will be backed by an np.ndarray that links directly to the C
    ADI data via np.PyArray_SimpleNewFromData

    Args:
        adi_dataset (cds3.Group): An ADI dataset object.

    Returns:
        xr.Dataset: The corresponding XArray dataset object.
    -----------------------------------------------------------------------"""
    # Get Global attrs
    adi_atts: List[cds3.Att] = adi_dataset.get_atts()
    attrs = {att.get_name(): dsproc.get_att_value(adi_dataset, att.get_name(), att.get_type()) for att in adi_atts}

    # Loop through parent groups to pull dims, coordinate variables, and bounds vars that may be
    # associated with parent coord system group
    adi_dims = get_dataset_dims(adi_dataset)
    adi_vars = get_dataset_vars(adi_dataset)

    # Convert adi dims to dictionary
    dims = {dim.get_name(): dim.get_length() for dim in adi_dims}

    # Find the coordinate variable names
    coord_var_names = []
    for dim in adi_dims:
        adi_var = dim.get_var()
        if adi_var is not None:
            coord_var_names.append(adi_var.get_name())

    # Get coordinate & data variables
    coords = {}
    data_vars = {}
    for adi_var in adi_vars:
        var_name = adi_var.get_name()
        var_as_dict = get_adi_var_as_dict(adi_var)
        if var_name in coord_var_names:
            coords[var_name] = var_as_dict
        else:
            data_vars[var_name] = var_as_dict

    # Create a dictionary from the values
    data_dict = {
        'attrs': attrs,
        'dims': dims,
        'coords': coords,
        'data_vars': data_vars
    }

    dataset = xr.Dataset.from_dict(data_dict)
    return dataset


def get_cds_type(value: Any) -> int:
    """-----------------------------------------------------------------------
    For a given Python data value, convert the data type into the corresponding
    ADI CDS data type.

    Args:
        value (Any): Can be a single value, a List of values, or a numpy.ndarray
            of values.

    Returns:
        int: The corresponding CDS data type
    -----------------------------------------------------------------------"""
    val = value

    # Convert value to a numpy array so we can use dsproc method which
    # only works if value is a numpy ndarray
    if type(value) == list:
        val = np.array(value)

    elif type(value) != np.ndarray:
        # We need to wrap value in a list so np constructor doesn't get confused
        # if value is numeric
        val = np.array([value])

    if val.dtype.type == np.str_:
        # This comparison is always failing from within the cython, because
        # in the cython, dtype.type = 85 instead of np.str_.
        # So I'm adding it here instead.  This checks for any string type.
        cds_type = cds3.CHAR

    else:
        cds_type = dsproc.dtype_to_cds_type(val.dtype)

    return cds_type


def _sync_attrs(xr_atts_dict: Dict, adi_obj: cds3.Object):
    """-----------------------------------------------------------------------
    Sync Xarray attributes back to an ADI object (dataset or variable) by
    checking if the following changes were made:

        - Attribute values changed
        - Attributes were added
        - Attributes were deleted
        - An attribute type changed

    Args:
        xr_atts (Dict):
            Dictionary of Xarray attributes, where the keys are
            attribute names, and values are attribute values

        adi_obj (cds3.Object):
            ADI dataset or variable
    -----------------------------------------------------------------------"""
    # Get lists of attribute names for comparison between two lists
    adi_atts = {att.get_name() for att in adi_obj.get_atts()}
    xr_atts = []

    for att_name in xr_atts_dict:
        if att_name.startswith('__'):
            # special attributes start with '__' and are handled separately
            continue
        xr_atts.append(att_name)

    # First remove deleted atts
    deleted_atts = [att_name for att_name in adi_atts if att_name not in xr_atts]
    for att_name in deleted_atts:
        adi_att = dsproc.get_att(adi_obj, att_name)
        status = cds3.Att.delete(adi_att)
        if status < 1:
            raise Exception(f'Could not delete attribute {att_name}')

    # Then add new atts
    added_atts = [att_name for att_name in xr_atts if att_name not in adi_atts]
    for att_name in added_atts:
        att_value = xr_atts_dict.get(att_name)
        cds_type = get_cds_type(att_value)
        status = dsproc.set_att(adi_obj, 1, att_name, cds_type, att_value)
        if status < 1:
            raise Exception(f'Could not create attribute {att_name}')

    # Next change the value for other atts if the value changed
    other_atts = [att_name for att_name in xr_atts if att_name not in added_atts]
    for att_name in other_atts:
        att_value = xr_atts_dict.get(att_name)

        # For now, if the att is already defined in adi, we assume that the user will not
        # change the type, just the value.
        cds_type = dsproc.get_att(adi_obj, att_name).get_type()
        existing_value = dsproc.get_att_value(adi_obj, att_name, cds_type)

        if not np.array_equal(att_value, existing_value):
            status = dsproc.set_att(adi_obj, 1, att_name, cds_type, att_value)
            if status < 1:
                raise Exception(f'Could not update attribute {att_name}')


def _sync_dims(xr_dims: Dict, adi_dataset: cds3.Group):
    """-----------------------------------------------------------------------
    Sync Xarray dimensions back to ADI dataset by checking if:
        - Any dimensions were deleted and attempts to delete them from the dataset
        - Any dimensions were added and attempts to add them to the dataset
        - Any dimension length (e.g. size) and attempts to change the length

    Args:
        xr_dims (Dict):   
            Dictionary of Xarray dimiension, where the keys are
            dimension names, and values are dimension size

        adi_dataset (cds3.Group):
            ADI dataset
    -----------------------------------------------------------------------"""

    # key is dimension name, value is cds3.Dim object
    adi_dim_objs: List[cds3.Dim] = get_dataset_dims(adi_dataset)
    adi_dims = {dim.get_name(): dim for dim in adi_dim_objs}

    # Check if dimension needs to be deleted from ADI dataset
    deleted_dims = [dim_name for dim_name in adi_dims if dim_name not in xr_dims]

    # Delete appropriate dimensions
    for dim_name in deleted_dims:

        adi_dim = adi_dims[dim_name]

        # This function will also delete all variables that use the specified
        # dimension.
        status = cds3.Dim.delete(adi_dim)

        if status == 0:
            raise Exception(f'Could not delete dimension {dim_name}')

    # Check if dimension needs to be added to ADI Dataset
    added_dims = [dim_name for dim_name in xr_dims if dim_name not in adi_dims]

    # Add appropriate dimensions (assume dimension is not unlimited)
    is_unlimited = 0
    for dim_name in added_dims:

        dim_size = xr_dims[dim_name]
        dim_obj = adi_dataset.define_dim(dim_name, dim_size, is_unlimited)
        
        if dim_obj is None:
            raise Exception(f'Could not define dimension {dim_name}')

    # Check if existing dimension size changed and set new value, if appropriate
    existing_dims = [dim_name for dim_name in adi_dims if dim_name in xr_dims]

    for dim_name in existing_dims:
        adi_dim_size = adi_dims[dim_name].get_length()
        xr_dim_size = xr_dims[dim_name]

        if adi_dim_size != xr_dim_size:
            status = dsproc.set_dim_length(adi_dataset, dim_name, xr_dim_size)

            if status == 0:
                raise Exception(f'Could not change dimension length of {dim_name}')


def _add_variable_to_adi(xr_var: xr.DataArray, adi_dataset: cds3.Group):
    """-----------------------------------------------------------------------
    Add a new variable specified by an xarray DataArray to the given ADI
    dataset.
    -----------------------------------------------------------------------"""
    # First create the variable
    cds_type = get_cds_type(xr_var.data)
    dim_names = xr_dims = list(xr_var.dims)
    adi_var = dsproc.define_var(adi_dataset, xr_var.name, cds_type, dim_names)

    # Now assign attributes
    _sync_attrs(xr_var.attrs, adi_var)

    # Now set the data
    _set_adi_variable_data(xr_var, adi_var)

    # Finally, need to check SpecialXrAttributes.COORDINATE_SYSTEM and SpecialXrAttributes.OUTPUT_TARGETS
    coord_sys_name = xr_var.attrs.get(SpecialXrAttributes.COORDINATE_SYSTEM, None)
    dsproc.set_var_coordsys_name(adi_var, coord_sys_name)

    output_targets = xr_var.attrs.get(SpecialXrAttributes.OUTPUT_TARGETS, None)
    if output_targets is not None:
        for dsid, datastream_variable_name in output_targets:
            dsproc.add_var_output_target(adi_var, dsid, datastream_variable_name)


def _set_time_variable_data_if_needed(xr_var: xr.DataArray, adi_var: cds3.Var):
    """-----------------------------------------------------------------------
    Check to see if the time values have changed, and if so, then push back to
    ADI.  We can't rely on the data pointer for time, because the times are
    converted into datetime64 objects for xarray.

    TODO: if this becomes a performance issue to do this comparison, then we
    can add a parameter to the sync dataset method so that the user can
    explicitly declare whether syncing the time variable is needed or not
    -----------------------------------------------------------------------"""
    # astype will produce nanosecond precision, so we have to convert to seconds
    timevals = xr_var.data.astype('float') / 1000000000

    # We have to truncate to 6 decimal places so it matches ADI
    timevals = np.around(timevals, 6)

    # Compare with the original values to see if there have been any changes
    adi_times = dsproc.get_sample_timevals(adi_var, 0)

    if np.array_equal(timevals, adi_times) is False:

        # Wipe out any existing data
        adi_var.delete_data()

        # Set the timevals in seconds in ADI
        dsproc.set_sample_timevals(adi_var, 0, timevals)


def _set_adi_variable_data(xr_var: xr.DataArray, adi_var: cds3.Var):
    """-----------------------------------------------------------------------
    For the given Xarray DataArray, copy the data values back to ADI.  This
    method will only be called if the deveoper has replaced the original
    DataArray of values with a new array by means of a Python operation.
    In this case, the data values will no longer be mapped directly to the
    ADI data structure, so they will have to be manually copied over.
    -----------------------------------------------------------------------"""
    missing_value = None
    missing_values = xr_var.attrs.get(ADIAtts.MISSING_VALUE)
    if missing_values is not None and len(missing_values) > 0:
        missing_value = missing_values[0]

    sample_start = 0

    # Get the length of the first dimension
    sample_count = 1
    if len(xr_var.dims) > 0:
        sample_count = xr_var.sizes[xr_var.dims[0]]

    # Wipe out any existing data
    adi_var.delete_data()

    # Store the new values
    status = dsproc.set_var_data(adi_var, sample_start, sample_count, missing_value, xr_var.data)

    if status is None:
        raise Exception(f'Could not set data for variable {adi_var.get_name()}')


def _sync_vars(xr_dataset: xr.Dataset, adi_dataset: cds3.Group):
    """-----------------------------------------------------------------------
    Sync Xarray variables back to ADI dataset by checking if:
        - a variable's attributes were changed
        - a variable’s dimensions changed
        - a variable was added or deleted
        - a variable's data array was replaced

    Args:
        xr_dataset (xr.Dataset):
            The xarray dataset to sync

        adi_dataset (csd3.Group):
            The ADI group where changes will be applied

    -----------------------------------------------------------------------"""
    adi_var_objs = get_dataset_vars(adi_dataset)
    adi_vars = {var.get_name() for var in adi_var_objs}
    xr_vars = {var_name for var_name in xr_dataset.variables}

    # First remove deleted vars from the dataset
    deleted_vars = [var_name for var_name in adi_vars if var_name not in xr_vars]
    for var_name in deleted_vars:
        adi_var = dsproc.get_var(adi_dataset, var_name)
        dsproc.delete_var(adi_var)

    # Then add new vars to the dataset
    added_vars = [var_name for var_name in xr_vars if var_name not in adi_vars]
    for var_name in added_vars:
        xr_var: xr.DataArray = xr_dataset.get(var_name)
        _add_variable_to_adi(xr_var, adi_dataset)

    # Now sync up the remaining variables if they have been changed
    other_vars = [var_name for var_name in xr_vars if var_name not in added_vars]
    for var_name in other_vars:
        xr_var: xr.DataArray = xr_dataset.get(var_name)
        adi_var: cds3.Var = dsproc.get_var(adi_dataset, var_name)

        if adi_var is None:
            # adi_var may be None only for coordinate variables of transformed datasets, since the coordinate vars
            # are stored on the parent Group in adi.  Users should never be changing the coordinate variables on
            # transformed datasets, so we are skipping these.
            ADILogger.info(f'Not syncing coordinate variable {var_name} in transformed dataset {adi_dataset.get_name()} because it comes from the parent coordinate system dataset.')
            continue

        # Check if dims have changed
        adi_dims: List[str] = adi_var.get_dim_names()
        xr_dims = list(xr_var.dims)
        if adi_dims != xr_dims:
            raise Exception('Changing dimensions on an existing variable is not supported by ADI!')

        # sync attributes
        _sync_attrs(xr_var.attrs, adi_var)

        # sync data
        # If the data pointer has changed or does not exist, we need to wipe out any previous data and then
        # create new data for the adi variable
        adi_data = adi_var.get_datap()
        adi_pointer = adi_data.__array_interface__['data'][0] if adi_data is not None else None
        xr_pointer = xr_var.data.__array_interface__['data'][0]

        # If the pointers don't match, then we also compare all the values in each data array,
        # and only if values are different do we sync the values back to ADI.  Note:  we have
        # to do this because in the xarray dataset.from_dict() method, xarray always changes
        # the arrays of coordinate variables, so the pointers will never match.
        if adi_pointer != xr_pointer:
            if var_name == 'time':
                _set_time_variable_data_if_needed(xr_var, adi_var)
            elif adi_data is None or (adi_data != xr_var.data).any():
                _set_adi_variable_data(xr_var, adi_var)

        # TODO: I don't think we need to change SpecialXrAttributes.COORDINATE_SYSTEM on an existing
        # variable, but if there is a use case for it, we should add it here

        # TODO: I don't think we need to change SpecialXrAttributes.OUTPUT_TARGETS on an existing
        # variable, but if there is a use case for it, we should add it here


def sync_xarray(xr_dataset: xr.Dataset, adi_dataset: cds3.Group):
    """-----------------------------------------------------------------------
    Carefully inspect the xr.Dataset and synchronize any changes back to the
    given ADI dataset.

    Args:
        xr_dataset (xr.Dataset): The XArray dataset to sync

        adi_dataset (csd3.Group): The ADI dataset where changes will be applied

    -----------------------------------------------------------------------"""

    # Sync global attributes
    _sync_attrs(xr_dataset.attrs, adi_dataset)

    # Sync dimensions
    _sync_dims(xr_dataset.sizes, adi_dataset)

    # Sync variables
    _sync_vars(xr_dataset, adi_dataset)


def correct_zero_length_dims(dsid: Optional[int] = None, datastream_name: Optional[str] = None):
    """------------------------------------------------------------------------------------------
    Using dsproc APIs, inspect the given output dataset.  If there are dimensions with 0 length,
    correct them to be of length 1.  For corresponding coordinate variables, create a data
    array of length 1 and set the value to -9999.

    Apparently you only have to add values for the coordinate variable - Krista says ADI will
    automatically fill out missing values for the second dimension for data variables.

    Args:
        dsid (Optional[int]): The dsproc dsid of the dataset (used to find the dataset)

        datastream_name (Optional[str]): Or alternatively, the datastream name of the output dataset

    ------------------------------------------------------------------------------------------"""
    if dsid is None:
        dsid = get_datastream_id(datastream_name, dataset_type=ADIDatasetType.OUTPUT)

    if dsid is not None:
        # Loop through all the files in the dataset
        for i in itertools.count(start=0):
            adi_dataset: cds3.Group = dsproc.get_output_dataset(dsid, i)
            if not adi_dataset:
                break

            # Get all the dimensions associated with this dataset 
            adi_dims: List[cds3.Dim] = get_dataset_dims(adi_dataset)

            # Loop over dimensions
            for dim in adi_dims:
                dimension_name: str = dim.get_name()
                if dimension_name == "time" or dimension_name == "bound": 
                    continue

                # Check to see if any dimensions have zero length and if so set their 
                # length to 1 and any dependent variable values to missing.
                if dim.get_length() == 0:
                    new_dim_length: int = 1

                    # Change the dimension length to 1 in dsproc
                    ADILogger.info(f'No length set for {dimension_name}, setting length to 1')
                    status = dsproc.set_dim_length(adi_dataset, dimension_name, new_dim_length)
                    if status == 0:
                        raise Exception(
                            f'Could not set dimension {dimension_name} length = {new_dim_length}')

                    # If there is a coordinate variable for this dimension, then
                    # use dsproc to set value to -9999
                    adi_var = dsproc.get_output_var(dsid, dimension_name, obs_index=i)
                    if adi_var is None:
                        ADILogger.info(f'No coordinate variable exists for dimension {dimension_name}.')
                    else:
                        coordvar_data = dsproc.alloc_var_data_index(adi_var, 0, new_dim_length)
                        if coordvar_data is None:
                            raise Exception(
                                f'Could not allocate data for {dimension_name} coordinate variable.')
                        
                        coordvar_data[0] = -9999   


def get_xr_datasets(
    dataset_type: ADIDatasetType,
    dsid: Optional[int] = None,
    datastream_name: Optional[str] = None,
    site: Optional[str] = None,
    facility: Optional[str] = None,
    coordsys_name: Optional[str] = None
) -> List[xr.Dataset]:
    """-----------------------------------------------------------------------
    Get an ADI dataset converted to an xarray.Dataset.

    Args:
        dataset_type (ADIDatasetType):
            The type of the dataset to convert (RETRIEVED, TRANSFORMED, OUTPUT)

        dsid (int):
            If the dsid is known, you can use it to look up the adi dataset.  If it is not known,
            then use datastream_name, and optionally site/facility to identify the dataset.

        datastream_name (str):
            The name of one of the process' datastreams as specified in the PCM.

        coordsys_name (str):
            Optional parameter used only to find TRANSFORMED datasets.  Must be a coordinate
            system specified in the PCM or None if no coordinate system was specified.

        site (str):
            Optional parameter used only to find some input datasets (RETRIEVED or TRANSFORMED).
            Site is only required if the retrieval rules in the PCM specify two different
            rules for the same datastream that differ by site.

        facility (str):
            Optional parameter used only to find some input datasets (RETRIEVED or TRANSFORMED).
            Facility is only required if the retrieval rules in the PCM specify two different
            rules for the same datastream that differ by facility.

    Returns:
        List[xr.Dataset]: Returns a list of xr.Datasets, one for each file. If there are
            no files / datasets for the specified datastream / site / facility / coord system
            then the list will be empty.
    -----------------------------------------------------------------------"""
    datasets: List[xr.Dataset] = []
    if dsid is None:
        dsid = get_datastream_id(datastream_name, site=site, facility=facility, dataset_type=dataset_type)

    if dsid is not None:
        for i in itertools.count(start=0):

            if dataset_type is ADIDatasetType.RETRIEVED:
                adi_dataset = dsproc.get_retrieved_dataset(dsid, i)
            elif dataset_type is ADIDatasetType.TRANSFORMED:
                adi_dataset = dsproc.get_transformed_dataset(coordsys_name, dsid, i)
            else:
                adi_dataset = dsproc.get_output_dataset(dsid, i)

            if not adi_dataset:
                break

            xr_dataset: xr.Dataset = to_xarray(adi_dataset)

            # Add special metadata
            xr_dataset.attrs[SpecialXrAttributes.DATASET_TYPE] = dataset_type
            xr_dataset.attrs[SpecialXrAttributes.DATASTREAM_DSID] = dsid
            xr_dataset.attrs[SpecialXrAttributes.OBS_INDEX] = i
            if coordsys_name is not None:
                xr_dataset.attrs[SpecialXrAttributes.COORDINATE_SYSTEM] = coordsys_name

            datasets.append(xr_dataset)

    return datasets


def sync_xr_dataset(xr_dataset: xr.Dataset):
    """-----------------------------------------------------------------------
    Sync the contents of the given XArray.Dataset with the corresponding ADI
    data structure.

    Args:
        xr_dataset (xr.Dataset):  The xr.Dataset(s) to sync.

    -----------------------------------------------------------------------"""
    dataset_type = xr_dataset.attrs[SpecialXrAttributes.DATASET_TYPE]
    obs_index = xr_dataset.attrs[SpecialXrAttributes.OBS_INDEX]
    coordsys_name = xr_dataset.attrs.get(SpecialXrAttributes.COORDINATE_SYSTEM)
    dsid =  int(xr_dataset.attrs[SpecialXrAttributes.DATASTREAM_DSID])

    if dataset_type is ADIDatasetType.RETRIEVED:
        adi_dataset = dsproc.get_retrieved_dataset(dsid, obs_index)
    elif dataset_type is ADIDatasetType.TRANSFORMED:
        adi_dataset = dsproc.get_transformed_dataset(coordsys_name, dsid, obs_index)
    else:
        adi_dataset = dsproc.get_output_dataset(dsid, obs_index)

    sync_xarray(xr_dataset, adi_dataset)


def get_datastream_files(dsid: int, begin_date: int, end_date: int) -> List[str]:
    """-----------------------------------------------------------------------
    Return the full path to each data file found for the given datastream
    and time range.

    Args:
        dsid (int): the datastream id (call get_dsid() to retrieve)

        begin_date (int): the begin timestamp of the current processing interval
            (seconds since 1970)

        end_date (int): the end timestamp of the current processing interval
            (seconds since 1970)

    Returns:
        List[str]: A list of file paths that match the datastream query.
    -----------------------------------------------------------------------"""
    datastream_path = dsproc.datastream_path(dsid)
    files = dsproc.find_datastream_files(dsid, begin_date, end_date)
    file_paths = []
    for file in files:
        file_path = f"{datastream_path}/{file}"
        file_paths.append(file_path)

    return file_paths
