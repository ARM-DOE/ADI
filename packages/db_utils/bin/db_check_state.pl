#!/usr/bin/env perl
################################################################################
#
#  COPYRIGHT (C) 2013 Battelle Memorial Institute.  All Rights Reserved.
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
use Getopt::Long;

use DSDB;

my $gVersion      = '@VERSION@';
my @gCommand      = split(/\//,$0);
my $gCommand      = $gCommand[$#gCommand];

my $gExitSignals  = 'HUP INT QUIT BUS SEGV TERM ALRM';
my $gCaughtSignal = 0;

my $gSite         = '';
my $gFacility     = '';
my $gProcName     = '';
my $gProcType     = '';
my $gDBAlias      = 'dsdb_data';

my $gDSDB;

################################################################################
#
#  Exit Usage
#

sub exit_usage($)
{
    my ($status) = @_;

    print STDOUT <<EOF;

NAME

    $gCommand - Check the state of a process in the database.

SYNOPSIS

    $gCommand  site fac type name

ARGUMENTS

    site       The site name.
    fac        The facility name.
    type       The type of the process (Ingest, Bundle, etc...)
    name       The name of the process (sonde, mwr3c, etc...)

OPTIONS

    -a alias   Specify the .db_connect alias to use (default: dsdb_data).

    -help      Display help message.
    -version   Display software version.

EXIT VALUES

    0 = enabled
    1 = disabled
    2 = undefined or an error occurred

EOF

    exit(1);
}

################################################################################
#
#  Database Subroutines
#

sub db_connect()
{
    my $db_error;

    if ($gDSDB) {

        return(1) if ($gDSDB->is_connected());

        unless ($gDSDB->reconnect()) {
            $db_error = $gDSDB->error();
            print(STDERR "check_db_state error: ", $db_error);
            return(0);
        }
    }
    else {
        $gDSDB = DSDB->new();

        unless ($gDSDB->connect($gDBAlias)) {
            $db_error = $gDSDB->error();
            print(STDERR "check_db_state error: ", $db_error);
            undef($gDSDB);
            return(0);
        }
    }

    return(1);
}

sub db_disconnect()
{
    if ($gDSDB) {
        $gDSDB->disconnect();
    }
}

sub db_init_process_methods($$$$)
{
    my ($site,$fac,$proc_type,$proc_name) = @_;
    my $db_error;

    # Connect to the database if a connection has not already been
    # established.

    return(0) unless (db_connect());

    unless ($gDSDB->init_process_methods(
        $site, $fac, $proc_type, $proc_name)) {

        $db_error = $gDSDB->error();
        print(STDERR "check_db_state error: ", $db_error);
        return(0);
    }

    return(1);
}

sub db_is_process_enabled($$$$)
{
    my ($site,$fac,$proc_type,$proc_name) = @_;
    my $is_enabled;
    my $db_error;

    return(undef) unless (db_init_process_methods(
        $site, $fac, $proc_type, $proc_name));

    $is_enabled = $gDSDB->is_family_process_enabled();

    unless (defined($is_enabled)) {

        $db_error = $gDSDB->error();

        if ($db_error) {
            print(STDERR "check_db_state error: ", $db_error);
            db_disconnect();
            return(undef);
        }
    }

    return($is_enabled);
}

################################################################################
#
#  Main
#

sub signal_handler($)
{
    my ($signal) = @_;

    print(
    "Caught signal SIG$signal: Completing current operation and exiting...\n");

    $gCaughtSignal = $signal;
}

# Get command line options

exit_usage(2) unless (GetOptions(
    "a=s"     => \$gDBAlias,
    "help"    => sub { exit_usage(1); },
    "version" => sub { print("$gVersion\n"); exit(1); },
));

exit_usage(2) unless ($ARGV[3]);

$gSite     = $ARGV[0];
$gFacility = $ARGV[1];
$gProcType = $ARGV[2];
$gProcName = $ARGV[3];

# Initialize the signal handlers

foreach (split(/\s+/,$gExitSignals)) {
    $SIG{$_} = \&signal_handler;
}

# Get the process state

my $is_enabled = db_is_process_enabled(
    $gSite, $gFacility, $gProcType, $gProcName);

exit(2) unless (defined($is_enabled));
exit(1) unless ($is_enabled);
exit(0);
