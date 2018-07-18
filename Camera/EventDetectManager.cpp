/*
 * EventManager.cpp
 *
 *  Created on: 2015. 1. 23.
 *      Author:  Chulho Kang
 */

#include "../Include/EventDetectManager.h"
#include <linux/videodev2.h>

extern bool flag_stream;

EventManager* EventManager::m_EventManager = NULL;

EventManager* EventManager::get_instance() {
  if (m_EventManager == NULL) {
    cout << "EventManager::Not creat yet" << endl;
    m_EventManager = new EventManager;
  }
  return m_EventManager;
}

EventManager::EventManager() {
  // TODO Auto-generated constructor stub
  camera = NULL;
  client = NULL;
  omx = NULL;
  memset(fileName, 0, sizeof(fileName));
  threshold1 = 40;
  threshold2 = 300;
  memset(&image, 0, sizeof(SEImage));		//영상을 받은 이미지 변수
}

EventManager::~EventManager() {
  // TODO Auto-generated destructor stub
}

void EventManager::MakeFileName() {

  if (camera_id < 10) {
    sprintf(fileName, "%d", 0);
    sprintf(fileName + 1, "%d", camera_id);
  } else
    sprintf(fileName, "%d", camera_id);
  sprintf(fileName + 2, "_");
  strcpy(fileName + 3, camera->getEvent_date());
  sprintf(fileName + 14, ".");
  sprintf(fileName + 15, "m");
  sprintf(fileName + 16, "p");
  sprintf(fileName + 17, "4");
}

int EventManager::BackgroundSubtraction() {
  omx->SwitchCapturePort(72, OMX_TRUE);
  memcpy(&temp, omx->OMXCaptureFrame(), sizeof(SEImage));
  omx->SwitchCapturePort(72, OMX_FALSE);
  memcpy(&dest, &temp, sizeof(SEImage));

  omx->SwitchCapturePort(72, OMX_TRUE);
  memcpy(&image, omx->OMXCaptureFrame(), sizeof(SEImage));
  omx->SwitchCapturePort(72, OMX_FALSE);

  if (Subtraction(image, temp, dest) == 1) {
    return 1;
  } else {
    return 0;
  }
}

int EventManager::Subtraction(SEImage img1, SEImage img2, SEImage dst) {
  //차영상을 식별하는 함수
  //YUV420 기반으로 이전프레임과 현재프레임의 각 픽셀을 비교하여, Y 값이 40이상 차이나는
  //픽셀이 총 300개 이상이면 차영상으로 인식하고 1을 반환.
  int value;
  int cnt = 0;
  int x, y;

  //한 프레임내의 가로, 세로 픽셀을 도는 2중 for문
  for (x = 0; x < img1.height; x++) {  //이미지 차연산
    for (y = 0; y < img1.width; y++) {
      value = abs(
          ((unsigned char) img1.pData[x * img1.width + y])
              - ((unsigned char) img2.pData[x * img2.width + y]));
      //현 프레임과 이전 프레임 간의 Y 40 이상의 차이가 나는 픽셀의 갯수를 카운트(cnt++).
      if (value > threshold1) {  // Y 비교.
        dst.pData[x * dst.width + y + 0] = (unsigned char) img1.pData[x
            * img1.width + y + 0];
        cnt++;	//차이나는 픽셀의 갯수 카운트.
      }
      //그 외에 40이상 차이나지 않는 픽셀의 경우엔 Y 값을 16으로 채워서 검은색으로 만들어
      //차영상의 이미지를 윈도우 화면으로 보여준다.
      else {
        dst.pData[x * dst.width + y + 0] = 16;
      }
    }
  }
  if (cnt > threshold2)  //한 프레임을 2중for문으로 모두 비교한 뒤, 식별된 픽셀의 수가 300개 이상일 경우(임의의 기준)
    return 1;  //차영상으로 식별하고 1을 반환.
  return 0;  //차영상으로 식별되지 않을 경우 0을 반환.
}

void EventManager::Run() {
  cout << "EventManager Start" << endl;
  camera = CameraManager::get_instance();
  client = ClientSocket::get_instance();
  omx = OpenMAX::get_instance();

  while (1) {
    pthread_mutex_lock(&mutex_EventManager);
    if (BackgroundSubtraction() == 1 && flag_stream == false) {
      omx->ResetConfigure(1280, 720, 10, 71);
      cout << "EventManager::Evnet Detect!" << endl;
      camera->EventMake();
      MakeFileName();
      client->send_Event(camera->getEvent_date());
      omx->VideoEncode(fileName, 15);
      client->send_file(fileName);

      omx->SwitchCapturePort(71, OMX_FALSE);
      omx->SwitchBufferFlush();
      omx->DisablePort();
      omx->FreeBuffer();
      omx->SwitchIdle();
      omx->SwitchStateLoaded();

      cout << "reset component" << endl;

      omx->InitComponent();
      omx->Load_camera_driver(0);
      omx->ConfigOpenMAX(352, 282, 10, 10000000, 0, 0, 50, 0, 0, 100);
      omx->InitOpenMAXPort();
      omx->SwitchIdle();
      omx->EnablePort();
      omx->AllocateBuffer();
      omx->SwitchExecuting();
      omx->SwitchCapturePort(72, OMX_TRUE);
    }
    pthread_mutex_unlock(&mutex_EventManager);
    usleep(1000);
  }
}
