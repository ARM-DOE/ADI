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
*     email: brian.ermold@pnnl.gov
*
*******************************************************************************/

#include "cds3.h"
#include "cds3_factory.h"

int cds_factory_define_dims(CDSGroup *group, DimDef *defs)
{
    DimDef *def;
    CDSDim *dim;
    int     di;

    for (di = 0; defs[di].name; di++) {

        def = &defs[di];

        dim = cds_define_dim(group, def->name, def->length, def->is_unlimited);

        if (!dim) return(0);
    }

    return(1);
}

int cds_factory_define_atts(void *parent, AttDef *defs)
{
    AttDef *def;
    CDSAtt *att;
    int     ai;

    for (ai = 0; defs[ai].name; ai++) {

        def = &defs[ai];

        if (def->type == CDS_CHAR && def->length == 0) {
            if (def->value) {
                att = cds_define_att_text(parent, def->name, def->value);
            }
            else {
                att = cds_define_att_text(parent, def->name,
                    "%s attribute value", def->name);
            }
        }
        else {
            att = cds_define_att(
                parent, def->name, def->type, def->length, def->value);
        }

        if (!att) return(0);
    }

    return(1);
}

int cds_factory_define_vars(CDSGroup *group, VarDef *defs)
{
    VarDef *def;
    CDSVar *var;
    int     ndims;
    int     vi;

    for (vi = 0; defs[vi].name; vi++) {

        def = &defs[vi];

        ndims = 0;
        if (def->dim_names) {
            for (; def->dim_names[ndims]; ndims++);
        }

        var = cds_define_var(
            group, def->name, def->type, ndims, def->dim_names);

        if (!var) return(0);

        /* Define variable attributes */

        if (def->atts) {
            if (!cds_factory_define_atts(var, def->atts)) {
                return(0);
            }
        }

        /* Define variable data */

        if (def->nsamples) {

            if (!cds_put_var_data(
                var, 0, def->nsamples, def->type, def->data)) {

                return(0);
            }
        }
    }

    return(1);
}

CDSGroup *cds_factory_define_groups(CDSGroup *parent, GroupDef *def)
{
    CDSGroup *group;
    int       gi;

    group = cds_define_group(parent, def->name);

    if (!group) return(0);

    /* Define dimensions */

    if (def->dims) {
        if (!cds_factory_define_dims(group, def->dims)) {
            return(group);
        }
    }

    /* Define attributes */

    if (def->atts) {
        if (!cds_factory_define_atts(group, def->atts)) {
            return(group);
        }
    }

    /* Define variables */

    if (def->vars) {
        if (!cds_factory_define_vars(group, def->vars)) {
            return(group);
        }
    }

    /* Define subgroups */

    if (def->groups) {

        for (gi = 0; def->groups[gi].name; gi++) {

            if (!cds_factory_define_groups(group, &(def->groups[gi]))) {
                break;
            }
        }
    }

    return(group);
}
