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

#ifndef __TinyGPSPlus_h
#define __TinyGPSPlus_h

#include "Arduino.h"
#include <limits.h>

#define _GPS_VERSION "1.0.2" // software version of this library
#define _GPS_MPH_PER_KNOT 1.15077945
#define _GPS_MPS_PER_KNOT 0.51444444
#define _GPS_KMPH_PER_KNOT 1.852
#define _GPS_MILES_PER_METER 0.00062137112
#define _GPS_KM_PER_METER 0.001
#define _GPS_FEET_PER_METER 3.2808399
#define _GPS_MAX_FIELD_SIZE 15

struct RawDegrees
{
   uint16_t deg;
   uint32_t billionths;
   bool negative;
public:
   RawDegrees() : deg(0), billionths(0), negative(false)
   {}
};

struct TinyGPSLocation
{
   friend class TinyGPSPlus;
public:
   bool isValid() const    { return valid; }
   bool isUpdated() const  { return updated; }
   uint32_t age() const    { return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX; }
   const RawDegrees &rawLat()     { updated = false; return rawLatData; }
   const RawDegrees &rawLng()     { updated = false; return rawLngData; }
   double lat();
   double lng();

   TinyGPSLocation() : valid(false), updated(false)
   {}

private:
   bool valid, updated;
   RawDegrees rawLatData, rawLngData, rawNewLatData, rawNewLngData;
   uint32_t lastCommitTime;
   void commit();
   void setLatitude(const char *term);
   void setLongitude(const char *term);
};

struct TinyGPSDate
{
   friend class TinyGPSPlus;
public:
   bool isValid() const       { return valid; }
   bool isUpdated() const     { return updated; }
   uint32_t age() const       { return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX; }

   uint32_t value()           { updated = false; return date; }
   uint16_t year();
   uint8_t month();
   uint8_t day();

   TinyGPSDate() : valid(false), updated(false), date(0)
   {}
   bool inRange();

private:
   bool valid, updated;
   uint32_t date, newDate;
   uint32_t lastCommitTime;
   void commit();
   void setDate(const char *term);
};

struct TinyGPSTime
{
   friend class TinyGPSPlus;
public:
   bool isValid() const       { return valid; }
   bool isUpdated() const     { return updated; }
   uint32_t age() const       { return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX; }

   uint32_t value()           { updated = false; return time; }
   uint8_t hour();
   uint8_t minute();
   uint8_t second();
   uint8_t centisecond();

   TinyGPSTime() : valid(false), updated(false), time(0)
   {}
   bool inRange();

private:
   bool valid, updated;
   uint32_t time, newTime;
   uint32_t lastCommitTime;
   void commit();
   void setTime(const char *term);
};

struct TinyGPSDecimal
{
   friend class TinyGPSPlus;
public:
   bool isValid() const    { return valid; }
   bool isUpdated() const  { return updated; }
   uint32_t age() const    { return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX; }
   int32_t value()         { updated = false; return val; }

   TinyGPSDecimal() : valid(false), updated(false), val(0)
   {}

private:
   bool valid, updated;
   uint32_t lastCommitTime;
   int32_t val, newval;
   void commit();
   void set(const char *term);
};

struct TinyGPSInteger
{
   friend class TinyGPSPlus;
public:
   bool isValid() const    { return valid; }
   bool isUpdated() const  { return updated; }
   uint32_t age() const    { return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX; }
   uint32_t value()        { updated = false; return val; }

   TinyGPSInteger() : valid(false), updated(false), val(0)
   {}

private:
   bool valid, updated;
   uint32_t lastCommitTime;
   uint32_t val, newval;
   void commit();
   void set(const char *term);
};

struct TinyGPSSpeed : TinyGPSDecimal
{
   double knots()    { return value() / 100.0; }
   double mph()      { return _GPS_MPH_PER_KNOT * value() / 100.0; }
   double mps()      { return _GPS_MPS_PER_KNOT * value() / 100.0; }
   double kmph()     { return _GPS_KMPH_PER_KNOT * value() / 100.0; }
};

struct TinyGPSCourse : public TinyGPSDecimal
{
   double deg()      { return value() / 100.0; }
};

