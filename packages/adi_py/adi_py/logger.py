
"""-----------------------------------------------------------------------
This module contains the **ADILogger** class which provides a python-like
logging API facade around the dsproc logging methods.
-----------------------------------------------------------------------"""
import io
import sys
import traceback

import dsproc3 as dsproc


class ADILogger:
    """-----------------------------------------------------------------------
    This class provides python-like logging API facade around the dsproc
    logging methods.
    -----------------------------------------------------------------------"""
    @staticmethod
    def info(message):
        dsproc.log(message)

    @staticmethod
    def warning(message):
        dsproc.warning(message)

    @staticmethod
    def error(message):
        dsproc.error(message)

    @staticmethod
    def exception(message):
        """-----------------------------------------------------------------------
        Use this method to log the stack trace of any raised exception to the process's
        ADI log file.

        Args:
            - message: str
                An optional additional message to log, in addition to the stack trace.
        -----------------------------------------------------------------------"""
        with io.StringIO() as buffer:
            exception_type, exception_value, exception_traceback = sys.exc_info()
            traceback.print_tb(exception_traceback, file=buffer)

            exception_details = (f"Error Message:      {message}\n"
                                 f"Error Type:         {exception_type.__name__}\n"
                                 f"Exception Message:  {str(exception_value)}\n\n"
                                 f"Stack Trace:\n"
                                 f"{buffer.getvalue().strip()}"
                                 )

        dsproc.error(message, exception_details)
        pass

    @staticmethod
    def debug(message, debug_level=1):
        if debug_level == 1:
            dsproc.debug_lv1(message)
        elif debug_level == 2:
            dsproc.debug_lv2(message)
        elif debug_level == 3:
            dsproc.debug_lv3(message)
        else:
            dsproc.debug_lv4(message)
