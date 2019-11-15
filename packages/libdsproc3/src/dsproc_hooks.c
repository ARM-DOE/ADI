/*******************************************************************************
*
*  COPYRIGHT (C) 2012 Battelle Memorial Institute.  All Rights Reserved.
*
********************************************************************************
*
*  Author:
*     name:  Brian Ermold
*     phone: (509) 375-2277
*     email: brian.ermold@pnl.gov
*
********************************************************************************
*
*  REPOSITORY INFORMATION:
*    $Revision: 80275 $
*    $Author: ermold $
*    $Date: 2017-08-28 18:38:53 +0000 (Mon, 28 Aug 2017) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsproc_hooks.c
 *  DSProc Hook Functions.
 */

#include "dsproc3.h"

/** @privatesection */

/*******************************************************************************
 *  Static Data and Functions Visible Only To This Module
 */

static int   _HasQuicklookFunction = 0;
static void *_user_data;

static void *(*_init_process_hook)(void);
static void  (*_finish_process_hook)(void *user_data);

static int (*_process_data_hook)(
    void     *user_data,
    time_t    begin_date,
    time_t    end_date,
    CDSGroup *input_data);

static int (*_pre_retrieval_hook)(
    void     *user_data,
    time_t    begin_date,
    time_t    end_date);

static int (*_post_retrieval_hook)(
    void     *user_data,
    time_t    begin_date,
    time_t    end_date,
    CDSGroup *ret_data);

static int (*_pre_transform_hook)(
    void     *user_data,
    time_t    begin_date,
    time_t    end_date,
    CDSGroup *ret_data);

static int (*_post_transform_hook)(
    void     *user_data,
    time_t    begin_date,
    time_t    end_date,
    CDSGroup *trans_data);

static int (*_process_file_hook)(
    void       *user_data,
    const char *input_dir,
    const char *file_name);

static int (*_quicklook_hook)(
    void     *user_data,
    time_t    begin_date,
    time_t    end_date);

static int (*_custom_qc_hook)(
        void       *user_data,
        int         ds_id,
        CDSGroup   *dataset);

/*******************************************************************************
 *  Private Data Functions Visible Only To This Library
 */

/** Flag used to indicate we are inside the user's finish_process hook */
int _InsideFinishProcessHook = 0;

/**
 *  Private: Run the init_process_hook function.
 *
 *  @return
 *    -  1 if successful
 *    -  0 if an error occurred.
 */
int _dsproc_run_init_process_hook(void)
{
    const char *status_text;

    if (_init_process_hook) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "\n----- ENTERING INIT PROCESS HOOK -------\n");

        _user_data = _init_process_hook();

        if (!_user_data) {

            status_text = dsproc_get_status();
            if (status_text[0] == '\0') {

                ERROR( DSPROC_LIB_NAME,
                    "Error message not set by init_process_hook function\n");

                dsproc_set_status("Unknown Data Processing Error (check logs)");
            }

            return(0);
        }

        DEBUG_LV1( DSPROC_LIB_NAME,
            "----- EXITING INIT PROCESS HOOK --------\n\n");
    }

    return(1);
}

/**
 *  Private: Run the finish_process_hook function.
 */
void _dsproc_run_finish_process_hook()
{
    if (_finish_process_hook) {

        _InsideFinishProcessHook = 1;

        DEBUG_LV1( DSPROC_LIB_NAME,
            "\n----- ENTERING FINISH PROCESS HOOK -----\n");

        _finish_process_hook(_user_data);

        DEBUG_LV1( DSPROC_LIB_NAME,
            "----- EXITING FINISH PROCESS HOOK ------\n\n");

        _InsideFinishProcessHook = 0;
    }
}

/**
 *  Private: Run the process_data_hook function.
 *
 *  @param begin_date - the begin time of the current processing interval
 *  @param end_date   - the end time of the current processing interval
 *  @param input_data - pointer to the parent CDSGroup containing the input data
 *
 *  @return
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current processing interval
 *         and continue on to the next one.
 *    - -1 if a fatal error occurred and the process should exit.
 */
