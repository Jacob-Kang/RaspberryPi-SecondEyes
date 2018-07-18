/*
 * MotorManager.cpp
 *
 *  Created on: 2015. 1. 23.
 *      Author:  Chulho Kang
 */

#include "../Include/MotorManager.h"

MotorManager* MotorManager::m_MotorManager_instance = NULL;

MotorManager* MotorManager::get_instance() {
  if (m_MotorManager_instance == NULL) {
    cout << "MotorManager::Not create yet" << endl;
    m_MotorManager_instance = new MotorManager;
  }
  return m_MotorManager_instance;
}

MotorManager::MotorManager()
    : pwm1(0),
      pwm2(0),
      _x(0),
      _y(0) {
  // TODO Auto-generated constructor stub

}

MotorManager::~MotorManager() {
  // TODO Auto-generated destructor stub
}

#if IN_RASP

int MotorManager::SetMortor() {
  if (wiringPiSetup() < 0)
    return 1;
  pwm1 = MID_PWM;
  pwm2 = MID_PWM;
  softPwmCreate(PIN1, 0, DUTY_CYCLE);
  softPwmCreate(PIN2, 0, DUTY_CYCLE);
  softPwmWrite(PIN1, MID_PWM);
  softPwmWrite(PIN2, MID_PWM);
  delay(DELAY_TIME);
}

void MotorManager::QuitMotor() {
  softPwmStop(PIN1);
  softPwmStop(PIN2);
  delay(DELAY_TIME);
}

void MotorManager::RestartMotor() {
  softPwmCreate(PIN1, 0, DUTY_CYCLE);
  softPwmCreate(PIN2, 0, DUTY_CYCLE);
  softPwmWrite(PIN1, pwm1);
  softPwmWrite(PIN2, pwm2);
  delay(DELAY_TIME);
}
void MotorManager::MoveMortor(packet_cntrl_motor _recvPkg) {

  _x = MID_PWM - (_recvPkg.left + _recvPkg.right) * 83;
  _y = MID_PWM - (_recvPkg.down + _recvPkg.up) * 100;

  if (MIN_PWM <= _x <= MAX_PWM && MIN_PWM <= _y <= MAX_PWM) {
    softPwmWrite(PIN1, _x);
    softPwmWrite(PIN2, _y);
    delay(DELAY_TIME);
  } else {
    printf("input is out of range\n");
  }
}

#endif
void MotorManager::Run() {
  cout << "MotorManager Start" << endl;
  while (1) {

    int rc = pthread_mutex_lock(&mutex_MotorManager);
    if (rc)
      continue;
    else
      cout << "MotorManagerLock" << endl;
    pthread_cond_wait(&cond_MotorManager, &mutex_MotorManager);
    cout << "MotorManagerUnLock" << endl;
///////////////////////////  Critical Section  ////////////////////////////////////////////////////////////////

    while ((receiveMessage = ReceiveMessage()).getPriority() != 0) {  //존재하는 메시지 모두 처리
      cout << "Received msg" << endl;
      memcpy(&recv_pkg, (packet_cntrl_motor*) receiveMessage.getData(),
             sizeof(packet_cntrl_motor));

      if (receiveMessage.getCMD() == S2M_FIRST_START) {
        cout << "MotorManager::first set" << endl;
#if IN_RASP
        SetMortor();
#endif
        break;
      } else if (receiveMessage.getCMD() == S2M_RESTART) {
        cout << "MotorManager::set restart" << endl;
#if IN_RASP
        RestartMotor();
#endif
        break;
      } else if (receiveMessage.getCMD() == S2M_STOP) {
        cout << "MotorManager::quitMotor" << endl;
#if IN_RASP
        QuitMotor();
#endif
        break;
      }

      cout << "up = " << recv_pkg.up << "down = " << recv_pkg.down << "left = "
           << recv_pkg.left << "right = " << recv_pkg.right << endl;
      MoveMortor(recv_pkg);
#if IN_RASP

#if POWERMEASURE
      cout << "측정 모드" << endl;
      if(recv_pkg.up == 0x1000) {
        int count = 0;
        cout << "모터 id = " << recv_pkg.up << endl;
        while(count <5) {
          if (5 < pwm1) {
            softPwmWrite(PIN1, (pwm1 - 1));
            delay(200 - (pwm1 - 1));
            pinMode(PIN1, OUTPUT);
            digitalWrite(PIN1, 0);
            softPwmCreate(PIN1, 0, 200);
            pwm1--;
            break;
          } else {
            softPwmWrite(PIN1, pwm1);
            softPwmCreate(PIN1, 0, 200);  //delay(10);//delay(200 - pwm1);
            break;
          }
          count++;
        }
        count = 0;
        while(count <5) {
          if (25 > pwm1) {
            softPwmWrite(PIN1, (pwm1 + 1));
            delay(200 - (pwm1 + 1));
            digitalWrite(PIN1, 0);
            softPwmCreate(PIN1, 0, 200);
            pwm1++;
            break;
          } else {
            softPwmWrite(PIN1, pwm1);
            softPwmCreate(PIN1, 0, 200);  //delay(10);//delay(200 - pwm1);
            break;
          }
          count++;
        }
        count = 0;
      }
#endif

#endif
    }
///////////////////////////  Critical Section  ////////////////////////////////////////////////////////////////
    pthread_mutex_unlock(&mutex_MotorManager);
    usleep(1000);
  }
}

