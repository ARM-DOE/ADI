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

/** @file dsproc_parse_args.c
*  DSProc Parse Arguments.
*/

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** @privatesection */

/*******************************************************************************
*  Static Data and Functions Visible Only To This Module
*/

/**
*  Command line string
*/
static char _CommandLine[8192];

/**
*  List of all internally defined command line options
*/
struct {
    char    short_opt;
    char   *long_opt;
} _DSProcOpts[] = 
{
    { 'a',  "alias"              },
    { 'b',  "begin"              },
    { 'd',  NULL                 },
    { 'e',  "end"                },
    { 'f',  "facility"           },
    { 'h',  "help"               },
    { 'n',  "name"               },
    { 's',  "site"               },
    { 'v',  "version"            },
    { 'D',  "debug"              },
    { 'F',  "force"              },
    { 'N',  "no-quicklook"       },
    { 'Q',  "quicklook-only"     },
    { 'R',  "reprocess"          },
    { '\0', "asynchronous"       },
    { '\0', "disable-db-updates" },
    { '\0', "disable-email"      },
    { '\0', "dynamic-dods"       },
    { '\0', "files"              },
    { '\0', "log-dir"            },
    { '\0', "log-file"           },
    { '\0', "log-id"             },
    { '\0', "max-runtime"        },
    { '\0', "max-warnings"       },
    { '\0', "output-csv"         },
    { '\0', "provenance"         },
    { '\0', "real-time"          },
    { '\0', NULL                 }
};

/**
*  User Defined Options
*/
typedef struct DSProcUserOpt DSProcUserOpt;
struct DSProcUserOpt {

    DSProcUserOpt  *next;
    char            short_opt;
    char           *long_opt;
    char           *arg_name;
    char           *opt_desc;
    int             found;
    char           *value;

};

static DSProcUserOpt *_DSProcUserOptions;

/**
*  Static: Free all memory used by a DSProcUserOpt strycture.
*
*  @param  opt - pointer to the user option
*/
static void _dsproc_free_user_option(DSProcUserOpt *opt)
{
    if (opt) {
        if (opt->long_opt) free(opt->long_opt);
        if (opt->arg_name) free(opt->arg_name);
        if (opt->opt_desc) free(opt->opt_desc);

        free(opt);
    }
}

/**
*  Static: Check if an argument is a valid command line option.
*
*  @param  arg - argument specified on the command line
*
*  @retval  1  argument is a command line option
*  @retval  0  argument is not a command line option
*/
static int _dsproc_is_option(const char *arg)
{
    DSProcUserOpt *opt;
    int ndashes;
    int check_short_opt = 0;
    int check_long_opt  = 0;
    int i;

    ndashes = 0;
    while (*arg == '-') {
        arg++;
        ndashes++;
        if (ndashes == 2) break;
    }

    if (!ndashes || *arg == '\0') return(0);

    if (ndashes == 1) {
        if (strlen(arg) == 1) {
            check_short_opt = 1;
        }
        else {
            return(0);
        }
    }
    else if (ndashes == 2) {
        check_long_opt = 1;
    }
    else {
        return(0);
    }

    /* Check if this is an internal or reserved option */

    for (i = 0; _DSProcOpts[i].short_opt || _DSProcOpts[i].long_opt; ++i) {

        if (_DSProcOpts[i].short_opt && check_short_opt &&
            _DSProcOpts[i].short_opt == *arg) {

            return(1);
        }

        if (_DSProcOpts[i].long_opt && check_long_opt &&
            strcmp(_DSProcOpts[i].long_opt, arg) == 0) {

            return(1);
        }
    }

    /* Check if this is a user specified option */

    for (opt = _DSProcUserOptions; opt; opt = opt->next) {

        if (opt->short_opt && check_short_opt &&
            opt->short_opt == *arg) {

            return(1);
        }

        if (opt->long_opt && check_long_opt &&
            strcmp(opt->long_opt, arg) == 0) {

            return(1);
        }
    }

    return(0);
}

/**
*  Static: Set the command line string.
*
*  This function will set the string to use for the command_line global
*  attribute value when new datasets are created. This value will only
*  be set in datasets that have the command_line attribute defined.
*
*  @param  argc - argc value from the main function
*  @param  argv - argv pointer from the main function
*/
static void _dsproc_set_command_line(int argc, char **argv)
{
    int   max_length = PATH_MAX - 5;
    char *command;
    int   length;
    int   i;

    if (!argc || !argv || !argv[0]) {
        _CommandLine[0] = '\0';
        return;
    }

    command = strrchr(argv[0], '/');

    if (!command) {
        strncpy(_CommandLine, argv[0], max_length);
    }
    else {
        strncpy(_CommandLine, ++command, max_length);
    }
    _CommandLine[max_length] = '\0';

    length = 0;

    for (i = 1; i < argc; i++) {

        length += strlen(argv[i]) + 1;

        if (length > max_length) {
            strcat(_CommandLine, " ...");
            break;
        }

        strcat(_CommandLine, " ");
        strcat(_CommandLine, argv[i]);
    }
}

/**
*  Static: Convert an input date string to seconds since 1970.
*
*  This function will verify the time is earlier than the current
*  time specified by the 'now' argument, and later than 1992 (the
*  earliest data collected by the ARM program). 
*
*  An error will be printed to stderr if the time is invalid.
*
*  @param  program_name - name of the program
*  @param  type         - 'begin' or 'end' to use in error messages
*  @param  now          - current time in seconds since 1970,
*                         or 0 to disable future time check.
*  @param  string       - input string in the form YYYYMMDD[.hh[mm[ss]]]
*
*  @retval  time  seconds since 1970
*  @retval  0     invalid time
*/
static time_t _dsproc_input_string_to_time(
    const char *program_name,
    const char *type,
    time_t      now,
    const char *string)
{
    int     year;
    int     month;
    int     day;
    int     hour;
    int     min;
    int     sec;
    int     count;
    time_t  secs1970;

    if (strchr(string, '.')) {
        count = sscanf(string, "%4d%2d%2d.%2d%2d%2d",
            &year, &month, &day, &hour, &min, &sec);
    }
    else { // depricated
        count = sscanf(string, "%4d%2d%2d%2d%2d%2d",
            &year, &month, &day, &hour, &min, &sec);
    }

    if (count < 6) sec  = 0;
    if (count < 5) min  = 0;
    if (count < 4) hour = 0;

    if (count < 3) {

        fprintf(stderr, "\n%s: Invalid %s time format: '%s'\n\n",
            program_name, type, string);
    
        return(0);
    }

    secs1970 = get_secs1970(year, month, day, hour, min, sec);

    // 694224000 = '1992-01-01 00:00:00'
    if (secs1970 < 694224000) {

        fprintf(stderr,
            "\n%s: Invalid %s time: '%s'\n"
            " -> time is less than earliest ARM data (1992)\n\n",
            program_name, type, string);

        return(0);
    }

    if (now && (secs1970 > now)) {

        fprintf(stderr,
            "\n%s: Invalid %s time: '%s'\n"
            " -> time is greater than the current time\n\n",
            program_name, type, string);

        return(0);
    }

    return(secs1970);
}

