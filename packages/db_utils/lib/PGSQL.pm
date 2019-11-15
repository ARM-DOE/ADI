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

package PGSQL;

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
    undef($self->{'DBHost'});
    undef($self->{'DBName'});
    undef($self->{'DBUser'});
    undef($self->{'DBPass'});

    $self->{'RaiseError'} = 0;
    $self->{'AutoCommit'} = 0;
    $self->{'PrintError'} = 0;
    $self->{'MinMessage'} = 'WARNING';
    $self->{'Timeout'}    = 30;
    $self->{'pg_server_prepare'} = 0;

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

sub _enable_timeout_alarm(\%$$)
{
    my ($self, $timeout, $sth) = @_;
    my $mask    = POSIX::SigSet->new( SIGALRM );
    my $action;

    if ($sth) {
        $action = POSIX::SigAction->new(
            sub { $sth->cancel; die "TIMEOUT\n"; }, $mask);
    }
    else {
        $action = POSIX::SigAction->new(
            sub { die "TIMEOUT\n"; }, $mask);
    }

    my $oldaction = POSIX::SigAction->new();

    sigaction( SIGALRM, $action, $oldaction);

    alarm($timeout);

    return($oldaction);
}

sub _disable_timeout_alarm(\%$)
{
    my ($self, $oldaction) = @_;
    alarm(0);
    sigaction( SIGALRM, $oldaction );
}

sub set_message_level(\%$)
{
    my ($self, $msg_level) = @_;
    my ($query, @args, $retval);

    # DEBUG5, DEBUG4, DEBUG3, DEBUG2, DEBUG1, LOG, NOTICE, WARNING, ERROR
    $query   = "SELECT set_config('client_min_messages',?,FALSE);";
    $args[0] = $msg_level;
    $retval  = $self->query_scalar($query, \@args);

# This seem to fail occationally for some unknown reason but we
# do not want to fail when it does.
#
#    unless($retval) {
#        $self->{'Error'} =
#            "Could not set minimum client messages.\n" .
#            " -> $DBI::errstr\n";
#        return(0);
#    }

    return(1);
}

sub set_timeout(\%$)
{
    my ($self, $timeout) = @_;

    $self->{'Timeout'} = $timeout;
}

sub connect(\%$$$$\%)
{
    my ($self, $dbhost, $dbname, $dbuser, $dbpass, $href) = @_;
    my ($host, $port, $db);
    my $retval;
    my $errstr;

    if ($self->{'DBH'}) {
        $self->disconnect();
    }

    $self->{'DBHost'} = $dbhost;
    $self->{'DBName'} = $dbname;
    $self->{'DBUser'} = $dbuser;
    $self->{'DBPass'} = $dbpass;

    ($host, $port) = split(/:/, $dbhost);
    if ($port) {
        $db = "dbi:Pg:dbname=${dbname};host=$host;port=$port";
    }
    else {
        $db = "dbi:Pg:dbname=${dbname};host=${dbhost}";
    }

    if ($href->{'RaiseError'}) {
        $self->{'RaiseError'} = $href->{'RaiseError'};
    }

    if ($href->{'AutoCommit'}) {
        $self->{'AutoCommit'} = $href->{'AutoCommit'};
    }

    if ($href->{'PrintError'}) {
        $self->{'PrintError'} = $href->{'PrintError'};
    }

    if ($href->{'MinMessage'}) {
        $self->{'MinMessage'} = $href->{'MinMessage'};
    }

    if ($href->{'Timeout'}) {
        $self->{'Timeout'} = $href->{'Timeout'};
    }

    if ($href->{'pg_server_prepare'}) {
        $self->{'pg_server_prepare'} = $href->{'pg_server_prepare'};
    }

    $ENV{'PGCONNECT_TIMEOUT'} = $self->{'Timeout'};

    undef($self->{'Error'});

    $self->{'DBH'} = DBI->connect($db, $dbuser, $dbpass,
                    { 'RaiseError' => $self->{'RaiseError'},
                      'AutoCommit' => $self->{'AutoCommit'},
                      'PrintError' => $self->{'PrintError'},
                      'pg_server_prepare' => $self->{'pg_server_prepare'}
                    });

    unless ($self->{'DBH'}) {
        chomp($errstr = $DBI::errstr);
        $self->{'Error'} =
            "User '$dbuser' can not connect to database '$dbname' on host '$dbhost'\n" .
            " -> $errstr\n";
        return(undef);
    }

    if ($self->{'MinMessage'}) {
        unless($self->set_message_level($self->{'MinMessage'})) {
            $self->disconnect();
            return(undef);
        }
    }

    # The following option needs to be set to disable the effect of
    # destroying a database handle when the calling application forks
    # off a child process

# Our version of DBD:Pg is too old to use this:
#
#    $self->{'DBH'}->{'AutoInactiveDestroy'} = 1;

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

    undef $self->{'DBH'};
}

