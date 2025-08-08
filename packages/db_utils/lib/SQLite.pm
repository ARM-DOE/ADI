################################################################################
#
#  COPYRIGHT (C) 2000 Battelle Memorial Institute.  All Rights Reserved.
#
################################################################################
#
#  Author:
#     name:  Brian Ermold
#     phone: (509) 375-2277
#     email: brian.ermold@arm.gov
#
################################################################################

package SQLite;

use strict;

use DBI;
use Time::Local;
use POSIX qw(:signal_h);

my $_Version = '@VERSION@';

sub new(\%) {
    my $class = shift;
    my $self  = {};

    undef($self->{'Error'});
    undef($self->{'DBH'});
    undef($self->{'DBName'});

    $self->{'RaiseError'} = 0;
    $self->{'AutoCommit'} = 0;
    $self->{'Timeout'}    = 15;

    $self->{'Debug'}      = 0;
    bless ($self, $class);
    return $self;
}

sub _init_db_method(\%)
{
    my $self = shift;

    undef($self->{'Error'});

    unless ($self->{'DBH'}) {
        $self->{'Error'} =
            "Database connection has not been established!\n";
        return(0);
    }

    return(1);
}

sub _set_db_error(\%$) {

    my ($self, $errstr) = @_;
    my $dbi_error = $DBI::errstr;
    
    chomp($errstr);
    $self->{'Error'} = $errstr . "\n";

    if ($dbi_error) {
        chomp($dbi_error);
        $self->{'Error'} .= " -> " . $dbi_error . "\n";
    }

    if (!$self->{'AutoCommit'}) {
        $self->{'DBH'}->rollback();
    }
}

sub set_timeout(\%$)
{
    my ($self, $timeout) = @_;

    $self->{'Timeout'} = $timeout;
}

sub connect(\%$\%)
{
    my ($self, $dbname, $href) = @_;
    my $dsn = "dbi:SQLite:dbname=$dbname";
    my $timeout = $self->{'Timeout'} * 1000;
    my $retval;

    if ($self->{'DBH'}) {
        $self->disconnect();
    }

    $self->{'DBName'} = $dbname;

    if ($href->{'RaiseError'}) {
        $self->{'RaiseError'} = $href->{'RaiseError'};
    }

    if ($href->{'AutoCommit'}) {
        $self->{'AutoCommit'} = $href->{'AutoCommit'};
    }

    if ($href->{'Timeout'}) {
        $self->{'Timeout'} = $href->{'Timeout'};
    }

    undef($self->{'Error'});

    $self->{'DBH'} = DBI->connect($dsn, "", "",
                    { 'RaiseError' => $self->{'RaiseError'},
                      'AutoCommit' => $self->{'AutoCommit'},
                      'sqlite_use_immediate_transaction' => 1
                    });

    unless ($self->{'DBH'}) {
        $self->_set_db_error("Could not connect to database: $dbname\n");
        return(undef);
    }

    # Set the busy timeout interval.

    $self->{'DBH'}->sqlite_busy_timeout($timeout);

    # The following option needs to be set to disable the effect of
    # destroying a database handle when the calling application forks
    # off a child process

    $self->{'DBH'}->{'InactiveDestroy'} = 1;

    return($self->{'DBH'});
}

sub is_connected(\%)
{
    my $self = shift;

    if ($self->{'DBH'}) {
        my $state = $self->{'DBH'}->ping();
        return(1) if ($state);
    }

    return(0);
}

sub ping(\%)
{
    my $self = shift;

    if ($self->{'DBH'}) {
        my $state = $self->{'DBH'}->ping();
        return($state);
    }

    return(0);
}

sub disconnect(\%)
{
    my $self = shift;

    if ($self->{'DBH'}) {
        $self->{'DBH'}->disconnect();
    }

    delete($self->{'DBH'});
}

sub reconnect(\%)
{
    my $self = shift;
    my ($dbname, %conn_hash);
    my $retval;

    $dbname = $self->{'DBName'};

    $conn_hash{'RaiseError'} = $self->{'RaiseError'};
    $conn_hash{'AutoCommit'} = $self->{'AutoCommit'};
    $conn_hash{'Timeout'}    = $self->{'Timeout'};

    $retval = $self->connect($dbname, \%conn_hash);

    return($retval);
}

