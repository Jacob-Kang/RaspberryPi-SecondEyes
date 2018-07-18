/*
 * StreamManager.cpp
 *
 *  Created on: 2015. 1. 23.
 *      Author:  Chulho Kang
 */

#include "../Include/StreamManager.h"
#include "../Include/ClientSocket.h"
#include "../Include/MotorManager.h"

extern bool flag_stream;

StreamManager::StreamManager()
    : flag_motor(0),
      interCMD(0) {
  motorManager = NULL;
  camera = NULL;
  omx = NULL;
  client = NULL;
  // TODO Auto-generated constructor stub
}

StreamManager::~StreamManager() {
  // TODO Auto-generated destructor stub
}

void StreamManager::Run() {
  cout << "StreamManager Start" << endl;
  omx = OpenMAX::get_instance();
  while (1) {
    int rc = pthread_mutex_lock(&mutex_StreamManager);
    if (rc)
      continue;
    else
      cout << "StreamManagerLock" << endl;
    pthread_cond_wait(&cond_StreamManager, &mutex_StreamManager);
    cout << "StreamManagerSignal" << endl;
    pthread_mutex_lock(&mutex_EventManager);

    while ((receiveMessage = ReceiveMessage()).getPriority() != 0) {  //존재하는 메시지 모두 처리
      cout << "Received msg" << endl;
      memcpy(&recv_stream, (packet_stream*) receiveMessage.getData(),
             sizeof(recv_stream));
      cout << "ID = " << recv_stream.cam_id << endl;
#if POWERMEASURE
      Size frameSize = Size((int) vc_capture.get(CV_CAP_PROP_FRAME_WIDTH),(int) vc_capture.get(CV_CAP_PROP_FRAME_HEIGHT));
      VideoWriter *video = new VideoWriter;
      int count = 0;
      // 촬영상태
      if (recv_stream.cam_id == 0x1000) {
        cout << "촬영 모드 시작" << endl;
        while (count < 100) {
          vc_capture >> frame;
          count++;
        }
        count = 0;
        cout << "촬영 모드 종료" << endl;
        // 촬영+압축
      } else if (recv_stream.cam_id == 0x2000) {
        cout << "촬영+압축 모드 시작" << endl;
        video->open("record_test.avi", CV_FOURCC('D', 'I', 'V', 'X'), MAX_FPS, frameSize,true);
        if (!video->isOpened()) {
          std::cout << "!!! Output video could not be opened"<< std::endl;
          break;
        }
        std::cout << "start recording" << endl;
        while (count <= MAX_FPS * 15) {
          vc_capture >> frame;
          *video << frame;
          count++;
        }
        cout << "촬영+압축 모드 종료" << endl;
        // 촬영+압축+무선전송
      } else if(recv_stream.cam_id == 0x3000) {
        cout << "촬영+압축+전송 모드 시작" << endl;
        video->open("record_test.avi", CV_FOURCC('D', 'I', 'V', 'X'), MAX_FPS, frameSize,true);
        if (!video->isOpened()) {
          std::cout << "!!! Output video could not be opened"<< std::endl;
          break;
        }
        std::cout << "start recording" << endl;
        while (count <= MAX_FPS * 15) {
          vc_capture >> frame;
          *video << frame;
          count++;
        }
        client->send_stream("send_test.avi");
        cout << "촬영+압축+전송 모드 종료" << endl;
      }
#else if
      if (camera_id == recv_stream.cam_id) {
        omx->ResetConfigure(352, 282, 10, 71);
        cout << "StreamManager::stream ready" << endl;
        flag_stream = true;
        sendMessage.setDest(MOTORMANAGER);
        sendMessage.setPriority(1);
        cout << "flag_motor = " << flag_motor << endl;
        if (flag_motor == 0)
          interCMD = S2M_FIRST_START;
        else if (flag_motor == 2)
          interCMD = S2M_RESTART;
        flag_motor = 1;
        sendMessage.setCMD(interCMD);
        SendMessage(sendMessage);
        omx->StartStreaming();
        omx->SwitchCapturePort(72, OMX_FALSE);
        sendMessage.setDest(MOTORMANAGER);
        sendMessage.setPriority(1);
        interCMD = S2M_STOP;
        sendMessage.setCMD(interCMD);
        SendMessage(sendMessage);
        flag_motor = 2;
        flag_stream = false;
        omx->ResetConfigure(320, 240, 10, 72);
      }
#endif
    }
    pthread_mutex_unlock(&mutex_EventManager);
    pthread_mutex_unlock(&mutex_StreamManager);
    usleep(1000);
  }
}

