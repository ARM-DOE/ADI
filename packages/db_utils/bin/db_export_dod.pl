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

use DBI;
use CGI;
use CGI::Session;
use JSON::XS;
use File::Basename;
use Cwd qw(abs_path);

use Getopt::Long;
Getopt::Long::Configure qw(no_ignore_case);

use DODDB;
use DBCORE;
use DODUtils;

my $DB;

my $DBConnect = abs_path("$ENV{'HOME'}/.db_connect");
unless (-e $DBConnect) {
    my $DSDBHome = ($ENV{DSDB_HOME}) ? $ENV{DSDB_HOME} : '/apps/ds';
    $DBConnect = abs_path("$DSDBHome/conf/dsdb/.db_connect");
}

################################################################################

sub cleanup {
    if ($DB) {
		$DB->disconnect();
	}
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

$program [-h|v] [-type maker|hash|archive] -dod <class>.<level>-<version>

    [-h|help]       => Display this usage message.
    [-v]            => Show version.
    
    [-a alias]      => DB alias from user .db_connect file.
                       [ default: dsdb_ref ]

    [-type]         => maker:   MAKER file for BW.
                       hash:    Perl hash used for DB import.
                       archive: CSV file to be submitted to John Bell
                                at the Archive.
    
    [-ds ds]        => <class>:   Datastream class (e.g. vceil25k).
                       <level>:   Data level (e.g. c1).
                       
                       Only applicable for type "archive".

    [-dod dod]      => <class>:   Datastream class (e.g. mfrsr).
                       <level>:   Data level (e.g. b1).
                       <version>: DOD version (e.g. 1.2).

      EXAMPLES:
        > $program -type hash -dod 15ebbr.b1-2.0
        > $program -type maker -dod mwravg.c1-1.1

EOF
    exit(0);
}

sub handle_error {
    my ($line, @err) = @_;

	print STDERR __FILE__ . ":$line >> " . join("\n", @err);
}


################################################################################

