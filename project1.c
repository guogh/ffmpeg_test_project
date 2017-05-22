#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <stdio.h>

//编译脚本
//gcc -Wall -ggdb project1.c  -I/usr/local/include     -L/usr/local/lib -lavformat -lavcodec -lz -lswresample -lswscale -lavutil -lm -pthread -o project1.out


void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
    FILE *pFile;
    char szFilename[32];
    int  y;
    
    // Open file
    sprintf(szFilename, "frame%d.ppm", iFrame);
    pFile=fopen(szFilename, "wb");
    if(pFile==NULL)
        return;
    
    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);
    
    // Write pixel data
    for(y=0; y<height; y++)
        fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
    
    // Close file
    fclose(pFile);
}

int main(int argc, char *argv[]) {
    AVFormatContext *pFormatCtx = NULL;
    int             i, videoStream;
    AVCodecContext  *pCodecCtx = NULL;
    AVCodec         *pCodec = NULL;
    AVFrame         *pFrame = NULL;
    AVFrame         *pFrameRGB = NULL;
    AVPacket        packet;
    int             frameFinished;
    int             numBytes;
    uint8_t         *buffer = NULL;
    
    AVDictionary    *optionsDict = NULL;
    struct SwsContext      *sws_ctx = NULL;
    
    if(argc < 2) {
        printf("Please provide a movie file\n");
        return -1;
    }
    // Register all formats and codecs
    av_register_all();
    
    // Open video file
    if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL)!=0)
        return -1; // Couldn't open file
    
    // Retrieve stream information 寻找封装格式
    if(avformat_find_stream_info(pFormatCtx, NULL)<0)
        return -1; // Couldn't find stream information
    
    // Dump information about file onto standard error
    av_dump_format(pFormatCtx, 0, argv[1], 0);
    
    // Find the first video stream 寻找视频流，一般封装格式包含 视频流，音频流，字幕流
    videoStream=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            videoStream=i;
            break;
        }
    if(videoStream==-1)
        return -1; // Didn't find a video stream
    
    // Get a pointer to the codec context for the video stream  获取指向视频流的编解码器上下文的指针
    pCodecCtx=pFormatCtx->streams[videoStream]->codec;
    
    // Find the decoder for the video stream  寻找解码器
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
        fprintf(stderr, "Unsupported codec!\n"); //不支持的编码方式
        return -1; // Codec not found
    }
    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0)
        return -1; // Could not open codec
    
    // Allocate video frame   创建 获取视频帧 内存
    pFrame=av_frame_alloc();
    
    // Allocate an AVFrame structure 创建 获取视频帧 内存
    pFrameRGB=av_frame_alloc();
    if(pFrameRGB==NULL)
        return -1;
    
    // Determine（确定计算） required buffer size and allocate buffer
    numBytes=avpicture_get_size(AV_PIX_FMT_RGB24,
                                pCodecCtx->width,
                                pCodecCtx->height);
    
    buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
    
    sws_ctx = sws_getContext(pCodecCtx->width,
                             pCodecCtx->height,
                             pCodecCtx->pix_fmt,
                             pCodecCtx->width,
                             pCodecCtx->height,
                             AV_PIX_FMT_RGB24,
                             SWS_BILINEAR,
                             NULL,
                             NULL,
                             NULL);
    
    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24,pCodecCtx->width, pCodecCtx->height);
    
    // Read frames and save first five frames to disk
    i=0;
    while(av_read_frame(pFormatCtx, &packet)>=0) {
        // Is this a packet from the video stream?
        if(packet.stream_index==videoStream) {
            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished,
                                  &packet);
            
            // Did we get a video frame?
            if(frameFinished) {
                // Convert the image from its native format to RGB
                sws_scale
                (
                 sws_ctx,
                 (uint8_t const * const *)pFrame->data,
                 pFrame->linesize,
                 0,
                 pCodecCtx->height,
                 pFrameRGB->data,
                 pFrameRGB->linesize
                 );
                
                // Save the frame to disk
                
                SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height,
                          i);
                printf("decode %d frame\n", i);
                i++;
            }
        }
        
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }
    
    // Free the RGB image
    av_free(buffer);
    av_free(pFrameRGB);
    
    // Free the YUV frame
    av_free(pFrame);
    
    // Close the codec
    avcodec_close(pCodecCtx);
    
    // Close the video file
    avformat_close_input(&pFormatCtx);
    
    return 0;
}