/**
 *  Static: Parse common short option.
 *
 *  @param  program_name - name of the program
 *  @param  nproc_names  - number of valid process names
 *  @param  c            - current switch character
 *  @param  argc         - pointer to command line argc
 *  @param  argv         - pointer to command line argv
 *
 *  @retval  1   found
 *  @retval  0   not found
 *  @retval  -1  error
 */
static int _dsproc_parse_common_short_option(
    const char *program_name,
    int         nproc_names,
    char        c,
    int        *argc,
    char     ***argv)
{
    DSProcUserOpt *user;
    int  intval;
    int  found;
    char *arg;

#define GET_NEXT_ARG \
    if (--(*argc) <= 0) goto MISSING_ARG; \
    arg = *++(*argv); \
    if (_dsproc_is_option(arg)) goto MISSING_ARG;

    found = 1;

    switch(c) {
    case 'a':
        GET_NEXT_ARG
        if (!(_DSProc->db_alias = strdup(arg)) ) {
            goto MEMORY_ERROR;
        }
        break;
    case 'f':
        GET_NEXT_ARG
        if (!(_DSProc->facility = strdup(arg))) {
            goto MEMORY_ERROR;
        }
        break;
    case 'n':
        if (nproc_names != 1) {
            GET_NEXT_ARG
            if (!(_DSProc->name = strdup(arg))) {
                goto MEMORY_ERROR;
            }
        }
        else {
            found = 0;
        }
        break;
    case 's':
        GET_NEXT_ARG
        if (!(_DSProc->site = strdup(arg))) {
            goto MEMORY_ERROR;
        }
        break;
    case 'v':
        fprintf(stdout,
            "%s version: %s\n",
            program_name, _DSProc->version);
        _dsproc_destroy();
        exit(0);
        break;
    case 'D':
        if ((*argc) > 1 && isdigit(*((*argv)+1)[0])) {
            intval = atoi(*++(*argv));
            *argc -= 1;
        }
        else {
            intval = 1;
        }
        msngr_set_debug_level(intval);
        break;
    case 'R':
        dsproc_set_reprocessing_mode(1);
        break;
    default:

        found = 0;

        for (user = _DSProcUserOptions; user; user = user->next) {

            if (!user->short_opt) continue;

            if (c == user->short_opt) {

                if (user->arg_name) {

                    GET_NEXT_ARG
                    user->value = strdup(arg);

                    if (!user->value) {
                        goto MEMORY_ERROR;
                    }
                }

                user->found = 1;
                found       = 1;
                break;
            }
        }
    }

    return(found);

MISSING_ARG:

    fprintf(stderr,
        "\n%s: Missing required argument for option: '-%c'\n"
        " -> run '%s -h' for help\n\n",
        program_name, c, program_name);

    return(-1);

MEMORY_ERROR:

    fprintf(stderr,
        "\n%s: Memory allocation error parsing command line\n\n",
        program_name);

    return(-1);
}

/**
 *  Static: Parse common long option.
 *
 *  @param  program_name - name of the program
 *  @param  nproc_names  - number of valid process names
 *  @param  argc         - pointer to command line argc
 *  @param  argv         - pointer to command line argv
 *
 *  @retval  1   found
 *  @retval  0   not found
 *  @retval  -1  error
 */
