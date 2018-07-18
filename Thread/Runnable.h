/*
 * Runnable.h
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 */

#ifndef RUNNABLE_H_
#define RUNNABLE_H_

/*
 * Interface Runnable
 * Thread 클래스에서 상속받는다.
 */

class Runnable {
 public:
  virtual void Run() = 0;
};

#endif /* RUNNABLE_H_ */
