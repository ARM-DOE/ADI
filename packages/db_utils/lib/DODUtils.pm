################################################################################
#
#  COPYRIGHT (C) 2007 Battelle Memorial Institute.  All Rights Reserved.
#
################################################################################
#
#  Author:
#     name:  Brian Ermold
#     phone: (509) 375-2277
#     email: brian.ermold@arm.gov
#
################################################################################

package DODUtils;

use strict;

use Exporter;
use JSON::XS;

use vars qw(@ISA @EXPORT);
use vars qw($gDODUtilsError %gDODSTD %gDOD);

my $_Version = '@VERSION@';

@ISA    = qw(Exporter);
@EXPORT = qw(

    DODUtilsError

    get_dod_standards_hash

    get_dimid
    get_attid
    get_varid

    copy_dim
    copy_att
    copy_var

    define_dim
    define_att
    define_var

    apply_dod_standards

    load_dod_hash
    load_dod

    print_dod_hash
);

%gDODSTD = (

  'time_dim' => {
    'name'         => 'time',
    'length'       => 0,
    'is_unlimited' => 1,
  },

  'global_zebra_atts' => [
    {
      'name'  => 'zeb_platform',
    },
    {
      'name'  => 'history',
    },
  ],

  'global_ingest_atts' => [
    {
      'name'  => 'command_line',
    },
    {
      'name'  => 'process_version',
    },
    {
      'name'  => 'ingest_software',
    },
    {
      'name'  => 'dod_version',
    },
    {
      'name'  => 'site_id',
    },
    {
      'name'  => 'facility_id',
    },
    {
      'name'  => 'data_level',
    },
    {
      'name'  => 'input_source',
    },
  ],

  'resolution_description' =>
    {
      'name'  => 'resolution_description',
      'value' =>
'The resolution field attributes refer to the number of significant digits relative to the decimal point that should be used in calculations. Using fewer digits might result in greater uncertainty. Using a larger number of digits should have no effect and thus is unnecessary. However, analyses based on differences in values with a larger number of significant digits than indicated could lead to erroneous results or misleading scientific conclusions.

resolution for lat = 0.001
resolution for lon = 0.001
resolution for alt = 1',
    },

  'global_mentor_qc_atts' => [
    {
      'name'  => 'qc_standards_version',
      'value' => '1.0',
    },
    {
      'name'  => 'qc_method',
      'value' => 'Standard Mentor QC',
    },
    {
      'name'  => 'qc_comment',
      'value' =>
'The QC field values are a bit packed representation of true/false values for the tests that may have been performed. A QC value of zero means that none of the tests performed on the value failed.

The QC field values make use of the internal binary format to store the results of the individual QC tests. This allows the representation of multiple QC states in a single value. If the test associated with a particular bit fails the bit is turned on. Turning on the bit equates to adding the integer value of the failed test to the current value of the field. The QC field\'s value can be interpreted by applying bit logic using bitwise operators, or by examining the QC value\'s integer representation. A QC field\'s integer representation is the sum of the individual integer values of the failed tests. The bit and integer equivalents for the first 5 bits are listed below:

bit_1 = 00000001 = 0x01 = 2^0 = 1
bit_2 = 00000010 = 0x02 = 2^1 = 2
bit_3 = 00000100 = 0x04 = 2^2 = 4
bit_4 = 00001000 = 0x08 = 2^3 = 8
bit_5 = 00010000 = 0x10 = 2^4 = 16',
    },
    {
      'name'  => 'qc_bit_1_description',
      'value' => 'Value is equal to missing_value.',
    },
    {
      'name'  => 'qc_bit_1_assessment',
      'value' => 'Bad',
    },
    {
      'name'  => 'qc_bit_2_description',
      'value' => 'Value is less than the valid_min.',
    },
    {
      'name'  => 'qc_bit_2_assessment',
      'value' => 'Bad',
    },
    {
      'name'  => 'qc_bit_3_description',
      'value' => 'Value is greater than the valid_max.',
    },
    {
      'name'  => 'qc_bit_3_assessment',
      'value' => 'Bad',
    },
    {
      'name'  => 'qc_bit_4_description',
      'value' =>
'Difference between current and previous values exceeds valid_delta.',
    },
    {
      'name'  => 'qc_bit_4_assessment',
      'value' => 'Indeterminate',
    },
  ],

  'time_vars' => [
    {
      'name' => 'base_time',
      'type' => 'int',
      'atts' => [
        {
          'name'  => 'string',
        },
        {
          'name'  => 'long_name',
          'value' => 'Base time in Epoch',
        },
        {
          'name'  => 'units',
          'value' => 'seconds since 1970-1-1 0:00:00 0:00',
        },
      ],
    },
    {
      'name' => 'time_offset',
      'type' => 'double',
      'dims' => 'time',
      'atts' => [
        {
          'name'  => 'long_name',
          'value' => 'Time offset from base_time',
        },
        {
          'name'  => 'units',
        },
      ],
    },
    {
      'name' => 'time',
      'type' => 'double',
      'dims' => 'time',
      'atts' => [
        {
          'name'  => 'long_name',
          'value' => 'Time offset from midnight',
        },
        {
          'name'  => 'units',
        },
      ],
    },
  ],

  'qc_time_var' => {

    'name' => 'qc_time',
    'type' => 'int',
    'dims' => 'time',
    'atts' => [
      {
        'name'  => 'long_name',
        'value' => 'Quality check results on field: Time offset from midnight',
      },
      {
        'name'  => 'units',
        'value' => 'unitless',
      },
      {
        'name'  => 'description',
        'value' =>
'This field contains bit packed values which should be interpreted as listed. No bits set (zero) represents good data.',
      },
      {
        'name'  => 'bit_1_description',
        'value' => 'Delta time between current and previous samples is zero.',
      },
      {
        'name'  => 'bit_1_assessment',
        'value' => 'Indeterminate',
      },
      {
        'name'  => 'bit_2_description',
        'value' =>
'Delta time between current and previous samples is less than the delta_t_lower_limit field attribute.',
      },
      {
        'name'  => 'bit_2_assessment',
        'value' => 'Indeterminate',
      },
      {
        'name'  => 'bit_3_description',
        'value' =>
'Delta time between current and previous samples is greater than the delta_t_upper_limit field attribute.',
      },
      {
        'name'  => 'bit_3_assessment',
        'value' => 'Indeterminate',
      },
      {
        'name'  => 'delta_t_lower_limit',
        'type'  => 'double',
      },
      {
        'name'  => 'delta_t_upper_limit',
        'type'  => 'double',
      },
      {
        'name'  => 'prior_sample_flag',
        'type'  => 'int',
      },
      {
        'name'  => 'comment',
        'value' =>
'If the \'prior_sample_flag\' is set the first sample time from a new raw file will be compared against the time just previous to it in the stored data. If it is not set the qc_time value for the first sample will be set to 0.',
      },
    ],
  },

  'qc_var' => {

    'name' => 'qc___VAR_NAME__',
    'type' => 'int',
    'dims' => '__VAR_DIMS__',
    'atts' => [
      {
        'name'  => 'long_name',
        'value' => 'Quality check results on field: __VAR_LONG_NAME__',
      },
      {
        'name'  => 'units',
        'value' => 'unitless',
      },
      {
        'name'  => 'description',
        'value' => 'See global attributes for individual bit descriptions.',
      },
    ],
  },

  'location_vars' => [
    {
      'name' => 'lat',
      'type' => 'float',
      'atts' => [
        {
          'name'  => 'long_name',
          'value' => 'North latitude',
        },
        {
          'name'  => 'units',
          'value' => 'degree_N',
        },
        {
          'name'  => 'valid_min',
          'type'  => 'float',
          'value' => '-90',
        },
        {
          'name'  => 'valid_max',
          'type'  => 'float',
          'value' => '90',
        },
      ],
    },
    {
      'name' => 'lon',
      'type' => 'float',
      'atts' => [
        {
          'name'  => 'long_name',
          'value' => 'East longitude',
        },
        {
          'name'  => 'units',
          'value' => 'degree_E',
        },
        {
          'name'  => 'valid_min',
          'type'  => 'float',
          'value' => '-180',
        },
        {
          'name'  => 'valid_max',
          'type'  => 'float',
          'value' => '180',
        },
      ],
    },
    {
      'name' => 'alt',
      'type' => 'float',
      'atts' => [
        {
          'name'  => 'long_name',
          'value' => 'Altitude above mean sea level',
        },
        {
          'name'  => 'units',
          'value' => 'm',
        },
      ],
    },
  ],
);

