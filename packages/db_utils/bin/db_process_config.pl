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
Getopt::Long::Configure qw(no_ignore_case);

use vars qw($gVersion @gCommand $gCommand);

use vars qw($opt_h $opt_help $opt_v);
use vars qw($opt_a $opt_s $opt_f $opt_n $opt_t);
use vars qw($opt_defkey $opt_delkey $opt_defval $opt_delval $opt_list);

use vars qw($gDSDB $gDBAlias);
use vars qw($gProcType $gProcName $gSite $gFac $gExitValue);

$gVersion = '@VERSION@';
@gCommand = split(/\//,$0);
$gCommand = $gCommand[$#gCommand];

$gDBAlias  = 'dsdb_data';
$gProcType = undef;
$gProcName = undef;
$gSite     = undef;
$gFac      = undef;

################################################################################

sub exit_usage()
{
print STDOUT <<EOF;

Usage:

  $gCommand

    [-h|-help]  => Display this help message.
    [-v]        => Print version.

    [-a alias]  => The database alias specified in the users .db_connect file.
                   If not specified the '$gDBAlias' entry will be used.

    [-t type]   => The process type  (i.e. Ingest)
    [-n name]   => The process name  (i.e. mfrsr)
    [-s site]   => The site name     (i.e. sgp)
    [-f fac]    => The facility name (i.e. C1)

    -defkey key       => Define the specified key
    -delkey key       => Delete the specified key

    -defval key value => Define the specified key value
    -delval key       => Delete the specified key value

    -list   key       => List all config entries for the specified key

EOF
  exit(1);
}

################################################################################
#
#  Main
#

unless (GetOptions(
    "a=s", "f=s", "h", "help", "n=s", "s=s", "t=s", "v",
    "defkey", "delkey", "defval", "delval", "list")) {

    exit_usage();
}

if ($opt_h || $opt_help || !defined($ARGV[0])) { exit_usage(); }
if ($opt_defval && !defined($ARGV[1])) { exit_usage(); }

if ($opt_v) {
    print("$gVersion\n");
    exit(0);
}

# Command line options

$gDBAlias  = $opt_a if ($opt_a);
$gProcType = $opt_t if ($opt_t);
$gProcName = $opt_n if ($opt_n);
$gSite     = $opt_s if ($opt_s);
$gFac      = $opt_f if ($opt_f);

# Connect to the database

$gDSDB = DSDB->new();
unless ($gDSDB->connect($gDBAlias)) {
    print($gDSDB->error());
    exit(1);
}

# Initialize process methods

unless ($gDSDB->init_process_methods($gSite, $gFac, $gProcType, $gProcName)) {
    print($gDSDB->error());
    $gDSDB->disconnect();
    exit(1);
}

# Perform the selected action

$gExitValue = 0;

if ($opt_defkey) {

    unless ($gDSDB->define_process_config_key($ARGV[0])) {
        print($gDSDB->error());
        $gExitValue = 1;
    }
}
elsif ($opt_delkey) {

    my $retval = $gDSDB->delete_process_config_key($ARGV[0]);

    unless (defined($retval)) {
        print($gDSDB->error());
        $gExitValue = 1;
    }
}
elsif ($opt_defval) {

    unless ($gDSDB->update_process_config_value($ARGV[0], $ARGV[1])) {
        print($gDSDB->error());
        $gExitValue = 1;
    }
}
elsif ($opt_delval) {

    my $retval = $gDSDB->delete_process_config_value($ARGV[0]);

    unless (defined($retval)) {
        print($gDSDB->error());
        $gExitValue = 1;
    }
}
elsif ($opt_list) {

    my $retval = $gDSDB->get_process_config_values($ARGV[0]);

    if ($retval) {
        my @hash_keys =
            $gDSDB->get_sp_return_hash('process_config_values', 'get');

        my $table = $gDSDB->hasharray_to_table(\@hash_keys, $retval);

        print($table);
    }
    else {
        print($gDSDB->error());
        $gExitValue = 1;
    }
}

# Disconnect from the database

$gDSDB->disconnect();

exit($gExitValue);
