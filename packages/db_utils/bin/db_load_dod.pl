#!/usr/bin/env perl
################################################################################
#
#  COPYRIGHT (C) 2007 Battelle Memorial Institute.  All Rights Reserved.
#
################################################################################
#
#  Author:
#     name:  Brian Ermold
#     phone: (509) 375-2277
#     email: brian.ermold@pnl.gov
#
################################################################################

use strict;

use DODUtils;
use DBCORE;

use Getopt::Long;
Getopt::Long::Configure qw(no_ignore_case);

use vars qw($opt_a $opt_h $opt_help $opt_q $opt_u $opt_v);

our $gVersion = '@VERSION@';
our @gCommand = split(/\//,$0);
our $gCommand = $gCommand[$#gCommand];

our %gConnArgs = (
    'DBAlias'     => 'dsdb_data',
    'Component'   => 'dsdb',
    'ObjectGroup' => 'dod',
);

our $gDB;
our $gVarProps;

################################################################################
#
#  Usage Subroutine
#

sub exit_usage()
{
print STDOUT <<EOF;

USAGE:

  $gCommand [-a alias] file

    [-h|-help] => Display this help message.
    [-v]       => Print version.

    [-a alias] => The database alias specified in the users .db_connect file.
                  (default: $gConnArgs{'DBAlias'})

    [-q]       => Quiet. Does not complain if the DOD already exists.

    [-u]       => Update an existing DOD in the database.

    file       => The DOD definition file

EOF
  exit(1);
}

################################################################################
#
#  Database Subroutines
#

sub cache_and_delete_var_props($$$)
{
    #inputs: dsc_name, dsc_level, dod_version
    my (@sp_args) = @_;
    my $retval;

    $gVarProps = $gDB->sp_call('dodvar_property_values', 'get_by_dod', @sp_args);

    unless (defined($gVarProps)) {
        print($gDB->error());
        return(0);
    }

    $retval = $gDB->sp_call('dodvar_property_values', 'delete_by_dod', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub delete_dod_version_definition($$$)
{
    #inputs: dsc_name, dsc_level, dod_version
    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('dod_versions', 'delete_definition', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub define_dod_version($$$)
{
    #inputs: dsc_name, dsc_level, dod_version
    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('dod_versions', 'define', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub define_dod_dim($$$$$$)
{
    #inputs: dsc_name, dsc_level, dod_version
    #        dim_name, dim_length, dim_order
    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('dod_dims', 'define', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub define_dod_att($$$$$$$)
{
    #inputs: dsc_name, dsc_level, dod_version
    #        att_name, att_type, att_value, att_order
    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('dod_atts', 'define', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub define_dod_var($$$$$$)
{
    #inputs: dsc_name, dsc_level, dod_version
    #        var_name, var_type, var_order
    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('dod_vars', 'define', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub define_dod_var_dim($$$$$$)
{
    #inputs: dsc_name, dsc_level, dod_version
    #        var_name, dim_name, var_dim_order
    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('dod_var_dims', 'define', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub define_dod_var_att($$$$$$$$)
{
    #inputs: dsc_name, dsc_level, dod_version
    #        var_name, att_name, att_type, att_value, var_att_order
    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('dod_var_atts', 'define', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub define_dod_var_props()
{
    my $prop;
    my $retval;

    return(1) unless ($gVarProps && @{$gVarProps});

    foreach $prop (@{$gVarProps}) {

        # I have no idea where this is coming from
        next if ($prop->{'var_name'} eq 'ui_state');

        $retval = $gDB->sp_call('dodvar_property_values', 'define',
            $prop->{'dsc_name'},
            $prop->{'dsc_level'},
            $prop->{'dod_version'},
            $prop->{'var_name'},
            $prop->{'prop_key'},
            $prop->{'prop_value'});

        unless (defined($retval)) {
            print($gDB->error());
            return(0);
        }
    }

    return(1);
}

################################################################################
#
#  Main Subroutines
#

sub db_load_dod($)
{
    my ($file) = @_;
    my $dod;
    my $dsc_name;
    my $dsc_level;
    my $dod_version;
    my $dim;
    my $att;
    my $att_type;
    my $var;
    my $dim_order;
    my $att_order;
    my $var_order;
    my $dim_name;

    # Load the DOD Hash Structure

    unless ($dod = load_dod($file)) {
        print(DODUtilsError());
        exit(1);
    }

    ($dsc_name,$dsc_level) = split(/\./, $dod->{'ds_class'});
    $dod_version = $dod->{'dod_version'};

    # Check if the DOD version exists #########################################

    my $retval = $gDB->sp_call(
        'dod_versions', 'inquire', $dsc_name, $dsc_level, $dod_version);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    if (@{$retval}[0]) {
        
        if ($opt_u) {

            return(0) unless (cache_and_delete_var_props(
                $dsc_name, $dsc_level, $dod_version));

            return(0) unless (delete_dod_version_definition(
                $dsc_name, $dsc_level, $dod_version));
        }
        else {
            unless ($opt_q) {
                my $version = "$dsc_name-$dsc_level-$dod_version";
                print("DOD version is already defined: '$version'\n");
            }
            return(1);
        }
    }
    else {
        unless (define_dod_version($dsc_name, $dsc_level, $dod_version)) {
            return(0);
        }
    }

    # Define Dimensions #######################################################

    $dim_order = 1;
    foreach $dim (@{$dod->{'dims'}}) {

        if ($dim->{'is_unlimited'}) {

            unless (define_dod_dim(
                $dsc_name, $dsc_level, $dod_version,
                $dim->{'name'}, 0, $dim_order)) {

                return(0);
            }
        }
        else {
            unless (define_dod_dim(
                $dsc_name, $dsc_level, $dod_version,
                $dim->{'name'}, $dim->{'length'}, $dim_order)) {

                return(0);
            }
        }

        $dim_order++;
    }

    # Define Attributes #######################################################

    $att_order = 1;
    foreach $att (@{$dod->{'atts'}}) {

        $att_type = ($att->{'type'}) ? $att->{'type'} : 'char';

        unless (define_dod_att(
            $dsc_name, $dsc_level, $dod_version,
            $att->{'name'}, $att_type, $att->{'value'}, $att_order)) {

            return(0);
        }

        $att_order++;
    }

    # Define Variables #######################################################

    $var_order = 1;
    foreach $var (@{$dod->{'vars'}}) {

        unless (define_dod_var(
            $dsc_name, $dsc_level, $dod_version,
            $var->{'name'}, $var->{'type'}, $var_order)) {

            return(0);
        }

        $var_order++;

        # Define Variable Dimensions ##########################################

        if ($var->{'dims'}) {

            $dim_order = 1;
            foreach $dim_name (split(/\s*,\s*/, $var->{'dims'})) {

                unless (define_dod_var_dim(
                    $dsc_name, $dsc_level, $dod_version,
                    $var->{'name'},
                    $dim_name, $dim_order)) {

                    return(0);
                }

                $dim_order++;
            }
        }

        # Define Variable Attributes ##########################################

        $att_order = 1;
        foreach $att (@{$var->{'atts'}}) {

            $att_type = ($att->{'type'}) ? $att->{'type'} : 'char';

            unless (define_dod_var_att(
                $dsc_name, $dsc_level, $dod_version,
                $var->{'name'},
                $att->{'name'}, $att_type, $att->{'value'}, $att_order)) {

                return(0);
            }

            $att_order++;
        }
    }

    if ($opt_u && $gVarProps) {
        return(0) unless (define_dod_var_props());
    }

    return(1);
}

################################################################################
#
#  Main
#

unless (GetOptions("a=s", "h", "help", "q", "u", "v")) {
    exit_usage();
}

if ($opt_v) {
    print("$gVersion\n");
    exit(0);
}

if ($opt_h || $opt_help || !$ARGV[0]) {
    exit_usage();
}

# Command line options

if ($opt_a) {
    $gConnArgs{'DBAlias'} = $opt_a;
}

# Connect to the database

$gDB = DBCORE->new();

unless ($gDB->connect(\%gConnArgs)) {
    print($gDB->error());
    exit(1);
}

# Run the main subroutine

unless (db_load_dod($ARGV[0])) {
    $gDB->disconnect();
    exit(1);
}

# Disconnect from the database

$gDB->disconnect();

exit(0);