sub DODUtilsError() { return($gDODUtilsError); }

sub get_dod_standards_hash() { return(\%gDODSTD); }

################################################################################
#
#  Utility Subroutines
#

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

sub load_hash_file($)
{
    my ($file) = @_;
    my $retval;

    unless ($retval = do $file) {
        if ($@) {
            $gDODUtilsError =
                "Could not compile definition file: $@\n -> $file\n";
        }
        elsif (!defined($retval)) {
            $gDODUtilsError =
                "Could not read definition file: $!\n -> $file\n";
        }
        else {
            $gDODUtilsError =
                "Bad return value loading definition file:\n -> $file\n";
        }
        return(0);
    }

    return(1);
}

################################################################################
#
#  Create DOD Using The DOD Definition and Standards
#

sub get_dimid($$)
{
    my ($dims, $dim_name) = @_;
    my $ndims = @{$dims};
    my $dimid;

    for ($dimid = 0; $dimid < $ndims; $dimid++) {
        if ($dim_name eq @{$dims}[$dimid]->{'name'}) {
            return($dimid);
        }
    }

    return(-1);
}

sub get_attid($$)
{
    my ($atts, $att_name) = @_;
    my $natts = @{$atts};
    my $attid;

    for ($attid = 0; $attid < $natts; $attid++) {
        if ($att_name eq @{$atts}[$attid]->{'name'}) {
            return($attid);
        }
    }

    return(-1);
}

