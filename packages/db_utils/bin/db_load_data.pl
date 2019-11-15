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

use DSDB;

use Getopt::Long;

use vars qw($opt_a $opt_delete $opt_h $opt_help $opt_v);
use vars qw($gVersion @gCommand $gCommand);

use vars qw($gDBAlias $gDSDB $gComp);

$gVersion = '@VERSION@';
@gCommand = split(/\//,$0);
$gCommand = $gCommand[$#gCommand];

$gDBAlias  = 'dsdb_data';

sub exit_usage()
{
  print STDOUT <<EOF;

USAGE: $gCommand

    [-h|-help] => Show this usage message.
    [-v]       => Show version information.

    [-a db_alias] => The db_alias to use from the users .db_connect file.
    [-delete]     => Delete the data from the DSDB rather than load it.

    file          => The file containing the DBData hash

EOF
  exit(0);
}

sub myexit($)
{
    my ($exit_value) = @_;

    # Disconnect from the database and exit

    print("Disconnecting From The Database...\n");

    $gDSDB->disconnect();

    exit($exit_value);
}

############################################################################
#
#  Main
#

unless (GetOptions("a=s", "delete", "h", "help", "v")) {
    exit_usage();
}

if ($opt_h || $opt_help || !$ARGV[0]) { exit_usage(); }

if ($opt_v) {
    print("$gVersion\n");
    exit(0);
}

if ($opt_a) {
    $gDBAlias = $opt_a;
}

# Connect to the database

print("Connecting to Database...\n");

$gDSDB = DSDB->new();
$gDSDB->disable_autocommit();

unless ($gDSDB->connect($gDBAlias)) {
    print($gDSDB->error());
    exit(1);
}

print("Initializing DSDBOG Methods...\n");

unless ($gDSDB->init_dsdbog_methods()) {
    print($gDSDB->error());
    myexit(1);
}

if ($opt_delete) {
    print("Deleting $ARGV[0] Data...\n");

    unless ($gDSDB->process_data_file('delete', $ARGV[0])) {
        print($gDSDB->error());
        myexit(1);
    }
}
else {
    print("Loading $ARGV[0] Data...\n");

    unless ($gDSDB->process_data_file('define', $ARGV[0])) {
        print($gDSDB->error());
        myexit(1);
    }
}

$gDSDB->commit();

myexit(0);

