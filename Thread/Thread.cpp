/*
 * Thread.cpp
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 */

#include "Thread.h"

Thread::Thread()
    : _thread(0),
      _runnable(0) {
  // TODO Auto-generated constructor stub

}

Thread::Thread(Runnable* pRunnable)
    : _thread(0),
      _runnable(pRunnable) {

}

Thread::~Thread() {
  // TODO Auto-generated destructor stub
}

/* pthread_create 함수를 객체의 맴버함수에서 실행시키기 위한 해결방안 */
void* Thread::Main(void* pInst) {
  //static_cast => void* 타입 변환시 사용
  Thread* pt = static_cast<Thread*>(pInst);
  pt->Run();
  return 0;
}

void Thread::Start() {
  pthread_create(&_thread, NULL, &Thread::Main, this);
  if (_thread < 0) {
    perror("thread create error");
    exit(0);
  }
}

void Thread::Wait() {
  void* pData;
  pthread_join(_thread, &pData);
}

void Thread::Run() {
  if (_runnable != 0) {
    _runnable->Run();
  }
}

