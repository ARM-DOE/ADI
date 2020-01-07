/*******************************************************************************
*
*  COPYRIGHT (C) 2010 Battelle Memorial Institute.  All Rights Reserved.
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
*    $Revision: 80271 $
*    $Author: ermold $
*    $Date: 2017-08-28 01:47:10 +0000 (Mon, 28 Aug 2017) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsproc_update_stored_metadata.c
 *  Update Stored Metadata Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

/** @privatesection */

/*******************************************************************************
 *  Static Data and Functions Visible Only To This Module
 */

typedef struct InDSAttNode InDSAttNode;

struct InDSAttNode {

    InDSAttNode *next;
    char        *name;
    char        *version;
    time_t       start;
    time_t       end;

};

static void _dsproc_free_input_ds_att_nodes(InDSAttNode *root_node)
{
    InDSAttNode *node;
    InDSAttNode *next;

    for (node = root_node; node; node = next) {

        next = node->next;

        if (node->name)    free(node->name);
        if (node->version) free(node->version);
        free(node);
    }
}

static InDSAttNode *_dsproc_create_input_ds_att_node(
    const char *name,
    const char *version,
    time_t      start,
    time_t      end)
{
    InDSAttNode *node = (InDSAttNode *)calloc(1, sizeof(InDSAttNode));
    if (!node) goto MEMORY_ERROR;

    node->name = strdup(name);
    if (!node->name) goto MEMORY_ERROR;

    node->version = strdup(version);
    if (!node->version) goto MEMORY_ERROR;

    node->start = start;
    node->end   = end;

    return(node);

MEMORY_ERROR:

    _dsproc_free_input_ds_att_nodes(node);

    ERROR( DSPROC_LIB_NAME,
        "Could not create input_datastreams attribute node\n"
        " -> memory allocation error\n");

    dsproc_set_status(DSPROC_ENOMEM);
    return((InDSAttNode *)NULL);
}

static int _dsproc_parse_input_datastreams_att(
    const char   *att_value,
    InDSAttNode **root_node)
{
    InDSAttNode *node;
    InDSAttNode *last;
    const char  *linep;
    char         name[256];
    char         version[32];
    int          s_year, s_mon, s_day, s_hour, s_min, s_sec;
    int          e_year, e_mon, e_day, e_hour, e_min, e_sec;
    int          count;
    time_t       start;
    time_t       end;

    linep =  att_value;
    last  = *root_node;

    while (linep) {

        /*  Parse the next line of the attibute value.
         *
         *  Lines have the following format:
         *
         *  "sgpsashenirhisunC1.a0 : 2.1 : 20120625.000000-20120626.000000",
         */

        if (strcmp(linep, "N/A") == 0) {
            /* We can get a value of N/A if no input datastreams were found
             * but the process creates an output dataset anyway. */
            break;
        }

        count = sscanf(linep,
            "%s : %s : %4d%2d%2d.%2d%2d%2d-%4d%2d%2d.%2d%2d%2d",
            name, version,
            &s_year, &s_mon, &s_day, &s_hour, &s_min, &s_sec,
            &e_year, &e_mon, &e_day, &e_hour, &e_min, &e_sec);

        if (count != 8 &&
            count != 14) {

            ERROR( DSPROC_LIB_NAME,
                "Could not parse input_datastreams attribute\n"
                " -> invalid line format: %s\n", linep);

            dsproc_set_status("Invalid input_datastreams Attribute Value Format");
            return(0);
        }

        start = get_secs1970(s_year, s_mon, s_day, s_hour, s_min, s_sec);
        end   = 0;

        if (count == 14) {
            end = get_secs1970(e_year, e_mon, e_day, e_hour, e_min, e_sec);
        }

        for (node = *root_node; node; node = node->next) {

            last = node;

            if ((strcmp(name,    node->name)    == 0) &&
                (strcmp(version, node->version) == 0)) {

                if (start < node->start) {

                    if (node->end == 0) {
                        node->end = node->start;
                    }

                    node->start = start;
                }

                if (end) {
                    if (end > node->end) {
                        node->end = end;
                    }
                }
                else if (node->end) {

                    if (start > node->end) {
                        node->end = start;
                    }
                }
                else {
                    if (start > node->start) {
                        node->end = start;
                    }
                }

                break;
            }
        }

        if (!node) {

            node = _dsproc_create_input_ds_att_node(name, version, start, end);
            if (!node) return(0);

            if (last) {
                last->next = node;
            }
            else {
                *root_node = node;
            }
        }

        /* Set linep to the start of the next line */

        linep = strchr(linep, '\n');
        if (linep) linep += 1;
    }

    return(1);
}

