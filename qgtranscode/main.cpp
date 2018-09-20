//
//  main.cpp
//  qgtranscode
//
//  Created by leolliang on 2018/9/19.
//  Copyright © 2018年 leolliang. All rights reserved.
//

#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
    
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
    
#ifdef __cplusplus
}
#endif

void yuv420pSave(AVFrame *pFrame, int width, int height, int64_t iFrame)
{
    int i = 0;
    FILE *pFile;
    char szFilename[32];
    
    int height_half = height / 2, width_half = width / 2;
    int y_wrap = pFrame->linesize[0];
    int u_wrap = pFrame->linesize[1];
    int v_wrap = pFrame->linesize[2];
    
    unsigned char *y_buf = pFrame->data[0];
    unsigned char *u_buf = pFrame->data[1];
    unsigned char *v_buf = pFrame->data[2];
    sprintf(szFilename, "frame%lld.yuv", iFrame);
    pFile = fopen(szFilename, "wb");
    
    //save y
    for (i = 0; i < height; i++)
        fwrite(y_buf + i * y_wrap, 1, width, pFile);
    fprintf(stderr, "===>save Y success\n");
    
    //save u
    for (i = 0; i < height_half; i++)
        fwrite(u_buf + i * u_wrap, 1, width_half, pFile);
    fprintf(stderr, "===>save U success\n");
    
    //save v
    for (i = 0; i < height_half; i++)
        fwrite(v_buf + i * v_wrap, 1, width_half, pFile);
    fprintf(stderr, "===>save V success\n");
    
    fflush(pFile);
    fclose(pFile);
}

void decodeVideo(AVCodecContext *videoCodecCtx, AVStream *videoStream, AVPacket *avPacket) {
    AVFrame *avFrame = av_frame_alloc();
    int len, gotFrame;
    len = avcodec_decode_video2(videoCodecCtx, avFrame, &gotFrame, avPacket);
    if (len < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error decoding video frame (%d), (%s)\n", len, av_err2str(len));
        av_frame_unref(avFrame);
        av_frame_free(&avFrame);
    } else if (gotFrame) {
        int64_t pts = avFrame->pts == AV_NOPTS_VALUE ? avPacket->dts : avFrame->pts;
        AVRational tb = videoStream->time_base;
        pts = pts * av_q2d(tb) * 1000;
        //        yuv420pSave(avFrame, avFrame->width, avFrame->height, pts);
        av_frame_unref(avFrame);
        av_frame_free(&avFrame);
        //        av_log(NULL, AV_LOG_INFO, "Decoding video frame pts: %lld\n", pts);
    }
}

int decodeStream(const char *url)
{
    int ret;
    unsigned int i;
    AVFormatContext *ifmtCtx = NULL;
    if ((ret = avformat_open_input(&ifmtCtx,url, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot openinput file\n");
        return ret;
    }
    if ((ret = avformat_find_stream_info(ifmtCtx, NULL))< 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot findstream information\n");
        return ret;
    }
    
    AVCodecContext *videoCodecCtx;
    AVStream *videoStream;
    AVCodecContext *audioCodecCtx;
    AVStream *audioStream;
    for (i = 0; i < ifmtCtx->nb_streams; i++) {
        AVStream *stream = ifmtCtx->streams[i];
        AVCodecContext *codecCtx = stream->codec;
        /* Reencode video & audio and remux subtitles etc. */
        if (codecCtx->codec_type == AVMEDIA_TYPE_VIDEO
            || codecCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
            /* Open decoder */
            ret = avcodec_open2(codecCtx, avcodec_find_decoder(codecCtx->codec_id), NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed toopen decoder for stream #%u\n", i);
                return ret;
            }
            if (codecCtx->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoCodecCtx = codecCtx;
                videoStream = stream;
            } else if (codecCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
                audioCodecCtx = codecCtx;
                audioStream = stream;
            }
        }
    }
    
    av_dump_format(ifmtCtx, 0, url, 0);
    
    while (true) {
        AVPacket *avPacket = av_packet_alloc();
        ret = av_read_frame(ifmtCtx, avPacket);
        if (ret != 0) {
            av_log(NULL, AV_LOG_ERROR, "av_read_frame error (%d), (%s)\n", ret, av_err2str(ret));
            av_packet_unref(avPacket);
            av_packet_free(&avPacket);
            return ret;
        }
        
        if (avPacket->stream_index == videoStream->index) {
            decodeVideo(videoCodecCtx, videoStream, avPacket);
        }
        
        av_packet_unref(avPacket);
        av_packet_free(&avPacket);
    }
    
    return 0;
}


int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "Hello, World!\n";
    decodeStream("http://flv265-3954.liveplay.myqcloud.com/live/3954_28748180.flv?bizid=3954&txSecret=efb8e95af2d12d25a5b6e08b4f5e2f76&txTime=5baadb3d");
    return 0;
}
