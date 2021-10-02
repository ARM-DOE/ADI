import numpy as np
import xarray as xr
from typing import Callable, List

from .constants import SpecialXrAttributes
from .process import Process


@xr.register_dataset_accessor('adi')
class ADIDatasetAccessor:
    """-----------------------------------------------------------------------
    Used to apply special ADI functions to an xarray dataset with the
    namespace 'adi'

    -----------------------------------------------------------------------"""

    def __init__(self, xarray_obj):
        self._obj = xarray_obj

    def get_qc_variable(self, variable_name: str):
        return Process.get_qc_variable(self._obj, variable_name)

    def add_variable(self,
                     variable_name: str,
                     dim_names: List[str],
                     data: np.ndarray,
                     long_name: str = None,
                     standard_name: str = None,
                     units: str = None,
                     valid_min=None,
                     valid_max=None,
                     missing_value: np.ndarray = None,
                     fill_value=None):
        return Process.add_variable(self._obj, variable_name, dim_names, data,
                                    long_name=long_name,
                                    standard_name=standard_name,
                                    units=units,
                                    valid_min=valid_min,
                                    valid_max=valid_max,
                                    missing_value=missing_value,
                                    fill_value=fill_value)

    def add_qc_variable(self, variable_name: str):
        return Process.add_qc_variable(self._obj, variable_name)

    def get_companion_transform_variable_names(self, variable_name: str) -> List[str]:
        return Process.get_companion_transform_variable_names(self._obj, variable_name)

    def drop_variables(self, variable_names: List[str]) -> xr.Dataset:
        return Process.drop_variables(self._obj, variable_names)

    def drop_transform_metadata(self, variable_names: List[str]) -> xr.Dataset:
        return Process.drop_transform_metadata(self._obj, variable_names)

    def variables_exist(self, variable_names: List[str] = []) -> np.ndarray:
        return Process.variables_exist(self._obj, variable_names)

    def record_qc_results(self,
                          variable_name: str,
                          bit_number: int = None,
                          test_results: np.ndarray = None):
        Process.record_qc_results(self._obj, variable_name, bit_number, test_results)

    def convert_units(self,
                      old_units: str,
                      new_units: str,
                      variable_names: List[str] = None,
                      converter_function: Callable = None):
        Process.convert_units([self._obj], old_units, new_units, variable_names, converter_function)


@xr.register_dataarray_accessor('adi')
class ADIDataArrayAccessor:
    """-----------------------------------------------------------------------
    Used to apply special ADI functions to an xarray data array (i.e., variable)
    with the namespace 'adi'

    -----------------------------------------------------------------------"""

    def __init__(self, xarray_obj):
        self._obj = xarray_obj

    def assign_coordinate_system(self, coordinate_system_name: str):
        Process.assign_coordinate_system_to_variable(self._obj, coordinate_system_name)

    def assign_output_datastream(self, output_datastream_name: str, variable_name_in_datastream: str = None):
        Process.assign_output_datastream_to_variable(self._obj, output_datastream_name, variable_name_in_datastream)

    @property
    def source_ds_name(self) -> str:
        return self._obj.attrs.get(SpecialXrAttributes.SOURCE_DS_NAME, None)

    @property
    def source_var_name(self) -> str:
        return self._obj.attrs.get(SpecialXrAttributes.SOURCE_VAR_NAME, None)

    @property
    def nsamples(self) -> int:
        return Process.get_nsamples(self._obj)
