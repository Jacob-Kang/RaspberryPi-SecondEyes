/*
 * JpegManager.h
 *
 *  Created on: 2015. 2. 27.
 *      Author:  Chulho Kang
 */

#ifndef CAMERA_JPEGMANAGER_H_
#define CAMERA_JPEGMANAGER_H_

#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>

class JpegManager {
 public:
  JpegManager();

  void write_JPEG_file(char * filename, int quality, unsigned char* raw_data,
                       int image_width, int image_height);
  virtual ~JpegManager();

 private:
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  FILE * outfile;
  JSAMPROW row_pointer[1];
  int row_stride;
  typedef struct my_error_mgr * my_error_ptr;
};

#endif /* CAMERA_JPEGMANAGER_H_ */
