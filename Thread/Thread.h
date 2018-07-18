/*
 * Thread.h
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 */

#ifndef THREAD_H_
#define THREAD_H_

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "Runnable.h"

class Thread : public Runnable {
 public:
  Thread();
  Thread(Runnable* pRunnable);
  virtual ~Thread();

  void Start();  //pthread_create를 실행
  void Wait();
  virtual void Run();  //Thread가 실행하는 main 로직 구현 함수.

 private:
  pthread_t _thread;
  Runnable* _runnable;
  static void* Main(void* pInst);  //pInst를 Thread 타입으로 변환 후 Run() 함수 실행
};

#endif /* THREAD_H_ */
