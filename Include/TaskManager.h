/*
 * TaskManager.h
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 *
 *
 *      ( TaskManager들은 각각 필요한 큐를 CommonEntity의 queue 클래스를 재정의하여 사용한다. )
 *      *각 TaskManager는 자신을 QuqueManager에게 자신의 식별자를 등록해야된다. ( 초기화 과정 )*
 *      *각 TaskManager는 자신의 큐를 생성하여 등록한다.*
 *     각 task들은 해당 클래스를 상속받는다.
 *
 */

#ifndef TASKMANAGER_H_
#define TASKMANAGER_H_

#include "QueueManager.h"
#include "SEcommon.h"
#include "../Thread/Thread.h"

class TaskManager : public Thread {
 public:
  TaskManager();
  virtual ~TaskManager();
  void initManager(QueueManager* _qManager);
  Message ReceiveMessage();	//자신의 큐에서 메시지를 읽어온다.
  void SendMessage(Message s_msg);	//서비스큐에 메시지를 등록한다.
  int getSize();
 protected:
  // 상속받은 클래스만 사용할 수 있도록
  //자신의 큐를 등록해야됨, 상속받아서 구현해야된다. 반환값으로 큐의 포인터값 반환(동적할당 해야됨) task번호 초기화 해야됨
  virtual ReceiveQueue* CreateMessageQueue() = 0;
  task_t _task;  //task 식별자
  QueueManager* p_qManager;		//전역객체 포인터
  ReceiveQueue* _receiveQueue;	//receive queue 식별자
 private:
  inline bool IsInit();
  inline bool ReceiveQueueIsEmpty();

};

#endif /* TASKMANAGER_H_ */