sub main {
    my $version     = '@VERSION@';
    my $command     = join(' ', @ARGV);
    my ($program)   = ($0 =~ m/([^\/]+)$/);

    my ($type, $dod_arg, $ds_arg, $db_alias);
    
    exit_usage($program) unless (
        GetOptions(
            'help',     => sub { exit_usage($program); },
            'version',  => sub { print "$version\n"; exit(1); },
            
            'alias=s'   => \$db_alias,
            'type=s',   => \$type,
            'dod=s',    => \$dod_arg,
            'ds=s',     => \$ds_arg,
        )
    );

	exit_usage($program) unless (defined($type) && (defined($dod_arg) || defined($ds_arg)));
    
	$type = uc($type);
	exit_usage($program) unless ($type eq 'MAKER' || $type eq 'HASH' || $type eq 'ARCHIVE');
	
	my ($ds_class, $ds_level, $dod_version);
	
	if (defined($dod_arg)) {
	    ($ds_class, $ds_level, $dod_version) = ($dod_arg =~ m/^([a-zA-Z0-9\-_]+)\.([0a-z][0-9])-([0-9\.]+)$/);
	    exit_usage($program) unless (defined($ds_class) && defined($ds_level) && defined($dod_version));
	    exit_usage($program) unless ($type eq 'MAKER' || $type eq 'HASH');

    } else {
        ($ds_class, $ds_level) = ($ds_arg =~ m/^([a-zA-Z0-9\-_]+)\.([0a-z][0-9])$/);
        exit_usage($program) unless (defined($ds_class) && defined($ds_level));
        exit_usage($program) unless ($type eq 'ARCHIVE');
    }

	unless (defined($db_alias)) {
	    $db_alias = 'dsdb_ref';
	}

	$DB = DODDB::get_connection($db_alias, $DBConnect);
	unless (defined($DB)) {
		handle_error(__LINE__, DODDB::get_error());
		exit_failure();
	}
	
	my ($dod, $ds);
	
	if (defined($ds_arg)) {
	    $ds = DODDB::get_ds_class_info($DB, $ds_class, $ds_level);
	    unless (defined($ds)) {
	        handle_error(__LINE__, "Failed to read datastream", "$ds_class.$ds_level");
	        exit_failure();
	    }
	}
	
	if (defined($dod_arg)) {
	    $dod = DODDB::get_dod_version($DB, $ds_class, $ds_level, $dod_version);
	    unless (defined($dod)) {
		    handle_error(__LINE__, "Failed to read DOD version", "$ds_class.$ds_level-v$dod_version");
		    exit_failure();
	    }
	
    	for (@{$dod->{'vars'}}) {
    		$_->{'dims'} = join(',',@{$_->{'dims'}});
    	}
    }
	
	if ($type eq 'HASH' && defined($dod)) {

		$dod->{'ds_class'}	  = "$ds_class.$ds_level";
		$dod->{'dod_version'} = "$dod_version";
		
		DODUtils::print_dod_hash($dod, undef);
		
	} elsif ($type eq 'ARCHIVE') {
	    
	    if (defined($ds) && $ds) {
	        $ds = from_json($ds);
	        
	        for my $pm (@{$ds->{'pm'}}) {
	            my $v = $pm->{'v'}->{'name'};
	            my $m = $pm->{'m'}->{'code'};
	            print "$v|$m||\n";
	        }
	        
	    } elsif (defined($dod)) {

    	    for (@{$dod->{'vars'}}) {
    	        my ($name, $long_name, $units);
    	        for my $a (@{$_->{'atts'}}) {
    	            last if (defined($long_name) && defined($units));
    	            if ($a->{'name'} eq 'long_name') {
    	                $long_name = $a->{'value'};
    	                next;
    	            }
    	            if ($a->{'name'} eq 'units') {
    	                $units = $a->{'value'};
    	                next;
    	            }
    	        }
    	        $name = $_->{'name'};
    	        $long_name = (defined($long_name)) ? $long_name : '';
    	        $units = (defined($units)) ? $units : '';
	        
    	        print "$name|$long_name|$units||\n";
    	    }
        }
	    
	} elsif ($type eq 'MAKER' && defined($dod)) {
		
		my @atts = ();
		for (@{$dod->{'atts'}}) {
			if (defined($_->{'value'}) && $_->{'value'} ne '') {
				if ($_->{'type'} eq 'char') {
					$_->{'value'} =~ s/\n/ \\\n     /g;
					push(@atts, '   ' . $_->{'name'} . ' = ' . $_->{'value'});
				} else {
					my @vals = split(',', $_->{'value'});
					push(@atts, '   ' . $_->{'name'} . '{' . scalar(@vals) . ',' . $_->{'type'} . '} = ' . $_->{'value'});
				}
			}
		}
		
		my @dims = ();
		for (@{$dod->{'dims'}}) {
			push(@dims, '   ' . $_->{'name'} . ((defined($_->{'length'})) ? '   ' . $_->{'length'} : ''));
		}
		
		my @vars = ();
		for (@{$dod->{'vars'}}) {
			next if (
				$_->{'name'} eq 'base_time' ||
				$_->{'name'} eq 'time_offset' ||
				$_->{'name'} eq 'time' ||
				$_->{'name'} eq 'lat' ||
				$_->{'name'} eq 'lon' ||
				$_->{'name'} eq 'alt'
			);
			
			my @vatts = ();
			for (@{$_->{'atts'}}) {
				if (defined($_->{'value'}) && $_->{'value'} ne '') {
					if ($_->{'type'} eq 'char') {
						$_->{'value'} =~ s/\n/ \\\n       /g;
						push(@vatts, '     ' . $_->{'name'} . ' = ' . $_->{'value'});
					} else {
						my @vals = split(',', $_->{'value'});
						push(@vatts, '     ' . $_->{'name'} . '{' . scalar(@vals) . ',' . $_->{'type'} . '} = ' . $_->{'value'});
					}
				}
			}
			my $var = '   ' . $_->{'name'} . '(' . $_->{'dims'} . ')   ' . scalar(@vatts) . '   [' . $_->{'type'} . "]\n";
			$var .= join("\n", @vatts);
			push(@vars, $var);
		}
		
		my $platform = '   <site>' . $ds_class . '<fac>.' . $ds_level;
		$platform .= '   nspace   ' . scalar(@atts) . '   <N>   ' . scalar(@vars) . '   <N>';
		
		my $atts = join("\n", @atts);
		my $dims = '   ' . scalar(@dims) . "\n" . join("\n", @dims);
		my $vars = join("\n", @vars);
		
		print STDOUT <<EOF;

#  Below is an example.  The actual format needed is commented at the bottom.
#
# NPLATFORMS
     1
#
# PLATFORM             CLASS     NPLATATTR      NSUBPLATFORMS  NFIELDS  maxSamples
$platform
#
#   PLATFORM ATTRIBUTES:
$atts
#
#   SUBPLATFORM INFORMATION:
#       lat     lon     alt
        -999.00     -999.00     -999.00
#
# Dimension info
#
$dims
#
#   FIELD INFORMATION (field name followed by field attributes):
#       field           nFieldAttr
$vars
#
#
# PRODUCTION ATTRIBUTES
        0
#
#
# CLASS ATTRIBUTES
#
        0

EOF
	}

    return 1;
}

exit_failure() unless (main());
exit_success();
