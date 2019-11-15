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
*    $Revision: 54278 $
*    $Author: ermold $
*    $Date: 2014-05-09 18:58:41 +0000 (Fri, 09 May 2014) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file cds_objects.c
 *  CDS Objects.
 */

#include "cds3.h"
#include "cds_private.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  PRIVATE: Free the memory used by the members of a CDS Object.
 *
 *  @param  cds_object - pointer to a CDSGroup, CDSDim, CDSAtt, or CDSVar
 */
void _cds_free_object_members(void *cds_object)
{
    CDSObject *object = (CDSObject *)cds_object;
    int        i;

    if (!object) return;

    if (object->obj_path) {
        free(object->obj_path);
    }

    if (object->user_data) {
        for (i = 0; object->user_data[i]; i++) {
            if (object->user_data[i]->free_value) {
                object->user_data[i]->free_value(object->user_data[i]->value);
            }
            free(object->user_data[i]->key);
            free(object->user_data[i]);
        }
        free(object->user_data);
    }

    free(object->name);
}

/**
 *  PRIVATE: Initialize the members of a CDS Object.
 *
 *  @param  cds_object - pointer to a CDSGroup, CDSDim, CDSAtt, or CDSVar
 *  @param  obj_type   - object type
 *  @param  parent     - pointer to the parent object
 *  @param  name       - object name
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int _cds_init_object_members(
    void          *cds_object,
    CDSObjectType  obj_type,
    void          *parent,
    const char    *name)
{
    CDSObject *object = (CDSObject *)cds_object;

    if (object) {
        object->obj_type = obj_type;
        object->obj_path = (char *)NULL;
        object->parent   = parent;
        object->name     = strdup(name);
    }

    return(1);
}

/**
 *  PRIVATE: Get a CDS Object from an array of CDS Objects.
 *
 *  @param  array    - pointer to an array of CDS Object pointers
 *  @param  nobjects - number of objects in the array
 *  @param  name     - name of the object to return
 *
 *  @return
 *    - pointer to the CDS Object
 *    - NULL if not found
 */
void *_cds_get_object(void **array, int nobjects, const char *name)
{
    CDSObject **objects = (CDSObject **)array;
    int         i;

    if (objects && nobjects && name) {
        for (i = 0; i < nobjects; i++) {
            if (strcmp(objects[i]->name, name) == 0) {
                return((void *)objects[i]);
            }
        }
    }

    return((void *)NULL);
}

/**
 *  PRIVATE: Get the name of a CDS Object Type.
 *
 *  @param  obj_type - object type
 *
 *  @return  name of the object type
 */
const char *_cds_obj_type_name(CDSObjectType obj_type)
{
    const char *name;

    switch (obj_type) {
        case CDS_GROUP:
            name = "group";
            break;
        case CDS_DIM:
            name = "dimension";
            break;
        case CDS_ATT:
            name = "attribute";
            break;
        case CDS_VAR:
            name = "variable";
            break;
        case CDS_VARGROUP:
            name = "vargroup";
            break;
        case CDS_VARARRAY:
            name = "vararray";
            break;
        default:
            name = "invalid";
            break;

    }

    return(name);
}

/**
 *  PRIVATE: Remove a CDS Object from an array of CDS Objects.
 *
 *  @param  array    - pointer to an array of CDS Object pointers
 *  @param  nobjects - pointer to the number of objects in the array
 *  @param  object   - pointer to the object to remove
 */
