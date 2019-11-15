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

use vars qw($opt_a $opt_h $opt_help $opt_v);
use vars qw($gVersion @gCommand $gCommand);
use vars qw($gDB %gConnArgs);

$gVersion = '@VERSION@';
@gCommand = split(/\//,$0);
$gCommand = $gCommand[$#gCommand];

$gConnArgs{'DBAlias'}     = 'dsdb_data';
$gConnArgs{'Component'}   = 'dsdb';
$gConnArgs{'ObjectGroup'} = 'dod';

################################################################################
#
#  Usage Subroutine
#

sub exit_usage()
{
print STDOUT <<EOF;

USAGE:

  $gCommand [-a alias] name level version

    [-h|-help] => Display this help message.
    [-v]       => Print version.

    [-a alias] => The database alias specified in the users .db_connect file.
                  (default: $gConnArgs{'DBAlias'})

    name       => Datastream Class Name
    level      => Datastream Class Level
    version    => DOD Version

EOF
  exit(1);
}

sub delete_dod_version($$$)
{
    #inputs: dsc_name, dsc_level, dod_version
    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('dod_versions', 'delete', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

################################################################################
#
#  Main
#

unless (GetOptions("a=s", "h", "help", "v")) {
    exit_usage();
}

if ($opt_v) {
    print("$gVersion\n");
    exit(0);
}

if ($opt_h || $opt_help || !$ARGV[2]) {
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

# Delete the specified DOD version

unless (delete_dod_version($ARGV[0], $ARGV[1], $ARGV[2])) {
    $gDB->disconnect();
    exit(1);
}

# Disconnect from the database

$gDB->disconnect();

exit(0);
