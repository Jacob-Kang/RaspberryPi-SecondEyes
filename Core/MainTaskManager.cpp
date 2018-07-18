/*
 * MainTaskManager.cpp
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 */

#include "../Include/MainTaskManager.h"

QueueManager qm;

pthread_cond_t cond_MotorManager;  //communication task manager condition
pthread_mutex_t mutex_MotorManager;  //communication task manager mutex

pthread_cond_t cond_StreamManager;
pthread_mutex_t mutex_StreamManager;

pthread_mutex_t mutex_EventManager;

bool flag_stream = false;
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

MainTaskManager::MainTaskManager() {
  // TODO Auto-generated constructor stub

}

MainTaskManager::~MainTaskManager() {
  // TODO Auto-generated destructor stub
}

void MainTaskManager::Run() {
  pthread_mutex_init(&mutex_MotorManager, NULL);
  pthread_cond_init(&cond_MotorManager, NULL);

  pthread_mutex_init(&mutex_StreamManager, NULL);
  pthread_cond_init(&cond_StreamManager, NULL);

  pthread_mutex_init(&mutex_EventManager, NULL);

  QueueServiceManager qsm(&qm);

  CommunicationService cs;
  EventManager em;
  StreamManager sm;
  MotorManager mm;

  cs.initManager(&qm);	// queue 등록
  em.initManager(&qm);
  sm.initManager(&qm);
  mm.initManager(&qm);

  qsm.Start();
  em.Start();
  sm.Start();
  cs.Start();
  mm.Start();

  qsm.Wait();
  em.Wait();
  sm.Wait();
  cs.Wait();
  mm.Wait();
}
