/*****************************************************************************
* Copyright (C) 2014, Battelle Memorial Institute
* All rights reserved.
*
* Author:
*    name:  Brian Ermold
*    email: brian.ermold@pnnl.gov
*
******************************************************************************/

/** @file csv_ingestor_read_data.c
 *  CSV Ingestor Read Data.
 */

#include "csv_ingestor.h"

/**
 *  Read in the data from a CSV data file.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param   data   pointer to the UserData structure
 *  @param   ds     pointer to the DsData structure
 *
 *  @retval  nrecs  number of records read in
 *  @retval  -1     if a fatal error occurred
 */
int csv_ingestor_read_data(UserData *data, DsData *ds)
{
    CSVConf    *conf      = ds->conf;
    CSVParser  *csv       = ds->csv;
    char        delim     = conf->delim;
    char        exp_ncols = conf->exp_ncols;
    const char *delims    = ",\t";

    char       *linep;
    int         ndelims;

    int         nfields;
    int         di, li;
    int         status;

    int         header_linenum;

    /************************************************************
    *  Load the data file into the CSVParser
    *************************************************************/

    status = dsproc_load_csv_file(csv, data->input_dir, data->file_name);
    if (status <= 0) return(status);

    /************************************************************
    *  Find the header line number
    *************************************************************/

    header_linenum = 0;

    if (conf->header_linenum) {

        header_linenum = conf->header_linenum;

        if (header_linenum > csv->nlines) {

            DSPROC_ERROR( NULL,
                "Invalid file format: %s\n"
                " -> line number of header '%d' is > number of lines in file '%d'\n",
                data->file_name, header_linenum, csv->nlines);

            return(-1);
        }
    }
    else if (conf->header_tag) {

        while ((linep = dsproc_get_next_csv_line(csv))) {

            if (strstr(linep, conf->header_tag)) {
                header_linenum = csv->linenum;
                break;
            }
        }

        if (!header_linenum) {

            DSPROC_ERROR( NULL,
                "Invalid file format: %s\n"
                " -> could not find header line containing: '%s'\n",
                data->file_name, conf->header_tag);

            return(-1);
        }
    }
    else if (!conf->header_line) {

        while ((linep = dsproc_get_next_csv_line(csv))) {

            if (delim) {

                if (exp_ncols) {

                    ndelims = dsproc_count_csv_delims(linep, delim);

                    if (ndelims == exp_ncols - 1) {
                        header_linenum = csv->linenum;
                        break;
                    }
                }
                else if (dsproc_find_csv_delim(linep, delim)) {
                    header_linenum = csv->linenum;
                    break;
                }
            }
            else {

                for (di = 0; (delim = delims[di]); ++di) {

                    if (exp_ncols) {

                        ndelims = dsproc_count_csv_delims(linep, delim);

                        if (ndelims == exp_ncols - 1) {
                            header_linenum = csv->linenum;
                            break;
                        }
                    }
                    else if (dsproc_find_csv_delim(linep, delim)) {
                        header_linenum = csv->linenum;
                        break;
                    }
                }

                if (header_linenum) break;
            }
        }

        if (!header_linenum) {

            DSPROC_ERROR( NULL,
                "Invalid file format: %s\n"
                " -> could not find header line\n",
                data->file_name);

            return(-1);
        }
    }

    /************************************************************
    *  Parse lines before header line
    *************************************************************/

    csv->linenum = 0;

    if (header_linenum) {

        while (csv->linenum < header_linenum - 1) {
            linep = dsproc_get_next_csv_line(csv);
        }
    }

    /************************************************************
    *  Parse header line
    *************************************************************/

    if (conf->header_line) {
        linep = (char *)conf->header_line;
    }
    else {
        linep = dsproc_get_next_csv_line(csv);
    }

    if (!delim) {

        for (di = 0; (delim = delims[di]); ++di) {
            if (dsproc_find_csv_delim(linep, delim)) {
                break;
            }
        }

        if (!delim) {

            DSPROC_ERROR( "Could not determine delimiter from header line",
                "Could not determine delimiter from header line: '%s'\n",
                linep);

            return(-1);
        }
    }

    csv->delim = delim;

    nfields = dsproc_parse_csv_header(csv, linep);
    if (nfields < 0) return(-1);

    if (nfields == 0) {

        DSPROC_ERROR( NULL,
            "Invalid file format: %s\n"
            " -> zero length header line\n",
            data->file_name);

        return(-1);
    }

    if (exp_ncols) {

        if (nfields != exp_ncols) {

            DSPROC_ERROR( NULL,
                "Invalid file format: %s\n"
                " -> expected %d fields in header line but found %d.\n",
                data->file_name, exp_ncols, nfields);

            return(-1);
        }
    }

    /************************************************************
    *  Skip extra header lines
    *************************************************************/

    for (li = 1; li < conf->header_nlines; ++li) {

        linep = dsproc_get_next_csv_line(csv);

        if (!linep) {

            DSPROC_BAD_FILE_WARNING(data->file_name,
                "Unexpected end of file while skipping %d header lines\n",
                conf->header_nlines);

            return(0);
        }
    }

    /************************************************************
    *  Parse the data records
    *************************************************************/

    while ((linep = dsproc_get_next_csv_line(csv))) {

        /* Note: The dsproc_parse_csv_record() function will verify
         * that the number of values found matches the number of
         * header fields found.  If it does not, a bad line warning
         * will be generated and the line will be skipped. */

        status = dsproc_parse_csv_record(csv, linep, 0);
        if (status < 0) return(-1);
    }

    if (csv->nrecs == 0) return(0);

    /************************************************************
    *  Set begin and end time for this dataset
    *************************************************************/

    if (csv->nrecs > 0) {

        data->begin_time = csv->tvs[0].tv_sec;

        if (csv->nrecs > 1) {
            data->end_time = csv->tvs[csv->nrecs-1].tv_sec;
        }
    }

    return(csv->nrecs);
}
