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

use DBCORE;

use Getopt::Long;
Getopt::Long::Configure qw(no_ignore_case);

use vars qw($opt_a $opt_comp $opt_dbog $opt_h $opt_help $opt_v);
use vars qw($opt_doc $opt_cols $opt_I);
use vars qw($gVersion @gCommand $gCommand $gExitValue);

use vars qw($gDB %gConnArgs);
use vars qw($gSPClass $gSPMethod @gSPArgs @gDispCols);
use vars qw($gInteractive %gLastAnswer);
use vars qw($gExitGood $gExitBad);

$gVersion   = '@VERSION@';
@gCommand   = split(/\//,$0);
$gCommand   = $gCommand[$#gCommand];

$gExitValue = 0;
$gExitGood  = 0;
$gExitBad   = 1;

$gSPClass  = '';
$gSPMethod = '';
@gSPArgs   = ();
@gDispCols = ();

$gInteractive = 0;

$gConnArgs{'Component'}   = 'dsdb';
$gConnArgs{'ObjectGroup'} = 'dsdb';

$gLastAnswer{'RunAgain'}  = 'n';
$gLastAnswer{'CustomOut'} = 'n';

################################################################################
#
#  Usage Subroutine
#

sub usage()
{
print STDOUT <<EOF;

DESCRIPTION:

  This script provides an interactive command line interface to all safe-to-use
  DSDB stored procedures. It *does not* allow access to any "define" or "delete"
  procedures.  It *does* allow access to the "update" procedures but the user
  must know the correct password to run these.

USAGE:

  $gCommand [-a alias] [-I] -comp name -dbog name -doc sp_class sp_method
  $gCommand [-a alias] [-I] -comp name -dbog name sp_class sp_method sp_args...

    -h|-help   => Display this help message.
    -v         => Print version.

    -I         => Enable interactive mode.

    -a alias   => The database alias specified in the users .db_connect file.
                  If not specified the 'default' entry will be used.

    -cols list => Comma delimited list of the output columns to display.

    -comp name => The component the object group defs file is located in.
                  (default: $gConnArgs{'Component'})

    -dbog name => The "database object group" to load the definitions for.
                  (default: $gConnArgs{'ObjectGroup'})

    -doc       => Display documentation

    sp_class   => The stored procedure class
    sp_method  => The stored procedure method
    sp_args... => The stored procedure arguments

EOF
  exit($gExitBad);
}

sub print_error($$) { print(STDERR "$gCommand:$_[0]\t$_[1]"); }

################################################################################
#
#  Print Subroutines
#

sub print_sp_classes($)
{
    my ($dbh) = @_;
    my $sp_class;

    print("\n");
    print("Available Stored Procedure Classes:\n\n");

    foreach $sp_class  (sort(keys(%{ $dbh->{'SPDefs'} }))) {
        print("    $sp_class\n");
    }

    return(1);
}

sub print_sp_methods($$)
{
    my ($dbh, $sp_class) = @_;
    my $sp_method;

    unless(defined($dbh->{'SPDefs'}{$sp_class})) {
        print_error(__LINE__,
            "Invalid Stored Procedure Class: $sp_class\n\n");
        return(0);
    }

    print("\n");
    print("Available '$sp_class' Methods:\n\n");

    foreach $sp_method (sort(keys(%{ $gDB->{'SPDefs'}{$sp_class} }))) {

        next if ($sp_method =~ /^define/ || $sp_method =~ /^delete/);

        print("    $sp_method\n");
    }

    return(1);
}

################################################################################
#
#  Routines to get user input
#

sub get_input($$)
{
    my($question, $default) = @_;
    my $input = '';

    if ($default) {
        print ("$question [$default]: ");
    }
    else {
        print ("$question: ");
    }

    $input = <STDIN>;

    $input =~ s/\s*\n\s*//g;

    unless ($input) {
        return($default);
    }

    return($input);
}

sub get_required_input($$$)
{
    my($question, $default, $message) = @_;
    my $input = '';

    while (!$input) {
        $input = get_input($question, $default);
        print("\n$message\n") if (!$input);
    }

    return($input);
}

sub get_yes_no($$)
{
    my($question, $default) = @_;
    my $reply = '';

    $default = 'n' unless ($default);

    print ("$question [$default]: ");
    chop($reply = <STDIN>);

    if ($reply eq "y"   || $reply eq "Y"
     || $reply eq "yes" || $reply eq "Yes") {
        return(1);
    }
    elsif ($reply eq "" && $default eq "y") {
        return(1);
    }
    else {
        return(0);
    }
}

sub get_comp()
{
    print("\n");

    my $input = get_required_input(
        'Please specify the Component Name',
        $gConnArgs{'Component'},
        'You must specify a Component Name...');

    return($input);
}

sub get_dbog()
{
    print("\n");

    my $input = get_required_input(
        'Please specify the Database Object Group',
        $gConnArgs{'ObjectGroup'},
        'You must specify a Database Object Group...');

    return($input);
}

sub get_sp_class($$)
{
    my ($dbh, $default) = @_;
    my $sp_class = undef;

    if ($default) {
        unless (defined($dbh->{'SPDefs'}{$default})) {
            $default = '';
        }
    }

    print_sp_classes($gDB);

    print("\n");

    while (!$sp_class) {

        $sp_class = get_input(
                        'Please specify the Stored Procedure Class',
                        $default);

        if (!$sp_class) {
            print("\nYou must specify a Stored Procedure Class...\n");
        }
        elsif (!defined($dbh->{'SPDefs'}{$sp_class})) {
            print("\nInvalid Stored Procedure Class: $sp_class\n");
            $sp_class = '';
        }
    }

    return($sp_class);
}

sub get_sp_method($$$)
{
    my ($dbh, $sp_class, $default) = @_;
    my $sp_method = undef;

    if ($default) {
        unless (defined($dbh->{'SPDefs'}{$sp_class}{$default})) {
            $default = '';
        }
    }

    unless ($default) {
        if (defined($dbh->{'SPDefs'}{$sp_class}{'inquire'})) {
            $default = 'inquire';
        }
    }

    print_sp_methods($gDB, $gSPClass);

    print("\n");

    while (!$sp_method) {

        $sp_method = get_input(
                        'Please specify the Stored Procedure Method',
                        $default);

        if (!$sp_method) {
            print("\nYou must specify a Stored Procedure Method...\n");
        }
        elsif (!defined($dbh->{'SPDefs'}{$sp_class}{$sp_method})) {
            print("\nInvalid Stored Procedure Method: $sp_method\n");
            $sp_method = '';
        }
    }

    return($sp_method);
}

################################################################################
#
#  Main Subroutines
#

sub armdb_sp_doc($$)
{
    my ($sp_class, $sp_method) = @_;
    my $sp_description;
    my $is_sp_call;

    $is_sp_call = $gDB->is_sp_call($sp_class, $sp_method);

    unless ($is_sp_call) {
        print_error(__LINE__, $gDB->error());
        return($gExitBad);
    }

    $sp_description = $gDB->sp_comment($sp_class, $sp_method);

    if ($sp_description) {
        print("\n$sp_description\n");
    }
    else {
        print("\nNo description found for: $sp_class::$sp_method\n");
    }

    return($gExitGood);
}

sub armdb_sp_call($$@)
{
    my ($sp_class, $sp_method, @sp_args) = @_;
    my $retval;
    my @hash_keys;
    my $table_view;
    my @arg_names;
    my @arg_vals;
    my @ret_cols;
    my $default;
    my $yes;
    my $col;
    my $i;

    my $is_sp_call = $gDB->is_sp_call($sp_class, $sp_method);

    unless (defined($is_sp_call)) {
        print_error(__LINE__, $gDB->error());
        return($gExitBad);
    }

    if ($sp_method =~ /^define/ || $sp_method =~ /^delete/) {
        print("\nAll define and delete methods are currently disabled...\n\n");
        return($gExitBad);
    }

    @arg_names = @{$gDB->{'SPDefs'}{$sp_class}{$sp_method}{'args'}};

    if ($gDB->{'SPDefs'}{$sp_class}{$sp_method}{'rets'}) {
        @ret_cols = @{$gDB->{'SPDefs'}{$sp_class}{$sp_method}{'rets'}};
    }
    else {
        @ret_cols = ();
    }

    if ($gInteractive) {

        @arg_vals = ();

        print("\nPlease specify the Stored Procedure Arguments:\n\n");

        for ($i = 0; $i <= $#arg_names; $i++) {

            if (defined($gLastAnswer{'ArgVal'}{$arg_names[$i]})) {
                $default = $gLastAnswer{'ArgVal'}{$arg_names[$i]};
            }
            elsif ($sp_args[$i]) {
                $default = $sp_args[$i];
            }
            elsif (@ret_cols) {
                $default = '%';
            }
            else {
                $default = '';
            }

            $arg_vals[$i] = get_required_input(
                $arg_names[$i],
                $default,
                "You must specify a value for the '$arg_names[$i]' argument...");

            $gLastAnswer{'ArgVal'}{$arg_names[$i]} = $arg_vals[$i];
        }
    }
    else {
        @arg_vals = @sp_args;
    }

    $retval = $gDB->sp_call($sp_class, $sp_method, @arg_vals);

    unless (defined($retval)) {
        print_error(__LINE__, $gDB->error());
        return($gExitBad);
    }

    if (@ret_cols) {

        @hash_keys = @ret_cols;

        if ($gInteractive) {

            $yes = get_yes_no(
                "\nWould you like to customize the output columns?",
                $gLastAnswer{'CustomOut'});

            if ($yes) {

                @hash_keys = ();
                $gLastAnswer{'CustomOut'} = 'y';

                print("\n");

                foreach $col (@ret_cols) {

                    if (defined($gLastAnswer{'OutCol'}{$col})) {
                        $default = $gLastAnswer{'OutCol'}{$col};
                    }
                    else {
                        $default = 'y';
                    }

                    $yes = get_yes_no("Display column '$col'", $default);

                    if ($yes) {
                        $gLastAnswer{'OutCol'}{$col} = 'y';
                        push(@hash_keys, $col);
                    }
                    else {
                        $gLastAnswer{'OutCol'}{$col} = 'n';
                    }
                }
            }
        }
        elsif (@gDispCols) {
            @hash_keys  = @gDispCols;
        }

        $table_view = $gDB->hasharray_to_table(\@hash_keys, $retval);
        print("\n$table_view\n");
    }
    else {
        print("\nReturn Value: $retval\n\n");
    }

    return($gExitGood);
}

################################################################################
#
#  Main
#

unless (GetOptions("a=s", "comp=s", "dbog=s", "h", "help", "v",
                   "cols=s", "doc", "I")) {
    usage();
}

if ($opt_h || $opt_help) { usage(); }
if ($opt_v) {
    print("$gVersion\n");
    exit(0);
}

$gInteractive = 1 if ($opt_I);

if ($opt_comp) {
    $gConnArgs{'Component'} = $opt_comp;
}
elsif ($gInteractive) {
    $gConnArgs{'Component'} = get_comp();
}

if ($opt_dbog) {
    $gConnArgs{'ObjectGroup'} = $opt_dbog;
}
elsif ($gInteractive) {
    $gConnArgs{'ObjectGroup'} = get_dbog();
}

$gConnArgs{'DBAlias'} = $opt_a if ($opt_a);

if ($opt_cols) {
    $opt_cols =~ s/\s*,\s*/,/g;
    @gDispCols = split(/,/, $opt_cols);
}

# Connect to the database

$gDB = DBCORE->new();
unless ($gDB->connect(\%gConnArgs)) {
    print_error(__LINE__, $gDB->error());
    exit($gExitBad);
}

# get the stored procedure class and method

if ($ARGV[0]) {
    $gSPClass = shift(@ARGV);
}
elsif (!$gInteractive) {
    usage();
}

if ($ARGV[0]) {
    $gSPMethod = shift(@ARGV);
}
elsif (!$gInteractive) {
    usage();
}

@gSPArgs = @ARGV;

for (;;) {

    if ($gInteractive) {
        $gSPClass  = get_sp_class($gDB, $gSPClass);
        $gSPMethod = get_sp_method($gDB, $gSPClass, $gSPMethod);
    }

    # run the main subroutine

    if ($opt_doc) {
        $gExitValue = armdb_sp_doc($gSPClass, $gSPMethod);
    }
    else {
        $gExitValue = armdb_sp_call($gSPClass, $gSPMethod, @gSPArgs);
    }

    last unless($gInteractive);

    if (get_yes_no(
        "\nWould you like to call another stored procedure?",
        $gLastAnswer{'RunAgain'})) {

        $gLastAnswer{'RunAgain'} = 'y';
    }
    else {
        last;
    }
}

# Disconnect from the database

$gDB->disconnect();

exit($gExitValue);