int _dsproc_run_process_data_hook(
    time_t    begin_date,
    time_t    end_date,
    CDSGroup *input_data)
{
    const char *status_text;
    int         status;

    status = 1;

    if (_process_data_hook) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "\n----- ENTERING PROCESS DATA HOOK -------\n");

        status = _process_data_hook(
            _user_data, begin_date, end_date, input_data);

        if (status < 0) {

            status_text = dsproc_get_status();
            if (status_text[0] == '\0') {

                ERROR( DSPROC_LIB_NAME,
                    "Error message not set by process_data_hook function\n");

                dsproc_set_status("Unknown Data Processing Error (check logs)");
            }
        }

        DEBUG_LV1( DSPROC_LIB_NAME,
            "----- EXITING PROCESS DATA HOOK --------\n\n");
    }

    return(status);
}

/**
 *  Private: Run the pre_retrieval_hook function.
 *
 *  @param begin_date - the begin time of the current processing interval
 *  @param end_date   - the end time of the current processing interval
 *
 *  @return
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current processing interval
 *         and continue on to the next one.
 *    - -1 if a fatal error occurred and the process should exit.
 */
int _dsproc_run_pre_retrieval_hook(
    time_t   begin_date,
    time_t   end_date)
{
    const char *status_text;
    int         status;

    status = 1;

    if (_pre_retrieval_hook) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "\n----- ENTERING PRE-RETRIEVAL HOOK ------\n");

        status = _pre_retrieval_hook(_user_data, begin_date, end_date);

        if (status < 0) {

            status_text = dsproc_get_status();
            if (status_text[0] == '\0') {

                ERROR( DSPROC_LIB_NAME,
                    "Error message not set by pre_retrieval_hook function\n");

                dsproc_set_status("Unknown Data Processing Error (check logs)");
            }
        }

        DEBUG_LV1( DSPROC_LIB_NAME,
            "----- EXITING PRE-RETRIEVAL HOOK -------\n\n");
    }

    return(status);
}

/**
 *  Private: Run the post_retrieval_hook function.
 *
 *  @param begin_date - the begin time of the current processing interval
 *  @param end_date   - the end time of the current processing interval
 *  @param ret_data   - pointer to the parent CDSGroup containing the retrieved data
 *
 *  @return
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current processing interval
 *         and continue on to the next one.
 *    - -1 if a fatal error occurred and the process should exit.
 */
int _dsproc_run_post_retrieval_hook(
    time_t    begin_date,
    time_t    end_date,
    CDSGroup *ret_data)
{
    const char *status_text;
    int         status;

    status = 1;

    if (_post_retrieval_hook) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "\n----- ENTERING POST-RETRIEVAL HOOK -----\n");

        status = _post_retrieval_hook(
            _user_data, begin_date, end_date, ret_data);

        if (status < 0) {

            status_text = dsproc_get_status();
            if (status_text[0] == '\0') {

                ERROR( DSPROC_LIB_NAME,
                    "Error message not set by post_retrieval_hook function\n");

                dsproc_set_status("Unknown Data Processing Error (check logs)");
            }
        }

        DEBUG_LV1( DSPROC_LIB_NAME,
            "----- EXITING POST-RETRIEVAL HOOK ------\n\n");
    }

    return(status);
}

/**
 *  Private: Run the pre_transform_hook function.
 *
 *  @param begin_date - the begin time of the current processing interval
 *  @param end_date   - the end time of the current processing interval
 *  @param ret_data   - pointer to the parent CDSGroup containing the retrieved data
 *
 *  @return
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current processing interval
 *         and continue on to the next one.
 *    - -1 if a fatal error occurred and the process should exit.
 */
