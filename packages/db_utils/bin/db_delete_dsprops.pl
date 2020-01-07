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

our $gVersion = '@VERSION@';
our @gCommand = split(/\//,$0);
our $gCommand = $gCommand[$#gCommand];

our $gQuiet   = 0;

our $gDB;
our %gConnArgs = (
    'DBAlias'     => 'dsdb_data',
    'Component'   => 'dsdb',
    'ObjectGroup' => 'dod',
);

our %gDSPROPS;

################################################################################
#
#  Exit Usage
#

sub exit_usage($)
{
    my ($status) = @_;

    print STDOUT <<EOF;

NAME

    $gCommand - Delete a DSPROPs definition file from the database.

SYNOPSIS

    $gCommand file

ARGUMENTS

    file       => The DSPROPs definition file

OPTIONS

    [-a alias] => The database alias as defined in the .db_connect file
                  (default: $gConnArgs{'DBAlias'})

    [-q]       => Do not complain about datastreams that have not been 
                  defined in the database.

    [-h|help]  => Display help message.
    [-v]       => Display software version.

EOF

    exit($status);
}

################################################################################
#
#  Database Subroutines
#

sub define_ds_property($$$$$$$$)
{
    #inputs: dsc_name, dsc_level, site, fac, var_name,
    #        ds_prop_name, ds_prop_time, ds_prop_value

    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('ds_properties', 'define', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub delete_ds_property($$$$$$$)
{
    #inputs: dsc_name, dsc_level, site, fac, var_name,
    #        ds_prop_name, ds_prop_time

    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('ds_properties', 'delete', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub is_datastream_class_defined($$)
{
    #inputs: dsc_name, dsc_level

    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('datastream_classes', 'inquire', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(-1);
    }

    return(0) unless (@{$retval});
    return(1);
}

################################################################################
#
#  Main Subroutines
#

sub load_dsprops_hash($)
{
    my ($file) = @_;
    my $retval;

    %gDSPROPS = ();

    unless ($retval = do $file) {
        if ($@) {
            print("Could not compile definition file: $@\n -> $file\n");
        }
        elsif (!defined($retval)) {
            print("Could not read definition file: $!\n -> $file\n");
        }
        else {
            print("Bad return value loading definition file:\n -> $file\n");
        }
        return(undef);
    }

    return(\%gDSPROPS);
}

sub db_delete_dsprops($)
{
    my ($file) = @_;
    my $dsprops;
    my ($key, $ds_class, $dsc_name, $dsc_level);
    my ($sitefac, $site, $facs, $fac);
    my ($time, $var, $name, $value);
    my ($retval, $hash);

    # Load the DSDODS Hash Structure

    $dsprops = load_dsprops_hash($file);
    unless ($dsprops) {
        return(0);
    }

    # Create single hash for each site and facility.

    foreach $key (sort(keys(%{$dsprops}))) {
    foreach $ds_class (split(/\s*,\s*/, $key)) {

        ($dsc_name, $dsc_level) = split(/\./, $ds_class);

        $retval = is_datastream_class_defined($dsc_name, $dsc_level);

        next if ($retval < 0);
        if ($retval == 0) {
            unless ($gQuiet) {
                print("Not defined in database: $ds_class\n");
            }
            next;
        }

        foreach $sitefac (sort(keys(%{$dsprops->{$key}}))) {

            if ($sitefac) {

                ($site, $facs) = split(/\./, $sitefac);

                foreach $fac  (split(/\s*,\s*/, $facs)) {
                foreach $time (sort(keys(%{$dsprops->{$key}{$sitefac}}))) {
                foreach $var  (sort(keys(%{$dsprops->{$key}{$sitefac}{$time}}))) {
                foreach $name (sort(keys(%{$dsprops->{$key}{$sitefac}{$time}{$var}}))) {

                    $value = $dsprops->{$key}{$sitefac}{$time}{$var}{$name};

                    return (0) unless (delete_ds_property(
                        $dsc_name, $dsc_level, $site, $fac, $var,
                        $name, $time));
                }}}}
            }
            else {
            
                $site = undef;
                $fac  = undef;

                foreach $time (sort(keys(%{$dsprops->{$key}{$sitefac}}))) {
                foreach $var  (sort(keys(%{$dsprops->{$key}{$sitefac}{$time}}))) {
                foreach $name (sort(keys(%{$dsprops->{$key}{$sitefac}{$time}{$var}}))) {

                    $time  = undef unless ($time);
                    $var   = undef unless ($var);
                    $value = $dsprops->{$key}{$sitefac}{$time}{$var}{$name};

                    return (0) unless (delete_ds_property(
                        $dsc_name, $dsc_level, $site, $fac, $var,
                        $name, $time));
                }}}
            }
        }
    }}

    return(1);
}

################################################################################
#
#  Main
#

# Get command line options

exit_usage(2) unless (GetOptions(
    "alias=s" => \$gConnArgs{'DBAlias'},
    "quiet"   => \$gQuiet,
    "help"    => sub { exit_usage(1); },
    "version" => sub { print("$gVersion\n"); exit(1); },
));

exit_usage(1) unless ($ARGV[0]);

# Connect to the database

$gDB = DBCORE->new();

unless ($gDB->connect(\%gConnArgs)) {
    print($gDB->error());
    exit(1);
}

unless($gDB->load_defs('dsdb', 'dsdb')) {
    return(0);
}

# Run the main subroutine

unless (db_delete_dsprops($ARGV[0])) {
    $gDB->disconnect();
    exit(1);
}

# Disconnect from the database

$gDB->disconnect();

exit(0);
