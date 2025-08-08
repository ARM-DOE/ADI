/*******************************************************************************
*
*  Copyright © 2014, Battelle Memorial Institute
*  All rights reserved.
*
*******************************************************************************/

/** @file dsproc_solar_position.c
 *  Functions to calculate position of the sun
 */

#include <math.h>
#include "dsproc3.h"
#include "dsproc3_internal.h"

/* Math definitions. */

#define PI      3.1415926535897932
#define TWOPI   6.2831853071795864
#define DEG_RAD 0.017453292519943295
#define RAD_DEG 57.295779513082323

/** @privatesection */
/*******************************************************************************
 *  Static Data and Functions Visible Only To This Module
 */

/* 'daynum()' returns the sequential daynumber of a calendar date during a
 *  Gregorian calendar year (for years 1 onward).
 *  The integer arguments are the four-digit year, the month number, and
 *  the day of month number.
 *  (Jan. 1 = 01/01 = 001; Dec. 31 = 12/31 = 365 or 366.)
 *  A value of -1 is returned if the year is out of bounds.
 */

/* Author: Nels Larson
 *         Pacific Northwest Lab.
 *         P.O. Box 999
 *         Richland, WA 99352
 *         U.S.A.
 */

static int _daynum(int year, int month, int day)
{
  static int begmonth[13] = {0,0,31,59,90,120,151,181,212,243,273,304,334};
  int dnum,
      leapyr = 0;

  /* There is no year 0 in the Gregorian calendar and the leap year cycle
   * changes for earlier years. */

  if (year < 1)
    return (-1);

  /* Leap years are divisible by 4, except for centurial years not divisible
   * by 400. */

  if (((year%4) == 0 && (year%100) != 0) || (year%400) == 0)
    leapyr = 1;

  dnum = begmonth[month] + day;
  if (leapyr && (month > 2))
    dnum += 1;

  return (dnum);
}

/**
 *  Calculate solar position.
 *
 *  <b>Author:</b> Nels Larson, PNNL
 *
 *  This function employs the low precision formulas for the Sun's coordinates
 *  given in the "Astronomical Almanac" of 1990 to compute the Sun's apparent
 *  right ascension, apparent declination, altitude, atmospheric refraction
 *  correction applicable to the altitude, azimuth, and distance from Earth.
 *  The "Astronomical Almanac" (A. A.) states a precision of 0.01 degree for the
 *  apparent coordinates between the years 1950 and 2050, and an accuracy of
 *  0.1 arc minute for refraction at altitudes of at least 15 degrees.
 *
 *  The following assumptions and simplifications are made:
 *    - refraction is calculated for standard atmosphere pressure and
 *      temperature at sea level.
 *    - diurnal parallax is ignored, resulting in 0 to 9 arc seconds error in
 *      apparent position.
 *    - diurnal aberration is also ignored, resulting in 0 to 0.02 second error
 *      in right ascension and 0 to 0.3 arc second error in declination.
 *    - geodetic site coordinates are used, without correction for polar motion
 *      (maximum amplitude of 0.3 arc second) and local gravity anomalies.
 *    - local mean sidereal time is substituted for local apparent sidereal time
 *      in computing the local hour angle of the Sun, resulting in an error of
 *      about 0 to 1 second of time as determined explicitly by the equation of
 *      the equinoxes.
 *
 *  Right ascension is measured in hours from 0 to 24, and declination in
 *  degrees from 90 to -90.
 *  Altitude is measured from 0 degrees at the horizon to 90 at the zenith or
 *  -90 at the nadir. Azimuth is measured from 0 to 360 degrees starting at
 *  north and increasing toward the east at 90.
 *  The refraction correction should be added to the altitude if Earth's
 *  atmosphere is to be accounted for.
 *  Solar distance from Earth is in astronomical units, 1 a.u. representing the
 *  mean value.
 *
 *  <b>Revision by:</b> Tim Shippert, PNNL
 *
 *  Added a smoother to refraction, so it varies linearly from it's value at
 *  alt == -1 to zero at alt == -2.  This eliminates the discontinuity at alt
 *  == -1, and might even be kinda scientifically justified.
 *
 *  @param year        - Four digit year (Gregorian calendar).
 *                         - 1950 through 2049
 *                         - 0 o.k. if using days_1900
 *
 *  @param month       - Month number.
 *                         - 1 through 12
 *                         - 0 o.k. if using daynumber for day
 *
 *  @param day         - Calendar day.fraction, or daynumber.fraction.
 *                         - if month is NOT 0:
 *                             - 0 through 32
 *                             - 31st @ 18:10:00 UT = 31.75694
 *                         - if month IS 0:
 *                             - 0 through 367
 *                             - 366 @ 18:10:00 UT = 366.75694
 *
 *  @param days_1900   - Days since 1900 January 0 @ 00:00:00 UT.
 *                         - 18262.0 (1950/01/00) through 54788.0 (2049/12/32)
 *                         - 1990/01/01 @ 18:10:00 UT = 32873.75694
 *                         - 0.0 o.k. if using {year, month, day}
 *                           or {year, daynumber}]
 *
 *  @param latitude    - Observation site geographic latitude.
 *                       [degrees.fraction, North positive]
 *
 *  @param longitude   - Observation site geographic longitude.
 *                       [degrees.fraction, East positive]
 *
 *  @param ap_ra       - output: Apparent solar right ascension.
 *                       [hours: 0.0 <= *ap_ra < 24.0]
 *
 *  @param ap_dec      - output: Apparent solar declination.
 *                       [degrees: -90.0 <= *ap_dec <= 90.0]
 *
 *  @param altitude    - output: Solar altitude, uncorrected for refraction.
 *                       [degrees: -90.0 <= *altitude <= 90.0]
 *
 *  @param refraction  - output: Refraction correction for solar altitude.
 *                       Add this to altitude to compensate for refraction.
 *                       [degrees: 0.0 <= *refraction]
 *
 *  @param azimuth     - output: Solar azimuth.
 *                       [degrees: 0.0 <= *azimuth < 360.0, East is 90.0]
 *
 *  @param distance    - output: Distance of Sun from Earth (heliocentric-geocentric).
 *                       [astronomical units: 1 a.u. is mean distance]
 *
 *  @return
 *    -  0 if successful
 *    - -1 if an input parameter is out of bounds
 */