int _dsproc_run_pre_transform_hook(
    time_t    begin_date,
    time_t    end_date,
    CDSGroup *ret_data)
{
    const char *status_text;
    int         status;

    status = 1;

    if (_pre_transform_hook) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "\n----- ENTERING PRE-TRANSFORM HOOK ------\n");

        status = _pre_transform_hook(
            _user_data, begin_date, end_date, ret_data);

        if (status < 0) {

            status_text = dsproc_get_status();
            if (status_text[0] == '\0') {

                ERROR( DSPROC_LIB_NAME,
                    "Error message not set by pre_transform_hook function\n");

                dsproc_set_status("Unknown Data Processing Error (check logs)");
            }
        }

        DEBUG_LV1( DSPROC_LIB_NAME,
            "----- EXITING PRE-TRANSFORM HOOK -------\n\n");
    }

    return(status);
}

/**
 *  Private: Run the post_transform_hook function.
 *
 *  @param begin_date - the begin time of the current processing interval
 *  @param end_date   - the end time of the current processing interval
 *  @param trans_data - pointer to the parent CDSGroup containing the transformed data
 *
 *  @return
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current processing interval
 *         and continue on to the next one.
 *    - -1 if a fatal error occurred and the process should exit.
 */
int _dsproc_run_post_transform_hook(
    time_t    begin_date,
    time_t    end_date,
    CDSGroup *trans_data)
{
    const char *status_text;
    int         status;

    status = 1;

    if (_post_transform_hook) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "\n----- ENTERING POST-TRANSFORM HOOK -----\n");

        status = _post_transform_hook(
            _user_data, begin_date, end_date, trans_data);

        if (status < 0) {

            status_text = dsproc_get_status();
            if (status_text[0] == '\0') {

                ERROR( DSPROC_LIB_NAME,
                    "Error message not set by post_transform_hook function\n");

                dsproc_set_status("Unknown Data Processing Error (check logs)");
            }
        }

        DEBUG_LV1( DSPROC_LIB_NAME,
            "----- EXITING POST-TRANSFORM HOOK ------\n\n");
    }

    return(status);
}

/**
 *  Private: Run the quicklook_hook function.
 *
 *  @param begin_date - the begin time of the current processing interval
 *  @param end_date   - the end time of the current processing interval
 *
 *  @return
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current processing interval
 *         and continue on to the next one.
 *    - -1 if a fatal error occurred and the process should exit.
 */
int _dsproc_run_quicklook_hook(
    time_t    begin_date,
    time_t    end_date)
{
    const char *status_text;
    int         status;

    status = 1;

    if (_quicklook_hook) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "\n----- ENTERING QUICKLOOK HOOK -----\n");

        status = _quicklook_hook(
            _user_data, begin_date, end_date);

        if (status < 0) {

            status_text = dsproc_get_status();
            if (status_text[0] == '\0') {

                ERROR( DSPROC_LIB_NAME,
                    "Error message not set by quicklook_hook function\n");

                dsproc_set_status("Unknown Data Processing Error (check logs)");
            }
        }

        DEBUG_LV1( DSPROC_LIB_NAME,
            "----- EXITING QUICKLOOK HOOK ------\n\n");
    }

    return(status);
}

/**
 *  Private: Run the process_file_hook function.
 *
 *  @param input_dir - full path to the input directory
 *  @param file_name - name of the file to process
 *
 *  @return
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current file
 *         and continue on to the next one.
 *    - -1 if a fatal error occurred and the process should exit.
 */
int _dsproc_run_process_file_hook(
    const char *input_dir,
    const char *file_name)
{
    int         force_mode = dsproc_get_force_mode();
    const char *status_text;
    int         status;

    status = 1;

    if (_process_file_hook) {

        if (force_mode) {
            dsproc_set_status(NULL);
        }

        DEBUG_LV1( DSPROC_LIB_NAME,
            "\n----- ENTERING PROCESS FILE HOOK -------\n");

        status = _process_file_hook(
            _user_data, input_dir, file_name);

        if (status < 0) {

            status_text = dsproc_get_status();
            if (status_text[0] == '\0') {

                ERROR( DSPROC_LIB_NAME,
                    "Error message not set by process_file_hook function\n");

                dsproc_set_status("Unknown Data Processing Error (check logs)");
            }

            /* If the force option is enabled we need to try to move
             * the file and continue processing. */

            if (force_mode && !dsproc_is_fatal(errno)) {

                LOG( DSPROC_LIB_NAME,
                    "FORCE: Forcing ingest to continue\n");

                if (dsproc_force_rename_bad(input_dir, file_name)) {
                    status = 0;
                }
            }
        }

        DEBUG_LV1( DSPROC_LIB_NAME,
            "----- EXITING PROCESS FILE HOOK --------\n\n");
    }

    return(status);
}