sub reconnect(\%)
{
    my $self = shift;
    my ($dbhost, $dbname, $dbuser, $dbpass, %conn_hash);
    my $retval;

    $dbhost = $self->{'DBHost'};
    $dbname = $self->{'DBName'};
    $dbuser = $self->{'DBUser'};
    $dbpass = $self->{'DBPass'};

    $conn_hash{'RaiseError'} = $self->{'RaiseError'};
    $conn_hash{'AutoCommit'} = $self->{'AutoCommit'};
    $conn_hash{'PrintError'} = $self->{'PrintError'};
    $conn_hash{'MinMessage'} = $self->{'MinMessage'};
    $conn_hash{'Timeout'}    = $self->{'Timeout'};

    $retval = $self->connect($dbhost, $dbname, $dbuser, $dbpass, \%conn_hash);

    return($retval);
}

sub clone
{
    my $self  = shift;
    my $href  = ($_[0]) ? shift : ();
    my $clone = PGSQL->new();
    my ($dbhost, $dbname, $dbuser, $dbpass, %conn_hash);
    my $retval;

    $dbhost = ($href->{'DBHost'}) ? $href->{'DBHost'} : $self->{'DBHost'};
    $dbname = ($href->{'DBName'}) ? $href->{'DBName'} : $self->{'DBName'};
    $dbuser = ($href->{'DBUser'}) ? $href->{'DBUser'} : $self->{'DBUser'};
    $dbpass = ($href->{'DBPass'}) ? $href->{'DBPass'} : $self->{'DBPass'};

    $conn_hash{'RaiseError'} = ($href->{'RaiseError'}) ? $href->{'RaiseError'} : $self->{'RaiseError'};
    $conn_hash{'AutoCommit'} = ($href->{'AutoCommit'}) ? $href->{'AutoCommit'} : $self->{'AutoCommit'};
    $conn_hash{'PrintError'} = ($href->{'PrintError'}) ? $href->{'PrintError'} : $self->{'PrintError'};
    $conn_hash{'MinMessage'} = ($href->{'MinMessage'}) ? $href->{'MinMessage'} : $self->{'MinMessage'};
    $conn_hash{'Timeout'}    = ($href->{'Timeout'})    ? $href->{'Timeout'}    : $self->{'Timeout'};

    $retval = $clone->connect($dbhost, $dbname, $dbuser, $dbpass, \%conn_hash);

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
    return($self->{'DBHost'});
}

sub db_name(\%)
{
    my $self = shift;
    return($self->{'DBName'});
}

sub db_user(\%)
{
    my $self = shift;
    return($self->{'DBUser'});
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

sub do(\%$)
{
    my ($self, $sql_string) = @_;
    my $retval;
    my $errstr;

    return(undef) unless($self->_init_db_method());

    eval {
        my $oldaction = $self->_enable_timeout_alarm($self->{'Timeout'}, undef);

        $retval = $self->{'DBH'}->do($sql_string);

        $self->_disable_timeout_alarm($oldaction);

        unless (defined($retval)) {
            chomp($errstr = $DBI::errstr);
            die $errstr . "\n" if ($errstr);
        }
    };

    if ($@) {
        $errstr = $@;
        chomp($errstr);
        $self->{'Error'} =
            "Could not execute SQL string:\n$sql_string\n -> $errstr\n";

        if (!$self->{'AutoCommit'}) {
            $self->{'DBH'}->rollback();
        }

        return(undef);
    }

    return($retval);
}

###############################################################################
#
#  Query Methods
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

    $nargs = defined($sp_def->{'args'}) ? @{$sp_def->{'args'}} : 0;
    $query = defined($sp_def->{'rets'}) ? "SELECT * FROM $sp_def->{'proc'}" :
                                          "SELECT $sp_def->{'proc'}";
    $query .= '(';
    if ($nargs) {
        $query .= '?';
        for ($nargs -= 1; $nargs; $nargs--) {
            $query .= ',?';
        }
    }
    $query .= ')';

    return($query);
}

