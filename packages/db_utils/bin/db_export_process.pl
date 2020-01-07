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

use Data::Dumper;
use Cwd qw(abs_path);

use Getopt::Long;
Getopt::Long::Configure qw(no_ignore_case);

use PCMDB;

my $DB;


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

$program [-h|v] <process>-<type>

    [-h|help]       => Display this usage message.
    [-v]            => Show version.

    [-a alias]      => DB alias from user .db_connect file.
                       [ default: dsdb_ref ]

      EXAMPLES:
        > $program mfrsr-ingest

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
    my $db_alias;
    
    exit_usage($program) unless (
        GetOptions(
            'help',     => sub { exit_usage($program); },
            'version',  => sub { print "$version\n"; exit(1); },
            'alias=s'   => \$db_alias,
        )
    );

	exit_usage($program) unless ($#ARGV == 0);
	
	my ($proc_name, $proc_type) = split(/\-/, $ARGV[0]);
	
	exit_usage($program) unless (defined($proc_name) && defined($proc_type));
	
	$proc_type = uc($proc_type);
	$proc_type = ($proc_type eq 'INGEST') ? 'Ingest' : $proc_type;

	unless (defined($db_alias)) {
	    $db_alias = 'dsdb_ref';
	}
	
	$DB = PCMDB::get_connection($db_alias);
	unless (defined($DB)) {
		handle_error(__LINE__, PCMDB::get_error());
		return 0;
	}
	
	unless (PCMDB::process_exists($DB, $proc_type, $proc_name)) {
	    handle_error(__LINE__, "Process $proc_name:$proc_type does not exist.");
	    return 0;
	}
	
	my $proc = PCMDB::get_process($DB, $proc_type, $proc_name);
	unless (defined($proc)) {
	    handle_error(__LINE__, "Failed to get process: $proc_name:$proc_type");
	    return 0;
	}
	
	$Data::Dumper::Terse = 1;
	$Data::Dumper::Indent = 1;
	print Dumper($proc);

    return 1;
}

exit_failure() unless (main());
exit_success();
