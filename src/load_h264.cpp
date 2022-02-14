#include <string.h>

#include <vector>

#include "cnalu.h"
#include "h264_parser.h"
#include "stdlib.h"

using namespace std;

vector<Nalu *> nalu_vector;

char sps_data[128] = {0};
int sps_data_length = 0;

char pps_data[128] = {0};
int pps_data_length = 0;

static void dump(unsigned char *buffer, int length, int type) {
    printf("\n=======================\n");
    printf("NALU type: %d, length: %d\n", type, length);
    for (int i = 0; i < length; i += 8) {
        for (int j = 0; j < 8; j++) {
            if (i + j < length) {
                printf("%02X ", buffer[i + j]);
            } else {
                printf("   ");
            }
        }
        printf("\n");
    }
    printf("=======================\n\n");
}

void out_nalu(unsigned char *buffer, int size, NaluType naluType, int time_base, int pts, int dts) {
    // dump((unsigned char *)buffer, size, naluType);
    // std::cout << "rtp_packet >>> " << binToHex((unsigned char *)buffer, size);

    if (NALU_TYPE_SPS == naluType) {
        memcpy(sps_data, buffer, size);
        sps_data_length = size;
        return;
    }

    if (NALU_TYPE_PPS == naluType) {
        memcpy(pps_data, buffer, size);
        pps_data_length = size;
        return;
    }

    Nalu *nalu = new Nalu;
    bool is_i_frame = (NALU_TYPE_IDR == naluType);

    char *packet = (char *)malloc(is_i_frame ? (size + sps_data_length + pps_data_length) : size * sizeof(char));
    if (is_i_frame) {
        memcpy(packet, sps_data, sps_data_length);
        memcpy(packet + sps_data_length, pps_data, pps_data_length);
        memcpy(packet + sps_data_length + pps_data_length, buffer, size);
        size += (sps_data_length + pps_data_length);
        nalu->spsLength = sps_data_length;
        nalu->ppsLength = pps_data_length;
    } else {
        memcpy(packet, buffer, size);
    }
    nalu->packet = packet;
    nalu->length = size;
    nalu->type = naluType;
    nalu->time_base = time_base;
    nalu->pts = pts;
    nalu->dts = dts;

    nalu_vector.push_back(nalu);
}

int load(const char *path) { return simplest_h264_parser(path, out_nalu); }