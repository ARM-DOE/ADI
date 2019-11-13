#!/usr/bin/env perl
################################################################################
#
#  COPYRIGHT (C) 2005 Battelle Memorial Institute.  All Rights Reserved.
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

use vars qw($opt_a $opt_h $opt_help $opt_v);
use vars qw($opt_s $opt_f $opt_n $opt_t $opt_long);

use vars qw($gDSDB $gDBAlias);
use vars qw($gCat $gClass $gSite $gFac $gProcType $gProcName);
use vars qw($gSummary $gExitValue $gExitGood $gExitBad);

$gVersion  = '@VERSION@';
@gCommand  = split(/\//,$0);
$gCommand  = $gCommand[$#gCommand];

$gDBAlias  = 'dsdb_read';
$gCat      = '%';
$gClass    = '%';
$gSite     = '%';
$gFac      = '%';
$gProcType = '%';
$gProcName = '%';

$gSummary   = 1;

$gExitValue = 0;
$gExitGood  = 0;
$gExitBad   = 1;

################################################################################

sub exit_usage()
{
print STDOUT <<EOF;

USAGE:

  $gCommand [-a alias] [-s site] [-f fac] [-n name] [-t type] [-long]

    [-h|-help] => Display this help message.
    [-v]       => Print version.

    [-a alias] => The database alias specified in the users .db_connect file.
                  If not specified the '$gDBAlias' entry will be used.

    [-s site]  => The site name     (i.e. 'sgp')
    [-f fac]   => The facility name (i.e. 'C1')
    [-n name]  => The process name  (i.e. 'mfrsr')
    [-t type]  => The process type  (i.e. 'Ingest|Collection')

    [-long]    => Print the long listing of state and status information.

    Note: The site, fac, type, and name arguments can be SQL regular
    expresions and default to '%' if not specified.

EOF
  exit(1);
}

sub print_error($$) { print(STDERR "$gCommand:$_[0]\t$_[1]"); }

################################################################################

sub get_local_proc_stats()
{
    my ($category, $class, $site, $facility, $proc_name, $proc_type);
    my ($state_name, $state_text, $state_time);
    my ($status_name, $status_text);
    my ($started_time, $completed_time, $successful_time);
    my $retval;
    my $hash;

    my $i;
    my @hasharray;
    my @hash_keys;

    $retval = $gDSDB->inquire_local_family_process_stats($gCat, $gClass);
    unless ($retval) {
        print_error(__LINE__, $gDSDB->error());
        return($gExitBad);
    }

    # Get the process states.

    $i = 0;
    foreach $hash (@{$retval}) {

        $category        = $hash->{'category'};
        $class           = $hash->{'class'};
        $site            = $hash->{'site'};
        $facility        = $hash->{'facility'};
        $proc_name       = $hash->{'proc_name'};
        $proc_type       = $hash->{'proc_type'};

        $state_name      = $hash->{'state_name'};
        $state_text      = $hash->{'state_text'};
        $state_time      = $hash->{'state_time'};
        $status_name     = $hash->{'status_name'};
        $status_text     = $hash->{'status_text'};
        $started_time    = $hash->{'started_time'};
        $completed_time  = $hash->{'completed_time'};
        $successful_time = $hash->{'successful_time'};

        $state_time =~ s/\.\d+$//;

        chomp($state_text);
        chomp($status_text);

        if ($gSummary) {

            $hasharray[$i]{'site'}   = $site;
            $hasharray[$i]{'fac'}    = $facility;
            $hasharray[$i]{'name'}   = $proc_name;
            $hasharray[$i]{'type'}   = $proc_type;
            $hasharray[$i]{'state'}  = $state_name;
            $hasharray[$i]{'status'} = $status_text;

            $i++;
        }
        else {

            print(
                "------------------------------------------------------------\n" .
                "$site $proc_name $facility $proc_type:\n" .
                "------------------------------------------------------------\n" .
                "State           => $state_name: $state_text\n" .
                "State Time      => $state_time\n" .
                "Status          => $status_name: $status_text\n" .
                "Last Started    => $started_time\n" .
                "Last Completed  => $completed_time\n" .
                "Last Successful => $successful_time\n" .
                "\n");
        }
    }

    if ($gSummary) {
        @hash_keys = ('site', 'fac', 'name', 'type', 'state', 'status');
        $retval    = $gDSDB->hasharray_to_table(\@hash_keys, \@hasharray);
        print("$retval");
    }

    return($gExitGood);
}

################################################################################
#
#  Main
#

unless (GetOptions(
    "a=s", "f=s", "h", "help", "n=s", "s=s", "t=s", "v", "long")) {
    exit_usage();
}

if ($opt_h || $opt_help) { exit_usage(); }
if ($opt_v) {
    print("$gVersion\n");
    exit(0);
}

# Command line options

$gDBAlias   = $opt_a if ($opt_a);
$gFac      = $opt_f if ($opt_f);
$gSite     = $opt_s if ($opt_s);
$gProcName = $opt_n if ($opt_n);
$gProcType = $opt_t if ($opt_t);

$gSummary = 0 if ($opt_long);

# Connect to the database

$gDSDB = DSDB->new();
unless ($gDSDB->connect($gDBAlias)) {
    print_error(__LINE__, $gDSDB->error());
    exit(1);
}

# Initialize Process Methods

unless ($gDSDB->init_process_methods($gSite, $gFac, $gProcType, $gProcName)) {
    print_error(__LINE__, $gDSDB->error());
    $gDSDB->disconnect();
    exit(1);
}

# Run the main subroutine

$gExitValue = get_local_proc_stats();

# Disconnect from the database

$gDSDB->disconnect();

exit($gExitValue);
