#include "h264_parser.h"

#include <stdlib.h>
#include <string.h>

#include "spdlog/spdlog.h"

typedef enum { NALU_PRIPORITY_DISPOSABLE = 0, NALU_PRIORITY_LOW = 1, NALU_PRIORITY_HIGH = 2, NALU_PRIORITY_HIGHTEST = 3, a } NaluPriority;

typedef struct {
    int startcodeprefix_len;  //! 4 for parameter sets and first slice in picture, 3 for everything else (suggest)
    unsigned len;             //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU
    int max_size;             //! Nalu Unit Buffer size
    int forbidden_bit;        //! should be always FALSE
    int nal_reference_idc;    //! NALU_PRIPORITY_xxxx
    int nal_unit_type;        //! NALU_TYPE_xxxx
    char *buf;                //! contains the first byte followed by the EBSP
    unsigned char frametype;  //! 帧类型
} NALU_t;

typedef struct Tag_bs_t {
    unsigned char *p_start;  //缓冲区首地址(这个开始是最低地址)
    unsigned char *p;        //缓冲区当前的读写指针 当前字节的地址，这个会不断的++，每次++，进入一个新的字节
    unsigned char *p_end;    //缓冲区尾地址		//typedef unsigned char   uint8_t;
    int i_left;              // p所指字节当前还有多少 “位” 可读写 count number of available(可用的)位
} bs_t;

enum Frametype_e { FRAME_I = 15, FRAME_P = 16, FRAME_B = 17 };

FILE *h264bitstream = NULL;  //! the bit stream file

int info2 = 0, info3 = 0;

static int FindStartCode2(unsigned char *Buf) {
    if (Buf[0] != 0 || Buf[1] != 0 || Buf[2] != 1)
        return 0;  // 0x00 0001 ？
    else
        return 1;
}

static int FindStartCode3(unsigned char *Buf) {
    if (Buf[0] != 0 || Buf[1] != 0 || Buf[2] != 0 || Buf[3] != 1)
        return 0;  // 0x00 000001?
    else
        return 1;
}

static int GetAnnexbNALU(NALU_t *nalu) {
    int pos = 0;
    int startCodeFound, rewind;
    unsigned char *Buf;

    if ((Buf = (unsigned char *)calloc(nalu->max_size, sizeof(char))) == NULL) printf("GetAnnexbNALU: Could not allocate Buf memory\n");

    nalu->startcodeprefix_len = 3;

    if (3 != fread(Buf, 1, 3, h264bitstream)) {
        free(Buf);
        return 0;
    }

    info2 = FindStartCode2(Buf);
    if (info2 != 1) {  //不是0x000001
        if (1 != fread(Buf + 3, 1, 1, h264bitstream)) {
            free(Buf);
            return -1;
        }
        info3 = FindStartCode3(Buf);
        if (info3 != 1) {  //不是0x00 000001?
            free(Buf);
            return -1;
        } else {
            pos = 4;
            nalu->startcodeprefix_len = 4;
        }
    } else {
        pos = 3;
        nalu->startcodeprefix_len = 3;
    }

    startCodeFound = 0;
    info2 = 0;
    info3 = 0;

    while (!startCodeFound) {
        if (feof(h264bitstream)) {
            nalu->len = (pos - 1) - nalu->startcodeprefix_len;
            memcpy(nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);
            nalu->forbidden_bit = nalu->buf[0] & 0x80;      // 1 bit
            nalu->nal_reference_idc = nalu->buf[0] & 0x60;  // 2 bit
            nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;    // 5 bit
            free(Buf);
            return pos - 1;
        }
        Buf[pos++] = fgetc(h264bitstream);
        info3 = FindStartCode3(&Buf[pos - 4]);
        if (info3 != 1) info2 = FindStartCode2(&Buf[pos - 3]);
        startCodeFound = (info2 == 1 || info3 == 1);
    }

    // Here we have found another start code (and read length of startcode bytes more than we should
    // have, hence ,go back in the file)
    rewind = info3 == 1 ? -4 : -3;
    if (0 != fseek(h264bitstream, rewind, SEEK_CUR)) {
        free(Buf);
        printf("GetAnnexbNALU:Cannot fseek in the bit stream file");
    }

    // Here the Start code, the complete NALU, and the next start code is in the Buf
    // The size of Buf is pos , pos+rewind are the number of bytes excluding the next
    // start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code
    nalu->len = (pos + rewind) - nalu->startcodeprefix_len;
    memcpy(nalu->buf, Buf, nalu->len + nalu->startcodeprefix_len);
    nalu->forbidden_bit = nalu->buf[nalu->startcodeprefix_len] & 0x80;      // 1 bit
    nalu->nal_reference_idc = nalu->buf[nalu->startcodeprefix_len] & 0x60;  // 2 bit
    nalu->nal_unit_type = nalu->buf[nalu->startcodeprefix_len] & 0x1f;      // 5 bit

    // nalu->len = (pos + rewind) - nalu->startcodeprefix_len;
    // memcpy(nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);
    // nalu->forbidden_bit = nalu->buf[0] & 0x80;       // 1 bit
    // nalu->nal_reference_idc = nalu->buf[0] & 0x60;   // 2 bit
    // nalu->nal_unit_type = nalu->buf[0] & 0x1f;       // 5 bit
    free(Buf);

    return (pos + rewind);
}