/**
 *  Private: Run the custom_qc_hook function.
 *
 *  @param  ds_id:     datastream ID
 *  @param  dataset:   pointer to the dataset
 *
 *  @return
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current dataset and continue
 *    - -1 if a fatal error occurred and the process should exit
 */
int _dsproc_custom_qc_hook(
    int       ds_id,
    CDSGroup *dataset)
{
    const char *status_text;
    int         status;

    status = 1;

    if (_custom_qc_hook) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "\n----- ENTERING CUSTOM QC HOOK ----------\n");

        status = _custom_qc_hook(
            _user_data, ds_id, dataset);

        if (status < 0) {

            status_text = dsproc_get_status();
            if (status_text[0] == '\0') {

                ERROR( DSPROC_LIB_NAME,
                    "Error message not set by custom_qc_hook function\n");

                dsproc_set_status("Unknown Data Processing Error (check logs)");
            }
        }

        DEBUG_LV1( DSPROC_LIB_NAME,
            "----- EXITING CUSTOM QC HOOK -----------\n\n");
    }

    return(status);
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Check if a quicklook function has been set.
 *
 *  @return
 *    -  1 if a quicklook function has been set
 *    -  0 if a quicklook function has not been set
 */
int dsproc_has_quicklook_function(void)
{
    return(_HasQuicklookFunction);
}

/**
 *  VAP/Ingest: Set hook function to call when a process is first initialized.
 *
 *  This function must be called from the main function before dsproc_main()
 *  is called.
 *
 *  The specified init_process_hook() function will be called once just before
 *  the main data processing loop begins and before the initial database
 *  connection is closed.
 *
 *  The init_process_hook() function does not take any arguments, but it must
 *  return:
 *
 *    - void pointer to a user defined data structure or value that will be
 *      passed in as user_data to all other hook functions.
 *    - (void *)1 if no user data is returned.
 *    - NULL if a fatal error occurred and the process should exit.
 *
 *  @param init_process_hook - function to call when the process is initialized
 */
void dsproc_set_init_process_hook(
    void *(*init_process_hook)(void))
{
    _init_process_hook = init_process_hook;
}

/**
 *  VAP/Ingest: Set hook function to call before a process finishes.
 *
 *  This function must be called from the main function before dsproc_main()
 *  is called, or from the init_process_hook() function.
 *
 *  The specified finish_process_hook() function will be called once just after
 *  the main data processing loop finishes.  This function should be used to
 *  clean up any temporary files used, and to free any memory used by the
 *  structure returned by the init_process_hook() function.
 *
 *  The finish_process_hook function must take the following argument:
 *
 *    - void *user_data:  value returned by the init_process_hook() function
 *
 *  @param finish_process_hook - function to call before the process finishes
 */
void dsproc_set_finish_process_hook(
    void (*finish_process_hook)(void *user_data))
{
    _finish_process_hook = finish_process_hook;
}

/**
 *  VAP: Set the main data processing function.
 *
 *  This function must be called from the main function before dsproc_main()
 *  is called, or from the init_process_hook() function.
 *
 *  The specified process_data_hook function will be called once per
 *  processing interval just after the output datasets are created, but
 *  before they are stored to disk.
 *
 *  The process_data_hook function must take the following arguments:
 *
 *    - void     *user_data:  value returned by the init_process_hook() function
 *    - time_t    begin_date: the begin time of the current processing interval
 *    - time_t    end_date:   the end time of the current processing interval
 *    - CDSGroup *input_data: pointer to the parent CDSGroup containing the input data.
 *
 *  And must return:
 *
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current processing interval
 *         and continue on to the next one.
 *    - -1 if a fatal error occurred and the process should exit.
 *
 *  @param process_data_hook - the main data processing function
 */
