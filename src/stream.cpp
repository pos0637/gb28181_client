//
//  stream.c
//  H264Analysis
//
//  Created by Jinmmer on 2018/5/16.
//  Copyright © 2018年 Jinmmer. All rights reserved.
//

#include "stream.h"

unsigned char *file_buff;

// 读取h264文件，读取失败返回-1，否则返回文件大小
int readAnnexbBitStreamFile(const char *fp) {
    FILE *fp_h264 = fopen(fp, "rb");
    if (fp_h264 == NULL) {
        printf("打开h264文件失败\n");
        return -1;
    }

    fseek(fp_h264, 0, SEEK_END);
    int size = ftell(fp_h264);
    fseek(fp_h264, 0, SEEK_SET);

    file_buff = (unsigned char *)malloc(size);
    int file_size = (int)fread(file_buff, sizeof(unsigned char), size, fp_h264);
    fclose(fp_h264);

    return file_size;
}

void freeFilebuffer(void) { free(file_buff); }