static int _dsproc_parse_common_long_option(
    const char *program_name,
    int         nproc_names,
    int        *argc,
    char     ***argv)
{
    DSProcUserOpt *user;
    int  intval;
    int  found;
    char *opt;
    char *arg;

#define GET_NEXT_ARG \
    if (--(*argc) <= 0) goto MISSING_ARG; \
    arg = *++(*argv); \
    if (_dsproc_is_option(arg)) goto MISSING_ARG;

    /* This prevents negative values from being specified as option values */
    // if (*arg == '-') goto MISSING_ARG;

    opt   = *(*argv);
    found = 1;

    if (strcmp(opt, "--alias") == 0) {
        GET_NEXT_ARG
        if (!(_DSProc->db_alias = strdup(arg)) ) {
            goto MEMORY_ERROR;
        }
    }
    else if (strcmp(opt, "--asynchronous") == 0) {
        dsproc_enable_asynchronous_mode();
    }
    else if (strcmp(opt, "--debug") == 0) {
        if ((*argc) > 1 && isdigit(*((*argv)+1)[0])) {
            intval = atoi(*++(*argv));
            *argc -= 1;
        }
        else {
            intval = 1;
        }
        msngr_set_debug_level(intval);
    }
    else if (strcmp(opt, "--disable_db_updates") == 0) {
        // old form of option left here for backward compatibility
        dsproc_disable_db_updates();
    }
    else if (strcmp(opt, "--disable-db-updates") == 0) {
        dsproc_disable_db_updates();
    }
    else if (strcmp(opt, "--disable-email") == 0) {
        dsproc_disable_mail_messages();
    }
    else if (strcmp(opt, "--dynamic-dods") == 0) {
        dsproc_set_dynamic_dods_mode(1);
    }
    else if (strcmp(opt, "--facility") == 0) {
        GET_NEXT_ARG
        if (!(_DSProc->facility = strdup(arg))) {
            goto MEMORY_ERROR;
        }
    }
    else if (strcmp(opt, "--log-dir") == 0) {
        GET_NEXT_ARG
        if (!dsproc_set_log_dir(arg)) {
            return(-1);
        }
    }
    else if (strcmp(opt, "--log-file") == 0) {
        GET_NEXT_ARG
        if (!dsproc_set_log_file(arg)) {
            return(-1);
        }
    }
    else if (strcmp(opt, "--log-id") == 0) {
        GET_NEXT_ARG
        if (!dsproc_set_log_id(arg)) {
            return(-1);
        }
    }
    else if (strcmp(opt, "--max-runtime") == 0) {
        GET_NEXT_ARG
        intval = atoi(arg);
        if (intval < 0) {
            fprintf(stderr,
                "\n%s: The maxmimum runtime must be greater than or equal to 0\n\n",
                program_name);
            return(-1);
        }
        dsproc_set_max_runtime(intval);
    }
    else if (strcmp(opt, "--max-warnings") == 0) {
        GET_NEXT_ARG
        intval = atoi(arg);
        if (intval < 0) {
            fprintf(stderr,
                "\n%s: The maxmimum number of warnings must be greater than or equal to 0\n\n",
                program_name);
            return(-1);
        }
        dsproc_set_max_warnings(intval);
    }
    else if (strcmp(opt, "--name") == 0) {
        if (nproc_names != 1) {
            GET_NEXT_ARG
            if (!(_DSProc->name = strdup(arg))) {
                goto MEMORY_ERROR;
            }
        }
        else {
            found = 0;
        }
    }
    else if (strcmp(opt, "--output-csv") == 0) {
        dsproc_set_output_format(DSF_CSV);
    }
    else if (strcmp(opt, "--provenance") == 0) {
        if (*argc > 1 && isdigit(*((*argv)+1)[0])) {
            intval = atoi(*++(*argv));
            *argc -= 1;
        }
        else {
            intval = 1;
        }
        msngr_set_provenance_level(intval);
    }
    else if (strcmp(opt, "--reprocess") == 0) {
        dsproc_set_reprocessing_mode(1);
    }
    else if (strcmp(opt, "--site") == 0) {
        GET_NEXT_ARG
        if (!(_DSProc->site = strdup(arg))) {
            goto MEMORY_ERROR;
        }
    }
    else if (strcmp(opt, "--version") == 0) {
        fprintf(stdout,
            "%s version: %s\n",
            program_name, _DSProc->version);
        _dsproc_destroy();
        exit(0);
    }
    else {

        found = 0;

        for (user = _DSProcUserOptions; user; user = user->next) {

            if (!user->long_opt) continue;

            if (strcmp(user->long_opt, opt + 2) == 0) {

                if (user->arg_name) {

                    GET_NEXT_ARG
                    user->value = strdup(arg);

                    if (!user->value) {
                        goto MEMORY_ERROR;
                    }
                }

                user->found = 1;
                found       = 1;
                break;
            }
        }
    }

    return(found);

MISSING_ARG:

    fprintf(stderr,
        "\n%s: Missing required argument for option: '%s'\n"
        " -> run '%s -h' for help\n\n",
        program_name, opt, program_name);

    return(-1);

MEMORY_ERROR:

    fprintf(stderr,
        "\n%s: Memory allocation error parsing command line\n\n",
        program_name);

    return(-1);
}

/**
 *  Static: Print Advanced Options.
 *
 */
static void _dsproc_print_advanced_options(FILE *output_stream, int type)
{
    fprintf(output_stream,
"\n"
"  --asynchronous        Enabling asynchronous processing mode will allow\n"
"                        multiple processes to be executed concurrently. This\n"
"                        option will:\n"
"\n"
"                        - disable the process lock file\n"
"                        - disable checks for chronological data processing\n"
"                        - disable overlap checks with previously processed data\n"
"                        - force new file to be created for every output dataset\n"
"\n"
"  --disable-db-updates  Disabling database updates will prevent the process\n"
"                        from storing runtime status information in the\n"
"                        database. This can be used to run processes that are\n"
"                        using a read-only database.\n"
"\n"
"  --disable-email       Disable email messages.\n"
"\n"
"  --dynamic-dods        Allows DODs to be automatically generated and/or\n"
"                        updated using the information from the input datasets\n"
"                        that are mapped to the output dataset.\n"
    );

    if (type == 1) { // ingest
        fprintf(output_stream,
"\n"
"  --files        files  Comma separated list of files to process.\n"
        );
    }

    fprintf(output_stream,
"\n"
"  --log-dir      dir    Overrides the default log file directory.\n"
"\n"
"  --log-file     file   Overrides the default log file name.\n"
"\n"
"  --log-id       Id     Replace timestamp in log file name with specified Id.\n"
"                        This option is designed to be used in conjunction with\n"
"                        --asynchronous. The suggested form of the Id should be\n"
"                        the time the processing was scheduled along with the\n"
"                        job Id number in the form YYYYMMDD.hhmmss.#####.\n"
"\n"
"  --max-runtime  time   The maximum allowed runtime for the process in seconds.\n"
"                        Specify 0 to allow the process to run indefinitely.\n"
"                        The default value on the production system is 3000\n"
"                        (50 minutes), but this can be changed in the database\n"
"                        for each process. On the development system the\n"
"                        defaults are 86400 (1 day) for Ingests and 0\n"
"                        (indefinitely) for VAPs.\n"
"\n"
"  --max-warnings num    Maximum number of warning messages to log per\n"
"                        processing segment. (default: 100)\n"
"\n"
"  --output-csv          Create an output CSV file instead of NetCDF. The CSV\n"
"                        file will only contain the single dimension time\n"
"                        varying fields. The column names in the output CSV file\n"
"                        will contain the variable names and units, but all\n"
"                        other metadata and variables will be lost.\n"
"\n"
"  --provenance   level  Enable provenance log file. This log file will contain\n"
"                        information similar to what is displayed in --debug\n"
"                        mode but wil be in a different format. The level should\n"
"                        be a number between 1 and 5, 1 being the least verbose.\n"
    );

    if (type == 2) { // vap
        fprintf(output_stream,
"\n"
"  --real-time   [time]  Enable real-time processing mode and set the time in\n"
"                        hours to wait for missing input data. When this option\n"
"                        is used the --begin and --end dates do not need to be\n"
"                        specified. (default: 72.0 hours)\n"
        );
    }
}