void dsproc_set_process_data_hook(
    int (*process_data_hook)(
        void     *user_data,
        time_t    begin_date,
        time_t    end_date,
        CDSGroup *input_data))
{
    _process_data_hook = process_data_hook;
}

/**
 *  VAP: Set hook function to call before data is retrieved.
 *
 *  This function must be called from the main function before dsproc_main()
 *  is called, or from the init_process_hook() function.
 *
 *  The specified pre_retrieval_hook function will be called once per
 *  processing interval just prior to data retrieval, and must take the
 *  following arguments:
 *
 *  The pre_retrieval_hook function must take the following arguments:
 *
 *    - void   *user_data:  value returned by the init_process_hook() function
 *    - time_t  begin_date: the begin time of the current processing interval
 *    - time_t  end_date:   the end time of the current processing interval
 *
 *  And must return:
 *
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current processing interval
 *         and continue on to the next one.
 *    - -1 if a fatal error occurred and the process should exit.
 *
 *  @param pre_retrieval_hook - function to call before data is retrieved
 */
void dsproc_set_pre_retrieval_hook(
    int (*pre_retrieval_hook)(
        void     *user_data,
        time_t    begin_date,
        time_t    end_date))
{
    _pre_retrieval_hook = pre_retrieval_hook;
}

/**
 *  VAP: Set hook function to call after data is retrieved.
 *
 *  This function must be called from the main function before dsproc_main()
 *  is called, or from the init_process_hook() function.
 *
 *  The specified post_retrieval_hook function will be called once per
 *  processing interval just after data retrieval, but before the retrieved
 *  observations are merged and QC is applied.
 *
 *  The post_retrieval_hook function must take the following arguments:
 *
 *    - void     *user_data:  value returned by the init_process_hook() function
 *    - time_t    begin_date: the begin time of the current processing interval
 *    - time_t    end_date:   the end time of the current processing interval
 *    - CDSGroup *ret_data:   pointer to the parent CDSGroup containing the retrieved data
 *
 *  And must return:
 *
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current processing interval
 *         and continue on to the next one.
 *    - -1 if a fatal error occurred and the process should exit.
 *
 *  @param post_retrieval_hook - function to call after data is retrieved
 */
void dsproc_set_post_retrieval_hook(
    int (*post_retrieval_hook)(
        void     *user_data,
        time_t    begin_date,
        time_t    end_date,
        CDSGroup *ret_data))
{
    _post_retrieval_hook = post_retrieval_hook;
}

/**
 *  VAP: Set hook function to call before the data is transformed.
 *
 *  This function must be called from the main function before dsproc_main()
 *  is called, or from the init_process_hook() function.
 *
 *  The specified pre_transform_hook function will be called once per
 *  processing interval just prior to data transformation, and after
 *  the retrieved observations are merged and QC is applied.
 *
 *  The pre_transform_hook function must take the following arguments:
 *
 *    - void     *user_data:  value returned by the init_process_hook() function
 *    - time_t    begin_date: the begin time of the current processing interval
 *    - time_t    end_date:   the end time of the current processing interval
 *    - CDSGroup *ret_data:   pointer to the parent CDSGroup containing the retrieved data
 *
 *  And must return:
 *
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current processing interval
 *         and continue on to the next one.
 *    - -1 if a fatal error occurred and the process should exit.
 *
 *  @param pre_transform_hook - function to call before the data is transformed
 */
void dsproc_set_pre_transform_hook(
    int (*pre_transform_hook)(
        void     *user_data,
        time_t    begin_date,
        time_t    end_date,
        CDSGroup *ret_data))
{
    _pre_transform_hook = pre_transform_hook;
}