sub _expand_query_args(\%$)
{
    my ($self,$args) = @_;
    my $string = '';

    foreach my $arg (@{$args}) {
        $string .= "\'$arg\',";
    }
    chop($string);
    return($string);
}

sub _not_null_row_bug(\%$)
{
    my ($self,$error) = @_;
    my $bug_str = "ERROR:  function returning row cannot return null value";

    if ($error eq $bug_str) {
        return(0);
    }

    return(1);
}

sub query(\%$$)
{
    my ($self,$query,$args) = @_;
    my ($sth,@row,$i);
    my @array = ();
    my $retval;
    my $errstr;

    return(undef) unless($self->_init_db_method());

    $sth = $self->{'DBH'}->prepare_cached($query);
    if (!$sth) {
        $self->{'Error'} =
            "Could not prepare query: '$query'\n -> $DBI::errstr\n";
        return(undef);
    }

    eval {
        my $oldaction = $self->_enable_timeout_alarm($self->{'Timeout'}, $sth);

        $retval = $sth->execute(@{$args});

        $self->_disable_timeout_alarm($oldaction);

        if (defined($retval)) {
            for ($i = 0; @row = $sth->fetchrow_array(); $i++) {
                @{$array[$i]} = @row;
            }
        }
        elsif ($self->_not_null_row_bug($DBI::errstr)) {
            chomp($errstr = $DBI::errstr);
            die $errstr . "\n" if ($errstr);
        }
    };

    $sth->finish();

    if ($@) {
        $errstr = $@;
        chomp($errstr);
        unless ($errstr) {
            $errstr = "query args: " . $self->_expand_query_args($args) . "\n";
        }

        $self->{'Error'} =
            "Could not execute query: '$query'\n -> $errstr\n";

        if (!$self->{'AutoCommit'}) {
            $self->{'DBH'}->rollback();
        }

        return(undef);
    }

    return(\@array);
}

sub query_hasharray(\%$$$)
{
    my ($self,$query,$args,$cols) = @_;
    my ($sth,@row,$i,$j,$col);
    my @array = ();
    my $retval;
    my $errstr;

    return(undef) unless($self->_init_db_method());

    $sth = $self->{'DBH'}->prepare_cached($query);
    if (!$sth) {
        $self->{'Error'} =
            "Could not prepare query: '$query'\n -> $DBI::errstr\n";
        return(undef);
    }

    eval {
        my $oldaction = $self->_enable_timeout_alarm($self->{'Timeout'}, $sth);

        $retval = $sth->execute(@{$args});

        $self->_disable_timeout_alarm($oldaction);

        if (defined($retval)) {
            for ($i = 0; @row = $sth->fetchrow_array(); $i++) {
                for ($j = 0; $col = @{$cols}[$j]; $j++) {
                    $array[$i]{$col} = $row[$j];
                }
            }
        }
        elsif ($self->_not_null_row_bug($DBI::errstr)) {
            chomp($errstr = $DBI::errstr);
            die $errstr . "\n" if ($errstr);
        }
    };

    $sth->finish();

    if ($@) {
        $errstr = $@;
        chomp($errstr);
        unless ($errstr) {
            $errstr = "query args: " . $self->_expand_query_args($args) . "\n";
        }

        $self->{'Error'} =
            "Could not execute query: '$query'\n -> $errstr\n";

        if (!$self->{'AutoCommit'}) {
            $self->{'DBH'}->rollback();
        }

        return(undef);
    }

    return(\@array);
}