/**
 *  Static: Print User Defined Options.
 *
 */
static void _dsproc_print_user_options(FILE *output_stream)
{
    DSProcUserOpt *opt = _DSProcUserOptions;
    int   max_opt_len  = 0;
    int   max_arg_len  = 0;
    int   opt_len;
    int   arg_len;

    int   length;
    int   indent;
    char  opt_arg_format[32];
    char  opt_format[32];
    char  desc_format1[32];
    char  desc_format2[32];
    char *desc_format;
    char  buffer[256];
    char  *opt_desc;
    char  *nlp;

    /* Determine format settings */

    for (opt = _DSProcUserOptions; opt; opt = opt->next) {

        if (opt->arg_name) {

            opt_len = (opt->long_opt) ? strlen(opt->long_opt) + 2 : 0;
            if (opt->short_opt) {
                opt_len += (opt->long_opt) ? 4 : 2;
            }

            arg_len = strlen(opt->arg_name);

            if (max_arg_len < arg_len) {
                max_arg_len = arg_len;
            }

            if (max_opt_len < opt_len) {
                max_opt_len = opt_len;
            }
        }
    }

    /* Create formating strings */

    if (max_arg_len) {

        length = max_opt_len + max_arg_len;

        if (length < 20) {
            max_arg_len += (20 - length)/2;
            max_opt_len  = 20 - max_arg_len;
        }
    }
    else {
        max_opt_len = 20;
    }

    indent = max_opt_len + max_arg_len + 1;

    sprintf(opt_format, "\n  %%-%ds ", indent);

    sprintf(opt_arg_format,
        "\n  %%-%ds %%-%ds ",
        max_opt_len, max_arg_len);

    sprintf(desc_format1, "%%s%%s\n");
    sprintf(desc_format2, "  %%%ds %%s\n", indent);

    /* Loop over all user defined options */

    for (opt = _DSProcUserOptions; opt; opt = opt->next) {

        /* Create option(s) string */

        if (opt->short_opt) {
            if (opt->long_opt) {
                snprintf(buffer, 256, "-%c, --%s",
                    opt->short_opt, opt->long_opt);
            }
            else {
                snprintf(buffer, 256, "-%c", opt->short_opt);
            }
        }
        else if (opt->long_opt) {
            snprintf(buffer, 256, "--%s", opt->long_opt);
        }

        /* Print option and argument name */

        if (opt->arg_name) {
            fprintf(output_stream, opt_arg_format, buffer, opt->arg_name); 
        }
        else {
            fprintf(output_stream, opt_format, buffer); 
        }

        /* Print option description */

        opt_desc    = (opt->opt_desc) ? opt->opt_desc : "\n";
        desc_format = desc_format1;

        while ( (nlp = strchr(opt_desc, '\n')) ) {

            length = nlp - opt_desc;

            if (length > 255) length = 255;
            snprintf(buffer, length + 1, "%s", opt_desc);

            fprintf(output_stream, desc_format, "", buffer);

            opt_desc    = nlp + 1;
            desc_format = desc_format2;
        }

        if (*opt_desc != '\0') {
            fprintf(output_stream, desc_format, "", opt_desc);
        }
    }
}

/**
 *  Static: Print Ingest Process Usage.
 *
 *  @param  program_name - name of the Ingest program
 *  @param  nproc_names  - number of valid process names
 *  @param  proc_names   - list of valid process names
 *  @param  long_help    - display long options
 *  @param  exit_value   - program exit value
 */
static void _dsproc_ingest_exit_usage(
    const char  *program_name,
    int          nproc_names,
    const char **proc_names,
    int          long_help,
    int          exit_value)
{
    FILE       *output_stream  = (exit_value) ? stderr : stdout;
    const char *proc_name_arg  = "";
    const char *proc_name_desc = "";
    int i;

    fflush(stderr);
    fflush(stdout);
    
    if (nproc_names != 1) {
        proc_name_arg  = " -n proc_name";
        proc_name_desc =
        "\n"
        "  -n, --name      name  Process name as defined in the PCM database.\n";
    }

    fprintf(output_stream,
"\n"
"SYNOPSIS\n"
"\n"
"    %s%s -s site -f facility\n"
"\n"
"REQUIRED ARGUMENTS\n"
"%s"
"\n"
"  -s, --site      Id    ARM site identifier.\n"
"\n"
"  -f, --facility  Id    ARM facility identifier.\n"
"\n"
"STANDARD OPTIONS\n"
"\n"
"  -a, --alias    name   The name of the alias to use in the .db_connect file.\n"
"                        (default: dsdb_data).\n"
"\n"
"  -h, --help            Display usage messages. The long option form will also\n"
"                        display the advanced options.\n"
"\n"
"  -v, --version         Display process version.\n"
"\n"
"  -D, --debug    level  Enable debug mode. The level should be a number\n"
"                        between 1 and 5, 1 being the least verbose.\n"
"\n"
"  -F, --force           Force processing past errors that would normally cause\n"
"                        a process to fail and require operator intervention.\n"
"                        This option was designed to be used on local site\n"
"                        systems that have minimal operator support. It will\n"
"                        filter overlapping records that would otherwise cause\n"
"                        the process to fail, and will attempt to move problem\n"
"                        files out of the way so processing can continue.\n"
"\n"
"  -R, --reprocess       Enable reprocessing/development mode. Enabling this\n"
"                        mode will allow data to be processed that is earlier\n"
"                        than the latest processed data. It will also allow\n"
"                        existing files to be overwritten if the output file\n"
"                        has the same name as an existing file.\n"
"\n",
program_name, proc_name_arg, proc_name_desc);

    if (long_help) {
        fprintf(output_stream, "ADVANCED OPTIONS\n");
        _dsproc_print_advanced_options(output_stream, 1);
        fprintf(output_stream, "\n");
    }

    if (_DSProcUserOptions) {
        fprintf(output_stream, "APPLICATION SPECIFIC OPTIONS\n");
        _dsproc_print_user_options(output_stream);
        fprintf(output_stream, "\n");
    }

    if (nproc_names > 1) {

        fprintf(output_stream,
            "VALID PROCESS NAMES\n\n");

        for (i = 0; i < nproc_names; i++) {
            fprintf(output_stream,
                "    %s\n", proc_names[i]);
        }

        fprintf(output_stream, "\n");
    }

    exit(exit_value);
}

