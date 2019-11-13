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

package DSDB;

use DBCORE;

use strict;

use vars qw(%DBData);

my $_Version = '@VERSION@';

sub new(\%) {
    my $class = shift;
    my $self  = {};

    $self->{'DBCORE'} = DBCORE->new();

    undef($self->{'tdb_name'});

    undef($self->{'site'});
    undef($self->{'facility'});
    undef($self->{'proc_type'});
    undef($self->{'proc_name'});

    $self->{'tdbog_is_loaded'} = 0;
    $self->{'dsdbog_is_loaded'} = 0;
    $self->{'attnfileog_is_loaded'} = 0;

    $self->{'AutoCommit'} = 1;

    bless ($self, $class);
    return $self;
}

sub error(\%)
{
    my $self = shift;

    return($self->{'DBCORE'}->error());
}

################################################################################
#
#  Database Connection Subroutines
#

sub disable_autocommit(\%)
{
    my ($self) = @_;

    $self->{'AutoCommit'} = 0;
}

sub connect(\%$)
{
    my ($self, $db_alias) = @_;
    my %conn_args;

    $conn_args{'DBAlias'} = ($db_alias) ? $db_alias : 'dsdb';
    $conn_args{'AutoCommit'} = $self->{'AutoCommit'};

    return($self->{'DBCORE'}->connect(\%conn_args));
}

sub is_connected(\%)
{
    my $self = shift;

    return($self->{'DBCORE'}->is_connected());
}

sub ping(\%)
{
    my $self = shift;

    return($self->{'DBCORE'}->ping());
}

sub disconnect(\%)
{
    my $self = shift;

    $self->{'DBCORE'}->disconnect();
}

sub reconnect(\%)
{
    my $self = shift;

    return($self->{'DBCORE'}->reconnect());
}

sub commit(\%)
{
    my $self = shift;

    return($self->{'DBCORE'}->commit());
}

sub rollback(\%)
{
    my $self = shift;

    return($self->{'DBCORE'}->rollback());
}

################################################################################
#
#  Utility Subroutines
#

sub seconds_to_timestamp(\%$)
{
    my ($self, $seconds) = @_;

    return($self->{'DBCORE'}->seconds_to_timestamp($seconds));
}

sub timestamp_to_seconds(\%$)
{
    my ($self, $timestamp) = @_;

    return($self->{'DBCORE'}->timestamp_to_seconds($timestamp));
}

sub get_sp_return_hash(\%$$)
{
    my ($self, $class, $method) = @_;
    my @hash_keys = @{$self->{'DBCORE'}->{'SPDefs'}{$class}{$method}{'rets'}};

    return(@hash_keys);
}

sub hasharray_to_table(\%\@\@)
{
    my ($self, $hash_keys, $hasharray) = @_;

    return($self->{'DBCORE'}->hasharray_to_table($hash_keys, $hasharray));
}

################################################################################
#
#  Subroutines for loading definition files
#

sub init_dsdbog_methods(\%)
{
    my ($self) = @_;

    unless($self->{'dsdbog_is_loaded'}) {
        unless($self->{'DBCORE'}->load_defs('dsdb', 'dsdb')) {
            return(0);
        }
        $self->{'dsdbog_is_loaded'} = 1;
    }

    return(1);
}

sub init_attnfileog_methods(\%)
{
    my ($self) = @_;

    unless($self->{'attnfileog_is_loaded'}) {
        unless($self->{'DBCORE'}->load_defs('dsdb', 'attnfile')) {
            return(0);
        }
        $self->{'attnfileog_is_loaded'} = 1;
    }

    return(1);
}

sub init_tdbog_methods(\%)
{
    my ($self) = @_;

    unless($self->{'tdbog_is_loaded'}) {
        unless($self->{'DBCORE'}->load_defs('dsdb', 'tdb')) {
            return(0);
        }
        $self->{'tdbog_is_loaded'} = 1;
    }

    return(1);
}

################################################################################
#
#  Generic Stored Procedure call
#

sub sp_call(\%$$@)
{
    my ($self, $class, $method, @args) = @_;

    return($self->{'DBCORE'}->sp_call($class, $method, @args));
}

################################################################################
#
#  "Data Factory" procedure for loading data into the database
#

sub process_data_file(\%$$)
{
    my ($self, $method, $file) = @_;
    my $reverse_load_order;
    my $retval;

    unless ($retval = do $file) {

        if ($@) {
            $self->{'DBCORE'}->{'Error'} =
                "Could not compile data file: $@\n -> $file\n";
        }
        elsif (!defined($retval)) {
            $self->{'DBCORE'}->{'Error'} =
                "Could not read data file: $!\n -> $file\n";
        }
        else {
            $self->{'DBCORE'}->{'Error'} =
                "Bad return value loading data file:\n -> $file\n";
        }

        return(0);
    }

    if ($method eq 'define') {
        $reverse_load_order = 0;
    }
    elsif ($method eq 'delete') {
        $reverse_load_order = 1;
    }
    else {
        $self->{'DBCORE'}->{'Error'} =
            "Unknown Data Method Encountered: $method\n";
        return(0);
    }

    $retval =
        $self->{'DBCORE'}->data_factory($method, \%DBData, $reverse_load_order);

    %DBData = ();

    return($retval);
}