struct TinyGPSAltitude : TinyGPSDecimal
{
   double meters()       { return value() / 100.0; }
   double miles()        { return _GPS_MILES_PER_METER * value() / 100.0; }
   double kilometers()   { return _GPS_KM_PER_METER * value() / 100.0; }
   double feet()         { return _GPS_FEET_PER_METER * value() / 100.0; }
};

struct TinyGPSHDOP : TinyGPSDecimal
{
   double hdop() { return value() / 100.0; }
};

class TinyGPSPlus;
class TinyGPSCustom
{
public:
   TinyGPSCustom() {};
   TinyGPSCustom(TinyGPSPlus &gps, const char *sentenceName, int termNumber);
   void begin(TinyGPSPlus &gps, const char *_sentenceName, int _termNumber);

   bool isUpdated() const  { return updated; }
   bool isValid() const    { return valid; }
   uint32_t age() const    { return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX; }
   const char *value()     { updated = false; return buffer; }

private:
   void commit();
   void set(const char *term);

   char stagingBuffer[_GPS_MAX_FIELD_SIZE + 1];
   char buffer[_GPS_MAX_FIELD_SIZE + 1];
   unsigned long lastCommitTime;
   bool valid, updated;
   const char *sentenceName;
   int termNumber;
   friend class TinyGPSPlus;
   TinyGPSCustom *next;
};

typedef std::string STRING;
static const unsigned int MAX_SATS{30};
class SatsInView
{
    friend class TinyGPSPlus;
    static const int INVALID_ID{-1};
    class SatInView
    {
        public:
        SatInView(): id_{}, snr_{}{}
        SatInView(const int _id, const STRING& _snr): id_{_id}, snr_{_snr}{}
        const int& id() const { return id_; }
        int& id() { return id_; }
        const STRING& snr() const { return snr_; }
        unsigned int snrInt() const;
        STRING& snr() { return snr_; }
        private:
        int id_;
        STRING snr_;
    };
public:
    SatsInView();
    void init();
    bool isUpdated() const { return updated; }
    bool isValid() const { return valid; }
    unsigned int messageAmount() const { return numMsgs; }
    unsigned int numOf() const { return numSats; }
    unsigned int numOfDb() const;
    void commit();
    const SatInView& operator[](const int i) const
    {
        if (i < MAX_SATS)
        {
            return sats[i];
        }
        return invalidSat;
    }
    SatInView& findSat(const int id);
    void setNumOf(const char *term);
    void addSatId(const char *term);
    void addSnr(const char *term);
    unsigned int totalSnr() const;
private:
    SatInView* findNewSat(const int id);
    bool updated;
    bool valid;
    unsigned int numSats;
    SatInView sats[MAX_SATS];
    const SatInView invalidSat;
    SatInView* prevSat;
    unsigned int numMsgs;
};

class GroundSpeed
{
public:
    GroundSpeed(): updated{false}, valid{false}, val{0.0}{}
    bool isUpdated() const { return updated; }
    bool isValid() const { return valid; }
    void commit() { updated = valid = true;}
    double value() { updated = false; return val; }
    void set(const char*);
private:
    bool updated;
    bool valid;
    double val;
};

class Gsa
{
public:
    Gsa(): updated{false}, valid{false}, numSats_{0}, pdop_{0.0}, vdop_{0.0}, hdop_{0.0}, fix_{}, mode_{}, amount_{}
    {
        init();
    }
    void init();
    bool isUpdated() const { return updated; }
    bool isValid() const { return valid; }
    void commit() { updated = valid = true;}
    const char* fix() const { return fix_; }
    bool fixIs3d() const;
    const char mode() const { return mode_; }
    int numSats() const { return numSats_; }
    double pdop() const { return pdop_; };
    double vdop() const { return vdop_; };
    double hdop() const { return hdop_; };
    void setMode(const char*);
    void setFix(const char*);
    void setPdop(const char*);
    void setVdop(const char*);
    void setHdop(const char*);
    void setSat(const char*);
    const int* sats() const { return satId; }
    int amount() const { return amount_; }
    int& amount() { return amount_; }
private:
    bool updated;
    bool valid;
    int numSats_;
    int satId[MAX_SATS];
    double pdop_, vdop_, hdop_;
    static const char fixNone[];
    static const char fixNotApplicable[];
    static const char fix2d[];
    static const char fix3d[];
    const char* fix_;
    char mode_;
    int amount_;
};

