/*******************************************************************************
*
*  Copyright © 2014, Battelle Memorial Institute
*  All rights reserved.
*
********************************************************************************
*
*  Author:
*     name:  Brian Ermold
*     phone: (509) 375-2277
*     email: brian.ermold@pnnl.gov
*
*******************************************************************************/

#include "cds3_factory.h"

/*******************************************************************************
*  Data
*/

unsigned char  CharData[]   = {         'A',         'B',   'C',        'D',        'E' };
signed char    ByteData[]   = {        -128,         -64,     0,         64,        127 };
short          ShortData[]  = {      -32768,      -16384,     0,      16384,      32767 };
int            IntData[]    = { -2147483648, -1073741824,     0, 1073741824, 2147483647 };

float          FloatData[]  = {         -3.402823e+38f,         -2.147484e+09, 0.0,         2.147484e+09,         3.402823e+38f };
double         DoubleData[] = { -1.79769313486231e+308, -9.22337203685478e+17, 0.0, 9.22337203685478e+17, 1.79769313486231e+308 };

long long      Int64Data[]  = { -9223372036854775807LL-1, -4611686018427387904LL, 0, 4611686018427387904LL, 9223372036854775807LL };

unsigned char      UByteData[]  = { 0,     32,     64,     128,     255 };
unsigned short     UShortData[] = { 0,   8192,  16384,   32768,   65535 };
unsigned int       UIntData[]   = { 0,  536870912,  1073741824,  2147483648,  4294967295U };
unsigned long long UInt64Data[] = { 0,  2305843009213692952, 4611686018427387904,  9223372036854775808ULL,  18446744073709551615ULL };

static char *StringData[] = {
    "string 1", "string 2", "string 3", "string 4", "string 5"
};

char           TextAtt[]      = "Single line text attribute.";
char           MultiLineAtt[] = "Multi line text attribute:\n"
                                "    - Line 1\n"
                                "    - Line 2\n"
                                "    - Line 3";

double TimeData[]  = {  10.0,  20.1,  30.2,  40.3,  50.4,
                        60.5,  70.6,  80.7,  90.8, 100.9 };

int    RangeData[] = {  15,    30,   45,   60,   75,   90,  105,  120,  135,  150,
                       165,   180,  195,  210,  225,  240,  255,  270,  285,  300,
                       315,   330,  345,  360,  375,  390,  405,  420,  435,  450 };

double DoubleData2D[] = {
       1.0,    2.1,    3.2,    4.3,    5.4,    6.5,    7.6,    8.7,    9.8,   10.9,
      11.0,   12.1,   13.2,   14.3,   15.4,   16.5,   17.6,   18.7,   19.8,   20.9,
      21.0,   22.1,   23.2,   24.3,   25.4,   26.5,   27.6,   28.7,   29.8,   30.9,
      31.0,   32.1,   33.2,   34.3,   35.4,   36.5,   37.6,   38.7,   39.8,   40.9,
      41.0,   42.1,   43.2,   44.3,   45.4,   46.5,   47.6,   48.7,   49.8,   50.9,
      51.0,   52.1,   53.2,   54.3,   55.4,   56.5,   57.6,   58.7,   59.8,   60.9,
      61.0,   62.1,   63.2,   64.3,   65.4,   66.5,   67.6,   68.7,   69.8,   70.9,
      71.0,   72.1,   73.2,   74.3,   75.4,   76.5,   77.6,   78.7,   79.8,   80.9,
      81.0,   82.1,   83.2,   84.3,   85.4,   86.5,   87.6,   88.7,   89.8,   90.9,
      91.0,   92.1,   93.2,   94.3,   95.4,   96.5,   97.6,   98.7,   99.8,  100.9,
     101.0,  102.1,  103.2,  104.3,  105.4,  106.5,  107.6,  108.7,  109.8,  110.9,
     111.0,  112.1,  113.2,  114.3,  115.4,  116.5,  117.6,  118.7,  119.8,  120.9,
     121.0,  122.1,  123.2,  124.3,  125.4,  126.5,  127.6,  128.7,  129.8,  130.9,
     131.0,  132.1,  133.2,  134.3,  135.4,  136.5,  137.6,  138.7,  139.8,  140.9,
     141.0,  142.1,  143.2,  144.3,  145.4,  146.5,  147.6,  148.7,  149.8,  150.9 };

char CharData2D[] =
    "char array 1\0"
    "char array 2\0"
    "char array 3\0"
    "char array 4\0"
    "char array 5\0"
    "char array 6\0"
    "char array 7\0"
    "char array 8\0"
    "char array 9\0"
    "char array 10"
    "char array 11"
    "char array 12"
    "char array 13"
    "char array 14"
    "char array 15";

/*******************************************************************************
*  Dimension Definitions
*/

