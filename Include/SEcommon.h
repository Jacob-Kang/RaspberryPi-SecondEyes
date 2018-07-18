/*
 * SEcommon.h
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 */

#ifndef SECOMMON_H_
#define SECOMMON_H_

#include <unistd.h>
#include <iostream>
#include <string>
#include <string.h>
#include "IL/OMX_Types.h"
using std::string;
using std::cout;
using std::endl;

typedef struct {
  int nSize; /* sizeof(SEImage) */
  int width;
  int height;
  OMX_U8* pData; /* Pointer to aligned image data. */
  char colorModel[4]; /* image Format */
} SEImage;

#define SE_DEBUG							0		//debug flag
#define SERVER_PORT						1201 //controllet connection port number
#define UBUNTU							0
#define IN_RASP							1
#define POWERMEASURE						0

//Manager 식별자
#define MAINTASKMANAGER					1
#define COMMUNICATIONTASKMANAGER		2
#define COMMUNICATIONSERVICE			3
#define EVENTDETECTMANAGER				4
#define STREAMMANAGER					5
#define MOTORMANAGER						6
#define CONVERTINGMANAGER				8

#define CAPTUREVIDEO						1
#define CAPTUREIMAGE						2

#define S2M_FIRST_START					0xa1
#define S2M_RESTART						0xa2
#define S2M_STOP							0xa3

#define MAX_WIDTH							1920
#define MAX_HEIGHT						1080
// 고정 값
#define MAX_FPS							30

typedef int task_t;  //task 구분자
typedef int dest_t;  // Message 목적지를 나타냄
typedef int prov_t;  // Message 제공자를 나타냄
typedef int s_32;
typedef unsigned int u_32;
typedef char s_8;
typedef unsigned char u_8;

class QueueManager;
extern QueueManager qm;

extern int camera_id;
extern pthread_cond_t cond_communication_task;  //communication task manager condition
extern pthread_mutex_t mutex_communication_task;  //communication task manager mutex

extern pthread_cond_t cond_StreamManager;
extern pthread_mutex_t mutex_StreamManager;

extern pthread_cond_t cond_MotorManager;
extern pthread_mutex_t mutex_MotorManager;

extern pthread_mutex_t mutex_ConvertingManager;

extern pthread_mutex_t mutex_EventManager;

extern char* sock_addr;
extern char* sock_port;

#define BUF_SIZE 1024

#endif /* COMMON_H_ */
