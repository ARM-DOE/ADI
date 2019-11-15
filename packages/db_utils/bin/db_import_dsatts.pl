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

our $gVersion = '@VERSION@';
our @gCommand = split(/\//,$0);
our $gCommand = $gCommand[$#gCommand];

our $gDB;
our %gConnArgs = (
    'DBAlias'     => 'dsdb_data',
    'Component'   => 'dsdb',
    'ObjectGroup' => 'dod',
);

our $gDebug = 0;

################################################################################
#
#  Exit Usage
#

sub exit_usage($)
{
    my ($status) = @_;

    print STDOUT <<EOF;

NAME

    $gCommand - Import a DSATTS definition table into the database.

SYNOPSIS

    $gCommand file

ARGUMENTS

    file            The DSATTS definition file.

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

sub define_ds_att($$$$$$$)
{
    #inputs: site, fac, dsc_name, dsc_level
    #        att_name, att_type, att_value
    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('ds_atts', 'define', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub define_ds_time_att($$$$$$$$)
{
    #inputs: site, fac, dsc_name, dsc_level
    #        att_name, att_time, att_type, att_value
    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('ds_time_atts', 'define', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub define_ds_var_att($$$$$$$$)
{
    #inputs: site, fac, dsc_name, dsc_level, var_name
    #        att_name, att_type, att_value
    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('ds_var_atts', 'define', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub define_ds_var_time_att($$$$$$$$$)
{
    #inputs: site, fac, dsc_name, dsc_level, var_name
    #        att_name, att_time, att_type, att_value
    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('ds_var_time_atts', 'define', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub delete_ds_att($$$$$)
{
    #inputs: site, fac, dsc_name, dsc_level, att_name
    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('ds_atts', 'delete', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub delete_ds_time_att($$$$$$)
{
    #inputs: site, fac, dsc_name, dsc_level, att_name, att_time
    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('ds_time_atts', 'delete', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub delete_ds_var_att($$$$$$)
{
    #inputs: site, fac, dsc_name, dsc_level, var_name, att_name
    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('ds_var_atts', 'delete', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

sub delete_ds_var_time_att($$$$$$$)
{
    #inputs: site, fac, dsc_name, dsc_level, var_name, att_name, att_time
    my (@sp_args) = @_;
    my $retval;

    $retval = $gDB->sp_call('ds_var_time_atts', 'delete', @sp_args);

    unless (defined($retval)) {
        print($gDB->error());
        return(0);
    }

    return(1);
}

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

sub load_dsatts_file($)
{
    my ($file) = @_;
    my $fh;
    my $data;
    my $dsatts;

    unless (open $fh, '<', $file) {
        print("Could not open file: $file\n -> $!\n");
        return(undef);
    }

    $/ = undef;
    $data = <$fh>;
    close $fh;

    $dsatts = eval($data);

    if ($@) {
        print("Could not load file: $file\n -> $@\n");
        return(undef);
    }

    return($dsatts);
}

sub cleanup_old_dsatts($$$)
{
    my ($dsatts, $name, $level) = @_;
    my ($dbres, $row);
    my ($site, $fac, $var, $att, $type, $time);
    my ($tbl, $sf);

    my $ds = "$name.$level";
    my %dsatts = ();

    # Global Attributes

    $dbres = inquire_ds_atts($name, $level);
    return(0) unless (defined($dbres));

    if (@{$dbres}) {

        $tbl = $dsatts->{$ds}{'global_atts'};

        foreach $row (@{$dbres}) {

            $site = $row->{'site'};
            $fac  = $row->{'facility'};
            $att  = $row->{'att_name'};
            $type = $row->{'att_type'};

            $sf = "$site.$fac";

            unless (exists($tbl->{$att}{$type}{$sf})) {

                if ($gDebug) {
                    print("delete: $ds\t$att\t$sf\n");
                }

                return(0) unless (delete_ds_att(
                    $site, $fac, $name, $level, $att));
            }
        }
    }

    # Global Time Attributes

    $dbres = inquire_ds_time_att($name, $level);
    return(0) unless (defined($dbres));

    if (@{$dbres}) {

        $tbl = $dsatts->{$ds}{'global_time_atts'};

        foreach $row (@{$dbres}) {

            $site = $row->{'site'};
            $fac  = $row->{'facility'};
            $att  = $row->{'att_name'};
            $type = $row->{'att_type'};
            $time = $row->{'att_time'};

            $sf = "$site.$fac";

            unless (exists($tbl->{$att}{$type}{$sf}{$time})) {

                if ($gDebug) {
                    print("delete: $ds\t$att\t$sf\t$time\n");
                }

                return(0) unless (delete_ds_time_att(
                    $site, $fac, $name, $level, $att, $time));
            }
        }
    }

    # Variable Attributes

    $dbres = inquire_ds_var_att($name, $level);
    return(0) unless (defined($dbres));

    if (@{$dbres}) {

        $tbl = $dsatts->{$ds}{'var_atts'};

        foreach $row (@{$dbres}) {

            $site = $row->{'site'};
            $fac  = $row->{'facility'};
            $var  = $row->{'var_name'};
            $att  = $row->{'att_name'};
            $type = $row->{'att_type'};

            $sf = "$site.$fac";

            unless (exists($tbl->{$var}{$att}{$type}{$sf})) {

                if ($gDebug) {
                    print("delete: $ds\t$var:$att\t$sf\n");
                }

                return(0) unless (delete_ds_var_att(
                    $site, $fac, $name, $level, $var, $att));
            }
        }
    }

    # Variable Time Attributes

    $dbres = inquire_ds_var_time_att($name, $level);
    return(0) unless (defined($dbres));

    if (@{$dbres}) {

        $tbl = $dsatts->{$ds}{'var_time_atts'};

        foreach $row (@{$dbres}) {

            $site = $row->{'site'};
            $fac  = $row->{'facility'};
            $var  = $row->{'var_name'};
            $att  = $row->{'att_name'};
            $type = $row->{'att_type'};
            $time = $row->{'att_time'};

            $sf = "$site.$fac";

            unless (exists($tbl->{$var}{$att}{$type}{$sf}{$time})) {

                if ($gDebug) {
                    print("delete: $ds\t$var:$att\t$sf\t$time\n");
                }

                return(0) unless (delete_ds_var_time_att(
                    $site, $fac, $name, $level, $var, $att, $time));
            }
        }
    }

    return(1);
}

sub db_import_dsatts($)
{
    my ($file) = @_;
    my ($ds, $tbl, $var, $att, $sf, $time, $type, $value);
    my ($name, $level, $site, $fac);
    my $dsatts;

    # Load the DSATTS definition file

    $dsatts = load_dsatts_file($file);
    return(0) unless ($dsatts);

    # Import the DSATTS into the database

    foreach $ds (sort(keys(%{$dsatts}))) {

        ($name, $level) = split(/\./, $ds);

        # Cleanup old DSATTS entries that have been removed.

        unless (cleanup_old_dsatts($dsatts, $name, $level)) {
            return(0);
        }

        # Global Attributes

        $tbl = $dsatts->{$ds}{'global_atts'};

        foreach $att  (sort(keys(%{$tbl}))) {
        foreach $type (sort(keys(%{$tbl->{$att}}))) {
        foreach $sf   (sort(keys(%{$tbl->{$att}{$type}}))) {

            ($site, $fac) = split(/\./, $sf);

            $value = $tbl->{$att}{$type}{$sf};

            if ($gDebug) {
                print("define: $ds\t$att\t$type\t$sf\t'$value'\n");
            }

            return(0) unless (define_ds_att(
                $site, $fac, $name, $level, $att, $type, $value));
        }}}

        # Global Time Attributes

        $tbl = $dsatts->{$ds}{'global_time_atts'};

        foreach $att  (sort(keys(%{$tbl}))) {
        foreach $type (sort(keys(%{$tbl->{$att}}))) {
        foreach $sf   (sort(keys(%{$tbl->{$att}{$type}}))) {
        foreach $time (sort(keys(%{$tbl->{$att}{$type}{$sf}}))) {

            ($site, $fac) = split(/\./, $sf);

            $value = $tbl->{$att}{$type}{$sf}{$time};

            if ($gDebug) {
                print("define: $ds\t$att\t$type\t$sf\t$time\t'$value'\n");
            }

            return(0) unless (define_ds_time_att(
                $site, $fac, $name, $level, $att, $time, $type, $value));
        }}}}

        # Variable Attributes

        $tbl = $dsatts->{$ds}{'var_atts'};

        foreach $var  (sort(keys(%{$tbl}))) {
        foreach $att  (sort(keys(%{$tbl->{$var}}))) {
        foreach $type (sort(keys(%{$tbl->{$var}{$att}}))) {
        foreach $sf   (sort(keys(%{$tbl->{$var}{$att}{$type}}))) {

            ($site, $fac) = split(/\./, $sf);

            $value = $tbl->{$var}{$att}{$type}{$sf};

            if ($gDebug) {
                print("define: $ds\t$var:$att\t$type\t$sf\t'$value'\n");
            }

            return(0) unless (define_ds_var_att(
                $site, $fac, $name, $level, $var, $att, $type, $value));
        }}}}

        # Variable Time Attributes

        $tbl = $dsatts->{$ds}{'var_time_atts'};

        foreach $var  (sort(keys(%{$tbl}))) {
        foreach $att  (sort(keys(%{$tbl->{$var}}))) {
        foreach $type (sort(keys(%{$tbl->{$var}{$att}}))) {
        foreach $sf   (sort(keys(%{$tbl->{$var}{$att}{$type}}))) {
        foreach $time (sort(keys(%{$tbl->{$var}{$att}{$type}{$sf}}))) {

            ($site, $fac) = split(/\./, $sf);

            $value = $tbl->{$var}{$att}{$type}{$sf}{$time};

            if ($gDebug) {
                print("define: $ds\t$var:$att\t$type\t$sf\t$time\t'$value'\n");
            }

            return(0) unless (define_ds_var_time_att(
                $site, $fac, $name, $level, $var, $att, $time, $type, $value));
        }}}}}
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
    "debug"   => \$gDebug,
    "help"    => sub { exit_usage(1); },
    "version" => sub { print("$gVersion\n"); exit(1); },
));

exit_usage(1) unless ($ARGV[0]);

# Connect to the database

$gDB = DBCORE->new();

unless ($gDB->connect(\%gConnArgs)) {
    print($gDB->error());
    exit(1);
}

# Run the main subroutine

unless (db_import_dsatts($ARGV[0])) {
    $gDB->disconnect();
    exit(1);
}

# Disconnect from the database

$gDB->disconnect();

exit(0);