/**
 *  Static: Print VAP Process Usage.
 *
 *  @param  program_name - name of the VAP program
 *  @param  nproc_names  - number of valid process names
 *  @param  proc_names   - list of valid process names
 *  @param  long_help    - display long options
 *  @param  exit_value   - program exit value
 */
static void _dsproc_vap_exit_usage(
    const char  *program_name,
    int          nproc_names,
    const char **proc_names,
    int          long_help,
    int          exit_value)
{
    FILE       *output_stream  = (exit_value) ? stderr : stdout;
    const char *proc_name_arg  = "";
    const char *proc_name_desc = "";
    const char *date_args      = "";
    const char *date_args_desc = "";
    const char *date_opts_desc = "";
    int         rt_mode        = dsproc_get_real_time_mode();

    int has_quicklook_hook = dsproc_has_quicklook_function();
    int i;

    fflush(stderr);
    fflush(stdout);
    
    if (nproc_names != 1) {
        proc_name_arg  = " -n proc_name";
        proc_name_desc =
        "\n"
        "  -n, --name      name  Process name as defined in the PCM database.\n";
    }

    date_args      = " -b begin_date -e end_date";
    date_args_desc =
"\n"
"  -b, --begin    date   Begin date in the form YYYYMMDD.[hh[mm[ss]]].\n"
"\n"
"  -e, --end      date   End date in the form YYYYMMDD.[hh[mm[ss]]]. Data will\n"
"                        be processed up to but not including this date.\n";

    if (rt_mode) {
        date_opts_desc = date_args_desc;
        date_args      = "";
        date_args_desc = "";
    }

    fprintf(output_stream,
"\n"
"SYNOPSIS\n"
"\n"
"    %s%s -s site -f facility%s\n"
"\n"
"REQUIRED ARGUMENTS\n"
"%s"
"\n"
"  -s, --site      Id    ARM site identifier.\n"
"\n"
"  -f, --facility  Id    ARM facility identifier.\n"
"%s"
"\n"
"STANDARD OPTIONS\n"
"\n"
"  -a, --alias    name   The name of the alias to use in the .db_connect file.\n"
"                        (default: dsdb_data).\n"
"\n"
"  -h, --help            Display usage messages. The long option form will also\n"
"                        display the advanced options.\n"
"\n"
"  -v, --version         Display process version.\n"
"%s"
"\n"
"  -D, --debug    level  Enable debug mode. The level should be a number\n"
"                        between 1 and 5, 1 being the least verbose.\n"
"\n"
"  -R, --reprocess       Enable reprocessing/development mode. Enabling this\n"
"                        mode will allow data to be processed that is earlier\n"
"                        than the latest processed data. It will also allow\n"
"                        existing files to be overwritten if the output file\n"
"                        has the same name as an existing file.\n"
"\n",
program_name, proc_name_arg, date_args, proc_name_desc,
date_args_desc, date_opts_desc);

    if (has_quicklook_hook) {
    fprintf(output_stream,
"  -N, --no-quicklook    Do not run the quicklook function\n"
"\n"
"  -Q, --quicklook-only  Only run the quicklook function\n"
"\n");
    }

    if (long_help) {
        fprintf(output_stream, "ADVANCED OPTIONS\n");
        _dsproc_print_advanced_options(output_stream, 2);
        fprintf(output_stream, "\n");
    }

    if (_DSProcUserOptions) {
        fprintf(output_stream, "APPLICATION SPECIFIC OPTIONS\n");
        _dsproc_print_user_options(output_stream);
        fprintf(output_stream, "\n");
    }

    if (nproc_names > 1) {

        fprintf(output_stream,
            "VALID PROCESS NAMES\n\n");

        for (i = 0; i < nproc_names; i++) {
            fprintf(output_stream,
                "    %s\n", proc_names[i]);
        }

        fprintf(output_stream, "\n");
    }

    exit(exit_value);
}

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/**
 *  Get the command line string.
 *
 *  @return
 *    - command line string
 *    - NULL if not set
 */
const char *_dsproc_get_command_line(void)
{
    if (_CommandLine[0]) return((const char *)_CommandLine);
    return((const char *)NULL);
}

/**
 *  Parse the command line arguments for an Ingest process.
 *
 *  If an error occurs in this function the memory used by the internal
 *  _DSProc structure will be freed and the process will exit with value 1.
 *
 *  @param  argc        - command line argc
 *  @param  argv        - command line argv
 *  @param  nproc_names - number of valid process names
 *  @param  proc_names  - list of valid process names
 */