sub clone
{
    my $self  = shift;
    my $href  = ($_[0]) ? shift : ();
    my $clone = PGSQL->new();
    my ($dbhost, $dbname, $dbuser, $dbpass, %conn_hash);
    my $retval;

    $dbname = ($href->{'DBName'}) ? $href->{'DBName'} : $self->{'DBName'};

    $conn_hash{'RaiseError'} = ($href->{'RaiseError'}) ? $href->{'RaiseError'} : $self->{'RaiseError'};
    $conn_hash{'AutoCommit'} = ($href->{'AutoCommit'}) ? $href->{'AutoCommit'} : $self->{'AutoCommit'};
    $conn_hash{'Timeout'}    = ($href->{'Timeout'})    ? $href->{'Timeout'}    : $self->{'Timeout'};

    $retval = $clone->connect($dbname, \%conn_hash);

    unless ($retval) {
        $self->{'Error'} =
            "Could not clone database connection:\n" .
            $clone->error();
        undef($clone);
        return(undef);
    }

    return($clone);
}

sub db_host(\%)
{
    my $self = shift;
    return("");
}

sub db_name(\%)
{
    my $self = shift;
    return($self->{'DBName'});
}

sub db_user(\%)
{
    my $self = shift;
    return("");
}

sub error(\%)
{
    my $self = shift;

    return($self->{'Error'});
}

sub commit(\%)
{
    my $self = shift;

    if ($self->{'DBH'}) {
        $self->{'DBH'}->commit();
    }
}

sub rollback(\%)
{
    my $self = shift;

    if ($self->{'DBH'}) {
        $self->{'DBH'}->rollback();
    }
}

###############################################################################
#
#  Execute Methods
#

sub build_query_string(\%\%)
{
  # $sp_def is a refrense to a hash of the form:
  #
  # {
  #   'proc' => 'inquire_site',
  #   'args' => ['site'],
  #   'rets' => ['site', 'description'],
  # }
  #
    my ($self, $sp_def) = @_;
    my ($nargs, $query);
    my $pn;

    $nargs = defined($sp_def->{'args'}) ? @{$sp_def->{'args'}} : 0;
    $query = defined($sp_def->{'rets'}) ? "SELECT * FROM $sp_def->{'proc'}" :
                                          "SELECT $sp_def->{'proc'}";
    $query .= '(';
    if ($nargs) {
        for ($pn = 1; $pn <= $nargs; $pn++) {
            $query .= "\$$pn,";
        }
        chop($query);
    }

    $query .= ')';

    return($query);
}

sub _get_stored_procedure(\%$)
{
    my ($self,$command) = @_;
    my $sp;
    my $query;
    my $sth;
    my $retval;
    my $args;
    my @args;
    my $i;

    return(undef) unless($self->_init_db_method());

    if ($self->{'Debug'}) {
        print STDERR "COMMAND: ", $command, "\n";
    }

#    ($sp) = ($command =~ /\s(\S+)\s*\(\s*[\$\?\)]/);
    ($sp,$args) = ($command =~ /\s(\S+)\s*\(\s*(.*?)\s*\)/);

    if ($sp) {

        if ($args) {
            $args =~ s/\s+//g;
            @args = split(/\s*,\s*/, $args);
            for ($i = 1; $i <= @args; $i++) {
                $args[$i-1] = "\$$i";
            }
            $args = join(",", @args);
        }
        else {
            $args = '';
        }

        if ($self->{'Debug'}) {
            print STDERR "SP:      ", $sp,   "\n";
            print STDERR "ARGS:    '", $args, "'\n";
        }

#        $query = "SELECT sp_query FROM stored_procedures WHERE sp_command LIKE '% $sp(%'";
        $query = "SELECT sp_query FROM stored_procedures WHERE sp_command LIKE '% $sp($args)%'";

        if ($self->{'Debug'}) {
            print STDERR "QUERY:   ", $query, "\n";
        }

        $sth   = $self->{'DBH'}->prepare_cached($query);
        if (!$sth) {
            $self->_set_db_error("Could not prepare query: '$query'\n");
            return(undef);
        }

        $retval = $sth->execute();
        unless (defined($retval)) {
            $self->_set_db_error("Could not execute query: '$query'\n");
            return(undef);
        }

        $retval = $sth->fetchrow();
        if ($sth->err) {
            $self->_set_db_error("Could not fetchrow from query: '$query'\n");
            return(undef);
        }

        $sth->finish();
    }

    return(($retval) ? $retval : "");
}

sub _expand_sp_args(\%$$)
{
    my ($self,$sp,$args) = @_;
    my $pn = @{$args};
    my $arg;
    my $tmp;

    foreach $arg (reverse(@{$args})) {
        if (defined($arg)) {
            $tmp = $arg;
            $tmp =~ s/\"/""/g;
            $sp  =~ s/\$$pn/"$tmp"/g;
        }
        else {
            $sp =~ s/\$$pn/NULL/g;
        }
        
        $pn -= 1;
    }

    return($sp);
}

