/*
 * CommunicationService.cpp
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 */

#include "../Include/CommunicationService.h"
#include "../Include/ExternalProtocol.h"

extern bool flag_stream;
CommunicationService* CommunicationService::m_CommunicationService_instance = NULL;

CommunicationService* CommunicationService::createnew(void) {  // 싱글톤 패턴
  if (m_CommunicationService_instance == NULL) {
    m_CommunicationService_instance = new CommunicationService();
  }
  return m_CommunicationService_instance;
}

CommunicationService::CommunicationService() {
  m_CommunicationService_instance = this;
  omx = OpenMAX::get_instance();
  camera = CameraManager::get_instance();
  client = ClientSocket::get_instance();
  client->login_camera_id();
  GetMACAddress();
  client->send_MAC(MAC);
  cout << "before snapshot" << endl;
  camera->CaptureImage("snapshot.jpg", 100);
  cout << "after snapshot" << endl;
  client->send_current_pic("snapshot.jpg");
}

CommunicationService::~CommunicationService() {
}

void CommunicationService::ParsingMessage() {
  packet_total* recv_pkg;
  int head;
  Message msg;
  recv_pkg = (packet_total*) client->recv_message();

  memcpy(&head, &recv_pkg->head, sizeof(head));

  if (head == RECV_MOTOR) {
    msg.setData((void*) recv_pkg, sizeof(packet_cntrl_motor));
    SendMessageToMotorManager(msg, 2);
  } else if (head == RECV_STREAM && flag_stream == false) {
    msg.setData((void*) recv_pkg, sizeof(packet_stream));
    SendMessageToStreamManager(msg, 2);
  } else if (head == RECV_STREAM_STOP && flag_stream == true) {
    cout << "CommunicationService::receive STREAM STOP" << endl;
    omx->SignalEnd();
  }
}

void CommunicationService::GetMACAddress() {
  struct ifreq ifr;
  struct ifconf ifc;
  char buf[1024];
  int success = 0;

  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock == -1) { /* handle error*/
  };

  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;
  if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) { /* handle error */
  }

  struct ifreq* it = ifc.ifc_req;
  const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

  for (; it != end; ++it) {
    strcpy(ifr.ifr_name, it->ifr_name);
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
      if (!(ifr.ifr_flags & IFF_LOOPBACK)) {  // don't count loopback
        if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
          success = 1;
          break;
        }
      }
    } else { /* handle error */
    }
  }

  unsigned char mac_address[6];
  if (success)
    memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
  char buff[2];
  for (int i = 0; i < 6; i++) {
    buff[0] = (ifr.ifr_hwaddr.sa_data[i] & 0xf0) >> 4;
    buff[1] = ifr.ifr_hwaddr.sa_data[i] & 0x0f;
    for (int j = 0; j < 2; j++) {
      if (buff[j] < 10)
        MAC[i * 3 + j] = buff[j] + '0';
      else
        MAC[i * 3 + j] = buff[j] + '7';
    }
    MAC[i * 3 + 2] = '-';
  }
  MAC[17] = '\0';
}

void CommunicationService::SendMessageToStreamManager(Message message,
                                                      int priority) {
  message.setDest(STREAMMANAGER);
  message.setProv(COMMUNICATIONSERVICE);
  message.setPriority(priority);
  SendMessage(message);
  cout << "CommunicationService::SendMessageToStreamManager" << endl;
}

void CommunicationService::SendMessageToMotorManager(Message message,
                                                     int priority) {
  message.setDest(MOTORMANAGER);
  message.setProv(COMMUNICATIONSERVICE);
  message.setPriority(priority);
  SendMessage(message);
//	cout << "CommunicationService::sendMessageToMotorManager" << endl;
}

void CommunicationService::Run() {

  cout << "CommunicationService Start" << endl;
  while (1) {
    ParsingMessage();
    receiveMessage = ReceiveMessage();
    if (receiveMessage.getPriority() != 0) {
      cout << "CommunicationService::ReceiveMessage" << endl;
    }
    usleep(1000);
  }
  cout << "exit communicationservice" << endl;
}