void _dsproc_ingest_parse_args(
    int          argc,
    char       **argv,
    int          nproc_names,
    const char **proc_names)
{
    const char  *program_name = argv[0];
    const char  *switches;
    char         c;
    char        *opt;
    char        *arg;
    int          found;
    int          ni;

    _dsproc_set_command_line(argc, argv);

    /************************************************************
    *  Parse command line arguments
    *************************************************************/

    while(--argc > 0) {

        opt = *++argv;

        if (opt[0] != '-') {

            fprintf(stderr,
                "%s: Ignoring Invalid Argument: '%s'\n",
                program_name, opt);

            continue;
        }

        if (opt[1] == '-') {

            /* Long options */

            found = 1;

            if (strcmp(opt, "--files") == 0) {

                if (--argc <= 0) goto MISSING_ARG;
                arg = *++argv;
                if (*arg == '-') goto MISSING_ARG;

                if (!_dsproc_set_input_file_list(arg)) {
                    found = -1;
                }
            }
            else if (strcmp(opt, "--help") == 0) {
                _dsproc_ingest_exit_usage(
                    program_name, nproc_names, proc_names, 1, 0);
            }
            else if (strcmp(opt, "--force") == 0) {
                dsproc_set_force_mode(1);
            }
            else {
                found = _dsproc_parse_common_long_option(
                    program_name, nproc_names, &argc, &argv);
            }

            if (found < 0) {
                goto EXIT_ERROR;
            }
            else if (found == 0) {

                if (argc > 1 && *(argv+1)[0] != '-') {
                    argc--;
                    argv++;
                    fprintf(stderr,
                        "%s: Ignoring Invalid Option: '%s %s'\n",
                        program_name, opt, *argv);
                }
                else {
                    fprintf(stderr,
                        "%s: Ignoring Invalid Option: '%s'\n",
                        program_name, opt);
                }
            }
        }
        else {

            /* Short options */

            switches = opt;
            opt      = NULL;

            while ((c = *++switches)) {

                found = 1;

                switch(c) {

                case 'h':
                    _dsproc_ingest_exit_usage(
                        program_name, nproc_names, proc_names, 0, 0);
                    break;
                case 'F':
                    dsproc_set_force_mode(1);
                    break;
                default:
                    found = _dsproc_parse_common_short_option(
                        program_name, nproc_names, c, &argc, &argv);
                }

                if (found < 0) {
                    goto EXIT_ERROR;
                }
                else if (found == 0) {

                    if (argc > 1 && *(argv+1)[0] != '-') {
                        argc--;
                        argv++;
                        fprintf(stderr,
                            "%s: Ignoring Invalid Option: '-%c %s'\n",
                            program_name, c, *argv);
                    }
                    else {
                        fprintf(stderr,
                            "%s: Ignoring Invalid Option: '-%c'\n",
                            program_name, c);
                    }
                }

            } // end while loop over switches
        }
    }

    /************************************************************
    *  Check for required fields
    *************************************************************/

    if (!_DSProc->site     ||
        !_DSProc->facility ||
        !_DSProc->name) {

        fprintf(stderr,
            "\n%s: Missing Required Argument(s)\n"
            " -> run '%s -h' for help\n\n",
            program_name, program_name);

        goto EXIT_ERROR;
    }

    /************************************************************
    *  Make sure we have a valid process name
    *************************************************************/

    if (nproc_names > 1) {

        for (ni = 0; ni < nproc_names; ni++) {
            if (strcmp(_DSProc->name, proc_names[ni]) == 0) {
                break;
            }
        }

        if (ni == nproc_names) {

            fprintf(stderr,
                "\n%s: Invalid Process Name: '%s'\n"
                " -> run '%s -h' for help\n\n",
                program_name, _DSProc->name, program_name);

            goto EXIT_ERROR;
        }
    }

    return;

MISSING_ARG:

    if (opt) {
        fprintf(stderr,
            "\n%s: Missing required argument for option: '%s'\n"
            " -> run '%s -h' for help\n\n",
            program_name, opt, program_name);
    }
    else {
        fprintf(stderr,
            "\n%s: Missing required argument for option: '-%c'\n"
            " -> run '%s -h' for help\n\n",
            program_name, c, program_name);
    }

    goto EXIT_ERROR;

EXIT_ERROR:

    _dsproc_destroy();
    exit(1);
}

/**
 *  Parse the command line arguments for a VAP process.
 *
 *  If an error occurs in this function the memory used by the internal
 *  _DSProc structure will be freed and the process will exit with value 1.
 *
 *  @param  argc        - command line argc
 *  @param  argv        - command line argv
 *  @param  nproc_names - number of valid process names
 *  @param  proc_names  - list of valid process names
 */
