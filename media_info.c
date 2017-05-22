#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <stdio.h>
int main(int argc, char *argv[]) {
   AVFormatContext   *pFormatCtx = NULL;
   if(argc < 2) {
        printf("Please provide a movie file\n");
        return -1;
  }
   av_register_all();    
   if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL)!=0)
        return -1; // Couldn't open file
   if(avformat_find_stream_info(pFormatCtx, NULL)<0)
        return -1; // Couldn't find stream information

   av_dump_format(pFormatCtx, 0, argv[1], 0);
   avformat_close_input(&pFormatCtx);
   return 0;

}


//gcc -Wall -ggdb media_info.c  -I/usr/local/include     -L/usr/local/lib -lavformat -lavcodec -lz -lswresample -lswscale -lavutil -lm -pthread -o media_info.out
