/*
 * CommunicationService.h
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 *
 *      CommunicationService는 Server로부터 받은 Packet을 CommunicationManager에게 전달한다.
 *      CommunicationManager로부터 받은 메시지를 SecondEyes로 전달한다.
 *
 */

#ifndef COMMUNICATIONSERVICE_H_
#define COMMUNICATIONSERVICE_H_

#include "TaskManager.h"
#include "ClientSocket.h"
#include "CameraManager.h"
#include "SEcommon.h"
#include "OpenMAX.h"
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>

class CommunicationService : public TaskManager {
 public:
  static CommunicationService* createnew(void);

  CommunicationService();
  virtual ~CommunicationService();
  virtual void Run();
  void ParsingMessage();
  void GetMACAddress();
 protected:
  virtual ReceiveQueue* CreateMessageQueue() {
    _task = COMMUNICATIONSERVICE;
    return new ReceiveQueue();
  }

 private:
  static CommunicationService* m_CommunicationService_instance;
  void SendMessageToStreamManager(Message msg, int priority);
  void SendMessageToMotorManager(Message msg, int priority);
  Message sendMessage;  //queue send message
  Message receiveMessage;  //queue receive message
  ClientSocket* client;
  CameraManager* camera;
  OpenMAX* omx;
  char MAC[18];
};

#endif /* COMMUNICATIONSERVICE_H_ */
