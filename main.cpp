/*
 * main.cpp
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 */

#include "Include/MainTaskManager.h"
#include "Include/SEcommon.h"

char* sock_addr;
char* sock_port;

int main(int argc, char* argv[]) {
  MainTaskManager* mtm = new MainTaskManager();
  mtm->Start();
  mtm->Wait();
  return 0;
}
