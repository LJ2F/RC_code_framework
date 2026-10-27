// Derived from Keten's XboxRemote.cpp, Copyright (c) 2026.
#include "XboxRemote.hpp"
#include "stm32h7xx_hal.h"
#include <cstring>
uint8_t XboxRemote::init() {
  used_=0; controller_data_={}; seen_=verified_=false; return 1;
}
uint8_t XboxRemote::processByte(uint8_t byte){return processByte(byte,HAL_GetTick());}
uint8_t XboxRemote::processByte(uint8_t byte,uint32_t now_ms) {
  frame_[used_++]=byte;
  for(;;) {
    if(used_==0) return 0;
    const bool bad=frame_[0]!=0xFC || (used_>=2 && frame_[1]!=0xFB) ||
      (used_>=3 && frame_[2]!=0x01) || (used_>=4 && frame_[3]!=28);
    if(!bad && used_<sizeof frame_) return 0;
    if(!bad && frame_[34]==0xFD && frame_[35]==0xFE &&
       (!check_ || check_(frame_,sizeof frame_,user_))) {
      onFrameComplete(frame_+4,28);
      used_=0;last_ms_=now_ms;seen_=true;verified_=check_!=nullptr;return 1;
    }
    std::memmove(frame_,frame_+1,--used_);
  }
}
void XboxRemote::onFrameComplete(const uint8_t *data, uint8_t len) {
  if (data == nullptr || len < 28) {
    return;
  }

  // 解析按键数据
  controller_data_.btnY = data[0];
  controller_data_.btnB = data[1];
  controller_data_.btnA = data[2];
  controller_data_.btnX = data[3];
  controller_data_.btnShare = data[4];
  controller_data_.btnStart = data[5];
  controller_data_.btnSelect = data[6];
  controller_data_.btnXbox = data[7];
  controller_data_.btnLB = data[8];
  controller_data_.btnRB = data[9];
  controller_data_.btnLS = data[10];
  controller_data_.btnRS = data[11];
  controller_data_.btnDirUp = data[12];
  controller_data_.btnDirLeft = data[13];
  controller_data_.btnDirRight = data[14];
  controller_data_.btnDirDown = data[15];

  // 解析霍尔传感器值（16位，大端）
  controller_data_.joyLHori = (static_cast<uint16_t>(data[16]) << 8) | data[17];
  controller_data_.joyLVert = (static_cast<uint16_t>(data[18]) << 8) | data[19];
  controller_data_.joyRHori = (static_cast<uint16_t>(data[20]) << 8) | data[21];
  controller_data_.joyRVert = (static_cast<uint16_t>(data[22]) << 8) | data[23];
  controller_data_.trigLT = (static_cast<uint16_t>(data[24]) << 8) | data[25];
  controller_data_.trigRT = (static_cast<uint16_t>(data[26]) << 8) | data[27];
}

bool XboxRemote::button_continue(bool currentBtnState, bool *lastBtnState) {
  if (!lastBtnState) return false;
  bool result = false;
  if (currentBtnState && !(*lastBtnState)) {
    result = true;
  } else if (!currentBtnState && (*lastBtnState)) {
    result = false;
  }
  *lastBtnState = currentBtnState;
  return result;
}

bool XboxRemote::button_switch(bool currentBtnState, bool *lastBtnState) {
  if (!lastBtnState) return false;
  if (currentBtnState && !(*lastBtnState)) {
    *lastBtnState = currentBtnState;
    return true;
  }
  *lastBtnState = currentBtnState;
  return false;
}
