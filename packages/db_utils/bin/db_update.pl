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

use vars qw($opt_a $opt_comp $opt_f $opt_h $opt_n $opt_p $opt_pkg $opt_u);
use vars qw($opt_d $opt_help $opt_t $opt_v $opt_revert);
use vars qw($gVersion @gCommand $gCommand $gDebug $gExitValue);

use vars qw($gDB @gDBClones %gDBClone @gCreatedDBs @gDropDBs);
use vars qw($gObjectGroup $gCompName $gCompHome $gPkgName $gPkgDir);
use vars qw($gTargetVersion $gRevert $gDefsFile);

use vars qw($gUpdateSummaryFile $gUpdateOrderFile %gConnArgs %DBData);

$gVersion   = '@VERSION@';
@gCommand   = split(/\//,$0);
$gCommand   = $gCommand[$#gCommand];
$gDebug     = 0;
$gExitValue = 0;

$gUpdateSummaryFile = "UPDATE_SUMMARY";
$gUpdateOrderFile   = "UPDATE_ORDER";

%gConnArgs      = ();
$gTargetVersion = 0;
$gRevert        = 0;
@gCreatedDBs    = ();
@gDropDBs       = ();
@gDBClones      = ();
%gDBClone       = ();

$gCompName      = 'dsdb';

sub usage()
{
print STDOUT <<EOF;

USAGE:

  $gCommand [-a alias] [-comp name] [-pkg name] object_group

    [-d]         => Print debuging information.
    [-help]      => Display this help message.
    [-v]         => Print version.

    [-a alias]   => The database alias specified in the users .db_connect file.
    [-f file]    => The database connection file to use.
    [-h host]    => The host the database server is located on.
    [-n name]    => The name of the database to connect to.
    [-p pass]    => The database users password.
    [-u user]    => The database user.

    [-comp name] => The component the object group package is located in.
                    (default: $gCompName)

    [-pkg name]  => The object group package to run the update for.
                    (defaults to the specified object_group)

    [-revert]    => Revert the object group package to a previous version.
                    WARNING: This will remove the entire object group
                    package if a target version is not specified.

    [-t version] => The target version to updated/revert the package to.

    object_group => The "database object group" to perform the update(s) for.

EOF
  exit(1);
}

sub print_debug($)
{
    my ($message) = @_;
    return unless $gDebug;
    print(STDOUT "$message");
}

sub print_warning($)
{
    my ($message) = @_;

    print(STDERR "WARNING -> $message");
}

sub print_error($$)
{
    my ($line,$message) = @_;

    print(STDERR "$message");
}

################################################################################
#
#  Object Group Version Tracking Subroutines
#

sub get_object_group_package_version($$)
{
    my ($object_group, $pkg_name) = @_;
    my $is_sp_call;
    my $version;

    print_debug("Getting '$object_group-$pkg_name' Version ...\n");

    # If this is the first DBCORE Object Group update then we have a
    # "chicken and the egg" problem.  To get arround this, check that
    # the "get database version" stored procedure exists.

    $is_sp_call = $gDB->is_sp_call('object_group_packages',
                                   'get_version');

    unless (defined($is_sp_call)) {
        print_error(__LINE__, $gDB->error());
        return(undef);
    }

    unless ($is_sp_call) {

        if ($object_group eq 'dbcore' && $pkg_name eq 'dbcore') {

            # This is a DBCORE Object Group update and the stored procedure
            # does not exist so this must be the first DBCORE Object Group
            # update ... return version 0.

            print_debug("Current '$object_group-$pkg_name' Version: 0.0.0\n");
            return('0.0.0');
        }
        else {

            # Not a DBCORE Database Object Group update.

            print_error(__LINE__,
                "Please install the latest release of 'dbcore-dbog_dbcore'.\n");
            return(undef);
        }
    }

    $version = $gDB->sp_call('object_group_packages', 'get_version', @_);

    unless (defined($version)) {
        print_error(__LINE__, $gDB->error());
        return(undef);
    }

    unless ($version) {
        $version = '0.0.0';
    }

    print_debug("Current '$object_group-$pkg_name' Version: $version\n");

    return($version);
}

sub update_object_group_package_version($$$$$)
{
    my $revert = shift;
    my ($object_group, $pkg_name, $version, $update_summary) = @_;
    my ($retval);

    if ($revert) {
        print_debug("Reverting '$object_group-$pkg_name' To Version: $version\n");
    }
    else {
        print_debug("Updating '$object_group-$pkg_name' To Version: $version\n");
    }

    $retval = $gDB->sp_call('object_group_packages',
                            'update_version', @_);

    unless (defined($retval)) {
        print_error(__LINE__, $gDB->error());
        return(0);
    }

    return(1);
}

################################################################################
#
#  Database Creation/Dropping Subroutines
#

sub clone_connection($)
{
    my ($dbname) = @_;

    unless(defined($gDBClone{$dbname})) {

        print_debug("CONNECT: $dbname\n");
        $gDBClone{$dbname} = $gDB->clone({'DBName' => $dbname});

        unless($gDBClone{$dbname}) {
            print_error(__LINE__, $gDB->error());
            return(0);
        }

        push(@gDBClones, $dbname);
    }

    return(1);
}

sub create_databases(@)
{
    my @sql_strings = @_;
    my $dbh;
    my $sql_string;

    $dbh = $gDB->clone({'AutoCommit' => 1});

    unless($dbh) {
        print_error(__LINE__, $gDB->error());
        return(0);
    }

    foreach $sql_string (@sql_strings) {

        print_debug("$sql_string");

        unless($dbh->do($sql_string)) {
            print_error(__LINE__, $dbh->error());
            $dbh->disconnect();
            undef($dbh);
            return(0);
        }

        if ($sql_string =~ /^\s*CREATE\s+DATABASE\s+(\w+)/i) {
            push(@gCreatedDBs, $1);
        }
    }

    $dbh->disconnect();
    undef($dbh);

    return(1);
}

sub drop_databases(@)
{
    my @dbnames = @_;
    my $dbname;
    my $dbh;
    my $retval;

    $dbh = $gDB->clone({'AutoCommit' => 1});

    unless($dbh) {
        print_error(__LINE__, $gDB->error());
        return(0);
    }

    $retval = 1;
    foreach $dbname (@dbnames) {

        print_debug("DROP DATABASE $dbname;\n");

        unless($dbh->do("DROP DATABASE $dbname;")) {
            print_error(__LINE__, $dbh->error());
            $retval = 0;
        }
    }

    $dbh->disconnect();
    undef($dbh);

    return($retval);
}

################################################################################
#
#  Object Group Revert Subroutines
#

sub revert_data($)
{
    my ($method) = @_;
    my $retval;

    if ($method eq 'define') {

        print_debug("Deleting Data...\n");

        $retval = $gDB->data_factory('delete', \%DBData, 1);
    }
    elsif ($method eq 'delete') {

        print_debug("Defining Data...\n");

        $retval = $gDB->data_factory('define', \%DBData, 0);
    }
    else {
        print_error(__LINE__,
            "CAN NOT REVERT: Data added with the '$method' method\n");
        return(0);
    }

    unless (defined($retval)) {
        print_error(__LINE__, $gDB->error());
        return(0);
    }

    return(1);
}

sub revert_updates_file($)
{
    my ($file) = @_;
    my $line;
    my $sql_string;
    my @sql_strings;
    my $found_sql_command;
    my $dbname;
    my $href;

    print_debug("Reverting Updates...\n");

    unless(open(FH, $file)) {
        print_error(__LINE__,
            "Could not open updates file: $!\n" .
            " -> $file\n");
        return(0);
    }

    @sql_strings = ();
    $dbname      = 'default';

    foreach $line (<FH>) {

        $sql_string = '';

      # Database Connection Command
        if ($line =~ /^\\c\s+(\w+)/i ||
            $line =~ /^\\connect\s+(\w+)/i) {
            $dbname = $1;
            next;
        }

      # ALTER GROUP ... ADD USER ...
        elsif ($line =~ /^\s*ALTER\s+GROUP\s+(\w+)\s+ADD\s+USER\s+(.+)\s*;/i) {
            $sql_string = "ALTER GROUP $1 DROP USER $2;";
        }

      # ALTER SEQUENCE ... OWNED BY ...
        elsif ($line =~ /^\s*ALTER\s+SEQUENCE\s+(\w+)\s+OWNED\s+BY\s+(.+)\s*;/i) {
            $sql_string = "ALTER SEQUENCE $1 OWNED BY NONE;";
        }

      # ALTER TABLE ... ADD CONSTRAINT ...
        elsif ($line =~ /^\s*ALTER\s+TABLE\s+(\w+)\s+ADD\s+CONSTRAINT\s+(\w+)/i) {
            $sql_string = "ALTER TABLE $1 DROP CONSTRAINT $2;";
        }

      # COMMENT
        elsif ($line =~ /^\s*COMMENT\s+/i) {
            chomp($line);

            # Reverting a COMMENT is impossible because there is no way to
            # determine what the previous comment was ...

            print_warning("CAN NOT REVERT: '$line'\n");

            next;
        }

      # CREATE DATABASE
        elsif ($line =~ /^\s*CREATE\s+DATABASE\s+(\w+)/i) {
            push(@gDropDBs, $1);
            next;
        }

      # CREATE FUNCTION
        elsif ($line =~ /^\s*CREATE\s+FUNCTION\s+(\w+)\s*\((.*)\)/i) {
            $sql_string = "DROP FUNCTION $1($2);";
        }

      # CREATE OR REPLACE FUNCTION
        elsif ($line =~ /^\s*CREATE\s+OR\s+REPLACE\s+FUNCTION\s+(\w+)\s*\((.*)\)/i) {
            $sql_string = "DROP FUNCTION $1($2);";
        }

      # CREATE GROUP
        elsif ($line =~ /^\s*CREATE\s+GROUP\s+(\w+)/i) {
            $sql_string = "DROP GROUP $1;";
        }

      # CREATE INDEX
        elsif ($line =~ /^\s*CREATE\s+INDEX\s+(\w+)/i) {
            $sql_string = "DROP INDEX $1;";
        }

      # CREATE SEQUENCE
        elsif ($line =~ /^\s*CREATE\s+SEQUENCE\s+(\w+)/i) {
            $sql_string = "DROP SEQUENCE $1;";
        }

      # CREATE TABLE
        elsif ($line =~ /^\s*CREATE\s+TABLE\s+(\w+)/i) {
            $sql_string = "DROP TABLE $1;";
        }

      # CREATE TRIGGER
        elsif ($line =~ /^\s*CREATE\s+TRIGGER\s+(\w+)\s+.+\s+ON\s+(\w+)/i) {
            $sql_string = "DROP TRIGGER $1 ON $2;";
        }

      # CREATE TYPE
        elsif ($line =~ /^\s*CREATE\s+TYPE\s+(\w+)/i) {
            $sql_string = "DROP TYPE $1;";
        }

      # CREATE USER
        elsif ($line =~ /^\s*CREATE\s+USER\s+(\w+)/i) {
            $sql_string = "DROP USER $1;";
        }

      # GRANT ... ON ... TO
        elsif ($line =~ /^\s*GRANT\s+(.+)\s+ON\s+(.+)\s+TO\s+(.+)\s*;/i) {
            $sql_string = "REVOKE $1 ON $2 FROM $3;";
        }

      # REVOKE
        elsif ($line =~ /^\s*REVOKE\s+/i) {
            chomp($line);

            # Reverting a REVOKE is impossible because there is no way to
            # determine what the original privileges were ...

            print_warning("CAN NOT REVERT: '$line'\n");

            next;
        }

      # The following are checks for "SQL Commands" that have not been
      # recognized by the previous checks.  If any of these are found
      # the revert must fail to prevent database corruption...

      # DROP FUNCTION
      # This is dangerous so I only do it in development...
      # See comment about reverting "DROP" commands below
      #
      #  elsif ($line =~ /^\s*DROP\s+FUNCTION\s+/i) {
      #
      #      chomp($line);
      #      print_warning("CAN NOT REVERT: '$line'\n");
      #      next;
      #  }

        else {
            $found_sql_command = 0;

            if ($line =~ /^\s*ALTER\s+AGGREGATE\s+/i)             { $found_sql_command = 1; }
            if ($line =~ /^\s*ALTER\s+CONVERSION\s+/i)            { $found_sql_command = 1; }
            if ($line =~ /^\s*ALTER\s+DATABASE\s+/i)              { $found_sql_command = 1; }
            if ($line =~ /^\s*ALTER\s+DOMAIN\s+/i)                { $found_sql_command = 1; }
            if ($line =~ /^\s*ALTER\s+FUNCTION\s+/i)              { $found_sql_command = 1; }
            if ($line =~ /^\s*ALTER\s+GROUP\s+/i)                 { $found_sql_command = 1; }
            if ($line =~ /^\s*ALTER\s+INDEX\s+/i)                 { $found_sql_command = 1; }
            if ($line =~ /^\s*ALTER\s+LANGUAGE\s+/i)              { $found_sql_command = 1; }
            if ($line =~ /^\s*ALTER\s+OPERATOR\s+/i)              { $found_sql_command = 1; }
            if ($line =~ /^\s*ALTER\s+OPERATOR\s+CLASS\s+/i)      { $found_sql_command = 1; }
            if ($line =~ /^\s*ALTER\s+SCHEMA\s+/i)                { $found_sql_command = 1; }
            if ($line =~ /^\s*ALTER\s+SEQUENCE\s+/i)              { $found_sql_command = 1; }
            if ($line =~ /^\s*ALTER\s+TABLE\s+/i)                 { $found_sql_command = 1; }
            if ($line =~ /^\s*ALTER\s+TABLESPACE\s+/i)            { $found_sql_command = 1; }
            if ($line =~ /^\s*ALTER\s+TRIGGER\s+/i)               { $found_sql_command = 1; }
            if ($line =~ /^\s*ALTER\s+TYPE\s+/i)                  { $found_sql_command = 1; }
            if ($line =~ /^\s*ALTER\s+USER\s+/i)                  { $found_sql_command = 1; }

            if ($line =~ /^\s*CLUSTER\s+/i)                       { $found_sql_command = 1; }
            if ($line =~ /^\s*COMMENT\s+/i)                       { $found_sql_command = 1; }
            if ($line =~ /^\s*COPY\s+/i)                          { $found_sql_command = 1; }

            if ($line =~ /^\s*CREATE\s+AGGREGATE\s+/i)            { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+CAST\s+/i)                 { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+CONSTRAINT\s+TRIGGER\s+/i) { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+CONVERSION\s+/i)           { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+DATABASE\s+/i)             { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+DOMAIN\s+/i)               { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+FUNCTION\s+/i)             { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+GROUP\s+/i)                { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+LANGUAGE\s+/i)             { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+OPERATOR\s+/i)             { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+OPERATOR\s+CLASS\s+/i)     { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+RULE\s+/i)                 { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+SCHEMA\s+/i)               { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+SEQUENCE\s+/i)             { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+TABLE\s+/i)                { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+TABLESPACE\s+/i)           { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+TRIGGER\s+/i)              { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+TYPE\s+/i)                 { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+USER\s+/i)                 { $found_sql_command = 1; }
            if ($line =~ /^\s*CREATE\s+VIEW\s+/i)                 { $found_sql_command = 1; }

            # Reverting a DROP will required a lot more code.  If something is
            # dropped we will need to scan back through the versions to find
            # where it was created.  Then we will need to continue to scan
            # forward from that point for any ALTER commands on the object.

            if ($line =~ /^\s*DROP\s+AGGREGATE\s+/i)              { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+CAST\s+/i)                   { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+CONVERSION\s+/i)             { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+DATABASE\s+/i)               { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+DOMAIN\s+/i)                 { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+FUNCTION\s+/i)               { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+GROUP\s+/i)                  { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+INDEX\s+/i)                  { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+LANGUAGE\s+/i)               { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+OPERATOR\s+/i)               { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+OPERATOR\s+CLASS\s+/i)       { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+RULE\s+/i)                   { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+SCHEMA\s+/i)                 { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+SEQUENCE\s+/i)               { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+TABLE\s+/i)                  { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+TABLESPACE\s+/i)             { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+TRIGGER\s+/i)                { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+TYPE\s+/i)                   { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+USER\s+/i)                   { $found_sql_command = 1; }
            if ($line =~ /^\s*DROP\s+VIEW\s+/i)                   { $found_sql_command = 1; }
            if ($line =~ /^\s*GRANT\s+/i)                         { $found_sql_command = 1; }

            if ($line =~ /^\s*REVOKE\s+/i)                        { $found_sql_command = 1; }

            if ($line =~ /^\s*COMMIT\s+/i)                        { $found_sql_command = 1; }
            if ($line =~ /^\s*ROLLBACK\s+/i)                      { $found_sql_command = 1; }

            if ($found_sql_command) {
                print_error(__LINE__,
                "\n" .
                "Can not revert SQL Command: $line\n" .
                "Please contact the current developer of the $gCommand script.\n\n");
                return(0);
            }
        }

        if ($sql_string) {
            push(@sql_strings,
                 {'dbname' => $dbname, 'sql_string' => $sql_string});
        }
    }

    close(FH);

    @sql_strings = reverse(@sql_strings);

    foreach $href (@sql_strings) {

        $dbname     = $href->{'dbname'};
        $sql_string = $href->{'sql_string'};

        if ($dbname eq 'default') {
            unless($gDB->do($sql_string)) {
                print_error(__LINE__, $gDB->error());
                return(0);
            }
        }
        else {
            unless($gDBClone{$dbname}) {
                unless(clone_connection($dbname)) {
                    return(0);
                }
            }

            unless($gDBClone{$dbname}->do($sql_string)) {
                print_error(__LINE__, $gDBClone{$dbname}->error());
                return(0);
            }
        }
    }

    return(1);
}

################################################################################
#
#  Object Group Update Subroutines
#

sub load_data($)
{
    my ($method) = @_;
    my $retval;

    print_debug("Running $method Data Method...\n");

    if ($method eq 'delete') {
        $retval = $gDB->data_factory('delete', \%DBData, 1);
    }
    else {
        $retval = $gDB->data_factory($method, \%DBData, 0);
    }

    unless (defined($retval)) {
        print_error(__LINE__, $gDB->error());
        return(0);
    }

    return(1);
}

sub load_updates_file($)
{
    my ($file) = @_;
    my $line;
    my @sql_strings;
    my $sql_string;
    my $index;
    my $dbname;
    my $href;

    print_debug("Loading Updates...\n");

    unless(open(FH, $file)) {
        print_error(__LINE__,
            "Could not open updates file: $!\n" .
            " -> $file\n");
        return(0);
    }

    @sql_strings = ();
    $sql_string  = '';
    $index       = 0;
    $dbname      = 'default';

    $sql_strings[$index]{'dbname'} = $dbname;

    foreach $line (<FH>) {
        if ($line =~ /^\\c\s+(\w+)/i ||
            $line =~ /^\\connect\s+(\w+)/i) {

            $dbname = $1;
            $index++;
            $sql_strings[$index]{'dbname'}     = $dbname;
            $sql_strings[$index]{'sql_string'} = '';
            $sql_strings[$index]{'found_sql'}  = 0;
        }
        elsif ($line =~ /^\s*CREATE\s+DATABASE\s+(\w+)/i) {
            unless (create_databases($line)) {
                return(0);
            }
        }
        else {
            $sql_strings[$index]{'sql_string'} .= $line;

            unless ($line =~ /^\s*$/ || $line =~ /^--/) {
                $sql_strings[$index]{'found_sql'} = 1;
            }
        }
    }

    close(FH);

    foreach $href (@sql_strings) {

        next unless ($href->{'found_sql'});

        $dbname     = $href->{'dbname'};
        $sql_string = $href->{'sql_string'};

        if ($dbname eq 'default') {
            unless($gDB->do($sql_string)) {
                print_error(__LINE__, $gDB->error());
                return(0);
            }
        }
        else {
            unless($gDBClone{$dbname}) {
                unless(clone_connection($dbname)) {
                    return(0);
                }
            }

            unless($gDBClone{$dbname}->do($sql_string)) {
                print_error(__LINE__, $gDBClone{$dbname}->error());
                return(0);
            }
        }
    }

    return(1);
}

################################################################################
#
#  Main Subroutines
#

sub read_data_file($)
{
    my ($file) = @_;
    my $retval;

    print_debug("Reading Data File:               $file\n");

    unless ($retval = do $file) {

        if ($@) {
            print_error(__LINE__,
                "Could not compile data file: $@\n" .
                " -> $file\n");
        }
        elsif (!defined($retval)) {
            print_error(__LINE__,
                "Could not read data file: $!\n" .
                " -> $file\n");
        }
        else {
            print_error(__LINE__,
                "Bad return value loading data file:\n" .
                " -> $file\n");
        }

        return(0);
    }

    return(1);
}

sub process_updates_file($$$$$$)
{
    my ($pkg_dir, $version_dir, $current_dir, $file, $method, $revert) = @_;
    my $updates_file = "$current_dir/$file";
    my ($extension)  = ($file =~ /\.(\w+)$/);
    my $retval;

    print_debug("Processing Updates File:         $updates_file\n");

    if ($extension eq 'data') {

        unless(read_data_file($updates_file)) {
            return(0);
        }

        if ($revert) {
            unless (revert_data($method)) {
                return(0);
            }
        }
        else {
            unless (load_data($method)) {
                return(0);
            }
        }
    }
    else {
        if ($revert) {
            unless (revert_updates_file($updates_file)) {
                return(0);
            }
        }
        else {
            unless (load_updates_file($updates_file)) {
                return(0);
            }
        }
    }

    return(1);
}

sub get_update_order_list($)
{
    my ($current_dir) = @_;
    my ($line, @line, @list, $count, @dir_list, $file);
    my $update_order_file = "$current_dir/$gUpdateOrderFile";

    @list  = ();
    $count = 0;

    if (-f $update_order_file) {

        print_debug("Parsing UPDATE_ORDER File:       $update_order_file\n");

        unless (open(FH, $update_order_file)) {
            print_error(__LINE__,
                "Could not open UPDATE_ORDER file: $!\n" .
                " -> $update_order_file\n");
            return(undef);
        }

        foreach $line (<FH>) {
            next if ($line =~ /^\s*$|^\s*#.*/); # Skip blank lines and comments
            $line =~ s/^\s+//;                  # Remove leading whitespace
            $line =~ s/\s+$//;                  # Remove trailing whitespace
            $line =~ s/\,/ /g;                  # Comma to space

            @line = split(/\s+/, $line);

            if ($line[0]) {
                $list[$count]{'file'} = $line[0];
                if ($line[1]) {
                    $list[$count]{'method'} = $line[1];
                }
            }

            $count++;
        }

        close(FH);
    }
    else {

        print_debug("Getting Directory List:          $current_dir\n");

        unless(opendir(DIR, $current_dir)) {
            print_error(__LINE__,
                "Could not open directory: $!\n" .
                " -> $current_dir\n");
            return(0);
        }
        @dir_list = sort(grep(!/^\./,readdir(DIR)));
        closedir(DIR);

        foreach $file (@dir_list) {

            # Skip RCS directories and the UPDATE_SUMMARY file.

            next if ($file eq 'RCS');
            next if ($file eq 'UPDATE_SUMMARY');

            $list[$count]{'file'}   = $file;
            $count++;
        }
    }

    return(@list);
}

sub process_updates_dir($$$$$)
{
    my ($pkg_dir, $version_dir, $current_dir, $method, $revert) = @_;
    my (@update_order, $href, $file, $my_method);

    print_debug("Processing Updates Directory:    $current_dir\n");

    @update_order = get_update_order_list($current_dir);

    if ($revert) {
        @update_order = reverse(@update_order);
    }

    foreach $href (@update_order) {

        $file      = $href->{'file'};
        $my_method = ($href->{'method'}) ? $href->{'method'} : $method;

        if (-d "$current_dir/$file") {
            unless(&process_updates_dir($pkg_dir, $version_dir,
                "$current_dir/$file", $my_method, $revert)) {
                return(0);
            }
        }
        else {
            unless(process_updates_file($pkg_dir, $version_dir,
                $current_dir, $file, $my_method, $revert)) {
                return(0);
            }
        }
    }

    return(1);
}

sub get_update_summary($)
{
    my ($path) = @_;
    my $file = "$path/$gUpdateSummaryFile";
    my ($line, $summary);

    print_debug("Getting Update Summary From:     $file\n");

    unless(open(FH, $file)) {
        print_error(__LINE__,
            "Could not open UPDATE_SUMMARY file: $!\n" .
            " -> $file\n");
        return(undef);
    }

    $summary = '';
    foreach $line (<FH>) {
        $summary .= $line;
    }

    close(FH);

    chomp($summary);

    return($summary);
}

sub version_compare($$$)
{
    my ($ver1,$op,$ver2) = @_;
    my @ver1 = split(/\./, $ver1);
    my @ver2 = split(/\./, $ver2);
    my $i;

    if ($ver1 eq $ver2) {
        return(1) if ($op =~ /=/);
    }

    for ($i = 0; $i < 3; $i++) {
        next if ($ver1[$i] == $ver2[$i]);

        if ($op =~ /</) {
            return(($ver1[$i] < $ver2[$i]) ? 1 : 0);
        }
        elsif ($op =~ />/) {
            return(($ver1[$i] > $ver2[$i]) ? 1 : 0);
        }
    }

    return(0);
}

sub rollback_transaction()
{
    my @dbclones = reverse(@gDBClones);
    my $dbname;

    foreach $dbname (@dbclones) {
        if ($gDBClone{$dbname}) {

            print_debug("ROLLBACK: $dbname\n");
            $gDBClone{$dbname}->rollback();

            print_debug("DISCONNECT: $dbname\n");
            $gDBClone{$dbname}->disconnect();
        }
    }

    print_debug("ROLLBACK\n");

    $gDB->rollback();

    if (@gCreatedDBs) {
        drop_databases(@gCreatedDBs);
        @gCreatedDBs = ();
    }

    @gDBClones = ();
    %gDBClone  = ();
    @gDropDBs  = ();
}

sub commit_transaction()
{
    my $dbname;

    print_debug("COMMIT\n");

    $gDB->commit();

    foreach $dbname (@gDBClones) {
        if ($gDBClone{$dbname}) {

            print_debug("COMMIT: $dbname\n");
            $gDBClone{$dbname}->commit();

            print_debug("DISCONNECT: $dbname\n");
            $gDBClone{$dbname}->disconnect();
        }
    }

    if (@gDropDBs) {
        drop_databases(@gDropDBs);
        @gDropDBs = ();
    }

    @gDBClones   = ();
    %gDBClone    = ();
    @gCreatedDBs = ();
}

sub db_disconnect()
{
    my @dbclones = reverse(@gDBClones);
    my $dbname;

    foreach $dbname (@dbclones) {
        if ($gDBClone{$dbname}) {

            print_debug("DISCONNECT: $dbname\n");
            $gDBClone{$dbname}->disconnect();
        }
    }

    print_debug("Disconnecting From Database\n");

    $gDB->disconnect();

    @gDBClones = ();
    %gDBClone  = ();
}

sub db_update($$$$$)
{
    my ($comp_name, $object_group, $pkg_name, $target_version, $revert) = @_;
    my (%versions, @versions, $version, $previous_version, %previous_version);
    my ($major,$minor,$micro);
    my $pkg_dir;
    my $start_version;
    my $found_version;
    my $update_summary;

    # Get the path to the object group package directory

    $pkg_dir = $gDB->get_object_group_pkg_dir($comp_name, $object_group, $pkg_name);
    unless ($pkg_dir) {
        print_error(__LINE__, $gDB->error());
        return(0);
    }

    unless (-d $pkg_dir) {
        print_error(__LINE__,
            "The '$pkg_name' object group package directory does not exist:\n" .
            "  -> $pkg_dir\n");
        return(0);
    }

    # Get the current object group package version.

    $start_version = get_object_group_package_version($object_group, $pkg_name);
    unless (defined($start_version)) {
        return(0);
    }

    # Get the list of version directories.

    print_debug("Scanning Package Directory:      $pkg_dir\n");

    unless(opendir(DIR, $pkg_dir)) {
        print_error(__LINE__,
            "Could not open package directory: $!\n" .
            " -> $pkg_dir\n");
        return(0);
    }
    @versions = sort(grep(!/^\./,readdir(DIR)));
    closedir(DIR);

    # Ensure proper version directory name format and order

    foreach $version (@versions) {

        next unless (-d "$pkg_dir/$version");

        if ($version =~ /^(\d+)\.(\d+)\.(\d+)$/) {
            $versions{$1}{$2}{$3} = 1;
        }
    }

    @versions         = ();
    $previous_version = '0.0.0';
    %previous_version = ();

    foreach $major (sort {$a <=> $b} keys(%versions)) {
    foreach $minor (sort {$a <=> $b} keys(%{$versions{$major}})) {
    foreach $micro (sort {$a <=> $b} keys(%{$versions{$major}{$minor}})) {

        $version = "$major.$minor.$micro";
        push(@versions, $version);

        $previous_version{$version} = $previous_version;
        $previous_version = $version;

    }}}

    # Reverse the versions order if reverting to a previous version.

    if ($revert) {
        @versions = reverse(@versions);
    }

    # Make sure the target version exists if one was specified.

    if ($target_version) {
        $found_version = 0;
        foreach $version (@versions) {
            if ($version eq $target_version) {
                $found_version = 1;
                last;
            }
        }
        unless ($found_version) {
            print_error(__LINE__,
                "The specified target version, '$target_version', could not be found.\n");
            return(0);
        }
    }

    # Loop over all version directories.

    foreach $version (@versions) {

        # If reverting skip versions greater than the current version.
        # Otherwise skip versions less than or equal to the current version.

        if ($revert) {
            next if (version_compare($version, '>', $start_version));
            if ($target_version) {
                last if ($version eq $target_version);
            }
        }
        else {
            next if (version_compare($version, '<=', $start_version));
        }

        # Get the update summary

        $update_summary = get_update_summary("$pkg_dir/$version");
        unless(defined($update_summary)) {
            return(0);
        }

        # If reverting the object group to a previous version, revert the
        # version before reverting the updates.  This is primarily needed
        # to prevent the "chicken and the egg" problem when reverting the
        # DBCORE Object Group tables to version 0.

        if ($revert) {
            $update_summary = "Reverted: $update_summary";
            unless(update_object_group_package_version(
                        $revert,
                        $object_group,
                        $pkg_name,
                        $previous_version{$version},
                        $update_summary)) {

                rollback_transaction();
                return(0);
            }
        }

        # Do the updates

        unless(process_updates_dir($pkg_dir, $version,
            "$pkg_dir/$version", 'define', $revert)) {
            rollback_transaction();
            return(0);
        }

        unless ($revert) {
            unless(update_object_group_package_version(
                        $revert,
                        $object_group,
                        $pkg_name,
                        $version,
                        $update_summary)) {

                rollback_transaction();
                return(0);
            }
        }

        commit_transaction();

        # We are finished if a target version was specified and it is
        # equal to the version just processed.

        if ($target_version) {
            last if ($version eq $target_version);
        }
    }

    return(1);
}

################################################################################
#
#  Main
#

GetOptions("comp=s", "d", "help", "pkg=s", "revert", "t=s", "v",
           "a=s", "f=s", "h=s", "n=s", "p=s", "u=s");

if ($opt_help) { usage(); }
if ($opt_v) {
    print("$gVersion\n");
    exit(0);
}

# Check if debug mode was specified

if ($opt_d) {
    $gDebug = 1;
    print_debug("Debug Mode Enabled\n");
}

# Set the component, package, and object group names

if ($ARGV[0]) {
    $gObjectGroup = $ARGV[0];
}
else {
    usage();
}

if ($opt_comp) {
    $gCompName = $opt_comp;
}

$gPkgName = ($opt_pkg) ? $opt_pkg : $gObjectGroup;

print_debug("Component Name:            $gCompName\n");
print_debug("Database Object Group:     $gObjectGroup\n");
print_debug("Package Name:              $gPkgName\n");

# Check if the revert option was specified

if ($opt_revert) {
    $gRevert = 1;
    print_debug("Reverting Updates\n");
}

# Check if a target version was specified

if ($opt_t) {
    $gTargetVersion = $opt_t;
    print_debug("Target Version:            $opt_t\n");
}

# Get any database connection options specified on the command line

if ($opt_a) {
    $gConnArgs{'DBAlias'} = $opt_a;
    print_debug("Database Connection Alias: $opt_a\n");
}

if ($opt_f) {
    $gConnArgs{'DBConnFile'} = $opt_f;
    print_debug("Database Connection File:  $opt_f\n");
}

if ($opt_h) {
    $gConnArgs{'DBHost'} = $opt_h;
    print_debug("Database Host:             $opt_h\n");
}

if ($opt_n) {
    $gConnArgs{'DBName'} = $opt_n;
    print_debug("Database Name:             $opt_n\n");
}

if ($opt_p) {
    $gConnArgs{'DBPass'} = $opt_p;
}

if ($opt_u) {
    $gConnArgs{'DBUser'} = $opt_u;
    print_debug("Database User:             $opt_u\n");
}

# Connect to the database

print_debug("Connecting To Database\n");

$gConnArgs{'Component'}   = 'dbcore';
$gConnArgs{'ObjectGroup'} = 'dbcore';

$gDB = DBCORE->new();
unless ($gDB->connect(\%gConnArgs)) {
    print_error(__LINE__, $gDB->error());
    exit(1);
}

# Make sure the required dbcore object group is up-to-date

unless (db_update('dbcore', 'dbcore', 'dbcore', 0, 0)) {
    exit(1);
}

# Load the Object Group Definitions file if one exists.
# This will only be needed to do data updates.

$gDefsFile = $gDB->get_defs_file($gCompName, $gObjectGroup);

if (-f $gDefsFile) {
    unless ($gDB->load_defs_file($gDefsFile)) {
        print_error(__LINE__, $gDB->error());
        exit(1);
    }
}

# Process the update files

unless ($gExitValue) {
    unless(db_update($gCompName,
                     $gObjectGroup,
                     $gPkgName,
                     $gTargetVersion,
                     $gRevert)) {
        $gExitValue = 1;
    }
}

# Disconnect from the database

db_disconnect();

exit($gExitValue);
