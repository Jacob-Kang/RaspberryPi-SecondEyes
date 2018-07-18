/*
 * Message.h
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 *
 *      큐의 기본 데이터 타입. 상의 후 데이터 필드 추가해야됨
 *
 *
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "SEcommon.h"

class Message {
 public:
  Message();
  Message(prov_t _p, dest_t _d);
  Message(int size);
  virtual ~Message();

  void setDest(dest_t _d) {
    _dest = _d;
  }
  dest_t getDest();

  void setProv(prov_t _p) {
    _prov = _p;
  }
  prov_t getProv();

//	void setMSG(string _msg);
//	string getMSG();

  void setPriority(int _p);
  int getPriority();

  void setData(void* _pdata, int size) {
    memcpy(data, _pdata, size);
    dataSize = size;
  }

  unsigned char* getData() {
    return data;
  }

  int getDataSize() {
    return dataSize;
  }

  void setCMD(unsigned char c) {
    _cmd = c;
  }
  unsigned char getCMD() {
    return _cmd;
  }

  void initMessage() {
    _priority = 0;
  }  //message 초기화, 임시적 해결 방안
 private:
  unsigned char data[BUF_SIZE];  //buffer, 여러가지 의미의 버퍼
  int dataSize;
  int _priority;
  dest_t _dest;
  prov_t _prov;
  unsigned char _cmd;
};

#endif /* MESSAGE_H_ */
