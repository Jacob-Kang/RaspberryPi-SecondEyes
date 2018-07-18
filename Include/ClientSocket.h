/*
 * ClientSocket.h
 *
 *  Created on: 2014. 11. 8.
 *      Author: kbright0912
 */

#ifndef CLIENTSOCKET_H_
#define CLIENTSOCKET_H_

#include "SEcommon.h"
#include "ExternalProtocol.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>

class ClientSocket {
 public:
  ClientSocket();
  virtual ~ClientSocket();
  void error_handling(char *message);
  int file_size(char *f);
  void send_file(char* file);
  void login_camera_id();
  void* recv_message();
  void send_streamAddress(char* addr, int size);
  void send_message(void* pkg_send);
  void send_stream(char* file);
  static ClientSocket* get_instance();
  void send_current_pic(char *file);
  void send_Event(char* event_time);
  void send_MAC(char* _mac);

 private:
  static ClientSocket* m_Client_instance;
  int clnt_sock;
  socklen_t serv_addr_size;
  int str_len;			// 송신할 패킷 크기
  int sended;		// 보내진 파일 크기
  int received;
  int entire_size;
  int fd;				// 보낼 file descriptor
  FILE* readfp;
  //socket address structure
  struct sockaddr_in serv_addr;
  struct sockaddr_in clnt_addr;

  packet_login pkg_login;
  packet_filename pkg_filename;
  packet_filedata pkg_filedata;
  packet_cntrl_motor* pkg_cntrl_motor;
  packet_stream* pkg_stream;

  packet_stream_addr send_stream_addr;
  packet_stream_data send_stream_data;
  packet_stream_start send_stream_start;

  packet_eventDate pkg_eventDate;
  packet_current_pic_start pkg_current_start;
  packet_current_pic_data pkg_current_data;
  packet_return pkg_return;
  packet_total pkg_recv;
  packet_total* pkg_send;
  packet_mac pkg_mac;

  FILE * writefp;					// open 을 위한 file pointer

};

#endif /* CLIENTSOCKET_H_ */