class TinyGPSPlus
{
public:
  enum class EncodeStatus
  {
      UNFINISHED = 0,
      INVALID = 1,
      RMC = 2,
      GGA = 3,
      GSV = 4,
      VTG = 5,
      GSA = 6,
      GLL = 7
  };
  TinyGPSPlus();
  bool readSerial();
  bool encode(char c); // process one character received from GPS
  EncodeStatus readSerialGiveStatus();
  EncodeStatus encodeGiveStatus(char c); // process one character received from GPS
  TinyGPSPlus &operator << (char c) {encode(c); return *this;}

  TinyGPSLocation location;
  TinyGPSDate date;
  TinyGPSTime time;
  TinyGPSSpeed speed;
  TinyGPSCourse course;
  TinyGPSAltitude altitude;
  TinyGPSInteger satellites;
  TinyGPSHDOP hdop;
  SatsInView satsInView;
  GroundSpeed groundSpeed;
  Gsa gsa;
  bool ggaFix;
  struct Stats
  {
      unsigned int rmc{};
      unsigned int gga{};
      unsigned int gsa{};
      unsigned int gsv{};
      unsigned int gll{};
      unsigned int vtg{};
  };
  Stats stats;

  const String sentence_GsvOff;
  const String sentence_GsvOn;
  const String sentence_GsaOff;
  const String sentence_GsaOn;
  const String sentence_VtgOff;
  const String sentence_VtgOn;
  const String sentence_GllOff;
  const String sentence_GllOn;
  const uint8_t sentence_5000msPeriod[14];
  const uint8_t sentence_100msPeriod[14];

  static const char *libraryVersion() { return _GPS_VERSION; }

  static double distanceBetween(double lat1, double long1, double lat2, double long2);
  static double courseTo(double lat1, double long1, double lat2, double long2);
  static const char *cardinal(double course);

  static int32_t parseDecimal(const char *term);
  static void parseDegrees(const char *term, RawDegrees &deg);

  uint32_t charsProcessed()   const { return encodedCharCount; }
  uint32_t sentencesWithFix() const { return sentencesWithFixCount; }
  uint32_t failedChecksum()   const { return failedChecksumCount; }
  uint32_t passedChecksum()   const { return passedChecksumCount; }
  void baudrateTo115200() const;
  void switchOffGsv() const;
  void setMinimumNmeaSentences() const;
  void periodTo5000ms() const;
  void periodTo100ms() const;
  void sendStringSentence(const String& sentence) const;
  void sendByteSentence(const uint8_t* sentence, uint32_t const length) const;

private:
  enum {GPS_SENTENCE_GPGGA, GPS_SENTENCE_GPRMC, GPS_SENTENCE_GPGSV, GPS_SENTENCE_GPVTG, GPS_SENTENCE_GPGSA, GPS_SENTENCE_GPGLL, GPS_SENTENCE_OTHER};

  // parsing state variables
  uint8_t parity;
  bool isChecksumTerm;
  char term[_GPS_MAX_FIELD_SIZE];
  uint8_t curSentenceType;
  uint8_t curTermNumber;
  uint8_t curTermOffset;
  bool sentenceHasFix;

  // custom element support
  friend class TinyGPSCustom;
  TinyGPSCustom *customElts;
  TinyGPSCustom *customCandidates;
  void insertCustom(TinyGPSCustom *pElt, const char *sentenceName, int index);

  // statistics
  uint32_t encodedCharCount;
  uint32_t sentencesWithFixCount;
  uint32_t failedChecksumCount;
  uint32_t passedChecksumCount;

  // internal utilities
  int fromHex(char a);
  TinyGPSPlus::EncodeStatus endOfTermHandler();
};

#endif // def(__TinyGPSPlus_h)
