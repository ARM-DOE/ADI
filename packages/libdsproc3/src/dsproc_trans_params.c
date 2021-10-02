/*******************************************************************************
*
*  Copyright © 2014, Battelle Memorial Institute
*  All rights reserved.
*
********************************************************************************
*
*  Author:
*     name:  Brian Ermold
*     phone: (509) 375-2277
*     email: brian.ermold@pnl.gov
*
*******************************************************************************/

/** @file dsproc_trans_params.c
 *  Transformation Parameter Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** @privatesection */


/*******************************************************************************
 *  Static Functions Visible Only To This Module
 */

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/**
 *  Private: Free an array of trans param dim_groups.
 *
 *  @param  trans_dim_groups - pointer to TransDimGroup array
 */
void _dsproc_free_trans_dim_groups(TransDimGroup *trans_dim_groups)
{
    int dgi;

    if (trans_dim_groups) {

        for (dgi = 0; trans_dim_groups[dgi].in_dim; ++dgi) {

            if (trans_dim_groups[dgi].out_dims)
                free(trans_dim_groups[dgi].out_dims);

            free(trans_dim_groups[dgi].in_dim);
        }

        free(trans_dim_groups);
    }
}

/**
 *  Private: Get the trans param dim_groups for a variable.
 *
 *  This function looks for and parses the tramsformation parameter
 *  'dim_grouping' originally added to support the Caracena transformation
 *  method.  This parameter has the format:
 *
 *  {time}, {station:lat,lon}, {height}
 *
 *  The memory used by the output array is dynamically allocated and must
 *  be freed by _dsproc_free_trans_dim_groups.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_group         - pointer to the CDSGroup
 *  @param  var_name         - variable name
 *  @param  trans_dim_groups - output: pointer to TransDimGroup array
 *
 *  @return
 *    -  ngroups  the number of transformation dimension groups
 *    -  0        if the dim_grouping parameter is not defined for the variable
 *    - -1        if an error occurred
 */
int _dsproc_get_trans_dim_groups(
    CDSGroup       *ds_group,
    const char     *var_name,
    TransDimGroup **trans_dim_groups)
{
    char   *value;
    size_t  length;
    char   *chrp;
    char   *strp;
    char   *colon;
    int     count;
    int     ngroups;
    char   *startp;
    char   *endp;
    int     out_ndims;
    char  **out_dims;

    TransDimGroup *dim_group;

    *trans_dim_groups = (TransDimGroup *)NULL;

    /* Check if a dim_grouping was specified for this variable */

    value = cds_get_transform_param_from_group(ds_group, var_name,
        "dim_grouping", CDS_CHAR, &length, NULL);

    if (!value) return(0);

    /* Remove all spaces so we don't have to deal with them later */

    strp = value;
    for (chrp = value; *chrp != '\0'; ++chrp) {
        if (!isspace(*chrp)) *strp++ = *chrp;
    }

    *strp = '\0';

    /* Allocate memory for array of trans_dim_groups */

    strp  = value;
    count = 0;

    while ((strp = strchr(strp, '{'))) {
        count += 1;
        strp  += 1;
    }

    *trans_dim_groups = (TransDimGroup *)calloc(count + 1, sizeof(TransDimGroup));
    if (!trans_dim_groups) goto MEM_ERROR;

    /* Parse dim_grouping string */

    startp  = value;
    ngroups = 0;

    while ((startp = strchr(startp, '{'))) {

        startp += 1;

        // find end bracket

        endp = strchr(startp, '}');
        if (!endp) goto INVALID_FORMAT;

        *endp = '\0';

        if (endp == startp + 1) goto INVALID_FORMAT;

        // set in_dim

        dim_group = &(*trans_dim_groups)[ngroups];
        dim_group->in_dim = strdup(startp);
        if (!dim_group->in_dim) goto MEM_ERROR;

        // find colon

        colon = strchr(dim_group->in_dim, ':');

        if (colon) {

            if (colon == dim_group->in_dim + 1) goto INVALID_FORMAT;

            *colon = '\0';

            // count commas

            out_ndims = 1;
            strp      = colon + 1;

            while ((strp = strchr(strp, ','))) {
                strp      += 1;
                out_ndims += 1;
            }

            // allocate memory for out_dims array

            out_dims = (char **)calloc(out_ndims, sizeof(char *));
            if (!out_dims) goto MEM_ERROR;

            // parse out dims

            out_ndims   = 1;
            strp        = colon + 1;
            out_dims[0] = strp;

            while ((strp = strchr(strp, ','))) {
                *strp = '\0';
                strp      += 1;
                out_dims[out_ndims++] = strp;
            }

            dim_group->out_ndims = out_ndims;
            dim_group->out_dims  = out_dims;
        }

        ngroups += 1;
        startp = endp + 1;
    }

    if (ngroups != count) goto INVALID_FORMAT;

    free(value);
    return(ngroups);

MEM_ERROR:

    if (value) free(value);

    ERROR( DSPROC_LIB_NAME,
        "Could not parse transformation dimension groups for: %s\n"
        " -> memory allocation error\n",
        var_name);

    dsproc_set_status(DSPROC_ENOMEM);
    return(-1);

INVALID_FORMAT:

    if (value) free(value);

    ERROR( DSPROC_LIB_NAME,
        "Invalid dim_grouping transformation parameter format for variable: %s\n",
        var_name);

    dsproc_set_status("Invalid dim_grouping Transformation Parameter Format");
    return(-1);
}

