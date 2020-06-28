
#include "gtest/gtest.h"
#include "TinyGPS++.h"

class TestChecksums : public ::testing::Test
{
protected:
    std::string getChecksum(const std::string& header, const std::string& sentence)
    {
        std::ostringstream ret;
        ret << "*";
        uint8_t parity{0};
        for (const char& c : sentence)
        {
            if(c == '$') continue;
            parity ^= c;
            //std::cout << c << ": 0x" << std::hex << (int) parity << std::endl;
        }
        std::cout << header << ": " << sentence << "*" << std::hex << (int) parity << std::endl;
        ret << std::hex << (int) parity;
        return ret.str();
    }
    std::pair<uint8_t, uint8_t> getChecksumBinary(const std::string& header, const std::vector<uint8_t>& sentence)
    {
        std::pair<uint8_t, uint8_t> chk_sum{};
        std::ostringstream sentenceStr{};
        sentenceStr << "0x" << std::hex << (int) sentence[0];
        sentenceStr << ", 0x" << std::hex << (int) sentence[1];
        for (size_t i = 2; i < sentence.size(); i++)
        {
            sentenceStr << ", 0x" << std::hex << (int) sentence[i];
            chk_sum.first = chk_sum.first + sentence[i];
            chk_sum.second = chk_sum.second + chk_sum.first;
        }
        sentenceStr << ", 0x" << std::hex << (int) chk_sum.first;
        sentenceStr << ", 0x" << std::hex << (int) chk_sum.second;

        std::cout << header << ": " << sentenceStr.str() << std::endl;
        return chk_sum;
    }
};

TEST_F(TestChecksums, getChecksum)
{
    EXPECT_EQ(std::string("*18"), getChecksum("baudrate to 115200", "$PUBX,41,1,0007,0003,115200,0"));
    EXPECT_EQ(std::string("*4d"), getChecksum("some random GSV", "$GPGSV,4,4,13,32,08,058,20"));
    getChecksum("switch off GLL", "$PUBX,40,GLL,1,0,0,0,0,0");
    getChecksum("switch on GLL", "$PUBX,40,GLL,1,1,0,0,0,0");
    getChecksum("switch off GSV", "$PUBX,40,GSV,1,0,0,0,0,0");
}
TEST_F(TestChecksums, getChecksumBinary)
{
    getChecksumBinary("100ms rate",  {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x64, 0x00, 0x01, 0x00, 0x01, 0x00});
    getChecksumBinary("5000ms rate", {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x88, 0x13, 0x01, 0x00, 0x01, 0x00});
}