DimDef G1_Dims[] = {
    { "time",             0, 1 },
    { "range",           10, 0 },
    { NULL,               0, 0 }
};

DimDef G2_Dims[] = {
    { NULL,               0, 0 }
};

DimDef RootDims[] = {
    { "time",             0, 1 },
    { "range",           20, 0 },
    { "static",           5, 0 },
    { "chrlen",          13, 0 },
    { NULL,               0, 0 }
};

/*******************************************************************************
*  Attribute Definitions
*/

AttDef TimeVarAtts[] = {
    { "long_name", CDS_CHAR, 0, "Time offset from midnight" },
    { "units",     CDS_CHAR, 0, "seconds since 2010-03-28 00:00:00 0:00" },
    { NULL,        CDS_NAT,  0, NULL }
};

AttDef RangeVarAtts[] = {
    { "long_name", CDS_CHAR, 0, "Distance to the center of the corresponding range bin." },
    { "units",     CDS_CHAR, 0, "m" },
    { NULL,        CDS_NAT,  0, NULL }
};

AttDef CharVarAtts[] = {
    { "long_name", CDS_CHAR, 0, "Character variable test" },
    { "units",     CDS_CHAR, 0, "unitless" },
    { NULL,        CDS_NAT,  0, NULL }
};

AttDef ByteVarAtts[] = {
    { "long_name", CDS_CHAR, 0, "Byte variable test" },
    { "units",     CDS_CHAR, 0, "unitless" },
    { NULL,        CDS_NAT,  0, NULL }
};

AttDef ShortVarAtts[] = {
    { "long_name", CDS_CHAR, 0, "Short variable test" },
    { "units",     CDS_CHAR, 0, "unitless" },
    { NULL,        CDS_NAT,  0, NULL }
};

AttDef IntVarAtts[] = {
    { "long_name", CDS_CHAR, 0, "Integer variable test" },
    { "units",     CDS_CHAR, 0, "unitless" },
    { NULL,        CDS_NAT,  0, NULL }
};

AttDef FloatVarAtts[] = {
    { "long_name", CDS_CHAR, 0, "Float variable test" },
    { "units",     CDS_CHAR, 0, "unitless" },
    { NULL,        CDS_NAT,  0, NULL }
};

double DoubleFill = 0;

AttDef DoubleVarAtts[] = {
    { "long_name", CDS_CHAR, 0, "Double variable test" },
    { "units",     CDS_CHAR, 0, "unitless" },
    { NULL,        CDS_NAT,  0, NULL }
};

AttDef Int64VarAtts[] = {
    { "long_name", CDS_CHAR, 0, "64 bit Integer variable test" },
    { "units",     CDS_CHAR, 0, "unitless" },
    { NULL,        CDS_NAT,  0, NULL }
};

AttDef UByteVarAtts[] = {
    { "long_name", CDS_CHAR, 0, "Unsigned Byte variable test" },
    { "units",     CDS_CHAR, 0, "unitless" },
    { NULL,        CDS_NAT,  0, NULL }
};

AttDef UShortVarAtts[] = {
    { "long_name", CDS_CHAR, 0, "Unsigned Short variable test" },
    { "units",     CDS_CHAR, 0, "unitless" },
    { NULL,        CDS_NAT,  0, NULL }
};

AttDef UIntVarAtts[] = {
    { "long_name", CDS_CHAR, 0, "Unsigned Integer variable test" },
    { "units",     CDS_CHAR, 0, "unitless" },
    { NULL,        CDS_NAT,  0, NULL }
};

AttDef UInt64VarAtts[] = {
    { "long_name", CDS_CHAR, 0, "Unsigned 64 bit Integer variable test" },
    { "units",     CDS_CHAR, 0, "unitless" },
    { NULL,        CDS_NAT,  0, NULL }
};

AttDef StringVarAtts[] = {
    { "long_name", CDS_CHAR, 0, "String variable test" },
    { "units",     CDS_CHAR, 0, "unitless" },
    { NULL,        CDS_NAT,  0, NULL }
};

AttDef Var2DAtts[] = {
    { "long_name", CDS_CHAR, 0, "2D variable test" },
    { "units",     CDS_CHAR, 0, "unitless" },
    { NULL,        CDS_NAT,  0, NULL }
};

AttDef CharVar2DAtts[] = {
    { "long_name", CDS_CHAR, 0, "2D Char variable test" },
    { "units",     CDS_CHAR, 0, "unitless" },
    { NULL,        CDS_NAT,  0, NULL }
};

AttDef SG1_Atts[] = {
    { "att_float",      CDS_FLOAT,  5,  (void *)FloatData },
    { "att_double",     CDS_DOUBLE, 5,  (void *)DoubleData },
    { "att_text",       CDS_CHAR,   0,  (void *)TextAtt },
    { "att_multi_line", CDS_CHAR,   0,  (void *)MultiLineAtt },
    { NULL,             CDS_NAT,    0,  NULL }
};