static int _dsproc_update_input_datastreams_att(
    int          ncid,
    InDSAttNode *root_node)
{
    InDSAttNode *node;
    size_t       length;
    char        *value;
    char         start[32];
    char         end[32];
    char        *strp;
    int          status;

    if (!root_node) {
        return(1);
    }

    length = 0;

    // "sgpsashenirhisunC1.a0 : 2.1 : 20120625.000000-20120626.000000",

    /* Determine the max length of the new attribute value */

    for (node = root_node; node; node = node->next) {
        length += strlen(node->name) + strlen(node->version) + 64;
    }

    /* Allocate memory for the new attribute value */

    value = calloc(length, sizeof(char *));
    if (!value) {

        ERROR( DSPROC_LIB_NAME,
            "Could not update input_datastreams attribute\n"
            " -> memory allocation error\n");

        dsproc_set_status(DSPROC_ENOMEM);
        return(0);
    }

    /* Create the new attribute value */

    strp = value;

    for (node = root_node; node; node = node->next) {

        dsproc_create_timestamp(node->start, start);

        if (node->end) {

            dsproc_create_timestamp(node->end, end);

            strp += sprintf(strp, "%s : %s : %s-%s\n",
                node->name, node->version, start, end);
        }
        else {
            strp += sprintf(strp, "%s : %s : %s\n",
                node->name, node->version, start);
        }
    }

    if (strp != value) {
        *--strp = '\0';
    }

    /* Update the attribute value in the output dataset.
     *
     * NOTE:
     *
     * When the _dsproc_update_stored_metadata function is updated to handle
     * user defined atts and static data to update in the previously stored
     * dataset, this should update the att value in the output dataset and
     * then add it to the update list so the redef/enddef is only done once.
     */

    if (!ncds_redef(ncid)) {

        ERROR( DSPROC_LIB_NAME,
            "Could not update input_datastreams attribute\n"
            " -> nc_redef failed\n");

        dsproc_set_status(DSPROC_ENCWRITE);
        free(value);
        return(0);
    }

    length = strlen(value) + 1;

    status = nc_put_att_text(
        ncid, NC_GLOBAL, "input_datastreams", length, value);

    if (status != NC_NOERR) {

        ERROR( DSPROC_LIB_NAME,
            "Could not redefine input_datastreams attribute\n"
            " -> %s\n",
            nc_strerror(status));

        dsproc_set_status(DSPROC_ENCWRITE);
        free(value);
        return(0);
    }

    if (!ncds_enddef(ncid)) {

        ERROR( DSPROC_LIB_NAME,
            "Could not update input_datastreams attribute\n"
            " -> nc_enddef failed\n");

        dsproc_set_status(DSPROC_ENCWRITE);
        free(value);
        return(0);
    }

    free(value);
    return(1);
}

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/**
 *  Update metadata in a stored dataset.
 *
 *  For now this function only works to merge the input_datastreams att
 *  values.  In the future it can be extended to allow the user to
 *  provide a list of attributes and static data that should be updated
 *  using the values in the current dataset being stored.
 */
int _dsproc_update_stored_metadata(CDSGroup *dataset, int ncid)
{
    CDSAtt      *cds_att;
    size_t       length;
    char        *nc_att_value;
    InDSAttNode *root_node;

    /* Check if the input and output datasets both have the
     * input_datastreams attribute defined. */

    cds_att = cds_get_att(dataset, "input_datastreams");
    if (!cds_att || cds_att->type != CDS_CHAR) return(1);

    length = ncds_get_att_text(ncid, NC_GLOBAL, "input_datastreams", &nc_att_value);
    if (length == (size_t)-1) {
        dsproc_set_status(DSPROC_ENCREAD);
        return(0);
    }
    else if (length == 0) {
        return(1);
    }

    /* Check if the attribute value has changed. */

    if (strcmp(cds_att->value.cp, nc_att_value) == 0) {
        free(nc_att_value);
        return(1);
    }

    /* Parse and merge both attribute values. */

    root_node = (InDSAttNode *)NULL;

    if (!_dsproc_parse_input_datastreams_att(nc_att_value, &root_node)) {
        free(nc_att_value);
        return(0);
    }

    if (!_dsproc_parse_input_datastreams_att(cds_att->value.cp, &root_node)) {
        _dsproc_free_input_ds_att_nodes(root_node);
        free(nc_att_value);
        return(0);
    }

    /* Update the attribute value in the stored dataset. */

    if (root_node) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s: Updating input_datastreams attribute in output file\n",
            dataset->name);

        if (!_dsproc_update_input_datastreams_att(ncid, root_node)) {

            _dsproc_free_input_ds_att_nodes(root_node);
            free(nc_att_value);
            return(0);
        }

        _dsproc_free_input_ds_att_nodes(root_node);
    }

    free(nc_att_value);

    return(1);
}
