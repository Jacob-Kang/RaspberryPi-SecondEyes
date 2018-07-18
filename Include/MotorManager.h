/*
 * MotorManager.h
 *
 *  Created on: 2015. 1. 23.
 *      Author:  Chulho Kang
 */

#ifndef CAMERA_MOTORMANAGER_H_
#define CAMERA_MOTORMANAGER_H_
#include "TaskManager.h"
#include <string.h>
#include "SEcommon.h"
#include "ExternalProtocol.h"
#include "ClientSocket.h"

// 모터제어
#if IN_RASP
#include <wiringPi.h>
#include <softPwm.h>
#endif

#define MID_PWM 		1500		//middle point of duty cycle
#define MAX_PWM		2500
#define MIN_PWM 		500
#define DUTY_CYCLE 	20000		//term of duty cycle
#define DELAY_TIME 	50			//delay term for 1 duty cycle
#define TERM 			50			//speed 1
#define TERM2 		75			//speed 2
#define PIN1 			1 			//GPIO 18
#define PIN2 			2 			//GPIO 27

class MotorManager : public TaskManager {
 public:

  MotorManager();
  virtual ~MotorManager();

  int SetMortor();
  void QuitMotor();
  void RestartMotor();

  void MoveMortor(packet_cntrl_motor _recvPkg);

  void ParsingMotor(packet_cntrl_motor _recvPkg);

  static MotorManager* get_instance();
  virtual void Run();

 protected:
  virtual ReceiveQueue* CreateMessageQueue() {
    _task = MOTORMANAGER;
    return new ReceiveQueue();
  }

 private:
  static MotorManager* m_MotorManager_instance;

  Message receiveMessage;  //queue receive message
  packet_cntrl_motor recv_pkg;
  int pwm1;
  int pwm2;

  int _x, _y;
};

#endif /* CAMERA_MOTORMANAGER_H_ */
