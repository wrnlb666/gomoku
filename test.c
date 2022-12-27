#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <gc.h>
#include <time.h>
#include <signal.h>
#include <dirent.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_audio.h>
#include <zmq.h>

SDL_Event           event   = { 0 };
volatile    bool    quit    = false;

int main( int argc, char** argv ){


    SDL_Init( SDL_INIT_EVERYTHING );
    SDL_Window      *win    = SDL_CreateWindow( "五子棋", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                                800, 600, 
                                                SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN  );
    if ( win == NULL )
    {
        SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "ERROR", "窗口创建失败", NULL);
        return 1;
    }
    SDL_Renderer    *ren    = SDL_CreateRenderer( win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
    if ( ren == NULL )
    {
        return 1;
    }


    while ( !quit )
    {
        while ( SDL_PollEvent( &event ) )
        {
            switch ( event.type )
            {
                case SDL_QUIT:
                {
                    quit = true;
                    break;
                }
                case SDL_WINDOWEVENT:
                {
                    switch ( event.window.event )
                    {
                        case SDL_WINDOWEVENT_RESIZED:
                        {
                            SDL_RenderPresent( ren );
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }





    return 0;
}



