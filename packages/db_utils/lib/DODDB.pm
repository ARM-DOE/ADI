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

package DODDB;

use strict;
use warnings;

use JSON::XS;

use DBCORE;

my $_Version = '@VERSION@';

my $Error = '';

################################################################################

sub _set_error {
    my ($line, @err) = @_;

    $Error = ":$line >> " . join("\n", grep { defined($_) } @err ) . "\n\n";
}

sub get_error {
    return $Error;
}

sub _trim {
    my ($str) = @_;
    
    $str =~ s/^\s*//;
    $str =~ s/\s*$//;
    
    return $str;
}

sub date_to_seconds {
    my ($Y,$M,$D,$h,$m,$s) = @_;
    
    $Y = (defined($Y)) ? $Y : 0;
    $M = (defined($M)) ? int($M)-1 : 0;
    $D = (defined($D)) ? $D : 0;
    $h = (defined($h)) ? $h : 0;
    $m = (defined($m)) ? $m : 0;
    $s = (defined($s)) ? $s : 0;
    
    my $seconds = Time::Local::timegm_nocheck($s,$m,$h,$D,$M,$Y);
    
    if ($seconds >= 2147483648 || $seconds < 0) {
        return 0;
    }
    
    return $seconds;
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
    
    return($db);
}


################################################################################

sub get_ds_class_id {
    my ($db, $ds_class, $ds_level) = @_;
    
	my @args = ($ds_class, $ds_level);
    return $db->query_scalar('SELECT * FROM get_datastream_class_id(?,?,false)', \@args);
}