/**
 *  Private: Set the transformation parameters using the bounds variable.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dim - pointer to the CDSDim
 *
 *  @return
 *    -  1 if successful
 *    -  0 if a boundary variable does not exist
 *    - -1 if an error occurred
 */
int _dsproc_set_trans_params_from_bounds_var(CDSDim *dim)
{
    CDSGroup   *dataset    = (CDSGroup *)dim->parent;
    const char *dim_name   = dim->name;
    double     *front_edge = (double *)NULL;
    double     *back_edge  = (double *)NULL;
    CDSVar     *coord_var;
    CDSVar     *bounds_var;
    CDSAtt     *att;
    size_t      length;
    CDSData     data;
    double      dbl_val;
    size_t      bi;

    /* Check if there is a coordinate variable for this dimension */

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Checking for bounds variable for: %s:%s\n",
        dataset->name, dim->name);

    coord_var = cds_get_var(dataset, dim_name);

    if (!coord_var ||
        coord_var->ndims   != 1 ||
        coord_var->dims[0] != dim) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            " - coordinate variable not found\n");

        return(0);
    }

    /* Check for a valid bounds variable */

    bounds_var = cds_get_bounds_var(coord_var);
    if (bounds_var) {

        if (bounds_var->ndims   != 2   ||
            bounds_var->dims[0] != dim ||
            bounds_var->dims[1]->length != 2) {

            DEBUG_LV1( DSPROC_LIB_NAME,
                " - bounds variable not found\n");

            bounds_var = (CDSVar *)NULL;
        }
    }

    if (bounds_var) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            " - setting front/back_edge transform params using bounds variable\n");

        /* Set front and back edge using bounds variable */

        length = bounds_var->dims[0]->length;

        if (!(front_edge = malloc(length * sizeof(double))) ||
            !(back_edge  = malloc(length * sizeof(double)))) {

            goto MEMORY_ERROR;
        }

        data.vp = bounds_var->data.vp;

        switch (bounds_var->type) {

            case CDS_BYTE:
                for (bi = 0; bi < length; ++bi) {
                    front_edge[bi] = (double)*data.bp++;
                    back_edge[bi]  = (double)*data.bp++;
                }
                break;

            case CDS_SHORT:
                for (bi = 0; bi < length; ++bi) {
                    front_edge[bi] = (double)*data.sp++;
                    back_edge[bi]  = (double)*data.sp++;
                }
                break;

            case CDS_INT:
                for (bi = 0; bi < length; ++bi) {
                    front_edge[bi] = (double)*data.ip++;
                    back_edge[bi]  = (double)*data.ip++;
                }
                break;

            case CDS_FLOAT:
                for (bi = 0; bi < length; ++bi) {
                    front_edge[bi] = (double)*data.fp++;
                    back_edge[bi]  = (double)*data.fp++;
                }
                break;

            case CDS_DOUBLE:
                for (bi = 0; bi < length; ++bi) {
                    front_edge[bi] = *data.dp++;
                    back_edge[bi]  = *data.dp++;
                }
                break;

            default:

                ERROR( DSPROC_LIB_NAME,
                    "Invalid data type '%d' for bounds variable: %s\n",
                    bounds_var->type, cds_get_object_path(bounds_var));

                dsproc_set_status(DSPROC_EBOUNDSVAR);

                free(front_edge);
                free(back_edge);
                return(-1);
        }

        if (!cds_set_transform_param(dataset, dim_name,
            "front_edge", CDS_DOUBLE, length, front_edge)) {

            goto MEMORY_ERROR;
        }

        if (!cds_set_transform_param(dataset, dim_name,
            "back_edge", CDS_DOUBLE, length, back_edge)) {

            goto MEMORY_ERROR;
        }

        free(front_edge);
        free(back_edge);

        return(1);
    }
    else {

        /* Check for the Conventions attribute.  If this has been
         * defined and specifies the ARM or CF convention we can
         * assume the values are instantaneous. */

        DEBUG_LV1( DSPROC_LIB_NAME,
            " - checking for 'Conventions' global attribute\n");

        att = cds_get_att(dataset, "Conventions");
        if (att && att->type == CDS_CHAR && att->length && att->value.vp) {

            if (strstr(att->value.cp, "ARM") ||
                strstr(att->value.cp, "CF")) {

                DEBUG_LV1( DSPROC_LIB_NAME,
                    " - found 'Conventions' == '%s'\n"
                    " - assuming point values (width = 0, alignment = 0.5)\n",
                    att->value.cp);

                dbl_val = 0.0;

                if (!cds_set_transform_param(dataset, dim_name,
                    "width", CDS_DOUBLE, 1, (void *)&dbl_val)) {

                    goto MEMORY_ERROR;
                }

                dbl_val = 0.5;

                if (!cds_set_transform_param(dataset, dim->name,
                    "alignment", CDS_DOUBLE, 1, (void *)&dbl_val)) {

                    goto MEMORY_ERROR;
                }

                return(1);
            }
            else {
                DEBUG_LV1( DSPROC_LIB_NAME,
                    " - unknown value for 'Conventions' attribute: '%s'\n",
                    att->value.cp);
            }
        }
        else {
            DEBUG_LV1( DSPROC_LIB_NAME,
                " - 'Conventions' attribute not found\n");
        }
    }

    return(0);

