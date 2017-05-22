
//gcc -Wall -ggdb sdl_play.c  -I/usr/local/include     -L/usr/local/lib -lavformat -lavcodec -lz -lswresample -lswscale -lavutil -lm `sdl-config --cflags --libs` -pthread -o project2.out

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <SDL.h>
#include <SDL_thread.h>

#ifdef __MINGW32__
#undef main /* Prevents SDL from overriding main() 防止 SDL 覆盖主 函数*/
#endif

#include <stdio.h>

int main(int argc, char *argv[]) {
    AVFormatContext *pFormatCtx = NULL;
    int             i, videoStream;
    AVCodecContext  *pCodecCtx = NULL;
    AVCodec         *pCodec = NULL;
    AVFrame         *pFrame = NULL;
    AVPacket        packet;
    int             frameFinished;
    //float           aspect_ratio; //缩放比例
    
    AVDictionary    *optionsDict = NULL;
    struct SwsContext *sws_ctx = NULL;
    
    SDL_Overlay     *bmp = NULL;
    SDL_Surface     *screen = NULL;
    SDL_Rect        rect;
    SDL_Event       event;
    
    if(argc < 2) {
        fprintf(stderr, "Usage: test <file>\n");
        exit(1);
    }
    // Register all formats and codecs 注册所有格式和编解码器
    av_register_all();
    
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }
    
    // Open video file
    if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL)!=0)
        return -1; // Couldn't open file
    
    // Retrieve stream information 获取流信息，寻找视频的格式信息
    if(avformat_find_stream_info(pFormatCtx, NULL)<0)
        return -1; // Couldn't find stream information
    
    // Dump information about file onto standard error 将文件信息转储到标准错误,即输出到控制台
    av_dump_format(pFormatCtx, 0, argv[1], 0);
    
    // Find the first video stream 寻找视频流，一般视频封装文件包含视频流，音频流，字幕流
    videoStream=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            videoStream=i;
            break;
        }
    
    if(videoStream==-1)
        return -1; // Didn't find a video stream
    
    // Get a pointer to the codec context for the video stream 获取指向视频流的编解码器上下文的指针
    pCodecCtx=pFormatCtx->streams[videoStream]->codec;
    
    // Find the decoder for the video stream  查找视频流的解码器，如H264，H265，heav
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        return -1; // Codec not found
    }
    
    // Open codec 打开编码器
    if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0)
        return -1; // Could not open codec
    
    // Allocate video frame 分配视频帧内存
    pFrame=av_frame_alloc();
    
    // Make a screen to put our video ,创建一个窗口用于显示 video
#ifndef __DARWIN__
    screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);
#else
    screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 24, 0);
#endif
    if(!screen) {
        fprintf(stderr, "SDL: could not set video mode - exiting\n");
        exit(1);
    }
    
    // Allocate a place to put our YUV image on that screen 分配一个内存空间 把我们的YUV图像在屏幕上
    bmp = SDL_CreateYUVOverlay(pCodecCtx->width,
                               pCodecCtx->height,
                               SDL_YV12_OVERLAY,
                               screen);
    
    sws_ctx =sws_getContext(
                            pCodecCtx->width,
                            pCodecCtx->height,
                            pCodecCtx->pix_fmt,
                            pCodecCtx->width,
                            pCodecCtx->height,
                            AV_PIX_FMT_YUV420P,
                            SWS_BILINEAR,
                            NULL,
                            NULL,
                            NULL
                            );
    
    // Read frames and save first five frames to disk ，读取帧并保存前五帧到磁盘
    i=0;
    while(av_read_frame(pFormatCtx, &packet)>=0)
    {
        // Is this a packet from the video stream? 判断这是视频流中的数据包吗？
        if(packet.stream_index==videoStream)
        {
            // Decode video frame 解码视频帧
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
            
            // Did we get a video frame? 是否解码完成了帧？
            if(frameFinished)
            {
                SDL_LockYUVOverlay(bmp);
                
                AVPicture pict;
                pict.data[0] = bmp->pixels[0];
                pict.data[1] = bmp->pixels[2];
                pict.data[2] = bmp->pixels[1];
                
                pict.linesize[0] = bmp->pitches[0];
                pict.linesize[1] = bmp->pitches[2];
                pict.linesize[2] = bmp->pitches[1];
                
                // Convert the image into YUV format that SDL uses 将图像转换为YUV格式，SDL使用
                sws_scale(sws_ctx,
                          (uint8_t const * const *)pFrame->data,
                          pFrame->linesize,
                          0,
                          pCodecCtx->height,
                          pict.data,
                          pict.linesize);
                
                SDL_UnlockYUVOverlay(bmp);
                
                rect.x = 0;
                rect.y = 0;
                rect.w = pCodecCtx->width;
                rect.h = pCodecCtx->height;
                SDL_DisplayYUVOverlay(bmp, &rect);
                SDL_Delay(40);
            }
        }
        
        // Free the packet that was allocated by av_read_frame 释放av_read_frame函数开辟的 packet 内存空间
        av_free_packet(&packet);
        SDL_PollEvent(&event);
        switch(event.type) {
            case SDL_QUIT:
                SDL_Quit();
                exit(0);
                break;
            default:
                break;
        }
    }
    
    // Free the YUV frame
    av_free(pFrame);
    
    // Close the codec
    avcodec_close(pCodecCtx);
    
    // Close the video file
    avformat_close_input(&pFormatCtx);
    
    return 0;
}