sub get_ds_list {
    my ($db, $with_bods) = @_;
    
    my $retval;
    my @args = ( );
    my @cols = ( );
    
    if ($with_bods) {
        @cols = ( 'name', 'level', 'bods', 'pmts' );
        $retval = $db->query_hasharray(_trim("

        SELECT ds_class_name, ds_class_level, i.ds_class_id, ii.ds_class_id
        FROM datastream_classes dc
        LEFT OUTER JOIN ds_class_info i ON dc.ds_class_id = i.ds_class_id
        LEFT OUTER JOIN ds_class_info ii ON ( dc.ds_class_id = ii.ds_class_id AND ii.ds_class_info NOT LIKE '%\"pm\":[]%' )

        "), \@args, \@cols);
        unless (defined($retval)) {
            _set_error(__LINE__, 'Failed to get list of datastream classes.', $db->error());
            return undef;
        }
        
    } else {
        @cols = ( 'name', 'level' );
        $retval = $db->query_hasharray(_trim("

        SELECT ds_class_name, ds_class_level
        FROM datastream_classes dc

        "), \@args, \@cols);
        unless (defined($retval)) {
            _set_error(__LINE__, 'Failed to get list of datastream classes.', $db->error());
            return undef;
        }
    }
    
    return $retval;
}

sub save_ds_class {
    my ($db, $ds_class, $ds_level, $desc) = @_;
    
    my $retval = $db->sp_call('datastream_classes', 'define', $ds_class, $ds_level, $desc);
	unless (defined($retval)) {
		_set_error(__LINE__, $db->error());
		return 0;
	}
	
	return $retval;
}

sub delete_ds_class {
    my ($db, $ds_class, $ds_level) = @_;
    
    my $retval = $db->sp_call('datastream_classes', 'delete', $ds_class, $ds_level);
	unless (defined($retval)) {
		_set_error(__LINE__, $db->error());
		return 0;
	}
	
	return 1;
}

sub get_ds_class_info {
    my ($db, $ds_class, $ds_level) = @_;
    
    my $class_id = (defined($ds_level)) ? get_ds_class_id($db, $ds_class, $ds_level) : $ds_class;
    
    my @args = ( $class_id );
    my @cols = ( 'info' );
    my $stmt = _trim("
        SELECT ds_class_info
        FROM ds_class_info
        WHERE ds_class_id = ?
    ");
    my $retval = $db->query_hasharray($stmt, \@args, \@cols);
    return undef unless (defined($retval));
    return '' unless (defined($retval->[0]));
    return $retval->[0]->{'info'};
}

sub save_ds_class_info {
    my ($db, $class_id, $class_info) = @_;
    
    my $info = get_ds_class_info($db, $class_id);
    return(0) unless defined($info);
    
    my $dinfo = json_decode($class_info);
    return(0) unless defined($dinfo);
    
    my $iclass = $dinfo->{'iclass'};
    my $icdesc = $dinfo->{'icdesc'};
    my $ic_id;
    if (defined($iclass)) {
        $ic_id = get_instrument_class_id($db, $iclass);
        if (!defined($ic_id) && defined($icdesc)) {
            if (define_instrument_class($db, $iclass, $icdesc)) {
                $ic_id = get_instrument_class_id($db, $iclass);
            }
        }
    }
    
    $ic_id = (defined($ic_id)) ? "$ic_id" : 'NULL';
    
    $class_info = $db->{'DBH'}->{'DBH'}->quote($class_info);
    
    my $stmt;
    my $retval;
    if ($info eq '') {
        $stmt = _trim("
        
        INSERT INTO ds_class_info
        (ds_class_id, ds_class_info, inst_class_id)
        VALUES(%s,%s,%s)
        
        ");
        $retval = $db->do(sprintf($stmt, ($class_id, $class_info, $ic_id)));
        
    } else {
        $stmt = _trim("

        UPDATE ds_class_info
        SET ds_class_info = %s, inst_class_id = %s
        WHERE ds_class_id = %s

        ");
        $retval = $db->do(sprintf($stmt, ($class_info, $ic_id, $class_id)));
    }
    
    return(0) unless (defined($retval));
    
    return 1;
}

sub delete_ds_class_info {
    my ($db, $class, $level) = @_;
    
    my $class_id = get_ds_class_id($db, $class, $level);
    return(0) unless ($class_id);
    
    my $stmt = _trim("
    
    DELETE FROM ds_class_info
    WHERE ds_class_id = %d
    
    ");
    my $retval = $db->do(sprintf($stmt, ($class_id)));
    unless (defined($stmt)) {
        _set_error(__LINE__, $db->error());
        return 0;
    }
    
    return 1;
}

sub get_ds_classes_with_info {
    my ($db) = @_;
    
    my @args = ( );
    my @cols = ( 'class', 'level' );
    my $stmt = _trim("
    
    SELECT ds_class_name, ds_class_level
    FROM datastream_classes
    NATURAL JOIN ds_class_info
    
    ");

    return $db->query_hasharray($stmt, \@args, \@cols);
}

sub get_instrument_classes {
    my ($db) = @_;
    
    my @args = ( );
    my @cols = ( 'name', 'desc' );
    my $stmt = _trim("
    
        SELECT inst_class, inst_class_desc
        FROM instrument_classes
        
    ");
    return $db->query_hasharray($stmt, \@args, \@cols);
}

sub get_unused_instrument_classes {
    my ($db) = @_;
    
    my @args = ( );
    my @cols = ( 'name' );
    my $stmt = _trim("
    
        SELECT ic.inst_class
        FROM instrument_classes ic
        LEFT OUTER JOIN ds_class_info ds ON ic.inst_class_id = ds.inst_class_id
    
    ");
    return $db->query_hasharray($stmt, \@args, \@cols);
}

sub get_instrument_class_id {
    my ($db, $name) = @_;
    
    my @args = ( $name );
    my $stmt = _trim("
    
        SELECT inst_class_id FROM instrument_classes
        WHERE inst_class = ?
        
    ");
    my $retval = $db->query_scalar($stmt, \@args);
    unless (defined($retval)) {
        _set_error(__LINE__, "Error looking up instrument class: $name", $db->error());
        return undef;
    }
    
    return $retval;
}

sub define_instrument_class {
    my ($db, $name, $desc) = @_;
    
    my $esc_name = $db->{'DBH'}->{'DBH'}->quote($name);
    my $esc_desc = $db->{'DBH'}->{'DBH'}->quote($desc);
    
    my @args = ( $name );
    my @cols = ( 'desc' );
    my $stmt = _trim("
        SELECT inst_class_desc FROM instrument_classes WHERE inst_class = ?
    ");

    my $ret = $db->query_hasharray($stmt, \@args, \@cols);
    if (defined($ret) && defined($ret->[0])) {
        return(0) if ($ret->[0]->{'desc'} eq $desc);

        $stmt = _trim("
        
        UPDATE instrument_classes
        SET inst_class_desc = %s
        WHERE inst_class = %s
        
        ");
        $ret = $db->do(sprintf($stmt, ($esc_desc, $esc_name)));
        return(-1) if (defined($ret));
        
    } else {
        $stmt = _trim("
        
        INSERT INTO instrument_classes
        (inst_class, inst_class_desc)
        VALUES( %s, %s )
        
        ");
        $ret = $db->do(sprintf($stmt, ($esc_name, $esc_desc)));
        return(1) if (defined($ret));
    }
    
    _set_error(__LINE__, "Error defining instrument class: $name ($desc)", $db->error());
    return undef;
}

sub delete_instrument_class {
    my ($db, $name) = @_;

    my $stmt = _trim("
    
    DELETE FROM instrument_classes
    WHERE inst_class = '%s'
    
    ");
    my $retval = $db->do(sprintf($stmt, ($name)));
    unless (defined($retval)) {
        _set_error(__LINE__, "Error deleting instrument class: $name", $db->error());
        return 0;
    }
    
    return 1;
}

################################################################################

sub get_dsclass_fields {
    my ($db, $class, $level) = @_;
    
    my @args = ( $class, $level );
    my @cols = ( 'name', 'type', 'dims' );
    my $retval = $db->query_hasharray(_trim("
    
    SELECT DISTINCT
     vn.var_name AS vname,
     dt.data_type AS vtype,
     ARRAY_TO_STRING( ARRAY(
       SELECT dn.dim_name
       FROM dod_var_dims dvd
        NATURAL JOIN dod_dims dd
        NATURAL JOIN dimensions d
        NATURAL JOIN dim_names dn
       WHERE dvar.var_id = dvd.var_id
        AND dv.dod_id = dvd.dod_id
       ORDER BY dvd.var_dim_order
     ),',' ) as vdims

    FROM variables v
     NATURAL JOIN data_types dt
     NATURAL JOIN var_names vn
     NATURAL JOIN dod_vars dvar
     NATURAL JOIN dod_versions dv

    WHERE dv.dod_id IN (
     SELECT DISTINCT(dod_id)
     FROM dod_versions
     NATURAL JOIN datastream_classes
     WHERE ds_class_name = ?
     AND ds_class_level = ?
    )
    
    "), \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, $db->error());
        return undef;
    }
    
    return $retval;
}

sub get_data_fields {
    my ($db) = @_;
    
    my @args = ( );
    my @cols = ( 'class', 'level', 'name', 'type', 'dims' );
    my $list_vars = $db->query_hasharray(_trim("
    
    SELECT DISTINCT
     dc.ds_class_name as class,
     dc.ds_class_level as level,
     vn.var_name AS vname,
     dt.data_type AS vtype,
     ARRAY_TO_STRING( ARRAY(
       SELECT dn.dim_name
       FROM dod_var_dims dvd
        NATURAL JOIN dod_dims dd
        NATURAL JOIN dimensions d
        NATURAL JOIN dim_names dn
       WHERE dvar.var_id = dvd.var_id
        AND dv.dod_id = dvd.dod_id
       ORDER BY dvd.var_dim_order
     ),',' ) as vdims

    FROM variables v
     NATURAL JOIN data_types dt
     NATURAL JOIN var_names vn
     NATURAL JOIN dod_vars dvar
     NATURAL JOIN dod_versions dv
     NATURAL JOIN datastream_classes dc
    
    "), \@args, \@cols);
    unless (defined($list_vars)) {
        _set_error(__LINE__, $db->error());
        return undef;
    }
    
    my $list_desc = get_fields_att($db, 'long_name');
    return(undef) unless (defined($list_desc));
    
    my %map_desc = ( );
    for (@{ $list_desc }) {
        my $key = join('.', $_->{'class'}, $_->{'level'}, $_->{'var'});
        $map_desc{ $key } = $_->{'att'};
    }
    
    my $list_units = get_fields_att($db, 'units');
    return(undef) unless (defined($list_units));
    
    my %map_units = ( );
    for (@{ $list_units }) {
        my $key = join('.', $_->{'class'}, $_->{'level'}, $_->{'var'});
        $map_units{ $key } = $_->{'att'};
    }
    
    my @data = ( );
    for (@{ $list_vars }) {
        next unless (defined($_->{'dims'}));
        next if ($_->{'name'} eq $_->{'dims'});
        next if ($_->{'name'} eq 'time_offset');
        next if ($_->{'name'} eq 'base_time');
        next if ($_->{'name'} eq 'lat');
        next if ($_->{'name'} eq 'lon');
        next if ($_->{'name'} eq 'alt');
        next if ($_->{'name'} =~ /^.?qc_/);
        
        my $key = join('.', $_->{'class'}, $_->{'level'}, $_->{'name'});
        my %r = (
            'class' => $_->{'class'},
            'level' => $_->{'level'},
            'vname' => $_->{'name'},
            'dtype' => $_->{'type'},
            'dims'  => $_->{'dims'},
            'desc'  => (defined($map_desc{ $key }))  ? $map_desc{ $key }  : '',
            'units' => (defined($map_units{ $key })) ? $map_units{ $key } : '',
        );
        push(@data, \%r);
    }
    
    return \@data;
}

sub get_fields_att {
    my ($db, $aname) = @_;
    
    my @args = ( $aname );
    my @cols = ( 'class', 'level', 'var', 'att' );
    my $retval = $db->query_hasharray(_trim("
    
    SELECT DISTINCT
     ds_class_name,
     ds_class_level,
     var_name,
     att_value

    FROM          datastream_classes
     NATURAL JOIN dod_versions
     NATURAL JOIN dod_vars
     NATURAL JOIN
         (SELECT var_id,var_name
          FROM         variables
          NATURAL JOIN var_names) AS var
     NATURAL JOIN dod_var_atts
     NATURAL JOIN attributes
     NATURAL JOIN att_names
     NATURAL JOIN data_types
    
    WHERE att_name = ?
    
    "), \@args, \@cols);
    unless (defined($retval)) {
        _set_error(__LINE__, $db->error());
        return undef;
    }
    
    return $retval;
}

sub get_dod_list {
	my ($db, $class, $level, $db_prod) = @_;
	
	$class = (defined($class)) ? $class : '%';
	$level = (defined($level)) ? $level : '%';
	
	my $retval = $db->sp_call('dod_versions', 'inquire', $class, $level, '%');
	unless (defined($retval)) {
		_set_error(__LINE__, $db->error());
		return undef;
	}
	
	my @dods = ( );
	my %dods_map = ( );
	my $dods_map = \%dods_map;
	for (@{$retval}) {
	    my $key = $_->{'dsc_name'} . $_->{'dsc_level'} . $_->{'dod_version'};
	    my %dod = (
	        'class'     => $_->{'dsc_name'},
	        'level'     => $_->{'dsc_level'},
	        'version'   => $_->{'dod_version'},
	        'runs'      => [ ],
	        'prod'      => 0,
	    );
	    push(@dods, \%dod);
	    $dods_map->{ $key } = \%dod;
	}
	
	$retval = $db->sp_call('ds_dods', 'inquire', '%', '%', $class, $level, '%');
	unless (defined($retval)) {
	    _set_error(__LINE__, $db->error());
	    return undef;
	}
	
	for my $r (@{$retval}) {
	    my $key = $r->{'dsc_name'} . $r->{'dsc_level'} . $r->{'dod_version'};
	    if (defined($dods_map->{ $key })) {
	        my %h = (
                'site' => $r->{'site'},
                'fac'  => $r->{'facility'},
                'time' => int(date_to_seconds($r->{'dod_time'} =~ /^(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2})/)),
            );
	        push(@{$dods_map->{ $key }->{'runs'}}, \%h);
	    }
	}
	
	if (defined($db_prod)) {
	    my $retval_prod = get_dod_list($db_prod, $class, $level);
	    unless (defined($retval_prod)) {
	        _set_error(__LINE__, $db->error());
	        return undef;
	    }
	    
	    for my $r (@{$retval_prod}) {
	        my $key = $r->{'class'} . $r->{'level'} . $r->{'version'};
	        if (defined($dods_map->{ $key })) {
	            $dods_map->{ $key }->{'prod'} = 1;
	        } else {
	            $r->{'prod'} = 1;
	            push(@dods, $r);
	        }
	    }
	}
	
	return \@dods;
}

sub dod_exists {
	my ($db, $ds_class, $proc_level, $version) = @_;
	
	my $retval = $db->sp_call('dod_versions', 'inquire', $ds_class, $proc_level, $version);
	if (defined($retval) && defined($retval->[0])) { return 1 }
	
	return 0;
}

sub save_dod {
    my ($db, $class, $level, $version, $dod, $user, $changes, $props_only) = @_;
    my $retval;
    
    if (dod_exists($db, $class, $level, $version)) {
        
	    $retval = $db->sp_call('dodvar_property_values', 'delete_by_dod', $class, $level, $version);
	    unless (defined($retval)) {
	        _set_error(__LINE__, 'Failed to remove existing DOD variable property keys');
	        return 0;
	    }
	    
	    unless ($props_only) {
		    $retval = $db->sp_call('dod_versions', 'delete_definition', $class, $level, $version);
		    unless (defined($retval) && $retval) {
			    _set_error(__LINE__, 'Failed to overwrite existing DOD version');
			    return 0;
		    }
	    }
		
	} else {
	    
	    unless ($props_only) {
		    $retval = $db->sp_call('dod_versions', 'define', $class, $level, $version);
		    unless (defined($retval) && $retval) {
			    _set_error(__LINE__, 'Failed to define DOD version');
			    return 0;
		    }
	    }
	}

	my $dim_id = 0;
    for my $dim (@{$dod->{'dims'}}) {
        last if ($props_only);
        $retval = $db->sp_call('dod_dims', 'define',
            $class, $level, $version,
            $dim->{'name'}, $dim->{'length'}, $dim_id
        );
        unless (defined($retval)) {
            _set_error(__LINE__, "Failed to define dimension: " . $dim->{'name'});
            return 0;
        }
		$dim_id++;
    }

	my $att_id = 0;
    for my $att (@{$dod->{'atts'}}) {
        last if ($props_only);
        $retval = $db->sp_call('dod_atts', 'define',
            $class, $level, $version,
            $att->{'name'}, $att->{'type'}, $att->{'value'}, $att_id
        );
        unless (defined($retval)) {
            _set_error(__LINE__, "Failed to define attribute: " . $att->{'name'});
            return 0;
        }
		$att_id++;
    }

	my $var_id = 0;
    for my $var (@{$dod->{'vars'}}) {
        
        unless ($props_only) {
            $retval = $db->sp_call('dod_vars', 'define',
                $class, $level, $version,
                $var->{'name'}, $var->{'type'}, $var_id
            );
            unless (defined($retval)) {
                _set_error(__LINE__, "Failed to define variable: " . $var->{'name'});
                return 0;
            }
        
            my $vdim_id = 0;
            for my $vdim (@{$var->{'dims'}}) {
                $retval = $db->sp_call('dod_var_dims', 'define',
                    $class, $level, $version,
                    $var->{'name'}, $vdim, $vdim_id
                );
                unless (defined($retval)) {
                    _set_error(__LINE__, "Failed to define var dim: " . $var->{'name'} . ":$vdim");
                    return 0;
                }
                $vdim_id++;
            }

    		my $vatt_id = 0;
            for my $vatt (@{$var->{'atts'}}) {
                $retval = $db->sp_call('dod_var_atts', 'define',
                    $class, $level, $version,
                    $var->{'name'}, $vatt->{'name'}, $vatt->{'type'}, $vatt->{'value'}, $vatt_id
                );
                unless (defined($retval)) {
                    _set_error(__LINE__, "Failed to define var att: " . $var->{'name'} . ":" . $vatt->{'name'});
                    return 0;
                }
    			$vatt_id++;
            }
        }
        
        if (defined($var->{'props'})) {
            for my $key (keys(%{$var->{'props'}})) {
                $retval = $db->sp_call('dodvar_property_values', 'define',
                    $class, $level, $version, $var->{'name'}, $key, $var->{'props'}->{$key}
                );
                unless (defined($retval)) {
                    _set_error(__LINE__, "Failed to define var property: " . $var->{'name'} . ":$key");
                    return 0;
                }
            }
        }
        
		$var_id++;
    }

    if (defined($user) && !$props_only) {
        
        $retval = $db->sp_call('dod_revisions', 'define',
    		$class, $level, $version, $user, $changes
    	);
        unless (defined($retval)) {
            _set_error(__LINE__, 'Failed to save DOD revision');
            return 0;
        }
    }
    
    return 1;
}

sub delete_dod {
    my ($dbh, $class, $level, $version) = @_;
    my $retval;
    
    $retval = $dbh->sp_call('dodvar_property_values', 'delete_by_dod', $class, $level, $version);
	unless (defined($retval)) {
	    _set_error(__LINE__, $dbh->error());
		return 0;
	}
	
	$retval = $dbh->sp_call('dod_revisions', 'delete', $class, $level, $version);
	unless (defined($retval)) {
		_set_error(__LINE__, $dbh->error());
		return 0;
	}
	
	$retval = $dbh->sp_call('dod_versions', 'delete', $class, $level, $version);
	unless (defined($retval)) {
		_set_error(__LINE__, $dbh->error());
		return 0;
	}
	
	return 1;
}

sub get_dod {
    my ($dbh, $class, $level, $version, $loadtime, $isdev) = @_;
    
    my ($retval, @args, @cols);

    if ($isdev && defined($loadtime) && $loadtime > 0) {
	
    	@args = ( $class, $level, $version, $loadtime );
    	@cols = ( 'rev', 'date' );
    	$retval = $dbh->query_hasharray(_trim("

    	SELECT rev_num, rev_date

    	FROM dod_versions
    	NATURAL JOIN datastream_classes
    	NATURAL JOIN dod_revisions

    	WHERE ds_class_name = ?
    	AND ds_class_level = ?
    	AND dod_version = ?
    	AND extract(epoch from rev_date) > ?

    	"), \@args, \@cols);

    	unless (defined($retval)) {
    	    _set_error(__LINE__, $dbh->error());
    	    return undef;
    	}
	
    	unless (defined($retval->[0])) {
    		return undef;
    	}
    }

    my $dod = get_dod_version($dbh, $class, $level, $version);
    unless (defined($dod)) {
        _set_error(__LINE__, $dbh->error());
        return undef;
    }
    
    $retval = $dbh->sp_call('ds_dods', 'inquire', '%', '%', $class, $level, $version);
    unless (defined($retval)) {
        _set_error(__LINE__, $dbh->error());
        return undef;
    }
    
    my @runs = ( );
    for (@{$retval}) {
        my $s = $_->{'site'};
        my $f = $_->{'facility'};
        my $t = int(date_to_seconds($_->{'dod_time'} =~ /^(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2})/));
        push(@runs, { 'site' => $s, 'fac' => $f, 'time' => $t });
    }
    
    $dod->{'runs'} = \@runs;

    if ($isdev) {

    	@args = ( $class, $level, $version );
    	@cols = ( 'num', 'user', 'date', 'changes' );
    	$retval = $dbh->query_hasharray(_trim("

    	SELECT
    	rev_num,
    	rev_user,
    	rev_date,
    	rev_changes

    	FROM dod_revisions
    	NATURAL JOIN dod_versions
    	NATURAL JOIN datastream_classes

    	WHERE ds_class_name = ?
    	AND ds_class_level = ?
    	AND dod_version = ?

    	ORDER BY rev_num

    	"), \@args, \@cols);

    	unless (defined($retval)) {
    		_set_error(__LINE__, $dbh->error());
    		return undef;
    	}

    	my @revs = ();
    	for (@{$retval}) {
    		my %row = (
    			'num'  => int($_->{'num'}),
    			'user' => $_->{'user'},
    			'date' => int(date_to_seconds($_->{'date'} =~ /^(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2})/)),
    			'meta' => json_decode($_->{'changes'}),
    		);
    		push(@revs, \%row);
    	}

    	$dod->{'revs'} = \@revs;
    }
    
    return $dod;
}

sub get_dod_version {
    my ($db, $ds_class, $proc_level, $version) = @_;
    
    return (undef) unless (dod_exists($db, $ds_class, $proc_level, $version));

	my $dims = _get_dod_dims($db, $ds_class, $proc_level, $version);
	return(undef) unless defined($dims);
	
	my $atts = _get_dod_atts($db, $ds_class, $proc_level, $version);
	return(undef) unless defined($atts);
	
	my $vars = _get_dod_vars($db, $ds_class, $proc_level, $version);
	return(undef) unless defined($vars);
    
    my %dod = (
        'dims' => $dims,
        'atts' => $atts,
        'vars' => $vars,
    );
    
    return \%dod;
}

sub _get_dod_dims {
    my ($db, $ds_class, $proc_level, $version) = @_;
    
    my $retval = $db->sp_call('dod_dims', 'get', $ds_class, $proc_level, $version);
    unless (defined($retval)) {
        _set_error(__LINE__, "Failed to get DOD dimensions: $ds_class.$proc_level, v$version", $db->error());
		return undef;
    }

	@{$retval} = sort { int($a->{'dim_order'}) <=> int($b->{'dim_order'}) } @{$retval};
    
	my @dims = ();
    for (@{$retval}) {
        my %dim = (
            'name'   => $_->{'dim_name'},
            'length' => $_->{'dim_length'},
        );
        push(@dims, \%dim);
    }
    
    return \@dims;
}

sub _get_dod_atts {
    my ($db, $ds_class, $proc_level, $version) = @_;
    
    my $retval = $db->sp_call('dod_atts', 'get', $ds_class, $proc_level, $version);
    unless (defined($retval)) {
        _set_error(__LINE__, "Failed to get DOD attributes: $ds_class.$proc_level, v$version", $db->error());
		return undef;
    }

	@{$retval} = sort { int($a->{'att_order'}) <=> int($b->{'att_order'}) } @{$retval};
    
	my @atts = ();
    for (@{$retval}) {
        my %att = (
            'name'  => $_->{'att_name'},
            'type'  => $_->{'att_type'},
            'value' => $_->{'att_value'},
        );
        push(@atts, \%att);
    }
    
    return \@atts;
}

sub _get_dod_vars {
    my ($db, $ds_class, $proc_level, $version) = @_;

    my $retval;
    
    $retval = $db->sp_call('dod_vars', 'get', $ds_class, $proc_level, $version);
    unless (defined($retval)) {
        _set_error(__LINE__, "Failed to get DOD variables: $ds_class.$proc_level, v$version", $db->error());
		return undef;
    }

	@{$retval} = sort { int($a->{'var_order'}) <=> int($b->{'var_order'}) } @{$retval};
    
    my %vars = ();
    my $vars = \%vars;

	my @vars = ();
    
    for (@{$retval}) {
        my @vdims = ();
        my @vatts = ();
        my %var = (
            'name' => $_->{'var_name'},
            'type' => $_->{'var_type'},
            'dims' => \@vdims,
            'atts' => \@vatts,
        );
        $vars->{$_->{'var_name'}} = \%var;
		push(@vars, \%var);
    }
    
    $retval = $db->sp_call('dod_var_dims', 'get', $ds_class, $proc_level, $version, '%');
    unless (defined($retval)) {
        _set_error(__LINE__, "Failed to get DOD var dims: $ds_class.$proc_level, v$version", $db->error());
		return undef;
    }
    
    @{$retval} = sort { int($a->{'var_dim_order'}) <=> int($b->{'var_dim_order'}) } @{$retval};
    
    for (@{$retval}) {
        push(@{$vars->{$_->{'var_name'}}->{'dims'}}, $_->{'dim_name'});
    }
    
    $retval = $db->sp_call('dod_var_atts', 'get', $ds_class, $proc_level, $version, '%');
    unless (defined($retval)) {
        _set_error(__LINE__, "Failed to get DOD var atts: $ds_class.$proc_level, v$version", $db->error());
		return undef;
    }

	@{$retval} = sort { int($a->{'var_att_order'}) <=> int($b->{'var_att_order'}) } @{$retval};
    
    for (@{$retval}) {
        my %att = (
            'name'  => $_->{'att_name'},
            'type'  => $_->{'att_type'},
            'value' => $_->{'att_value'},
        );
        push(@{$vars->{$_->{'var_name'}}->{'atts'}}, \%att);
    }
    
    my $props = _get_dod_var_props($db, $ds_class, $proc_level, $version);
    return undef unless (defined($props));
    
    for (@vars) {
        if (defined($props->{ $_->{'name'} })) {
            $_->{ 'props' } = $props->{ $_->{'name'} };
        }
    }
    
    return \@vars;
}

sub _get_dod_var_props {
    my ($db, $ds_class, $proc_level, $version) = @_;
    
    my $retval;
    
    $retval = $db->sp_call('dodvar_property_values', 'get_by_dod', $ds_class, $proc_level, $version);
    unless (defined($retval)) {
        _set_error(__LINE__, "Failed to get DOD variable properties: $ds_class.$proc_level, v$version", $db->error());
        return undef;
    }
    
    my %map = ( );
    my $map = \%map;
    
    for (@{$retval}) {
        unless (defined($map->{ $_->{'var_name'} })) {
            my %props = ( );
            $map->{ $_->{'var_name'} } = \%props;
        }
        $map->{ $_->{'var_name'} }->{ $_->{'prop_key'} } = $_->{'prop_value'};
    }
    
    return $map;
}

1;
