
#include "gtest/gtest.h"
#include "TinyGPS++.h"

class TestTinyGpsPlus : public ::testing::Test
{
public:
    TestTinyGpsPlus()
    : gps{nullptr},
      charsProcessed{0}
    {}
    void SetUp()
    {
        gps = new TinyGPSPlus();
        charsProcessed = 0;
    }
    void TearDown()
    {
        delete gps;
    }
protected:
    void encode(const std::string& s)
    {
        for (char c : s)
        {
            gps->encode(c);
        }
        charsProcessed += s.size();
        EXPECT_EQ(charsProcessed, gps->charsProcessed());
    }
    TinyGPSPlus* gps;
    int charsProcessed;
};
TEST_F(TestTinyGpsPlus, construct)
{
    
    EXPECT_EQ(0, gps->charsProcessed());
}
TEST_F(TestTinyGpsPlus, encodeRMC_NoFix)
{
    std::string s{"$GPRMC,175404.00,V,,,,,,,081019,,,N*7F\n"};
    encode(s);
    
    EXPECT_EQ(0, gps->sentencesWithFix());
    EXPECT_EQ(1, gps->passedChecksum());
    EXPECT_EQ(0, gps->failedChecksum());
}
TEST_F(TestTinyGpsPlus, encodeRMC_Fix)
{
    std::string s{"$GPRMC,175628.00,A,6504.56965,N,02529.16680,E,0.866,,081019,,,A*7D\n"};
    encode(s);
    EXPECT_EQ(1, gps->sentencesWithFix());
    EXPECT_EQ(1, gps->passedChecksum());
    EXPECT_EQ(0, gps->failedChecksum());
}
TEST_F(TestTinyGpsPlus, encodeGGA_NoSatellitesNoFix)
{
    EXPECT_EQ(false, gps->satellites.isValid());
    EXPECT_EQ(false, gps->satellites.isUpdated());
    std::string s{"$GPGGA,175405.00,,,,,0,00,99.99,,,,,,*64\n"};
    encode(s);
    EXPECT_EQ(0, gps->sentencesWithFix());
    EXPECT_EQ(1, gps->passedChecksum());
    EXPECT_EQ(0, gps->failedChecksum());
    EXPECT_EQ(true, gps->satellites.isValid());
    EXPECT_EQ(true, gps->satellites.isUpdated());
    EXPECT_EQ(0, gps->satellites.value());
}
TEST_F(TestTinyGpsPlus, encodeGGA_FiveSatellitesAndFix)
{
    std::string s{"$GPGGA,175628.00,6504.56965,N,02529.16680,E,1,05,3.69,117.3,M,21.0,M,,*56\n"};
    encode(s);
    EXPECT_EQ(1, gps->sentencesWithFix());
    EXPECT_EQ(1, gps->passedChecksum());
    EXPECT_EQ(0, gps->failedChecksum());
    EXPECT_EQ(true, gps->satellites.isValid());
    EXPECT_EQ(true, gps->satellites.isUpdated());
    EXPECT_EQ(5, gps->satellites.value());
}
TEST_F(TestTinyGpsPlus, encodeCustomGSV_NineSatsInViewOneId)
{
    std::string s{"$GPGSV,3,3,09,30,71,180,19*44\n"};
    TinyGPSCustom satsInView(*gps, "GPGSV", 3);
    TinyGPSCustom satId(*gps, "GPGSV", 4);
    encode(s);
    EXPECT_EQ(true, satsInView.isUpdated());
    EXPECT_EQ(true, satsInView.isValid());
    EXPECT_STREQ("09", satsInView.value());
    EXPECT_EQ(true, satId.isUpdated());
    EXPECT_EQ(true, satId.isValid());
    EXPECT_STREQ("30", satId.value());
}

