"""-----------------------------------------------------------------------
This module defines ADI exceptions which may be thrown from process hooks.
-----------------------------------------------------------------------"""
from adi_py.logger import LogLevel


class SkipProcessingIntervalException(Exception):
    """-----------------------------------------------------------------------
    Processes should throw this exception if the current processing interval
    should be skipped.  All other exceptions will be considered to fail the
    process.
    -----------------------------------------------------------------------"""
    def __init__(self, msg: str = '', log_level: LogLevel = LogLevel.INFO):
        self.msg = f'SKIP PROCESSING INTERVAL: {msg}'
        super().__init__(self.msg)
        self.log_level: LogLevel = log_level



class DatasetConversionException(Exception):
    """-----------------------------------------------------------------------
    Exception used when converting from XArray to ADI or vice versa and the
    data are incompatible.
    -----------------------------------------------------------------------"""
    pass


