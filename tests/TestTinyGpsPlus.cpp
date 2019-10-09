
#include "gtest/gtest.h"
#include "TinyGPS++.h"

class TestTinyGpsPlus : public ::testing::Test
{
public:
    TestTinyGpsPlus()
    : gps{nullptr}
    {}
    void SetUp()
    {
        gps = new TinyGPSPlus();
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
        EXPECT_EQ(s.size(), gps->charsProcessed());
    }
    TinyGPSPlus* gps;
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