TEST_F(TestTinyGpsPlus, encodeGSV_TwoSatsTwice)
{
    std::string s1{"$GPGSV,1,1,02,07,,,32,21,,,31*7C\n"};
    std::string s2{"$GPGSV,1,1,02,07,,,35,21,,,37*7C\n"};
    encode(s1);
    encode(s2);
    EXPECT_EQ(true, gps->satsInView.isUpdated());
    EXPECT_EQ(true, gps->satsInView.isValid());
    EXPECT_EQ(2, gps->satsInView.numOf());
    EXPECT_EQ(2, gps->satsInView.numOfDb());
    EXPECT_EQ(7, gps->satsInView[0].id());
    EXPECT_STREQ("35", gps->satsInView[0].snr().c_str());
    EXPECT_EQ(21, gps->satsInView[1].id());
    EXPECT_STREQ("37", gps->satsInView[1].snr().c_str());
}
TEST_F(TestTinyGpsPlus, encodeGSV_FourSats)
{
    std::string s{"$GPGSV,1,1,04,07,,,31,17,,,20,21,,,31,27,,,35*7E\n"};
    encode(s);
    EXPECT_EQ(true, gps->satsInView.isUpdated());
    EXPECT_EQ(true, gps->satsInView.isValid());
    EXPECT_EQ(4, gps->satsInView.numOf());
    EXPECT_EQ(7, gps->satsInView[0].id());
    EXPECT_STREQ("31", gps->satsInView[0].snr().c_str());
    EXPECT_EQ(17, gps->satsInView[1].id());
    EXPECT_STREQ("20", gps->satsInView[1].snr().c_str());
    EXPECT_EQ(21, gps->satsInView[2].id());
    EXPECT_STREQ("31", gps->satsInView[2].snr().c_str());
    EXPECT_EQ(27, gps->satsInView[3].id());
    EXPECT_STREQ("35", gps->satsInView[3].snr().c_str());
}
TEST_F(TestTinyGpsPlus, encodeGSV_NineSatsInThreeSentences)
{
    std::string s1{"$GPGSV,3,1,09,05,45,242,14,07,57,095,33,08,21,080,31,09,12,126,13*72\n"};
    std::string s2{"$GPGSV,3,2,09,13,39,278,27,15,09,295,,21,18,341,29,27,24,040,26*76\n"};
    std::string s3{"$GPGSV,3,3,09,30,71,180,22*4C\n"};
    encode(s1);
    encode(s2);
    encode(s3);
    EXPECT_EQ(true, gps->satsInView.isUpdated());
    EXPECT_EQ(true, gps->satsInView.isValid());
    EXPECT_EQ(9, gps->satsInView.numOf());
    EXPECT_EQ(5, gps->satsInView[0].id());
    EXPECT_STREQ("14", gps->satsInView[0].snr().c_str());
    EXPECT_EQ(7, gps->satsInView[1].id());
    EXPECT_STREQ("33", gps->satsInView[1].snr().c_str());
    EXPECT_EQ(8, gps->satsInView[2].id());
    EXPECT_STREQ("31", gps->satsInView[2].snr().c_str());
    EXPECT_EQ(9, gps->satsInView[3].id());
    EXPECT_STREQ("13", gps->satsInView[3].snr().c_str());
    EXPECT_EQ(13, gps->satsInView[4].id());
    EXPECT_STREQ("27", gps->satsInView[4].snr().c_str());
    EXPECT_EQ(15, gps->satsInView[5].id());
    EXPECT_STREQ("0", gps->satsInView[5].snr().c_str());
    EXPECT_EQ(21, gps->satsInView[6].id());
    EXPECT_STREQ("29", gps->satsInView[6].snr().c_str());
    EXPECT_EQ(27, gps->satsInView[7].id());
    EXPECT_STREQ("26", gps->satsInView[7].snr().c_str());
    EXPECT_EQ(30, gps->satsInView[8].id());
    EXPECT_STREQ("22", gps->satsInView[8].snr().c_str());
}
TEST_F(TestTinyGpsPlus, encodeGroundSpeed_empty)
{
    std::string s{"$GPVTG,,,,,,,,,N*30\n"};
    encode(s);
    EXPECT_EQ(true, gps->groundSpeed.isUpdated());
    EXPECT_EQ(true, gps->groundSpeed.isValid());
    EXPECT_EQ(0.0, gps->groundSpeed.value());
}
TEST_F(TestTinyGpsPlus, encodeVTG_groundSpeed)
{
    std::string s{"$GPVTG,,T,,M,0.866,N,1.605,K,A*29\n"};
    encode(s);
    EXPECT_EQ(true, gps->groundSpeed.isUpdated());
    EXPECT_EQ(true, gps->groundSpeed.isValid());
    EXPECT_DOUBLE_EQ(1.605, gps->groundSpeed.value());
}
TEST_F(TestTinyGpsPlus, encodeGSA_NoFixNoSatsNoDop)
{
    std::string s{"$GPGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99*30\n"};
    encode(s);
    EXPECT_EQ(true, gps->gsa.isUpdated());
    EXPECT_EQ(true, gps->gsa.isValid());
    EXPECT_STREQ("No", gps->gsa.fix());
    EXPECT_EQ(0, gps->gsa.numSats());
    EXPECT_DOUBLE_EQ(99.99, gps->gsa.pdop());
    EXPECT_DOUBLE_EQ(99.99, gps->gsa.hdop());
    EXPECT_DOUBLE_EQ(99.99, gps->gsa.vdop());
}
TEST_F(TestTinyGpsPlus, encodeGSA_3dFix7SatsDop)
{
    std::string s{"$GPGSA,A,3,30,08,21,07,05,27,13,,,,,,3.45,1.67,3.02*0C\n"};
    encode(s);
    EXPECT_EQ(true, gps->gsa.isUpdated());
    EXPECT_EQ(true, gps->gsa.isValid());
    EXPECT_STREQ("3D", gps->gsa.fix());
    EXPECT_EQ(7, gps->gsa.numSats());
    EXPECT_DOUBLE_EQ(3.45, gps->gsa.pdop());
    EXPECT_DOUBLE_EQ(1.67, gps->gsa.hdop());
    EXPECT_DOUBLE_EQ(3.02, gps->gsa.vdop());
    EXPECT_EQ(30, gps->gsa.sats()[0]);
}
