#include "h264_parser.h"

#include <stdlib.h>
#include <string.h>

#include "nalu.h"
#include "parset.h"
#include "slice.h"
#include "spdlog/spdlog.h"
#include "stream.h"

typedef enum { NALU_PRIPORITY_DISPOSABLE = 0, NALU_PRIORITY_LOW = 1, NALU_PRIORITY_HIGH = 2, NALU_PRIORITY_HIGHTEST = 3, a } NaluPriority;

slice_t *currentSlice;  // 当前正在解码的slice

int simplest_h264_parser(const char *url, void (*out_nalu)(unsigned char *buffer, int size, NaluType type))
// int simplest_h264_parser(const char *url)
{
    // 0. 读取h264文件
    int buff_size = readAnnexbBitStreamFile(url);
    printf("totalSize: %d\n", buff_size);

    // 1. 开辟nalu_t保存nalu_header和SODB
    nalu_t *nalu = allocNalu(MAX_NALU_SIZE);
    currentSlice = allocSlice();

    FILE *myout = stdout;
    int nal_num = 0;
    int nal_length = 0;
    int curr_nal_start = 0;   // 当前找到的nalu起始位置
    int curr_find_index = 0;  // 当前查找的位置索引

    printf("-----+-------- NALU Table ------+--------+---------\n");
    printf(" NUM |    POS  |    IDC |  TYPE |  FRAME |  LEN   |\n");
    printf("-----+---------+--------+-------+--------+---------\n");

    // 2.找到h264码流中的各个nalu
    while ((nalu->len = nal_length = find_nal_unit(file_buff, nalu, buff_size, &curr_nal_start, &curr_find_index)) > 0) {
        // 读取/解析 nalu
        read_nal_unit(nalu);
        nal_num++;

        char type_str[20] = {0};
        switch (nalu->nal_unit_type) {
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
            default:
                break;
        }

        char idc_str[20] = {0};
        switch (nalu->nal_ref_idc >> 5) {
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
            default:
                break;
        }

        char frame_type_str[20] = {0};
        if (nalu->nal_unit_type == NALU_TYPE_IDR) {
            sprintf(frame_type_str, "IDR");
        } else if (nalu->nal_unit_type == NALU_TYPE_SLICE) {
            switch (currentSlice->slice_header.slice_type) {
                case Slice_Type_I:
                case Slice_Type_SI:
                    sprintf(frame_type_str, "I");
                    break;
                case Slice_Type_P:
                case Slice_Type_SP:
                    sprintf(frame_type_str, "P");
                    break;
                case Slice_Type_B:
                    sprintf(frame_type_str, "B");
                    break;
                default:
                    break;
            }
        }

        fprintf(myout, "%5d| %8d| %7s| %6s| %8s| %8d|\n", nal_num, nalu->nal_start_index, idc_str, type_str, frame_type_str, nal_length + (curr_nal_start - nalu->nal_start_index));

        if (out_nalu != NULL && nalu->nal_unit_type != NALU_TYPE_SEI) {
            out_nalu(file_buff + nalu->nal_start_index, nal_length + (curr_nal_start - nalu->nal_start_index), static_cast<NaluType>(nalu->nal_unit_type));
        }
    }

    freeSlice(currentSlice);
    freeNalu(nalu);
    freeFilebuffer();

    return 0;
}