MEMORY_ERROR:

    if (front_edge) free(front_edge);
    if (back_edge)  free(back_edge);

    ERROR( DSPROC_LIB_NAME,
        "Could not set transformation parameters for: %s:%s\n"
        " -> memory allocation error\n",
        dataset->name, dim->name);

    dsproc_set_status(DSPROC_ENOMEM);
    return(-1);
}

/**
 *  Private: Set the transformation parameters from ds_property table.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dsid - datastream ID
 *  @param  dim  - pointer to the CDSDim
 *
 *  @return
 *    -  1 if successful
 *    -  0 if both width and alignment were not found in the DSDB
 *    - -1 if an error occurred
 */
int _dsproc_set_trans_params_from_dsprops(int dsid, CDSDim *dim)
{
    CDSGroup   *dataset   = (CDSGroup *)dim->parent;
    const char *dim_name  = dim->name;
    timeval_t   data_time = { 0, 0 };
    size_t      length;
    size_t      found_width;
    size_t      found_alignment;
    int         status;
    double      dbl_val;
    const char *str_val;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Checking for trans params defined in ds_property table for: %s:%s\n",
        dataset->name, dim_name);

    /* Get the time of the first sample in the dataset. */

    length = 1;
    if (!dsproc_get_sample_timevals(dataset, 0, &length, &data_time)) {
        if (length != 0) return(-1);
    }

    /* Set the width from the datastream properties table. */

    status = dsproc_get_datastream_property(
        dsid, dim_name, "trans_bin_width", data_time.tv_sec, &str_val);

    if (status  < 0) return(-1);
    if (status == 1) {

        dbl_val = atof(str_val);

        DEBUG_LV1( DSPROC_LIB_NAME,
            " - found: width     = %g\n",
            dbl_val);

        if (!cds_set_transform_param(dataset, dim_name,
            "width", CDS_DOUBLE, 1, (void *)&dbl_val)) {

            goto MEMORY_ERROR;
        }
    }

    found_width = status;

    /* Set the alignment from the datastream properties table. */

    status = dsproc_get_datastream_property(
        dsid, dim_name, "trans_bin_alignment", data_time.tv_sec, &str_val);

    if (status  < 0) return(0);
    if (status == 1) {

        dbl_val = atof(str_val);

        DEBUG_LV1( DSPROC_LIB_NAME,
            " - found: alignment = %g\n",
            dbl_val);

        if (!cds_set_transform_param(dataset, dim_name,
            "alignment", CDS_DOUBLE, 1, (void *)&dbl_val)) {

            goto MEMORY_ERROR;
        }
    }

    found_alignment = status;

    if (found_width || found_alignment) {
        return(1);
    }
    else {
        DEBUG_LV1( DSPROC_LIB_NAME,
            " - none found\n");
    }

    return(0);

MEMORY_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Could not set transformation parameters for: %s:%s\n"
        " -> memory allocation error\n",
        dataset->name, dim_name);

    dsproc_set_status(DSPROC_ENOMEM);
    return(-1);
}