################################################################################
#
#  Process Methods
#

sub init_process_methods(\%$$$$)
{
    my ($self,$site,$facility,$proc_type,$proc_name) = @_;

    return(0) unless($self->init_dsdbog_methods());

    $self->{'site'}       = $site;
    $self->{'facility'}   = $facility;
    $self->{'proc_type'}  = $proc_type;
    $self->{'proc_name'}  = $proc_name;

    return(1);
}

# Process Config Procedures

sub define_process_config_key(\%$)
{
    my ($self,$key) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('process_config_keys', 'define', $key);

    return($retval);
}

sub delete_process_config_key(\%$)
{
    my ($self,$key) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('process_config_keys', 'delete', $key);

    return($retval);
}

sub delete_process_config_value(\%$)
{
    my ($self,$key) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('process_config_values', 'delete',
                $self->{'proc_type'}, $self->{'proc_name'},
                $self->{'site'}, $self->{'facility'}, $key);

    return($retval);
}

sub get_process_config_values(\%$)
{
    my ($self,$key) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('process_config_values', 'get',
                $self->{'proc_type'}, $self->{'proc_name'},
                $self->{'site'}, $self->{'facility'}, $key);

    return($retval);
}

sub update_process_config_value(\%$$)
{
    my ($self,$key,$value) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('process_config_values', 'update',
                $self->{'proc_type'}, $self->{'proc_name'},
                $self->{'site'}, $self->{'facility'}, $key, $value);

    return($retval);
}

# Location Procedures

sub inquire_facilities(\%)
{
    my ($self) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('facilities', 'inquire',
                $self->{'site'}, $self->{'facility'});

    return($retval);
}

# Local Process Family Procedures

sub get_local_process_families(\%)
{
    my ($self) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('local_process_families', 'get');

    return($retval);
}

sub inquire_local_process_families(\%$$)
{
    my ($self, $category, $class) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('local_process_families', 'inquire',
                $category, $class,
                $self->{'site'}, $self->{'facility'});

    return($retval);
}

# Local Family Process Procedures

sub get_local_family_processes(\%)
{
    my ($self) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('local_family_processes', 'get');

    return($retval);
}

