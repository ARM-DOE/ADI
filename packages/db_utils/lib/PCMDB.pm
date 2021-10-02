################################################################################
#
#  COPYRIGHT (C) 2007 Battelle Memorial Institute.  All Rights Reserved.
#
################################################################################
#
#  Author:
#     name:  Sherman Beus
#     phone: (509) 375-2662
#     email: sherman.beus@pnl.gov
#
################################################################################

package PCMDB;

use strict;
use warnings;

use DBCORE;
use JSON::XS;

my $_Version = '@VERSION@';

my @Error = ( );

################################################################################

sub _set_error {
    my ($line, @err) = @_;

    push(@Error, ":$line >> " . join("\n", grep { defined($_) } @err ));
}

sub get_error {
    return join("\n\n", @Error);
}

sub _db_def {
    my ($db, $sp, @args) = @_;
    my $nargs = scalar(@args);

    $sp .= '(';
    if ($nargs > 0) {
        $sp .= '?';
        for ($nargs -= 1; $nargs; $nargs--) {
            $sp .= ',?';
        }
    }
    $sp .= ')';

    my $retval = $db->query_scalar("SELECT $sp", \@args);
    unless (defined($retval)) {
        _set_error(__LINE__, 'Definition Error', $db->error());
        return 0;
    }

    return 1;
}

sub _select_to_key {
    my ($s) = @_;
    my @parts = ( );

    push(@parts, $s->{'class'}) if ($s->{'class'});
    push(@parts, $s->{'level'}) if ($s->{'level'});
    push(@parts, $s->{'site'}) if ($s->{'site'});
    push(@parts, $s->{'fac'}) if ($s->{'fac'});
    push(@parts, $s->{'sdep'}) if ($s->{'sdep'});
    push(@parts, $_->{'fdep'}) if ($_->{'fdep'});
    push(@parts, $_->{'tdep0'}) if ($_->{'tdep0'});

    return join(':', @parts);
}

################################################################################
# Copied trim, json_decode, and json_encode from CGIUtils.pm
# to remove the dependency.

sub trim {
    my ($str) = @_;

    $str =~ s/^\s*//;
    $str =~ s/\s*$//;

    return $str;
}

# Wrapper around JSON::XS JSON encode.
sub json_encode {
    my ($var) = @_;

    if (JSON::XS->can('encode_json')) {
        return JSON::XS::encode_json($var);
    }
    return JSON::XS::to_json($var);
}

# Wrapper around JSON::XS JSON decode.
sub json_decode {
    my ($var) = @_;

    if (JSON::XS->can('decode_json')) {
        return JSON::XS::decode_json($var);
    }
    return JSON::XS::from_json($var);
}

################################################################################

sub get_connection {
    my ($alias, $conn_file) = @_;

    unless (defined($conn_file)) {
        my $root = ($ENV{HOME}) ? $ENV{HOME} : './';
        $conn_file = "$root/.db_connect";
        unless (-e $conn_file) {
            $root = ($ENV{DSDB_HOME}) ? $ENV{DSDB_HOME} : '/apps/ds';
            $conn_file = "$root/conf/dsdb/.db_connect";
        }
    }

    my %db_conn_args = ();

    if (defined($alias) && $alias ne '') {
        $db_conn_args{'DBHost'} = undef;
        $db_conn_args{'DBName'} = undef;
        $db_conn_args{'DBUser'} = undef;
        $db_conn_args{'DBPass'} = undef;
        $db_conn_args{'DBAlias'} = $alias;

    } elsif (!defined($db_conn_args{'DBAlias'}) && (
             !defined($db_conn_args{'DBHost'}) ||
             !defined($db_conn_args{'DBUser'}) ||
             !defined($db_conn_args{'DBPass'}) ))
    {
        _set_error(__LINE__, 'Insufficient DB credentials.');
        return(undef);
    }

    if (defined($conn_file)) {
        $db_conn_args{'DBConnFile'} = $conn_file;
    }

    my $db = new DBCORE();
    unless($db->connect(\%db_conn_args)) {
        _set_error(__LINE__, "Error connecting to DSDB", $db->error());
        return(undef);
    }

    unless ($db->load_defs('dsdb', 'dsdb')) {
        _set_error(__LINE__, "Error loading the DSDB functions", $db->error());
        return(undef);
    }

    unless ($db->load_defs('dsdb', 'dod')) {
        _set_error(__LINE__, "Error loading the DOD functions", $db->error());
        return(undef);
    }

    # No .defs file yet.
    # unless ($db->load_defs('dsdb', 'retrieval')) {
    #    _set_error(__LINE__, "Error loading the Retriever functions", $db->error());
    #    return(undef);
    # }

    return($db);
}

sub ds_class_exists {
    my ($db, $class, $level) = @_;

    my @args = ( $class, $level );
    my $retval = $db->query_scalar('SELECT * FROM get_datastream_class_id(?,?,false)', \@args);
    return (defined($retval) && $retval > 0) ? 1 : 0;
}


################################################################################