/**
 *  Private: Set the transformation parameters for a retrieved dimension.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dsid - datastream ID
 *  @param  dim  - pointer to the retrieved dimension
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int _dsproc_set_ret_dim_trans_params(int dsid, CDSDim *dim)
{
    CDSGroup   *dataset      = (CDSGroup *)dim->parent;
    const char *dim_name     = dim->name;
    double     *front_edge   = (double *)NULL;
    double     *back_edge    = (double *)NULL;
    size_t      length;
    size_t      bi;

    double      dbl_val;
    int         status;

    double      width;

    size_t      found_front_edge;
    size_t      found_back_edge;
    size_t      found_width;
    size_t      found_alignment;
    int         found_bounds;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Checking input transformation parameters for: %s:%s\n",
        dataset->name, dim_name);

    /* Check for parameter values that have already been set,
     * we do not want to overwrite them if they have. */

    found_front_edge = 1;
    cds_get_transform_param(dim,
        "front_edge", CDS_DOUBLE, &found_front_edge, &dbl_val);

    found_back_edge = 1;
    cds_get_transform_param(dim,
        "back_edge", CDS_DOUBLE, &found_back_edge, &dbl_val);

    found_width = 1;
    cds_get_transform_param(dim,
        "width", CDS_DOUBLE, &found_width, &width);

    found_alignment = 1;
    cds_get_transform_param(dim,
        "alignment", CDS_DOUBLE, &found_alignment, &dbl_val);

    /* Check if a width has been defined. */

    if (found_width) {

        if (found_front_edge && !found_back_edge) {

            /* Compute back_edge from front_edge and width */

            DEBUG_LV1( DSPROC_LIB_NAME,
                " - computing back_edge from front_edge and width\n");

            front_edge = cds_get_transform_param(dim,
                "front_edge", CDS_DOUBLE, &length, NULL);

            if (!front_edge) goto MEMORY_ERROR;

            back_edge = (double *)malloc(length * sizeof(double));
            if (!back_edge) goto MEMORY_ERROR;

            for (bi = 0; bi < length; ++bi) {
                back_edge[bi] = front_edge[bi] + width;
            }

            if (!cds_set_transform_param(dataset, dim->name,
                "back_edge", CDS_DOUBLE, length, back_edge)) {

                free(back_edge);
                goto MEMORY_ERROR;
            }

            free(back_edge);
            found_back_edge = 1;
        }
        else if (found_back_edge && !found_front_edge) {

            /* Compute front_edge from back_edge and width */

            DEBUG_LV1( DSPROC_LIB_NAME,
                " - computing front_edge from back_edge and width\n");

            back_edge = cds_get_transform_param(dim,
                "back_edge", CDS_DOUBLE, &length, NULL);

            if (!back_edge) goto MEMORY_ERROR;

            front_edge = (double *)malloc(length * sizeof(double));
            if (!front_edge) goto MEMORY_ERROR;

            for (bi = 0; bi < length; ++bi) {
                front_edge[bi] = back_edge[bi] - width;
            }

            if (!cds_set_transform_param(dataset, dim_name,
                "front_edge", CDS_DOUBLE, length, front_edge)) {

                free(front_edge);
                goto MEMORY_ERROR;
            }

            free(front_edge);
            found_front_edge = 1;
        }
        else if (!found_alignment) {

            /* Assume center bin alignment */

            DEBUG_LV1( DSPROC_LIB_NAME,
                " - found width without alignment\n"
                " - assuming center bin alignment (alignment == 0.5)\n");

            dbl_val = 0.5;

            if (!cds_set_transform_param(dataset, dim_name,
                "alignment", CDS_DOUBLE, 1, (void *)&dbl_val)) {

                goto MEMORY_ERROR;
            }

            found_alignment = 1;
        }
    }

    /* If front_edge/back_edge or width/alignment information hasn't
     * been defined we need to check for a boundary variable, or
     * if the Conventions attribute specifies ARM or CF. */

    if ((!found_front_edge || !found_back_edge) &&
        (!found_width || !found_alignment)) {

        found_bounds = _dsproc_set_trans_params_from_bounds_var(dim);
        if (found_bounds < 0) return(0);

        /* If the bounds variable was not found we need to get the
         * information from the datastream properties table. */

        if (!found_bounds) {
            status = _dsproc_set_trans_params_from_dsprops(dsid, dim);
            if (status < 0) return(0);
        }
    }

    return(1);

MEMORY_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Could not set input transformation parameters for: %s:%s\n"
        " -> memory allocation error\n",
        dataset->name, dim_name);

    dsproc_set_status(DSPROC_ENOMEM);
    return(0);
}