AttDef G1_Atts[] = {
    { "att_char",       CDS_CHAR,   5,  (void *)CharData },
    { "att_byte",       CDS_BYTE,   5,  (void *)ByteData },
    { "att_short",      CDS_SHORT,  5,  (void *)ShortData },
    { "att_int",        CDS_INT,    5,  (void *)IntData },
    { NULL,             CDS_NAT,    0,  NULL }
};

AttDef G2_Atts[] = {
    { "att_float",      CDS_FLOAT,  5,  (void *)FloatData },
    { "att_double",     CDS_DOUBLE, 5,  (void *)DoubleData },
    { "att_text",       CDS_CHAR,   0,  (void *)TextAtt },
    { "att_multi_line", CDS_CHAR,   0,  (void *)MultiLineAtt },
    { NULL,             CDS_NAT,    0,  NULL }
};

AttDef RootAtts[] = {
    { "att_char",       CDS_CHAR,   5,  (void *)CharData },
    { "att_byte",       CDS_BYTE,   5,  (void *)ByteData },
    { "att_short",      CDS_SHORT,  5,  (void *)ShortData },
    { "att_int",        CDS_INT,    5,  (void *)IntData },
    { "att_float",      CDS_FLOAT,  5,  (void *)FloatData },
    { "att_double",     CDS_DOUBLE, 5,  (void *)DoubleData },
    { "att_text",       CDS_CHAR,   0,  (void *)TextAtt },
    { "att_multi_line", CDS_CHAR,   0,  (void *)MultiLineAtt },
    { "att_int64",      CDS_INT64,  5,  (void *)Int64Data },
    { "att_ubyte",      CDS_UBYTE,  5,  (void *)UByteData },
    { "att_ushort",     CDS_USHORT, 5,  (void *)UShortData },
    { "att_uint",       CDS_UINT,   5,  (void *)UIntData },
    { "att_uint64",     CDS_UINT64, 5,  (void *)UInt64Data },
    { "att_string",     CDS_STRING, 5,  (void *)StringData },
    { NULL,             CDS_NAT,    0,  NULL }
};

/*******************************************************************************
*  Variable Definitions
*/

const char *TimeDim[]        = { "time",   NULL  };
const char *RangeDim[]       = { "range",  NULL };
const char *StaticDim[]      = { "static", NULL };
const char *TimeRangeDims[]  = { "time", "range",  NULL };
const char *TimeCharDims[]   = { "time", "chrlen", NULL };

VarDef SG1_Vars[] = {

    { "var_int_static",    CDS_INT,    StaticDim,      IntVarAtts,    5, IntData },
    { "var_float_static",  CDS_FLOAT,  StaticDim,      FloatVarAtts,  5, FloatData },
    { "var_double_static", CDS_DOUBLE, StaticDim,      DoubleVarAtts, 5, DoubleData },

    { "var_int",           CDS_INT,    TimeDim,        IntVarAtts,    5, IntData },
    { "var_float",         CDS_FLOAT,  TimeDim,        FloatVarAtts,  5, FloatData },
    { "var_double",        CDS_DOUBLE, TimeDim,        DoubleVarAtts, 5, DoubleData },

    { "var_char_2D",       CDS_CHAR,   TimeCharDims,   CharVar2DAtts, 5, &CharData2D[130] },
    { NULL,                CDS_NAT,    NULL,           NULL,          0, NULL }
};

VarDef G1_Vars[] = {

    { "time",              CDS_DOUBLE, TimeDim,        TimeVarAtts,   5, &TimeData[5] },
    { "range",             CDS_INT,    RangeDim,       RangeVarAtts, 10, &RangeData[20] },

    { "var_char_static",   CDS_CHAR,   StaticDim,      CharVarAtts,   5, CharData },
    { "var_byte_static",   CDS_BYTE,   StaticDim,      ByteVarAtts,   5, ByteData },
    { "var_short_static",  CDS_SHORT,  StaticDim,      ShortVarAtts,  5, ShortData },

    { "var_char",          CDS_CHAR,   TimeDim,        CharVarAtts,   5, CharData },
    { "var_byte",          CDS_BYTE,   TimeDim,        ByteVarAtts,   5, ByteData },
    { "var_short",         CDS_SHORT,  TimeDim,        ShortVarAtts,  5, ShortData },

    { "var_2D",            CDS_DOUBLE, TimeRangeDims,  Var2DAtts,     5, &DoubleData2D[100] },
    { NULL,                CDS_NAT,    NULL,           NULL,          0, NULL }
};

