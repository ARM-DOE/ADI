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

/** @file cds_units_map.h
 *  Private CDS Units Map.
 */

#ifndef _CDS_UNITS_MAP_
#define _CDS_UNITS_MAP_

typedef struct CDS_UNITS_MAP {
    const char *bad;
    const char *good;
} CDS_UNITS_MAP;

static CDS_UNITS_MAP CDSBadUnitsMap[] = {
    { " ",                                          "1" },
    { "  G = < 5 degrees, B = > 5 degrees",         "1" },
    { "  good= R  or  T , bad= X",                  "1" },
    { "  good= p ,bad= F  or  S ,unknown= -",       "1" },
    { "  temp=T, wind=W, both=B ",                  "1" },
    { " good= p ,bad= B ,unknown= -",               "1" },
    { "#/cm^3",                                     "1/cm^3" },
    { "#/m^3",                                      "1/m^3" },
    { "% ",                                         "%" },
    { "% RH",                                       "%" },
    { "% of full scale; -1 indicates amp=0, data not included in ave.", "%" },
    { "%RH",                                        "%" },
    { "%ss",                                        "1" },
    { "(0 - land, 1 - sea)",                        "1" },
    { "(0 - no, 1 - yes)",                          "1" },
    { "(0-1)",                                      "1" },
    { "(DN/ms)/(W/m^2-nm)",                         "(count/ms)/(W/m^2/nm)" },
    { "(W/m^2-nm)/(DN/ms)",                         "(W/m^2/nm)/(count/ms)" },
    { "(km-ster) ** -1",                            "1/(km sr)" },
    { "(mw/(m2 sr cm-1))^2",                        "(mW/(m2 sr cm-1))^2" },
    { "(ratio)",                                    "1" },
    { "(ster km)^(-1)",                             "1/(sr km)" },
    { "(ster km)^-1",                               "1/(sr km)" },
    { "-",                                          "1" },
    { "/1",                                         "1" },
    { "0 = no clutter, 1 = clutter",                "1" },
    { "0 or 1",                                     "1" },
    { "0 or 1 or 2",                                "1" },
    { "0-23",                                       "hour" },
    { "0-59",                                       "min" },
    { "0=clear with snow/ice, 1=water, 2=ice, 3=no retrieval, 4=clear, 5=bad retrieval, 6=weak water, 7=weak ice", "1" },
    { "1 digit flags packed into a long integer",   "1" },
    { "1/(km-ster)",                                "1/(sr km)" },
    { "1/(srad km)",                                "1/(sr km)" },
    { "1/(srad*km*10000)",                          "1/(sr km 10000)" },
    { "1/(srad*km*10000.0)",                        "1/(sr km 10000)" },
    { "1/(ster km)",                                "1/(sr km)" },
    { "1/srad",                                     "1/sr" },
    { "10 * log(signal power in milliwatts)",       "10 lg(re 1 mW)" },
    { "100 * (% of niquist vel)",                   "m/s" },
    { "100* (% of niquist vel)",                    "m/s" },
    { "16 bit field packed into floating point number. Each bit represents the result from a system test where value 1 is 'PASS' or 'TRUE' and 0 is 'FAIL' or 'FALSE'. ", "1" },
    { "32-bit words",                               "1" },
    { "A.U.",                                       "au" },
    { "ASCII",                                      "1" },
    { "AU",                                         "au" },
    { "Astronomical Units",                         "au" },
    { "Boolean with negative error conditions: 1:Open 0:Closed -1:Fault -2:Outside Valid Range -3:Neither Open Nor Closed", "1" },
    { "Counts/[mw/(m2 sr cm-1)]",                   "count(mW/(m^2 sr cm^-1))" },
    { "DEGaz",                                      "degree" },
    { "Date and Time in format = yyyymmddhhmmss",   "1" },
    { "Day fraction offset from 00:00 on this day", "day" },
    { "Deg",                                        "degree" },
    { "Deg C",                                      "degC" },
    { "Deg relative to true north",                 "degree_true" },
    { "Degrees C * 100",                            "100 degC" },
    { "Degrees Celcius",                            "degC" },
    { "Degrees East(-180 to 180)",                  "degree_east" },
    { "Degrees North(-90 to 90)",                   "degree_north" },
    { "Fraction",                                   "1" },
    { "HHMMSS",                                     "1" },
    { "HHMMSS.SS (hours, minutes, seconds)",        "1" },
    { "Hours from 00:00 UTC",                       "hour" },
    { "Integer code",                               "1" },
    { "Irradiance (W/m^2/nm)",                      "(W/m^2/nm)" },
    { "J/Kg",                                       "J/kg" },
    { "Kohms",                                      "kohms" },
    { "Kpa",                                        "kPa" },
    { "LPM",                                        "L/min" },
    { "MM/HR",                                      "mm/hr" },
    { "Mw/volt",                                    "mW/V" },
    { "None",                                       "1" },
    { "None.",                                      "1" },
    { "PSU",                                        "g/kg" },
    { "Packed Binary",                              "1" },
    { "Packer Binary",                              "1" },
    { "Percent(x100)",                              "(1/100) %" },
    { "Phe counts * km^2 / (us * mJ)",              "count km^2 / (us mJ)" },
    { "Phe counts / us",                            "count / us" },
    { "RH",                                         "%" },
    { "Ratio",                                      "1" },
    { "SCCM",                                       "cc/min" },
    { "SLPM",                                       "L/min" },
    { "Type of scene that has been calibrated (ASCII character as float)", "1" },
    { "UTC Hours",                                  "hour" },
    { "Unitless",                                   "1" },
    { "V (DC)",                                     "V" },
    { "W m{-2}",                                    "W m^-2" },
    { "W/M**2",                                     "W m^-2" },
    { "Watts/square meter",                         "W m^-2" },
    { "YYMMDD",                                     "1" },
    { "YYYYMMDD [UTC]",                             "1" },
    { "a.u.",                                       "au" },
    { "amu/z",                                      "amu" },
    { "angular degrees",                            "degree" },
    { "arbitrary",                                  "1" },
    { "chars",                                      "1" },
    { "cm^(-3)",                                    "cm^-3" },
    { "code",                                       "1" },
    { "counts per bin",                             "count" },
    { "counts/[mW/(m^2 sr cm^-1)]",                 "count(mW/(m^2 sr cm^-1))" },
    { "dBZ (X100)",                                 "(1/100) dBZ" },
    { "dBz",                                        "dBZ" },
    { "date",                                       "1" },
    { "day of month",                               "day" },
    { "days since last day of the previous year",   "day" },
    { "dbm",                                        "dBm" },
    { "dbm per degree",                             "dBm/degree" },
    { "dbm/degree",                                 "dBm/degree" },
    { "dd:mm:yyyy, hh:mm:ss",                       "1" },
    { "deg",                                        "degree" },
    { "deg C",                                      "degC" },
    { "deg E",                                      "degree_east" },
    { "deg E of N",                                 "degree" },
    { "deg F",                                      "degF" },
    { "deg K",                                      "K" },
    { "deg N",                                      "degree_north" },
    { "deg W",                                      "degree_west" },
    { "deg east",                                   "degree_east" },
    { "deg north",                                  "degree_north" },
    { "deg/day",                                    "degree / day" },
    { "deg/dy",                                     "degree / day" },
    { "deg/km",                                     "degree / km" },
    { "deg/s",                                      "degree / s" },
    { "degApp",                                     "degree" },
    { "degRel",                                     "degree" },
    { "degTrue",                                    "degree_true" },
    { "degree_per_km",                              "degree / km" },
    { "degrees C.",                                 "degC" },
    { "degrees east",                               "degree_east" },
    { "degrees north",                              "degree_north" },
    { "degrees true",                               "degree_true" },
    { "degrees_from_north",                         "degree" },
    { "degrees_from_vertical",                      "degree" },
    { "degrees_per_km",                             "degree / km" },
    { "degrees_south",                              "-1 degree_north" },
    { "dimensionless",                              "1" },
    { "flag",                                       "1" },
    { "frac",                                       "1" },
    { "fraction",                                   "1" },
    { "fraction.",                                  "1" },
    { "fractional days",                            "day" },
    { "fractional seconds",                         "seconds" },
    { "g/Kg",                                       "g/kg" },
    { "gHz",                                        "GHz" },
    { "gm/m^3",                                     "g/m^3" },
    { "good= p ,bad= H  or  L ,unknown= -",         "1" },
    { "good= p ,bad= W  or  C ,unknown= -",         "1" },
    { "gpm",                                        "m" },
    { "hhmmss",                                     "1" },
    { "hit/(hr cm^2)",                              "count/(hour cm^2)" },
    { "hit/cm^2",                                   "count/cm^2" },
    { "hour [UTC]",                                 "hour" },
    { "hour of day",                                "hour" },
    { "hour of the day",                            "hour" },
    { "hours since 1999-Jan-1 00:00:00.00, GMT",    "hours since 1999-01-01 00:00:00 0:00" },
    { "hours since 2010-02-19 00:00:00 0:00 GMT",   "hours since 2010-02-19 00:00:00 0:00" },
    { "hours since 2010-07-09 00:00:00 0:00 GMT",   "hours since 2010-07-09 00:00:00 0:00" },
    { "hours since midnight of the day",            "hours" },
    { "human-readable max date/time",               "1" },
    { "human-readable min date/time",               "1" },
    { "index",                                      "1" },
    { "index ",                                     "1" },
    { "integer flag",                               "1" },
    { "jan=1, feb=2, etc",                          "month" },
    { "jan=1,feb=2,etc.",                           "month" },
    { "k/hour",                                     "K/hour" },
    { "kPa ",                                       "kPa" },
    { "kg/(m2*s) = mm/s",                           "kg/(m^2 s)" },
    { "kg/kg * kg/(m2*s))",                         "(kg/kg) kg/(m^2 s)" },
    { "kilometers AGL",                             "km" },
    { "kilometers above sea level",                 "km" },
    { "km ** -1",                                   "km^-1" },
    { "km AGL",                                     "km" },
    { "km Above Ground Level (AGL)",                "km" },
    { "km above MSL",                               "km" },
    { "kmASL",                                      "km" },
    { "km^(-1)",                                    "km^-1" },
    { "level",                                      "1" },
    { "levels",                                     "l" },
    { "log(W/(m^2 nm))",                            "ln(re W/(m^2 nm))" },
    { "logcounts",                                  "1" },
    { "lpm",                                        "L/min" },
    { "m (of water equivalent)",                    "m" },
    { "m (of water)",                               "m" },
    { "m > MSL",                                    "m" },
    { "m A.M.S.",                                   "m" },
    { "m AGL",                                      "m" },
    { "m ASL",                                      "m" },
    { "m MSL",                                      "m" },
    { "m above MSL",                                "m" },
    { "m above ground level",                       "m" },
    { "m msl",                                      "m" },
    { "m of water equivalent",                      "m" },
    { "m, AGL",                                     "m" },
    { "m/s (X1000)",                                "(1/1000) m/s" },
    { "m/s; 100*%, see comment",                    "m/s" },
    { "m/z",                                        "amu" },
    { "mASL",                                       "m" },
    { "mB",                                         "hPa" },
    { "meter (pressure altitude, msl)",             "m" },
    { "meters A.M.S.",                              "m" },
    { "meters AGL",                                 "m" },
    { "meters above Mean Sea Level",                "m" },
    { "meters above ground level",                  "m" },
    { "meters above mean sea level",                "m" },
    { "meters above sea level",                     "m" },
    { "meters_per_second",                          "m/s" },
    { "microWatt/(cm^2 * nm * ster)",               "uW/(cm^2 nm sr)" },
    { "microjoules per pulse",                      "uJ" },
    { "minute of hour",                             "min" },
    { "mixed units",                                "1" },
    { "mixed units -- see comments below",          "1" },
    { "mixed units -- see field arb above",         "1" },
    { "mixed units -- see obs_flag field above",    "1" },
    { "mmHG",                                       "mmHg" },
    { "month of year",                              "month" },
    { "motor steps",                                "1" },
    { "msec since 0:00 GMT",                        "ms" },
    { "mv",                                         "mV" },
    { "mw/(m2 sr cm-1)",                            "mW/(m^2 sr cm^-1)" },
    { "mw/(m^2 sr cm^-1)",                          "mW/(m^2 sr cm^-1)" },
    { "n/a",                                        "1" },
    { "non-dim",                                    "1" },
    { "none",                                       "1" },
    { "number of samples",                          "count" },
    { "numeric",                                    "1" },
    { "parameter dependent",                        "1" },
    { "parameters dependent",                       "1" },
    { "parts/cc",                                   "1/cm^3" },
    { "percent cloudy",                             "%" },
    { "percentage",                                 "%" },
    { "pixels",                                     "count" },
    { "psia",                                       "lb/in^2" },
    { "r for raw data, i for interpolated",         "1" },
    { "r,l,T,f,or t",                               "1" },
    { "ratio",                                      "1" },
    { "s since 1970/01/01 00:00:00 UTC",            "seconds since 1970-01-01 00:00:00 UTC" },
    { "scaled counts",                              "count" },
    { "sccm",                                       "cc/min" },
    { "sec_UTC",                                    "s" },
    { "second of minute",                           "s" },
    { "seconds since 1970-0101 00:00 UTC",          "seconds since 1970-01-01 00:00:00 UTC" },
    { "seconds since 1970/01/01 00:00:00.00",       "seconds since 1970-01-01 00:00:00 UTC" },
    { "seconds since January 1, 1970",              "seconds since 1970-01-01 00:00:00 UTC" },
    { "slpm",                                       "L/min" },
    { "steps",                                      "1" },
    { "ster",                                       "sr" },
    { "ster km ** -1",                              "sr/km" },
    { "ster km(-1)",                                "sr/km" },
    { "ster km^(-1)",                               "sr/km" },
    { "string format = dd:mm:yyyy, hh:mm:ss",       "1" },
    { "tenths of mm",                               "mm/10" },
    { "tips, (.01 inch)",                           "in/100" },
    { "uJ * 1000",                                  "uJ 1000" },
    { "umo/mol",                                    "umol/mol" },
    { "unimararsclwacrbnd1kolliasshipcorM1.c1.20171101.230001.nctless", "1" },
    { "uniteless",                                  "1" },
    { "unitles",                                    "1" },
    { "unitless",                                   "1" },
    { "unitless.",                                  "1" },
    { "units depends on the index; see comments below",  "1" },
    { "units depends on the index; see the field above", "1" },
    { "vokt",                                       "V" },
    { "w/m**2",                                     "W/m^2" },
    { "w/m^2",                                      "W/m^2" },
    { "watts per square meter",                     "W/m^2" },
    { "yymmdd",                                     "1" },
    { "yymmdd.hhmmss",                              "1" },
    { "yyyydddhhmmss",                              "1" },
    { "yyyymmdd",                                   "1" },
    { NULL,                                         NULL }
};

#endif // CDS_UNITS_MAP