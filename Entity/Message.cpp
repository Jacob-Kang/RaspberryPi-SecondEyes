/*
 * Message.cpp
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 */

#include "../Include/Message.h"

//기본 생성자는 오류를 나타낸다.(비어있는 메시지)
Message::Message()
    : _dest(0),
      _prov(0),
      _priority(0),
      dataSize(0),
      _cmd(0) {
}

Message::Message(prov_t _p, dest_t _d)
    : _dest(_d),
      _prov(_p),
      _priority(0),
      dataSize(0),
      _cmd(0) {
}

Message::Message(int size)
    : _dest(0),
      _prov(0),
      _priority(0),
      _cmd(0) {
  dataSize = size;
}

Message::~Message() {

}

dest_t Message::getDest() {
  return _dest;
}

prov_t Message::getProv() {
  return _prov;
}

void Message::setPriority(int _p) {
  _priority = _p;
}

int Message::getPriority() {
  return _priority;
}

