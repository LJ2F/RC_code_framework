/**
 * Derived from XboxRemote, 老冯 / Keten (2863861004@qq.com), 2026.
 */
#pragma once
#include <cstdint>
#include <cstddef>
class UartPort;
class XboxRemote {
public:
  struct xbox_controller_data_t {
    // 按键数据（bool类型）
    bool btnY;
    bool btnY_last;
    bool btnB;
    bool btnB_last;
    bool btnA;
    bool btnA_last;
    bool btnX;
    bool btnX_last;
    bool btnShare;
    bool btnShare_last;
    bool btnStart;
    bool btnStart_last;
    bool btnSelect;
    bool btnSelect_last;
    bool btnXbox;
    bool btnXbox_last;
    bool btnLB;
    bool btnLB_last;
    bool btnRB;
    bool btnRB_last;
    bool btnLS;
    bool btnLS_last;
    bool btnRS;
    bool btnRS_last;
    bool btnDirUp;
    bool btnDirUp_last;
    bool btnDirLeft;
    bool btnDirLeft_last;
    bool btnDirRight;
    bool btnDirRight_last;
    bool btnDirDown;
    bool btnDirDown_last;

    // 霍尔值（16位数值）
    uint16_t joyLHori;
    uint16_t joyLVert;
    uint16_t joyRHori;
    uint16_t joyRVert;
    uint16_t trigLT;
    uint16_t trigRT;

    float joyLHori_map;
    float joyLVert_map;
    float joyRHori_map;
    float joyRVert_map;
    float trigLT_map;
    float trigRT_map;
  };

  using Checksum=bool(*)(const uint8_t* whole_frame,size_t len,void* user);
  XboxRemote()=default;
  explicit XboxRemote(UartPort&) {}
  uint8_t init();
  uint8_t processByte(uint8_t byte);
  uint8_t processByte(uint8_t byte,uint32_t now_ms);
  uint8_t xbox_process(uint8_t byte){return processByte(byte);}
  const xbox_controller_data_t& getControllerData() const{return controller_data_;}
  void setChecksum(Checksum f,void* user=nullptr){check_=f;user_=user;verified_=false;}
  void setTimeout(uint32_t ms){timeout_ms_=ms;}
  bool online(uint32_t now_ms) const{return seen_ && uint32_t(now_ms-last_ms_)<timeout_ms_;}
  bool crcVerified() const{return verified_;}
  bool button_continue(bool current,bool* last);
  bool button_switch(bool current,bool* last);
private:
  void onFrameComplete(const uint8_t*,uint8_t);
  uint8_t frame_[36]{};
  size_t used_{0};
  xbox_controller_data_t controller_data_{};
  Checksum check_{};
  void* user_{};
  uint32_t last_ms_{0},timeout_ms_{350};
  bool seen_{false},verified_{false};
};
