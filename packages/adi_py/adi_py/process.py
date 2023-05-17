"""-----------------------------------------------------------------------
This module contains the **Process** class which should be used for
implementing ADI processes in Python.

Important:
    All new ADI Python processes MUST extend this base class.  This class
    implements empty hook functions for all possible ADI hooks.  You
    only need to override the hook methods that are required for your
    process.
-----------------------------------------------------------------------"""
import numpy as np
import os
import re
import sys
import warnings
import xarray as xr
from time import gmtime, strftime
from typing import Any, Callable, List, Union, Optional

from .constants import ADIDatasetType, ADIAtts, SpecialXrAttributes, SplitMode, TransformAttributes
from .logger import ADILogger
from .utils import get_datastream_id, get_xr_datasets, sync_xr_dataset, get_datastream_files, adi_hook_exception_handler, \
    DatastreamIdentifier

try:
    import dsproc3 as dsproc
    import cds3 as cds
except ImportError as e:
    print("Could not import ADI")


class Process:
    """-----------------------------------------------------------------------
    The base class for running an ADI process in Python.  All Python processes
    should extend this class.
    -----------------------------------------------------------------------"""

    def __init__(self):
        self._process_names = []
        self._process_model = dsproc.PM_TRANSFORM_VAP
        self._process_version = ''
        self._rollup_qc = False
        self._include_debug_dumps = True

    @property
    def process_names(self) -> List[str]:
        """-----------------------------------------------------------------------
        The name(s) of the process(es) that could run this code.  Subclasses must
        define the self._names field in their constructor.

        Returns:
            List[str]: One or more process names
        -----------------------------------------------------------------------"""
        return self._process_names or ['']

    @property
    def process_name(self) -> str:
        """-----------------------------------------------------------------------
        The name of the process that is currently being run.

        Returns:
            str: the name of the current process
        -----------------------------------------------------------------------"""
        return dsproc.get_name()

    @property
    def debug_level(self) -> int:
        """-----------------------------------------------------------------------
        Get the debug level passed on the command line when running the process.

        Returns:
            int: the debug level
        -----------------------------------------------------------------------"""
        return dsproc.get_debug_level()

    @property
    def process_model(self) -> int:
        """-----------------------------------------------------------------------
        The processing model to use.  It can be one of:

        - dsproc.PM_GENERIC
        - dsproc.PM_INGEST
        - dsproc.PM_RETRIEVER_INGEST
        - dsproc.PM_RETRIEVER_VAP
        - dsproc.PM_TRANSFORM_INGEST
        - dsproc.PM_TRANSFORM_VAP

        Default value is PM_TRANSFORM_VAP.  Subclasses can override in their constructor.

        Returns:
            int: The processing modelj (see dsproc.ProcModel cdeftype)
        -----------------------------------------------------------------------"""
        return self._process_model or dsproc.PM_TRANSFORM_VAP

    @property
    def process_version(self) -> str:
        """-----------------------------------------------------------------------
        The version of this process's code.  Subclasses must define the
        self._process_version field in their constructor.

        Returns:
            str: The process version
        -----------------------------------------------------------------------"""
        return self._process_version or 'not_specified'

    @property
    def rollup_qc(self) -> bool:
        """-----------------------------------------------------------------------
        ADI setting controlling whether all the qc bits are rolled up into a
        single 0/1 value or not.

        Returns:
            bool: Whether this process should rollup qc or not
        -----------------------------------------------------------------------"""
        return self._rollup_qc

    @property
    def include_debug_dumps(self) -> bool:
        """-----------------------------------------------------------------------
        Setting controlling whether this process should provide debug dumps of the
        data after each hook.

        Returns:
            bool: Whether debug dumps should be automatically included.  If True
            and debug level is > 1, then debug dumps will be performed
            automatically before and after each code hook.
        -----------------------------------------------------------------------"""
        return self._include_debug_dumps

    @property
    def location(self) -> dsproc.PyProcLoc:
        """-----------------------------------------------------------------------
        Get the location where this invocation of the process is running.

        Returns:
            dsproc.PyProcLoc: A class containing the alt, lat, and lon where the
            process is running.
        -----------------------------------------------------------------------"""
        return dsproc.get_location()

    @property
    def site(self) -> str:
        """-----------------------------------------------------------------------
        Get the site where this invocation of the process is running

        Returns:
            str: The site where this process is running
        -----------------------------------------------------------------------"""
        return dsproc.get_site()

    @property
    def facility(self) -> str:
        """-----------------------------------------------------------------------
        Get the facility where this invocation of the process is running

        Returns:
            str: The facility where this process is running
        -----------------------------------------------------------------------"""
        return dsproc.get_facility()

    @staticmethod
    def get_dsid(datastream_name: str, site: str = None, facility: str = None,
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
        return get_datastream_id(datastream_name, site=site, facility=facility, dataset_type=dataset_type)

    @staticmethod
    def get_retrieved_dataset(
        input_datastream_name: str,
        site: Optional[str] = None,
        facility: Optional[str] = None,
    ) -> Optional[xr.Dataset]: 
        """-----------------------------------------------------------------------
        Get an ADI retrieved dataset converted to an xr.Dataset.

        Note: This method will return at most a single xr.Dataset. If you expect
        multiple datasets, or would like to handle cases where multiple dataset files
        may be retrieved, please use the `Process.get_retrieved_datasets()` function.

        Args:
            input_datastream_name (str):
                The name of one of the process' input datastreams as specified in the PCM.

            site (str):
                Optional parameter used only to find some input datasets (RETRIEVED or TRANSFORMED).
                Site is only required if the retrieval rules in the PCM specify two different
                rules for the same datastream that differ by site.

            facility (str):
                Optional parameter used only to find some input datasets (RETRIEVED or TRANSFORMED).
                Facility is only required if the retrieval rules in the PCM specify two different
                rules for the same datastream that differ by facility.

        Returns:
            xr.Dataset | None: Returns a single xr.Dataset, or None if no retrieved datasets
                exist for the specified datastream / site / facility.
        -----------------------------------------------------------------------"""
        datasets = Process.get_retrieved_datasets(
            input_datastream_name=input_datastream_name,
            site=site,
            facility=facility,
        )
        datasets = get_xr_datasets(
            ADIDatasetType.RETRIEVED,
            datastream_name=input_datastream_name,
            site=site,
            facility=facility,
        )
        if not datasets:
            return None
        if len(datasets) > 1:
            raise Exception(f'Datastream "{input_datastream_name}" contains more than one observation (i.e., file)'
                            f' of data.  Please use the  get_retrieved_datasets() method to get the full list of Xarray'
                            f' datasets (one for each file).')
        return datasets[0]

    @staticmethod
    def get_retrieved_datasets(
        input_datastream_name: str,
        site: Optional[str] = None,
        facility: Optional[str] = None,
    ) -> List[xr.Dataset]:
        """-----------------------------------------------------------------------
        Get the ADI retrieved datasets converted to a list of xarray Datasets.

        Args:
            input_datastream_name (str):
                The name of one of the process' input datastreams as specified in the PCM.

            site (str):
                Optional parameter used only to find some input datasets (RETRIEVED or TRANSFORMED).
                Site is only required if the retrieval rules in the PCM specify two different
                rules for the same datastream that differ by site.

            facility (str):
                Optional parameter used only to find some input datasets (RETRIEVED or TRANSFORMED).
                Facility is only required if the retrieval rules in the PCM specify two different
                rules for the same datastream that differ by facility.

        Returns:
            List[xr.Dataset]: Returns a list of xr.Datasets. If no retrieved datasets
                exist for the specified datastream / site / facility / coord system
                then the list will be empty.
        -----------------------------------------------------------------------"""
        return get_xr_datasets(
            ADIDatasetType.RETRIEVED,
            datastream_name=input_datastream_name,
            site=site,
            facility=facility,
        )

    @staticmethod
    def get_retrieved_dataset_by_dsid(dsid: int) -> Optional[xr.Dataset]:
        datasets = get_xr_datasets(ADIDatasetType.RETRIEVED, dsid=dsid)
        if not datasets:
            return None
        if len(datasets) > 1:
            raise Exception(f'Datastream "{dsid}" contains more than one observation (i.e., file) of data.  '
                            f'Please use the get_retrieved_datasets_by_dsid() method to get the full list of Xarray'
                            f' datasets (one for each file).')
        return datasets[0]
    
    @staticmethod
    def get_retrieved_datasets_by_dsid(dsid: int) -> List[xr.Dataset]:
        return get_xr_datasets(ADIDatasetType.RETRIEVED, dsid=dsid)

    @staticmethod
    def get_transformed_dataset(
        input_datastream_name: str,
        coordinate_system_name: str,
        site: Optional[str] = None,
        facility: Optional[str] = None,
    ) -> Optional[xr.Dataset]: 
        """-----------------------------------------------------------------------
        Get an ADI transformed dataset converted to an xr.Dataset.

        Note: This method will return at most a single xr.Dataset. If you expect
        multiple datasets, or would like to handle cases where multiple dataset files
        may be retrieved, please use the `Process.get_retrieved_datasets()` function.

        Args:
            input_datastream_name (str):
                The name of one of the process' input datastreams as specified in the PCM.

            coordinate_system_name (str):  
                A coordinate system specified in the PCM or None if no coordinate system was
                specified.

            site (str):
                Optional parameter used only to find some input datasets (RETRIEVED or TRANSFORMED).
                Site is only required if the retrieval rules in the PCM specify two different
                rules for the same datastream that differ by site.

            facility (str):
                Optional parameter used only to find some input datasets (RETRIEVED or TRANSFORMED).
                Facility is only required if the retrieval rules in the PCM specify two different
                rules for the same datastream that differ by facility.

        Returns:
            xr.Dataset | None: Returns a single xr.Dataset, or None if no transformed
                datasets exist for the specified datastream / site / facility / coord system.
        -----------------------------------------------------------------------"""
        datasets = Process.get_transformed_datasets(
            input_datastream_name=input_datastream_name,
            coordinate_system_name=coordinate_system_name,
            site=site,
            facility=facility,
        )
        if not datasets:
            return None
        if len(datasets) > 1:
            raise Exception(f'Datastream "{input_datastream_name}" contains more than one observation (i.e., file)'
                            f' of data.  Please use the get_transformed_datasets() method to get the full list of Xarray'
                            f' datasets (one for each file).')
        return datasets[0]

    @staticmethod
    def get_transformed_datasets(
        input_datastream_name: str,
        coordinate_system_name: str,
        site: Optional[str] = None,
        facility: Optional[str] = None,
    ) -> List[xr.Dataset]: 
        """-----------------------------------------------------------------------
        Get an ADI transformed dataset converted to an xr.Dataset.

        Args:
            input_datastream_name (str):
                The name of one of the process' input datastreams as specified in the PCM.

            coordinate_system_name (str):  
                A coordinate system specified in the PCM or None if no coordinate system was
                specified.

            site (str):
                Optional parameter used only to find some input datasets (RETRIEVED or TRANSFORMED).
                Site is only required if the retrieval rules in the PCM specify two different
                rules for the same datastream that differ by site.

            facility (str):
                Optional parameter used only to find some input datasets (RETRIEVED or TRANSFORMED).
                Facility is only required if the retrieval rules in the PCM specify two different
                rules for the same datastream that differ by facility.

        Returns:
            List[xr.Dataset]: Returns a list of xr.Datasets. If no transformed datasets
                exist for the specified datastream / site / facility / coord system
                then the list will be empty.
        -----------------------------------------------------------------------"""
        return get_xr_datasets(
            ADIDatasetType.TRANSFORMED,
            datastream_name=input_datastream_name,
            coordsys_name=coordinate_system_name,
            site=site,
            facility=facility,
        )

    @staticmethod
    def get_transformed_dataset_by_dsid(dsid: int, coordinate_system_name: str) -> Optional[xr.Dataset]:
        datasets = get_xr_datasets(ADIDatasetType.TRANSFORMED, dsid=dsid, coordsys_name=coordinate_system_name)
        if not datasets:
            return None
        if len(datasets) > 1:
            raise Exception(f'Datastream "{dsid}" contains more than one observation (i.e., file) of data.  '
                            f'Please use the  get_transformed_datasets_by_dsid() method to get the full list of Xarray'
                            f' datasets (one for each file).')
        return datasets[0]

    @staticmethod
    def get_transformed_datasets_by_dsid(dsid: int, coordinate_system_name: str) -> List[xr.Dataset]:
        return get_xr_datasets(ADIDatasetType.TRANSFORMED, dsid=dsid, coordsys_name=coordinate_system_name)

    @staticmethod
    def get_output_dataset(output_datastream_name: str) -> Optional[xr.Dataset]: 
        """-----------------------------------------------------------------------
        Get an ADI output dataset converted to an xr.Dataset.

        Note: This method will return at most a single xr.Dataset. If you expect
        multiple datasets, or would like to handle cases where multiple dataset files
        may be retrieved, please use the `Process.get_retrieved_datasets()` function.

        Args:
            output_datastream_name (str):
                The name of one of the process' output datastreams as specified in the PCM.

        Returns:
            xr.Dataset | None: Returns a single xr.Dataset, or None if no output
                datasets exist for the specified datastream / site / facility / coord system.
        -----------------------------------------------------------------------"""
        datasets = Process.get_output_datasets(output_datastream_name)
        if not datasets:
            return None
        if len(datasets) > 1:
            raise Exception(f'Datastream "{output_datastream_name}" contains more than one observation (i.e., file)'
                            f' of data.  Please use the  get_output_datasets() method to get the full list of Xarray'
                            f' datasets (one for each file).')
        return datasets[0]

   
    @staticmethod
    def get_output_datasets(output_datastream_name: str) -> List[xr.Dataset]: 
        """-----------------------------------------------------------------------
        Get an ADI output dataset converted to an xr.Dataset.
        
        Args:
            output_datastream_name (str):
                The name of one of the process' output datastreams as specified in the PCM.

        Returns:
            List[xr.Dataset]: Returns a list of xr.Datasets. If no output datasets
                exist for the specified datastream / site / facility / coord system
                then the list will be empty.
        -----------------------------------------------------------------------"""
        return get_xr_datasets(ADIDatasetType.OUTPUT, datastream_name=output_datastream_name)

    @staticmethod
    def get_output_dataset_by_dsid(dsid: int) -> Optional[xr.Dataset]:
        datasets = get_xr_datasets(ADIDatasetType.OUTPUT,  dsid=dsid)
        if not datasets:
            return None
        if len(datasets) > 1:
            raise Exception(f'Datastream "{dsid}" contains more than one observation (i.e., file) of data.  '
                            f'Please use the get_output_datasets_by_dsid() method to get the full list of Xarray'
                            f' datasets (one for each file).')
        return datasets[0]
    
    @staticmethod
    def get_output_datasets_by_dsid(dsid: int) -> List[xr.Dataset]:
        return get_xr_datasets(ADIDatasetType.OUTPUT,  dsid=dsid)

    @staticmethod
    def sync_datasets(*args: xr.Dataset):
        """-----------------------------------------------------------------------
        Sync the contents of one or more XArray.Datasets with the corresponding ADI
        data structure.

        Important:
            This method MUST be called at the end of a hook function if any changes
            have been made to the XArray Dataset so that updates can be pushed back to ADI.

        Important:
            This dataset must have been previously loaded via one of
            the get_*_dataset methods in order to have the correct embedded
            metadata to be able to sync to ADI.  Specifically, this will include
            datastream name, coordinate system, dataset type, and obs_index.

        See Also:
            - get_retrieved_dataset
            - get_transformed_dataset
            - get_output_dataset

        Args:
            *args (xr.Dataset): One or more xr.Datasets to sync.  **Note:**
                These datasets must have been previously loaded via one of the
                get_*_dataset methods in order to have the correct embedded
                metadata to be able to sync to ADI.  Specifically, this will
                include datastream name, coordinate system, dataset type, and
                obs_index.
        -----------------------------------------------------------------------"""
        for xr_dataset in args:
            if xr_dataset is not None:
                sync_xr_dataset(xr_dataset)

    @staticmethod
    def set_datastream_flags(dsid: int, flags: int):
        """-----------------------------------------------------------------------
        Apply a set of ADI control flags to a datastream as identified by the
        dsid.  Multiple flags can be combined together using a bitwise OR (e.g.,
        dsproc.DS_STANDARD_QC | dsproc.DS_FILTER_NANS). The allowed flags are
        identified below:

        - dsproc.DS_STANDARD_QC     = Apply standard QC before storing a dataset.
    
        - dsproc.DS_FILTER_NANS     = Replace NaN and Inf values with missing values
                                      before storing a dataset.
    
        - dsproc.DS_OVERLAP_CHECK   = Check for overlap with previously processed data.
                                      This flag will be ignored and the overlap check
                                      will be skipped if reprocessing mode is enabled,
                                      or asynchronous processing mode is enabled.
    
        - dsproc.DS_PRESERVE_OBS    = Preserve distinct observations when retrieving
                                      data. Only observations that start within the
                                      current processing interval will be read in.
    
        - dsproc.DS_DISABLE_MERGE   = Do not merge multiple observations in retrieved
                                      data. Only data for the current processing interval
                                      will be read in.
    
        - dsproc.DS_SKIP_TRANSFORM  = Skip the transformation logic for all variables
                                      in this datastream.
    
        - dsproc.DS_ROLLUP_TRANS_QC = Consolidate the transformation QC bits for all
                                      variables when mapped to the output datasets.
    
        - dsproc.DS_SCAN_MODE       = Enable scan mode for datastream that are not
                                      expected to be continuous. This prevents warning
                                      messages from being generated when data is not
                                      found within a processing interval. Instead, a
                                      message will be written to the log file indicating
                                      that the procesing interval was skipped.
    
        - dsproc.DS_OBS_LOOP        = Loop over observations instead of time intervals.
                                      This also sets the DS_PRESERVE_OBS flag.
    
        - dsproc.DS_FILTER_VERSIONED_FILES = Check for files with .v# version extensions
                                             and filter out lower versioned files. Files
                                             without a version extension take precedence.

        Call self.get_dsid() to obtain the dsid value for a specific datastream.
        If the flags value is < 0, then the following default flags will be set:
        - dsprc.DS_STANDARD_QC              'b' level datastreams
        - dsproc.DS_FILTER_NANS             'a' and 'b' level datastreams
        - dsproc.DS_OVERLAP_CHECK           all output datastreams
        - dsproc.DS_FILTER_VERSIONED_FILES  input datastreams that are not level '0'

        Args:
            dsid (int):  Datastream ID
            flags (int): Flags to set

        Returns:
            int: The processing modelj (see dsproc.ProcModel cdeftype)
        -----------------------------------------------------------------------"""
        dsproc.set_datastream_flags(dsid, flags)

    @staticmethod
    def get_datastream_files(datastream_name: str, begin_date: int, end_date: int) -> List[str]:
        """-----------------------------------------------------------------------
        See :func:`.utils.get_datastream_files`
        -----------------------------------------------------------------------"""
        dsid = get_datastream_id(datastream_name)
        return get_datastream_files(dsid, begin_date, end_date)

    @staticmethod
    def get_nsamples(xr_var: xr.DataArray) -> int:
        """-----------------------------------------------------------------------
        Get the ADI sample count for the given variable (i.e., the length
        of the first dimension or 1 if the variable has no dimensions)

        Args:
            xr_var (xr.DataArray):

        Returns:
            int: The ADI sample count
        -----------------------------------------------------------------------"""
        # Get the length of the first dimension
        sample_count = 1
        if len(xr_var.dims) > 0:
            sample_count = xr_var.sizes[xr_var.dims[0]]

        return sample_count

    @staticmethod
    def variables_exist(xr_dataset: xr.Dataset, variable_names: np.ndarray = np.array([])) -> np.ndarray:
        """-----------------------------------------------------------------------
        Check if the given variables exist in the given dataset.

        Args:
            xr_dataset (xr.Dataset): The dataset
            variable_names (np.ndarray[str]): The variable names to check.  Any
                array-like object can be provided, but ndarrays work best in
                order to easily select missing or existing variables from
                the results array (mask).

        Returns:
            np.ndarray: Array of same length as variable_names where each value is
            True or False.  True if the variable exists.  Use np.ndarray.all() to
            check if all the variables exist.
        -----------------------------------------------------------------------"""

        shape = [len(variable_names)]
        statuses = np.full(shape, False, dtype=np.bool)
        for idx, var_name in enumerate(variable_names):
            if xr_dataset.get(var_name) is not None:
                statuses[idx] = True

        return statuses

    @staticmethod
    def convert_units(xr_datasets: List[xr.Dataset],
                      old_units: str,
                      new_units: str,
                      variable_names: List[str] = None,
                      converter_function: Callable = None):
        """-----------------------------------------------------------------------
        For the specified variables, convert the units from old_units to new_units.
        For applicable variables, this conversion will include changing the units
        attribute value and optionally converting all the data values if a converter
        function is provided.

        This method is needed for special cases where the units conversion is not
        supported by udunits and the default ADI converters.

        Args:
            xr_datasets (List[xr.Dataset]): One or more xarray datasets upon which
                to apply the conversion

            old_units (str): The old units (e.g., 'degree F')

            new_units (str): The new units (e.g., 'K')

            variable_names (List[str]): A list of specific variable names to convert.
                If not specified, it converts all variables with the given old_units
                to new_units.

            converter_function (): A function to run on an Xarray variable (i.e.,
                DataArray that converts a variable's values from old_units to new_units.
                If not specified, then only the units attribute value will be changed. This
                could happen if we just want to change the units attribute value
                because of a typo.

                The function should take one parameter, an xarray.DataArray, and operate
                in place on the variable's values.
        -----------------------------------------------------------------------"""

        for xr_dataset in xr_datasets:
            if xr_dataset is None:
                # If the dataset does not exist, then skip it
                continue

            if not variable_names:
                variable_names = list(xr_dataset.variables.keys())

            for variable_name in variable_names:
                xr_var: xr.DataArray = xr_dataset.get(variable_name)
                if xr_var is not None:
                    units = xr_var.attrs.get('units')
                    if units == old_units:
                        # Change the units attribute
                        xr_var.attrs['units'] = new_units

                        # Convert the values
                        if converter_function:
                            converter_function(xr_var)

    def get_quicklooks_file_name(self,
                                 datastream_name: str,
                                 begin_date: int,
                                 description: str = None,
                                 ext: str = 'png',
                                 mkdirs: bool = False):
        """-----------------------------------------------------------------------
        Create a properly formatted file name where a quicklooks plot should be
        saved for the given processing interval.  For example:

        `${QUICKLOOK_DATA}/ena/enamfrsrcldod1minC1.c1/2021/01/01/enamfrsrcldod1minC1.c1.20210101.000000.lwp.png`

        Args:
            datastream_name(str): The name of the datastream which this plot applies to.
                For example, `mfrsrcldod1min.c1`

            begin_date (int): The begin timestamp of the current processing interval
                as passed to the quicklook hook function

            description (str): The description of the plot to be used in the file name
                For example, in the file `enamfrsrcldod1minC1.c1.20210101.000000.lwp.png`,
                the description is 'lwp'.

            ext (str): The file extension for the image.  Default is 'png'

            mkdirs (bool):  If True, then the folder path to the quicklooks file
                will be automatically created if it does not exist.  Default is False.

        Returns:
            str: The full path to where the quicklooks file should be saved.

        -----------------------------------------------------------------------"""

        # Get timestamp for this plot
        tstamp = dsproc.create_timestamp(begin_date)
        yyyy, mm, dd = tstamp[0:4], tstamp[4:6], tstamp[6:8]

        # Get the quicklooks root directory from an environment parameter
        root_dir = os.environ.get('QUICKLOOK_DATA')
        if root_dir is None:
            raise Exception("QUICKLOOK_DATA environment variable is not defined.")

        # Compute the fully qualified datastream used in file naming conventions
        dsclass, dslevel = datastream_name.split('.')
        fq_datastream = f"{self.site}{dsclass}{self.facility}.{dslevel}"

        # Parent folder for the file (create full path if desired)
        qlook_dir = f"{root_dir}/{self.site}/{fq_datastream}/{yyyy}/{mm}/{dd}"
        if mkdirs:
            os.makedirs(qlook_dir, exist_ok=True)

        if description is not None:
            qlook_filename = f"{qlook_dir}/{fq_datastream}.{tstamp}.{description}.{ext}"
        else:
            qlook_filename = f"{qlook_dir}/{fq_datastream}.{tstamp}.{ext}"

        return qlook_filename

    @staticmethod
    def get_missing_value_mask(*args) -> xr.DataArray:
        """-----------------------------------------------------------------------
        Get True/False mask of same shape as passed variable(s) which is used to
        select data points for which one or more of the values of any of the
        specified variables are missing.

        Args:
            *args (xr.DataArray): Pass one or more xarray variables to check for
                missing values.  All variables in the list must have the same shape.

        Returns:
            xr.DataArray: An array of True/False values of the same shape as the input
            variables where each True represents the case where one or more
            of the variables has a missing_value at that index.
        -----------------------------------------------------------------------"""

        mask = None

        for variable in args:
            missing_values = variable.attrs['missing_value']
            if mask is None:
                mask = variable.isin(missing_values)
            else:
                mask = mask | variable.isin(missing_values)

        return mask

    @staticmethod
    def get_non_missing_value_mask(*args) -> xr.DataArray:
        """-----------------------------------------------------------------------
        Get a True/False mask of same shape as passed variable(s) that is used to
        select data points for which none of the values of any of the specified
        variables are missing.

        Args:
            *args (xr.DataArray): Pass one or more xarray variables to check.  All
                variables in the list must have the same shape.

        Returns:
            xr.DataArray: An array of True/False values of the same shape as the input
            variables where each True represents the case where all variables
            passed in have non-missing value data at that index.

        -----------------------------------------------------------------------"""
        return np.logical_not(Process.get_missing_value_mask(*args))

    @staticmethod
    def get_bad_qc_mask(dataset: xr.Dataset,
                        variable_name: str,
                        include_indeterminate: bool = False,
                        bit_numbers: List[int] = None) -> np.ndarray:
        """-----------------------------------------------------------------------
        Get a mask of same shape as the variable's data which contains True values
        for each data point that has a corresponding bad qc bit set.

        Args:
            dataset (xr.Dataset): The dataset containing the variables

            variable_name (str):  The variable name to check qc for

            include_indeterminate (bool): Whether to include indeterminate bits when
                determining the mask.  By default this is False and only bad bits
                are used to compute the mask.

            bit_numbers (List(int)): The specific bit numbers to include in the qc
                check (i.e., 1,2,3,4, etc.).  Note that if not specified,
                all bits will be used to compute the mask.

        Returns:
            np.ndarray: An array of same shape as the variable consisting of True/False
            values, where each True indicates that the corresponding data
            point had bad (or indeterminate if include_indeterminate is specified)
            qc for the specified bit numbers (all bits if bit_numbers not specified).
        -----------------------------------------------------------------------"""
        xr_var = dataset.get(variable_name)
        if xr_var is None:
            raise Exception(f"Variable {variable_name} does not exist!")

        qc_var = Process.get_qc_variable(dataset, variable_name)
        if qc_var is not None:
            # We are including everything, so any value above 0 is a match
            if include_indeterminate and bit_numbers is None:
                mask = qc_var > 0

            else:
                # Function to get indeterminate bits
                def get_bits(assessments: List[str]):
                    bit_list = []
                    pattern = re.compile('bit_(\d+)_assessment')  # i.e., bit_1_assessment
                    for att_name, att_value in qc_var.attrs.items():
                        match = pattern.match(att_name)
                        if match and att_value in assessments:
                            bit_list.append(int(match.group(1)))
                    return set(bit_list)

                # Get the bit numbers to use in the mask
                bit_numbers = [] if bit_numbers is None else bit_numbers
                assessments = ['Bad']
                if include_indeterminate:
                    assessments.append('Indeterminate')
                user_specified_bits = set(bit_numbers)
                bits = user_specified_bits.union(get_bits(assessments))
                bit_mask = 0
                for bit_number in bits:
                    bit_mask |= (1 << (bit_number - 1))

                mask = (qc_var.data & bit_mask) > 0

        else:
            # There is no qc data, so return all False
            mask = xr.zeros_like(xr_var, dtype=bool)

        return mask

    @staticmethod
    def get_qc_variable(dataset: xr.Dataset, variable_name: str) -> xr.DataArray:
        """-----------------------------------------------------------------------
        Return the companion qc variable for the given data variable.

        Args:
            dataset (xr.Dataset):
            variable_name (str):

        Returns:
            xr.DataArray: The companion qc variable or None if it doesn't exist
        -----------------------------------------------------------------------"""

        qc_var_name = f'qc_{variable_name}'
        qc_var = dataset.get(qc_var_name, None)
        return qc_var

    @staticmethod
    def add_variable(dataset: xr.Dataset,
                     variable_name: str,
                     dim_names: List[str],
                     data: np.ndarray,
                     long_name: str = None,
                     standard_name: str = None,
                     units: str = None,
                     valid_min: Any = None,
                     valid_max: Any = None,
                     missing_value: np.ndarray = None,
                     fill_value: Any = None):
        """-----------------------------------------------------------------------
        Create a new variable in the given xarray dataset with the specified dimensions,
        data, and attributes.

        Important:
            If you want to add the created variable to a given coordinate system, then
            you follow this with a call to assign_coordinate_system_to_variable.
            Similarly, if you want to add the created variable to a given output
            datastream, then you should follow this with a call to
            assign_output_datastream_to_variable

        See Also:
            - assign_coordinate_system_to_variable
            - assign_output_datastream_to_variable

        Args:
            dataset (xr.Dataset):  The xarray dataset to add the new variable to

            variable_name (str):   The name of the variable

            dim_names (List[str]): A list of dimension names for the variable

            data (np.ndarray):     A multidimensional array of the variable's data
                Must have the same shape as the dimensions.

            long_name (str):       The long_name attribute for the variable

            standard_name (str):   The standard_name attribute for the variable

            units (str):           The units attribute for the variable

            valid_min (Any):       The valid_min attribute for the variable.
                Must be the same data type as the variable.

            valid_max (Any):       The valid_max attribute for the variable
                Must be the same data type as the variable.

            missing_value (np.ndarray): An array of possible missing_value attributes
                for the variable.  Must be the same data type as the variable.

            fill_value ():         The fill_value attribute for the variable.
                Must be the same data type as the variable.

        Returns:
            The newly created variable (i.e., xr.DataArray object)
        -----------------------------------------------------------------------"""
        attrs = {}

        def add_if_set(att_name, att_value):
            if att_value is not None:
                attrs[att_name] = att_value

        add_if_set(ADIAtts.LONG_NAME, long_name)
        add_if_set(ADIAtts.STANDARD_NAME, standard_name)
        add_if_set(ADIAtts.UNITS, units)
        add_if_set(ADIAtts.VALID_MIN, valid_min)
        add_if_set(ADIAtts.VALID_MAX, valid_max)
        add_if_set(ADIAtts.MISSING_VALUE, missing_value)
        add_if_set(ADIAtts.FILL_VALUE, fill_value)

        var_as_dict = {
            'dims': dim_names,
            'attrs': attrs,
            'data': data
        }
        xr_var = xr.DataArray.from_dict(var_as_dict)
        dataset[variable_name] = xr_var

        return xr_var

    @staticmethod
    def find_retrieved_variable(retrieved_variable_name) -> Optional[DatastreamIdentifier]:
        """-----------------------------------------------------------------------
        Find the input datastream where the given retrieved variable came
        from.  We may need this if there are complex retrieval rules and the
        given variable may be retrieved from different datastreams depending
        upon the site/facility where this process runs.  We need to get the
        DatastreamIdentifier so we can load the correct xarray dataset if we need
        to modify the data values.

        Args:
            retrieved_variable_name (str): The name of the retrieved variable to
                find

        Returns:
            A DatastreamIdentifier containing all the information needed to look
            up the given dataset or None if the retrieved variable was not found.
        -----------------------------------------------------------------------"""

        adi_var = dsproc.get_retrieved_var(retrieved_variable_name, 0)
        if adi_var is not None:
            dsid = dsproc.get_source_ds_id(adi_var)
            level = dsproc.datastream_class_level(dsid)
            cls = dsproc.datastream_class_name(dsid)
            datastream_name = f"{cls}.{level}"
            site = dsproc.datastream_site(dsid)
            fac = dsproc.datastream_facility(dsid)

            return DatastreamIdentifier(datastream_name=datastream_name, site=site, facility=fac, dsid=dsid)

        return None

    @staticmethod
    def add_qc_variable(dataset: xr.Dataset, variable_name: str):
        """-----------------------------------------------------------------------
        Add a companion qc variable for the given variable

        Args:
            dataset (xr.Dataset):
            variable_name (str):

        Returns:
            The newly created DataArray
        -----------------------------------------------------------------------"""
        data_var: xr.DataArray = dataset.get(variable_name)
        qc_var_name = f'qc_{variable_name}'

        attrs = {
            [ADIAtts.FILL_VALUE]: 0,
            [ADIAtts.LONG_NAME]: f'Quality check results on field: {variable_name}',
            [ADIAtts.UNITS]: 'unitless',
            [
                ADIAtts.DESCRIPTION]: 'This field contains bit packed values which should be interpreted as listed. No bits set (zero) represents good data.'
        }
        dim_names = list(data_var.dims)

        # Create a np.ndarray from the shape using np.full()
        # https://numpy.org/doc/stable/reference/generated/numpy.full.html
        data = np.full(data_var.shape, 0, dtype=np.int32)

        var_as_dict = {
            'dims': dim_names,
            'attrs': attrs,
            'data': data
        }
        xr_var = xr.DataArray.from_dict(var_as_dict)
        dataset[qc_var_name] = xr_var

        # We also have to add the qc variable as an ancillary variable to the data variable
        # TODO: what to do if there are more than one ancillary variables?
        data_var.attrs[ADIAtts.ANCILLARY_VARIABLES] = qc_var_name

        return xr_var

    @staticmethod
    def record_qc_results(xr_dataset: xr.Dataset,
                          variable_name: str,
                          bit_number: int = None,
                          test_results: np.ndarray = None):
        """-----------------------------------------------------------------------
        For the given variable, add bitwise test results to the companion qc
        variable for the given test.

        Args:
            xr_dataset (xr.Dataset):  The xr dataset

            variable_name (str): The name of the data variable (e.g., rh_ambient).

            bit_number (int): The bit/test number to record
                Note that bit numbering starts at 1 (i.e., 1, 2, 3, 4, etc.)

            test_results (np.ndarray): A ndarray mask of the same shape as the
                variable with True/False values for each data point.  True
                means the test failed for that data point.  False means the
                test passed for that data point.

        -----------------------------------------------------------------------"""
        qc_variable = xr_dataset.get(f'qc_{variable_name}')

        if qc_variable is None:
            # TODO: for ADI, we assume the qc variables must exist because they
            # must be defined in the DOD.  If we find a use case where qc
            # variables need to be created, then we can add the capability
            # to create the qc variable if it does not exist.
            raise Exception(f'No qc variable found for {variable_name}')

        bit_index = bit_number - 1
        if bit_index < 0:
            raise Exception("Bit numbering for record_qc_results() should start at 1.")

        # Need to check if the user passed in the test results mask as int instead of bool
        # because if we use ints as a mask, it is flagging the first value, even if it is 0
        if test_results.dtype != np.bool:
            test_results = test_results.astype(np.bool)

        qc_variable[test_results] |= (1 << bit_index)

    @staticmethod
    def get_companion_transform_variable_names(dataset: xr.Dataset, variable_name: str) -> List[str]:
        """-----------------------------------------------------------------------
        For the given variable, get a list of the companion/ancillary variables
        that were added as a result of the ADI transformation.

        Args:
            dataset (xr.Dataset): The dataset
            variable_name (str):  The name of a data variable in the dataset

        Returns:
            A list of string companion variable names that were created
            from the transform engine.  This is used for cleaning up associated
            variables when a variable is deleted from a dataset.
        -----------------------------------------------------------------------"""
        transform_vars = []
        vars_to_check = [
            f"{variable_name}_goodfraction",
            f"{variable_name}_std",
            f"qc_{variable_name}",
            f"source_{variable_name}"
        ]

        # TODO: should probably also check the ancillary_variables attribute and
        # add any additional variables that are referenced

        for var_name in vars_to_check:
            if var_name in dataset:
                transform_vars.append(var_name)

        return transform_vars

    @staticmethod
    def drop_variables(dataset: xr.Dataset, variable_names: List[str]) -> xr.Dataset:
        """-----------------------------------------------------------------------
        This method removes the given variables plus all associated companion
        variables that were added as part of the transformation step (if they
        exist).

        Args:
            dataset (xr.Dataset): The dataset containing the given variables.

            variable_names (List[str]): The variable names to remove.

        Returns:
            xr.Dataset: A new dataset with the given variables and their
            transform companion variables removed.

        -----------------------------------------------------------------------"""

        # This method removes the variable plus all its companion vars
        variables_to_drop = set(variable_names)
        for variable_name in variable_names:
            variables_to_drop.update(Process.get_companion_transform_variable_names(dataset, variable_name))

        return dataset.drop_vars(list(variables_to_drop))

    @staticmethod
    def drop_transform_metadata(dataset: xr.Dataset, variable_names: List[str]) -> xr.Dataset:
        """-----------------------------------------------------------------------
        This method removes all associated companion variables that are generated
        byt the transformation process (if they exist), as well as transformation
        attributes, but it does not remove the original variable.

        Args:
            dataset (xr.Dataset): The dataset containing the transformed variables.

            variable_names (List[str]): The variable names for which to remove
                transformation metadata.

        Returns:
            xr.Dataset: A new dataset with the transform companion variables and
            metadata removed.

        -----------------------------------------------------------------------"""
        # This method removes all associated companion variables, but does not remove the original var
        variables_to_drop = set()
        for variable_name in variable_names:
            # Remove the transform attributes from that variable
            if TransformAttributes.CELL_TRANSFORM in dataset[variable_name].attrs:
                del dataset[variable_name].attrs[TransformAttributes.CELL_TRANSFORM]

            # Also remove any companion variables that were created by the tranform
            variables_to_drop.update(Process.get_companion_transform_variable_names(dataset, variable_name))

        return dataset.drop_vars(list(variables_to_drop))

    @staticmethod
    def assign_coordinate_system_to_variable(variable: xr.DataArray, coordinate_system_name: str):
        """-----------------------------------------------------------------------
        Assign the given variable to the designated ADI coordinate system.

        Args:
            variable (xr.DataArray): A data variable from an xarray dataset

            coordinate_system_name (str): The name of one of the process's
                coordinate systems as specified in the PCM process definition.

        -----------------------------------------------------------------------"""
        variable.attrs[SpecialXrAttributes.COORDINATE_SYSTEM] = coordinate_system_name

    @staticmethod
    def assign_output_datastream_to_variable(variable: xr.DataArray,
                                             output_datastream_name: str,
                                             variable_name_in_datastream: str = None):
        """-----------------------------------------------------------------------
        Assign the given variable to the designated output datastream.

        Args:
            variable (xr.DataArray): A data variable from an xarray dataset

            output_datastream_name (str): An output datastream name as specified in
                PCM process definition

            variable_name_in_datastream (str): The name of the variable as it should
                appear in the output datastream.  If not specified, then the name
                of the given variable will be used.

        -----------------------------------------------------------------------"""
        if variable_name_in_datastream is None:
            variable_name_in_datastream = variable.name
        output_targets = variable.attrs.get(SpecialXrAttributes.OUPUT_TARGETS)
        output_targets = {} if output_targets is None else output_targets
        dsid = get_datastream_id(output_datastream_name)
        output_targets[dsid] = variable_name_in_datastream

        variable.attrs[SpecialXrAttributes.OUPUT_TARGETS] = output_targets

    @staticmethod
    def get_source_var_name(xr_var: xr.DataArray) -> str:
        """-----------------------------------------------------------------------
        For the given variable, get the name of the variable
        used in the input datastream
        Args:
            xr_var (xr.DataArray):

        Returns: str
        -----------------------------------------------------------------------"""
        return xr_var.attrs.get(SpecialXrAttributes.SOURCE_VAR_NAME, None)

    @staticmethod
    def get_source_ds_name(xr_var: xr.DataArray) -> str:
        """-----------------------------------------------------------------------
        For the given variable, get name of the input datastream
        where it came from
        Args:
            xr_var (xr.DataArray):

        Returns: str
        -----------------------------------------------------------------------"""
        return xr_var.attrs.get(SpecialXrAttributes.SOURCE_DS_NAME, None)

    @staticmethod
    def set_datastream_split_mode(output_datastream_name: str, split_mode: SplitMode,
                                  split_start: int, split_interval: int):
        """-----------------------------------------------------------------------
        This method should be called in your init_process_hook if you need to
        change the size of the output file for a given datastream.  For example,
        to create monthly output files.

        Args:
            output_datastream_name (str): The name of the output datastream whose
                file output size will be changed.

            split_mode (SplitMode): One of the options from the SplitMode enum

            split_start (int): Depends on the split_mode selected

            split_interval (int): Depends on the split_mode selected

        -----------------------------------------------------------------------"""
        dsid = get_datastream_id(output_datastream_name)
        dsproc.set_datastream_split_mode(dsid, split_mode.value, split_start, split_interval)

    @staticmethod
    def set_retriever_time_offsets(input_datastream_name: str, begin_offset: int, end_offset: int):
        """-----------------------------------------------------------------------
        This method should be called in your init_process_hook if you need to override
        the offsets per input datastream.  By default, PCM only allows you to set
        global offsets that apply to all datastreams.  If you need to change only
        one datastream, then you can do it via this method.

        Args:
            input_datastream_name (str): The specific input datastream to change the
                processing interval for.

            begin_offset (int):  Seconds of data to fetch BEFORE the process interval
                starts

            end_offset (int):  Seconds of data to fetch AFTER the process interval
                ends

        -----------------------------------------------------------------------"""
        dsid = get_datastream_id(input_datastream_name)
        dsproc.set_retriever_time_offsets(dsid, begin_offset, end_offset)

    @staticmethod
    def shift_processing_interval(seconds: int):
        """-----------------------------------------------------------------------
        This method should be called in your init_process_hook (i.e., before the
        processing loop begins) if you need to shift the processing interval.

        Args:
            seconds (int): Number of seconds to shift

        -----------------------------------------------------------------------"""
        dsproc.set_processing_interval_offset(seconds)

    @staticmethod
    def shift_output_interval(output_datastream_name: str, hours: int):
        """-----------------------------------------------------------------------
        This method should be called in your init_process_hook (i.e., before the
        processing loop begins) if you need to shift the output interval to
        account for the timezone difference at the data location.  For example, if
        you shift the output interval by -6 hours at SGP, the file will be split
        at 6:00 a.m. GMT.

        Args:
            output_datastream_name (str): The name of the output datastream whose
                file output will be shifted.

            hours (int): Number of hours to shift

        -----------------------------------------------------------------------"""
        dsid = get_datastream_id(output_datastream_name)
        dsproc.set_datastream_split_tz_offset(dsid, hours)

    def _internal_init_process_hook(self):
        """-----------------------------------------------------------------------
        This is the underlying hook method passed to ADI, which wraps the user-friendly,
        pythonic method init_process_hook that subclasses can implement.

        -----------------------------------------------------------------------"""
        try:
            ADILogger.debug("**************** Starting init_process_hook ****************\n")

            if self.rollup_qc:
                dsproc.set_trans_qc_rollup_flag(1)

            if self.debug_level == 0:
                warnings.filterwarnings("ignore")

            self.init_process_hook()

            # We don't need to manage a separate user data object, since we have this class instead.
            return 1

        except Exception:
            # Log the stack trace:
            ADILogger.exception("Hook init_process_hook failed.")

            # ADI cython expects None if a fatal error occurred in this hook
            return None

        finally:
            ADILogger.debug("**************** Finished init_process_hook ****************\n")

    def init_process_hook(self):
        """-----------------------------------------------------------------------
        This hook will will be called once just before the main data processing loop begins and before the initial
        database connection is closed.

        -----------------------------------------------------------------------"""
        pass

    def _internal_pre_retrieval_hook(self, user_data, begin_date: int, end_date: int) -> int:
        """-----------------------------------------------------------------------
        This is the underlying hook method passed to ADI, which wraps the user-friendly, pythonic method
        pre_retrieval_hook that subclasses can implement.

        -----------------------------------------------------------------------"""
        pre_retrieval_hook = adi_hook_exception_handler(self.pre_retrieval_hook)
        status = pre_retrieval_hook(begin_date, end_date)
        return status

    def pre_retrieval_hook(self, begin_date: int, end_date: int):
        """-----------------------------------------------------------------------
        This hook will will be called once per processing interval just prior to data retrieval.

        Args:
          - int begin_date: the begin time of the current processing interval
          - int end_date:   the end time of the current processing interval

        -----------------------------------------------------------------------"""
        pass

    def _internal_post_retrieval_hook(self, user_data, begin_date: int, end_date: int, ret_data) -> int:
        """-----------------------------------------------------------------------
        This is the underlying hook method passed to ADI, which wraps the user-friendly, pythonic method
        post_retrieval_hook that subclasses can implement.

        -----------------------------------------------------------------------"""
        def post_hook_func():
            if self.debug_level > 1 and self.include_debug_dumps:
                dsproc.dump_retrieved_datasets("./debug_dumps", "post_retrieval_end.debug", 0)

        post_retrieval_hook = adi_hook_exception_handler(self.post_retrieval_hook, post_hook_func=post_hook_func)
        status = post_retrieval_hook(begin_date, end_date)

        return status

    def post_retrieval_hook(self, begin_date: int, end_date: int):
        """-----------------------------------------------------------------------
        This hook will will be called once per processing interval just after data retrieval,
        but before the retrieved observations are merged and QC is applied.

        Args:
            begin_date (int): the begin time of the current processing interval
            end_date (int):   the end time of the current processing interval

        -----------------------------------------------------------------------"""
        pass

    def _internal_pre_transform_hook(self, user_data, begin_date: int, end_date: int, ret_data) -> int:
        """-----------------------------------------------------------------------
        This is the underlying hook method passed to ADI, which wraps the user-friendly, pythonic method
        pre_transform_hook that subclasses can implement.

        -----------------------------------------------------------------------"""
        def pre_hook_func():
            if self.debug_level > 1 and self.include_debug_dumps:
                dsproc.dump_retrieved_datasets("./debug_dumps", "pre_transform_start.debug", 0)

        def post_hook_func():
            if self.debug_level > 1 and self.include_debug_dumps:
                dsproc.dump_retrieved_datasets("./debug_dumps", "pre_transform_end.debug", 0)

        pre_transform_hook = adi_hook_exception_handler(self.pre_transform_hook,
                                                        pre_hook_func=pre_hook_func,
                                                        post_hook_func=post_hook_func)
        status = pre_transform_hook(begin_date, end_date)
        return status

    def pre_transform_hook(self, begin_date: int, end_date: int):
        """-----------------------------------------------------------------------
        This hook will be called once per processing interval just prior to data
        transformation,and after the retrieved observations are merged and QC is
        applied.

        Args:
            begin_date (int): the begin time of the current processing interval
            end_date (int): the end time of the current processing interval
        -----------------------------------------------------------------------"""
        pass

    def _internal_post_transform_hook(self, user_data, begin_date: int, end_date: int, trans_data) -> int:
        """-----------------------------------------------------------------------
        This is the underlying hook method passed to ADI, which wraps the user-friendly, pythonic method
        post_transform_hook that subclasses can implement.

        -----------------------------------------------------------------------"""

        def pre_hook_func():
            if self.debug_level > 1 and self.include_debug_dumps:
                dsproc.dump_transformed_datasets("./debug_dumps", "post_transform_start.debug", 0)

        def post_hook_func():
            if self.debug_level > 1 and self.include_debug_dumps:
                dsproc.dump_transformed_datasets("./debug_dumps", "post_transform_end.debug", 0)

        post_transform_hook = adi_hook_exception_handler(self.post_transform_hook,
                                                         pre_hook_func=pre_hook_func,
                                                         post_hook_func=post_hook_func)
        status = post_transform_hook(begin_date, end_date)
        return status

    def post_transform_hook(self, begin_date: int, end_date: int):
        """-----------------------------------------------------------------------
        This hook will be called once per processing interval just after data
        transformation, but before the output datasets are created.

        Args:
            begin_date (int): the begin time of the current processing interval
            end_date (int):   the end time of the current processing interval
        -----------------------------------------------------------------------"""
        pass

    def _internal_process_data_hook(self, user_data, begin_date: int, end_date: int, trans_or_ret_data) -> int:
        """-----------------------------------------------------------------------
        This is the underlying hook method passed to ADI, which wraps the user-friendly, pythonic method
        process_data_hook that subclasses can implement.

        -----------------------------------------------------------------------"""
        begin_time = strftime('%Y%m%d.%H%M%S', gmtime(begin_date))
        end_time = strftime('%Y%m%d.%H%M%S', gmtime(end_date))
        ADILogger.debug("begin_date = " + begin_time)
        ADILogger.debug("end_date = " + end_time)

        def pre_hook_func():
            if self.debug_level > 1 and self.include_debug_dumps:
                dsproc.dump_output_datasets("./debug_dumps", "process_data_start.debug", 0)

        def post_hook_func():
            if self.debug_level > 1 and self.include_debug_dumps:
                dsproc.dump_output_datasets("./debug_dumps", "process_data_end.debug", 0)

        process_data_hook = adi_hook_exception_handler(self.process_data_hook,
                                                       pre_hook_func=pre_hook_func,
                                                       post_hook_func=post_hook_func)
        status = process_data_hook(begin_date, end_date)
        return status

    def process_data_hook(self, begin_date: int, end_date: int):
        """-----------------------------------------------------------------------
        This hook will be called once per processing interval just after the output
        datasets are created, but before they are stored to disk.

        Args:
            begin_date (int): the begin time of the current processing interval
            end_date (int):   the end time of the current processing interval
        -----------------------------------------------------------------------"""
        pass

    def _internal_finish_process_hook(self, user_data):
        """-----------------------------------------------------------------------
        This is the underlying hook method passed to ADI, which wraps the user-friendly, pythonic method
        finish_process_hook that subclasses can implement.

        -----------------------------------------------------------------------"""
        finish_process_hook = adi_hook_exception_handler(self.finish_process_hook)
        finish_process_hook()

    def finish_process_hook(self):
        """-----------------------------------------------------------------------
        This hook will be called once just after the main data processing loop finishes.  This function should be used
        to clean up any temporary files used.

        -----------------------------------------------------------------------"""
        pass

    def _internal_quicklook_hook(self, user_data, begin_date: int, end_date: int) -> int:
        """-----------------------------------------------------------------------
        This is the underlying hook method passed to ADI, which wraps the user-friendly, pythonic method
        quicklook_hook that subclasses can implement.

        -----------------------------------------------------------------------"""
        quicklook_hook = adi_hook_exception_handler(self.quicklook_hook)
        return quicklook_hook(begin_date, end_date)

    def quicklook_hook(self, begin_date: int, end_date: int):
        """-----------------------------------------------------------------------
        This hook will be called once per processing interval just after all data
        is stored.

        Args:
            begin_date (int): the begin timestamp of the current processing interval
            end_date (int): the end timestamp of the current processing interval
        -----------------------------------------------------------------------"""
        pass

    def run(self) -> int:
        """-----------------------------------------------------------------------
        Run the process.

        Returns:
            int: The processing status:

            - 1 if an error occurred
            - 0 if successful
        -----------------------------------------------------------------------"""

        dsproc.use_nc_extension()

        # TODO: make sure that ADI doesn't care if we set a hook if it's not
        # used for the given processing model
        dsproc.set_init_process_hook(self._internal_init_process_hook)
        dsproc.set_pre_retrieval_hook(self._internal_pre_retrieval_hook)
        dsproc.set_post_retrieval_hook(self._internal_post_retrieval_hook)
        dsproc.set_pre_transform_hook(self._internal_pre_transform_hook)
        dsproc.set_post_transform_hook(self._internal_post_transform_hook)
        dsproc.set_process_data_hook(self._internal_process_data_hook)
        dsproc.set_finish_process_hook(self._internal_finish_process_hook)
        dsproc.set_quicklook_hook(self._internal_quicklook_hook)

        exit_value = dsproc.main(
            sys.argv,
            self.process_model,
            self.process_version,
            self.process_names)

        return exit_value