sub _execute_query(\%$$)
{
    my ($self,$query,$args) = @_;
    my $orig_query = $query;
    my $sth;
    my $sp;
    my @statements;
    my $statement;
    my $retval;

    $sp = $self->_get_stored_procedure($query);
    return(undef) unless(defined($sp));

    if ($sp) {
        $query = $sp;
#        if($args) {
#            $query = $self->_expand_sp_args($sp, $args);
#        }
#        else {
#            $query = $sp;
#        }
    }

    if ($self->{'Debug'}) {
        print STDERR "----- SQL String -----\n";
        print STDERR "$query\n";
    }

    $query =~ s/\s*;\s*$//;

    @statements = split(/\s*;\s*/, $query);
    $query      = pop(@statements);

    foreach $statement (@statements) {

        if ($args) {
            $statement = $self->_expand_sp_args($statement, $args);
        }

        if ($self->{'Debug'}) {
            print STDERR "----- Statement -----\n";
            print STDERR "$statement\n";
        }

        $sth = $self->{'DBH'}->prepare_cached($statement);
        unless ($sth) {
            if ($sp) {
                $self->_set_db_error(
                    "Could not prepare statement:\n'$statement'\n" .
                    " -> in stored procedure: '$orig_query'\n");
            }
            else {
                $self->_set_db_error("Could not prepare query: '$query'\n");
            }
            return(undef);
        }

        if ($sp) {
            $retval = $sth->execute();
        }
        else {
            $retval = $sth->execute(@{$args})
        }

        unless ($retval) {
            if ($sp) {
                $self->_set_db_error(
                    "Could not execute statement:\n'$statement'\n" .
                    " -> in stored procedure: '$orig_query'\n");
            }
            else {
                $self->_set_db_error("Could not execute query:\n'$query'\n");
            }
            return(undef);
        }

        $sth->finish;
    }

    if ($args) {
        $query = $self->_expand_sp_args($query, $args);
    }

    if ($self->{'Debug'}) {
        print STDERR "----- Query -----\n";
        print STDERR "$query\n";
        print STDERR "-----------------\n";
    }

    $sth = $self->{'DBH'}->prepare_cached($query);
    unless ($sth) {
        if ($sp) {
            $self->_set_db_error(
                "Could not prepare query:\n'$query'\n" .
                " -> in stored procedure: '$orig_query'\n");
        }
        else {
            $self->_set_db_error("Could not prepare query: '$query'\n");
        }
        return(undef);
    }

    if ($sp) {
        $retval = $sth->execute();
    }
    else {
        $retval = $sth->execute(@{$args})
    }

    unless ($retval) {
        if ($sp) {
            $self->_set_db_error(
                "Could not execute query:\n'$query'\n" .
                " -> in stored procedure: '$orig_query'\n");
        }
        else {
            $self->_set_db_error("Could not execute query:\n'$query'\n");
        }
        return(undef);
    }

    return($sth);
}

sub do
{
    my ($self, $sql_string, $args) = @_;
    my $retval;

    $retval = $self->_get_stored_procedure($sql_string);
    return(undef) unless(defined($retval));

    if ($retval) {
        $sql_string = $retval;
    }

    if (defined($args)) {
        $retval = $self->{'DBH'}->do($sql_string, undef, @{$args});
    }
    else {
        $retval = $self->{'DBH'}->do($sql_string);
    }

    unless (defined($retval)) {
        $self->_set_db_error("Could not execute SQL string:\n'$sql_string'\n");
        return(undef);
    }

    return($retval);
}

###############################################################################
#
#  Query Methods
#

sub query(\%$$)
{
    my ($self,$query,$args) = @_;
    my ($sth,@row,$i);
    my @array = ();

    $sth = $self->_execute_query($query,$args);
    return(undef) unless ($sth);

    for ($i = 0; @row = $sth->fetchrow_array(); $i++) {
        if ($sth->err) {
            $self->_set_db_error("Could not fetchrow from query: '$query'\n");
            return(undef);
        }
        @{$array[$i]} = @row;
    }

    $sth->finish();

    return(\@array);
}

sub query_hasharray(\%$$$)
{
    my ($self,$query,$args,$cols) = @_;
    my ($sth,@row,$i,$j,$col);
    my @array = ();

    $sth = $self->_execute_query($query,$args);
    return(undef) unless ($sth);

    for ($i = 0; @row = $sth->fetchrow_array(); $i++) {
        if ($sth->err) {
            $self->_set_db_error("Could not fetchrow from query: '$query'\n");
            return(undef);
        }
        for ($j = 0; $col = @{$cols}[$j]; $j++) {
            $array[$i]{$col} = $row[$j];
        }
    }

    $sth->finish();

    return(\@array);
}