sub get_varid($$)
{
    my ($vars, $var_name) = @_;
    my $nvars = @{$vars};
    my $varid;

    for ($varid = 0; $varid < $nvars; $varid++) {
        if ($var_name eq @{$vars}[$varid]->{'name'}) {
            return($varid);
        }
    }

    return(-1);
}

sub copy_dim($$)
{
    my ($out, $in) = @_;

    $out->{'name'}   = $in->{'name'};

    if (defined($in->{'length'})) {
        $out->{'length'} = $in->{'length'};
    }

    if (defined($in->{'is_unlimited'})) {
        $out->{'is_unlimited'} = $in->{'is_unlimited'};
    }
}

sub copy_att($$)
{
    my ($out, $in) = @_;

    $out->{'name'}  = $in->{'name'};

    if (defined($in->{'type'})) {
        $out->{'type'} = $in->{'type'};
    }

    if (defined($in->{'value'})) {
        $out->{'value'} = $in->{'value'};
    }
}

sub copy_var($$)
{
    my ($out, $in) = @_;
    my $att;

    $out->{'name'} = $in->{'name'};
    $out->{'type'} = $in->{'type'};

    if (defined($in->{'dims'})) {
        $out->{'dims'} = $in->{'dims'};
    }

    foreach $att (@{$in->{'atts'}}) {
        define_att(\@{$out->{'atts'}}, $att);
    }
}

sub define_dim($$)
{
    my ($dims, $dim) = @_;
    my $dimid = get_dimid($dims, $dim->{'name'});
    my %new_dim;

    if ($dimid < 0) {
        copy_dim(\%new_dim, $dim);
        push(@{$dims}, \%new_dim);
        $dimid = $#{$dims};
    }
    else {
        copy_dim(\%{@{$dims}[$dimid]}, $dim);
    }

    return($dimid);
}