sub inquire_local_family_processes(\%$$)
{
    my ($self, $category, $class) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('local_family_processes', 'inquire',
                $category, $class,
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'});

    return($retval);
}

sub get_local_family_process_stats(\%)
{
    my ($self) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('local_family_process_stats', 'get');

    return($retval);
}

sub inquire_local_family_process_stats(\%$$)
{
    my ($self, $category, $class) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('local_family_process_stats', 'inquire',
                $category, $class,
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'});

    return($retval);
}

# Family Process Procedures

sub get_family_process(\%)
{
    my ($self) = @_;
    my %hash   = ();
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('family_processes', 'get',
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'});

    if (defined($retval)) {

        if (defined(@{$retval}[0])) {
            %hash = %{@{$retval}[0]};
        }

        return(\%hash);
    }
    else {
        return(undef);
    }
}

sub get_family_process_location(\%)
{
    my ($self) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('family_processes', 'get_location',
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'});

    return(@{$retval}[0]);
}

sub get_family_process_state(\%)
{
    my ($self) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('family_process_states', 'get',
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'});

    return(@{$retval}[0]);
}

sub get_family_process_status(\%)
{
    my ($self) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('family_process_statuses', 'get',
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'});

    return(@{$retval}[0]);
}

sub inquire_family_process_states(\%$$)
{
    my ($self, $category, $class) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('family_process_states', 'inquire',
                $category, $class,
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'});

    return($retval);
}

sub is_family_process_enabled(\%)
{
    my ($self) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('family_process_states', 'is_enabled',
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'});

    return($retval);
}

sub update_family_process_state(\%$$$)
{
    my ($self, $state_name, $state_text, $state_time) = @_;
    my $retval;

    unless ($state_time) {
        $state_time = time();
    }

    $state_time = $self->seconds_to_timestamp($state_time);

    $retval = $self->{'DBCORE'}->sp_call('family_process_states', 'update',
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'},
                $state_name, $state_text, $state_time);

    return($retval);
}

sub update_family_process_started(\%$)
{
    my ($self, $started_time) = @_;
    my $retval;

    unless ($started_time) {
        $started_time = time();
    }

    $started_time = $self->seconds_to_timestamp($started_time);

    $retval = $self->{'DBCORE'}->sp_call('family_process_statuses', 'update_last_started',
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'},
                $started_time);

    return($retval);
}

sub update_family_process_completed(\%$)
{
    my ($self, $completed_time) = @_;
    my $retval;

    unless ($completed_time) {
        $completed_time = time();
    }

    $completed_time = $self->seconds_to_timestamp($completed_time);

    $retval = $self->{'DBCORE'}->sp_call('family_process_statuses', 'update_last_completed',
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'},
                $completed_time);

    return($retval);
}

sub update_family_process_status(\%$$$)
{
    my ($self, $status_name, $status_text, $status_time) = @_;
    my $retval;

    unless ($status_time) {
        $status_time = time();
    }

    $status_time = $self->seconds_to_timestamp($status_time);

    $retval = $self->{'DBCORE'}->sp_call('family_process_statuses', 'update',
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'},
                $status_name, $status_text, $status_time);

    return($retval);
}

sub inquire_family_processes(\%$$)
{
    my ($self, $category, $class) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('family_processes', 'inquire',
                $category, $class,
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'});

    return($retval);
}

# Datastream Class Procedures

sub inquire_process_input_ds_classes(\%$$)
{
    my ($self, $dsc_name, $dsc_level) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('process_input_ds_classes', 'inquire',
                $self->{'proc_type'}, $self->{'proc_name'},
                $dsc_name, $dsc_level);

    return($retval);
}

sub inquire_process_output_ds_classes(\%$$)
{
    my ($self, $dsc_name, $dsc_level) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('process_output_ds_classes', 'inquire',
                $self->{'proc_type'}, $self->{'proc_name'},
                $dsc_name, $dsc_level);

    return($retval);
}

sub get_process_family_output_ds_classes(\%$$)
{
    my ($self, $category, $class) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call(
                'process_family_output_ds_classes', 'get',
                $category, $class,
                $self->{'site'}, $self->{'facility'});

    return($retval);
}

sub inquire_process_family_output_ds_classes(\%$$$$)
{
    my ($self, $category, $class, $dsc_name, $dsc_level) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call(
                'process_family_output_ds_classes', 'inquire',
                $category, $class,
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'},
                $dsc_name, $dsc_level);

    return($retval);
}

################################################################################
#
#  Attention File Methods
#

sub init_attention_file_methods(\%$$$$)
{
    my ($self,$site,$facility,$proc_type,$proc_name) = @_;

    return(0) unless($self->init_attnfileog_methods());

    $self->{'site'}       = $site;
    $self->{'facility'}   = $facility;
    $self->{'proc_type'}  = $proc_type;
    $self->{'proc_name'}  = $proc_name;

    return(1);
}

sub delete_attention_file(\%$$)
{
    my ($self, $file_path, $file_name) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('attention_files', 'delete',
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'},
                $file_path, $file_name);

    return($retval);
}

sub get_attention_file(\%$$)
{
    my ($self, $file_path, $file_name) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('attention_files', 'get',
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'},
                $file_path, $file_name);

    return($retval);
}

sub get_attention_files(\%$$)
{
    my ($self, $state, $action) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('attention_files', 'gets',
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'},
                $state, $action);

    return($retval);
}

sub inquire_attention_files(\%$$$$$$)
{
    my ($self, $category, $class, $file_path, $file_name, $state, $action) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('attention_files', 'inquire',
                $category, $class,
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'},
                $file_path, $file_name, $state, $action);

    return($retval);
}

sub update_attention_file(\%$$$$$)
{
    my ($self, $file_path, $file_name, $file_size, $file_time, $state) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('attention_files', 'update',
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'},
                $file_path, $file_name, $file_size, $file_time, $state);

    return($retval);
}

sub update_attention_file_action(\%$$$)
{
    my ($self, $file_path, $file_name, $action) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('attention_files', 'update_action',
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'},
                $file_path, $file_name, $action);

    return($retval);
}

# Attention Messages

sub delete_attention_message(\%$$$)
{
    my ($self, $file_path, $file_name, $flag_name) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('attention_messages', 'delete',
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'},
                $file_path, $file_name, $flag_name);

    return($retval);
}

sub inquire_attention_messages(\%$$$$$)
{
    my ($self, $category, $class, $file_path, $file_name, $flag_name) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('attention_messages', 'inquire',
                $category, $class,
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'},
                $file_path, $file_name, $flag_name);

    return($retval);
}

sub update_attention_message(\%$$$$)
{
    my ($self, $file_path, $file_name, $flag_name, $message) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('attention_messages', 'update',
                $self->{'site'}, $self->{'facility'},
                $self->{'proc_type'}, $self->{'proc_name'},
                $file_path, $file_name, $flag_name, $message);

    return($retval);
}

################################################################################
#
#  TDB Methods
#

sub init_tdb_methods(\%$)
{
    my ($self, $tdb_name) = @_;

    unless($self->init_tdbog_methods()) {
        return(0);
    }

    $self->{'tdb_name'} = $tdb_name;

    return(1);
}

sub tdb_store(\%$$)
{
    my ($self, $key, $value) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('tdb_keys', 'update',
                    $self->{'tdb_name'}, $key, $value);

    return($retval);
}

sub tdb_scan(\%$)
{
    my ($self, $pattern) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('tdb_keys', 'inquire',
                    $self->{'tdb_name'}, $pattern);

    return($retval);
}

sub tdb_delete(\%$)
{
    my ($self, $key) = @_;
    my $retval;

    $retval = $self->{'DBCORE'}->sp_call('tdb_keys', 'delete',
                    $self->{'tdb_name'}, $key);

    return($retval);
}

1;