sub query_scalar(\%$$)
{
    my ($self,$query,$args) = @_;
    my $sth;
    my $retval;

    $sth = $self->_execute_query($query,$args);
    return(undef) unless ($sth);

    $retval = $sth->fetchrow();

    if ($sth->err) {
        $self->_set_db_error("Could not fetchrow from query: '$query'\n");
        return(undef);
    }

    $sth->finish();

    if ($self->{'Debug'}) {
        if (defined($retval)) {
            print STDERR "RETVAL: '$retval'\n";
        }
        else {
            print STDERR "RETVAL: undef\n";
        }
    }

    return($retval);
}

sub is_procedure(\%$)
{
    my ($self,$command) = @_;
    my $query;
    my $sth;
    my $retval;

    return(undef) unless($self->_init_db_method());

    $query = "SELECT sp_command FROM stored_procedures WHERE sp_command LIKE '% $command(%'";
    $sth   = $self->{'DBH'}->prepare_cached($query);
    if (!$sth) {
        $self->_set_db_error("Could not prepare query: '$query'\n");
        return(undef);
    }

    $retval = $sth->execute();
    unless (defined($retval)) {
        $self->_set_db_error("Could not execute query: '$query'\n");
        return(undef);
    }

    $retval = $sth->fetchrow();
    if ($sth->err) {
        $self->_set_db_error("Could not fetchrow from query: '$query'\n");
        return(undef);
    }

    $sth->finish();

    return(($retval) ? 1 : 0);
}

sub is_table(\%$)
{
    my $self = shift;
    $self->{'Error'} = "Not implemented for SQLite: is_table\n";
    return(undef);
}

###############################################################################
#
#  Comment Retrieving Methods
#

sub class_comment(\%$)
{
    my $self = shift;
    $self->{'Error'} = "Not implemented for SQLite: class_comment\n";
    return(undef);
}

sub procedure_comment(\%$)
{
    my $self = shift;
    $self->{'Error'} = "Not implemented for SQLite: procedure_comment\n";
    return(undef);
}

sub type_comment(\%$)
{
    my $self = shift;
    $self->{'Error'} = "Not implemented for SQLite: type_comment\n";
    return(undef);
}

###############################################################################
#
#  Data Dictionary Methods
#

sub type_attributes(\%$)
{
    my $self = shift;
    $self->{'Error'} = "Not implemented for SQLite: type_attributes\n";
    return(undef);
}

sub type_definition(\%$)
{
    my $self = shift;
    $self->{'Error'} = "Not implemented for SQLite: type_definition\n";
    return(undef);
}

sub table_columns(\%$)
{
    my $self = shift;
    $self->{'Error'} = "Not implemented for SQLite: table_columns\n";
    return(undef);
}

sub table_constraints(\%$)
{
    my $self = shift;
    $self->{'Error'} = "Not implemented for SQLite: table_constraints\n";
    return(undef);
}

sub table_definition(\%$)
{
    my $self = shift;
    $self->{'Error'} = "Not implemented for SQLite: table_definition\n";
    return(undef);
}

###############################################################################
#
#  Time Convertion Methods
#

sub seconds_to_timestamp(\%$)
{
    my ($self,$seconds) = @_;
    my $secs = int($seconds);
    my $usec = int(($seconds - $secs) * 1000000);
    my @gmt  = gmtime($secs);
    my $timestamp;

    $gmt[5] += 1900;
    $gmt[4]++;

    $timestamp = sprintf("%d-%02d-%02d %02d:%02d:%02d.%06d",
        $gmt[5], $gmt[4], $gmt[3], $gmt[2], $gmt[1], $gmt[0], $usec);

    $timestamp =~ s/0+$//;
    $timestamp =~ s/\.$//;

    return($timestamp);
}

sub timestamp_to_seconds(\%$)
{
    my ($self,$timestamp) = @_;
    my $seconds = 0;
    my @gmt;

    undef($self->{'Error'});

    if ($timestamp =~ /^(\d+)-(\d+)-(\d+)\s+(\d+):(\d+):(\d+)(\.\d+)*/) {
        @gmt = ($6,$5,$4,$3,$2-1,$1-1900);
        $seconds  = timegm(@gmt);
        $seconds += $7 if ($7);
    }
    else {
        $self->{'Error'} = "Invalid timestamp format: $timestamp\n";
    }

    return($seconds);
}

1;