static void bs_init(bs_t *s, void *p_data, int i_data) {
    s->p_start = (unsigned char *)p_data;  //用传入的p_data首地址初始化p_start，只记下有效数据的首地址
    s->p = (unsigned char *)p_data;        //字节首地址，一开始用p_data初始化，每读完一个整字节，就移动到下一字节首地址
    s->p_end = s->p + i_data;              //尾地址，最后一个字节的首地址?
    s->i_left = 8;                         //还没有开始读写，当前字节剩余未读取的位是8
}

static int bs_read(bs_t *s, int i_count) {
    static unsigned int i_mask[33] = {0x00,     0x01,     0x03,     0x07,      0x0f,      0x1f,      0x3f,      0x7f,       0xff,       0x1ff,      0x3ff,
                                      0x7ff,    0xfff,    0x1fff,   0x3fff,    0x7fff,    0xffff,    0x1ffff,   0x3ffff,    0x7ffff,    0xfffff,    0x1fffff,
                                      0x3fffff, 0x7fffff, 0xffffff, 0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff};
    /*
                      数组中的元素用二进制表示如下：
                      假设：初始为0，已写入为+，已读取为-

                      字节:		1		2		3		4
                               00000000 00000000 00000000 00000000		下标
                      0x00:							  00000000		x[0]
                      0x01:							  00000001		x[1]
                      0x03:							  00000011		x[2]
                      0x07:							  00000111		x[3]
                      0x0f:							  00001111		x[4]
                      0x1f:							  00011111		x[5]
                      0x3f:							  00111111		x[6]
                      0x7f:							  01111111		x[7]
                      0xff:							  11111111		x[8]	1字节
                     0x1ff:						 0001 11111111		x[9]
                     0x3ff:						 0011 11111111		x[10]	i_mask[s->i_left]
                     0x7ff:						 0111 11111111		x[11]
                     0xfff:						 1111 11111111		x[12]	1.5字节
                    0x1fff:					 00011111 11111111		x[13]
                    0x3fff:					 00111111 11111111		x[14]
                    0x7fff:					 01111111 11111111		x[15]
                    0xffff:					 11111111 11111111		x[16]	2字节
               0x1ffff:				0001 11111111 11111111		x[17]
               0x3ffff:				0011 11111111 11111111		x[18]
               0x7ffff:				0111 11111111 11111111		x[19]
               0xfffff:				1111 11111111 11111111		x[20]	2.5字节
              0x1fffff:			00011111 11111111 11111111		x[21]
              0x3fffff:			00111111 11111111 11111111		x[22]
              0x7fffff:			01111111 11111111 11111111		x[23]
              0xffffff:			11111111 11111111 11111111		x[24]	3字节
             0x1ffffff:	   0001 11111111 11111111 11111111		x[25]
             0x3ffffff:	   0011 11111111 11111111 11111111		x[26]
             0x7ffffff:    0111 11111111 11111111 11111111		x[27]
             0xfffffff:    1111 11111111 11111111 11111111		x[28]	3.5字节
            0x1fffffff:00011111 11111111 11111111 11111111		x[29]
            0x3fffffff:00111111 11111111 11111111 11111111		x[30]
            0x7fffffff:01111111 11111111 11111111 11111111		x[31]
            0xffffffff:11111111 11111111 11111111 11111111		x[32]	4字节
     */
    unsigned int i_shr;         //
    unsigned int i_result = 0;  //用来存放读取到的的结果 typedef unsigned   uint32_t;

    while (i_count > 0)  //要读取的比特数
    {
        //字节流的当前位置>=流结尾，即代表此比特流s已经读完了。
        if (s->p >= s->p_end) {
            break;
        }

        if ((i_shr = s->i_left - i_count) >= 0) {
            //当前字节剩余的未读位数，比要读取的位数多，或者相等
            // i_left当前字节剩余的未读位数，本次要读i_count比特，i_shr=i_left-i_count的结果如果>=0，说明要读取的都在当前字节内
            // i_shr>=0，说明要读取的比特都处于当前字节内
            //这个阶段，一次性就读完了，然后返回i_result(退出了函数)
            /* more in the buffer than requested */
            i_result |= (*s->p >> i_shr) & i_mask[i_count];  //“|=”:按位或赋值，A |= B 即 A = A|B
                                                             //|=应该在最后执行，把结果放在i_result(按位与优先级高于复合操作符|=)
                                                             // i_mask[i_count]最右侧各位都是1,与括号中的按位与，可以把括号中的结果复制过来
                                                             //!=,左边的i_result在这儿全是0，右侧与它按位或，还是复制结果过来了，好象好几步都多余
            /*读取后，更新结构体里的字段值*/
            s->i_left -= i_count;  //即i_left = i_left - i_count，当前字节剩余的未读位数，原来的减去这次读取的
            if (s->i_left == 0)    //如果当前字节剩余的未读位数正好是0，说明当前字节读完了，就要开始下一个字节
            {
                s->p++;         //移动指针，所以p好象是以字节为步长移动指针的
                s->i_left = 8;  //新开始的这个字节来说，当前字节剩余的未读位数，就是8比特了
            }
            return (i_result);  //可能的返回值之一为：00000000 00000000 00000000 00000001 (4字节长)
        } else {
            /* i_shr < 0 ,跨字节的情况*/
            //这个阶段，是while的一次循环，可能还会进入下一次循环，第一次和最后一次都可能读取的非整字节，比如第一次读了3比特，中间读取了2字节(即2x8比特)，最后一次读取了1比特，然后退出while循环
            //当前字节剩余的未读位数，比要读取的位数少，比如当前字节有3位未读过，而本次要读7位
            //???对当前字节来说，要读的比特，都在最右边，所以不再移位了(移位的目的是把要读的比特放在当前字节最右)
            /* less(较少的) in the buffer than requested */
            i_result |= (*s->p & i_mask[s->i_left]) << -i_shr;  //"-i_shr"相当于取了绝对值
                                                                //|= 和 << 都是位操作符，优先级相同，所以从左往右顺序执行
                                                                //举例:int|char ，其中int是4字节，char是1字节，sizeof(int|char)是4字节
                                                                // i_left最大是8，最小是0，取值范围是[0,8]
            i_count -= s->i_left;  //待读取的比特数，等于原i_count减去i_left，i_left是当前字节未读过的比特数，而此else阶段，i_left代表的当前字节未读的比特全被读过了，所以减它
            s->p++;                //定位到下一个新的字节
            s->i_left = 8;         //对一个新字节来说，未读过的位数当然是8，即本字节所有位都没读取过
        }
    }

    return (i_result);  //可能的返回值之一为：00000000 00000000 00000000 00000001 (4字节长)
}

