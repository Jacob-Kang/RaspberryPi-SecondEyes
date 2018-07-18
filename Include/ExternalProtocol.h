/*
 * ExternalProtocol.h
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 */

#ifndef EXTERNALPROTOCOL_H_
#define EXTERNALPROTOCOL_H_

#define PACK_MAX_SIZE 			8000
#define MAX_NAME_LEN 			15

// node -> server
#define SEND_FILENAME 			0x1000
#define SEND_FILEDATA 			0x1001
#define SEND_STREAM_START	 	0x1002
#define SEND_STREAM_DATA 		0x1003
#define SEND_EVNET_DETECT 		0x1004
#define SEND_CURRENT_PIC_START 	0x1005
#define SEND_CURRENT_PIC_DATA 	0x1006
#define SEND_MACADDRESS		 	0x1007

// server -> node
#define RECV_CAMID 				0x2000
#define RECV_MOTOR 				0x2001
#define RECV_STREAM 				0x2002
#define RECV_STREAM_STOP 		0x2003

#define ACK 0x80
#define NACK 0x70

typedef struct __login {
  int head;
  unsigned char sub;
  int cam_id;  // 서버에게 할당 받는 id
} packet_login;

typedef struct _filename {
  int head;
  unsigned char sub;
  int client_id;
  int name_len;						// datacomponent 에 담긴 데이터 크기
  int data_size;
  char file_name[MAX_NAME_LEN];
} packet_filename;

typedef struct _filedata {
  int head;
  unsigned char sub;
  int cam_id;						// datacomponent 에 담긴 데이터 크기
  int in_data_size;
  char payload[PACK_MAX_SIZE];
} packet_filedata;

typedef struct __cntrl_motor {
  int head;
  unsigned char sub;
  int cam_id;
  int up;						// datacomponent 에 담긴 데이터 크기
  int down;
  int left;
  int right;

} packet_cntrl_motor;

typedef struct __return_pkt {
  int head;
  unsigned char sub;
} packet_return;

typedef struct __recv_pkt {
  int head;
  unsigned char sub;
  unsigned char payload[PACK_MAX_SIZE];
} packet_total;

typedef struct __stream {
  int head;
  int cam_id;
} packet_stream;

typedef struct __stream_addr {
  int head;
  int client_id;
  int urlsize;
  char url_addr[35];
} packet_stream_addr;

typedef struct __stream_start {
  int head;
  char ack;
  int client_id;
  int filetotalsize;
} packet_stream_start;

typedef struct __stream_data {
  int head;
  char ack;
  int client_id;
  int in_data_size;
  char payload[PACK_MAX_SIZE];
} packet_stream_data;

typedef struct __current_pic_start {
  int head;
  int client_id;
  int file_total_size;
} packet_current_pic_start;

typedef struct __eventDate {
  int head;
  int cam_id;  // 서버에게 할당 받는 id
  char date[12];  // event 날짜
} packet_eventDate;

typedef struct __current_pic_data {
  int head;
  int client_id;
  int in_data_size;
  char payload[PACK_MAX_SIZE];
} packet_current_pic_data;

typedef struct __mac {
  int head;
  int cam_id;  // 서버에게 할당 받는 id
  char date[18];  // event 날짜
} packet_mac;

#endif /* EXTERNALPROTOCOL_H_ */
