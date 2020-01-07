#!/usr/bin/env perl
################################################################################
#
#  COPYRIGHT (C) 2018 Battelle Memorial Institute.  All Rights Reserved.
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

use DBCORE;
use Getopt::Long;

our $gVersion = '@VERSION@';
our @gCommand = split(/\//,$0);
our $gCommand = $gCommand[$#gCommand];

our $gDB;
our %gConnArgs = (
    'DBAlias'     => 'dsdb_data',
    'Component'   => 'dsdb',
    'ObjectGroup' => 'dod',
);

################################################################################
#
#  Exit Usage
#

sub exit_usage($)
{
    my ($status) = @_;

    print STDOUT <<EOF;

NAME

    $gCommand - Import a DSDODS definition table into the database.

SYNOPSIS

    $gCommand file

ARGUMENTS

    file            The DSDODs definition file.

OPTIONS

    -alias   alias  The database alias as defined in the .db_connect file
                    (default: $gConnArgs{'DBAlias'})

    -help           Display help message.
    -version        Display software version.

EOF

    exit($status);
}

################################################################################
#
#  Database Subroutines
#

sub define_ds_dod($$$$$$)
{
    #inputs: site, fac, dsc_name, dsc_level, dod_time, dod_version

    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('ds_dods', 'define', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub delete_ds_dod($$$$$)
{
    #inputs: site, fac, dsc_name, dsc_level, dod_time

    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('ds_dods', 'delete', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub inquire_ds_dods($$)
{
    my ($name, $level) = @_;
    my (@sp_args) = ('%', '%', $name, $level, '%');
    my $dbres;

    #inputs:  site, facility, dsc_name, dsc_level, dod_version
    #returns: site, facility, dsc_name, dsc_level, dod_time, dod_version
    $dbres = $gDB->sp_call('ds_dods', 'inquire', @sp_args);

    unless (defined($dbres)) {
        print($gDB->error());
    }

    return($dbres);
}

################################################################################
#
#  Main Subroutines
#

sub load_dsdods_file($)
{
    my ($file) = @_;
    my $fh;
    my $data;
    my $dsdods;

    unless (open $fh, '<', $file) {
        print("Could not open file: $file\n -> $!\n");
        return(undef);
    }

    $/ = undef;
    $data = <$fh>;
    close $fh;

    $dsdods = eval($data);

    if ($@) {
        print("Could not load file: $file\n -> $@\n");
        return(undef);
    }

    return($dsdods);
}

sub cleanup_old_dsdods($$$)
{
    my ($dsdods, $name, $level) = @_;
    my ($dbres, $row);
    my ($site, $fac, $time);
    my $sf;

    my $ds = "$name.$level";
    my %dsdods = ();

    # Get the DSDODS from the database

    $dbres = inquire_ds_dods($name, $level);
    return(0) unless (defined($dbres));
    return(1) unless (@{$dbres});

    # Delete entries from the database
    # that do not exist in the dsdods table.

    foreach $row (@{$dbres}) {

        $site = $row->{'site'};
        $fac  = $row->{'facility'};
        $time = $row->{'dod_time'};

        $sf = "$site.$fac";

        unless (exists($dsdods->{$ds}{$sf}{$time})) {
            return(0) unless (delete_ds_dod(
                $site, $fac, $name, $level, $time));
        }
    }

    return(1);
}

sub db_import_dsdods($)
{
    my ($file) = @_;
    my ($ds, $sf, $time, $version);
    my ($name, $level, $site, $fac);
    my $dsdods;

    # Load the DSDODS definition file

    $dsdods = load_dsdods_file($file);
    return(0) unless ($dsdods);

    # Import the DSDODS into the database

    foreach $ds (sort(keys(%{$dsdods}))) {
        
        ($name, $level) = split(/\./, $ds);

        # Cleanup old DSDOD entries that have been removed.

        unless (cleanup_old_dsdods($dsdods, $name, $level)) {
            return(0);
        }

        # Define the DSDOD entries in the database

        foreach $sf (sort(keys(%{$dsdods->{$ds}}))) {

            ($site, $fac) = split(/\./, $sf);

            foreach $time (sort(keys(%{$dsdods->{$ds}{$sf}}))) {

                $version = $dsdods->{$ds}{$sf}{$time};

                return (0) unless (define_ds_dod(
                    $site, $fac, $name, $level, $time, $version));
            }
        }
    }

    return(1);
}

################################################################################
#
#  Main
#

# Get command line options

exit_usage(2) unless (GetOptions(
    "alias=s" => \$gConnArgs{'DBAlias'},
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

# Run the main subroutine

unless (db_import_dsdods($ARGV[0])) {
    $gDB->disconnect();
    exit(1);
}

# Disconnect from the database

$gDB->disconnect();

exit(0);
