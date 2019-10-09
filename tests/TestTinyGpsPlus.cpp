
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
    }
    TinyGPSPlus* gps;
};
TEST_F(TestTinyGpsPlus, construct)
{
    
    EXPECT_EQ(0, gps->charsProcessed());
}
TEST_F(TestTinyGpsPlus, encodeInvalidRMC)
{
    std::string s{"$GPRMC,175404.00,V,,,,,,,081019,,,N*7F\n"};
    encode(s);
    EXPECT_EQ(s.size(), gps->charsProcessed());
    EXPECT_EQ(0, gps->sentencesWithFix());
    EXPECT_EQ(1, gps->passedChecksum());
    EXPECT_EQ(0, gps->failedChecksum());
}

