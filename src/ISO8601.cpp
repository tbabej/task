////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2006 - 2015, Paul Beckingham, Federico Hernandez.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// http://www.opensource.org/licenses/mit-license.php
//
////////////////////////////////////////////////////////////////////////////////

#include <cmake.h>
#include <sstream>
#include <stdlib.h>
#include <Lexer.h>
#include <ISO8601.h>
#include <Date.h>

#define DAY    86400
#define HOUR    3600
#define MINUTE    60
#define SECOND     1

static struct
{
  std::string unit;
  int seconds;
  bool standalone;
} durations[] =
{
  // These are sorted by first character, then length, so that Nibbler::getOneOf
  // returns a maximal match.
  {"annual",     365 * DAY,    true},
  {"biannual",   730 * DAY,    true},
  {"bimonthly",   61 * DAY,    true},
  {"biweekly",    14 * DAY,    true},
  {"biyearly",   730 * DAY,    true},
  {"daily",        1 * DAY,    true},
  {"days",         1 * DAY,    false},
  {"day",          1 * DAY,    true},
  {"d",            1 * DAY,    false},
  {"fortnight",   14 * DAY,    true},
  {"hours",        1 * HOUR,   false},
  {"hour",         1 * HOUR,   true},
  {"hrs",          1 * HOUR,   false},
  {"hr",           1 * HOUR,   true},
  {"h",            1 * HOUR,   false},
  {"minutes",      1 * MINUTE, false},
  {"minute",       1 * MINUTE, true},
  {"mins",         1 * MINUTE, false},
  {"min",          1 * MINUTE, true},
  {"monthly",     30 * DAY,    true},
  {"months",      30 * DAY,    false},
  {"month",       30 * DAY,    true},
  {"mnths",       30 * DAY,    false},
  {"mths",        30 * DAY,    false},
  {"mth",         30 * DAY,    true},
  {"mos",         30 * DAY,    false},
  {"mo",          30 * DAY,    true},
  {"m",           30 * DAY,    false},
  {"quarterly",   91 * DAY,    true},
  {"quarters",    91 * DAY,    false},
  {"quarter",     91 * DAY,    true},
  {"qrtrs",       91 * DAY,    false},
  {"qrtr",        91 * DAY,    true},
  {"qtrs",        91 * DAY,    false},
  {"qtr",         91 * DAY,    true},
  {"q",           91 * DAY,    false},
  {"semiannual", 183 * DAY,    true},
  {"sennight",    14 * DAY,    false},
  {"seconds",      1 * SECOND, false},
  {"second",       1 * SECOND, true},
  {"secs",         1 * SECOND, false},
  {"sec",          1 * SECOND, true},
  {"s",            1 * SECOND, false},
  {"weekdays",     1 * DAY,    true},
  {"weekly",       7 * DAY,    true},
  {"weeks",        7 * DAY,    false},
  {"week",         7 * DAY,    true},
  {"wks",          7 * DAY,    false},
  {"wk",           7 * DAY,    true},
  {"w",            7 * DAY,    false},
  {"yearly",     365 * DAY,    true},
  {"years",      365 * DAY,    false},
  {"year",       365 * DAY,    true},
  {"yrs",        365 * DAY,    false},
  {"yr",         365 * DAY,    true},
  {"y",          365 * DAY,    false},
};

#define NUM_DURATIONS (sizeof (durations) / sizeof (durations[0]))

////////////////////////////////////////////////////////////////////////////////
ISO8601d::ISO8601d ()
{
  clear ();
}

////////////////////////////////////////////////////////////////////////////////
ISO8601d::~ISO8601d ()
{
}

////////////////////////////////////////////////////////////////////////////////
ISO8601d::operator time_t () const
{
  return _value;
}

