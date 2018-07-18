/*
 * ClientSocket.cpp
 *
 *  Created on: 2014. 11. 8.
 *      Author: kbright0912
 */

#include "../Include/ClientSocket.h"
#include <stdlib.h>

int camera_id = 0;
ClientSocket* ClientSocket::m_Client_instance = NULL;

ClientSocket::ClientSocket() {
  pkg_cntrl_motor = new packet_cntrl_motor;
  pkg_stream = new packet_stream;
  sended = 0;
  clnt_sock = socket(PF_INET, SOCK_STREAM, 0);			// client 소켓 생성
  if (clnt_sock == -1) {
    fputs("error\n", stderr);
    exit(1);
  }
  memset(&clnt_addr, 0, sizeof(serv_addr));		// Server 주소 정보 담을 구조체 초기화
  serv_addr.sin_family = AF_INET;							// TCP
#if IN_RASP
  serv_addr.sin_addr.s_addr = inet_addr("203.252.118.75");			// 입력한 서버 IP로 접속
#else
      serv_addr.sin_addr.s_addr = inet_addr("203.252.118.75");
#endif
  serv_addr.sin_port = htons(atoi("1201"));						// 입력한 서버 포트
  if (connect(clnt_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))
      == -1) {
    fputs("error\n", stderr);
    exit(1);
  } else
    cout << "connect to server" << endl;
  // TODO Auto-generated constructor stub

}

ClientSocket::~ClientSocket() {
  // TODO Auto-generated destructor stub
}

ClientSocket* ClientSocket::get_instance() {
  if (m_Client_instance == NULL) {
    cout << "ClientSocket::Not create yet" << endl;
    m_Client_instance = new ClientSocket;
  }
  return m_Client_instance;
}

void ClientSocket::error_handling(char *message) {
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}
void ClientSocket::login_camera_id() {
  received = recv(clnt_sock, (char*) &pkg_login, sizeof(pkg_login), 0);  // 임시 버퍼에 패킷 수신
  if (received == -1)  // 수신 에러 처리
    error_handling("file_name.recv() error!");
  if (pkg_login.head == RECV_CAMID)
    camera_id = pkg_login.cam_id;
  cout << "received ID = " << camera_id << endl;
}

void ClientSocket::send_MAC(char* _mac) {
  pkg_mac.head = SEND_MACADDRESS;
  pkg_mac.cam_id = camera_id;
  memcpy(pkg_mac.date, _mac, sizeof(pkg_mac.date));
  str_len = send(clnt_sock, (char*) &pkg_mac, sizeof(pkg_mac), 0);	// 패킷  송신
  if (str_len == -1)
    error_handling("pkg_current_data.send() error!");
  cout << "send MAC" << endl;
}

void ClientSocket::send_message(void* temp) {
  pkg_send = (packet_total*) temp;
  packet_return temp_pkg;
  memcpy(&temp_pkg, pkg_send, sizeof(temp_pkg));
  if (temp_pkg.head == RECV_STREAM) {
    packet_stream send_stream;
    memcpy(&send_stream, temp, sizeof(packet_stream));
    str_len = send(clnt_sock, (char*) &send_stream, sizeof(send_stream), 0);  // 패킷  송신
    if (str_len == -1)
      error_handling("stream send error!");
  }
}

void ClientSocket::send_streamAddress(char* addr, int size) {
  send_stream_addr.head = SEND_STREAM_START;
  send_stream_addr.client_id = camera_id;
  send_stream_addr.urlsize = size;
  strcpy(send_stream_addr.url_addr, addr);
  cout << "send URL : (" << send_stream_addr.urlsize << ")"
       << send_stream_addr.url_addr << endl;
  cout << "size = " << sizeof(send_stream_addr);
  str_len = send(clnt_sock, (char*) &send_stream_addr, sizeof(send_stream_addr),
                 0);  // 패킷  송신
  if (str_len == -1)
    error_handling("stream send error!");
}

