#include "transfer_protocol.hpp"
#include "StreamTransport.hpp"
#include "check.hpp"
#include <array>
#include <vector>
#include <sstream>
#include <iterator>
using Packet=gdut::data_packet<gdut::crc16_algorithm>;
struct Counting:std::pmr::memory_resource {
 size_t active=0,peak=0;
 void* do_allocate(size_t n,size_t a) override {
   active+=n;peak=std::max(peak,active);
   return std::pmr::new_delete_resource()->allocate(n,a);
 }
 void do_deallocate(void* p,size_t n,size_t a) override {
   CHECK(active>=n);active-=n;std::pmr::new_delete_resource()->deallocate(p,n,a);
 }
 bool do_is_equal(const std::pmr::memory_resource& o) const noexcept override {return &o==this;}
};
static uint16_t reference(const uint8_t* d,size_t n) {
 uint16_t c=0xFFFF;
 for(size_t i=0;i<n;++i) {
   if(i==6 || i==7) continue;
   c^=d[i];
   for(int b=0;b<8;++b)c=(c&1)?uint16_t((c>>1)^0xA001):uint16_t(c>>1);
 }
 return c;
}
int main(){
 Counting resource;
 auto* old=std::pmr::set_default_resource(&resource);
 {
 std::array<uint8_t,gdut::max_payload_size> data{};data[0]=1;data.back()=0xAA;
 Packet maximum(0x1234,data.begin(),data.end(),gdut::build_packet);
 CHECK(maximum && maximum.size()==512 && maximum.body_size()==502);
 CHECK(maximum.code()==0x1234 && maximum.crc()==reference(maximum.data(),maximum.size()));
 CHECK(maximum.body_end()-maximum.body_begin()==502 && maximum.body_end()[-1]==0xAA);
 Packet copy(maximum.begin(),maximum.end(),gdut::from_whole_packet);CHECK(copy);
 std::array<uint8_t,503> oversized{};
 Packet invalid(1,oversized.begin(),oversized.end(),gdut::build_packet);CHECK(!invalid);
 Packet empty(0,data.begin(),data.begin(),gdut::build_packet);CHECK(empty && empty.size()==10);
 std::istringstream stream("abc");
 Packet input(3,std::istreambuf_iterator<char>(stream),std::istreambuf_iterator<char>(),gdut::build_packet);
 CHECK(input && input.body_size()==3 && input.body_begin()[2]=='c');
 gdut::data_packet<gdut::checksum_algorithm> sum(7,data.begin(),data.begin()+3,gdut::build_packet);
 CHECK(sum.verify_verification());
 gdut::packet_manager<gdut::crc16_algorithm> manager;
 int received=0; auto* count=&received;
 manager.set_receive_function([count](Packet p){CHECK(p.verify_verification());++*count;});
 CHECK(!manager.send(maximum));
 manager.set_send_function([](const uint8_t*,const uint8_t*){return false;});
 CHECK(!manager.send(maximum));
 manager.set_send_function([](const uint8_t* a,const uint8_t* b){return b-a==512;});
 CHECK(manager.send(maximum) && !manager.send(invalid));
 for(size_t split=0;split<=maximum.size();++split) {
   manager.receive(maximum.begin(),maximum.begin()+split);
   manager.receive(maximum.begin()+split,maximum.end());
 }
 CHECK(received==513 && manager.buffered_size()==0);
 std::vector<uint8_t> noise{0,0xAA,0xAA,0x55,0xFF,0xFF,1,2};
 auto bad=std::vector<uint8_t>(maximum.begin(),maximum.end());bad[6]^=0x12;
 noise.insert(noise.end(),bad.begin(),bad.end());bad.back()=0;
 noise.insert(noise.end(),bad.begin(),bad.end());
 noise.insert(noise.end(),empty.begin(),empty.end());
 noise.insert(noise.end(),maximum.begin(),maximum.end());
 manager.receive(noise.begin(),noise.end());CHECK(received==515);
 // Corrupt outer frame enclosing a complete valid inner frame must resynchronize bytewise.
 std::vector<uint8_t> nested(30,0);nested[0]=0xAA;nested[1]=0x55;nested[3]=30;
 std::copy(empty.begin(),empty.end(),nested.begin()+8);
 manager.receive(nested.begin(),nested.end());CHECK(received==516);
 int delivered=0;
 gdut::StreamTransport<gdut::crc16_algorithm> transport(
   [](const uint8_t*,size_t n,void*){return n<=512;},nullptr,
   [](uint16_t code,std::span<const uint8_t> body,void* u){
     CHECK(code==0x1234 && body.size()==502);++*static_cast<int*>(u);
   },&delivered);
 transport.input(maximum.data(),maximum.size());CHECK(delivered==1);
 CHECK(transport.send(1,std::span<const uint8_t>(data)));
 manager.receive(maximum.begin(),maximum.begin()+6);CHECK(manager.buffered_size()==6);
 manager.reset_receive();CHECK(manager.buffered_size()==0);
 const auto stable=resource.active;
 for(int i=0;i<10000;++i) {
   manager.receive(maximum.begin(),maximum.end());
   CHECK(manager.buffered_size()<=gdut::max_frame_size);
 }
 CHECK(resource.active==stable);
 gdut::function<int(int)> f=[](int x){return x+1;};
 auto g=f;auto h=std::move(g);CHECK(h(3)==4 && !g);f=nullptr;CHECK(!f);
 }
 CHECK(resource.active==0);std::pmr::set_default_resource(old);
 std::printf("protocol: passed; PMR peak=%zu bytes; all allocations released\n",resource.peak);
}