sub get_locations {
    my ($db) = @_;

    my @args = ( );
    my @cols = ( 'scode', 'sname', 'fcode', 'fname', 'lat', 'lon', 'alt' );
    my $retval = $db->query_hasharray(trim("

    SELECT DISTINCT
    s.site_name,
    s.site_desc,
    f.fac_name,
    l.loc_desc,
    l.lat,
    l.lon,
    l.alt

    FROM sites s
    NATURAL JOIN facilities f
    NATURAL JOIN locations l

    "), \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, $db->error());
        return undef;
    }

    my @s = ( );
    my @f = ( );

    my %map = ( );
    my $map = \%map;

    for (@{$retval}) {
        my $skey = $_->{'scode'};
        my $fkey = $_->{'scode'} . $_->{'fcode'};

        unless (defined($map->{$skey})) {
            my %s = (
                'code'   => $_->{'scode'},
                'name'   => $_->{'sname'},
                'lat'    => undef,
                'lon'    => undef,
                'alt'    => undef,
                'parent' => undef,
            );
            $map->{$skey} = \%s;
            push(@s, \%s);
        }

        my %f = (
            'code'   => $_->{'fcode'},
            'name'   => $_->{'fname'},
            'lat'    => $_->{'lat'},
            'lon'    => $_->{'lon'},
            'alt'    => $_->{'alt'},
            'parent' => $_->{'scode'},
        );
        $map->{$fkey} = \%f;
        push(@f, \%f);
    }

    my %result = ( 'sites' => \@s, 'facilities' => \@f );

    return \%result;
}

sub get_process_list {
    my ($db) = @_;

    my @args = ( );
    my @cols = ( 'name', 'desc', 'type' );
    my $list = $db->query_hasharray(trim("

    SELECT DISTINCT proc_name, proc_desc, proc_type_name
     FROM processes
     NATURAL JOIN process_types
     WHERE proc_type_name = 'Ingest' OR proc_type_name = 'VAP'

    "), \@args, \@cols);
    unless (defined($list)) {
        _set_error(__LINE__, 'Failed to get list of processes.', $db->error());
        return undef;
    }

    return $list;
}

sub _get_process_id {
    my ($db, $ptype, $pname) = @_;

    my @args = ( $ptype, $pname );
    my $retval = $db->query_scalar('SELECT get_process_id(?,?,FALSE)', \@args);
    return(undef) unless (defined($retval));

    return int($retval);
}

sub _get_process_family_class_id {
    my ($db, $pfcat, $pfclass) = @_;

    my @args = ( $pfcat, $pfclass );
    my $retval = $db->query_scalar('SELECT get_process_family_class_id(?,?,FALSE)', \@args);
    return(undef) unless (defined($retval));

    return int($retval);
}

sub _get_process_family_id {
    my ($db, $pfcat, $pfclass, $site, $fac) = @_;

    my @args = ( $pfcat, $pfclass, $site, $fac );
    my $retval = $db->query_scalar('SELECT get_process_family_id(?,?,?,?,FALSE)', \@args);
    return(undef) unless (defined($retval));

    return int($retval);
}

sub _parse_location {
    my ($f) = @_;

    my ($site, $fac);

    if (ref($f) eq 'HASH') {
        # Is a hash when read from DB.
        ($site, $fac) = ( $f->{'site'}, $f->{'fac'} );

    } else {

        # Is a string when sent from UI.  Darn my inconsistency.
        ($site, $fac) = split(/[\.\s]/, $f);
        unless (defined($site) && defined($fac)) {
            _set_error(__LINE__, "Badly formed location: $f");
            return 0;
        }

        $site = lc($site);
        $fac = uc($fac);
    }

    return ($site, $fac);
}

sub _delete_family_process {
    my ($db, $cat, $class, $type, $name, $site, $fac, $clean) = @_;

    $clean = (defined($clean) && !$clean) ? 0 : 1;

    my @args = ( $site, $fac, $type, $name );
    my $retval = $db->query_scalar('SELECT cascade_delete_family_process(?,?,?,?)', \@args);
    unless (defined($retval)) {
        _set_error(__LINE__, $db->error());
        return 0;
    }

    return(1) unless ($clean);

    # We only want to delete the entire process family for the site in question if it only
    # consisted of a single process (i.e. the one we're removing from this family).

    $retval = $db->sp_call('family_processes', 'inquire', $cat, $class, $site, $fac, '%', '%');
    unless (defined($retval)) {
        _set_error(__LINE__, $db->error());
        return 0;
    }
    if ($#{$retval} < 0) {

        $retval = $db->sp_call('dsview_process_families', 'delete', $cat, $class, $site, $fac);
        unless (defined($retval)) {
            _set_error(__LINE__, $db->error());
            return 0;
        }

        $retval = $db->sp_call('process_families', 'delete', $cat, $class, $site, $fac);
        unless (defined($retval)) {
            _set_error(__LINE__, $db->error());
            return 0;
        }
    }

    my $proc_id = _get_process_id($db, $type, $name);
    unless (defined($proc_id)) {
        _set_error(__LINE__, "Failed to look up process ID: $type-$name");
        return 0;
    }

    # Delete process output datastreams for this facility.
    @args = ( $fac, $site, $proc_id );
    $retval = $db->do(trim(sprintf("

    DELETE FROM process_output_datastreams
    WHERE fac_id IN (
        SELECT DISTINCT fac_id
        FROM facilities
        NATURAL JOIN sites
        WHERE fac_name = '%s'
        AND site_name = '%s'

    ) AND proc_out_dsc_id IN (
        SELECT DISTINCT proc_out_dsc_id
        FROM process_output_ds_classes
        WHERE proc_id = %d
    );

    ", @args)));
    unless (defined($retval)) {
        _set_error(__LINE__, $db->error());
        return 0;
    }

    return 1;
}

sub process_exists {
    my ($db, $ptype, $pname) = @_;

    my $retval = $db->sp_call('processes', 'inquire', $ptype, $pname);
    return(1) if (defined($retval) && defined($retval->[0]));

    return 0;
}

sub get_process_owner {
    my ($db, $ptype, $pname) = @_;

    my @args = ( $pname, $ptype );
    my @cols = ( 'user' );
    my $retval = $db->query_hasharray(trim("

    SELECT rev_user
    FROM proc_revisions
    NATURAL JOIN processes
    NATURAL JOIN process_types
    WHERE proc_name = ?
      AND proc_type_name = ?
    ORDER BY rev_num ASC

    "), \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, $db->error());
        return undef;
    }
    if (scalar(@{$retval}) > 0) {
        return $retval->[0]->{'user'};
    }

    return '';
}

sub set_process_owner {
    my ($db, $ptype, $pname, $user) = @_;

    my @args = ( $user, $pname, $ptype );
    my $retval = $db->do(trim("

    UPDATE proc_revisions p
    SET rev_user = ?
    FROM (
      SELECT proc_id, MIN(rev_num) as rev_num
      FROM processes
      NATURAL JOIN process_types
      NATURAL JOIN proc_revisions
      WHERE proc_name = ?
        AND proc_type_name = ?
      GROUP BY proc_id
    ) as pr
    WHERE p.proc_id = pr.proc_id
      AND p.rev_num = pr.rev_num

    "), \@args);
    unless (defined($retval)) {
        _set_error(__LINE__, $db->error());
        return undef;
    }

    return 1;
}

sub get_process_revisions {
    my ($db, $ptype, $pname) = @_;

    my @args = ( $ptype, $pname );
    my @cols = ( 'name', 'type', 'rev', 'user', 'date', 'encoded' );
    my $retval = $db->query_hasharray(trim("

    SELECT * FROM inquire_proc_revisions(?,?,'%')

    "), \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, $db->error());
        return undef;
    }

    return $retval;
}

sub get_process {
    my ($db, $ptype, $pname, $db_prod, $without_retrieval) = @_;

    my %proc = (
        'name'      => $pname,
        'type'      => $ptype,
        'desc'      => undef,
        'class'     => $pname, # We have the restriction currently that "class" and "name" must match (for auto generation of family processes).
        'category'  => undef,
        'cdesc'     => undef,
        'locations' => undef,
        'outputs'   => undef,
        'props'     => undef,
    );
    my $proc = \%proc;

    # Get process family classes and locations.
    #
    # NOTE: We are operating under the restriction that a process name will match its
    # parent class name.  Since we can't rely on family_processes existing, we have to
    # do it this way.

    my @args = ( $pname );
    my @cols = ( 'cat', 'desc' );
    my $retval = $db->query_hasharray(trim("

    SELECT DISTINCT proc_fam_cat_name, proc_fam_class_desc
     FROM process_family_classes
     NATURAL JOIN process_family_categories
     WHERE proc_fam_class_name = ?

    "), \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, 'Failed to get parent process family class.', $db->error());
        return undef;
    }
    if (scalar(@{$retval}) > 0) {
        $proc->{'category'} = $retval->[0]->{'cat'};
        $proc->{'cdesc'}    = $retval->[0]->{'desc'};
    }

    @args = ( $pname, $ptype );
    @cols = ( 'desc' );
    $retval = $db->query_hasharray(trim("

    SELECT proc_desc
     FROM processes
     NATURAL JOIN process_types
     WHERE proc_name = ?
       AND proc_type_name = ?

    "), \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, 'Failed to get process description.', $db->error());
        return undef;
    }
    if (scalar(@{$retval}) > 0) {
        $proc->{'desc'} = $retval->[0]->{'desc'};
    }

    $proc->{'locations'} = get_process_locations($db, $ptype, $pname);
    return(undef) unless (defined($proc->{'locations'}));

    # Get process inputs.

    if (has_retrieval($db, $ptype, $pname)) {

        $proc->{'ret'} = 1;

        unless (defined($without_retrieval) && $without_retrieval) {
            $proc->{'ret'} = get_retrieval($db, $ptype, $pname);
            unless (defined($proc->{'ret'})) {
                _set_error(__LINE__, 'Failed to load retrieval.', $db->error());
                return undef;
            }
        }

    } else {

        $retval = $db->sp_call('process_input_ds_classes', 'get', $ptype, $pname);
        unless (defined($retval)) {
            _set_error(__LINE__, 'Failed to get process inputs.', $db->error());
            return undef;
        }

        my @inputs = ( );
        for (@{$retval}) {
            push(@inputs, $_->{'dsc_name'} . '.' . $_->{'dsc_level'});
        }

        $proc->{'inputs'} = \@inputs;
    }

    # Get process outputs.

    $retval = $db->sp_call('process_output_ds_classes', 'get', $ptype, $pname);
    unless (defined($retval)) {
        _set_error(__LINE__, 'Failed to get process outputs.', $db->error());
        return undef;
    }

    my @outputs = ( );
    for (@{$retval}) {
        my $dsc_name = $_->{'dsc_name'} . '.' . $_->{'dsc_level'};
        push(@outputs, $dsc_name);
    }

    $proc->{'outputs'} = \@outputs;

    # Get process properties.

    $retval = $db->sp_call('process_config_values', 'inquire', $ptype, $pname, '%', '%', '%');
    unless (defined($retval)) {
        _set_error(__LINE__, 'Failed to get process config values.', $db->error());
        return undef;
    }

    my %props = ( );
    for (@{$retval}) {
        next unless (defined($_->{'proc_type'}) && defined($_->{'proc_name'}));
        $props{ $_->{'config_key'} } = $_->{'config_value'};
    }

    $proc->{'props'} = \%props;

    return $proc;
}

sub save_process {
    my ($db, $ptype, $pname, $data, $user, $full, $noclean) = @_;

    my $clean = (defined($noclean) && $noclean) ? 0 : 1;

    my $retval;
    my @args;
    my @cols;

    my $json_data;
    my $def;
    if (ref($data) eq 'HASH') {
        $def = $data;
        $json_data = json_encode($data);
    } else {
        $def = json_decode($data);
        $json_data = $data;
    }

    my $defc;
    if (process_exists($db, $ptype, $pname)) {
        $defc = get_process($db, $ptype, $pname, undef, 1); # We don't need the retrieval since we don't merge that one.
        unless (defined($defc)) {
            _set_error(__LINE__, 'Failed to load existing process');
            return 0;
        }
    }

    if ($ptype ne $def->{'type'} && defined($defc)) {
        return(0) unless (delete_process($db, $ptype, $pname));
        $defc = undef;
    }

    unless (defined($def->{'class'})) {
        $def->{'class'} = $def->{'name'};
    }

    my $proc_id;
    my $renamed = 0;
    my $changes = 0;

    if ($pname ne $def->{'name'} && defined($defc)) {

        # Cleanly rename without deleting.

        $proc_id = _get_process_id($db, $ptype, $pname);
        unless (defined($proc_id)) {
            _set_error(__LINE__, 'Failed to get process ID');
            return 0;
        }

        if ($proc_id > 0) {
            @args = ( $def->{'name'}, $proc_id );
            $retval = $db->do(trim(sprintf("

            UPDATE processes
            SET proc_name = '%s'
            WHERE proc_id = %d

            ", @args)));
            unless (defined($retval)) {
                _set_error(__LINE__, 'Failed to rename process');
                return 0;
            }

            $renamed = 1;
            $changes++;
        }
    }

    if (defined($defc)) {

        unless ($renamed) {

            # Define the process.

            $retval = $db->sp_call('processes', 'define',
                $def->{'type'},
                $def->{'name'},
                defined($def->{'desc'}) ? $def->{'desc'} : ''
            );
            unless (defined($retval)) {
                _set_error(__LINE__, $db->error());
                return 0;
            }

            $proc_id = _get_process_id($db, $def->{'type'}, $def->{'name'});

            $changes++;
        }

        # FAMILY CLASS.

        if ($def->{'class'} ne $defc->{'class'}) {

            my $pfc_id = _get_process_family_class_id($db, $def->{'category'}, $def->{'class'});

            unless ($pfc_id) {

                if ($def->{'class'} eq $def->{'name'} && $defc->{'class'} eq $defc->{'name'} && $def->{'category'} eq $defc->{'category'}) {

                    @args = ( $def->{'class'}, $defc->{'class'}, $defc->{'category'} );
                    $retval = $db->do(trim(sprintf("

                    UPDATE process_family_classes
                    SET proc_fam_class_name = '%s'
                    WHERE proc_fam_class_id IN (
                        SELECT DISTINCT proc_fam_class_id
                        FROM process_family_classes
                        NATURAL JOIN process_family_categories
                        WHERE proc_fam_class_name = '%s'
                        AND proc_fam_cat_name = '%s'
                    );

                    ", @args)));
                    unless (defined($retval)) {
                        _set_error(__LINE__, 'Failed to rename process family class name');
                        return 0;
                    }

                    $changes++;

                } else {
                    my $cdesc = defined($def->{'cdesc'}) ? $def->{'cdesc'} : '';
                    unless ($cdesc || !defined($def->{'desc'})) {
                        $cdesc = $def->{'desc'};
                    }
                    $retval = $db->sp_call('process_family_classes', 'define',
                        $def->{'category'},
                        $def->{'class'},
                        $cdesc,
                    );
                    unless (defined($retval)) {
                        _set_error(__LINE__, $db->error());
                        return 0;
                    }

                    my $pid_new = _get_process_family_class_id($db, $def->{'category'}, $def->{'class'});
                    my $pid_old = _get_process_family_class_id($db, $defc->{'category'}, $defc->{'class'});

                    @args = ( $pid_new, $pid_old, $proc_id );
                    $retval = $db->do(trim(sprintf("

                    UPDATE process_families
                    SET proc_fam_class_id = %d
                    WHERE proc_fam_class_id = %d
                    AND proc_fam_id IN (
                        SELECT DISTINCT proc_fam_id
                        FROM family_processes
                        WHERE proc_id = %d
                    )

                    ", @args)));
                    unless (defined($retval)) {
                        _set_error(__LINE__, 'Failed to update process family class ID references');
                        return 0;
                    }

                    $changes++;
                }
            }
        }

        # NOTE: Important to delete retrieval prior to deleting old inputs/outputs.

        if (has_retrieval($db, $def->{'type'}, $def->{'name'})) {

            return(0) unless (delete_retrieval($db, $def->{'type'}, $def->{'name'}));
            $defc->{'inputs'} = undef; # Because the above wipes these out.
        }


        # OUTPUTS.

        my %oc_map = ( );
        my $oc_map = \%oc_map;
        if (defined($defc->{'outputs'})) {
            for (@{$defc->{'outputs'}}) {
                $oc_map->{ $_ } = $_;
            }
        }

        my %o_map = ( );
        my $o_map = \%o_map;

        for (@{$def->{'outputs'}}) {
            $o_map->{ $_ } = $_;

            next if (defined($oc_map->{ $_ }));

            my ($class, $level);
            if (ref($_) eq 'HASH') {
                ($class, $level) = ( $_->{'class'}, $_->{'level'} );
            } else {
                ($class, $level) = split(/\./, $_);
                unless (defined($class) && defined($level)) {
                    _set_error(__LINE__, "Badly formed input ds class: $_");
                    return 0;
                }
            }

            $retval = $db->sp_call('datastream_classes', 'define', $class, $level, '');
            unless (defined($retval)) {
                _set_error(__LINE__, $db->error());
                return 0;
            }

            $retval = $db->sp_call('process_output_ds_classes', 'define', $def->{'type'}, $def->{'name'}, $class, $level);
            unless (defined($retval)) {
                _set_error(__LINE__, $db->error());
                return 0;
            }

            $changes++;
        }

        for (keys(%oc_map)) {
            next if (defined($o_map->{ $_ }));

            my ($class, $level) = split(/\./, $_);

            @args = ( $proc_id );
            $retval = $db->do(trim(sprintf("

            DELETE FROM process_output_datastreams
            WHERE proc_out_dsc_id IN (
                SELECT proc_out_dsc_id
                FROM process_output_ds_classes
                WHERE proc_id = %d
            );

            ", @args)));
            unless (defined($retval)) {
                _set_error(__LINE__, $db->error());
                return 0;
            }

            $retval = $db->sp_call('process_output_ds_classes', 'delete', $def->{'type'}, $def->{'name'}, $class, $level);
            unless (defined($retval)) {
                _set_error(__LINE__, $db->error());
                return 0;
            }

            $changes++;
        }

        # INPUTS.

        my %ic_map = ( );
        my $ic_map = \%ic_map;
        if (defined($defc->{'inputs'})) {
            for (@{$defc->{'inputs'}}) {
                $ic_map->{ $_ } = $_;
            }
        }

        my %i_map = ( );
        my $i_map = \%i_map;
        if (defined($def->{'inputs'})) {
            for (@{$def->{'inputs'}}) {
                $i_map->{ $_ } = $_;

                next if (defined($ic_map->{ $_ }));

                my ($class, $level);
                if (ref($_) eq 'HASH') {
                    ($class, $level) = ( $_->{'class'}, $_->{'level'} );
                } else {
                    ($class, $level) = split(/\./, $_);
                    unless (defined($class) && defined($level)) {
                        _set_error(__LINE__, "Badly formed output ds class: $_");
                        return 0;
                    }
                }

                $retval = $db->sp_call('datastream_classes', 'define', $class, $level, '');
                unless (defined($retval)) {
                    _set_error(__LINE__, $db->error());
                    return 0;
                }

                $retval = $db->sp_call('process_input_ds_classes', 'define', $def->{'type'}, $def->{'name'}, $class, $level);
                unless (defined($retval)) {
                    _set_error(__LINE__, $db->error());
                    return 0;
                }

                $changes++;
            }
        }

        for (keys(%ic_map)) {
            next if (defined($i_map->{ $_ }));

            my ($class, $level) = split(/\./, $_);

            $retval = $db->sp_call('process_input_ds_classes', 'delete', $def->{'type'}, $def->{'name'}, $class, $level);
            unless (defined($retval)) {
                _set_error(__LINE__, $db->error());
                return 0;
            }

            $changes++;
        }

        if (defined($def->{'ret'})) {

            # Includes deleting existing.
            return(0) unless (define_retrieval($db, $def->{'type'}, $def->{'name'}, $def->{'ret'}));

            $changes++;
        }

        # LOCATIONS.

        my %fc_map = ( );
        my $fc_map = \%fc_map;
        if (defined($defc->{'locations'})) {
            for my $f (@{$defc->{'locations'}}) {

                my ($site, $fac) = ( $f->{'site'}, $f->{'fac'} );

                $fc_map->{ "$site.$fac" } = $f;

                if ($def->{'class'} ne $defc->{'class'} || $def->{'category'} ne $defc->{'category'}) {

                    my $pid_new = _get_process_family_id($db, $def->{'category'},  $def->{'class'},  $site, $fac);
                    my $pid_old = _get_process_family_id($db, $defc->{'category'}, $defc->{'class'}, $site, $fac);

                    if ($pid_old && $pid_new) {
                        if ($clean) {
                            unless (_delete_family_process($db, $defc->{'category'}, $defc->{'class'}, $def->{'type'}, $def->{'name'}, $site, $fac)) {
                                _set_error(__LINE__, "Failed to delete family process ($site.$fac)");
                                return 0;
                            }
                        }

                        delete($fc_map->{ "$site.$fac" });
                    }

                    next;
                    # Below would be cleaner but apparently a trigger complains:
                    # ERROR: Family Process definition violates unique proc_id/fac_id constraint

                    if ($pid_new && $pid_old) {

                        @args = ( $pid_new, $pid_old, $proc_id );
                        $retval = $db->do(trim(sprintf("

                        UPDATE family_processes
                        SET proc_fam_id = %d
                        WHERE proc_fam_id = %d
                          AND proc_id = %d

                        ", @args)));
                        unless (defined($retval)) {
                            _set_error(__LINE__, "Failed to update family process ID reference: $site.$fac", $db->error());
                            return 0;
                        }

                        $changes++;
                    }
                }
            }
        }

        my %f_map = ( );
        my $f_map = \%f_map;

        for my $f (@{$def->{'locations'}}) {

            my ($site, $fac) = _parse_location($f);
            unless (defined($site) && defined($fac)) {
                _set_error(__LINE__, "Badly formed location: $f");
                return 0;
            }

            $f_map->{ "$site.$fac" } = $f;

            next if (defined($fc_map->{ "$site.$fac" }));

            if (defined($full) && $full) {
                my ($def_lat, $def_lon, $def_alt) = ( '-9999', '-9999', '-9999' );
                $retval = $db->sp_call('sites', 'inquire', $site);
                unless (defined($retval) && scalar(@{$retval}) > 0) {
                    $retval = $db->sp_call('sites', 'define', $site, '');
                    unless (defined($retval)) {
                        _set_error(__LINE__, $db->error());
                        return 0;
                    }
                }
                $retval = $db->sp_call('facilities', 'inquire', $site, $fac);
                unless (defined($retval) && scalar(@{$retval}) > 0) {
                    $retval = $db->sp_call('locations', 'inquire', $def_lat, $def_lon, $def_alt);
                    unless (defined($retval)) {
                        _set_error(__LINE__, $db->error());
                        return 0;
                    }
                    unless (scalar(@{$retval}) > 0) {
                        $retval = $db->sp_call('locations', 'define', $def_lat, $def_lon, $def_alt, '');
                        unless (defined($retval)) {
                            _set_error(__LINE__, $db->error());
                            return 0;
                        }
                    }
                    $retval = $db->sp_call('facilities', 'define', $site, $fac, $def_lat, $def_lon, $def_alt);
                    unless (defined($retval)) {
                        _set_error(__LINE__, $db->error());
                        return 0;
                    }
                }
            }

            my $pfid = _get_process_family_id($db, $def->{'category'}, $def->{'class'}, $site, $fac);
            unless ($pfid) {

                # Define process family.

                $retval = $db->sp_call('process_families', 'define', $def->{'category'}, $def->{'class'}, $site, $fac, 0, 0, 0);
                unless (defined($retval)) {
                    _set_error(__LINE__, $db->error());
                    return 0;
                }
            }

            # Define family process (not critical anymore since family processes can be created at run-time, but doesn't hurt).

            $retval = $db->sp_call('family_processes', 'define', $def->{'category'}, $def->{'class'}, $site, $fac, $def->{'type'}, $def->{'name'});
            unless (defined($retval)) {
                _set_error(__LINE__, $db->error());
                return 0;
            }

            $changes++;
        }

        if ($clean) {
            for my $f (keys(%fc_map)) {
                next if (defined($f_map->{ $f }));

                my ($site, $fac) = ( $fc_map->{ $f }->{'site'}, $fc_map->{ $f }->{'fac'} );

                unless (_delete_family_process($db, $def->{'category'}, $def->{'class'}, $def->{'type'}, $def->{'name'}, $site, $fac)) {
                    _set_error(__LINE__, "Failed to delete family process ($site.$fac)");
                    return 0;
                }

                $changes++;
            }
        }

    } else {

        unless (define_process($db, $def->{'type'}, $def->{'name'}, $def)) {
            _set_error(__LINE__, 'Failed to define process');
            return 0;
        }

        $changes++;
    }

    # Let's not do this, actually, since it might do something bad when this is "installing" to production.
    # return(0) unless (delete_process_properties($db, $def->{'type'}, $def->{'name'}));

    for (keys(%{ $def->{'props'} })) {

        my $val = $def->{'props'}->{ $_ };

        if (defined($val)) {
            $retval = $db->sp_call('process_config_values', 'update', $def->{'type'}, $def->{'name'}, undef, undef, $_, $val);
            unless (defined($retval)) {
                _set_error(__LINE__, "Failed to set config value: $_ : $val", $db->error());
                return 0;
            }
        } else {
            $retval = $db->sp_call('process_config_values', 'delete', $def->{'type'}, $def->{'name'}, undef, undef, $_);
            unless (defined($retval)) {
                _set_error(__LINE__, "Failed to delete config value: $_", $db->error());
                return 0;
            }
        }

        $changes++;
    }

    if (defined($user) && $changes > 0) {
        unless (define_process_revision($db, $def->{'type'}, $def->{'name'}, $user, $json_data)) {
            _set_error(__LINE__, 'Failed to define process revision');
            return 0;
        }
    }

    return 1;
}

sub delete_process {
    my ($db, $ptype, $pname) = @_;

    return(0) unless (delete_process_definition($db, $ptype, $pname));

    my @args = ( $ptype, $pname );
    my $retval = $db->query_scalar('SELECT delete_proc_revisions(?,?)', \@args);
    unless (defined($retval)) {
        _set_error(__LINE__, $db->error());
        return 0;
    }

    $retval = $db->sp_call('processes', 'delete', $ptype, $pname);
    unless (defined($retval)) {
        _set_error(__LINE__, $db->error());
        return 0;
    }

    $retval = $db->sp_call('processes', 'inquire', '%', $pname);
    if (defined($retval) && scalar(@{$retval}) == 0) {

        my $pfcid = _get_process_family_class_id($db, ($ptype eq 'VAP') ? 'VAP' : 'Instrument', $pname);
        unless (defined($pfcid)) {
            _set_error(__LINE__, 'Failed to get process family class ID', $db->error());
            return 0;
        }
        return(1) if ($pfcid == 0);

        $retval = $db->do(trim(sprintf("

        DELETE FROM process_family_classes
        WHERE proc_fam_class_id = %d
          AND proc_fam_class_desc = ''

        ", $pfcid)));

        unless (defined($retval)) {
            _set_error(__LINE__, $db->error());
            return 0;
        }
    }

    return 1;
}

sub delete_process_properties {
    my ($db, $ptype, $pname) = @_;

    my $retval = $db->do(trim(sprintf("

    DELETE FROM process_config_values
    WHERE proc_type_name = '%s'
      AND proc_name = '%s'

    ", $ptype, $pname)));
    unless (defined($retval)) {
        _set_error(__LINE__, "Failed to remove process properties for $ptype-$pname", $db->error());
        return 0;
    }

    return 1;
}

sub delete_process_definition {
    my ($db, $ptype, $pname) = @_;

    return(0) unless (delete_retrieval($db, $ptype, $pname));

    my $pid = _get_process_id($db, $ptype, $pname);
    unless (defined($pid)) {
        _set_error(__LINE__, 'Failed to get process ID', $db->error());
        return 0;
    }
    return(1) if ($pid == 0);

    my $pfcid = _get_process_family_class_id($db, ($ptype eq 'VAP') ? 'VAP' : 'Instrument', $pname);
    unless (defined($pfcid)) {
        _set_error(__LINE__, 'Failed to get process family class ID', $db->error());
        return 0;
    }
    return(1) if ($pfcid == 0);

    my $retval = $db->do(trim(sprintf("

    DELETE FROM process_output_datastreams
    WHERE proc_out_dsc_id IN (
        SELECT proc_out_dsc_id
        FROM process_output_ds_classes
        WHERE proc_id = %d
    );

    DELETE FROM process_output_ds_classes
    WHERE proc_id = %d;

    DELETE FROM process_input_ds_classes
    WHERE proc_id = %d;

    DELETE FROM dsview_process_families
    WHERE proc_fam_id IN (
        SELECT DISTINCT proc_fam_id
        FROM process_families
        WHERE proc_fam_class_id = %d
    );

    DELETE FROM family_process_states
    WHERE fam_proc_id IN (
        SELECT fam_proc_id
        FROM family_processes
        WHERE proc_id = %d
    );

    DELETE FROM family_process_statuses
    WHERE fam_proc_id IN (
        SELECT fam_proc_id
        FROM family_processes
        WHERE proc_id = %d
    );

    DELETE FROM family_process_states_history
    WHERE fam_proc_id IN (
        SELECT fam_proc_id
        FROM family_processes
        WHERE proc_id = %d
    );

    DELETE FROM family_process_statuses_history
    WHERE fam_proc_id IN (
        SELECT fam_proc_id
        FROM family_processes
        WHERE proc_id = %d
    );

    DELETE FROM family_process_runtime_history
    WHERE fam_proc_id IN (
        SELECT fam_proc_id
        FROM family_processes
        WHERE proc_id = %d
    );

    DELETE FROM family_processes
    WHERE proc_id = %d;

    DELETE FROM process_families
    WHERE proc_fam_class_id = %d

    ", $pid, $pid, $pid, $pfcid, $pid, $pid, $pid, $pid, $pid, $pid, $pfcid)));

    unless (defined($retval)) {
        _set_error(__LINE__, $db->error());
        return 0;
    }

    # I'm scared to do this.  So let's not for now.
    #return(0) unless (delete_process_properties($db, $ptype, $pname));

    return 1;
}

sub define_process_revision {
    my ($db, $ptype, $pname, $user, $encoded) = @_;

    my @args = ( $ptype, $pname, $user, $encoded );
    my $retval = $db->query_scalar('SELECT define_proc_revision(?,?,?,?)', \@args);
    unless (defined($retval)) {
        _set_error(__LINE__, $db->error());
        return 0;
    }

    return 1;
}

sub define_process {
    my ($db, $ptype, $pname, $proc) = @_;

    my $retval;

    # Define process.
    $retval = $db->sp_call('processes', 'define', $ptype, $pname, '');
    unless (defined($retval)) {
        _set_error(__LINE__, $db->error());
        return 0;
    }

    # Define process outputs.
    if ($proc->{'outputs'}) {
        for (@{$proc->{'outputs'}}) {
            my ($class, $level);
            if (ref($_) eq 'HASH') {
                ($class, $level) = ( $_->{'class'}, $_->{'level'} );
            } else {
                ($class, $level) = split(/\./, $_);
                unless (defined($class) && defined($level)) {
                    _set_error(__LINE__, "Badly formed output ds class: $_");
                    return 0;
                }
            }

            $retval = $db->sp_call('datastream_classes', 'define', $class, $level, '');
            unless (defined($retval)) {
                _set_error(__LINE__, $db->error());
                return 0;
            }

            $retval = $db->sp_call('process_output_ds_classes', 'define', $ptype, $pname, $class, $level);
            unless (defined($retval)) {
                _set_error(__LINE__, $db->error());
                return 0;
            }
        }

    } else {
        _set_error(__LINE__, 'Missing output list.');
        return 0;
    }

    # Define process inputs.
    if (defined($proc->{'ret'})) {

        return(0) unless (define_retrieval($db, $ptype, $pname, $proc->{'ret'}));

    } elsif (defined($proc->{'inputs'})) {

        for (@{$proc->{'inputs'}}) {
            my ($class, $level);
            if (ref($_) eq 'HASH') {
                ($class, $level) = ( $_->{'class'}, $_->{'level'} );
            } else {
                ($class, $level) = split(/\./, $_);
                unless (defined($class) && defined($level)) {
                    _set_error(__LINE__, "Badly formed input ds class: $_");
                    return 0;
                }
            }

            $retval = $db->sp_call('datastream_classes', 'define', $class, $level, '');
            unless (defined($retval)) {
                _set_error(__LINE__, $db->error());
                return 0;
            }

            $retval = $db->sp_call('process_input_ds_classes', 'define', $ptype, $pname, $class, $level);
            unless (defined($retval)) {
                _set_error(__LINE__, $db->error());
                return 0;
            }
        }

    } else {
        _set_error(__LINE__, 'Missing input list.');
        return 0;
    }

    my $class     = $proc->{'class'};
    my $category  = $proc->{'category'};
    my $locations = $proc->{'locations'};

    unless (
        defined($class) &&
        defined($category) &&
        defined($locations)
    ) {
        _set_error(__LINE__, 'Incomplete input structure.');
        return 0;
    }

    # Define process family class.
    $retval = $db->sp_call('process_family_classes', 'inquire', $category, $class);
    unless (defined($retval) && scalar(@{$retval}) > 0) {

        # The process family class cannot be redefined cleanly like a process and datastream class.
        # Instead, we need to only define it if it does not yet exist.  Not a big deal.

        $retval = $db->sp_call('process_family_classes', 'define', $category, $class, '');
        unless (defined($retval)) {
            _set_error(__LINE__, $db->error());
            return 0;
        }
    }

    for my $f (@{$locations}) {
        my ($site, $fac) = _parse_location($f);
        unless (defined($site) && defined($fac)) {
            _set_error(__LINE__, "Badly formed location: $f");
            return 0;
        }

        # Define process family.
        $retval = $db->sp_call('process_families', 'define', $category, $class, $site, $fac, 0, 0, 0);
        unless (defined($retval)) {
            _set_error(__LINE__, $db->error());
            return 0;
        }

        # Define family process.
        $retval = $db->sp_call('family_processes', 'define', $category, $class, $site, $fac, $ptype, $pname);
        unless (defined($retval)) {
            _set_error(__LINE__, $db->error());
            return 0;
        }
    }

    return 1;
}

sub get_retrieval_list {
    my ($db) = @_;

    my @args = ( );
    my @cols = ( 'name', 'desc', 'type' );
    my $list = $db->query_hasharray(trim("

    SELECT DISTINCT proc_name, proc_desc, proc_type_name
     FROM processes
     NATURAL JOIN process_types
     NATURAL JOIN ret_var_groups

    "), \@args, \@cols);
    unless (defined($list)) {
        _set_error(__LINE__, 'Failed to get list of retrievals.', $db->error());
        return undef;
    }

    return $list;
}

sub get_retrieval {
    my ($db, $ptype, $pname) = @_;

    my $pdesc = '';

    my %groups  = ( ); my $groups  = \%groups;
    my %queries = ( ); my $queries = \%queries;
    my %shapes  = ( ); my $shapes  = \%shapes;
    my @fields  = ( ); my $fields  = \@fields;
    my %trans   = ( ); my $trans   = \%trans;

    my $retval;
    my (@cols, @args);

    $retval = $db->sp_call('processes', 'inquire', $ptype, $pname);
    unless (defined($retval)) {
        _set_error(__LINE__, 'Failed to get description.', $db->error());
        return undef;
    }

    if (scalar(@{$retval}) > 0) {
        $pdesc = $retval->[0]->{'description'};
    }

    @cols = ( 'ptype', 'pname', 'gname' );
    @args = ( $ptype, $pname );
    $retval = $db->query_hasharray('SELECT * FROM get_ret_ds_groups(?,?)', \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, 'Failed to get group names.', $db->error());
        return undef;
    }
    for (@{$retval}) {
        my $gname = $_->{'gname'};

        @cols = ( 'ptype', 'pname', 'gname', 'qname', 'index' );
        @args = ( $ptype, $pname, $gname );
        my $retval = $db->query_hasharray('SELECT * FROM get_ret_ds_subgroups_by_group(?,?,?)', \@args, \@cols);
        unless (defined($retval)) {
            _set_error(__LINE__, "Failed to get query names: $gname", $db->error());
            return undef;
        }

        @{$retval} = sort { $a->{'index'} <=> $b->{'index'} } @{$retval};

        my @queries = ( );
        for (@{$retval}) {
            push(@queries, $_->{'qname'});

            unless (defined($queries->{ $_->{'qname'} })) {
                my @arr = ( );
                $queries->{ $_->{'qname'} } = \@arr;
            }
        }

        $groups->{ $_->{'gname'} } = \@queries;
    }

    @cols = ( 'ptype', 'pname', 'cname' );
    @args = ( $ptype, $pname );
    $retval = $db->query_hasharray('SELECT * FROM get_ret_coord_systems(?,?)', \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, 'Failed to get coordinate system names.', $db->error());
        return undef;
    }

    my %smap = ( );
    my $smap = \%smap;
    for (@{$retval}) {
        my $cname = $_->{'cname'};

        unless (defined($smap->{ $cname })) {
            my %h = ( );
            $smap->{ $cname } = \%h;
        }

        @cols = ( 'ptype', 'pname', 'cname', 'dname', 'index', 'query', 'interval', 'units', 'dtype', 'ttype', 'trange', 'talign', 'start', 'length' );
        @args = ( $ptype, $pname, $cname );
        my $retval = $db->query_hasharray('SELECT * FROM get_ret_coord_dims(?,?,?)', \@args, \@cols);
        unless (defined($retval)) {
            _set_error(__LINE__, "Failed to get dimensions: $cname", $db->error());
            return undef;
        }

        @{$retval} = sort { $a->{'index'} <=> $b->{'index'} } @{$retval};

        my @dims = ( );
        for (@{$retval}) {
            my $dname = $_->{'dname'};

            my @vars = ( );
            my %dim = (
                'name'     => $_->{'dname'},
                'query'    => $_->{'query'},
                'vars'     => \@vars,
                'units'    => $_->{'units'},
                'interval' => $_->{'interval'},
                'dtype'    => $_->{'dtype'},
                'ttype'    => $_->{'ttype'},
                'trange'   => $_->{'trange'},
                'talign'   => $_->{'talign'},
                'start'    => $_->{'start'},
                'length'   => $_->{'length'}
            );
            push(@dims, \%dim);
            $smap->{ $cname }->{ $dname } = \%dim;

            unless (!defined($_->{'query'}) || defined($queries->{ $_->{'query'} })) {
                my @arr = ( );
                $queries->{ $_->{'query'} } = \@arr;
            }
        }

        $shapes->{ $_->{'cname'} } = \@dims;
    }

    @cols = ( 'file_name', 'file_contents' );
    @args = ( $ptype, $pname );
    $retval = $db->query_hasharray(trim("

    SELECT transform_file_name, transform_file_contents
     FROM processes
     NATURAL JOIN process_types
     NATURAL JOIN ret_transform_params
     WHERE proc_type_name = ? AND proc_name = ?

    "), \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, 'Failed to get transform params.', $db->error());
        return undef;
    }
    for (@{$retval}) {
        $trans->{ $_->{'file_name'} } = $_->{'file_contents'};
    }

    @cols = ( 'ptype', 'pname', 'class', 'level', 'qname', 'site', 'fac', 'index', 'sdep', 'fdep', 'tdep0', 'tdep1' );
    @args = ( $ptype, $pname );
    $retval = $db->query_hasharray('SELECT * FROM get_ret_datastreams_by_process(?,?)', \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, 'Failed to get datastreams.', $db->error());
        return undef;
    }

    @{$retval} = sort {
        $a->{'index'} <=> $b->{'index'}
    } @{$retval};

    my %qmap = ( );
    my $qmap = \%qmap;

    for (@{$retval}) {
        my $q = $_->{'qname'};
        my %q = (
            'order' => int( $_->{'index'} ),
            'class' => $_->{'class'},
            'level' => $_->{'level'},
            'site'  => $_->{'site'},
            'fac'   => $_->{'fac'},
            'sdep'  => $_->{'sdep'},
            'fdep'  => $_->{'fdep'},
            'tdep0' => $_->{'tdep0'},
            'tdep1' => $_->{'tdep1'},
        );
        push(@{$queries->{ $q }}, \%q);
        unless (defined($qmap->{ $q })) {
            my %h = ( );
            $qmap->{ $q } = \%h;
        }
        $qmap->{ $q }->{ _select_to_key(\%q) } = $#{$queries->{ $q }};
    }

    @cols = ( 'ptype', 'pname', 'class', 'level', 'qname', 'site', 'fac', 'sdep', 'fdep', 'tdep0', 'sname', 'dname', 'vname', 'index' );
    @args = ( $ptype, $pname );
    $retval = $db->query_hasharray('SELECT * FROM get_ret_var_dim_names_by_process(?,?)', \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, "Failed to get coordinate var name(s).", $db->error());
        return undef;
    }

    @{$retval} = sort { $a->{'index'} <=> $b->{'index'} } @{$retval};

    for (@{$retval}) {
        my $s = $_->{'sname'};
        my $d = $_->{'dname'};

        my $key = _select_to_key($_);

        my $spos = $qmap->{ $smap->{ $s }->{ $d }->{'query'} }->{ $key };
        my $vars = $smap->{ $s }->{ $d }->{'vars'};

        if (defined($vars->[ $spos ])) {
            push(@{$vars->[ $spos ]}, $_->{'vname'});
        } else {
            my @a = ( $_->{'vname'} );
            $vars->[ $spos ] = \@a;
        }
    }

    @cols = ( 'ptype', 'pname', 'class', 'level', 'qname', 'site', 'fac', 'sdep', 'fdep', 'tdep0', 'gname', 'fname', 'vname', 'index' );
    @args = ( $ptype, $pname );
    $retval = $db->query_hasharray('SELECT * FROM get_ret_var_names_by_process(?,?)', \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, "Failed to get field names.", $db->error());
        return undef;
    }

    @{$retval} = sort { $a->{'index'} <=> $b->{'index'} } @{$retval};

    my %fvars = ( );
    my $fvars = \%fvars;
    for (@{$retval}) {
        my $f = $_->{'fname'};
        my $g = $_->{'gname'};
        my $q = $_->{'qname'};
        my $v = $_->{'vname'};

        unless (defined($fvars->{ $f })) {
            my @a = ( );
            $fvars->{ $f } = \@a;
        }

        my $offset = 0;
        for (@{$groups->{ $g }}) {
            last if ($_ eq $q);
            $offset += scalar(keys(%{$qmap->{ $_ }}));
        }

        my $key = _select_to_key($_);

        my $spos = $qmap->{ $_->{'qname'} }->{ $key };
        my $vars = $fvars->{ $f };

        $spos += $offset;

        unless (defined($vars->[ $spos ])) {
            @{$vars->[ $spos ]} = ();
        }
        push(@{$vars->[ $spos ]}, "$v");
    }

    @cols = ( 'gname', 'ptype', 'pname', 'fname', 'sname', 'units', 'dtype', 'w0', 'w1', 'max', 'min', 'delta', 'reqd', 'qc', 'qcreqd' );
    @args = ( $ptype, $pname );
    $retval = $db->query_hasharray('SELECT * FROM get_ret_var_groups_by_process(?,?)', \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, "Failed to get fields", $db->error());
        return undef;
    }

    for (@{$retval}) {
        my @a = ( );
        my %f = (
            'name'   => $_->{'fname'},
            'group'  => $_->{'gname'},
            'shape'  => $_->{'sname'},
            'dims'   => [ ],
            'units'  => $_->{'units'},
            'dtype'  => $_->{'dtype'},
            'window' => [ $_->{'w0'}, $_->{'w1'} ],
            'min'    => $_->{'min'},
            'max'    => $_->{'max'},
            'delta'  => $_->{'delta'},
            'reqd'   => ($_->{'reqd'})   ? 1 : 0,
            'qc'     => ($_->{'qc'})     ? 1 : 0,
            'qcreqd' => ($_->{'qcreqd'}) ? 1 : 0,
            'vars'   => (defined($fvars->{ $_->{'fname'} })) ? $fvars->{ $_->{'fname'} } : \@a,
            'out'    => [ ],
        );
        push(@fields, \%f);
    }

    @cols = ( 'gname', 'fname', 'dname', 'order' );
    @args = ( $ptype, $pname );
    $retval = $db->query_hasharray('SELECT * FROM get_ret_var_dims(?,?)', \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, 'Failed to get field dimensions', $db->error());
        return undef;
    }

    my %fmap = ( );
    my $fmap = \%fmap;
    for (@fields) {
        $fmap->{ $_->{'group'} . $_->{'name'} } = $_;
    }

    # Order is set in "get" stored procedure.

    for my $r (@{$retval}) {
        my $f = $fmap->{ $r->{'gname'} . $r->{'fname'} };
        next unless (defined($f));
        push(@{ $f->{'dims'} }, $r->{'dname'});
    }

    @cols = ( 'gname', 'fname', 'class', 'level', 'oname' );
    @args = ( $ptype, $pname );
    $retval = $db->query_hasharray('SELECT * FROM get_ret_var_outputs(?,?)', \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, 'Failed to get field outputs', $db->error());
        return undef;
    }

    for my $r (@{$retval}) {
        my $f = $fmap->{ $r->{'gname'} . $r->{'fname'} };
        next unless (defined($f));
        my %m = (
            'd' => $r->{'class'} . '.' . $r->{'level'},
            'f' => $r->{'oname'},
        );
        push(@{ $f->{'out'} }, \%m);
    }

    # We need epoch format for UI, but have to wait til now because the timestamp string is needed elsewhere.

    for (keys(%queries)) {
        for (@{$queries->{ $_ }}) {
            $_->{'tdep0'} = (defined($_->{'tdep0'})) ? $db->timestamp_to_seconds( $_->{'tdep0'} ) : undef;
            $_->{'tdep1'} = (defined($_->{'tdep1'})) ? $db->timestamp_to_seconds( $_->{'tdep1'} ) : undef;
        }
    }

    my %ret = (
        'name'    => $pname,
        'desc'    => $pdesc,
        'groups'  => $groups,
        'queries' => $queries,
        'shapes'  => $shapes,
        'fields'  => $fields,
        'transforms' => $trans,
    );

    return \%ret;
}

sub get_retrieval_slow {
    my ($db, $ptype, $pname) = @_;

    my $pdesc = '';

    my %groups  = ( ); my $groups  = \%groups;
    my %queries = ( ); my $queries = \%queries;
    my %shapes  = ( ); my $shapes  = \%shapes;
    my @fields  = ( ); my $fields  = \@fields;

    my $retval;
    my (@cols, @args);

    $retval = $db->sp_call('processes', 'inquire', $ptype, $pname);
    unless (defined($retval)) {
        _set_error(__LINE__, 'Failed to get description.', $db->error());
        return undef;
    }

    if (scalar(@{$retval}) > 0) {
        $pdesc = $retval->[0]->{'description'};
    }

    @cols = ( 'ptype', 'pname', 'gname' );
    @args = ( $ptype, $pname );
    $retval = $db->query_hasharray('SELECT * FROM get_ret_ds_groups(?,?)', \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, 'Failed to get group names.', $db->error());
        return undef;
    }
    for (@{$retval}) {
        my $gname = $_->{'gname'};

        @cols = ( 'ptype', 'pname', 'gname', 'qname', 'index' );
        @args = ( $ptype, $pname, $gname );
        my $retval = $db->query_hasharray('SELECT * FROM get_ret_ds_subgroups_by_group(?,?,?)', \@args, \@cols);
        unless (defined($retval)) {
            _set_error(__LINE__, "Failed to get query names: $gname", $db->error());
            return undef;
        }

        @{$retval} = sort { $a->{'index'} <=> $b->{'index'} } @{$retval};

        my @queries = ( );
        for (@{$retval}) {
            push(@queries, $_->{'qname'});

            unless (defined($queries->{ $_->{'qname'} })) {
                my @arr = ( );
                $queries->{ $_->{'qname'} } = \@arr;
            }
        }

        $groups->{ $_->{'gname'} } = \@queries;
    }

    @cols = ( 'ptype', 'pname', 'cname' );
    @args = ( $ptype, $pname );
    $retval = $db->query_hasharray('SELECT * FROM get_ret_coord_systems(?,?)', \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, 'Failed to get coordinate system names.', $db->error());
        return undef;
    }
    for (@{$retval}) {
        my $cname = $_->{'cname'};

        @cols = ( 'ptype', 'pname', 'cname', 'dname', 'index', 'query', 'interval', 'units' );
        @args = ( $ptype, $pname, $cname );
        my $retval = $db->query_hasharray('SELECT * FROM get_ret_coord_dims(?,?,?)', \@args, \@cols);
        unless (defined($retval)) {
            _set_error(__LINE__, "Failed to get dimensions: $cname", $db->error());
            return undef;
        }

        @{$retval} = sort { $a->{'index'} <=> $b->{'index'} } @{$retval};

        my @dims = ( );
        for (@{$retval}) {
            my @vars = ( );
            my %dim = (
                'name'     => $_->{'dname'},
                'query'    => $_->{'query'},
                'vars'     => \@vars,
                'interval' => $_->{'interval'},
                'units'    => $_->{'units'},
            );
            push(@dims, \%dim);

            unless (!defined($_->{'query'}) || defined($queries->{ $_->{'query'} })) {
                my @arr = ( );
                $queries->{ $_->{'query'} } = \@arr;
            }
        }

        $shapes->{ $_->{'cname'} } = \@dims;
    }

    for (keys(%queries)) {

        @cols = ( 'ptype', 'pname', 'class', 'level', 'qname', 'site', 'fac', 'index', 'sdep', 'fdep', 'tdep0', 'tdep1' );
        @args = ( $ptype, $pname, $_ );
        my $retval = $db->query_hasharray('SELECT * FROM get_ret_datastreams(?,?,?)', \@args, \@cols);
        unless (defined($retval)) {
            _set_error(__LINE__, "Failed to get datastreams: $_", $db->error());
            return undef;
        }

        @{$retval} = sort { $a->{'index'} <=> $b->{'index'} } @{$retval};

        my @selects = ( );
        for (@{$retval}) {
            my %q = (
                'order' => int( $_->{'index'} ),
                'class' => $_->{'class'},
                'level' => $_->{'level'},
                'site'  => $_->{'site'},
                'fac'   => $_->{'fac'},
                'sdep'  => $_->{'sdep'},
                'fdep'  => $_->{'fdep'},
                'tdep0' => $_->{'tdep0'},
                'tdep1' => $_->{'tdep1'},
            );
            push(@selects, \%q);
        }

        $queries->{ $_ } = \@selects;
    }

    for my $sname (keys(%shapes)) {

        for my $dim (@{$shapes->{ $sname }}) {
            next unless ($dim->{'query'});

            my $qlist = $queries->{ $dim->{'query'} };
            next unless (defined($qlist));

            for my $q (@{$qlist}) {

                @cols = ( 'ptype', 'pname', 'class', 'level', 'qname', 'site', 'fac', 'sdep', 'fdep', 'tdep0', 'sname', 'dname', 'vname', 'index' );
                @args = ( $ptype, $pname, $q->{'class'}, $q->{'level'}, $dim->{'query'}, $q->{'site'}, $q->{'fac'}, $q->{'sdep'}, $q->{'fdep'}, $q->{'tdep0'}, $sname, $dim->{'name'} );
                my $retval = $db->query_hasharray('SELECT * FROM get_ret_var_dim_names(?,?,?,?,?,?,?,?,?,?,?,?)', \@args, \@cols);
                unless (defined($retval)) {
                    _set_error(__LINE__, "Failed to get coordinate var name(s): $_->{'dname'}", $db->error());
                    return undef;
                }

                @{$retval} = sort { $a->{'index'} <=> $b->{'index'} } @{$retval};

                my @v = ( );
                for (@{$retval}) {
                    push(@v, $_->{'vname'});
                }
                push(@{$dim->{'vars'}}, \@v);
            }
        }
    }

    @cols = ( 'gname', 'ptype', 'pname', 'fname', 'sname', 'units', 'dtype', 'w0', 'w1', 'max', 'min', 'delta', 'reqd', 'qc', 'qcreqd' );
    @args = ( $ptype, $pname );
    $retval = $db->query_hasharray('SELECT * FROM get_ret_var_groups_by_process(?,?)', \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, "Failed to get fields", $db->error());
        return undef;
    }

    for (@{$retval}) {

        my $gname = $_->{'gname'};
        my $fname = $_->{'fname'};

        my @vnames = ( );

        for (@{$groups->{ $gname }}) {

            my $qname = $_;
            my $qlist = $queries->{ $_ };

            for my $q (@{$qlist}) {

                @cols = ( 'ptype', 'pname', 'class', 'level', 'qname', 'site', 'fac', 'sdep', 'fdep', 'tdep0', 'gname', 'fname', 'vname', 'index' );
                @args = ( $ptype, $pname, $q->{'class'}, $q->{'level'}, $qname, $q->{'site'}, $q->{'fac'}, $q->{'sdep'}, $q->{'fdep'}, $q->{'tdep0'}, $gname, $fname );
                my $retval = $db->query_hasharray('SELECT * FROM get_ret_var_names(?,?,?,?,?,?,?,?,?,?,?,?)', \@args, \@cols);
                unless (defined($retval)) {
                    _set_error(__LINE__, "Failed to get field names for $fname:$gname:$qname", $db->error());
                    return undef;
                }

                @{$retval} = sort { $a->{'index'} <=> $b->{'index'} } @{$retval};

                my @v = ( );
                for (@{$retval}) {
                    push(@v, $_->{'vname'});
                }
                push(@vnames, \@v);
            }
        }

        my %f = (
            'name'   => $fname,
            'group'  => $gname,
            'shape'  => $_->{'sname'},
            'units'  => $_->{'units'},
            'dtype'  => $_->{'dtype'},
            'window' => [ $_->{'w0'}, $_->{'w1'} ],
            'min'    => $_->{'min'},
            'max'    => $_->{'max'},
            'delta'  => $_->{'delta'},
            'reqd'   => ($_->{'reqd'})   ? 1 : 0,
            'qc'     => ($_->{'qc'})     ? 1 : 0,
            'qcreqd' => ($_->{'qcreqd'}) ? 1 : 0,
            'vars'   => \@vnames,
        );

        push(@fields, \%f);
    }

    # We need epoch format, but have to wait til now because the timestamp string is needed by other queries.

    for (keys(%queries)) {
        for (@{$queries->{ $_ }}) {
            $_->{'tdep0'} = (defined($_->{'tdep0'})) ? $db->timestamp_to_seconds( $_->{'tdep0'} ) : undef;
            $_->{'tdep1'} = (defined($_->{'tdep1'})) ? $db->timestamp_to_seconds( $_->{'tdep1'} ) : undef;
        }
    }

    my %ret = (
        'name'    => $pname,
        'desc'    => $pdesc,
        'groups'  => $groups,
        'queries' => $queries,
        'shapes'  => $shapes,
        'fields'  => $fields,
    );

    return \%ret;
}

sub define_retrieval {
    my ($db, $ptype, $pname, $ret) = @_;

    unless (
        defined($ret->{'groups'}) &&
        defined($ret->{'queries'}) &&
        defined($ret->{'shapes'}) &&
        defined($ret->{'fields'})
    ) {
        _set_error(__LINE__, 'Incomplete inputs.');
        return 0;
    }

    my $retval;
    my @args;

    return(0) unless (delete_retrieval($db, $ptype, $pname));

    my $proc_id = _get_process_id($db, $ptype, $pname);
    unless (defined($proc_id)) {
        _set_error(__LINE__, "Failed to look up process ID: $ptype-$pname");
        return 0;
    }

    if (defined($ret->{'transforms'})) {
        for my $file_name (keys(%{$ret->{'transforms'}})) {
            my $file_contents = $ret->{'transforms'}->{$file_name};

            @args = ( $proc_id, $file_name, $file_contents );
            $retval = $db->query_scalar(trim("

            SELECT insert_ret_transform_param(?, ?, ?)

            "), \@args);
            unless (defined($retval)) {
                _set_error(__LINE__, $db->error());
                return undef;
            }
        }
    }

    for my $qname (keys(%{$ret->{'queries'}})) {
        return(0) unless (_db_def($db, 'define_ret_ds_subgroup', $ptype, $pname, $qname));

        my $spos = 1;
        my $q = $ret->{'queries'}->{ $qname };

        my @spread = ( );
        for my $sel (@{$q}) {

            unless (ds_class_exists($db, $sel->{'class'}, $sel->{'level'})) {
                my $retval = $db->sp_call('datastream_classes', 'define', $sel->{'class'}, $sel->{'level'}, '');
                unless (defined($retval)) {
                    _set_error(__LINE__, $db->error());
                    return 0;
                }
            }

            $sel->{'tdep0'} = ($sel->{'tdep0'}) ? $db->seconds_to_timestamp($sel->{'tdep0'}) : undef;
            $sel->{'tdep1'} = ($sel->{'tdep1'}) ? $db->seconds_to_timestamp($sel->{'tdep1'}) : undef;

            return(0) unless(_db_def($db, 'define_ret_datastream',
                $ptype,
                $pname,
                $sel->{'class'},
                $sel->{'level'},
                $qname,
                $sel->{'site'},
                $sel->{'fac'},
                (defined( $sel->{ 'order' } )) ? $sel->{ 'order' } : $spos++,
                $sel->{'sdep'},
                $sel->{'fdep'},
                $sel->{'tdep0'},
                $sel->{'tdep1'}
            ));
            my %h = (
                'class' => $sel->{'class'},
                'level' => $sel->{'level'},
                'site'  => $sel->{'site'},
                'fac'   => $sel->{'fac'},
                'sdep'  => $sel->{'sdep'},
                'fdep'  => $sel->{'fdep'},
                'tdep0' => $sel->{'tdep0'},
                'tdep1' => $sel->{'tdep1'},
            );
            push(@spread, \%h);
        }
        $ret->{'queries'}->{ $qname} = \@spread;
    }
    for my $gname (keys(%{$ret->{'groups'}})) {
        return(0) unless(_db_def($db, 'define_ret_ds_group', $ptype, $pname, $gname));
        my $qpos = 1;
        for my $qname (@{$ret->{'groups'}->{$gname}}) {
            return(0) unless (_db_def($db, 'define_ret_ds_group_subgroup', $ptype, $pname, $gname, $qname, $qpos++));
        }
    }

    for my $shape (keys(%{$ret->{'shapes'}})) {
        return(0) unless (_db_def($db, 'define_ret_coord_system', $ptype, $pname, $shape));

        my $dpos = 1;
        my $c = $ret->{'shapes'}->{ $shape };
        for my $d (@{$c}) {
            return(0) unless (_db_def($db, 'define_ret_coord_dim',
                $ptype,
                $pname,
                $shape,
                $d->{'name'},
                $dpos++,
                $d->{'query'},
                (defined($d->{'interval'})) ? $d->{'interval'} * 1.0 : undef,
                (defined($d->{'units'}))    ? $d->{'units'}          : undef,
                (defined($d->{'dtype'}))    ? $d->{'dtype'}          : undef,
                (defined($d->{'ttype'}))    ? $d->{'ttype'}          : undef,
                (defined($d->{'trange'}))   ? $d->{'trange'}   * 1.0 : undef,
                (defined($d->{'talign'}))   ? int($d->{'talign'})    : undef,
                (defined($d->{'start'}))    ? $d->{'start'}    * 1.0 : undef,
                (defined($d->{'length'}))   ? $d->{'length'}   * 1.0 : undef
            ));

            next unless (defined($d->{'query'}) && defined($d->{'vars'}));

            my $q = $ret->{'queries'}->{ $d->{'query'} };
            next unless (defined($q));

            my $isel = 0;
            for my $v (@{$d->{'vars'}}) {
                if (ref($v) ne 'ARRAY') {
                    my @arr = ( $v );
                    $v = \@arr;
                }
                my $vpos = 1;
                for my $var (@{$v}) {
                    return(0) unless(_db_def($db, 'define_var_name', $var));
                    return(0) unless(_db_def($db, 'define_ret_var_dim_name',
                        $ptype,
                        $pname,
                        $q->[ $isel ]->{'class'},
                        $q->[ $isel ]->{'level'},
                        $d->{'query'},
                        $q->[ $isel ]->{'site'},
                        $q->[ $isel ]->{'fac'},
                        $q->[ $isel ]->{'sdep'},
                        $q->[ $isel ]->{'fdep'},
                        $q->[ $isel ]->{'tdep0'},
                        $shape,
                        $d->{'name'},
                        $var,
                        $vpos++
                    ));
                }
                $isel++;
            }
        }
    }

    for my $f (@{$ret->{'fields'}}) {
        return(0) unless (_db_def($db, 'define_ret_var_group',
            $f->{'group'},
            $ptype,
            $pname,
            $f->{'name'},
            $f->{'shape'},
            $f->{'units'},
            $f->{'dtype'},
            ($f->{'window'})?$f->{'window'}->[0]:undef,
            ($f->{'window'})?$f->{'window'}->[1]:undef,
            $f->{'min'},
            $f->{'max'},
            $f->{'delta'},
            ($f->{'reqd'})?1:0,
            ($f->{'qc'})?1:0,
            ($f->{'qcreqd'})?1:0
        ));

        my $vindex = 0;
        for my $qname (@{$ret->{'groups'}->{ $f->{'group'} }}) {
            my $q = $ret->{'queries'}->{ $qname };

            for my $sel (@{$q}) {
                my $v = $f->{'vars'}->[ $vindex ];
                if (ref($v) ne 'ARRAY') {
                    my @arr = ( $v );
                    $v = \@arr;
                }
                my $vpos = 1;
                for my $var (@{$v}) {
                    unless (defined($var)) {
                        $var = $f->{'name'}; # Empty var_name defaults to custom field name.
                    }

                    return(0) unless (_db_def($db, 'define_var_name', $var));
                    return(0) unless (_db_def($db, 'define_ret_var_name',
                        $ptype,
                        $pname,
                        $sel->{'class'},
                        $sel->{'level'},
                        $qname,
                        $sel->{'site'},
                        $sel->{'fac'},
                        $sel->{'sdep'},
                        $sel->{'fdep'},
                        $sel->{'tdep0'},
                        $f->{'group'},
                        $f->{'name'},
                        $var,
                        $vpos++
                    ));
                }
                $vindex++;
            }
        }

        if (defined($f->{'dims'})) {
            my $dindex = 0;
            for my $dim (@{ $f->{'dims'} }) {
                return(0) unless (
                    _db_def($db, 'define_ret_var_dim', $ptype, $pname, $f->{'group'}, $f->{'name'}, $dim, $dindex)
                );
                $dindex++;
            }
        }

        if (defined($f->{'out'})) {
            for my $o (@{ $f->{'out'} }) {
                my $d = $o->{'d'};
                my $v = $o->{'f'};
                next unless (defined($d) && defined($v));
                my ($class, $level) = split(/\./, $d);
                next unless (defined($class) && defined($level));
                return(0) unless (
                    _db_def($db, 'define_ret_var_output',
                        $ptype, $pname, $class, $level, $f->{'group'}, $f->{'name'}, $v
                    )
                );
            }
        }
    }

    return 1;
}

sub delete_retrieval {
    my ($db, $type, $name) = @_;

    # In case there are remnants of a retrieval, let's bypass this check.
    # return(1) unless( has_retrieval($db, $type, $name) );
    my @args = ( $type, $name );
      my $retval = $db->query_scalar("SELECT delete_retriever(?,?)", \@args);
    unless (defined($retval)) {
        _set_error(__LINE__, 'Failed to delete existing retrieval', $db->error());
        return 0;
    }

    return 1;
}

sub has_retrieval {
    my ($db, $type, $name) = @_;

    my @args = ( $type, $name );
    my $count = $db->query_scalar(trim("

    SELECT COUNT(DISTINCT(proc_name))
     FROM processes
     NATURAL JOIN process_types
     NATURAL JOIN ret_datastreams
     WHERE proc_type_name = ? AND proc_name = ?

    "), \@args);
    unless (defined($count)) {
        _set_error('Query Error', "Failed to determine if retrieval exists: $name:$type");
        return undef;
    }

    return $count;
}

sub get_process_inputs {
    my ($db, $ptype, $pname) = @_;
    my ( @cols, @args, $retval );

    @cols = ( 'ptype', 'pname', 'class', 'level', 'qname', 'site', 'fac', 'sdep', 'fdep', 'tdep0', 'gname', 'fname', 'vname', 'index' );
    @args = ( $ptype, $pname );
    $retval = $db->query_hasharray('SELECT * FROM get_ret_var_names_by_process(?,?)', \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, "Failed to get field names.", $db->error());
        return undef;
    }

    unless (scalar(@{$retval}) > 0) {
        @cols = ( 'ptype', 'pname', 'class', 'level' );
        @args = ( $ptype, $pname );
        $retval = $db->query_hasharray('SELECT * FROM get_process_input_ds_classes(?,?)', \@args, \@cols);
        unless (defined($retval)) {
            _set_error(__LINE__, "Failed to get input datastream class names: $ptype:$pname", $db->error());
            return undef;
        }
    }

    my %map = ( );
    my $map = \%map;
    for (@{$retval}) {
        my $key = $_->{'class'} . '.' . $_->{'level'};
        unless (defined($map->{ $key })) {
            my %h = (
                'class' => $_->{'class'},
                'level' => $_->{'level'},
                'sites' => [ ],
                'names' => [ ]
            );
            $map->{ $key } = \%h;
        }
        my $r = $map->{ $key };
        if (defined($_->{'site'}) || defined($_->{'sdep'})) {
            my $s = (defined($_->{'site'})) ? $_->{'site'} : $_->{'sdep'};
            my $f = (defined($_->{'fac'}))  ? $_->{'fac'}  : $_->{'fdep'};
            unless ( grep { $_ eq "$s.$f" } @{$r->{'sites'}}) {
                push(@{ $r->{'sites'} }, "$s.$f");
            }
        }
        if (defined($_->{'vname'})) {
            my $v = "$pname : $ptype" . ' | ' . $_->{'vname'};
            unless ( grep { $_ eq $v } @{$r->{'names'}}) {
                push(@{ $r->{'names'} }, $v);
            }
        }
    }

    my @list = values(%map);
    return \@list;
}

sub get_process_outputs {
    my ($db, $ptype, $pname) = @_;

    my @cols = ( 'ptype', 'pname', 'class', 'level' );
    my @args = ( $ptype, $pname );
    my $retval = $db->query_hasharray('SELECT * FROM get_process_output_ds_classes(?,?)', \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, "Failed to get output datastream class names: $ptype:$pname", $db->error());
        return undef;
    }

    for my $r (@{$retval}) {
        delete($r->{'ptype'});
        delete($r->{'pname'});
    }

    return $retval;
}

sub get_process_locations {
    my ($db, $ptype, $pname) = @_;

    my @args = ( $ptype, $pname );
    my @cols = ( 'site', 'fac' );
    my $retval = $db->query_hasharray(trim("

    SELECT DISTINCT site_name, fac_name
     FROM processes
     NATURAL JOIN process_types
     NATURAL JOIN family_processes
     NATURAL JOIN process_families
     JOIN         facilities USING (fac_id)
     NATURAL JOIN sites
     WHERE proc_type_name = ? AND proc_name = ?
     ORDER BY site_name, fac_name

    "), \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, "Failed to get list of process locations: $ptype-$pname", $db->error());
        return undef;
    }

    return $retval;
}

sub get_processes_by_input_dsclass {
    my ($db, $class, $level) = @_;

    my @args = ( $class, $level );
    my @cols = ( 'type', 'name' );
    my $retval = $db->query_hasharray(trim("

    SELECT DISTINCT proc_type_name, proc_name
     FROM processes
     NATURAL JOIN process_types
     NATURAL JOIN process_input_ds_classes
     NATURAL JOIN datastream_classes
     WHERE ds_class_name = ? AND ds_class_level = ?

    "), \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, "Failed to get list of processes that use: $class.$level", $db->error());
        return undef;
    }

    my %list = ( );
    my $list = \%list;
    for (@{$retval}) {
        my $key = $_->{'type'} . '-' . $_->{'name'};
        unless (defined($list->{ $key })) {
            my %r = ( 'type' => $_->{'type'}, 'name' => $_->{'name'}, 'vars' => { } );
            $list->{ $key } = \%r;
        }
    }

    @args = ( $class, $level );
    @cols = ( 'ptype', 'pname', 'vname', 'reqd', 'priority' );
    $retval = $db->query_hasharray(trim("

    SELECT DISTINCT proc_type_name, proc_name, var_name, ret_req_to_run_flg, ret_ds_subgroup_priority
     FROM         ret_var_names
     NATURAL JOIN process_types
     NATURAL JOIN processes
     NATURAL JOIN process_input_ds_classes
     NATURAL JOIN datastream_classes
     NATURAL JOIN ret_ds_subgroups
     NATURAL JOIN ret_datastreams
     NATURAL JOIN ret_ds_groups
     NATURAL JOIN ret_var_groups
     NATURAL JOIN var_names
     WHERE ds_class_name = ? AND ds_class_level = ?
     ORDER BY proc_type_name, proc_name, var_name, ret_ds_subgroup_priority DESC

    "), \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, "Failed to get list of processes that use (via retrieval): $class.$level", $db->error());
        return undef;
    }

    for (@{$retval}) {
        my $key = $_->{'ptype'} . '-' . $_->{'pname'};
        my $var = $_->{'vname'};
        unless (defined($list->{ $key })) {
            my %r = ( 'type' => $_->{'type'}, 'name' => $_->{'name'}, 'vars' => { } );
            $list->{ $key } = \%r;
        }
        unless (defined($list->{ $key }->{'vars'}->{ "$class.$level : $var" })) {
            my %v = ( 'reqd' => int($_->{'reqd'}), 'priority' => $_->{'priority'} );
            $list->{ $key }->{'vars'}->{ "$class.$level : $var" } = \%v;
        }
    }

    my @list = values(%list);
    return \@list;
}

sub get_processes_by_output_dsclass {
    my ($db, $class, $level) = @_;

    my @args = ( $class, $level );
    my @cols = ( 'type', 'name' );
    my $retval = $db->query_hasharray(trim("

    SELECT DISTINCT proc_type_name, proc_name
     FROM processes
     NATURAL JOIN process_types
     NATURAL JOIN process_output_ds_classes
     NATURAL JOIN datastream_classes
     WHERE ds_class_name = ? AND ds_class_level = ?

    "), \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, "Failed to get list of processes that produce: $class.$level", $db->error());
        return undef;
    }

    return $retval;
}

my %_in_history = ( );
my $_in_history = \%_in_history;

sub get_recursive_inputs {
    my ($db, $item, $pt_ignore, $dl_ignore) = @_;

    my @producers = ( );
    my $root;
    my $branch = $item->{'children'};

    if ($item->{'name'} && $item->{'type'}) {
        @producers = ( $item );
        $root = $item;

    } else {
        my $class = $item->{'class'};
        my $level = $item->{'level'};

        $_in_history->{ "$class.$level" } = $branch;

        my $producers = get_processes_by_output_dsclass($db, $class, $level);
        return(undef) unless (defined($producers));

        @producers = @{$producers};
    }

    for my $p (@producers) {
        next if (defined($pt_ignore->{ $p->{'type'} }));

        my %r = (
            'uid'      => $p->{'name'} . ':' . $p->{'type'},
            'name'     => $p->{'name'},
            'type'     => $p->{'type'},
            'position' => 'left',
            'children' => [ ],
        );
        my $r = ($root) ? $root : \%r;
        push(@{ $branch }, $r) unless ($root);

        my $inputs = get_process_inputs($db, $p->{'type'}, $p->{'name'});
        return(undef) unless (defined($inputs));

        for (@{$inputs}) {
            next if (defined($dl_ignore->{ $_->{'level'} }));

            my ( $class, $level ) = ( $_->{'class'}, $_->{'level'} );
            my %leaf = (
                'uid'      => "$class.$level",
                'class'    => $class,
                'level'    => $level,
                'names'    => $_->{'names'},
                'sites'    => $_->{'sites'},
                'position' => 'left',
                'children' => [ ],
            );
            my $l = \%leaf;
            push(@{ $r->{'children'} }, $l);
            unless (defined($_in_history->{ "$class.$level" })) {
                get_recursive_inputs($db, $l, $pt_ignore, $dl_ignore);
            }
        }
    }
}

my %_out_history = ( );
my $_out_history = \%_out_history;

sub get_recursive_outputs {
    my ($db, $item) = @_;

    my @consumers = ( );
    my $root;
    my $branch = $item->{'children'};

    if ($item->{'name'} && $item->{'type'}) {
        @consumers = ( $item );
        $root = $item;

    } else {
        my $class = $item->{'class'};
        my $level = $item->{'level'};

        $_out_history->{ "$class.$level" } = $branch;

        my $consumers = get_processes_by_input_dsclass($db, $class, $level);
        return(undef) unless (defined($consumers));

        @consumers = @{$consumers};
    }

    for my $c (@consumers) {
        my %r = (
            'uid'      => $c->{'name'} . ':' . $c->{'type'},
            'name'     => $c->{'name'},
            'type'     => $c->{'type'},
            'vars'     => $c->{'vars'},
            'position' => 'right',
            'children' => [ ],
        );
        my $r = ($root) ? $root : \%r;
        push(@{ $branch }, $r) unless ($root);

        my $outputs = get_process_outputs($db, $c->{'type'}, $c->{'name'});
        return(undef) unless (defined($outputs));

        for (@{$outputs}) {
            my ( $class, $level ) = ( $_->{'class'}, $_->{'level'} );
            my %leaf = (
                'uid'      => "$class.$level",
                'class'    => $class,
                'level'    => $level,
                'position' => 'right',
                'children' => [ ],
            );
            my $l = \%leaf;
            push(@{ $r->{'children'} }, $l);
            unless (defined($_out_history->{ "$class.$level" })) {
                get_recursive_outputs($db, $l);
            }
        }
    }
}

sub get_deps_tree {
    my ($db, $item, $level) = @_;

    my %tree;
    if ($item =~ m/^([^:]+):(.+)$/) {
        my ($proc, $type) = ( $1, $2 );
        %tree = (
            'uid'   => $item,
            'name'  => $proc,
            'type'  => $type,
            'vars'  => { },
        );

    } elsif ($item =~ m/^([^\.]+)\.(.+)$/) {
        my ($class, $level) = ( $1, $2 );
        %tree = (
            'uid'   => $item,
            'class' => $class,
            'level' => $level,
        );

    } else {
        return undef;
    }

    my %pt_ignore = ( );
    my %dl_ignore = ( );

    if ((defined($tree{'type'}) && $tree{'type'} eq 'VAP') || (defined($tree{'level'}) && $tree{'level'} =~ /^c/)) {
        %pt_ignore = ( 'Collection' => 1, 'Rename' => 1, 'Bundle' => 1 );
        %dl_ignore = ( '00' => 1 );
    }

    my @c = ( );

    $tree{'position'} = 'center';
    $tree{'children'} = \@c;

    get_recursive_inputs(  $db, \%tree, \%pt_ignore, \%dl_ignore );
    get_recursive_outputs( $db, \%tree );

    return \%tree;
}

1;