/**
 *  Private: Set the transform parameters for a retrieved coordinate variable.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dsid - datastream ID
 *  @param  obs  - pointer to the retrieved observation
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int _dsproc_set_ret_obs_params(int dsid, CDSGroup *obs)
{
    CDSDim *dim;
    int di;

    for (di = 0; di < obs->ndims; ++di) {

        dim = obs->dims[di];

        if (strcmp(dim->name, "bound") == 0) {
            continue;
        }

        if (!_dsproc_set_ret_dim_trans_params(dsid, dim)) {
            return(0);
        }
    }

    return(1);
}

/**
 *  Private: Set Transform paramters for a trans_coord_var.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param trans_coord_var - pointer to the trans_coord_var
 *  @param ret_dsid        - ID of the retrieved datastream
 *                           or -1 if this isn't a direct mapping
 *  @param ret_coord_dim   - pointer to the RetCoordDim structure
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int _dsproc_set_trans_coord_var_params(
    CDSVar      *trans_coord_var,
    int          ret_dsid,
    RetCoordDim *ret_coord_dim)
{
    CDSGroup   *trans_coordsys  = (CDSGroup *)trans_coord_var->parent;
    CDSDim     *trans_coord_dim = (CDSDim *)NULL;
    size_t      found_width     = 0;
    size_t      found_alignment = 0;
    int         found_bounds    = 0;
    const char *dim_name;
    size_t      length;
    double      dbl_val;
    int         int_val;
    int         nfound;
    int         status;

    if (trans_coord_var->ndims == 1) {
        trans_coord_dim = trans_coord_var->dims[0];
        dim_name = trans_coord_dim->name;
    }
    else {
        dim_name = trans_coord_var->name;
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Checking for trans params defined in ret_coord_dims table for: %s:%s\n",
        trans_coordsys->name, dim_name);

    found_width = 1;
    cds_get_transform_param_from_group(trans_coordsys, dim_name,
        "width", CDS_DOUBLE, &found_width, &dbl_val);

    found_alignment = 1;
    cds_get_transform_param_from_group(trans_coordsys, dim_name,
        "alignment", CDS_DOUBLE, &found_alignment, &dbl_val);

    nfound = 0;

    if (ret_coord_dim) {

        /* Set transformation type */

        if (ret_coord_dim->trans_type) {

            length = strlen(ret_coord_dim->trans_type) + 1;

            if (!cds_set_transform_param(trans_coordsys, dim_name,
                "transform", CDS_CHAR, length, (void *)ret_coord_dim->trans_type)) {

                goto MEMORY_ERROR;
            }

            nfound += 1;
        }

        /* Set range */

        if (ret_coord_dim->trans_range) {

            dbl_val = atof(ret_coord_dim->trans_range);

            if (!cds_set_transform_param(trans_coordsys, dim_name,
                "range", CDS_DOUBLE, 1, (void *)&dbl_val)) {

                goto MEMORY_ERROR;
            }

            nfound += 1;
        }

        /* Set width */

        if (ret_coord_dim->interval) {

            dbl_val = atof(ret_coord_dim->interval);

            if (!cds_set_transform_param(trans_coordsys, dim_name,
                "interval", CDS_DOUBLE, 1, (void *)&dbl_val)) {

                goto MEMORY_ERROR;
            }

            if (!found_width) {

                if (!cds_set_transform_param(trans_coordsys, dim_name,
                    "width", CDS_DOUBLE, 1, (void *)&dbl_val)) {

                    goto MEMORY_ERROR;
                }

                found_width = 1;
            }

            nfound += 1;
        }

        /* Set alignment */

        if (ret_coord_dim->trans_align) {

            int_val = atoi(ret_coord_dim->trans_align);

            if      (int_val == -1) dbl_val = 0.0;
            else if (int_val ==  0) dbl_val = 0.5;
            else                    dbl_val = 1.0;

            if (!cds_set_transform_param(trans_coordsys, dim_name,
                "alignment", CDS_DOUBLE, 1, (void *)&dbl_val)) {

                goto MEMORY_ERROR;
            }

            found_alignment = 1;
            nfound += 1;
        }
    }

    /* Print the transform parameters if we are in debug mode */

    if (nfound == 0) {
        DEBUG_LV1( DSPROC_LIB_NAME,
            " - none found\n");
    }
    else if (msngr_debug_level > 1) {

        fprintf(stdout,
        "\n"
        "--------------------------------------------------------------------\n"
        "Transformation Parameters For: %s:%s\n"
        " -> after loading parameters defined in ret_coord_dims table\n"
        "\n", trans_coordsys->name, dim_name);

        cds_print_transform_params(stdout, "    ",
            trans_coordsys, dim_name);

        fprintf(stdout,
        "--------------------------------------------------------------------\n"
        "\n");
    }

    if (trans_coord_dim) {

        /* If width/alignment information hasn't been defined we need to check
         * for a boundary variable, or if the Conventions attribute specifies
         * ARM or CF. */

        if (!found_width || !found_alignment) {

            found_bounds = _dsproc_set_trans_params_from_bounds_var(
                trans_coord_dim);

            if (found_bounds < 0) return(0);

            /* If a boundary variable was not found we need to get the
             * information from the datastream properties table. */

            if (!found_bounds && ret_dsid >= 0) {

                status = _dsproc_set_trans_params_from_dsprops(
                    ret_dsid, trans_coord_dim);

                if (status < 0) return(0);
            }
        }
    }

    return(1);

