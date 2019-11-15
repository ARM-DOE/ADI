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

package DBCORE;

use strict;

use PGSQL;
use SQLite;

use vars qw(%DBArgTypes @DBLoadOrder %DBSPDefs %DBAutoDoc);

my $_Version = '@VERSION@';

sub new(\%) {
    my $class = shift;
    my $self  = {};

    undef($self->{'DBH'});
    undef($self->{'Error'});

    undef($self->{'LoadOrder'});
    undef($self->{'ArgTypes'});
    undef($self->{'SPDefs'});
    undef($self->{'AutoDoc'});

    $self->{'DBOG_DIR'}   = 'dbog';

    $self->{'DBAlias'}    = 'default';
    $self->{'DBConnFile'} = undef;
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

################################################################################
#
# Error Subroutine
#

sub error(\%)
{
    my $self = shift;

    if ($self->{'Error'}) {
        return($self->{'Error'});
    }
    elsif ($self->{'DBH'}) {
        return($self->{'DBH'}->error());
    }
    else {
        return(undef);
    }
}

################################################################################
#
# Connection Subroutines
#

sub find_db_connect_file(\%)
{
    my ($self) = @_;
    my $search_path = $ENV{'DB_CONNECT_PATH'};
    my $path;

    if ($search_path) {
        foreach $path (split(/\s*:\s*/, $search_path)) {
            if (-f "$path/.db_connect") {
                return("$path/.db_connect");
            }
        }
    }

    if ("$ENV{'HOME'}/.db_connect") {
        return("$ENV{'HOME'}/.db_connect");
    }

    return('');
}

sub parse_db_connect_file(\%$)
{
    my ($self, $db_connect_file) = @_;
    my %db_connections = ();
    my @entry;

    undef($self->{'Error'});

    unless (open(FH, "<$db_connect_file")) {
        $self->{'Error'} =
            "Could not open database connection file: $db_connect_file\n" .
            " -> $!\n";
        return(undef);
    }

    foreach (<FH>) {
        next if (/^\s*$|^\s*#.*/); #  Skip blank lines and comments.
        s/#.*//;                   #  Remove inline comments.
        s/^\s+//;                  #  Remove leading whitespace.
        chomp();

        @entry = split(/\s+/);

        $db_connections{$entry[0]}{'DBHost'} = $entry[1];
        $db_connections{$entry[0]}{'DBName'} = $entry[2];
        $db_connections{$entry[0]}{'DBUser'} = $entry[3];
        $db_connections{$entry[0]}{'DBPass'} = $entry[4];
    }

    return(\%db_connections);
}

sub set_timeout(\%$)
{
    my ($self, $timeout) = @_;

    $self->{'Timeout'} = $timeout;

    if ($self->{'DBH'}) {
        $self->{'DBH'}->set_timeout($timeout);
    }
}

sub connect
{
    my $self = shift;
    my $href = ($_[0]) ? shift : ();
    my ($dbhost, $dbname, $dbuser, $dbpass);
    my $dbtype;
    my $dbc;
    my $retval;

    if ($self->{'DBH'}) {
        $self->disconnect();
        undef($self->{'DBH'});
    }

    undef($self->{'Error'});

    if ($href->{'DBOG_DIR'}) {
        $self->{'DBOG_DIR'} = $href->{'DBOG_DIR'};
    }

    if ($href->{'Component'} && $href->{'ObjectGroup'}) {
        unless($self->load_defs($href->{'Component'}, $href->{'ObjectGroup'})) {
            return(undef);
        }
    }

    if ($href->{'DBType'} && ($href->{'DBType'} =~ /^sqlite$/i)) {
        $dbtype = 'SQLite';
        $dbname = $href->{'DBName'};
    }
    elsif (
        $href->{'DBHost'} && $href->{'DBName'} &&
        $href->{'DBUser'} && $href->{'DBPass'}) {

        $dbtype = 'PGSQL';
        $dbhost = $href->{'DBHost'};
        $dbname = $href->{'DBName'};
        $dbuser = $href->{'DBUser'};
        $dbpass = $href->{'DBPass'};
    }
    else {

        if ($href->{'DBAlias'}) {
            $self->{'DBAlias'} = $href->{'DBAlias'};
        }

        if ($href->{'DBConnFile'}) {
            $self->{'DBConnFile'} = $href->{'DBConnFile'};
        }
        else {
            $self->{'DBConnFile'} = $self->find_db_connect_file();
        }

        if (-f $self->{'DBConnFile'}) {

            $dbc = $self->parse_db_connect_file($self->{'DBConnFile'});
            unless (defined($dbc)) {
                return(undef);
            }

            unless (defined($dbc->{$self->{'DBAlias'}})) {
                $self->{'Error'} =
                "Database connection '$self->{'DBAlias'}' does not exist in the db_connect file:\n" .
                " -> $self->{'DBConnFile'}\n";
                return(undef);
            }
        }

        $dbhost = ($href->{'DBHost'}) ? $href->{'DBHost'} : $dbc->{$self->{'DBAlias'}}{'DBHost'};
        $dbname = ($href->{'DBName'}) ? $href->{'DBName'} : $dbc->{$self->{'DBAlias'}}{'DBName'};
        $dbuser = ($href->{'DBUser'}) ? $href->{'DBUser'} : $dbc->{$self->{'DBAlias'}}{'DBUser'};
        $dbpass = ($href->{'DBPass'}) ? $href->{'DBPass'} : $dbc->{$self->{'DBAlias'}}{'DBPass'};

        $dbtype = ($dbhost =~ /^sqlite$/i) ? 'SQLite' : 'PGSQL';
    }

    $self->{'DBType'} = $dbtype;

    if ($dbtype eq 'SQLite') {
    
        if (!$dbname || $dbname eq 'prompt') {
            print("\n" .
            "Please specify the path to the SQLite database file: ");
            chomp($dbname = <STDIN>);
            unless($dbname) {
                $self->{'Error'} = "Database connection aborted.\n";
                return(undef);
            }
        }

        unless (-f $dbname) {
            $self->{'Error'} = "SQLite database file does not exist: $dbname\n";
            return(undef);
        }
    }
    else {

        if (!$dbhost || $dbhost eq 'prompt') {
            print("\n" .
            "Please specify the host name of the database server to connect to.\n" .
            "\n" .
            "Host: ");
            chomp($dbhost = <STDIN>);
            unless($dbhost) {
                $self->{'Error'} = "Database connection aborted.\n";
                return(undef);
            }
        }

        if (!$dbname || $dbname eq 'prompt') {
            print("\n" .
            "Please specify the database to connect to on host '$dbhost'.\n" .
            "\n" .
            "Database: ");
            chomp($dbname = <STDIN>);
            unless($dbname) {
                $self->{'Error'} = "Database connection aborted.\n";
                return(undef);
            }
        }

        if (!$dbuser || $dbuser eq 'prompt') {
            print("\n" .
            "A username is required to connect to database '$dbname'\n" .
            "on host '$dbhost'.\n" .
            "\n" .
            "User: ");
            chomp($dbuser = <STDIN>);
            unless($dbuser) {
                $self->{'Error'} = "Database connection aborted.\n";
                return(undef);
            }
        }

        if (!$dbpass || $dbpass eq 'prompt') {
            system('stty -echo');
            print("\n" .
            "A password is required for user '$dbuser' to connect to\n" .
            "database '$dbname' on host '$dbhost'.\n" .
            "\n" .
            "Password: ");
            chomp($dbpass = <STDIN>);
            print("\n");
            system('stty echo');
            unless($dbpass) {
                $self->{'Error'} = "Database connection aborted.\n";
                return(undef);
            }
        }
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

    if ($dbtype eq 'SQLite') {

        $self->{'DBH'} = SQLite->new();

        $retval = $self->{'DBH'}->connect($dbname,
                    {'RaiseError' => $self->{'RaiseError'},
                     'AutoCommit' => $self->{'AutoCommit'},
                     'Timeout'    => $self->{'Timeout'}
                    });
    }
    else {
        $self->{'DBH'} = PGSQL->new();

        $retval = $self->{'DBH'}->connect(
                    $dbhost, $dbname, $dbuser, $dbpass,
                    {'RaiseError' => $self->{'RaiseError'},
                     'AutoCommit' => $self->{'AutoCommit'},
                     'PrintError' => $self->{'PrintError'},
                     'MinMessage' => $self->{'MinMessage'},
                     'Timeout'    => $self->{'Timeout'},
                     'pg_server_prepare' => $self->{'pg_server_prepare'}
                    });
    }

    unless ($retval) {
        $self->{'Error'} = $self->{'DBH'}->error();
        undef($self->{'DBH'});
    }

    return($retval);
}

sub is_connected(\%)
{
    my $self = shift;

    if ($self->{'DBH'}) {
        if ($self->{'DBH'}->is_connected()) {
            return(1);
        }
    }

    return(0);
}

sub ping(\%)
{
    my $self = shift;

    if ($self->{'DBH'}) {
        return ($self->{'DBH'}->ping());
    }

    return(0);
}

sub disconnect(\%)
{
    my $self = shift;

    if ($self->{'DBH'}) {
        $self->{'DBH'}->commit() unless ($self->{'AutoCommit'});
        $self->{'DBH'}->disconnect();
    }
}

sub reconnect(\%)
{
    my $self = shift;
    my $retval;

    return(undef) unless($self->_init_db_method());

    if ($self->{'DBH'}->is_connected()) {
        $self->{'DBH'}->commit() unless ($self->{'AutoCommit'});
        $self->{'DBH'}->disconnect();
    }

    $retval = $self->{'DBH'}->reconnect();

    return($retval);
}

sub clone
{
    my $self  = shift;
    my $href  = ($_[0]) ? shift : ();
    my $clone = DBCORE->new();
    my %clone_hash;

    undef($clone->{'DBAlias'});
    undef($clone->{'DBConnFile'});

    $clone->{'DBOG_DIR'}   = $self->{'DBOG_DIR'};

    $clone->{'LoadOrder'}  = $self->{'LoadOrder'};
    $clone->{'SPDefs'}     = $self->{'SPDefs'};
    $clone->{'ArgTypes'}   = $self->{'ArgTypes'};
    $clone->{'AutoDoc'}    = $self->{'AutoDoc'};

    $clone->{'RaiseError'} = ($href->{'RaiseError'}) ? $href->{'RaiseError'} : $self->{'RaiseError'};
    $clone->{'AutoCommit'} = ($href->{'AutoCommit'}) ? $href->{'AutoCommit'} : $self->{'AutoCommit'};
    $clone->{'PrintError'} = ($href->{'PrintError'}) ? $href->{'PrintError'} : $self->{'PrintError'};
    $clone->{'MinMessage'} = ($href->{'MinMessage'}) ? $href->{'MinMessage'} : $self->{'MinMessage'};
    $clone->{'Timeout'}    = ($href->{'Timeout'})    ? $href->{'Timeout'}    : $self->{'Timeout'};

    $clone->{'DBH'} = $self->{'DBH'}->clone($href);

    unless ($clone->{'DBH'}) {
        $self->{'Error'} = $self->{'DBH'}->error();
        undef($clone);
        return(undef);
    }

    return($clone);
}

sub commit(\%)
{
    my $self = shift;

    if ($self->{'DBH'}) {
        $self->{'DBH'}->commit() unless ($self->{'AutoCommit'});
    }
}

sub rollback(\%)
{
    my $self = shift;

    if ($self->{'DBH'}) {
        $self->{'DBH'}->rollback();
    }
}

################################################################################
#
# Connection Info Subroutines
#

sub db_host(\%)
{
    my $self = shift;
    return($self->{'DBH'}->db_host());
}

sub db_name(\%)
{
    my $self = shift;
    return($self->{'DBH'}->db_name());
}

sub db_user(\%)
{
    my $self = shift;
    return($self->{'DBH'}->db_user());
}

################################################################################
#
# Stored Procedure Definitions Subroutines
#

sub get_object_group_pkg_dir(\%$$$)
{
    my ($self, $component, $object_group, $package) = @_;
    my $comp_env    = uc($component) . '_HOME';
    my $comp_home   = $ENV{$comp_env};
    my $dbog_dir    = $self->{'DBOG_DIR'};
    my $pkg_dir;

    undef($self->{'Error'});

    unless ($comp_home) {
        $self->{'Error'} =
        "Environment variable '$comp_env' does not exist.\n";
        return(undef);
    }

    $pkg_dir = "$comp_home/conf/$dbog_dir/$package";

    return($pkg_dir);
}

sub get_defs_file(\%$$)
{
    my ($self, $component, $object_group) = @_;
    my $comp_env    = uc($component) . '_HOME';
    my $comp_home   = $ENV{$comp_env};
    my $dbog_dir    = $self->{'DBOG_DIR'};
    my $defs_file;

    undef($self->{'Error'});

    unless ($comp_home) {
        $self->{'Error'} =
        "Environment variable '$comp_env' does not exist.\n";
        return(undef);
    }

    $defs_file = "$comp_home/conf/$dbog_dir/$object_group.defs";

    return($defs_file);
}

sub load_defs_file(\%$)
{
    my ($self, $file) = @_;
    my ($key, $retval);

    undef($self->{'Error'});

    unless ($retval = do $file) {
        if ($@) {
            $self->{'Error'} = "Could not compile definition file: $@\n" .
                             " -> $file\n";
        }
        elsif (!defined($retval)) {
            $self->{'Error'} = "Could not read definition file: $!\n" .
                             " -> $file\n";
        }
        else {
            $self->{'Error'} = "Bad return value loading definition file:\n" .
                             " -> $file\n";
        }
        return(undef);
    }

    foreach $key (@DBLoadOrder) {
        unless (grep(/^$key$/, @{$self->{'LoadOrder'}})) {
            push(@{$self->{'LoadOrder'}}, $key);
        }
    }

    foreach $key (keys(%DBSPDefs)) {
        $self->{'SPDefs'}{$key} = $DBSPDefs{$key};
    }

    foreach $key (keys(%DBArgTypes)) {
        $self->{'ArgTypes'}{$key} = $DBArgTypes{$key};
    }

    foreach $key (keys(%DBAutoDoc)) {
        $self->{'AutoDoc'}{$key} = $DBAutoDoc{$key};
    }

    undef(@DBLoadOrder);
    undef(%DBArgTypes);
    undef(%DBSPDefs);
    undef(%DBAutoDoc);

    return(1);
}

sub load_defs(\%$$)
{
    my ($self, $component, $object_group) = @_;
    my $defs_file;
    my $retval;

    undef($self->{'Error'});

    $defs_file = $self->get_defs_file($component, $object_group);

    unless ($defs_file) {
        return(undef);
    }

    unless (-f $defs_file) {
        $self->{'Error'} =
        "The object group definitions file does not exist:\n" .
        " -> $defs_file\n";
        return(undef);
    }

    $retval = $self->load_defs_file($defs_file);

    return($retval);
}

################################################################################
#
# Database Query Subroutines
#

sub do(\%$)
{
    my ($self, $sql_string) = @_;
    my $retval;

    return(undef) unless($self->_init_db_method());

    $retval = $self->{'DBH'}->do($sql_string);

    return($retval);
}

sub sp_call
{
    my ($self, $class, $method, @args) = @_;
    my ($query, $retval);

    return(undef) unless($self->_init_db_method());

    unless (defined($self->{'SPDefs'}{$class}{$method})) {
        $self->{'Error'} = "Unknown stored procedure definition: ${class}::$method\n";
        return(undef);
    }

    $query = $self->{'DBH'}->build_query_string(
                             \%{$self->{'SPDefs'}{$class}{$method}});

    if (defined($self->{'SPDefs'}{$class}{$method}{'rets'})) {
        $retval = $self->{'DBH'}->query_hasharray($query, \@args,
                                  \@{$self->{'SPDefs'}{$class}{$method}{'rets'}});
    }
    else {
        $retval = $self->{'DBH'}->query_scalar($query, \@args);
    }

    return($retval);
}

sub is_sp_call(\%$)
{
    my ($self, $class, $method) = @_;
    my $retval;

    return(undef) unless($self->_init_db_method());

    unless (defined($self->{'SPDefs'}{$class}{$method})) {
        $self->{'Error'} = "Unknown stored procedure definition: ${class}::$method\n";
        return(undef);
    }

    $retval = $self->{'DBH'}->is_procedure($self->{'SPDefs'}{$class}{$method}{'proc'});

    return($retval);
}

sub data_factory(\%$\%$)
{
    my ($self, $method, $sp_data, $reverse_load_order) = @_;
    my (@load_order, $class, $query, $data, $arg, @args, $retval, %out_hash);

    return(undef) unless($self->_init_db_method());

    @load_order = @{$self->{'LoadOrder'}};
    if ($reverse_load_order) {
        @load_order = reverse(@load_order);
    }

    foreach $class (@load_order) {

        if (defined($self->{'SPDefs'}{$class}{$method}) &&
            defined($sp_data->{$class})) {

            $query = $self->{'DBH'}->build_query_string(
                                \%{$self->{'SPDefs'}{$class}{$method}});

            foreach $data (@{$sp_data->{$class}}) {

                @args = ();

                foreach $arg (@{$self->{'SPDefs'}{$class}{$method}{'args'}}) {
                    push(@args, $data->{$arg});
                }

                if (defined($self->{'SPDefs'}{$class}{$method}{'rets'})) {
                    $retval = $self->{'DBH'}->query_hasharray($query, \@args,
                                \@{$self->{'SPDefs'}{$class}{$method}{'rets'}});
                }
                else {
                    $retval = $self->{'DBH'}->query_scalar($query, \@args);
                }

                unless (defined($retval)) {
                    return(undef);
                }

                push(@{$out_hash{$class}}, $retval);
            }
        }
    }

    return(\%out_hash);
}

sub query(\%$$)
{
    my ($self, $query, $args) = @_;
    my $retval;

    return(undef) unless($self->_init_db_method());

    $retval = $self->{'DBH'}->query($query, $args);

    return($retval);
}

sub query_hasharray(\%$$$)
{
    my ($self, $query, $args, $cols) = @_;
    my $retval;

    return(undef) unless($self->_init_db_method());

    $retval = $self->{'DBH'}->query_hasharray($query, $args, $cols);

    return($retval);
}

sub query_scalar(\%$$)
{
    my ($self, $query, $args) = @_;
    my $retval;

    return(undef) unless($self->_init_db_method());

    $retval = $self->{'DBH'}->query_scalar($query, $args);

    return($retval);
}

sub is_procedure(\%$)
{
    my ($self, $process_name) = @_;
    my $retval;

    return(undef) unless($self->_init_db_method());

    $retval = $self->{'DBH'}->is_procedure($process_name);

    return($retval);
}

sub is_table(\%$)
{
    my ($self, $table_name) = @_;
    my $retval;

    return(undef) unless($self->_init_db_method());

    $retval = $self->{'DBH'}->is_table($table_name);

    return($retval);
}

###############################################################################
#
#  Comment Retrieving Methods
#

sub class_comment(\%$)
{
    my ($self, $class_name) = @_;
    my $retval;

    return(undef) unless($self->_init_db_method());

    $retval = $self->{'DBH'}->class_comment($class_name);

    return($retval);
}

sub procedure_comment(\%$)
{
    my ($self, $procedure) = @_;
    my $retval;

    return(undef) unless($self->_init_db_method());

    $retval = $self->{'DBH'}->procedure_comment($procedure);

    return($retval);
}

sub sp_comment(\%$$)
{
    my ($self, $class, $method) = @_;
    my $href = $self->sp_definition($class, $method);
    my ($procedure, $retval);

    unless(defined($href)) {
        return(undef);
    }

    $procedure = "$href->{'name'}($href->{'arg_types'})";
    $retval    = $self->{'DBH'}->procedure_comment($procedure);

    return($retval);
}

sub type_comment(\%$)
{
    my ($self, $type_name) = @_;
    my $retval;

    return(undef) unless($self->_init_db_method());

    $retval = $self->{'DBH'}->type_comment($type_name);

    return($retval);
}

sub parse_procedure_comment(\%$)
{
    my ($self, $comment) = @_;
    my $line;

    my %hash      = ();
    my $section   = '';
    my $arg_count = 0;
#    my $ret_count = 0;

    foreach $line (split(/\n/, $comment)) {

        next if ($line =~ /^\s*$/); # skip blank lines

        if ($line =~ /^Description:/) {
            $section = 'description';
        }
        elsif ($line =~ /^Arguments:/) {
            $section = 'arguments';
        }
        elsif ($line =~ /^Returns:/) {
            $section = 'returns';
        }
        elsif ($line =~ /^Errors:/) {
            $section = 'errors';
        }
        elsif ($line =~ /^API Layer:/) {
            $section = 'api_layer';
        }
        elsif ($section) {

            $line =~ s/^\s*//; # trim leading white space
            $line =~ s/\s*$//; # trim trailing white space

            if ($section eq 'arguments') {
                if ($line =~ /^(.+?)\s*=>\s*(.+?)$/) {
                    $hash{$section}[$arg_count]{'type'} = $1;
                    $hash{$section}[$arg_count]{'desc'} = $2;
                    $arg_count++;
                }
            }
            elsif ($section eq 'returns') {
                if ($line =~ /^(.+?)\s*=>\s*(.+?)$/) {
#                    $hash{$section}[$ret_count]{'type'} = $1;
#                    $hash{$section}[$ret_count]{'desc'} = $2;
#                    $ret_count++;
                    $hash{$section}{'type'} = $1;
                    $hash{$section}{'desc'} = $2;
                }
                elsif(defined($hash{$section}{'type'})) {

                    if (defined($hash{$section}{'desc'})) {
                        $hash{$section}{'desc'} .= "\n";
                    }

                    if ($line =~ /^\s*=>\s*(.+?)$/) {
                        $hash{$section}{'desc'} .= "$1";
                    }
                    else {
                        $hash{$section}{'desc'} .= "$line";
                    }
                }
                else {
                    $hash{$section}{'type'} = $line;
                }
            }
            else {
                $hash{$section} .= "$line\n";
            }
        }
    }

    return(\%hash);
}

sub parse_type_comment(\%$)
{
    my ($self, $comment) = @_;
    my ($line, $name, $desc);

    my %hash      = ();
    my $section   = '';

    foreach $line (split(/\n/, $comment)) {

        next if ($line =~ /^\s*$/); # skip blank lines

        if ($line =~ /^Description:/) {
            $section = 'description';
        }
        elsif ($line =~ /^Attributes:/) {
            $section = 'attributes';
        }
        elsif ($section) {

            $line =~ s/^\s*//; # trim leading white space
            $line =~ s/\s*$//; # trim trailing white space

            if ($section eq 'attributes') {
                if ($line =~ /^(\w+)\s*=>\s*(.+?)$/) {

                    $name = $1;
                    $desc = $2;

                    push(@{$hash{'att_order'}}, $name);
                    $hash{$section}{$name} = $desc;
                }
            }
            else {
                $hash{$section} .= "$line\n";
            }
        }
    }

    return(\%hash);
}

###############################################################################
#
#  Data Dictionary Methods
#

sub sp_definition(\%$$)
{
    my ($self, $class, $method) = @_;
    my ($arg, %procedure);

    return(undef) unless($self->_init_db_method());

    unless (defined($self->{'SPDefs'}{$class}{$method})) {
        $self->{'Error'} = "Unknown stored procedure definition: ${class}::$method\n";
        return(undef);
    }

    $procedure{'name'}      = $self->{'SPDefs'}{$class}{$method}{'proc'};
    $procedure{'args'}      = '';
    $procedure{'arg_types'} = '';

    foreach $arg (@{$self->{'SPDefs'}{$class}{$method}{'args'}}) {
        $procedure{'args'}      .= "$arg,";
        $procedure{'arg_types'} .= $self->{'ArgTypes'}{$arg} . ',';
    }

    chop($procedure{'args'}) if ($procedure{'args'});
    chop($procedure{'arg_types'}) if ($procedure{'arg_types'});

    return(\%procedure);
}

sub type_attributes(\%$)
{
    my ($self, $type_name) = @_;
    my $retval;

    return(undef) unless($self->_init_db_method());

    $retval = $self->{'DBH'}->type_attributes($type_name);

    return($retval);
}

sub type_definition(\%$)
{
    my ($self, $type_name) = @_;
    my $retval;

    return(undef) unless($self->_init_db_method());

    $retval = $self->{'DBH'}->type_definition($type_name);

    return($retval);
}

sub table_columns(\%$)
{
    my ($self, $table_name) = @_;
    my $retval;

    return(undef) unless($self->_init_db_method());

    $retval = $self->{'DBH'}->table_columns($table_name);

    return($retval);
}

sub table_constraints(\%$)
{
    my ($self, $table_name) = @_;
    my $retval;

    return(undef) unless($self->_init_db_method());

    $retval = $self->{'DBH'}->table_constraints($table_name);

    return($retval);
}

sub table_definition(\%$)
{
    my ($self, $table_name) = @_;
    my $retval;

    return(undef) unless($self->_init_db_method());

    $retval = $self->{'DBH'}->table_definition($table_name);

    return($retval);
}

################################################################################
#
# Utility Subroutines
#

sub seconds_to_timestamp(\%$)
{
    my ($self, $seconds) = @_;

    undef($self->{'Error'});

    if ($self->{'DBH'}) {
        return($self->{'DBH'}->seconds_to_timestamp($seconds));
    }
    else {
        $self->{'Error'} = "Unknown database format for time conversion\n";
        return(undef);
    }
}

sub timestamp_to_seconds(\%$)
{
    my ($self, $timestamp) = @_;

    undef($self->{'Error'});

    if ($self->{'DBH'}) {
        return($self->{'DBH'}->timestamp_to_seconds($timestamp));
    }
    else {
        $self->{'Error'} = "Unknown database format for time conversion\n";
        return(undef);
    }
}

sub hasharray_to_table(\%\@\@)
{
    my ($self, $hash_keys, $hasharray) = @_;
    my ($col, $row, $key, @width, $length, $table, $pad, $count);

    # Get column name lengths

    for ($col = 0; $key = @{$hash_keys}[$col]; $col++) {
        $width[$col] = length($key) + 2;
    }

    # Determine max column width

    foreach $row (@{$hasharray}) {
        for ($col = 0; $key = @{$hash_keys}[$col]; $col++) {
            next unless ($row->{$key});
            $length = length($row->{$key}) + 2;
            $width[$col] = $length if ($length > $width[$col]);
        }
    }

    # Create header row

    $table = '';
    for ($col = 0; $key = @{$hash_keys}[$col]; $col++) {

        $length = length($key);
        $pad    = int(($width[$col] - $length) / 2);

        for ($count = $pad; $count > 0; $count--) {
            $table .= ' ';
        }

        $table .= $key;

        for ($count = $width[$col] - $length - $pad; $count > 0; $count--) {
            $table .= ' ';
        }

        $table .= '|';
    }
    chop($table);
    $table .= "\n";

    # Create seperator row

    for ($col = 0; $key = @{$hash_keys}[$col]; $col++) {

        $length = $width[$col];

        for (;$length > 0; $length--) {
            $table .= '-';
        }
        $table .= '+';
    }
    chop($table);
    $table .= "\n";

    # Create table rows

    foreach $row (@{$hasharray}) {
        for ($col = 0; $key = @{$hash_keys}[$col]; $col++) {

            if ($row->{$key}) {
                $length = length($row->{$key});
                $table .= " $row->{$key}";
            }
            else {
                $length = 0;
                $table .= " ";
            }

            for ($count = $width[$col] - $length - 1; $count > 0; $count--) {
                $table .= ' ';
            }

            $table .= '|';
        }
        chop($table);
        $table .= "\n";
    }

    return($table);
}

sub table_to_hasharray(\%\@$)
{
    my ($self, $hash_keys, $table) = @_;
    my ($found_dashes, $line, @line, $col, $key, @hasharray, $i);

    @hasharray = ();

    $i = 0;
    $found_dashes = 0;
    foreach $line (split(/\n/, $table)) {

        unless ($found_dashes) {
            $found_dashes = 1 if ($line =~ /^-/);
            next;
        }

        next if ($line =~ /^\s+$/); # skip blank lines

        @line = split(/\s\|\s/, $line);

        for ($col = 0; $key = @{$hash_keys}[$col]; $col++) {

            $line[$col] =~ s/^\s+//;
            $line[$col] =~ s/\s+$//;

            $hasharray[$i]{$key} = $line[$col];
        }

        $i++;
    }

    return(\@hasharray);
}

sub hasharray_to_text(\%\@\@)
{
    my ($self, $hash_keys, $hasharray) = @_;
    my ($max_width, $width, $key, $hash, $pad, $count);
    my $text;

    # Get maximum width of hash key names

    $max_width = 0;
    foreach $key (@{$hash_keys}) {
        $width     = length($key) + 2;
        $max_width = $width if ($width > $max_width);
    }

    $text = '';
    foreach $hash (@{$hasharray}) {

        $text .= "  {\n";

        foreach $key (@{$hash_keys}) {

            $text .= "    '$key'";

            $pad = $max_width - (length($key) + 2);

            for ($count = $pad; $count > 0; $count--) {
                $text .= ' ';
            }

            $text .= " => '$hash->{$key}',\n";
        }

        $text .= "  },\n";
    }

    return($text);
}

sub print_data(\%\%)
{
    my ($self, $sp_data) = @_;
    my $table;

    foreach $table (@{$self->{'LoadOrder'}}) {

        if (defined($self->{'SPDefs'}{$table}{'define'}) &&
            defined($sp_data->{$table})) {

            print(
                "\n" .
                "Table: $table\n" .
                "\n" .
                $self->hasharray_to_table(
                    \@{$self->{'SPDefs'}{$table}{'define'}{'args'}},
                    $sp_data->{$table}));
        }
    }
}

1;