VarDef G2_Vars[] = {

    { "var_int_static",    CDS_INT,    StaticDim,      IntVarAtts,    5, IntData },
    { "var_float_static",  CDS_FLOAT,  StaticDim,      FloatVarAtts,  5, FloatData },
    { "var_double_static", CDS_DOUBLE, StaticDim,      DoubleVarAtts, 5, DoubleData },

    { "var_int",           CDS_INT,    TimeDim,        IntVarAtts,    5, IntData },
    { "var_float",         CDS_FLOAT,  TimeDim,        FloatVarAtts,  5, FloatData },
    { "var_double",        CDS_DOUBLE, TimeDim,        DoubleVarAtts, 5, DoubleData },

    { "var_char_2D",       CDS_CHAR,   TimeCharDims,   CharVar2DAtts, 5, &CharData2D[65] },
    { NULL,                CDS_NAT,    NULL,           NULL,          0, NULL }
};

VarDef RootVars[] = {

    { "time",              CDS_DOUBLE, TimeDim,        TimeVarAtts,   5, TimeData },
    { "range",             CDS_INT,    RangeDim,       RangeVarAtts, 20, RangeData },

    { "var_char_static",   CDS_CHAR,   StaticDim,      CharVarAtts,   5, CharData },
    { "var_byte_static",   CDS_BYTE,   StaticDim,      ByteVarAtts,   5, ByteData },
    { "var_short_static",  CDS_SHORT,  StaticDim,      ShortVarAtts,  5, ShortData },
    { "var_int_static",    CDS_INT,    StaticDim,      IntVarAtts,    5, IntData },
    { "var_float_static",  CDS_FLOAT,  StaticDim,      FloatVarAtts,  5, FloatData },
    { "var_double_static", CDS_DOUBLE, StaticDim,      DoubleVarAtts, 5, DoubleData },

    { "var_int64_static",  CDS_INT64,  StaticDim,      Int64VarAtts,  5, Int64Data },
    { "var_ubyte_static",  CDS_UBYTE,  StaticDim,      UByteVarAtts,  5, UByteData },
    { "var_ushort_static", CDS_USHORT, StaticDim,      UShortVarAtts, 5, UShortData },
    { "var_uint_static",   CDS_UINT,   StaticDim,      UIntVarAtts,   5, UIntData },
    { "var_uint64_static", CDS_UINT64, StaticDim,      UInt64VarAtts, 5, UInt64Data },
    { "var_string_static", CDS_STRING, StaticDim,      StringVarAtts, 5, StringData },

    { "var_char",          CDS_CHAR,   TimeDim,        CharVarAtts,   5, CharData },
    { "var_byte",          CDS_BYTE,   TimeDim,        ByteVarAtts,   5, ByteData },
    { "var_short",         CDS_SHORT,  TimeDim,        ShortVarAtts,  5, ShortData },
    { "var_int",           CDS_INT,    TimeDim,        IntVarAtts,    5, IntData },
    { "var_float",         CDS_FLOAT,  TimeDim,        FloatVarAtts,  5, FloatData },
    { "var_double",        CDS_DOUBLE, TimeDim,        DoubleVarAtts, 5, DoubleData },

    { "var_int64",         CDS_INT64,  TimeDim,        Int64VarAtts,  5, Int64Data },
    { "var_ubyte",         CDS_UBYTE,  TimeDim,        UByteVarAtts,  5, UByteData },
    { "var_ushort",        CDS_USHORT, TimeDim,        UShortVarAtts, 5, UShortData },
    { "var_uint",          CDS_UINT,   TimeDim,        UIntVarAtts,   5, UIntData },
    { "var_uint64",        CDS_UINT64, TimeDim,        UInt64VarAtts, 5, UInt64Data },
    { "var_string",        CDS_STRING, TimeDim,        StringVarAtts, 5, StringData },

    { "var_2D",            CDS_DOUBLE, TimeRangeDims,  Var2DAtts,     5, DoubleData2D },
    { "var_char_2D",       CDS_CHAR,   TimeCharDims,   CharVar2DAtts, 5, CharData2D },
    { NULL,                CDS_NAT,    NULL,           NULL,          0, NULL }
};

GroupDef G1_Groups[] = {

   { "group_1_1", NULL, SG1_Atts, SG1_Vars, NULL },

   { NULL, NULL, NULL, NULL, NULL }
};

GroupDef RootSubgroups[] = {

   { "group_1", G1_Dims, G1_Atts, G1_Vars, G1_Groups },
   { "group_2", G2_Dims, G2_Atts, G2_Vars, NULL },

   { NULL, NULL, NULL, NULL, NULL }
};

GroupDef RootDef[] = {

   { "root", RootDims, RootAtts, RootVars, RootSubgroups },

   { NULL, NULL, NULL, NULL, NULL }
};
