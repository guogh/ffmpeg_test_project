// gcc -o test2.out sdl_test.c `sdl-config --cflags --libs`


#include "SDL/SDL.h"
int main( int argc, char* args[] )
{
    //声明表面
    SDL_Surface* hello = NULL;
    SDL_Surface* screen = NULL;
    
     //启动SDL
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }

    //设置窗口
    // Make a screen to put our video
#ifndef __DARWIN__
    screen = SDL_SetVideoMode(640, 320, 0, 0);
#else
    screen = SDL_SetVideoMode(640, 320, 24, 0);
#endif
    if(!screen) {
        fprintf(stderr, "SDL: could not set video mode - exiting\n");
        exit(1);
    }
    
    //加载图像
    hello = SDL_LoadBMP( "frame50.ppm" );
    if (hello==NULL) {
        printf("hello is null");
    }
    
    
    //将图像应用到窗口上
    SDL_BlitSurface( hello, NULL, screen, NULL );

    //更新窗口
    SDL_Flip( screen );

    //暂停
    SDL_Delay( 5000 );
    //释放已加载的图像
//    SDL_FreeSurface( hello );

    //退出SDL
//    SDL_Quit();

    return 0;
}
