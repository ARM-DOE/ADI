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
use Data::Dumper;

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

    $gCommand - Export a DSDODS definition table from the database.

SYNOPSIS

    $gCommand name.level

ARGUMENTS

    name.level      Datastream class name and data level

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

sub db_export_dsdods($$)
{
    my ($name, $level) = @_;
    my ($dbres, $row);
    my ($site, $fac, $time, $version);

    my $sf;
    my $ds   = "$name.$level";
    my %dsdods = ();

    # Get the DSDODS from the database

    $dbres = inquire_ds_dods($name, $level);
    return(0) unless (defined($dbres));

    unless (@{$dbres}) {
        print("No DSDODS defined for $name.$level\n");
        return(1);
    }

    # Create the hash table
    
    foreach $row (@{$dbres}) {

        $site    = $row->{'site'};
        $fac     = $row->{'facility'};
        $time    = $row->{'dod_time'};
        $version = $row->{'dod_version'};

        $sf = "$site.$fac";

        $dsdods{$ds}{$sf}{$time} = $version;
    }

    # Dump hash table to STDOUT

    $Data::Dumper::Terse    = 1;
    $Data::Dumper::Indent   = 1;
    $Data::Dumper::Sortkeys = 1;
    print Dumper(\%dsdods);

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

my ($name, $level) = split(/\./, $ARGV[0]);
unless ($level) {
    print("Invalid argument: $ARGV[0]\n");
}

# Connect to the database

$gDB = DBCORE->new();

unless ($gDB->connect(\%gConnArgs)) {
    print($gDB->error());
    exit(1);
}

# Run the main subroutine

unless (db_export_dsdods($name, $level)) {
    $gDB->disconnect();
    exit(1);
}

# Disconnect from the database

$gDB->disconnect();

exit(0);
