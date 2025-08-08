#!/usr/bin/env perl
################################################################################
#
#  COPYRIGHT (C) 2006 Battelle Memorial Institute.  All Rights Reserved.
#
################################################################################
#
#  Author:
#     name:  Sherman Beus
#     phone: (509) 375-2662
#     email: sherman.beus@pnl.gov
#
################################################################################

use strict;

use Cwd qw(abs_path);

use Data::Dumper;
use Getopt::Long;
Getopt::Long::Configure qw(no_ignore_case);
use JSON::XS;

use PCMDB;
use DODDB;
#use HistoryDB;

my $DB;


################################################################################

sub cleanup {
    if ($DB) {
		$DB->disconnect();
	}
	#HistoryDB::disconnect();
}

sub exit_success {
    cleanup();
    exit(0);
}

sub exit_failure {
    cleanup();
    exit(1);
}

sub exit_usage {
    my ($program) = shift;
    print STDOUT <<EOF;

USAGE:

$program [-h|v] [-a alias] [-replaces <process>-<type>] [-ret <process>-<type>] proc-file

    [-h|help]          => Display this usage message.
    [-v]               => Show version.
    
    [-a alias]         => DB alias from user .db_connect file.
                          [ default: dsdb_data ]
    
    -fmt <json|perl>   => Format of encoded process.  Defaults to "perl".
    
    -replaces          => Merge import with an existing process.
    
    -clean             => If specified, any existing "family processes"
                          that are not part of the imported configuration
                          (i.e. locations) will be removed.  The default
                          is to leave them in place.
    
    -ret               => Imports custom retrieval file for named
                          process.
    
    -prop-mail         => Also add warning and/or error mail properties
                          if they exist. The default is to exclude these 
                          so production runs do not fill unsuspecting
                          inboxes.

    -prop-mrt          => Include max_run_time property.  Excluded by default
                          so as to not interfere with production defaults.

      EXAMPLES:
        > $program new.process
        > $program -fmt json new.json
        > $program -replaces old-vap new.process
        > $program -ret name-vap ret.txt

EOF
    exit(0);
}

