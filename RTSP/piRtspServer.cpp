#include "../RTSP/piRtspServer.hh"
#include "../Include/ClientSocket.h"
#include <iostream>
int _port = 8554;
//
RTSPServer* rtspServer;

//ServerMediaSession* sms;
//TaskScheduler* scheduler;
//UsageEnvironment* env;
//
//
//PI_MEMORY_BUFFER* input_buffer;
//piMemoryBufferedSource* bufferedSource;
//
//RTPSink* videoSink;
//RTCPInstance* rtcp;

void* startRtspServer(void* arg) {
  ClientSocket* socketClient = ClientSocket::get_instance();

  // Begin by setting up our usage environment
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

  // Create groupsocks for RTP and RTCP
  // This is a multicast address
  struct in_addr destinationAddress;
  destinationAddress.s_addr = chooseRandomIPv4SSMAddress(*env);

  const unsigned short rtpPortNum = 18888;
  const unsigned short rtcpPortNum = rtpPortNum + 1;
  const unsigned char ttl = 255;

  const Port rtpPort(rtpPortNum);
  const Port rtcpPort(rtcpPortNum);

  Groupsock rtpGroupsock(*env, destinationAddress, rtpPort, ttl);
  rtpGroupsock.multicastSendOnly();
  Groupsock rtcpGroupsock(*env, destinationAddress, rtcpPort, ttl);
  rtcpGroupsock.multicastSendOnly();

  // Create an input buffer source
  // arg must be the input buffer structure
  PI_MEMORY_BUFFER* input_buffer = (PI_MEMORY_BUFFER*) arg;
  piMemoryBufferedSource* bufferedSource = piMemoryBufferedSource::createNew(
      *env, input_buffer, True);
  if (bufferedSource == NULL) {
    *env << "Unable to open input buffer\n";
    exit(1);
  }

  // Create an H264 Video RTP sink from the RTP groupsock
  OutPacketBuffer::maxSize = RTSP_INPUT_BUFFER_MAX;
  RTPSink* videoSink = H264VideoRTPSink::createNew(*env, &rtpGroupsock, 96);

  // Create and start an RTCP instance for this RTP sink
  const unsigned estimatedSessionBandwidth = 2000;  // in kbps
  const unsigned maxCNAMElen = 100;
  unsigned char CNAME[maxCNAMElen + 1];
  gethostname((char*) CNAME, maxCNAMElen);
  CNAME[maxCNAMElen] = 0;
  // This starts RTCP running automatically
  RTCPInstance* rtcp = RTCPInstance::createNew(*env, &rtcpGroupsock,
                                               estimatedSessionBandwidth, CNAME,
                                               videoSink, NULL, True);

  RTSPServer* rtspServer = RTSPServer::createNew(*env, _port);
  while (rtspServer == NULL) {
    *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    _port++;
    rtspServer = RTSPServer::createNew(*env, _port);
//		exit (1);
  }

  ServerMediaSession* sms = ServerMediaSession::createNew(*env, "SecondEyes",
                                                          "video_encode",
                                                          "\"streamed by pi\"",
                                                          True);
  sms->addSubsession(PassiveServerMediaSubsession::createNew(*videoSink, rtcp));

  rtspServer->addServerMediaSession(sms);

  char* url = rtspServer->rtspURL(sms);
  *env << "Stream \"" << url << "\"" << "\n";

  socketClient->send_streamAddress(url, strlen(url));
  delete[] url;

  // Create a framer for the Video Elementary Stream
  H264VideoStreamDiscreteFramer* videoSource =
      H264VideoStreamDiscreteFramer::createNew(*env, bufferedSource);

  *env << "Beginning to read from input buffer ...\n";
  videoSink->startPlaying(*videoSource, NULL, videoSink);

  // Never returns
  env->taskScheduler().doEventLoop();

  return 0;
}

void * endRtspServer() {
//	videoSink->stopPlaying();
  Medium::close(rtspServer);
  std::cout << "rtspServer" << std::endl;
//	Medium::close(rtcp);
//	std::cout<<"rtcp"<<std::endl;
//	Medium::close(videoSink);
//	std::cout<<"videoSink"<<std::endl;
//
//	Medium::close(bufferedSource);
//	std::cout<<"bufferedSource"<<std::endl;
//	delete scheduler;
//	std::cout<<"scheduler"<<std::endl;
//	env->reclaim();
//	std::cout<<"env"<<std::endl;
//
//
////
//	Medium::close(sms);
//	std::cout<<"sms"<<std::endl;

//	delete input_buffer;
//	delete bufferedSource;
//	delete scheduler;

}
