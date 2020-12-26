/*
TinyGPS++ - a small GPS library for Arduino providing universal NMEA parsing
Based on work by and "distanceBetween" and "courseTo" courtesy of Maarten Lamers.
Suggestion to add satellites, courseTo(), and cardinal() by Matt Monson.
Location precision improvements suggested by Wayne Holder.
Copyright (C) 2008-2013 Mikal Hart
All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "TinyGPS++.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define _GPRMCterm   "GPRMC"
#define _GPGGAterm   "GPGGA"
#define _GPGSVterm   "GPGSV"
#define _GPVTGterm   "GPVTG"
#define _GPGSAterm   "GPGSA"
#define _GPGLLterm   "GPGLL"
#define _GNRMCterm   "GNRMC"
#define _GNGGAterm   "GNGGA"

TinyGPSPlus::TinyGPSPlus()
  :  parity(0)
  ,  isChecksumTerm(false)
  ,  curSentenceType(GPS_SENTENCE_OTHER)
  ,  curTermNumber(0)
  ,  curTermOffset(0)
  ,  sentenceHasFix(false)
  ,  customElts(0)
  ,  customCandidates(0)
  ,  encodedCharCount(0)
  ,  sentencesWithFixCount(0)
  ,  failedChecksumCount(0)
  ,  passedChecksumCount(0)
  ,  sentence_GsvOff{"$PUBX,40,GSV,0,0,0,0,0,0*59"}
  ,  sentence_GsvOn{"$PUBX,40,GSV,0,1,0,0,0,0*58"}
  ,  sentence_GsaOff{"$PUBX,40,GSA,0,0,0,0,0,0*4E"}
  ,  sentence_GsaOn{"$PUBX,40,GSA,0,1,0,0,0,0*4F"}
  ,  sentence_VtgOff{"$PUBX,40,VTG,0,0,0,0,0,0*5E"}
  ,  sentence_VtgOn{"$PUBX,40,VTG,0,1,0,0,0,0*5F"}
  ,  sentence_GllOff{"$PUBX,40,GLL,0,0,0,0,0,0*5C"}
  ,  sentence_GllOn{"$PUBX,40,GLL,0,1,0,0,0,0*5D"}
  ,  sentence_5000msPeriod{0xb5, 0x62, 0x6, 0x8, 0x6, 0x0, 0x88, 0x13, 0x1, 0x0, 0x1, 0x0, 0xb1, 0x49}
  ,  sentence_100msPeriod{0xb5, 0x62, 0x6, 0x8, 0x6, 0x0, 0x64, 0x0, 0x1, 0x0, 0x1, 0x0, 0x7a, 0x12}
{
  term[0] = '\0';
}

//
// public methods
//

bool TinyGPSPlus::readSerial()
{
    bool retVal{false};
    while (Serial.available())
    {
        retVal |= encode(Serial.read());
    }
    return retVal;
}
TinyGPSPlus::EncodeStatus TinyGPSPlus::readSerialGiveStatus()
{
    EncodeStatus retVal{EncodeStatus::UNFINISHED};
    while (Serial.available() and retVal == EncodeStatus::UNFINISHED)
    {
        retVal = encodeGiveStatus(Serial.read());
    }
    return retVal;
}

bool TinyGPSPlus::encode(char c)
{
    EncodeStatus const status = encodeGiveStatus(c);
    return (status != EncodeStatus::UNFINISHED);
}

TinyGPSPlus::EncodeStatus TinyGPSPlus::encodeGiveStatus(char c)
{
  ++encodedCharCount;

  switch(c)
  {
  case ',': // term terminators
    parity ^= (uint8_t)c;
  case '\r':
  case '\n':
  case '*':
    {
      EncodeStatus status = EncodeStatus::UNFINISHED;
      if (curTermOffset < sizeof(term))
      {
        term[curTermOffset] = 0;
        status = endOfTermHandler();
      }
      ++curTermNumber;
      curTermOffset = 0;
      isChecksumTerm = c == '*';
      return status;
    }
    break;

  case '$': // sentence begin
    curTermNumber = curTermOffset = 0;
    parity = 0;
    curSentenceType = GPS_SENTENCE_OTHER;
    isChecksumTerm = false;
    sentenceHasFix = false;
    return EncodeStatus::UNFINISHED;

  default: // ordinary characters
    if (curTermOffset < sizeof(term) - 1)
      term[curTermOffset++] = c;
    if (!isChecksumTerm)
      parity ^= c;
    return EncodeStatus::UNFINISHED;
  }

  return EncodeStatus::UNFINISHED;
}

//
// internal utilities
//
int TinyGPSPlus::fromHex(char a)
{
  if (a >= 'A' && a <= 'F')
    return a - 'A' + 10;
  else if (a >= 'a' && a <= 'f')
    return a - 'a' + 10;
  else
    return a - '0';
}

// static
// Parse a (potentially negative) number with up to 2 decimal digits -xxxx.yy
int32_t TinyGPSPlus::parseDecimal(const char *term)
{
  bool negative = *term == '-';
  if (negative) ++term;
  int32_t ret = 100 * (int32_t)atol(term);
  while (isdigit(*term)) ++term;
  if (*term == '.' && isdigit(term[1]))
  {
    ret += 10 * (term[1] - '0');
    if (isdigit(term[2]))
      ret += term[2] - '0';
  }
  return negative ? -ret : ret;
}

// static
// Parse degrees in that funny NMEA format DDMM.MMMM
void TinyGPSPlus::parseDegrees(const char *term, RawDegrees &deg)
{
  uint32_t leftOfDecimal = (uint32_t)atol(term);
  uint16_t minutes = (uint16_t)(leftOfDecimal % 100);
  uint32_t multiplier = 10000000UL;
  uint32_t tenMillionthsOfMinutes = minutes * multiplier;

  deg.deg = (int16_t)(leftOfDecimal / 100);

  while (isdigit(*term))
    ++term;

  if (*term == '.')
    while (isdigit(*++term))
    {
      multiplier /= 10;
      tenMillionthsOfMinutes += (*term - '0') * multiplier;
    }

  deg.billionths = (5 * tenMillionthsOfMinutes + 1) / 3;
  deg.negative = false;
}

#define COMBINE(sentence_type, term_number) (((unsigned)(sentence_type) << 5) | term_number)

// Processes a just-completed term
// Returns true if new sentence has just passed checksum test and is validated
TinyGPSPlus::EncodeStatus TinyGPSPlus::endOfTermHandler()
{
  EncodeStatus retValue{EncodeStatus::UNFINISHED};
  // If it's the checksum term, and the checksum checks out, commit
  if (isChecksumTerm)
  {
    byte checksum = 16 * fromHex(term[0]) + fromHex(term[1]);
    //printf("%u vs %u \n", checksum, parity);
    if (checksum == parity)
    {
      passedChecksumCount++;
      if (sentenceHasFix)
        ++sentencesWithFixCount;

      switch(curSentenceType)
      {
      case GPS_SENTENCE_GPRMC:
        date.commit();
        time.commit();
        if (sentenceHasFix)
        {
           location.commit();
           speed.commit();
           course.commit();
        }
        stats.rmc++;
        retValue = EncodeStatus::RMC;
        break;
      case GPS_SENTENCE_GPGGA:
        time.commit();
        if (sentenceHasFix)
        {
          location.commit();
          altitude.commit();
        }
        satellites.commit();
        hdop.commit();
        stats.gga++;
        retValue = EncodeStatus::GGA;
        break;
      case GPS_SENTENCE_GPGSV:
        satsInView.commit();
        retValue = EncodeStatus::GSV;
        stats.gsv++;
        break;
      case GPS_SENTENCE_GPVTG:
        groundSpeed.commit();
        retValue = EncodeStatus::VTG;
        stats.vtg++;
        break;
      case GPS_SENTENCE_GPGSA:
          gsa.commit();
          retValue = EncodeStatus::GSA;
          stats.gsa++;
          break;
      case GPS_SENTENCE_GPGLL:
          stats.gll++;
          retValue = EncodeStatus::GLL;
          break;
      }

      // Commit all custom listeners of this sentence type
      for (TinyGPSCustom *p = customCandidates; p != NULL && strcmp(p->sentenceName, customCandidates->sentenceName) == 0; p = p->next)
         p->commit();
      return retValue;
    }

    else
    {
      ++failedChecksumCount;
      retValue = EncodeStatus::INVALID;
    }

    return retValue;
  }

  // the first term determines the sentence type
  if (curTermNumber == 0)
  {
    if (!strcmp(term, _GPRMCterm) || !strcmp(term, _GNRMCterm))
      curSentenceType = GPS_SENTENCE_GPRMC;
    else if (!strcmp(term, _GPGGAterm) || !strcmp(term, _GNGGAterm))
      curSentenceType = GPS_SENTENCE_GPGGA;
    else if (!strcmp(term, _GPGSVterm))
      curSentenceType = GPS_SENTENCE_GPGSV;
    else if (!strcmp(term, _GPVTGterm))
      curSentenceType = GPS_SENTENCE_GPVTG;
    else if (!strcmp(term, _GPGSAterm))
      curSentenceType = GPS_SENTENCE_GPGSA;
    else if (!strcmp(term, _GPGLLterm))
        curSentenceType = GPS_SENTENCE_GPGLL;
    else
      curSentenceType = GPS_SENTENCE_OTHER;

    // Any custom candidates of this sentence type?
    for (customCandidates = customElts; customCandidates != NULL && strcmp(customCandidates->sentenceName, term) < 0; customCandidates = customCandidates->next);
    if (customCandidates != NULL && strcmp(customCandidates->sentenceName, term) > 0)
       customCandidates = NULL;

    return retValue;
  }

  if (curSentenceType != GPS_SENTENCE_OTHER && term[0])
    switch(COMBINE(curSentenceType, curTermNumber))
  {
    case COMBINE(GPS_SENTENCE_GPRMC, 1): // Time in both sentences
    case COMBINE(GPS_SENTENCE_GPGGA, 1):
      time.setTime(term);
      break;
    case COMBINE(GPS_SENTENCE_GPRMC, 2): // GPRMC validity
      sentenceHasFix = term[0] == 'A';
      break;
    case COMBINE(GPS_SENTENCE_GPRMC, 3): // Latitude
    case COMBINE(GPS_SENTENCE_GPGGA, 2):
      location.setLatitude(term);
      break;
    case COMBINE(GPS_SENTENCE_GPRMC, 4): // N/S
    case COMBINE(GPS_SENTENCE_GPGGA, 3):
      location.rawNewLatData.negative = term[0] == 'S';
      break;
    case COMBINE(GPS_SENTENCE_GPRMC, 5): // Longitude
    case COMBINE(GPS_SENTENCE_GPGGA, 4):
      location.setLongitude(term);
      break;
    case COMBINE(GPS_SENTENCE_GPRMC, 6): // E/W
    case COMBINE(GPS_SENTENCE_GPGGA, 5):
      location.rawNewLngData.negative = term[0] == 'W';
      break;
    case COMBINE(GPS_SENTENCE_GPRMC, 7): // Speed (GPRMC)
      speed.set(term);
      break;
    case COMBINE(GPS_SENTENCE_GPRMC, 8): // Course (GPRMC)
      course.set(term);
      break;
    case COMBINE(GPS_SENTENCE_GPRMC, 9): // Date (GPRMC)
      date.setDate(term);
      break;
    case COMBINE(GPS_SENTENCE_GPGGA, 6): // Fix data (GPGGA)
      sentenceHasFix = term[0] > '0';
      break;
    case COMBINE(GPS_SENTENCE_GPGGA, 7): // Satellites used (GPGGA)
      satellites.set(term);
      break;
    case COMBINE(GPS_SENTENCE_GPGGA, 8): // HDOP
      hdop.set(term);
      break;
    case COMBINE(GPS_SENTENCE_GPGGA, 9): // Altitude (GPGGA)
      altitude.set(term);
      break;
    case COMBINE(GPS_SENTENCE_GPGSV, 2): // Sentence number (GPGSV)
      if (1 == atoi(term)) // begin new group
      {
          satsInView.numMsgs++;
          satsInView.init();
      }
      break;
    case COMBINE(GPS_SENTENCE_GPGSV, 3): // Number of satellites (GPGSV)
      satsInView.setNumOf(term);
      break;
    case COMBINE(GPS_SENTENCE_GPGSV, 4): // Id of Satellite @1 (GPGSV)
    case COMBINE(GPS_SENTENCE_GPGSV, 8): // Id of Satellite @2 (GPGSV)
    case COMBINE(GPS_SENTENCE_GPGSV, 12): // Id of Satellite @3 (GPGSV)
    case COMBINE(GPS_SENTENCE_GPGSV, 16): // Id of Satellite @4 (GPGSV)
      satsInView.addSatId(term);
      break;
    case COMBINE(GPS_SENTENCE_GPGSV, 7): // Id of Satellite @1 (GPGSV)
    case COMBINE(GPS_SENTENCE_GPGSV, 11): // Id of Satellite @2 (GPGSV)
    case COMBINE(GPS_SENTENCE_GPGSV, 15): // Id of Satellite @3 (GPGSV)
    case COMBINE(GPS_SENTENCE_GPGSV, 19): // Id of Satellite @4 (GPGSV)
      satsInView.addSnr(term);
      break;
    case COMBINE(GPS_SENTENCE_GPVTG, 7): // Ground speed km/h (GPVTG)
      groundSpeed.set(term);
      break;
    case COMBINE(GPS_SENTENCE_GPGSA, 1): // Mode (GPGSA)
      gsa.init(); // new sentence begins
      gsa.setMode(term);
      gsa.amount()++;
      break;
    case COMBINE(GPS_SENTENCE_GPGSA, 2): // Fix (GPGSA)
      gsa.setFix(term);
      break;
    case COMBINE(GPS_SENTENCE_GPGSA, 3): // Sat id @1 (GPGSA)
    case COMBINE(GPS_SENTENCE_GPGSA, 4): // Sat id @2 (GPGSA)
    case COMBINE(GPS_SENTENCE_GPGSA, 5): // Sat id @3 (GPGSA)
    case COMBINE(GPS_SENTENCE_GPGSA, 6): // Sat id @4 (GPGSA)
    case COMBINE(GPS_SENTENCE_GPGSA, 7): // Sat id @5 (GPGSA)
    case COMBINE(GPS_SENTENCE_GPGSA, 8): // Sat id @6 (GPGSA)
    case COMBINE(GPS_SENTENCE_GPGSA, 9): // Sat id @7 (GPGSA)
    case COMBINE(GPS_SENTENCE_GPGSA, 10): // Sat id @8 (GPGSA)
    case COMBINE(GPS_SENTENCE_GPGSA, 11): // Sat id @9 (GPGSA)
    case COMBINE(GPS_SENTENCE_GPGSA, 12): // Sat id @10 (GPGSA)
    case COMBINE(GPS_SENTENCE_GPGSA, 13): // Sat id @11 (GPGSA)
    case COMBINE(GPS_SENTENCE_GPGSA, 14): // Sat id @12 (GPGSA)
      gsa.setSat(term);
      break;
    case COMBINE(GPS_SENTENCE_GPGSA, 15): // PDOP (GPGSA)
      gsa.setPdop(term);
      break;
    case COMBINE(GPS_SENTENCE_GPGSA, 16): // HDOP (GPGSA)
      gsa.setHdop(term);
      break;
    case COMBINE(GPS_SENTENCE_GPGSA, 17): // VDOP (GPGSA)
      gsa.setVdop(term);
      break;
  }

  // Set custom values as needed
  for (TinyGPSCustom *p = customCandidates; p != NULL && strcmp(p->sentenceName, customCandidates->sentenceName) == 0 && p->termNumber <= curTermNumber; p = p->next)
    if (p->termNumber == curTermNumber)
         p->set(term);

  return retValue;
}

/* static */
double TinyGPSPlus::distanceBetween(double lat1, double long1, double lat2, double long2)
{
  // returns distance in meters between two positions, both specified
  // as signed decimal-degrees latitude and longitude. Uses great-circle
  // distance computation for hypothetical sphere of radius 6372795 meters.
  // Because Earth is no exact sphere, rounding errors may be up to 0.5%.
  // Courtesy of Maarten Lamers
  double delta = radians(long1-long2);
  double sdlong = sin(delta);
  double cdlong = cos(delta);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  double slat1 = sin(lat1);
  double clat1 = cos(lat1);
  double slat2 = sin(lat2);
  double clat2 = cos(lat2);
  delta = (clat1 * slat2) - (slat1 * clat2 * cdlong);
  delta = sq(delta);
  delta += sq(clat2 * sdlong);
  delta = sqrt(delta);
  double denom = (slat1 * slat2) + (clat1 * clat2 * cdlong);
  delta = atan2(delta, denom);
  return delta * 6372795;
}

