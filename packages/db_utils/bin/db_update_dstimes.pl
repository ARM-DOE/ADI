#!/usr/bin/env perl
################################################################################
#
#  COPYRIGHT (C) 2008 Battelle Memorial Institute.  All Rights Reserved.
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
Getopt::Long::Configure qw(no_ignore_case);

use vars qw($opt_a $opt_h $opt_help $opt_v $opt_delete);
use vars qw($gVersion @gCommand $gCommand);
use vars qw($gDB %gConnArgs);

$gVersion = '@VERSION@';
@gCommand = split(/\//,$0);
$gCommand = $gCommand[$#gCommand];

$gConnArgs{'DBAlias'}     = 'dsdb_data';
$gConnArgs{'Component'}   = 'dsdb';
$gConnArgs{'ObjectGroup'} = 'dsdb';

################################################################################
#
#  Usage Subroutine
#

sub exit_usage()
{
print STDOUT <<EOF;

USAGE:

  $gCommand [-a alias] site fac name type dsc_name dsc_level begin end

    [-h|-help] => Display this help message.
    [-v]       => Print version.

    [-a alias] => The database alias specified in the users .db_connect file.
                  (default: $gConnArgs{'DBAlias'})

    [-delete]  => Delete the datastream times from the database.

    site       => Site
    fac        => Facility
    name       => Process Name
    type       => Process Type
    dsc_name   => Datastream Class Name
    dsc_level  => Datastream Class Level
    begin      => Begin Time (YYYYMMDD.hhmmss)
    end        => End Time   (YYYYMMDD.hhmmss)

    Specify 0 for the begin or end time to preserve the current
    value in the database. If both begin and end times are 0,
    nothing will be done and the current values listed in the
    database will be printed to STDOUT.

EOF
  exit(1);
}

################################################################################
#
#  Database Subroutines
#

sub delete_datastream_times($$$$$$)
{
    #inputs: proc_type, proc_name, dsc_name, dsc_level, site, fac
    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('process_output_datastreams', 'delete', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub get_datastream_times($$$$$$)
{
    #inputs: proc_type, proc_name, dsc_name, dsc_level, site, fac
    my (@sp_args) = @_;
    my $retval;
    my $begin;
    my $end;

    $retval = $gDB->sp_call('process_output_datastreams', 'get', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(undef);
    }

    unless (defined(@{$retval}[0])) {
        return(0, 0);
    }

    $begin = @{$retval}[0]->{'first_time'};
    $end   = @{$retval}[0]->{'last_time'};

    return($begin, $end);
}

sub update_datastream_times($$$$$$$$)
{
    #inputs: proc_type, proc_name, dsc_name, dsc_level, site, fac, begin, end
    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('process_output_datastreams', 'update', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

################################################################################
#
#  Main Subroutines
#

sub format_input_time($)
{
    my ($input) = @_;

    if ($input =~ /^(\d{4})(\d{2})(\d{2}).(\d{2})(\d{2})(\d{2}.*)$/) {
        return("$1-$2-$3 $4:$5:$6");
    }

    print("\nInvalid Input Time: $input\n\n");

    exit_usage();
}

sub db_update_dstimes(@)
{
    my ($site, $fac, $name, $type, $dsc_name, $dsc_level, $begin, $end) = @_;
    my ($db_begin, $db_end);
    my $retval;

    # Delete the times if -delete was specified

    if ($opt_delete) {
        return(delete_datastream_times(
            $type, $name, $dsc_name, $dsc_level, $site, $fac));
    }

    # Convert input time format to DB timestamp format

    $begin = format_input_time($begin) if $begin;
    $end   = format_input_time($end)   if $end;

    # Get the current begin and end times from the database

    unless ($begin && $end) {

        ($db_begin, $db_end) =
            get_datastream_times($type,$name,$dsc_name,$dsc_level,$site,$fac);

        if (!$begin) {
            if (!$end) {
                if (!$db_begin) {
                    print(
                        "\n" .
                        "No times have ever been recorded in the database\n" .
                        "\n");
                }
                else {
                    print(
                        "\n" .
                        "Times currently recorded in the database:\n" .
                        " - begin time: $db_begin\n" .
                        " - end time:   $db_end\n" .
                        "\n");
                }
                return(1);
            }
            $begin = $db_begin;
        }
        else {
            $end = $db_end;
        }
    }

    unless ($begin && $end) {
        print("\n");
        print("You must specify a begin time.\n") unless ($begin);
        print("You must specify an end time.\n") unless ($end);
        print("\n");
        return(0);
    }

    # Delete the current begin and end times from the database

    unless(delete_datastream_times(
        $type, $name, $dsc_name, $dsc_level, $site, $fac)) {

        return(0);
    }

    # Update the current begin and end times in the database

    unless(update_datastream_times(
        $type, $name, $dsc_name, $dsc_level, $site, $fac, $begin, $end)) {

        return(0);
    }

    return(1);
}

################################################################################
#
#  Main
#

unless (GetOptions("a=s", "h", "help", "v", "delete")) {
    exit_usage();
}

if ($opt_v) {
    print("$gVersion\n");
    exit(0);
}

exit_usage() if ($opt_h || $opt_help);
exit_usage() if (@ARGV < 6);

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

# Update datastream times

unless (db_update_dstimes(@ARGV)) {
    $gDB->disconnect();
    exit(1);
}

# Disconnect from the database

$gDB->disconnect();

exit(0);