sub define_att($$)
{
    my ($atts, $att) = @_;
    my $attid = get_attid($atts, $att->{'name'});
    my %new_att;

    if ($attid < 0) {
        copy_att(\%new_att, $att);
        push(@{$atts}, \%new_att);
        $attid = $#{$atts};
    }
    else {
        copy_att(\%{@{$atts}[$attid]}, $att);
    }

    return($attid);
}

sub define_var($$)
{
    my ($vars, $var) = @_;
    my $varid = get_varid($vars, $var->{'name'});
    my %new_var;

    if ($varid < 0) {
        copy_var(\%new_var, $var);
        push(@{$vars}, \%new_var);
        $varid = $#{$vars};
    }
    else {
        copy_var(\%{@{$vars}[$varid]}, $var);
    }

    return($varid);
}

sub apply_dod_standards($$)
{
    my ($dodstd, $dod_hash) = @_;
    my %dod;
    my $att;
    my $dim;
    my $var;
    my $varid;
    my $long_name;
    my $has_qc_vars;
    my $has_res_atts;

    $dod{'ds_class'}    = $dod_hash->{'ds_class'};
    $dod{'dod_version'} = $dod_hash->{'dod_version'};

    # Define Dimensions #######################################################

    # time dimension

    if ($dod_hash->{'use_dod_standards'}{'time_vars'}) {
        define_dim(\@{$dod{'dims'}}, $dodstd->{'time_dim'});
    }

    # dod dimensions

    foreach $dim (@{$dod_hash->{'dims'}}) {
        define_dim(\@{$dod{'dims'}}, $dim);
    }

    # Define Variables #######################################################

    $has_qc_vars  = 0;
    $has_res_atts = 0;

    # time variables

    if ($dod_hash->{'use_dod_standards'}{'time_vars'}) {
        foreach $var (@{$dodstd->{'time_vars'}}) {
            define_var(\@{$dod{'vars'}}, $var);
        }
    }

    # qc_time variable

    if ($dod_hash->{'qc_time'}) {

        $varid = define_var(\@{$dod{'vars'}}, $dodstd->{'qc_time_var'});

        foreach $att (@{$dod{'vars'}[$varid]{'atts'}}) {
            if (defined($dod_hash->{'qc_time'}{$att->{'name'}})) {
                $att->{'value'} = $dod_hash->{'qc_time'}{$att->{'name'}};
            }
        }
    }

    # dod variables

    foreach $var (@{$dod_hash->{'vars'}}) {

        define_var(\@{$dod{'vars'}}, $var);

        # check for resolution attribute

        foreach $att (@{$var->{'atts'}}) {
            if ($att->{'name'} eq 'resolution') {
                $has_res_atts = 1;
            }
        }

        # standard mentor qc variable

        if ($var->{'add_qc_field'}) {

            $varid = define_var(\@{$dod{'vars'}}, $dodstd->{'qc_var'});

            # update qc_var name and dims

            $dod{'vars'}[$varid]{'name'} =~ s/__VAR_NAME__/$var->{'name'}/;

            if (defined($var->{'dims'})) {
                $dod{'vars'}[$varid]{'dims'} =~ s/__VAR_DIMS__/$var->{'dims'}/;
            }
            else {
                delete($dod{'vars'}[$varid]{'dims'});
            }

            # update qc_var long_name

            $long_name = $var->{'name'};

            foreach $att (@{$var->{'atts'}}) {
                if ($att->{'name'} eq 'long_name') {
                    $long_name = $att->{'value'};
                    last;
                }
            }

            foreach $att (@{$dod{'vars'}[$varid]{'atts'}}) {
                if ($att->{'name'} eq 'long_name') {
                    $att->{'value'} =~ s/__VAR_LONG_NAME__/$long_name/;
                    last;
                }
            }

            $has_qc_vars = 1;
        }
    }

    # location variables (lat, lon, and alt)

    if ($dod_hash->{'use_dod_standards'}{'location_vars'}) {
        foreach $var (@{$dodstd->{'location_vars'}}) {
            define_var(\@{$dod{'vars'}}, $var);
        }
    }

    # Define Attributes #######################################################

    # standard ingest attributes

    if ($dod_hash->{'use_dod_standards'}{'global_ingest_atts'}) {

        foreach $att (@{$dodstd->{'global_ingest_atts'}}) {
            define_att(\@{$dod{'atts'}}, $att);
        }

        if ($has_res_atts) {
            define_att(\@{$dod{'atts'}}, $dodstd->{'resolution_description'});
        }
    }

    # dod attributes

    foreach $att (@{$dod_hash->{'atts'}}) {
        define_att(\@{$dod{'atts'}}, $att);
    }

    # standard mentor qc attributes

    if ($dod_hash->{'use_dod_standards'}{'global_ingest_atts'}) {
        if ($has_qc_vars) {
            foreach $att (@{$dodstd->{'global_mentor_qc_atts'}}) {
                define_att(\@{$dod{'atts'}}, $att);
            }
        }
    }

    # zebra attributes

    if ($dod_hash->{'use_dod_standards'}{'global_zebra_atts'}) {
        foreach $att (@{$dodstd->{'global_zebra_atts'}}) {
            define_att(\@{$dod{'atts'}}, $att);
        }
    }

    return(\%dod);
}