MEMORY_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Could not set transformation parameters for: %s->%s\n"
        " -> memory allocation error\n",
        trans_coordsys->name, trans_coord_var->name);

    dsproc_set_status(DSPROC_ENOMEM);
    return(0);
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/**
 *  Internal: Load transformation parameters.
 *
 *  Load transformation parameters defined in the retriever definition and/or
 *  a transformation parameters file. See dsproc_load_ret_transform_params and
 *  dsproc_load_transform_params_file.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  group    - pointer to the CDSGroup to store the transformation
 *                     parameters in
 *  @param  site     - site name or
 *                     NULL to skip search patterns containing the site name.
 *  @param  facility - facility name or
 *                     NULL to skip search patterns containing the facility name.
 *  @param  name     - base name of the coordinate system to look for
 *  @param  level    - data level or
 *                     NULL to skip search patterns containing the data level.
 *
 *  @return
 *    -  1 if successful
 *    -  0 no transformation parameters found
 *    - -1 if an error occurred
 */
int dsproc_load_transform_params(
    CDSGroup   *group,
    const char *site,
    const char *facility,
    const char *name,
    const char *level)
{
    int retval = 0;
    int status;

    /* Load the transform parameters file first */

    status = dsproc_load_transform_params_file(
        group, site, facility, name, level);
    
    if (status < 0) return(-1);
    if (status > 0) retval = 1;

    /* Load the transform parameters from the ret_transform_params table,
     * these will overwrite any values defined in the conf file. */

    status = dsproc_load_ret_transform_params(
        group, site, facility, name, level);

    if (status < 0) return(-1);
    if (status > 0) retval = 1;

    return(retval);
}

/**
 *  Internal: Load a transformation parameters file.
 *
 *  This function will look for a transformation parameters file in the
 *  following directories in the order specified:
 *
 *      "dsenv_get_data_conf_root()/transform/<proc_name>"
 *      "dsenv_get_apps_conf_root()/transform"
 *
 *  The first file found in the following search order will be loaded:
 *
 *    - {site}{name}{facility}.{level}
 *    - {site}{name}.{level}
 *    - {name}.{level}
 *    - {name}
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  group    - pointer to the CDSGroup to store the transformation
 *                     parameters in
 *  @param  site     - site name or
 *                     NULL to skip search patterns containing the site name.
 *  @param  facility - facility name or
 *                     NULL to skip search patterns containing the facility name.
 *  @param  name     - base name of the file to look for
 *  @param  level    - data level or
 *                     NULL to skip search patterns containing the data level.
 *
 *  @return
 *    -  1 if successful
 *    -  0 no transformation parameters file found
 *    - -1 if an error occurred
 */