static int bs_read1(bs_t *s) {
    if (s->p < s->p_end) {
        unsigned int i_result;

        //当前字节未读取的位数少了1位
        s->i_left--;
        //把要读的比特移到当前字节最右，然后与0x01:00000001进行逻辑与操作，因为要读的只是一个比特，这个比特不是0就是1，与0000 0001按位与就可以得知此情况
        i_result = (*s->p >> s->i_left) & 0x01;
        //如果当前字节剩余未读位数是0，即是说当前字节全读过了
        if (s->i_left == 0) {
            s->p++;         //指针s->p 移到下一字节
            s->i_left = 8;  //新字节中，未读位数当然是8位
        }
        return i_result;  // unsigned int
    }

    return 0;  //返回0应该是没有读到东西
}

static int bs_read_ue(bs_t *s) {
    int i = 0;

    //条件为：读到的当前比特=0，指针未越界，最多只能读32比特
    while (bs_read1(s) == 0 && s->p < s->p_end && i < 32) {
        i++;
    }
    return ((1 << i) - 1 + bs_read(s, i));
}

static int getFrameType(NALU_t *nal) {
    bs_t s;
    int frame_type = 0;
    bs_init(&s, nal->buf + nal->startcodeprefix_len + 1, nal->len - 1);

    if (nal->nal_unit_type == NALU_TYPE_SLICE || nal->nal_unit_type == NALU_TYPE_IDR) {
        /* i_first_mb */
        bs_read_ue(&s);
        /* picture type */
        frame_type = bs_read_ue(&s);
        switch (frame_type) {
            case 0:
            case 5: /* P */
                nal->frametype = FRAME_P;
                break;
            case 1:
            case 6: /* B */
                nal->frametype = FRAME_B;
                break;
            case 3:
            case 8: /* SP */
                nal->frametype = FRAME_P;
                break;
            case 2:
            case 7: /* I */
                nal->frametype = FRAME_I;
                break;
            case 4:
            case 9: /* SI */
                nal->frametype = FRAME_I;
                break;
        }
    }

    return 1;
}

