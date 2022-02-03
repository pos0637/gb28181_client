#ifndef H264_PARSER_INCLUDE_H
#define H264_PARSER_INCLUDE_H

#include <stdio.h>
#include "cnalu.h"
int simplest_h264_parser(const char *url,void(*out_nalu)(unsigned char * buffer,int size, NaluType type));

#endif