int dsproc_load_transform_params_file(
    CDSGroup   *group,
    const char *site,
    const char *facility,
    const char *name,
    const char *level)
{
    char *conf_root;
    char  conf_path[PATH_MAX];
    char  file_name[PATH_MAX];
    int   dir_index;
    int   file_index;
    int   check;
    int   status;

    status = 0;

    for (dir_index = 0; dir_index < 2; ++dir_index) {

        /* Get the full path to the transformation params files. */

        switch (dir_index) {

            case 0:

                status = dsenv_get_data_conf_root(&conf_root);

                if (status < 0) {
                    dsproc_set_status(DSPROC_ENOMEM);
                    return(-1);
                }

                if (status == 0) {
                    continue;
                }

                snprintf(conf_path, PATH_MAX, "%s/transform/%s",
                    conf_root, _DSProc->name);

                break;

            case 1:

                status = dsenv_get_apps_conf_root(
                    _DSProc->name, _DSProc->type, &conf_root);

                if (status < 0) {
                    dsproc_set_status(DSPROC_ENOMEM);
                    return(-1);
                }

                if (status == 0) {
                    continue;
                }

                snprintf(conf_path, PATH_MAX, "%s/transform",
                    conf_root);

                break;
        }

        free(conf_root);
        status = 0;

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Checking for transformation parameter files in: %s\n", conf_path);

        if (access(conf_path, F_OK) != 0) {

            if (errno != ENOENT) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not access directory: %s\n"
                    " -> %s\n", conf_path, strerror(errno));

                dsproc_set_status(DSPROC_EACCESS);
                return(-1);
            }

            DEBUG_LV1( DSPROC_LIB_NAME,
                " - directory not found\n");

            continue;
        }

        /* Search for the transform parameter file */

        for (file_index = 0; file_index < 4; file_index++) {

            check  = 0;

            switch (file_index) {

                case 0:
                    /* <site><name><facility>.<level> */

                    if (site && facility && name && level) {

                        snprintf(file_name, PATH_MAX, "%s%s%s.%s",
                            site, name, facility, level);

                        check = 1;
                    }
                    break;
                case 1:
                    /* <site><name>.<level> */

                    if (site && name && level) {
                        snprintf(file_name, PATH_MAX, "%s%s.%s", site, name, level);
                        check = 1;
                    }
                    break;
                case 2:
                    /* <name>.<level> */

                    if (name && level) {
                        snprintf(file_name, PATH_MAX, "%s.%s", name, level);
                        check = 1;
                    }
                    break;
                case 3:
                    /* <name> */

                    if (name) {
                        snprintf(file_name, PATH_MAX, "%s", name);
                        check = 1;
                    }
                    break;
            }

            if (check) {

                DEBUG_LV1( DSPROC_LIB_NAME,
                    " - checking for: %s\n", file_name);

                status = cds_load_transform_params_file(
                    group, conf_path, file_name);
            }

            if (status != 0) {
                break;
            }
        }

        if (status == 0) {
            DEBUG_LV1( DSPROC_LIB_NAME,
                " - none found\n");
        }
        else {
            break;
        }
    }

    /* Cleanup and return */

    if (status < 0) {
        dsproc_set_status(DSPROC_ETRANSPARAMLOAD);
        return(-1);
    }

    if (status == 0) {
        return(0);
    }

    /* Print the transform parameters if we are in debug mode */

    if (msngr_debug_level > 1) {

        fprintf(stdout,
        "\n"
        "--------------------------------------------------------------------\n"
        "Transformation Parameters For: %s\n"
        " -> after loading parameters defined in file: %s/%s\n"
        "\n", group->name, conf_path, file_name);

        cds_print_transform_params(stdout, "    ", group, NULL);

        fprintf(stdout,
        "--------------------------------------------------------------------\n"
        "\n");
    }

    return(1);
}

/**
 *  Internal: Load transformation parameters defined in retriever definition.
 *
 *  The first coordinate system found in the following search order will be
 *  loaded:
 *
 *    - {site}{name}{facility}.{level}
 *    - {site}{name}.{level}
 *    - {name}.{level}
 *    - {name}
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  group    - pointer to the CDSGroup to store the transformation
 *                     parameters in
 *  @param  site     - site name or
 *                     NULL to skip search patterns containing the site name.
 *  @param  facility - facility name or
 *                     NULL to skip search patterns containing the facility name.
 *  @param  name     - base name of the coordinate system to look for
 *  @param  level    - data level or
 *                     NULL to skip search patterns containing the data level.
 *
 *  @return
 *    -  1 if successful
 *    -  0 no transformation parameters found
 *    - -1 if an error occurred
 */
int dsproc_load_ret_transform_params(
    CDSGroup   *group,
    const char *site,
    const char *facility,
    const char *name,
    const char *level)
{
    Retriever *ret = _DSProc->retriever;
    RetTransParams *trans_params;
    char  coordsys[PATH_MAX];
    char *params;
    int   index;
    int   check;
    int   status;
    int   i;

    if (!ret) return(0);

    status = 0;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Checking for trans params defined in ret_transform_params table for: %s\n",
        group->name);

    for (index = 0; index < 4; index++) {

        check = 0;

        switch (index) {

            case 0:
                /* <site><name><facility>.<level> */

                if (site && facility && name && level) {

                    snprintf(coordsys, PATH_MAX, "%s%s%s.%s",
                        site, name, facility, level);

                    check = 1;
                }
                break;
            case 1:
                /* <site><name>.<level> */

                if (site && name && level) {
                    snprintf(coordsys, PATH_MAX, "%s%s.%s", site, name, level);
                    check = 1;
                }
                break;
            case 2:
                /* <name>.<level> */

                if (name && level) {
                    snprintf(coordsys, PATH_MAX, "%s.%s", name, level);
                    check = 1;
                }
                break;
            case 3:
                /* <name> */

                if (name) {
                    snprintf(coordsys, PATH_MAX, "%s", name);
                    check = 1;
                }
                break;
        }

        if (check) {

            DEBUG_LV1( DSPROC_LIB_NAME,
                " - checking for: %s\n", coordsys);

            for (i = 0; i < ret->ntrans_params; ++i) {

                trans_params = ret->trans_params[i];

                if (strcmp(coordsys, trans_params->coordsys) == 0) {

                    params = strdup(trans_params->params);
                    if (!params) {
                        ERROR( DSPROC_LIB_NAME,
                            "Could not set transformation parameters for: %s\n"
                            " -> memory allocation error\n",
                            group->name);
                        dsproc_set_status(DSPROC_ENOMEM);
                        return(-1);
                    }

                    status = cds_parse_transform_params(group, params, NULL);
                    free(params);

                    if (status == 0) status = -1;
                    break;
                }
            }
        }

        if (status != 0) {
            break;
        }
    }

    /* Cleanup and return */

    if (status < 0) {
        dsproc_set_status(DSPROC_ETRANSPARAMLOAD);
        return(-1);
    }

    if (status == 0) {
        DEBUG_LV1( DSPROC_LIB_NAME,
            " - none found\n");
        return(0);
    }

    /* Print the transform parameters if we are in debug mode */

    if (msngr_debug_level > 1) {

        fprintf(stdout,
        "\n"
        "--------------------------------------------------------------------\n"
        "Transformation Parameters For: %s\n"
        " -> after loading parameters defined in ret_transform_params table\n"
        "\n", group->name);

        cds_print_transform_params(stdout, "    ", group, NULL);

        fprintf(stdout,
        "--------------------------------------------------------------------\n"
        "\n");
    }

    return(1);
}

