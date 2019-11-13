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

    $gCommand - Export a DSATTS definition table from the database.

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

sub inquire_ds_atts($$)
{
    my ($name, $level) = @_;
    my (@sp_args) = ('%', '%', $name, $level, '%', '%', '%');
    my $dbres;

    #inputs:  site, facility, dsc_name, dsc_level,
    #         att_name, att_type, att_value
    #
    #returns: site, facility, dsc_name, dsc_level,
    #         att_name, att_type, att_value

    $dbres = $gDB->sp_call('ds_atts', 'inquire', @sp_args);

    unless (defined($dbres)) {
        print($gDB->error());
    }

    return($dbres);
}

sub inquire_ds_time_att($$)
{
    my ($name, $level) = @_;
    my (@sp_args) = ('%', '%', $name, $level, '%', '%', '%');
    my $dbres;

    #inputs:  site, facility, dsc_name, dsc_level,
    #         att_name, att_type, att_value
    #
    #returns: site, facility, dsc_name, dsc_level,
    #         att_name, att_time, att_type, att_value

    $dbres = $gDB->sp_call('ds_time_atts', 'inquire', @sp_args);

    unless (defined($dbres)) {
        print($gDB->error());
    }

    return($dbres);
}

sub inquire_ds_var_att($$)
{
    my ($name, $level) = @_;
    my (@sp_args) = ('%', '%', $name, $level, '%', '%', '%', '%');
    my $dbres;

    #inputs:  site, facility, dsc_name, dsc_level, var_name,
    #         att_name, att_type, att_value
    #
    #returns: site, facility, dsc_name, dsc_level, var_name,
    #         att_name, att_type, att_value

    $dbres = $gDB->sp_call('ds_var_atts', 'inquire', @sp_args);

    unless (defined($dbres)) {
        print($gDB->error());
    }

    return($dbres);
}

sub inquire_ds_var_time_att($$)
{
    my ($name, $level) = @_;
    my (@sp_args) = ('%', '%', $name, $level, '%', '%', '%', '%');
    my $dbres;

    #inputs:  site, facility, dsc_name, dsc_level, var_name,
    #         att_name, att_type, att_value
    #
    #returns: site, facility, dsc_name, dsc_level, var_name,
    #         att_name, att_time, att_type, att_value

    $dbres = $gDB->sp_call('ds_var_time_atts', 'inquire', @sp_args);

    unless (defined($dbres)) {
        print($gDB->error());
    }

    return($dbres);
}

################################################################################
#
#  Main Subroutines
#

sub db_export_dsatts($$)
{
    my ($name, $level) = @_;
    my ($dbres, $row);
    my ($site, $fac, $var, $att, $time, $type, $value);

    my ($table, $sf);
    my $ds    = "$name.$level";
    my $found = 0;
    my %dsatts  = ();

    # Global Attributes

    $dbres = inquire_ds_atts($name, $level);
    return(0) unless (defined($dbres));

    if (@{$dbres}) {

        $table = 'global_atts';
        $found++;

        foreach $row (@{$dbres}) {

            $site  = $row->{'site'};
            $fac   = $row->{'facility'};
            $att   = $row->{'att_name'};
            $type  = $row->{'att_type'};
            $value = $row->{'att_value'};

            $sf = "$site.$fac";

            $dsatts{$ds}{$table}{$att}{$type}{$sf} = $value;
        }
    }

    # Global Time Attributes

    $dbres = inquire_ds_time_att($name, $level);
    return(0) unless (defined($dbres));

    if (@{$dbres}) {

        $table = 'global_time_atts';
        $found++;

        foreach $row (@{$dbres}) {

            $site  = $row->{'site'};
            $fac   = $row->{'facility'};
            $att   = $row->{'att_name'};
            $time  = $row->{'att_time'};
            $type  = $row->{'att_type'};
            $value = $row->{'att_value'};

            $sf = "$site.$fac";

            $dsatts{$ds}{$table}{$att}{$type}{$sf}{$time} = $value;
        }
    }

    # Variable Attributes

    $dbres = inquire_ds_var_att($name, $level);
    return(0) unless (defined($dbres));

    if (@{$dbres}) {

        $table = 'var_atts';
        $found++;

        foreach $row (@{$dbres}) {

            $site  = $row->{'site'};
            $fac   = $row->{'facility'};
            $var   = $row->{'var_name'};
            $att   = $row->{'att_name'};
            $type  = $row->{'att_type'};
            $value = $row->{'att_value'};

            $sf = "$site.$fac";

            $dsatts{$ds}{$table}{$var}{$att}{$type}{$sf} = $value;
        }
    }

    # Variable Time Attributes

    $dbres = inquire_ds_var_time_att($name, $level);
    return(0) unless (defined($dbres));

    if (@{$dbres}) {

        $table = 'var_time_atts';
        $found++;

        foreach $row (@{$dbres}) {

            $site  = $row->{'site'};
            $fac   = $row->{'facility'};
            $var   = $row->{'var_name'};
            $att   = $row->{'att_name'};
            $time  = $row->{'att_time'};
            $type  = $row->{'att_type'};
            $value = $row->{'att_value'};

            $sf = "$site.$fac";

            $dsatts{$ds}{$table}{$var}{$att}{$type}{$sf}{$time} = $value;
        }
    }

    # Dump hash table to STDOUT

    if ($found) {
        $Data::Dumper::Terse    = 1;
        $Data::Dumper::Indent   = 1;
        $Data::Dumper::Sortkeys = 1;
        print Dumper(\%dsatts);
    }
    else {
        print("No DSATTS defined for $name.$level\n");
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

unless (db_export_dsatts($name, $level)) {
    $gDB->disconnect();
    exit(1);
}

# Disconnect from the database

$gDB->disconnect();

exit(0);
