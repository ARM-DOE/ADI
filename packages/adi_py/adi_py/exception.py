"""-----------------------------------------------------------------------
This module defines ADI exceptions which may be thrown from process hooks.
-----------------------------------------------------------------------"""


class SkipProcessingIntervalException(Exception):
    """-----------------------------------------------------------------------
    Processes should throw this exception if the current processing interval
    should be skipped.  All other exceptions will be considered to fail the
    process.
    -----------------------------------------------------------------------"""
    pass