/**
 *  Internal: Load transformation parameters set by user code.
 *
 *  Load the transformation parameters set by the user using the
 *  dsproc_set_coordsys_trans_param() function.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  coordsys_name  - name of the coordinate system
 *  @param  trans_coordsys - pointer to the transformation coordinate system
 *
 *  @return
 *    -  1 if successful
 *    -  0 no transformation parameters found
 *    - -1 if an error occurred
 */
int dsproc_load_user_transform_params(
    const char *coordsys_name,
    CDSGroup   *trans_coordsys)
{
    CDSGroup *coordsys_params;
    int       status;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Checking for trans params defined by user for coodinate system: %s\n",
        coordsys_name);

    if (!_DSProc->trans_params) {
        DEBUG_LV1( DSPROC_LIB_NAME,
            " - none found\n");
        return(0);
    }

    coordsys_params = cds_get_group(
        _DSProc->trans_params, coordsys_name);

    if (!coordsys_params) {
        DEBUG_LV1( DSPROC_LIB_NAME,
            " - none found\n");
        return(0);
    }

    status = cds_copy_transform_params(
        coordsys_params, trans_coordsys);

    if (status <= 0) {
        return(-1);
    }

    /* Print the transform parameters if we are in debug mode */

    if (msngr_debug_level > 1) {

        fprintf(stdout,
        "\n"
        "--------------------------------------------------------------------\n"
        "Transformation Parameters For: %s\n"
        " -> after loading parameters defined by user code\n"
        "\n", trans_coordsys->name);

        cds_print_transform_params(stdout, "    ", trans_coordsys, NULL);

        fprintf(stdout,
        "--------------------------------------------------------------------\n"
        "\n");
    }

    return(1);
}

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Set the value of a coordinate system transformation parameter.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  coordsys_name - name of the coordinate system
 *  @param  field_name   - name of the field
 *  @param  param_name   - name of the parameter
 *  @param  type         - data type of the specified value
 *  @param  length       - length of the specified value
 *  @param  value        - pointer to the parameter value
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int dsproc_set_coordsys_trans_param(
    const char  *coordsys_name,
    const char  *field_name,
    const char  *param_name,
    CDSDataType  type,
    size_t       length,
    void        *value)
{
    CDSGroup *trans_params;
    CDSGroup *trans_coordsys;
    int       status;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Setting user defined transformation parameter: %s:%s:%s\n",
        coordsys_name, field_name, param_name);

    /* Define the parent CDSGroup used to store the trans params */

    if (!_DSProc->trans_params) {

        _DSProc->trans_params = cds_define_group(NULL, "user_trans_params");
        if (!_DSProc->trans_params) {
            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }
    }

    trans_params = _DSProc->trans_params;

    /* Get or create the CDSGroup for this coordinate system */

    trans_coordsys = cds_get_group(trans_params, coordsys_name);

    if (!trans_coordsys) {

        trans_coordsys = cds_define_group(trans_params, coordsys_name);
        if (!trans_coordsys) {
            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }
    }

    /* Set the transformation parameter */

    status = cds_set_transform_param(
        trans_coordsys, field_name, param_name, type, length, value);

    if (status <= 0) return(0);
    return(1);
}