################################################################################
#
#  Subroutines To Load DOD Using Definition and Standards Hashes
#

sub load_dod_json($)
{
    my ($file) = @_;
    
    $gDODUtilsError = '';
    %gDOD = ();
    
    unless (open(FH, "$file")) {
        $gDODUtilsError =
            "Count not open $file: $!";
        return(0);
    }
    my $txt = join("\n", <FH>);
    close(FH);
    
    my $dod;
    eval { $dod = json_decode($txt); };
    
    if (defined($dod) &&
        defined($dod->{'class'}) &&
        defined($dod->{'level'}) &&
        defined($dod->{'version'})) {
        $dod->{'ds_class'}    = join('.', $dod->{'class'}, $dod->{'level'});
        $dod->{'dod_version'} = $dod->{'version'};
        for (@{$dod->{'vars'}}) {
            $_->{'dims'} = join(',', @{$_->{'dims'}});
        }
        return($dod);
    }
    
    return(undef);
}

sub load_dod_hash($)
{
    my ($file) = @_;

    $gDODUtilsError = '';
    %gDOD = ();

    unless (load_hash_file($file)) {
        return(undef);
    }

    return(\%gDOD);
}

sub load_dod($)
{
    my ($dod_file) = @_;
    my $dod_hash;
    my $dod;

    $gDODUtilsError = '';
    
    unless ($dod_hash = load_dod_json($dod_file)) {
        unless ($dod_hash = load_dod_hash($dod_file)) {
            return(undef);
        }
    }

    $dod = apply_dod_standards(\%gDODSTD, $dod_hash);

    return($dod);
}

################################################################################
#
#  Print DOD Hash
#