double TinyGPSPlus::courseTo(double lat1, double long1, double lat2, double long2)
{
  // returns course in degrees (North=0, West=270) from position 1 to position 2,
  // both specified as signed decimal-degrees latitude and longitude.
  // Because Earth is no exact sphere, calculated course may be off by a tiny fraction.
  // Courtesy of Maarten Lamers
  double dlon = radians(long2-long1);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  double a1 = sin(dlon) * cos(lat2);
  double a2 = sin(lat1) * cos(lat2) * cos(dlon);
  a2 = cos(lat1) * sin(lat2) - a2;
  a2 = atan2(a1, a2);
  if (a2 < 0.0)
  {
    a2 += TWO_PI;
  }
  return degrees(a2);
}

const char *TinyGPSPlus::cardinal(double course)
{
  static const char* directions[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
  int direction = (int)((course + 11.25f) / 22.5f);
  return directions[direction % 16];
}

//const char baudTo115200Message[] = {0xb5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xd0, 0x08, 0x00, 0x00, 0x00, 0xc2, 0x01, 0x00, 0x07, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc4, 0x96, 0xb5, 0x62, 0x06, 0x00, 0x01, 0x00, 0x01, 0x08, 0x22};
//const char baudTo115200Message[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x00, 0xC2, 0x01, 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x7E};
const char* baudTo115200Message = {"$PUBX,41,1,0007,0003,115200,0*18"};
void TinyGPSPlus::baudrateTo115200() const
{
    delay(100);
    Serial.println(baudTo115200Message);
    delay(100);
    Serial.end();
    delay(100);
    Serial.begin(115200);
    delay(100);
}
void TinyGPSPlus::switchOffGsv() const
{
    sendStringSentence(sentence_GsvOff);
}
void TinyGPSPlus::setMinimumNmeaSentences() const
{
    sendStringSentence(sentence_GsvOff);
    sendStringSentence(sentence_GsaOff);
    sendStringSentence(sentence_VtgOff);
    sendStringSentence(sentence_GllOff);
}
void TinyGPSPlus::periodTo5000ms() const
{
    sendByteSentence(sentence_5000msPeriod, sizeof(sentence_5000msPeriod));
}
void TinyGPSPlus::periodTo100ms() const
{
    sendByteSentence(sentence_100msPeriod, sizeof(sentence_100msPeriod));
}
void TinyGPSPlus::sendStringSentence(const String& sentence) const
{
    Serial.println(sentence);
}
void TinyGPSPlus::sendByteSentence(const uint8_t* sentence, uint32_t const length) const
{
    for (int i = 0; i < length; i++)
    {
        Serial.write(sentence[i]);
    }
    Serial.println();
}
void TinyGPSLocation::commit()
{
   rawLatData = rawNewLatData;
   rawLngData = rawNewLngData;
   lastCommitTime = millis();
   valid = updated = true;
}

void TinyGPSLocation::setLatitude(const char *term)
{
   TinyGPSPlus::parseDegrees(term, rawNewLatData);
}

void TinyGPSLocation::setLongitude(const char *term)
{
   TinyGPSPlus::parseDegrees(term, rawNewLngData);
}

double TinyGPSLocation::lat()
{
   updated = false;
   double ret = rawLatData.deg + rawLatData.billionths / 1000000000.0;
   return rawLatData.negative ? -ret : ret;
}

double TinyGPSLocation::lng()
{
   updated = false;
   double ret = rawLngData.deg + rawLngData.billionths / 1000000000.0;
   return rawLngData.negative ? -ret : ret;
}

void TinyGPSDate::commit()
{
   date = newDate;
   lastCommitTime = millis();
   valid = updated = true;
}

void TinyGPSTime::commit()
{
   time = newTime;
   lastCommitTime = millis();
   valid = updated = true;
}
void SatsInView::commit()
{
   valid = updated = true;
}

void TinyGPSTime::setTime(const char *term)
{
   newTime = (uint32_t)TinyGPSPlus::parseDecimal(term);
}
void TinyGPSDate::setDate(const char *term)
{
   newDate = atol(term);
}
bool TinyGPSDate::inRange()
{
    bool retVal{true};
    if (valid)
    {
        if (year() < 2000 or year() > 2999 or
            month() > 12 or
            day() > 31)
        {
            retVal = false;
        }
    }
    return retVal;
}
uint16_t TinyGPSDate::year()
{
   updated = false;
   uint16_t year = date % 100;
   return year + 2000;
}

uint8_t TinyGPSDate::month()
{
   updated = false;
   return (date / 100) % 100;
}

uint8_t TinyGPSDate::day()
{
   updated = false;
   return date / 10000;
}
bool TinyGPSTime::inRange()
{
    bool retVal{true};
    if (valid)
    {
        if (hour() > 23 or
            minute() > 59 or
            second() > 59)
        {
            retVal = false;
        }
    }
    return retVal;
}
uint8_t TinyGPSTime::hour()
{
   updated = false;
   return time / 1000000;
}

uint8_t TinyGPSTime::minute()
{
   updated = false;
   return (time / 10000) % 100;
}

uint8_t TinyGPSTime::second()
{
   updated = false;
   return (time / 100) % 100;
}

uint8_t TinyGPSTime::centisecond()
{
   updated = false;
   return time % 100;
}

void TinyGPSDecimal::commit()
{
   val = newval;
   lastCommitTime = millis();
   valid = updated = true;
}

void TinyGPSDecimal::set(const char *term)
{
   newval = TinyGPSPlus::parseDecimal(term);
}

void TinyGPSInteger::commit()
{
   val = newval;
   lastCommitTime = millis();
   valid = updated = true;
}

void TinyGPSInteger::set(const char *term)
{
   newval = atol(term);
}

TinyGPSCustom::TinyGPSCustom(TinyGPSPlus &gps, const char *_sentenceName, int _termNumber)
{
   begin(gps, _sentenceName, _termNumber);
}

void TinyGPSCustom::begin(TinyGPSPlus &gps, const char *_sentenceName, int _termNumber)
{
   lastCommitTime = 0;
   updated = valid = false;
   sentenceName = _sentenceName;
   termNumber = _termNumber;
   memset(stagingBuffer, '\0', sizeof(stagingBuffer));
   memset(buffer, '\0', sizeof(buffer));

   // Insert this item into the GPS tree
   gps.insertCustom(this, _sentenceName, _termNumber);
}

void TinyGPSCustom::commit()
{
   strcpy(this->buffer, this->stagingBuffer);
   lastCommitTime = millis();
   valid = updated = true;
}

void TinyGPSCustom::set(const char *term)
{
   strncpy(this->stagingBuffer, term, sizeof(this->stagingBuffer));
}

void TinyGPSPlus::insertCustom(TinyGPSCustom *pElt, const char *sentenceName, int termNumber)
{
   TinyGPSCustom **ppelt;

   for (ppelt = &this->customElts; *ppelt != NULL; ppelt = &(*ppelt)->next)
   {
      int cmp = strcmp(sentenceName, (*ppelt)->sentenceName);
      if (cmp < 0 || (cmp == 0 && termNumber < (*ppelt)->termNumber))
         break;
   }

   pElt->next = *ppelt;
   *ppelt = pElt;
}
SatsInView::SatsInView(): updated{false}, valid{false}, invalidSat{INVALID_ID, "0"}, numMsgs{0}
{
    init();
}
void SatsInView::init()
{
    numSats = 0;
    prevSat = nullptr;
    for(SatInView& sat : sats)
    {
        sat = invalidSat;
    }
}
void SatsInView::setNumOf(const char *term)
{
    numSats = atoi(term);
}
void SatsInView::addSatId(const char *term)
{
    const int id = atoi(term);
    SatInView& sat = findSat(id);
    sat.id() = id;
}
SatsInView::SatInView& SatsInView::findSat(const int id)
{
    SatInView* sat = findNewSat(id);
    return *sat;
}
SatsInView::SatInView* SatsInView::findNewSat(const int id)
{
    prevSat = &sats[0];
    for(SatInView& sat : sats)
    {

        if(sat.id() == INVALID_ID)
        {
            prevSat = &sat;
            break;
        }
    }
    return prevSat;
}
void SatsInView::addSnr(const char *term)
{
    if (prevSat)
    {
        prevSat->snr() = STRING{term};
    }
}
unsigned int SatsInView::numOfDb() const
{
    unsigned int total = 0;
    for(const SatInView& sat : sats)
    {
        if(sat.id() != INVALID_ID)
        {
            total++;
        }
    }
    return total;
}
unsigned int SatsInView::totalSnr() const
{
    unsigned int total = 0;
    for(const SatInView& sat : sats)
    {
        if(sat.id() != INVALID_ID)
        {
            total += sat.snrInt();
        }
    }
    return total;
}
unsigned int SatsInView::SatInView::snrInt() const
{
    unsigned int ret{};
    const STRING& s = snr();
    ret = atoi(s.c_str());
    return ret;
}
void GroundSpeed::set(const char* term)
{
    val = atof(term);
}
void Gsa::setMode(const char* term)
{
    mode_ = term[0];
}
const char Gsa::fixNone[]{"No"};
const char Gsa::fix2d[]{"2D"};
const char Gsa::fix3d[]{"3D"};
const char Gsa::fixNotApplicable[]{"N/A"};
void Gsa::setFix(const char* term)
{
    int val = atoi(term);
    if (val == 1)
    {
        fix_ = fixNone;
    }
    else if (val == 2)
    {
        fix_ = fix2d;
    }
    else if (val == 3)
    {
        fix_ = fix3d;
    }
    else
    {
        fix_ = fixNotApplicable;
    }
}
void Gsa::setPdop(const char* term)
{
    pdop_ = atof(term);
}
void Gsa::setVdop(const char* term)
{
    vdop_ = atof(term);
}
void Gsa::setHdop(const char* term)
{
    hdop_ = atof(term);
}
void Gsa::setSat(const char* term)
{
    if (numSats_ < MAX_SATS)
    {
        const int id = atoi(term);
        satId[numSats_] = id;
        numSats_++;
    }
}
void Gsa::init()
{
    updated = false;
    valid = false;
    numSats_ = 0;
    pdop_ = 0.0;
    vdop_ = 0.0;
    hdop_ = 0.0;
    fix_ = fixNotApplicable;
    mode_ = "N"[0];
}
bool Gsa::fixIs3d() const
{
    return (0 == strcmp(fix3d, fix_));
}



