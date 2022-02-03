//
//  stream.h
//  H264Analysis
//
//  Created by Jinmmer on 2018/5/16.
//  Copyright © 2018年 Jinmmer. All rights reserved.
//

#ifndef stream_h
#define stream_h

#include <stdio.h>
#include <stdlib.h>

extern unsigned char *file_buff;

// 读取h264文件，读取失败返回-1，否则返回文件大小
int readAnnexbBitStreamFile(const char *fp);
void freeFilebuffer(void);

#endif /* stream_h */