static int solarposition(
        int     year,
        int     month,
        double  day,
        double  days_1900,
        double  latitude,
        double  longitude,
        double *ap_ra,
        double *ap_dec,
        double *altitude,
        double *refraction,
        double *azimuth,
        double *distance)
{
  int    daynumber,       /* Sequential daynumber during a year. */
         delta_days,      /* Whole days since 2000 January 0. */
         delta_years;     /* Whole years since 2000. */
  double cent_J2000,      /* Julian centuries since epoch J2000.0 at 0h UT. */
         cos_alt,         /* Cosine of the altitude of Sun. */
         cos_apdec,       /* Cosine of the apparent declination of Sun. */
         cos_az,          /* Cosine of the azimuth of Sun. */
         cos_lat,         /* Cosine of the site latitude. */
         cos_lha,         /* Cosine of the local apparent hour angle of Sun. */
         days_J2000,      /* Days since epoch J2000.0. */
         ecliptic_long,   /* Solar ecliptic longitude. */
         lmst,            /* Local mean sidereal time. */
         local_ha,        /* Local mean hour angle of Sun. */
         gmst0h,          /* Greenwich mean sidereal time at 0 hours UT. */
         integral,        /* Integral portion of double precision number. */
         mean_anomaly,    /* Earth mean anomaly. */
         mean_longitude,  /* Solar mean longitude. */
         mean_obliquity,  /* Mean obliquity of the ecliptic. */
         pressure =       /* Earth mean atmospheric pressure at sea level */
           1013.25,       /*   in millibars. */
         sin_apdec,       /* Sine of the apparent declination of Sun. */
         sin_az,          /* Sine of the azimuth of Sun. */
         sin_lat,         /* Sine of the site latitude. */
         tan_alt,         /* Tangent of the altitude of Sun. */
         temp =           /* Earth mean atmospheric temperature at sea level */
           15.0,          /*   in degrees Celsius. */
         ut;              /* UT hours since midnight. */


  /* Check latitude and longitude for proper range before calculating dates.
   */
  if (latitude < -90.0 || latitude > 90.0 ||
      longitude < -180.0 || longitude > 180.0)
    return (-1);

  /* If year is not zero then assume date is specified by year, month, day.
   * If year is zero then assume date is specified by days_1900.
   */
  if (year != 0)
  /* Date given by {year, month, day} or {year, 0, daynumber}. */
  {
    if (year < 1950 || year > 2049)
      return (-1);
    if (month != 0)
    {
      if (month < 1 || month > 12 || day < 0.0 || day > 32.0)
        return (-1);

      daynumber = _daynum(year, month, (int)day);
    }
    else
    {
      if (day < 0.0 || day > 367.0)
        return (-1);

      daynumber = (int)day;
    }

    /* Construct Julian centuries since J2000 at 0 hours UT of date,
     * days.fraction since J2000, and UT hours.
     */
    delta_years = year - 2000;
    /* delta_days is days from 2000/01/00 (1900's are negative). */
    delta_days = delta_years * 365 + delta_years / 4 + daynumber;
    if (year > 2000)
      delta_days += 1;
    /* J2000 is 2000/01/01.5 */
    days_J2000 = delta_days - 1.5;

    cent_J2000 = days_J2000 / 36525.0;

    ut = modf(day, &integral);
    days_J2000 += ut;
    ut *= 24.0;
  }
  else
  /* Date given by days_1900. */
  {
    /* days_1900 is 18262 for 1950/01/00, and 54788 for 2049/12/32.
     * A. A. 1990, K2-K4. */
    if (days_1900 < 18262.0 || days_1900 > 54788.0)
      return (-1);

    /* Construct days.fraction since J2000, UT hours, and
     * Julian centuries since J2000 at 0 hours UT of date.
     */
    /* days_1900 is 36524 for 2000/01/00. J2000 is 2000/01/01.5 */
    days_J2000 = days_1900 - 36525.5;

    ut = modf(days_1900, &integral) * 24.0;

    cent_J2000 = (integral - 36525.5) / 36525.0;
  }


  /* Compute solar position parameters.
   * A. A. 1990, C24.
   */

  mean_anomaly = (357.528 + 0.9856003 * days_J2000);
  mean_longitude = (280.460 + 0.9856474 * days_J2000);

  /* Put mean_anomaly and mean_longitude in the range 0 -> 2 pi. */
  mean_anomaly = modf(mean_anomaly / 360.0, &integral) * TWOPI;
  mean_longitude = modf(mean_longitude / 360.0, &integral) * TWOPI;

  mean_obliquity = (23.439 - 4.0e-7 * days_J2000) * DEG_RAD;
  ecliptic_long = ((1.915 * sin(mean_anomaly)) +
                   (0.020 * sin(2.0 * mean_anomaly))) * DEG_RAD +
                  mean_longitude;

  *distance = 1.00014 - 0.01671 * cos(mean_anomaly) -
              0.00014 * cos(2.0 * mean_anomaly);

  /* Tangent of ecliptic_long separated into sine and cosine parts for ap_ra. */
  *ap_ra = atan2(cos(mean_obliquity) * sin(ecliptic_long), cos(ecliptic_long));

  /* Change range of ap_ra from -pi -> pi to 0 -> 2 pi. */
  if (*ap_ra < 0.0)
    *ap_ra += TWOPI;
  /* Put ap_ra in the range 0 -> 24 hours. */
  *ap_ra = modf(*ap_ra / TWOPI, &integral) * 24.0;

  *ap_dec = asin(sin(mean_obliquity) * sin(ecliptic_long));

  /* Calculate local mean sidereal time.
   * A. A. 1990, B6-B7.
   */

  /* Horner's method of polynomial exponent expansion used for gmst0h. */
  gmst0h = 24110.54841 + cent_J2000 * (8640184.812866 + cent_J2000 *
            (0.093104 - cent_J2000 * 6.2e-6));
  /* Convert gmst0h from seconds to hours and put in the range 0 -> 24. */
  gmst0h = modf(gmst0h / 3600.0 / 24.0, &integral) * 24.0;
  if (gmst0h < 0.0)
    gmst0h += 24.0;

  /* Ratio of lengths of mean solar day to mean sidereal day is 1.00273790934
   * in 1990. Change in sidereal day length is < 0.001 second over a century.
   * A. A. 1990, B6.
   */
  lmst = gmst0h + (ut * 1.00273790934) + longitude / 15.0;
  /* Put lmst in the range 0 -> 24 hours. */
  lmst = modf(lmst / 24.0, &integral) * 24.0;
  if (lmst < 0.0)
    lmst += 24.0;

  /* Calculate local hour angle, altitude, azimuth, and refraction correction.
   * A. A. 1990, B61-B62.
   */

  local_ha = lmst - *ap_ra;
  /* Put hour angle in the range -12 to 12 hours. */
  if (local_ha < -12.0)
    local_ha += 24.0;
  else if (local_ha > 12.0)
    local_ha -= 24.0;

  /* Convert latitude and local_ha to radians. */
  latitude *= DEG_RAD;
  local_ha = local_ha / 24.0 * TWOPI;

  cos_apdec = cos(*ap_dec);
  sin_apdec = sin(*ap_dec);
  cos_lat = cos(latitude);
  sin_lat = sin(latitude);
  cos_lha = cos(local_ha);

  *altitude = asin(sin_apdec * sin_lat + cos_apdec * cos_lha * cos_lat);

  cos_alt = cos(*altitude);
  /* Avoid tangent overflow at altitudes of +-90 degrees.
   * 1.57079615 radians is equal to 89.99999 degrees.
   */
  if (fabs(*altitude) < 1.57079615)
    tan_alt = tan(*altitude);
  else
    tan_alt = 6.0e6;

  cos_az = (sin_apdec * cos_lat - cos_apdec * cos_lha * sin_lat) / cos_alt;
  sin_az = -(cos_apdec * sin(local_ha) / cos_alt);
  *azimuth = acos(cos_az);

  /* Change range of azimuth from 0 -> pi to 0 -> 2 pi. */
  if (atan2(sin_az, cos_az) < 0.0)
    *azimuth = TWOPI - *azimuth;

  /* Convert ap_dec, altitude, and azimuth to degrees. */
  *ap_dec *= RAD_DEG;
  *altitude *= RAD_DEG;
  *azimuth *= RAD_DEG;

  /* Compute refraction correction to be added to altitude to obtain actual
   * position.
   * Refraction calculated for altitudes of -1 degree or more allows for a
   * pressure of 1040 mb and temperature of -22 C. Lower pressure and higher
   * temperature combinations yield less than 1 degree refraction.
   * NOTE:
   * The two equations listed in the A. A. have a crossover altitude of
   * 19.225 degrees at standard temperature and pressure. This crossover point
   * is used instead of 15 degrees altitude so that refraction is smooth over
   * the entire range of altitudes. The maximum residual error introduced by
   * this smoothing is 3.6 arc seconds at 15 degrees. Temperature or pressure
   * other than standard will shift the crossover altitude and change the error.
   */

  /* We want to smooth out the transition for the refraction, rather */
  /* than having a discontinuity at zenith = 91 degrees.  Thus, we   */
  /* relax the refraction from it's value at alt == -1 to zero at   */
  /* alt == -2.  trs 3/4/03   */ 
  if (*altitude < -2 || tan_alt == 6.0e6)
    *refraction = 0.0;
  else if (*altitude < -1) 
  {
    /* 0.241277 * Pres/Temp is refraction at alt == -1 */
    /* (alt+2) goes linearly from 1 at alt == -1 to zero */
    /* at alt == -2  */
    *refraction = 0.241277 * (*altitude + 2) * pressure/(273.0 + temp);
  }
  else
  {
    if (*altitude < 19.225)
    {
      *refraction = (0.1594 + (*altitude) * (0.0196 + 0.00002 * (*altitude))) *
                    pressure;
      *refraction /= (1.0 + (*altitude) * (0.505 + 0.0845 * (*altitude))) *
                     (273.0 + temp);
    }
    else
      *refraction = 0.00452 * (pressure / (273.0 + temp)) / tan_alt;
  }

  return 0;
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/**
*  Calculate solar position.
*
*  Wrapper function for solarposition, allowing UTC time as input.
*
*  <b>Author:</b> Nels Larson, PNNL
*
*  This function employs the low precision formulas for the Sun's coordinates
*  given in the "Astronomical Almanac" of 1990 to compute the Sun's apparent
*  right ascension, apparent declination, altitude, atmospheric refraction
*  correction applicable to the altitude, azimuth, and distance from Earth.
*  The "Astronomical Almanac" (A. A.) states a precision of 0.01 degree for the
*  apparent coordinates between the years 1950 and 2050, and an accuracy of
*  0.1 arc minute for refraction at altitudes of at least 15 degrees.
*
*  The following assumptions and simplifications are made:
*    - refraction is calculated for standard atmosphere pressure and
*      temperature at sea level.
*    - diurnal parallax is ignored, resulting in 0 to 9 arc seconds error in
*      apparent position.
*    - diurnal aberration is also ignored, resulting in 0 to 0.02 second error
*      in right ascension and 0 to 0.3 arc second error in declination.
*    - geodetic site coordinates are used, without correction for polar motion
*      (maximum amplitude of 0.3 arc second) and local gravity anomalies.
*    - local mean sidereal time is substituted for local apparent sidereal time
*      in computing the local hour angle of the Sun, resulting in an error of
*      about 0 to 1 second of time as determined explicitly by the equation of
*      the equinoxes.
*
*  Right ascension is measured in hours from 0 to 24, and declination in
*  degrees from 90 to -90.
*
*  Altitude is measured from 0 degrees at the horizon to 90 at the zenith or
*  -90 at the nadir.
*
*  Azimuth is measured from 0 to 360 degrees starting at north and
*  increasing toward the east at 90.
*
*  The refraction correction should be added to the altitude if Earth's
*  atmosphere is to be accounted for.
*
*  Solar distance from Earth is in astronomical units, 1 a.u. representing the
*  mean value.
*
*  <b>Revision by:</b> Tim Shippert, PNNL
*
*  Added a smoother to refraction, so it varies linearly from it's value at
*  alt == -1 to zero at alt == -2.  This eliminates the discontinuity at alt
*  == -1, and might even be kinda scientifically justified.
*
*  @param  secs1970   - Seconds since 1970 UTC
*  @param  latitude   - Observation site geographic latitude. 
*                       [degrees.fraction, North positive]
*  @param  longitude  - Observation site geographic longitude. 
*                       [degrees.fraction, East positive]
*
*  @param  ap_ra      - output: Apparent solar right ascension.
*                       Argument can be NULL if the output value is not needed.
*                       [hours: 0.0 <= *ap_ra < 24.0]
*
*  @param  ap_dec     - output: Apparent solar declination.
*                       Argument can be NULL if the output value is not needed.
*                       [degrees: -90.0 <= *ap_dec <= 90.0]
*
*  @param  altitude   - output: Solar altitude, uncorrected for refraction.
*                       Argument can be NULL if the output value is not needed.
*                       [degrees: -90.0 <= *altitude <= 90.0]
*
*  @param  refraction - output: Refraction correction for solar altitude.
*                       Add this to altitude to compensate for refraction.
*                       Argument can be NULL if the output value is not needed.
*                       [degrees: 0.0 <= *refraction]
*
*  @param  azimuth    - output: Solar azimuth. 
*                       Argument can be NULL if the output value is not needed.
*                       [degrees: 0.0 <= *azimuth < 360.0, East is 90.0]
*
*  @param  distance   - output: Distance of Sun from Earth (heliocentric-geocentric).
*                       Argument can be NULL if the output value is not needed.
*                       [astronomical units: 1 a.u. is mean distance]
*
*  @return
*    -  1 if successful
*    -  0 if an input parameter is out of bounds
*/
int dsproc_solar_position(
        time_t secs1970, 
        double latitude, 
        double longitude,
        double *ap_ra,
        double *ap_dec,
        double *altitude,
        double *refraction,
        double *azimuth,
        double *distance)
{
    double day, day_fraction;
    int year, month, hour, min, sec;
    int status;
    struct tm *time;

    /* Possible outputs */
    double _ap_ra       = 0.0;
    double _ap_dec      = 0.0;
    double _altitude    = 0.0;
    double _refraction  = 0.0;
    double _azimuth     = 0.0;
    double _distance    = 0.0;

    /* Convert secs1970 into appropriate year, month, day */
    time  = gmtime(&secs1970);
    year  = time->tm_year + 1900;
    month = time->tm_mon + 1;
    day   = (double)time->tm_mday;
    hour  = time->tm_hour;
    min   = time->tm_min;
    sec   = time->tm_sec;

    /* Convert hour, minute, seconds into fraction of day */
    day_fraction = (double)hour/24.0
                 + (double)min/1440.0
                 + (double)sec/86400.0; 

    day = day + day_fraction;

    /* Calculate solar position */
    status = solarposition(
        year, month, day, 0.0, latitude, longitude,
        &_ap_ra, &_ap_dec, &_altitude, &_refraction, &_azimuth, &_distance);

    if (ap_ra)      *ap_ra      = _ap_ra;
    if (ap_dec)     *ap_dec     = _ap_dec;
    if (altitude)   *altitude   = _altitude;
    if (refraction) *refraction = _refraction;
    if (azimuth)    *azimuth    = _azimuth;
    if (distance)   *distance   = _distance;

    /* return 1 for success, 0 for bad input */
    if (status != 0) {
        return(0);
    }

    return(1);
}

/**
*  Calculate solar positions for an array of times.
*
*  See dsproc_solar_position() for description of outputs.
*
*  All output arguments can be NULL if the values are not needed.
*
*  The memory used by the output arrays are dynamically allocated
*  and must be freed by the calling process.
*
*  If an error occurs in this function it will be appended to the log and
*  error mail messages, and the process status will be set appropriately.
*
*  @param  ntimes     - Number of times
*  @param  times      - Array of times in seconds since 1970 UTC
*  @param  latitude   - Observation site geographic latitude. 
*                       [degrees.fraction, North positive]
*  @param  longitude  - Observation site geographic longitude. 
*                       [degrees.fraction, East positive]
*  @param  ap_ra      - output: pointer to array of apparent solar right ascensions.
*  @param  ap_dec     - output: pointer to array of apparent solar declinations.
*  @param  altitude   - output: pointer to array of solar altitudes.
*  @param  refraction - output: pointer to array of refraction corrections.
*  @param  azimuth    - output: pointer to array of solar azimuths. 
*  @param  distance   - output: pointer to array of distances of Sun from Earth.
*
*  @return
*    -  1 if successful
*    -  0 if an input parameter is out of bounds,
*         or a memory allocation error occurs
*/
int dsproc_solar_positions(
        size_t   ntimes,
        time_t  *times,
        double   latitude, 
        double   longitude,
        double **ap_ra,
        double **ap_dec,
        double **altitude,
        double **refraction,
        double **azimuth,
        double **distance)
{
    double    day, day_fraction;
    int       year, month, hour, min, sec;
    struct tm gmt;
    double    _ap_ra, _ap_dec, _altitude, _refraction, _azimuth, _distance;
    size_t    alloc_size;
    int       status;
    size_t    oi, ti;

    double **outpp;
    double **outputs[] = {
        ap_ra, ap_dec, altitude, refraction, azimuth, distance};

    /* Initialize output pointers to NULL */

    for (oi = 0; oi < 6; ++oi) {
        outpp = outputs[oi];
        if (outpp) {
            *outpp = (double *)NULL;
        }
    }

    /* Allocate memory for outout arrays */

    alloc_size = ntimes * sizeof(double);

    for (oi = 0; oi < 6; ++oi) {
        outpp = outputs[oi];
        if (outpp) {
             *outpp = (double *)malloc(alloc_size);
             if (!*outpp) goto ERROR_EXIT;
        }
    }

    for (ti = 0; ti < ntimes; ++ti) {

        /* Convert secs1970 into appropriate year, month, day */

        gmtime_r(&times[ti], &gmt);
        year  = gmt.tm_year + 1900;
        month = gmt.tm_mon + 1;
        day   = (double)gmt.tm_mday;
        hour  = gmt.tm_hour;
        min   = gmt.tm_min;
        sec   = gmt.tm_sec;

        /* Convert hour, minute, seconds into fraction of day */
        day_fraction = (double)hour/24.0
                     + (double)min/1440.0
                     + (double)sec/86400.0; 

        day = day + day_fraction;

        /* Calculate solar position */

        _ap_ra    = _ap_dec     = 0.0;
        _altitude = _refraction = 0.0;
        _azimuth  = _distance   = 0.0;

        status = solarposition(
            year, month, day, 0.0, latitude, longitude,
            &_ap_ra, &_ap_dec, &_altitude, &_refraction, &_azimuth, &_distance);

        if (status != 0) {
            goto ERROR_EXIT;
        }

        if (ap_ra)      (*ap_ra)[ti]      = _ap_ra;
        if (ap_dec)     (*ap_dec)[ti]     = _ap_dec;
        if (altitude)   (*altitude)[ti]   = _altitude;
        if (refraction) (*refraction)[ti] = _refraction;
        if (azimuth)    (*azimuth)[ti]    = _azimuth;
        if (distance)   (*distance)[ti]   = _distance;
    }

    return(1);

ERROR_EXIT:

    for (oi = 0; oi < 6; ++oi) {
        outpp = outputs[oi];
        if (outpp && *outpp) {
            free(*outpp);
            *outpp = (double *)NULL;
        }
    }

    return(0);
}