////////////////////////////////////////////////////////////////////////////////
// Supported:
//
//    result       ::= date 'T' time 'Z'                      # UTC
//                   | date 'T' time                          # Local
//                   | date-ext 'T' time-ext 'Z'              # UTC
//                   | date-ext 'T' time-ext offset-ext       # Specified TZ
//                   | date-ext 'T' time-ext                  # Local
//                   | date-ext                               # Local
//                   | time-ext 'Z'
//                   | time-ext offset-ext            Not needed
//                   | time-ext
//                   ;
//
//    date-ext     ::= ±YYYYY-MM-DD                   Νot needed
//                   | ±YYYYY-Www-D                   Νot needed
//                   | ±YYYYY-Www                     Νot needed
//                   | ±YYYYY-DDD                     Νot needed
//                   | YYYY-MM-DD
//                   | YYYY-DDD
//                   | YYYY-Www-D
//                   | YYYY-Www
//                   ;
//
//    time-ext     ::= hh:mm:ss[,ss]
//                   | hh:mm[,mm]
//                   | hh[,hh]                        Ambiguous (number)
//                   ;
//
//    time-utc-ext ::= hh:mm[:ss] 'Z' ;
//
//    offset-ext   ::= ±hh[:mm] ;
//
// Not yet supported:
//
//    recurrence ::=
//                 | 'R' [n] '/' designated '/' datetime-ext          # duration end
//                 | 'R' [n] '/' designated '/' datetime              # duration end
//                 | 'R' [n] '/' designated                           # duration
//                 | 'R' [n] '/' datetime-ext '/' designated          # start duration
//                 | 'R' [n] '/' datetime-ext '/' datetime-ext        # start end
//                 | 'R' [n] '/' datetime '/' designated              # start duration
//                 | 'R' [n] '/' datetime '/' datetime                # start end
//                 ;
//
bool ISO8601d::parse (const std::string& input, std::string::size_type& start)
{
  auto i = start;
  Nibbler n (input.substr (i));

  if (parse_date_time     (n)             ||   // Strictest first.
      parse_date_time_ext (n)             ||
      parse_date_ext      (n)             ||
      parse_time_utc_ext  (n)             ||
      parse_time_off_ext  (n)             ||
      parse_time_ext      (n))                 // Time last, as it is the most permissive.
  {
    // Check the values and determine time_t.
    if (validate ())
    {
      // Record cursor position.
      start = n.cursor ();

      resolve ();
      return true;
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
void ISO8601d::clear ()
{
  _year            = 0;
  _month           = 0;
  _week            = 0;
  _weekday         = 0;
  _julian          = 0;
  _day             = 0;
  _seconds         = 0;
  _offset          = 0;
  _utc             = false;
  _value           = 0;
}

////////////////////////////////////////////////////////////////////////////////
bool ISO8601d::parse_date_time (Nibbler& n)
{
  n.save ();
  int year, month, day, hour, minute, second;
  if (n.getDigit4 (year)   &&
      n.getDigit2 (month)  && month &&
      n.getDigit2 (day)    && day   &&
      n.skip      ('T')    &&
      n.getDigit2 (hour)   &&
      n.getDigit2 (minute) && minute < 60 &&
      n.getDigit2 (second) && second < 60)
  {
    if (n.skip ('Z'))
      _utc = true;

    _year    = year;
    _month   = month;
    _day     = day;
    _seconds = (((hour * 60) + minute) * 60) + second;

    return true;
  }

  _year    = 0;
  _month   = 0;
  _day     = 0;
  _seconds = 0;

  n.restore ();
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// date-ext 'T' time-ext 'Z'
// date-ext 'T' time-ext offset-ext
// date-ext 'T' time-ext
bool ISO8601d::parse_date_time_ext (Nibbler& n)
{
  n.save ();
  if (parse_date_ext (n))
  {
    if (n.skip ('T') &&
        parse_time_ext (n))
    {
      if (n.skip ('Z'))
        _utc = true;
      else if (parse_off_ext (n))
        ;

      if (! Lexer::isDigit (n.next ()))
        return true;
    }

    // Restore date_ext
    _year    = 0;
    _month   = 0;
    _week    = 0;
    _weekday = 0;
    _julian  = 0;
    _day     = 0;
  }

  n.restore ();
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// YYYY-MM-DD
// YYYY-DDD
// YYYY-Www-D
// YYYY-Www
bool ISO8601d::parse_date_ext (Nibbler& n)
{
  Nibbler backup (n);
  int year;
  if (n.getDigit4 (year) &&
      n.skip ('-'))
  {
    int month;
    int day;
    if (n.skip ('W') &&
        n.getDigit2 (_week) && _week)
    {
      if (n.skip ('-') &&
          n.getDigit (_weekday))
      {
      }

      _year = year;
      if (!Lexer::isDigit (n.next ()))
        return true;
    }
    else if (n.getDigit3 (_julian) && _julian)
    {
      _year = year;
      if (!Lexer::isDigit (n.next ()))
        return true;
    }
    else if (n.getDigit2 (month) && month &&
             n.skip ('-')        &&
             n.getDigit2 (day)   && day)
    {
      _year = year;
      _month = month;
      _day = day;
      if (!Lexer::isDigit (n.next ()))
        return true;
    }
  }

  n = backup;
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// ±hh[:mm]
bool ISO8601d::parse_off_ext (Nibbler& n)
{
  Nibbler backup (n);
  std::string sign;
  if (n.getN (1, sign) && (sign == "+" || sign == "-"))
  {
    int offset;
    int hh;
    int mm;
    if (n.getDigit2 (hh) && hh <= 12 &&
        !n.getDigit (mm))
    {
      offset = hh * 3600;

      if (n.skip (':'))
      {
        if (n.getDigit2 (mm) && mm < 60)
        {
          offset += mm * 60;
        }
        else
        {
          n = backup;
          return false;
        }
      }

      _offset = (sign == "-") ? -offset : offset;
      if (!Lexer::isDigit (n.next ()))
        return true;
    }
  }

  n = backup;
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// hh:mm[:ss]
bool ISO8601d::parse_time_ext (Nibbler& n)
{
  Nibbler backup (n);
  int seconds = 0;
  int hh;
  int mm;
  int ss;
  if (n.getDigit2 (hh) && hh <= 24 &&
      n.skip (':')     &&
      n.getDigit2 (mm) && mm < 60)
  {
    seconds = (hh * 3600) + (mm * 60);

    if (n.skip (':'))
    {
      if (n.getDigit2 (ss) && ss < 60)
      {
        seconds += ss;
        _seconds = seconds;

        if (!Lexer::isDigit (n.next ()))
          return true;
      }

      n = backup;
      return false;
    }

    _seconds = seconds;
    if (!Lexer::isDigit (n.next ()))
      return true;
  }

  n = backup;
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// time-ext 'Z'
bool ISO8601d::parse_time_utc_ext (Nibbler& n)
{
  n.save ();
  if (parse_time_ext (n) &&
      n.skip ('Z'))
  {
    _utc = true;
    if (!Lexer::isDigit (n.next ()))
      return true;
  }

  n.restore ();
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// time-ext offset-ext
bool ISO8601d::parse_time_off_ext  (Nibbler& n)
{
  Nibbler backup (n);
  if (parse_time_ext (n) &&
      parse_off_ext (n))
  {
    if (!Lexer::isDigit (n.next ()))
      return true;
  }

  n = backup;
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// Using Zeller's Congruence.
int ISO8601d::dayOfWeek (int year, int month, int day)
{
  int adj = (14 - month) / 12;
  int m = month + 12 * adj - 2;
  int y = year - adj;
  return (day + (13 * m - 1) / 5 + y + y / 4 - y / 100 + y / 400) % 7;
}

////////////////////////////////////////////////////////////////////////////////
// Validation via simple range checking.
bool ISO8601d::validate ()
{
  // _year;
  if ((_year    && (_year    <   1900 || _year    >                              2200)) ||
      (_month   && (_month   <      1 || _month   >                                12)) ||
      (_week    && (_week    <      1 || _week    >                                53)) ||
      (_weekday && (_weekday <      0 || _weekday >                                 6)) ||
      (_julian  && (_julian  <      1 || _julian  >          Date::daysInYear (_year))) ||
      (_day     && (_day     <      1 || _day     > Date::daysInMonth (_month, _year))) ||
      (_seconds && (_seconds <      1 || _seconds >                             86400)) ||
      (_offset  && (_offset  < -86400 || _offset  >                             86400)))
    return false;

  return true;
}

////////////////////////////////////////////////////////////////////////////////
// int tm_sec;       seconds (0 - 60)
// int tm_min;       minutes (0 - 59)
// int tm_hour;      hours (0 - 23)
// int tm_mday;      day of month (1 - 31)
// int tm_mon;       month of year (0 - 11)
// int tm_year;      year - 1900
// int tm_wday;      day of week (Sunday = 0)
// int tm_yday;      day of year (0 - 365)
// int tm_isdst;     is summer time in effect?
// char *tm_zone;    abbreviation of timezone name
// long tm_gmtoff;   offset from UTC in seconds
void ISO8601d::resolve ()
{
  // Don't touch the original values.
  int year    = _year;
  int month   = _month;
  int week    = _week;
  int weekday = _weekday;
  int julian  = _julian;
  int day     = _day;
  int seconds = _seconds;
  int offset  = _offset;
  bool utc    = _utc;

  // Get current time.
  time_t now = time (NULL);

  // A UTC offset needs to be accommodated.  Once the offset is subtracted,
  // only local and UTC times remain.
  if (offset)
  {
    seconds -= offset;
    now -= offset;
    utc = true;
  }

  // Get 'now' in the relevant location.
  struct tm* t_now = utc ? gmtime (&now) : localtime (&now);

  int seconds_now = (t_now->tm_hour * 3600) +
                    (t_now->tm_min  *   60) +
                     t_now->tm_sec;

  // Project forward one day if the specified seconds are earlier in the day
  // than the current seconds.
  if (year    == 0 &&
      month   == 0 &&
      day     == 0 &&
      week    == 0 &&
      weekday == 0 &&
      seconds < seconds_now)
  {
    seconds += 86400;
  }

  // Convert week + weekday --> julian.
  if (week)
  {
    julian = (week * 7) + weekday - dayOfWeek (year, 1, 4) - 3;
  }

  // Provide default values for year, month, day.
  else
  {
    // Default values for year, month, day:
    //
    // y   m   d  -->  y   m   d
    // y   m   -  -->  y   m   1
    // y   -   -  -->  y   1   1
    // -   -   -  -->  now now now
    //
    if (year == 0)
    {
      year  = t_now->tm_year + 1900;
      month = t_now->tm_mon + 1;
      day   = t_now->tm_mday;
    }
    else
    {
      if (month == 0)
      {
        month = 1;
        day   = 1;
      }
      else if (day == 0)
        day = 1;
    }
  }

  if (julian)
  {
    month = 1;
    day = julian;
  }

  struct tm t = {0};
  t.tm_isdst = -1;  // Requests that mktime/gmtime determine summer time effect.
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;

  if (seconds > 86400)
  {
    int days = seconds / 86400;
    t.tm_mday += days;
    seconds %= 86400;
  }

    t.tm_hour = seconds / 3600;
    t.tm_min = (seconds % 3600) / 60;
    t.tm_sec = seconds % 60;

  _value = utc ? timegm (&t) : mktime (&t);
}

////////////////////////////////////////////////////////////////////////////////
ISO8601p::ISO8601p ()
{
  clear ();
}

////////////////////////////////////////////////////////////////////////////////
ISO8601p::ISO8601p (time_t input)
{
  clear ();
  _value = input;
}

////////////////////////////////////////////////////////////////////////////////
ISO8601p::ISO8601p (const std::string& input)
{
  clear ();

  if (Lexer::isAllDigits (input))
  {
    time_t value = (time_t) strtol (input.c_str (), NULL, 10);
    if (value == 0 || value > 60)
    {
      _value = value;
      return;
    }
  }

  std::string::size_type idx = 0;
  parse (input, idx);
}

////////////////////////////////////////////////////////////////////////////////
ISO8601p::~ISO8601p ()
{
}

////////////////////////////////////////////////////////////////////////////////
ISO8601p& ISO8601p::operator= (const ISO8601p& other)
{
  if (this != &other)
  {
    _year    = other._year;
    _month   = other._month;
    _day     = other._day;
    _hours   = other._hours;
    _minutes = other._minutes;
    _seconds = other._seconds;
    _value   = other._value;
  }

  return *this;
}

////////////////////////////////////////////////////////////////////////////////
bool ISO8601p::operator< (const ISO8601p& other)
{
  return _value < other._value;
}

////////////////////////////////////////////////////////////////////////////////
bool ISO8601p::operator> (const ISO8601p& other)
{
  return _value > other._value;
}

////////////////////////////////////////////////////////////////////////////////
ISO8601p::operator std::string () const
{
  std::stringstream s;
  s << _value;
  return s.str ();
}

////////////////////////////////////////////////////////////////////////////////
ISO8601p::operator time_t () const
{
  return _value;
}

////////////////////////////////////////////////////////////////////////////////
// Supported:
//
//    duration   ::= designated                                     # duration
//
//    designated ::= 'P' [nn 'Y'] [nn 'M'] [nn 'D'] ['T' [nn 'H'] [nn 'M'] [nn 'S']]
//
// Not supported:
//
//    duration   ::= designated '/' datetime-ext                    # duration end
//                 | degignated '/' datetime                        # duration end
//                 | designated                                     # duration
//                 | 'P' datetime-ext '/' datetime-ext              # start end
//                 | 'P' datetime '/' datetime                      # start end
//                 | 'P' datetime-ext                               # start
//                 | 'P' datetime                                   # start
//                 | datetime-ext '/' designated                    # start duration
//                 | datetime-ext '/' 'P' datetime-ext              # start end
//                 | datetime-ext '/' datetime-ext                  # start end
//                 | datetime '/' designated                        # start duration
//                 | datetime '/' 'P' datetime                      # start end
//                 | datetime '/' datetime                          # start end
//                 ;
//
bool ISO8601p::parse (const std::string& input, std::string::size_type& start)
{
  // Attempt and ISO parse first.
  auto original_start = start;
  Nibbler n (input.substr (original_start));
  n.save ();

  if (parse_designated (n))
  {
    // Check the values and determine time_t.
    if (validate ())
    {
      // Record cursor position.
      start = n.cursor ();

      resolve ();
      return true;
    }
  }

  // Attempt a legacy format parse next.
  n.restore ();

  // Static and so preserved between calls.
  static std::vector <std::string> units;
  if (units.size () == 0)
    for (unsigned int i = 0; i < NUM_DURATIONS; i++)
      units.push_back (durations[i].unit);

  std::string number;
  std::string unit;

  if (n.getOneOf (units, unit))
  {
    if (n.depleted ()                           ||
        Lexer::isWhitespace         (n.next ()) ||
        Lexer::isSingleCharOperator (n.next ()))
    {
      start = original_start + n.cursor ();

      // Linear lookup - should instead be logarithmic.
      for (unsigned int i = 0; i < NUM_DURATIONS; i++)
      {
        if (durations[i].unit == unit &&
            durations[i].standalone == true)
        {
          _value = static_cast <int> (durations[i].seconds);
          return true;
        }
      }
    }
  }

  else if (n.getNumber (number) &&
           number.find ('e') == std::string::npos &&
           number.find ('E') == std::string::npos &&
           (number.find ('+') == std::string::npos || number.find ('+') == 0) &&
           (number.find ('-') == std::string::npos || number.find ('-') == 0))
  {
    n.skipWS ();
    if (n.getOneOf (units, unit))
    {
      // The "d" unit is a special case, because it is the only one that can
      // legitimately occur at the beginning of a UUID, and be followed by an
      // operator:
      //
      //   1111111d-0000-0000-0000-000000000000
      //
      // Because Lexer::isDuration is higher precedence than Lexer::isUUID,
      // the above UUID looks like:
      //
      //   <1111111d> <-> ...
      //   duration   op  ...
      //
      // So as a special case, durations, with units of "d" are rejected if the
      // quantity exceeds 10000.
      //
      if (unit == "d" &&
          strtol (number.c_str (), NULL, 10) > 10000)
        return false;

      if (n.depleted ()                           ||
          Lexer::isWhitespace         (n.next ()) ||
          Lexer::isSingleCharOperator (n.next ()))
      {
        start = original_start + n.cursor ();
        double quantity = strtod (number.c_str (), NULL);

        // Linear lookup - should instead be logarithmic.
        double seconds = 1;
        for (unsigned int i = 0; i < NUM_DURATIONS; i++)
        {
          if (durations[i].unit == unit)
          {
            seconds = durations[i].seconds;
            _value = static_cast <int> (quantity * static_cast <double> (seconds));
            return true;
          }
        }
      }
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
void ISO8601p::clear ()
{
  _year    = 0;
  _month   = 0;
  _day     = 0;
  _hours   = 0;
  _minutes = 0;
  _seconds = 0;
  _value   = 0;
}

////////////////////////////////////////////////////////////////////////////////
const std::string ISO8601p::format () const
{
  if (_value)
  {
    time_t t = _value;
    int seconds = t % 60; t /= 60;
    int minutes = t % 60; t /= 60;
    int hours   = t % 24; t /= 24;
    int days    = t;

    std::stringstream s;
    s << 'P';
    if (days)   s << days   << 'D';

    if (hours || minutes || seconds)
    {
      s << 'T';
      if (hours)   s << hours   << 'H';
      if (minutes) s << minutes << 'M';
      if (seconds) s << seconds << 'S';
    }

    return s.str ();
  }
  else
  {
    return "PT0S";
  }
}

////////////////////////////////////////////////////////////////////////////////
const std::string ISO8601p::formatVague () const
{
  char formatted[24];
  float days = (float) _value / 86400.0;

       if (_value >= 86400 * 365) sprintf (formatted, "%.1fy", (days / 365.0));
  else if (_value >= 86400 * 84)  sprintf (formatted, "%1dmo", (int) (days / 30));
  else if (_value >= 86400 * 13)  sprintf (formatted, "%dw",   (int) (float) (days / 7.0));
  else if (_value >= 86400)       sprintf (formatted, "%dd",   (int) days);
  else if (_value >= 3600)        sprintf (formatted, "%dh",   (int) (_value / 3600));
  else if (_value >= 60)          sprintf (formatted, "%dmin", (int) (_value / 60));
  else if (_value >= 1)           sprintf (formatted, "%ds",   (int) _value);
  else                            formatted[0] = '\0';

  return std::string (formatted);
}

////////////////////////////////////////////////////////////////////////////////
// 'P' [nn 'Y'] [nn 'M'] [nn 'D'] ['T' [nn 'H'] [nn 'M'] [nn 'S']]
bool ISO8601p::parse_designated (Nibbler& n)
{
  Nibbler backup (n);

  if (n.skip ('P'))
  {
    int value;
    n.save ();
    if (n.getUnsignedInt (value) && n.skip ('Y'))
      _year = value;
    else
      n.restore ();

    n.save ();
    if (n.getUnsignedInt (value) && n.skip ('M'))
      _month = value;
    else
      n.restore ();

    n.save ();
    if (n.getUnsignedInt (value) && n.skip ('D'))
      _day = value;
    else
      n.restore ();

    if (n.skip ('T'))
    {
      n.save ();
      if (n.getUnsignedInt (value) && n.skip ('H'))
        _hours = value;
      else
        n.restore ();

      n.save ();
      if (n.getUnsignedInt (value) && n.skip ('M'))
        _minutes = value;
      else
        n.restore ();

      n.save ();
      if (n.getUnsignedInt (value) && n.skip ('S'))
        _seconds = value;
      else
        n.restore ();
    }

    return true;
  }

  n = backup;
  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool ISO8601p::validate ()
{
  return _year    ||
         _month   ||
         _day     ||
         _hours   ||
         _minutes ||
         _seconds;
}

////////////////////////////////////////////////////////////////////////////////
// Allow un-normalized values.
void ISO8601p::resolve ()
{
  _value = (_year  * 365 * 86400) +
           (_month  * 30 * 86400) +
           (_day         * 86400) +
           (_hours       *  3600) +
           (_minutes     *    60) +
           _seconds;
}

////////////////////////////////////////////////////////////////////////////////