int simplest_h264_parser(const char *url, void (*out_nalu)(char *buffer, int size, NaluType type))
// int simplest_h264_parser(const char *url)
{
    NALU_t *n;
    int buffersize = 1000000;

    // FILE *myout=fopen("output_log.txt","wb+");
    // C语言中的 stdout 是一个定义在<stdio.h>的宏（macro），它展开到一个 FILE* （“指向 FILE
    // 的指针”）类型的表达式（不一定是常量），这个表达式指向一个与标准输出流（standard output stream）相关连的 FILE
    // 对象。
    FILE *myout = stdout;

    h264bitstream = fopen(url, "rb+");
    if (h264bitstream == NULL) {
        spdlog::critical("Open file error");
        exit(1);
    }

    n = (NALU_t *)calloc(1, sizeof(NALU_t));
    if (n == NULL) {
        spdlog::critical("Alloc NALU Error");
        exit(1);
    }

    n->max_size = buffersize;
    n->buf = (char *)calloc(buffersize, sizeof(char));
    if (n->buf == NULL) {
        free(n);
        spdlog::critical("AllocNALU:n->buf");
        exit(1);
    }

    int data_offset = 0;
    int nal_num = 0;
    printf("-----+-------- NALU Table ------+--------+---------\n");
    printf(" NUM |    POS  |    IDC |  TYPE |  FRAME |  LEN   |\n");
    printf("-----+---------+--------+-------+--------+---------\n");

    while (!feof(h264bitstream)) {
        int data_lenth;
        data_lenth = GetAnnexbNALU(n);

        char type_str[20] = {0};
        switch (n->nal_unit_type) {
            case NALU_TYPE_SLICE:
                sprintf(type_str, "SLICE");
                break;
            case NALU_TYPE_DPA:
                sprintf(type_str, "DPA");
                break;
            case NALU_TYPE_DPB:
                sprintf(type_str, "DPB");
                break;
            case NALU_TYPE_DPC:
                sprintf(type_str, "DPC");
                break;
            case NALU_TYPE_IDR:
                sprintf(type_str, "IDR");
                break;
            case NALU_TYPE_SEI:
                sprintf(type_str, "SEI");
                break;
            case NALU_TYPE_SPS:
                sprintf(type_str, "SPS");
                break;
            case NALU_TYPE_PPS:
                sprintf(type_str, "PPS");
                break;
            case NALU_TYPE_AUD:
                sprintf(type_str, "AUD");
                break;
            case NALU_TYPE_EOSEQ:
                sprintf(type_str, "EOSEQ");
                break;
            case NALU_TYPE_EOSTREAM:
                sprintf(type_str, "EOSTREAM");
                break;
            case NALU_TYPE_FILL:
                sprintf(type_str, "FILL");
                break;
        }

        char idc_str[20] = {0};
        switch (n->nal_reference_idc >> 5) {
            case NALU_PRIPORITY_DISPOSABLE:
                sprintf(idc_str, "DISPOS");
                break;
            case NALU_PRIORITY_LOW:
                sprintf(idc_str, "LOW");
                break;
            case NALU_PRIORITY_HIGH:
                sprintf(idc_str, "HIGH");
                break;
            case NALU_PRIORITY_HIGHTEST:
                sprintf(idc_str, "HIGHTEST");
                break;
        }

        char frame_type_str[20] = {0};
        if (getFrameType(n) >= 0) {
            switch (n->frametype) {
                case FRAME_I:
                    sprintf(frame_type_str, "I-Frame");
                    break;
                case FRAME_P:
                    sprintf(frame_type_str, "P-Frame");
                    break;
                case FRAME_B:
                    sprintf(frame_type_str, "B-Frame");
                    break;
                default:
                    sprintf(frame_type_str, "Unknown");
                    break;
            }
        }

        fprintf(myout, "%5d| %8d| %7s| %6s|  %8s| %8d|\n", nal_num, data_offset, idc_str, type_str, frame_type_str, n->len);

        if (out_nalu != NULL && n->nal_unit_type != NALU_TYPE_SEI) {
            out_nalu(n->buf, data_lenth, static_cast<NaluType>(n->nal_unit_type));
        }

        data_offset = data_offset + data_lenth;
        nal_num++;
    }

    // Free
    if (n) {
        if (n->buf) {
            free(n->buf);
            n->buf = NULL;
        }
        free(n);
    }
    return 0;
}