sub handle_error {
    my ($line, @err) = @_;

	print STDERR __FILE__ . ":$line >> " . join("\n", @err) . "\n";
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

sub increment_name {
    my ($name) = @_;
    
    my ($base, $sep, $num) = ($name =~ m/^(.+)([\s_])(\d+)$/);
    if (defined($num)) {
        return "$base$sep" . ( int($num) + 1 );
    }
    return "${name}_1";
}

sub parse_ret_file {
    my ($file) = @_;
    
    #unless (HistoryDB::connect()) {
    #    handle_error(__LINE__, HistoryDB::get_error());
    #    return undef;
    #}

    unless (open(FH, $file)) {
        handle_error(__LINE__, "Failed to open: $file", $!);
        return undef;
    }
    
    my %raw = ( );
    my $raw = \%raw;
    
    my ( $prev_class, @prev_levels );
    
    while (<FH>) {
        chomp($_);
        
        next if (/^\s*$/);
        next if (/^\s*#/);
        next if (/^\s*obs\s+\d+\s*$/);
        
        my ($class, $level);
        my ($ds, $locs) = (/^\s*([^:\s]+)\s*:(.*)/);
        unless (defined($ds)) {
            if (/^\s*([a-z]{3})(.+)([A-Z]\d{1,2})\.([0a-z]\d)\s*/) {
                ($class, $level) = ($2, $4);
            }
            $locs = '';
        }
        if (defined($ds) || (defined($class) && defined($level))) {
            ($class, $level) = split(/\./, $ds) unless (defined($class) && defined($level));
            unless (defined($class)) {
                print "Skipping malformed datastream class: $ds\n";
                $prev_class = undef;
                next;
            }
            my @levels = ( );
            if (defined($level)) {
                push(@levels, $level);
            } else {
                my @args = ( $class );
                my @cols = ( 'level' );
                my $retval = $DB->query_hasharray("
                
                SELECT DISTINCT ds_class_level
                FROM datastream_classes
                WHERE ds_class_name = ?
                
                ", \@args, \@cols);
                unless (defined($retval)) {
                    handle_error(__LINE__, "Failed to find data levels for => $class", $DB->error());
                    return undef;
                }
                unless (scalar(@{ $retval }) > 0) {
                    handle_error(__LINE__, "No data levels found for class => $class");
                    return undef;
                }
                for my $r (@{ $retval }) {
                    next if ($r->{'level'} eq '00');
                    push(@levels, $r->{'level'});
                }
            }
            $prev_class = $class;
            @prev_levels = @levels;
            
            for $level (@levels) {
                $ds = "$class.$level";
	            unless (defined($raw->{ $ds })) {
	                my %s = (
	                    'class'    => $class,
	                    'level'    => $level,

	                    'locs'     => [ ],
	                    'vars'     => [ ],
	                    'reqd'     => [ ],

	                    '__vars__' => { },
	                );
	                $raw->{ $ds } = \%s;

	                #my $retval = HistoryDB::get_dsclass_fields($class, $level);
                    #unless (defined($retval)) {
                    #    handle_error(__LINE__, HistoryDB::get_error());
                    #    return undef;
                    #}

                    #unless (scalar(@{$retval}) > 0) {
                        my $retval = DODDB::get_dsclass_fields($DB, $class, $level);
                        unless (defined($retval)) {
                            handle_error(__LINE__, DODDB::get_error());
                            return undef;
                        }
                    #}
                
                    my $vmap = $raw->{ $ds }->{'__vars__'};
                
                    for (@{$retval}) {
                        $vmap->{ $_->{'name'} } = $_;
                    }
	            }
            }
	        ($locs) = ($locs =~ m/^\s*<([^>]+)>/);
	        if (defined($locs)) {
	            my @locs = split(/,\s*/, $locs);
	            for my $sf (@locs) {
	                my @s = split(/:/, $sf);
	                next unless (scalar(@s) > 1);
	                
                    my @ps = ( $s[0] =~ m/^\s*([a-z]{3})([A-Z]\d+)\s*$/ );
                    my @pd = ( $s[1] =~ m/^\s*([a-z]{3})([A-Z]\d+)\s*$/ );
                    next if (scalar(@ps) < 2 || scalar(@pd) < 2);

                    my @a = ( join('.', @ps), join('.', @pd) );
                    push(@{ $raw->{ $ds }->{'locs'} }, \@a);
	            }
            }
            next;
        }
        
        my $reqd = 1;
        $reqd = 0 if (/^\*/ || /\*$/);
        
        my @vars = split(/,\s*/);
        unless (defined($prev_class) && scalar(@vars) > 0) {
            print "Skipping orphan line: $_\n";
            next;
        }

        my @vn = ( );
        my $vc = 0;
        for my $v (@vars) {
            $v =~ s/[\s\*]//g;
            $vc++;
            
            my $found = 0;
            for my $prev_level (sort { $b cmp $a } @prev_levels) {
    	        my $ds = $raw->{ "$prev_class.$prev_level" };
	            
	            my $vm = $ds->{'__vars__'}->{ $v };
	            next unless (defined($vm));
	            
	            if ($vc > 1) {
	                push(@{ $ds->{'vars'}->[ $#{ $ds->{'vars'} } ] }, $vm);
	            } else {
                    my @r = ( $vm );
	                push(@{ $ds->{'vars'} }, \@r);
                }
                push(@{ $ds->{'reqd'} }, $reqd);
	            
	            $found = 1;
	            last;
	        }
	        
	        unless ($found) {
	            if (scalar(@prev_levels) > 0) {
	                my $ds = $raw->{ "$prev_class." . $prev_levels[0] };
	            
	                if ($vc > 1) {
	                    push(@{ $ds->{'vars'}->[ $#{ $ds->{'vars'} } ] }, $v);
	                } else {
                        my @r = ( $v );
	                    push(@{ $ds->{'vars'} }, \@r);
                    }
                    push(@{ $ds->{'reqd'} }, $reqd);
                }

	            for my $prev_level (@prev_levels) {
	                my $ds = $raw->{ "$prev_class.$prev_level" };

    	            print "WARNING: variable $v in $prev_class.$prev_level is unknown.\n";
	                my @vnames = keys(%{ $ds->{'__vars__'} });
	                if (scalar(@vnames) > 0) {
	                    for my $vn (sort(@vnames)) {
	                        my $vt = $ds->{'__vars__'}->{ $vn }->{'type'};
	                        my $vd = $ds->{'__vars__'}->{ $vn }->{'dims'};
	                        $vt = (defined($vt)) ? $vt : '?';
	                        $vd = (defined($vd)) ? $vd : '';
	                        print " -- KNOWN => $prev_class.$prev_level : $vn ($vd) : $vt\n";
	                    }
	                } else {
	                    print " -- No names found for $prev_class.$prev_level to recommend --\n";
	                }
	                print "\n";
                }
	        }
        }
    }
    
    close(FH);
    
    my %ret = (
        'fields'  => [ ],
        'shapes'  => { },
        'groups'  => { },
        'queries' => { },
    );
    my $ret = \%ret;
    
    my $gname = 'G 1';
    my $qname = 'Q 1';
    
    my %fnames = ( );
    my $fnames = \%fnames;
    
    for my $ds (sort(keys(%{$raw}))) {
        my $dso = $raw->{ $ds };
        
        while (defined($ret->{'groups'}->{ $gname })) {
            $gname = increment_name($gname);
        }
        while (defined($ret->{'queries'}->{ $qname })) {
            $qname = increment_name($qname);
        }
        my @g = ( $qname );
        $ret->{ 'groups' }->{ $gname } = \@g;
        
        my @query = ( );
        
        push(@{$dso->{'locs'}}, '__rest__');
        for my $dep (@{$dso->{'locs'}}) {
            my ($s, $f)   = (undef, undef);
            my ($sd, $fd) = (undef, undef);
            if (ref($dep) eq 'ARRAY') {
                next unless (scalar(@{$dep}) == 2);
                
                ($s, $f)  = split(/\./, $dep->[0]);
                ($sd, $fd) = split(/\./, $dep->[1]);
                
            } elsif ($dep ne '__rest__') {
                next;
            }

            my %sel = (
                'order' => 1,
                'class' => $dso->{'class'},
                'level' => $dso->{'level'},
                'site'  => $s,
                'fac'   => $f,
                'sdep'  => $sd,
                'fdep'  => $fd,
                'tdep0' => undef,
                'tdep1' => undef,
            );
            push(@query, \%sel);
        }
        $ret->{ 'queries' }->{ $qname } = \@query;
        
        my $index = -1;
        for my $va (@{$dso->{'vars'}}) {
            $index++;
            
            my $var = (ref($va->[0]) eq 'HASH') ? $va->[0]->{'name'} : $va->[0];
            while (defined($fnames->{ $var })) {
                $var = increment_name($var);
            }
            
            my $dtype;
            my @names = ( );
            
            for my $vm (@{$va}) {
                if (ref($vm) eq 'HASH') {
                    push(@names, $vm->{'name'});
                    $dtype = $vm->{'type'} unless ($dtype || !defined($vm->{'type'}));
                } else {
                    push(@names, $vm);
                }
            }
            $dtype = 'float' unless (defined($dtype));
            
            my @v = ( );
            for my $s (@query) {
                my @n = @names;
                push(@v, \@n);
            }
            
            my %v = (
                'name'   => $var,
                'group'  => $gname,
                'vars'   => \@v,
                'dtype'  => $dtype,
                'reqd'   => $dso->{'reqd'}->[ $index ],
                'qc'     => 1,
                'qcreqd' => 0,
                'min'    => undef,
                'max'    => undef,
                'delta'  => undef,
                'units'  => undef,
                'shape'  => undef,
                'window' => [ 0, 0 ],
            );
            push(@{ $ret->{'fields'} }, \%v);
        }
    }
    
    return $ret;
}


################################################################################

sub main {
    my $version     = '@VERSION@';
    my $command     = join(' ', @ARGV);
    my ($program)   = ($0 =~ m/([^\/]+)$/);

    my ($proc_arg_rep, $proc_arg_ret, $prop_mail, $proc_print, $proc_clean, $proc_full, $prop_mrt, $db_alias, $fmt);
    
    exit_usage($program) unless (
        GetOptions(
            'help',       => sub { exit_usage($program); },
            'version',    => sub { print "$version\n"; exit(1); },
            
            'alias=s'     => \$db_alias,
            'replaces=s', => \$proc_arg_rep,
            'ret=s',      => \$proc_arg_ret,
            'print',      => \$proc_print,
            'clean',      => \$proc_clean,
            'full',       => \$proc_full,
            'prop-mail',  => \$prop_mail,
            'prop-mrt',   => \$prop_mrt,
            'fmt=s',      => \$fmt,
        )
    );

	exit_usage($program) unless ($#ARGV == 0);
	
	my $proc_arg = (defined($proc_arg_rep)) ? $proc_arg_rep : ((defined($proc_arg_ret)) ? $proc_arg_ret : undef);
	
	my ($p_name, $p_type);
	if (defined($proc_arg)) {
	    ($p_name, $p_type) = split(/\-/, $proc_arg);
	    
	    exit_usage($program) unless (defined($p_name) && defined($p_type));
	    
	    $p_type = uc($p_type);
	    $p_type = ($p_type eq 'INGEST') ? 'Ingest' : $p_type;
	}
	
	my ($rep_name, $rep_type) = ($p_name, $p_type) if (defined($proc_arg_rep));
	my ($ret_name, $ret_type) = ($p_name, $p_type) if (defined($proc_arg_ret));

	unless (defined($db_alias)) {
	    $db_alias = 'dsdb_data';
	}
	
	$DB = PCMDB::get_connection($db_alias, "$ENV{'HOME'}/.db_connect"); # Because PCMDB's default is the DSDB_HOME conf .db_connect, which is not appropriate.
	unless (defined($DB)) {
		handle_error(__LINE__, PCMDB::get_error());
		return 0;
	}
	
	my $proc_file = $ARGV[0];
	
	unless (-e $proc_file) {
	    handle_error(__LINE__, "File does not exist: $proc_file");
	    return 0;
	}
	
	if (defined($ret_name)) {

	    my $ret = parse_ret_file($proc_file);
	    return(0) unless (defined($ret));
	    
	    if (defined($proc_print)) {
	        print json_encode($ret);
	        return 1;
	    }
	    
	    unless (PCMDB::process_exists($DB, $ret_name, $ret_type)) {

            my $retval = $DB->sp_call('processes', 'define', $ret_type, $ret_name, '');
            unless (defined($retval)) {
                handle_error(__LINE__, "Failed to define process: $ret_name ($ret_type)", $DB->error());
                return 0;
            }
	    }
	    
	    unless (PCMDB::define_retrieval($DB, $ret_type, $ret_name, $ret)) {
	        handle_error(__LINE__, "Failed to define retrieval: $ret_name ($ret_type)", PCMDB::get_error());
	        return 0;
	    }
	    
	} else {
	    my $proc_def;
	    
	    if (defined($fmt) && $fmt eq 'json') {
	        my @lines = ( );
	        if (open(FH, "$proc_file")) {
	            @lines = <FH>;
    	        close(FH);
	        }
    	    $proc_def = json_decode(join("\n", @lines));
	    } else {
    	    $proc_def = do($proc_file);
    	}
        unless (defined($proc_def)) {
            handle_error(__LINE__, "Failed to read process file: $proc_file");
            return 0;
        }
    
        $rep_name = (!defined($rep_name)) ? $proc_def->{'name'} : $rep_name;
        $rep_type = (!defined($rep_type)) ? $proc_def->{'type'} : $rep_type;
        
        unless (defined($prop_mail)) {
            if (defined($proc_def->{'props'})) {
                for my $key (keys(%{ $proc_def->{'props'} })) {
                    if ($key =~ /^(warning|error)_mail$/ || $key =~ /^ui_/) {
                        delete($proc_def->{'props'}->{ $key });
                    }
                }
            }
        }

        unless (defined($prop_mrt)) {
            if (defined($proc_def->{'props'}) && defined($proc_def->{'props'}->{'max_run_time'})) {
                delete($proc_def->{'props'}->{'max_run_time'});
            }
        }

        if (defined($proc_def->{'props'}) && defined($proc_def->{'props'}->{'has_armflow_config'})) {
            delete($proc_def->{'props'}->{'has_armflow_config'});
        }

        my $noclean = (defined($proc_clean)) ? 0 : 1;
    
        unless (PCMDB::save_process($DB, $rep_type, $rep_name, $proc_def, undef, $proc_full, $noclean)) {
            handle_error(__LINE__, "Failed to import process", PCMDB::get_error());
            return 0;
        }
    }

    return 1;
}

exit_failure() unless (main());
exit_success();