void ClientSocket::send_stream(char* file) {
  send_stream_start.head = SEND_STREAM_START;
  send_stream_start.client_id = camera_id;
  send_stream_data.head = SEND_STREAM_DATA;
  send_stream_data.client_id = camera_id;
  fd = open(file, O_RDONLY);				// 보낼 파일 open
  if (fd == -1) {
    cout << file << endl;
    fputs(" : file open error!", stdout);
    return;
  }

  send_stream_start.filetotalsize = file_size(file);
  entire_size = send_stream_start.filetotalsize;
  str_len = send(clnt_sock, (char*) &send_stream_start,
                 sizeof(send_stream_start), 0);  // 패킷  송신
  if (str_len == -1)
    error_handling("file_name.send() error!");

  while (sended < send_stream_start.filetotalsize) {
    if ((entire_size - sended) < PACK_MAX_SIZE)
      send_stream_data.in_data_size = entire_size - sended;
    else
      send_stream_data.in_data_size = PACK_MAX_SIZE;

    sended += read(fd, send_stream_data.payload, send_stream_data.in_data_size);
    str_len = send(clnt_sock, (char*) &send_stream_data,
                   sizeof(send_stream_data), 0);	// 패킷  송신
    if (str_len == -1)
      error_handling("file_data.send() error!");
//		cout << "send data : " << sended << " byte / " << send_stream_start.filetotalsize << " byte" << endl;
    memset(&send_stream_data.payload, 0, sizeof(send_stream_data.payload));
  }
  memset(&pkg_return, 0, sizeof(pkg_return));
//	while (pkg_return.sub != ACK) {
//		cout << "wait quit" << endl;
//		received = recv(clnt_sock, (char*) &pkg_return, sizeof(pkg_return), 0); // 임시 버퍼에 패킷 수신
//		cout << "ack = " << pkg_return.sub << endl;
//		if (received == -1) // 수신 에러 처리
//			error_handling("file_name.recv() error!");
//		if (pkg_return.sub == ACK)
//			cout << "success" << endl;
//	}
  cout << "send_Complete" << endl;
  sended = 0;
  close(fd);
}

void* ClientSocket::recv_message() {
  received = recv(clnt_sock, (char*) &pkg_recv, sizeof(pkg_recv), 0);  // 임시 버퍼에 패킷 수신
  if (received == -1)  // 수신 에러 처리
    error_handling("recv_message() error!");
  return &pkg_recv;
}

int ClientSocket::file_size(char* f) {
  long size;
  int flag;
  struct stat buf;
  flag = stat(f, &buf);
  if (flag == -1)
    return -1;
  size = buf.st_size;
  return (size);
}

void ClientSocket::send_current_pic(char *file) {
  cout << "start send" << endl;
  fd = open(file, O_RDONLY);				// 보낼 파일 open
  if (fd == -1) {
    cout << file << endl;
    fputs(" : file open error!", stdout);
    return;
  }

  pkg_current_start.head = SEND_CURRENT_PIC_START;
  pkg_current_start.file_total_size = file_size(file);
  pkg_current_start.client_id = camera_id;
  entire_size = pkg_current_start.file_total_size;

  str_len = send(clnt_sock, (char*) &pkg_current_start,
                 sizeof(pkg_current_start), 0);				// 패킷  송신
  if (str_len == -1)
    error_handling("file_name.send() error!");

  cout << "pkg_filename.data_size : " << pkg_current_start.file_total_size;

  // 파일 전송
  pkg_current_data.client_id = camera_id;
  pkg_current_data.head = SEND_CURRENT_PIC_DATA;

  // 패킷 초기화?
  while (sended < pkg_current_start.file_total_size) {
    if ((entire_size - sended) < PACK_MAX_SIZE)
      pkg_current_data.in_data_size = entire_size - sended;
    else
      pkg_current_data.in_data_size = PACK_MAX_SIZE;

    sended += read(fd, pkg_current_data.payload, pkg_current_data.in_data_size);
    str_len = send(clnt_sock, (char*) &pkg_current_data,
                   sizeof(pkg_current_data), 0);  // 패킷  송신
    if (str_len == -1)
      error_handling("pkg_current_data.send() error!");
    cout << "send data : " << sended << " byte / "
         << pkg_current_start.file_total_size << " byte" << endl;
    memset(&pkg_current_data.payload, 0, sizeof(pkg_current_data.payload));
  }
  sended = 0;
}