sub print_dod_hash($$)
{
    my ($dod, $file) = @_;
    my $dim;
    my $att;
    my $var;
    my $value;
    my $print_newline;

    $gDODUtilsError = '';

    if ($file) {
        unless (open(FH, ">$file")) {
            $gDODUtilsError = "Could Not Open File: $file\n -> $!\n";
            return(0);
        }
    }
    else {
        unless (open(FH, ">&STDOUT")) {
            $gDODUtilsError = "Could Not Open STDOUT: $!\n";
            return(0);
        }
    }

    print(FH
        "\%gDOD = (\n" .
        "\n");

    # print DOD class and version if they are defined

    $print_newline = 0;

    if ($dod->{'ds_class'}) {
        print(FH "  'ds_class'    => '$dod->{'ds_class'}',\n");
        $print_newline = 1;
    }

    if ($dod->{'dod_version'}) {
        print(FH "  'dod_version' => '$dod->{'dod_version'}',\n");
        $print_newline = 1;
    }

    if ($print_newline) {
        print(FH "\n");
    }

    # print use_dod_standards section if it is defined

    if ($dod->{'use_dod_standards'}) {

        print(FH "  'use_dod_standards' => {\n");

        if ($dod->{'use_dod_standards'}{'global_ingest_atts'}) {
            print(FH "      'global_ingest_atts' => 1,\n");
        }
        if ($dod->{'use_dod_standards'}{'global_zebra_atts'}) {
            print(FH "      'global_zebra_atts'  => 1,\n");
        }
        if ($dod->{'use_dod_standards'}{'time_vars'}) {
            print(FH "      'time_vars'          => 1,\n");
        }
        if ($dod->{'use_dod_standards'}{'location_vars'}) {
            print(FH "      'location_vars'      => 1,\n");
        }

        print(FH "  },\n");
    }

    # print qc_time section if it is defined

    if ($dod->{'qc_time'}) {

        print(FH "  'qc_time' => {\n");

        $value = $dod->{'qc_time'}{'delta_t_lower_limit'};
        if ($value) {
            print(FH "      'delta_t_lower_limit' => $value,\n");
        }

        $value = $dod->{'qc_time'}{'delta_t_upper_limit'};
        if ($value) {
            print(FH "      'delta_t_upper_limit' => $value,\n");
        }

        $value = $dod->{'qc_time'}{'prior_sample_flag'};
        if ($value) {
            print(FH "      'prior_sample_flag'   => $value,\n");
        }

        print(FH "  },\n");
    }

    # print dimensions

    print(FH "  'dims' => [\n");

    foreach $dim (@{$dod->{'dims'}}) {

        print(FH
            "    {\n" .
            "      'name'   => '$dim->{'name'}',\n");

        if (defined($dim->{'length'})) {
            print(FH "      'length' => $dim->{'length'},\n");
        }

        if (defined($dim->{'is_unlimited'})) {
            print(FH "      'is_unlimited' => $dim->{'is_unlimited'},\n");
        }

        print(FH "    },\n");
    }

    print(FH "  ],\n");

    # print attributes

    print(FH "  'atts' => [\n");

    foreach $att (@{$dod->{'atts'}}) {

        print(FH
            "    {\n" .
            "      'name'  => '$att->{'name'}',\n");

        if (defined($att->{'type'})) {
            print(FH
                "      'type'  => '$att->{'type'}',\n");
        }

        if (defined($att->{'value'})) {
            $att->{'value'} =~ s/'/\\'/g;
            print(FH "      'value' => '$att->{'value'}',\n");
        }

        print(FH "    },\n");
    }

    print(FH "  ],\n");

    # print variables

    print(FH "  'vars' => [\n");

    foreach $var (@{$dod->{'vars'}}) {

        print(FH
            "    {\n" .
            "      'name' => '$var->{'name'}',\n" .
            "      'type' => '$var->{'type'}',\n");

        if (defined($var->{'dims'})) {
            print(FH "      'dims' => '$var->{'dims'}',\n");
        }

        # print variable attributes

        print(FH "      'atts' => [\n");

        foreach $att (@{$var->{'atts'}}) {

            print(FH
                "        {\n" .
                "          'name'  => '$att->{'name'}',\n");

            if (defined($att->{'type'})) {
                print(FH
                    "          'type'  => '$att->{'type'}',\n");
            }

            if (defined($att->{'value'})) {
                $att->{'value'} =~ s/'/\\'/g;
                print(FH "          'value' => '$att->{'value'}',\n");
            }

            print(FH "        },\n");
        }

        print(FH "      ],\n");

        if ($var->{'add_qc_field'}) {
            print(FH "      'add_qc_field' => 1,\n");
        }

        print(FH "    },\n");
    }

    print(FH
        "  ],\n" .
        ");\n\n");

    return(1);
}

1;