void _cds_remove_object(void **array, int *nobjects, void *object)
{
    int i;

    if (array && nobjects) {

        for (i = 0; i < *nobjects; i++) {
            if (array[i] == object) break;
        }

        for (i++; i <= *nobjects; i++) {
            array[i-1] = array[i];
        }

        (*nobjects)--;
    }
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Get the path of a CDS Object.
 *
 *  The memory used by the returned string must not be freed
 *  by the calling process.
 *
 *  @param  cds_object - pointer to a CDSDim, CDSAtt, CDSVar
 *                       CDSGroup, or CDSVarGroup
 *
 *  @return
 *    - path of the CDS Object
 *    - "MEM ERROR" if a memory allocation error occurred
 */
const char *cds_get_object_path(void *cds_object)
{
    CDSObject *object = (CDSObject *)cds_object;
    char      *path;
    size_t     name_length;
    size_t     path_length;
    size_t     sep_length;
    char      *sep;

    if (!object) {
        return("NULL_OBJECT");
    }

    /* Check if we have already created the path to this object */

    if (object->obj_path) {
        return(object->obj_path);
    }

    /* Figure out how much memory we will need */

    path_length = 0;

    while (object) {

        path_length += strlen(object->name);

        switch (object->obj_type) {
            case CDS_DIM:      path_length +=  8; break;
            case CDS_ATT:      path_length +=  8; break;
            case CDS_VAR:      path_length +=  8; break;
            case CDS_GROUP:    path_length +=  1; break;
            case CDS_VARGROUP: path_length += 13; break;
            case CDS_VARARRAY: path_length +=  1; break;
            default:           path_length +=  1; break;
        }

        object = object->parent;
    }

    path = (char *)malloc((path_length + 1) * sizeof(char));
    if (!path) {
        return("MEM ERROR");
    }

    path[path_length] = '\0';

    /* Create the path to this object */

    object = cds_object;

    while (object) {

        name_length  = strlen(object->name);
        path_length -= name_length;

        memcpy(&path[path_length], object->name, name_length);

        switch (object->obj_type) {
            case CDS_DIM:      sep_length = 8;  sep = "/_dims_/";      break;
            case CDS_ATT:      sep_length = 8;  sep = "/_atts_/";      break;
            case CDS_VAR:      sep_length = 8;  sep = "/_vars_/";      break;
            case CDS_GROUP:    sep_length = 1;  sep = "/";             break;
            case CDS_VARGROUP: sep_length = 13; sep = "/_vargroups_/"; break;
            case CDS_VARARRAY: sep_length = 1;  sep = "/";             break;
            default:           sep_length = 1;  sep = "/";             break;
        }

        path_length -= sep_length;

        memcpy(&path[path_length], sep, sep_length);

        object = object->parent;
    }

    object = cds_object;
    object->obj_path = path;

    return(path);
}

/**
 *  Set the definition lock for a CDS Object.
 *
 *  Setting the definition lock to a non-zero value will prevent the
 *  object from being updated or deleted. Objects that are locked
 *  will still be deleted if their parent object is deleted.
 *
 *  For groups this will prevent any dimensions, attributes, variables,
 *  or child groups from being added or removed.
 *
 *  For dimensions this will prevent the length from being changed
 *  unless it is an unlimited dimension.
 *
 *  For attributes this will prevent the value from being changed.
 *
 *  For variables this will prevent dimensions and attributes from
 *  being added or removed. It will also prevent the data type from
 *  being changed.  A locked variable will still be deleted if a
 *  dimension used by the variable is deleted.
 *
 *  @param  cds_object - pointer to a CDSGroup, CDSDim, CDSAtt, or CDSVar
 *  @param  value      - lock value (0 = unlocked, non-zero = locked)
 */
void cds_set_definition_lock(void *cds_object, int value)
{
    CDSObject *object = (CDSObject *)cds_object;

    if (object) {
        object->def_lock = value;
    }
}

/**
 *  Delete user defined data from a CDS Object.
 *
 *  @param  cds_object - pointer to a CDSGroup, CDSDim, CDSAtt, or CDSVar
 *  @param  key        - user defined key
 */
void cds_delete_user_data(void *cds_object, const char *key)
{
    CDSObject   *object = (CDSObject *)cds_object;
    CDSUserData *user_data;
    int          i;

    if (object && object->user_data && key) {

        for (i = 0; object->user_data[i]; i++) {

            user_data = object->user_data[i];

            if (strcmp(user_data->key, key) == 0) {

                if (user_data->free_value) {
                    user_data->free_value(user_data->value);
                }

                free(user_data->key);
                free(user_data);

                for (i++; object->user_data[i]; i++) {
                    object->user_data[i-1] = object->user_data[i];
                }

                object->user_data[i-i] = (CDSUserData *)NULL;

                break;
            }
        }
    }
}

/**
 *  Get user defined data from a CDS Object.
 *
 *  @param  cds_object - pointer to a CDSGroup, CDSDim, CDSAtt, or CDSVar
 *  @param  key        - user defined key
 *
 *  @return
 *    - the user defined value
 *    - NULL if the key was not found
 */
void *cds_get_user_data(void *cds_object, const char *key)
{
    CDSObject *object = (CDSObject *)cds_object;
    int        i;

    if (object && object->user_data && key) {
        for (i = 0; object->user_data[i]; i++) {
            if (strcmp(object->user_data[i]->key, key) == 0) {
                return(object->user_data[i]->value);
            }
        }
    }

    return((void *)NULL);
}

/**
 *  Attach user defined data to a CDS Object.
 *
 *  This function can be used to attach application specific data
 *  to a CDS Object. If the specified key already exists in the
 *  object, the old value will be replaced by the new one.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  cds_object  - pointer to a CDSGroup, CDSDim, CDSAtt, or CDSVar
 *  @param  key         - user defined key
 *  @param  value       - user defined value
 *  @param  free_value  - function to free the memory used by the value
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int cds_set_user_data(
    void       *cds_object,
    const char *key,
    void       *value,
    void      (*free_value)(void *))
{
    CDSObject    *object = (CDSObject *)cds_object;
    int           nkeys  = 0;
    CDSUserData **user_data;
    CDSUserData  *entry;

    /* Check if an entry with this key already exists */

    if (object->user_data) {

        for (; object->user_data[nkeys]; nkeys++) {

            entry = object->user_data[nkeys];

            if (strcmp(entry->key, key) == 0) {

                if (entry->free_value) {
                    entry->free_value(entry->value);
                }

                entry->value      = value;
                entry->free_value = free_value;

                return(1);
            }
        }
    }

    /* Increase size of user data array */

    user_data = (CDSUserData **)realloc(
        object->user_data, (nkeys + 2) * sizeof(CDSUserData *));

    if (!user_data) {

        ERROR( CDS_LIB_NAME,
            "Could not add user data '%s' to: %s"
            " -> memory allocation error\n",
            key, cds_get_object_path(object));

        return(0);
    }

    object->user_data = user_data;

    /* Allocate space for the new user data structure */

    user_data[nkeys] = (CDSUserData *)malloc(sizeof(CDSUserData));

    if (!user_data[nkeys]) {

        ERROR( CDS_LIB_NAME,
            "Could not add user data '%s' to: %s"
            " -> memory allocation error\n",
            key, cds_get_object_path(object));

        return(0);
    }

    /* Allocate space for the new user data key */

    user_data[nkeys]->key = (char *)malloc((strlen(key)+1) * sizeof(char));

    if (!user_data[nkeys]->key) {

        free(user_data[nkeys]);
        user_data[nkeys] = (CDSUserData *)NULL;

        ERROR( CDS_LIB_NAME,
            "Could not add user data '%s' to: %s"
            " -> memory allocation error\n",
            key, cds_get_object_path(object));

        return(0);
    }

    /* Set key and value in the new user data structure */

    strcpy(user_data[nkeys]->key, key);

    user_data[nkeys]->value      = value;
    user_data[nkeys]->free_value = free_value;

    user_data[nkeys + 1] = (CDSUserData *)NULL;

    return(1);
}
