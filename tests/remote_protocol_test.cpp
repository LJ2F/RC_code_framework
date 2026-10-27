#include "Remote_receiver.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

static void Require(bool ok, const char* reason)
{
    if(!ok) { std::fprintf(stderr, "FAIL: %s\n", reason); std::exit(1); }
}
// Independent polynomial-division reference for CRC-8/DVB-S2 (check=0xBC).
static uint8_t ReferenceCrc(const uint8_t* data, size_t len)
{
    uint16_t remainder = 0U;
    for(size_t i=0; i<len; ++i) {
        remainder ^= static_cast<uint16_t>(data[i]) << 8U;
        for(int b=0; b<8; ++b)
            remainder = (remainder & 0x8000U) ?
                static_cast<uint16_t>((remainder << 1U) ^ 0xD500U) :
                static_cast<uint16_t>(remainder << 1U);
    }
    return static_cast<uint8_t>(remainder >> 8U);
}
static std::array<uint8_t,16> Frame(std::array<uint16_t,4> channels)
{
    std::array<uint8_t,16> bytes{{0xAA,0x55}};
    for(size_t i=0;i<4;++i) {
        bytes[2+i*2] = static_cast<uint8_t>(channels[i]);
        bytes[3+i*2] = static_cast<uint8_t>(channels[i] >> 8U);
    }
    bytes[10]=0x00; bytes[11]=0x80; bytes[12]=2; bytes[13]=7;
    bytes[14]=ReferenceCrc(bytes.data()+2,12); bytes[15]=0xDE;
    return bytes;
}
static void Count(const Remote::State&, void* user) { ++*static_cast<int*>(user); }

int main()
{
    const uint8_t check[]={'1','2','3','4','5','6','7','8','9'};
    Require(ReferenceCrc(check,9)==0xBC, "reference CRC check");
    Require(Remote::Remote_receiver::crc8(check,9)==0xBC, "decoder CRC check");
    int notifications=0;
    Remote::Remote_receiver rx(Count,&notifications,350U);
    Require(rx.get_state().online==0, "initially offline");
    auto valid=Frame({{0,2048,4095,1824}});
    rx.feed(valid.data(),valid.size(),100U);
    Require(notifications==1 && rx.get_state().online, "valid frame accepted");
    for(int i=0;i<4;++i)
        Require(rx.get_state().ch[i]==std::array<int,4>{{0,2048,4095,1824}}[i], "raw channels unchanged");
    Require(rx.get_state().key==0x8000 && rx.get_state().page==2, "metadata");
    auto bad=valid; bad[14]^=1;
    rx.feed(bad.data(),bad.size(),110U);
    Require(notifications==1 && rx.get_state().last_rx_ms==100U, "bad CRC rejected");
    rx.feed(valid.data(),3U,120U);
    Require(notifications==1, "partial frame buffered");
    rx.feed(valid.data()+3,valid.size()-3,121U);
    Require(notifications==2 && rx.get_state().last_rx_ms==121U, "split frame accepted");
    const uint8_t noise[]={0,0xAA,0,0x55,0xAA};
    rx.feed(noise,sizeof(noise),130U);
    rx.feed(valid.data(),valid.size(),131U);
    Require(notifications==3, "resynchronizes after noise");
    auto invalid=Frame({{65535,2048,2048,2048}});
    rx.feed(invalid.data(),invalid.size(),140U);
    Require(notifications==3, "out of range channel rejected before narrowing");
    auto invalid_page=valid; invalid_page[12]=4; invalid_page[14]=ReferenceCrc(invalid_page.data()+2,12);
    rx.feed(invalid_page.data(),invalid_page.size(),141U);
    Require(notifications==3, "invalid page rejected");
    rx.poll(480U); Require(rx.get_state().online, "before timeout");
    rx.poll(481U); Require(!rx.get_state().online && notifications==4, "timeout at boundary");
    rx.poll(900U); Require(notifications==4, "offline notification only once");
    rx.feed(valid.data(),valid.size(),1000U);
    rx.break_stream(); Require(!rx.get_state().online, "break stream");
    rx.feed(valid.data(),valid.size(),UINT32_MAX-100U);
    rx.poll(248U); Require(rx.get_state().online, "wrap before timeout");
    rx.poll(249U); Require(!rx.get_state().online, "wrap timeout");
    std::puts("PASS: CRC, raw channels, framing, resync, bounds, disconnect, timeout, wraparound");
}
