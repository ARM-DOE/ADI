#!/usr/bin/env perl
################################################################################
#
#  COPYRIGHT (C) 2006 Battelle Memorial Institute.  All Rights Reserved.
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

use vars qw($opt_a $opt_h $opt_help $opt_v);
use vars qw($opt_c $opt_d $opt_e $opt_s $opt_f $opt_n);

use vars qw($gVersion @gCommand $gCommand);
use vars qw($gDSDB $gDBAlias);
use vars qw(%gConf $gSite $gFac $gProcName $gDisableMsg $gEnableMsg);

$gVersion = '@VERSION@';
@gCommand = split(/\//,$0);
$gCommand = $gCommand[$#gCommand];
$gDBAlias = 'dsdb_data';

%gConf       = ();
$gSite       = '';
$gFac        = '';
$gProcName   = '';
$gDisableMsg = '';
$gEnableMsg  = '';

################################################################################

sub usage()
{
print STDOUT <<EOF;

USAGE:

  $gCommand -c file [-s site] [-f fac] [-n name] [-d msg] [-e msg]
  $gCommand site facility type name state_name state_text

    [-h|-help] => Display this help message.
    [-v]       => Print version.

    [-a alias] => The database alias specified in the users .db_connect file.
                  If not specified the '$gDBAlias' entry will be used.

    -c file => File containing the process states to set.

    [-d msg] => Disable the processes using this message.
    [-e msg] => Enable the processes using this message.

    or
    
    site       => The site name (i.e. sgp)
    facility   => The facility name (i.e. C1)
    type       => The process type (i.e. Ingest)
    name       => The process name (i.e. vceil25k)
    state_name => The state name (Enabled|Disabled)
    state_text => A message describing the reason for the state change.
    
EOF
  exit(1);
}

sub update_process_state($$)
{
    my ($state_name, $state_text) = @_;
    my $state_time = time();
    
    unless ($gDSDB->update_family_process_state(
        $state_name, $state_text, $state_time)) {
        
        print($gDSDB->error());
        return(0);
    }

    return(1);
}

################################################################################
#
#  Load configuration file
#

sub load_config_file($)
{
    my ($conf_file) = @_;
    my $retval;
    my $site;
    my $fac;
    my $proc_type;
    my $proc_name;
    my $state_name;
    my $state_text;
    
    unless ($retval = do $conf_file) {

        if ($@) {
            print("Could not compile config file: $conf_file\n -> $@\n");
        }
        elsif (!defined($retval)) {
            print("Could not read config file: $conf_file\n -> $!\n");
        }
        else {
            print("Bad return value from config file: $conf_file\n");
        }

        return(0);
    }

    foreach $site      (sort(keys(%gConf))) {

        next if ($gSite && $site ne $gSite);

    foreach $fac       (sort(keys(%{ $gConf{$site} }))) {

        next if ($gFac && $fac ne $gFac);

    foreach $proc_type (sort(keys(%{ $gConf{$site}{$fac} }))) {
    foreach $proc_name (sort(keys(%{ $gConf{$site}{$fac}{$proc_type} }))) {

        next if ($gProcName && $proc_name ne $gProcName);

        $state_name = @{ $gConf{$site}{$fac}{$proc_type}{$proc_name} }[0];
        $state_text = @{ $gConf{$site}{$fac}{$proc_type}{$proc_name} }[1];
        
        if ($state_text eq 'Testing') {
            if ($gDisableMsg) {
                $state_name = 'Disabled';
                $state_text = $gDisableMsg;
            }
            elsif ($gEnableMsg && $state_name eq 'Enabled') {
                $state_text = $gEnableMsg;
            }
        }

        if ($gDSDB->init_process_methods(
            $site, $fac, $proc_type, $proc_name)) {
                
            update_process_state($state_name, $state_text);

            print("Set: $site $fac $proc_name $proc_type $state_name '$state_text'\n");
        }
        else {
            print($gDSDB->error());
        }
    }}}}
    
    return(1);
}

################################################################################
#
#  Main
#

unless (GetOptions("a=s", "c=s", "d=s", "e=s", "f=s", "h", "help", "n=s", "s=s", "v")) {
    usage();
}

if ($opt_h || $opt_help) { usage(); }
if ($opt_v) {
    print("$gVersion\n");
    exit(0);
}

if (!$opt_c && !$ARGV[5]) {
    usage();
}

if ($opt_a) {
    $gDBAlias = $opt_a;
}

if ($opt_f) {
    $gFac = $opt_f;
}

if ($opt_s) {
    $gSite = $opt_s;
}

if ($opt_n) {
    $gProcName = $opt_n;
}


# Connect to the database

$gDSDB = DSDB->new();
unless ($gDSDB->connect($gDBAlias)) {
    print($gDSDB->error());
    exit(1);
}

if ($opt_c) {

    if ($opt_d) {
        $gDisableMsg = $opt_d;
    }
    elsif ($opt_e) {
        $gEnableMsg  = $opt_e;
    }

    load_config_file($opt_c);
}
else {
    if ($gDSDB->init_process_methods(
        $ARGV[0], $ARGV[1], $ARGV[2], $ARGV[3])) {

        update_process_state($ARGV[4], $ARGV[5]);
    }
    else {
        print($gDSDB->error());
    }
}

# Disconnect from the database

$gDSDB->disconnect();

exit(0);