void ClientSocket::send_Event(char* event_time) {
  pkg_eventDate.head = SEND_EVNET_DETECT;
  pkg_eventDate.cam_id = camera_id;
  strcpy(pkg_eventDate.date, event_time);
  str_len = send(clnt_sock, (char*) &pkg_eventDate, sizeof(pkg_eventDate), 0);	// 패킷  송신
  if (str_len == -1)
    error_handling("pkg_current_data.send() error!");
  cout << "send Event" << endl;
}

//
void ClientSocket::send_file(char* file) {
  cout << "start send" << endl;
  fd = open(file, O_RDONLY);				// 보낼 파일 open
  if (fd == -1) {
    cout << file << endl;
    fputs(" : file open error!", stdout);
    return;
  }
  // camera id

  pkg_filename.head = SEND_FILENAME;
  pkg_filename.data_size = file_size(file);
  pkg_filename.client_id = camera_id;
  entire_size = pkg_filename.data_size;
  strcpy(pkg_filename.file_name, file);
  pkg_filename.name_len = strlen(pkg_filename.file_name);

//	while (1) {
  str_len = send(clnt_sock, (char*) &pkg_filename, sizeof(pkg_filename), 0);  // 패킷  송신
  if (str_len == -1)
    error_handling("file_name.send() error!");

//		received = recv(clnt_sock, (char*) &pkg_return, sizeof(pkg_return), 0); // 임시 버퍼에 패킷 수신
//		if (received == -1) // 수신 에러 처리
//			error_handling("file_name.recv() error!");
//		if (pkg_return.sub == 0x80) {
//			cout << "recv:: filename_return" << endl;
//			break;
//		}
//	}

  cout << "pkg_filename.data_size : " << pkg_filename.data_size;
  cout << endl << "pkg_filename.name_len : " << pkg_filename.name_len << endl;
  // 파일 전송
  pkg_filedata.cam_id = camera_id;
  pkg_filedata.head = SEND_FILEDATA;
  // 패킷 초기화?

  while (sended < pkg_filename.data_size) {
    if ((entire_size - sended) < PACK_MAX_SIZE)
      pkg_filedata.in_data_size = entire_size - sended;
    else
      pkg_filedata.in_data_size = PACK_MAX_SIZE;

    sended += read(fd, pkg_filedata.payload, pkg_filedata.in_data_size);
    str_len = send(clnt_sock, (char*) &pkg_filedata, sizeof(pkg_filedata), 0);  // 패킷  송신
    if (str_len == -1)
      error_handling("file_data.send() error!");
//		cout << "send data : " << sended << " byte / " << pkg_filename.data_size<< " byte" << endl;
    memset(&pkg_filedata.payload, 0, sizeof(pkg_filedata.payload));
  }
  cout << file << " : send Complete" << endl;
  memset(&pkg_return, 0, sizeof(pkg_return));
  sended = 0;
//	while (pkg_return.sub != ACK) {
//		cout << "wait quit" << endl;
//		received = recv(clnt_sock, (char*) &pkg_return, sizeof(pkg_return), 0); // 임시 버퍼에 패킷 수신
//		if (received == -1) // 수신 에러 처리
//			error_handling("file_name.recv() error!");\
//		if (pkg_return.sub == ACK)
//			cout << "success" << endl;
//	}

//	close(clnt_sock);

}