void _dsproc_vap_parse_args(
    int          argc,
    char       **argv,
    int          nproc_names,
    const char **proc_names)
{
    const char  *program_name = argv[0];
    const char  *switches;
    char         c;
    char        *opt;
    char        *arg;
    int          found;
    int          ni;

    float        fltval;
    time_t       secs1970;
    int          rt_mode;
    time_t       now;

    _dsproc_set_command_line(argc, argv);

    now = time(NULL);

    /************************************************************
    *  Parse command line arguments
    *************************************************************/

    while(--argc > 0) {

        opt = *++argv;

        if (opt[0] != '-') {

            fprintf(stderr,
                "%s: Ignoring Invalid Argument: '%s'\n",
                program_name, opt);

            continue;
        }

        if (opt[1] == '-') {

            /* Long options */

            found = 1;

            if (strcmp(opt, "--begin") == 0) {

                if (--argc <= 0) goto MISSING_ARG;
                arg = *++argv;
                if (*arg == '-') goto MISSING_ARG;

                secs1970 = _dsproc_input_string_to_time(
                    program_name, "begin", now, arg);

                if (secs1970 == 0) {
                    goto EXIT_ERROR;
                }

                _DSProc->cmd_line_begin = secs1970;
            }
            else if (strcmp(opt, "--end") == 0) {

                if (--argc <= 0) goto MISSING_ARG;
                arg = *++argv;
                if (*arg == '-') goto MISSING_ARG;

                secs1970 = _dsproc_input_string_to_time(
                    program_name, "end", 0, arg);
                    
                if (secs1970 == 0) {
                    goto EXIT_ERROR;
                }
                
                _DSProc->cmd_line_end = secs1970;
            }
            else if (strcmp(opt, "--help") == 0) {
                _dsproc_vap_exit_usage(
                    program_name, nproc_names, proc_names, 1, 0);
            }
            else if (strcmp(opt, "--no-quicklook") == 0) {
                dsproc_set_quicklook_mode(QUICKLOOK_DISABLE);
            }
            else if (strcmp(opt, "--real-time") == 0) {

                if (argc > 1 && isdigit(*(argv+1)[0])) {
                    fltval = (float)atof(*++argv);
                    argc--;
                }
                else {
                    /* default to three days max wait time */
                    fltval = 72.0;
                }

                dsproc_set_real_time_mode(1, fltval);
            }
            else if (strcmp(opt, "--quicklook-only") == 0) {
                dsproc_set_quicklook_mode(QUICKLOOK_ONLY);
            }
            else {
                found = _dsproc_parse_common_long_option(
                    program_name, nproc_names, &argc, &argv);
            }

            if (found < 0) {
                goto EXIT_ERROR;
            }
            else if (found == 0) {

                if (argc > 1 && *(argv+1)[0] != '-') {
                    argc--;
                    argv++;
                    fprintf(stderr,
                        "%s: Ignoring Invalid Option: '%s %s'\n",
                        program_name, opt, *argv);
                }
                else {
                    fprintf(stderr,
                        "%s: Ignoring Invalid Option: '%s'\n",
                        program_name, opt);
                }
            }
        }
        else {

            /* Short options */

            switches = opt;
            opt      = NULL;

            while ((c = *++switches)) {

                found = 1;

                switch(c) {

                case 'b':
                case 'd':
                    if (--argc <= 0) goto MISSING_ARG;
                    arg = *++argv;
                    if (*arg == '-') goto MISSING_ARG;

                    secs1970 = _dsproc_input_string_to_time(
                        program_name, "begin", now, arg);

                    if (secs1970 == 0) {
                        goto EXIT_ERROR;
                    }

                    _DSProc->cmd_line_begin = secs1970;
                    break;
                case 'e':
                    if (--argc <= 0) goto MISSING_ARG;
                    arg = *++argv;
                    if (*arg == '-') goto MISSING_ARG;

                    secs1970 = _dsproc_input_string_to_time(
                        program_name, "end", 0, arg);
                        
                    if (secs1970 == 0) {
                        goto EXIT_ERROR;
                    }
                    
                    _DSProc->cmd_line_end = secs1970;
                    break;
                case 'h':
                    _dsproc_vap_exit_usage(
                        program_name, nproc_names, proc_names, 0, 0);
                    break;
                case 'N':
                    dsproc_set_quicklook_mode(QUICKLOOK_DISABLE);
                    break;
                case 'Q':
                    dsproc_set_quicklook_mode(QUICKLOOK_ONLY);
                    break;
                default:
                    found = _dsproc_parse_common_short_option(
                        program_name, nproc_names, c, &argc, &argv);
                }

                if (found < 0) {
                    goto EXIT_ERROR;
                }
                else if (found == 0) {

                    if (argc > 1 && *(argv+1)[0] != '-') {
                        argc--;
                        argv++;
                        fprintf(stderr,
                            "%s: Ignoring Invalid Option: '-%c %s'\n",
                            program_name, c, *argv);
                    }
                    else {
                        fprintf(stderr,
                            "%s: Ignoring Invalid Option: '-%c'\n",
                            program_name, c);
                    }
                }

            } // end while loop over switches
        }
    }

    /************************************************************
    *  Check for required fields
    *************************************************************/

    rt_mode = dsproc_get_real_time_mode();

    if (!_DSProc->site     ||
        !_DSProc->facility ||
        !_DSProc->name     ||
        (!rt_mode && !_DSProc->cmd_line_begin)) {

        fprintf(stderr,
            "\n%s: Missing Required Argument(s)\n"
            " -> run '%s -h' for help\n\n",
            program_name, program_name);

        goto EXIT_ERROR;
    }

    if (_DSProc->cmd_line_begin && _DSProc->cmd_line_end &&
        _DSProc->cmd_line_begin >= _DSProc->cmd_line_end) {

        fprintf(stderr,
            "\n%s: Invalid begin and/or end time\n"
            " -> the begin time must be less than the end time\n\n",
            program_name);

        goto EXIT_ERROR;
    }

    /************************************************************
    *  Make sure we have a valid process name
    *************************************************************/

    if (nproc_names > 1) {

        for (ni = 0; ni < nproc_names; ni++) {
            if (strcmp(_DSProc->name, proc_names[ni]) == 0) {
                break;
            }
        }

        if (ni == nproc_names) {

            fprintf(stderr,
                "\n%s: Invalid Process Name: '%s'\n"
                " -> run '%s -h' for help\n\n",
                program_name, _DSProc->name, program_name);

            goto EXIT_ERROR;
        }
    }

    return;

MISSING_ARG:

    if (opt) {
        fprintf(stderr,
            "\n%s: Missing required argument for option: '%s'\n"
            " -> run '%s -h' for help\n\n",
            program_name, opt, program_name);
    }
    else {
        fprintf(stderr,
            "\n%s: Missing required argument for option: '-%c'\n"
            " -> run '%s -h' for help\n\n",
            program_name, c, program_name);
    }

    goto EXIT_ERROR;

EXIT_ERROR:

    _dsproc_destroy();
    exit(1);
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Free memory used by user defined command line options.
 *
 *  This function can be called after dsproc_main() returns to free all
 *  internal memory used by the user defined command line options.
 *
 * @see dsproc_setopt(), dsproc_getopt()
 */
void dsproc_freeopts(void)
{
    DSProcUserOpt *opt  = _DSProcUserOptions;
    DSProcUserOpt *opt_next;

    for (opt = _DSProcUserOptions; opt; opt = opt_next) {
        opt_next = opt->next;
        _dsproc_free_user_option(opt);
    }

    _DSProcUserOptions = (DSProcUserOpt *)NULL;
}

/**
 *  Get user defined command line option.
 *
 *  Check if a user defined option was set on the command line and get the
 *  value if applicable.
 *
 *  \b Example (see dsproc_setopt() for example that sets these options):
 *  \code
 *
 *      const char *strval;
 *      int         found;
 *  
 *      found = dsproc_getopt("o", &strval);
 *      if (found) {
 *          printf("Option '-o' or '--my-option' was set by user: '%s'\n",
 *              strval);
 *      }
 *  
 *      found = dsproc_getopt("p", &strval);
 *      if (found) {
 *          printf("Option '-p' was set by user.\n");
 *      }
 *  
 *      found = dsproc_getopt("long-only", &strval);
 *      if (found) {
 *          printf("Option '--long-only' was set by user.\n");
 *      }
 *
 *  \endcode
 *
 *  @param  option  The short or long option.
 *  @param  value   output: pointer to the value.
 *
 *  @retval  1  if the option was specified on the command line
 *  @retval  0  if the option was not specified on the command line
 *
 * @see dsproc_setopt(), dsproc_freeopts()
 */
int dsproc_getopt(
    const char  *option,
    const char **value)
{
    DSProcUserOpt *opt = _DSProcUserOptions;
    char           short_opt;

    while (*option == '-') ++option;

    *value    = (char *)NULL;
    short_opt = (strlen(option) == 1) ? option[0] : '\0';

    for (opt = _DSProcUserOptions; opt; opt = opt->next) {

        if (short_opt) {
            if (opt->short_opt &&
                opt->short_opt == short_opt) {

                if (opt->found) {
                    *value = opt->value;
                    return(1);
                }
            }
        }
        else if (opt->long_opt) {

            if (strcmp(opt->long_opt, option) == 0) {

                if (opt->found) {
                    *value = opt->value;
                    return(1);
                }
            }
        }
    }

    return(0);
}

/**
 *  Set user defined command line option.
 *
 *  This function must be called before calling dsproc_main.
 *
 *  \b Example (see dsproc_getopt() for example that gets these options):
 *  \code
 *
 *      if (!dsproc_setopt('o', "my-option", "value",
 *          "User defined option with both short and long option\n"
 *          "switches that also takes a user defined argument.")) {
 *          exit(1);
 *      }
 *  
 *      if (!dsproc_setopt('p', NULL, NULL,
 *          "User defined option with only a short option switch.\n")) {
 *          exit(1);
 *      }
 *  
 *      if (!dsproc_setopt('\0', "--long-only", NULL,
 *          "User defined option with only a long option switch.")) {
 *          exit(1);
 *      }
 *
 *  \endcode
 *
 *  @param  short_opt - Short options are single letters and are prefixed by a
 *                      single dash on the command line. Multiple short options
 *                      can be grouped behind a single dash. Specify NULL for
 *                      this argument if a short option should not be used.
 *  @param  long_opt  - Long options are prefixed by two consecutive dashes on
 *                      the command line. Specify NULL for this argument if a
 *                      long option should not be used.
 *  @param  arg_name  - A single word description of the option argument to be
 *                      used in the help message, or NULL if this option does
 *                      not take an argument on the command line.
 *  @param  opt_desc  - A brief description of the option to be used in the
 *                      help message.
 *
 *  @retval  1  if successful
 *  @retval  0  if the option has already been used,
 *              or a memory allocation error occurred
 *
 * @see dsproc_getopt(), dsproc_freeopts()
 */
int dsproc_setopt(
    const char    short_opt,
    const char   *long_opt,
    const char   *arg_name,
    const char   *opt_desc)
{
    DSProcUserOpt  *opt;
    DSProcUserOpt  *prev;
    const char     *strp;
    int             ndashes;
    int             i;

    if (!short_opt && !long_opt) {
        fprintf(stderr,
            "Could not add user defined option\n"
            " -> a short and/or long option must be specified\n");

        return(0);
    }

    /* Remove dashes from long_opt */

    if (long_opt) {

        ndashes = 0;
        strp = long_opt;
        while (*strp == '-') {
            strp++;
            ndashes++;
            if (ndashes == 2) break;
        }

        if (*strp == '\0') {
            fprintf(stderr,
                "Invalid user defined long option: '%s'\n",
                long_opt);
            return(0);
        }

        long_opt = strp;
    }

    /* Check if this is an internal or reserved option */

    for (i = 0; _DSProcOpts[i].short_opt || _DSProcOpts[i].long_opt; ++i) {

        if (_DSProcOpts[i].long_opt && long_opt &&
            strcmp(_DSProcOpts[i].long_opt, long_opt) == 0) {

            fprintf(stderr,
                "Could not add user defined option: '%s'\n"
                " -> option is already used, or reserved for future use\n",
                long_opt);

            return(0);
        }

        if (_DSProcOpts[i].short_opt && short_opt &&
            _DSProcOpts[i].short_opt == short_opt) {

            if (_DSProcOpts[i].long_opt) {
                fprintf(stderr,
                    "Could not add user defined option: '%c'\n"
                    " -> this is the short option for: '--%s\n"
                    " -> and is already used, or reserved for future use\n",
                    short_opt, _DSProcOpts[i].long_opt);
            }
            else {
                fprintf(stderr,
                    "Could not add user defined option: '%c'\n"
                    " -> option is already used, or reserved for future use\n",
                    short_opt);
            }

            return(0);
        }
    }

    /* Check if this user option has already been added */

    prev = NULL;

    for (opt = _DSProcUserOptions; opt; opt = opt->next) {

        if (opt->short_opt && short_opt &&
            opt->short_opt == short_opt) {

            fprintf(stderr,
                "Could not add user defined option: '%c'\n"
                " -> option has already already used\n",
                short_opt);

            return(0);
        }

        if (opt->long_opt && long_opt &&
            strcmp(opt->long_opt, long_opt) == 0) {

            fprintf(stderr,
                "Could not add user defined option: '%s'\n"
                " -> option has already already used\n",
                long_opt);

            return(0);
        }

        prev = opt;
    }

    opt = calloc(1, sizeof(DSProcUserOpt));
    if (!opt) {
        goto MEM_ALLOC_ERROR;
    }

    opt->short_opt = short_opt;
    opt->found     = 0;
    opt->value     = (char *)NULL;

    if (long_opt) {

        opt->long_opt = strdup(long_opt);
        if (!opt->long_opt) {
            goto MEM_ALLOC_ERROR;
        }
    }

    if (arg_name) {
        opt->arg_name = strdup(arg_name);
        if (!opt->arg_name) {
            goto MEM_ALLOC_ERROR;
        }
    }

    if (opt_desc) {
        opt->opt_desc = strdup(opt_desc);
        if (!opt->opt_desc) {
            goto MEM_ALLOC_ERROR;
        }
    }

    if (prev) {
        prev->next = opt;
    }
    else {
        _DSProcUserOptions = opt;
    }

    return(1);

MEM_ALLOC_ERROR:

    if (opt) {
        _dsproc_free_user_option(opt);
    }

    fprintf(stderr,
        "Memory allocation error setting user pption\n");

    return(0);
}