sub query_scalar(\%$$)
{
    my ($self,$query,$args) = @_;
    my $sth;
    my $retval;
    my $errstr;

    return(undef) unless($self->_init_db_method());

    $sth = $self->{'DBH'}->prepare_cached($query);
    if (!$sth) {
        $self->{'Error'} =
            "Could not prepare query: '$query'\n -> $DBI::errstr\n";
        return(undef);
    }

    eval {
        my $oldaction = $self->_enable_timeout_alarm($self->{'Timeout'}, $sth);

        $retval = $sth->execute(@{$args});

        $self->_disable_timeout_alarm($oldaction);

        if (defined($retval)) {
            $retval = $sth->fetchrow();
            if (!defined($retval) && defined($DBI::errstr)) {
                chomp($errstr = $DBI::errstr);
                die $errstr . "\n" if ($errstr);
            }
        }
        else {
            chomp($errstr = $DBI::errstr);
            die $errstr . "\n" if ($errstr);
        }
    };

    $sth->finish();

    if ($@) {
        $errstr = $@;
        chomp($errstr);
        unless ($errstr) {
            $errstr = "query args: " . $self->_expand_query_args($args) . "\n";
        }

        $self->{'Error'} =
            "Could not execute query: '$query'\n -> $errstr\n";

        if (!$self->{'AutoCommit'}) {
            $self->{'DBH'}->rollback();
        }

        return(undef);
    }

    return($retval);
}

sub is_procedure(\%$)
{
    my ($self, $process_name) = @_;
    my $query = "SELECT * FROM pg_proc WHERE proname='$process_name';";
    my $array;

    $array = $self->query($query, undef);

    unless ($array) {
        return(undef);
    }

    unless (@{$array}) {
        return(0);
    }

    return(1);
}

sub is_table(\%$)
{
    my ($self, $table_name) = @_;
    my $query = "SELECT * FROM pg_tables WHERE tablename='$table_name';";
    my $array;

    $array = $self->query($query, undef);

    unless ($array) {
        return(undef);
    }

    unless (@{$array}) {
        return(0);
    }

    return(1);
}

###############################################################################
#
#  Comment Retrieving Methods
#

sub class_comment(\%$)
{
    my ($self, $table_name) = @_;
    my $query = "SELECT obj_description('$table_name'::regclass, 'pg_class');";
    my $comment;

    $comment = $self->query_scalar($query,undef);

    return($comment);
}

sub procedure_comment(\%$)
{
    my ($self, $proc_name) = @_;
    my $query = "SELECT obj_description('$proc_name'::regprocedure, 'pg_proc');";
    my $comment;

    $comment = $self->query_scalar($query,undef);

    return($comment);
}

sub type_comment(\%$)
{
    my ($self, $type_name) = @_;
    my $query = "SELECT obj_description('$type_name'::regtype, 'pg_type');";
    my $comment;

    $comment = $self->query_scalar($query,undef);

    return($comment);
}

###############################################################################
#
#  Data Dictionary Methods
#

sub type_attributes(\%$)
{
    my ($self, $type_name) = @_;
    my $query;
    my $array;
    my @atts;
    my $row;
    my $ri;

    $query = "SELECT attname,atttypid,atttypmod " .
             "FROM pg_attribute " .
             "WHERE attrelid='$type_name'::regclass " .
             "ORDER BY attnum;";

    $array = $self->query($query, undef);

    unless (defined($array)) {
        return(undef);
    }

    $ri   = 0;
    @atts = ();

    foreach $row (@{$array}) {

        $atts[$ri]{'name'} = @{$row}[0];

        $query = "SELECT format_type(@{$row}[1], @{$row}[2]);";

        $atts[$ri]{'type'} = $self->query_scalar($query,undef);

        unless (defined($atts[$ri]{'type'})) {
            return(undef);
        }

        $ri++;
    }

    return(\@atts);
}

sub type_definition(\%$)
{
    my ($self, $type_name) = @_;
    my ($query, $array, $row);
    my %type_def;

    $type_def{'name'} = $type_name;

    $type_def{'comment'} = $self->type_comment($type_name);

    $type_def{'attributes'} = $self->type_attributes($type_name);

    unless(defined($type_def{'attributes'})) {
        return(undef);
    }

    return(\%type_def);
}