/**
 *  VAP: Set hook function to call after the data is transformed.
 *
 *  This function must be called from the main function before dsproc_main()
 *  is called, or from the init_process_hook() function.
 *
 *  The specified post_transform_hook function will be called once per
 *  processing interval just after data transformation, but before the
 *  output datasets are created.
 *
 *  The post_transform_hook function must take the following arguments:
 *
 *    - void     *user_data:  value returned by the init_process_hook() function
 *    - time_t    begin_date: the begin time of the current processing interval
 *    - time_t    end_date:   the end time of the current processing interval
 *    - CDSGroup *trans_data: pointer to the parent CDSGroup containing the transformed data
 *
 *  And must return:
 *
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current processing interval
 *         and continue on to the next one.
 *    - -1 if a fatal error occurred and the process should exit.
 *
 *  @param post_transform_hook - function to call after the data is transformed
 */
void dsproc_set_post_transform_hook(
    int (*post_transform_hook)(
        void     *user_data,
        time_t    begin_date,
        time_t    end_date,
        CDSGroup *trans_data))
{
    _post_transform_hook = post_transform_hook;
}

/**
 *  VAP or Ingest: Set hook function to call after all data is stored.
 *
 *  This function must be called from the main function before dsproc_main()
 *  is called, or from the init_process_hook() function.
 *
 *  The specified quicklook_hook function will be called once per
 *  processing interval just after all data is stored.
 *
 *  The quicklook_hook function must take the following arguments:
 *
 *    - void     *user_data:  value returned by the init_process_hook() function
 *    - time_t    begin_date: the begin time of the current processing interval
 *    - time_t    end_date:   the end time of the current processing interval
 *
 *  And must return:
 *
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current processing interval
 *         and continue on to the next one.
 *    - -1 if a fatal error occurred and the process should exit.
 *
 *  @param quicklook_hook - function to call after all data is stored
 */
void dsproc_set_quicklook_hook(
    int (*quicklook_hook)(
        void     *user_data,
        time_t    begin_date,
        time_t    end_date))
{
    _quicklook_hook = quicklook_hook;

    if (_quicklook_hook) {
        _HasQuicklookFunction = 1;
    }
    else {
        _HasQuicklookFunction = 0;
    }
}

/**
 *  Ingest: Set the main file processing function.
 *
 *  This function must be called from the main function before dsproc_main()
 *  is called, or from the init_process_hook() function.
 *
 *  The specified process_file_hook function will be called once for every
 *  file found in the input directory, and it must take the following arguments:
 *
 *    - void       *user_data: value returned by the init_process_hook() function
 *    - const char *input_dir: full path to the input directory
 *    - const char *file_name: name of the file to process
 *
 *  And must return:
 *
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current file
 *         and continue on to the next one.
 *    - -1 if a fatal error occurred and the process should exit.
 *
 *  @param process_file_hook - the main file processing function
 */
void dsproc_set_process_file_hook(
    int (*process_file_hook)(
        void       *user_data,
        const char *input_dir,
        const char *file_name))
{
    _process_file_hook = process_file_hook;
}

/**
 *  Ingest: Set the custom QC function.
 *
 *  This function must be called from the main function before dsproc_main()
 *  is called, or from the init_process_hook() function.
 *
 *  The specified custom_qc_hook function will be called just after the
 *  standard QC checks are applied when the data is stored, and it must take
 *  the following arguments:
 *
 *    - void     *user_data: value returned by the init_process_hook() function
 *
 *    - int       ds_id:     datastream ID
 *
 *    - CDSGroup *dataset:   pointer to the dataset
 *
 *  And must return:
 *
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current dataset and continue
 *    - -1 if a fatal error occurred and the process should exit
 *
 *  @param custom_qc_hook - function used to apply custom QC checks
 */
void dsproc_set_custom_qc_hook(
    int (*custom_qc_hook)(
        void       *user_data,
        int         ds_id,
        CDSGroup   *dataset))
{
    _custom_qc_hook = custom_qc_hook;
}