sub table_columns(\%$)
{
    my ($self, $table_name) = @_;
    my $query;
    my $array;
    my @columns;
    my $row;
    my $ri;

    $query = "SELECT * FROM information_schema.columns " .
             "WHERE table_name='$table_name' " .
             "ORDER BY ordinal_position;";

    $array = $self->query($query, undef);

    unless (defined($array)) {
        return(undef);
    }

    $ri      = 0;
    @columns = ();
    foreach $row (@{$array}) {

        $columns[$ri]{'table_catalog'}    = @{$row}[0];
        $columns[$ri]{'table_schema'}     = @{$row}[1];
        $columns[$ri]{'table_name'}       = @{$row}[2];
        $columns[$ri]{'column_name'}      = @{$row}[3];
        $columns[$ri]{'ordinal_position'} = @{$row}[4];
        $columns[$ri]{'column_default'}   = @{$row}[5];
        $columns[$ri]{'is_nullable'}      = @{$row}[6];

        if (@{$row}[8]) {
            $columns[$ri]{'data_type'} = "@{$row}[7](@{$row}[8])";
        }
        else {
            $columns[$ri]{'data_type'} = "@{$row}[7]";
        }

        $query = "SELECT col_description('$table_name'::regclass, @{$row}[4])";
        $columns[$ri]{'comment'} = $self->query_scalar($query,undef);

        $ri++;
    }

    return(\@columns);
}

sub table_constraints(\%$)
{
    my ($self, $table_name) = @_;
    my ($query, $array, $row);
    my (%constraints, $contype, $conoid);
    my ($count, $check, $foreign_key, $primary_key, $unique);

    $query = "SELECT * FROM information_schema.table_constraints " .
             "WHERE table_name='$table_name' " .
             "ORDER BY constraint_name;";

    $array = $self->query($query, undef);

    unless (defined($array)) {
        return(undef);
    }

    $check       = 0;
    $foreign_key = 0;
    $primary_key = 0;
    $unique      = 0;
    %constraints = ();

    foreach $row (@{$array}) {

        $query = "SELECT oid FROM pg_constraint " .
                 "WHERE conrelid='$table_name'::regclass " .
                 "AND   conname='@{$row}[2]';";

        $conoid = $self->query_scalar($query,undef);

        unless(defined($conoid)) {
            next;
        }

        $contype = @{$row}[6];

        if ($contype eq 'CHECK') {
            $contype = 'check';
            $count = $check;
            $check++;
        }
        elsif ($contype eq 'FOREIGN KEY') {
            $contype = 'foreign_key';
            $count = $foreign_key;
            $foreign_key++;
        }
        elsif ($contype eq 'PRIMARY KEY') {
            $contype = 'primary_key';
            $count = $primary_key;
            $primary_key++;
        }
        elsif ($contype eq 'UNIQUE') {
            $contype = 'unique';
            $count = $unique;
            $unique++;
        }

        $constraints{$contype}[$count]{'constraint_catalog'} = @{$row}[0];
        $constraints{$contype}[$count]{'constraint_schema'}  = @{$row}[1];
        $constraints{$contype}[$count]{'constraint_name'}    = @{$row}[2];
        $constraints{$contype}[$count]{'table_catalog'}      = @{$row}[3];
        $constraints{$contype}[$count]{'table_schema'}       = @{$row}[4];
        $constraints{$contype}[$count]{'table_name'}         = @{$row}[5];

        $query = "SELECT pg_get_constraintdef($conoid,TRUE);";

        $constraints{$contype}[$count]{'definition'}
             = $self->query_scalar($query,undef);

        unless(defined($constraints{$contype}[$count]{'definition'})) {
            return(undef);
        }

        $query = "SELECT obj_description($conoid, 'pg_constraint');";

        $constraints{$contype}[$count]{'comment'}
             = $self->query_scalar($query,undef);
    }

    return(\%constraints);
}

sub table_definition(\%$)
{
    my ($self, $table_name) = @_;
    my ($query, $array, $row);
    my %table_def;

    $table_def{'name'} = $table_name;

    $table_def{'comment'} = $self->class_comment($table_name);

    $table_def{'columns'} = $self->table_columns($table_name);
    unless(defined($table_def{'columns'})) {
        return(undef);
    }

    $table_def{'constraints'} = $self->table_constraints($table_name);
    unless(defined($table_def{'constraints'})) {
        return(undef);
    }

    return(\%table_def);
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
