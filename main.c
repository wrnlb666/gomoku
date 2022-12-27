#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <dirent.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_audio.h>
#include <zmq.h>


#define SCALE               0.75f
#define D_WIDTH             800

// type defines
typedef enum game_status
{
    MENU,
    LOBBY,
    SETTING,
    GAME,
    WIN,
    LOSE
} game_status;

typedef struct setting
{
    uint16_t    width;
    uint16_t    height;
} setting;

typedef enum files
{
    CONFIG_FILE,
    FONT_FILE,
    FILE_MAX,
} files;





// global variable
uint8_t             status  = MENU;
SDL_Event           event   = { 0 };
setting             config  = { 0 };
volatile    bool    quit    = false;



void main_loop( SDL_Window *win, SDL_Renderer *ren )
{
    // handle event
    while ( SDL_PollEvent( &event ) )
    {
        switch ( event.type )
        {
            // quit event
            case ( SDL_QUIT ): 
            {
                quit = true ;
                break;
            }
            // resize event
            case ( SDL_WINDOWEVENT ):
            {
                switch ( event.window.event )
                {
                    case ( SDL_WINDOWEVENT_MAXIMIZED ):
                    {
                        SDL_RestoreWindow( win );
                        break;
                    }
                    case ( SDL_WINDOWEVENT_RESIZED ):
                    {
                        config.height   = event.window.data2;
                        config.width    = (int) ( (float) event.window.data2 / SCALE );
                        SDL_SetWindowSize( win, config.width, config.height );
                        // TODO: change font size when window is resized
                        break;
                    }
                    default: break;
                }
                break;
            }
            
            default: break;
        }
    }


    // draw frame
    SDL_SetRenderDrawColor( ren, 255, 225, 200, SDL_ALPHA_OPAQUE );
    SDL_RenderClear( ren );


    // present
    SDL_RenderPresent( ren );
}


int main( int argc, char** argv )
{
    // unused variable
    (void) argc;
    (void) argv;

    
    // get setting and font
    bool file_check[FILE_MAX] = { 0 };
    struct dirent* file;
    DIR *dir = opendir( "./" );
    while( ( file = readdir(dir)) != NULL )
    {
        if( !strcmp( file->d_name, "font.ttf" ) ) file_check[ FONT_FILE ] = true;
        else if( !strcmp( file->d_name, "setting.config" ) ) file_check[ CONFIG_FILE ] = true;
    }
    for(int i = 0; i < FILE_MAX; i++)
    {
        if( !file_check[i] )
        {
            switch(i)
            {
                case ( FONT_FILE ):
                {
                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "字体丢失", "请在文件夹内添加“font.ttf”字体文件", NULL);
                    return 1;
                }
                case ( CONFIG_FILE ):
                {
                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "游戏配置文件丢失", "游戏配置文件丢失\n将使用默认配置", NULL);
                    config = (setting) 
                    {
                        .width      = D_WIDTH,
                        .height     = D_WIDTH * SCALE,
                    };
                    FILE* fp = fopen( "setting.config", "wb+" );
                    fwrite( &config, sizeof (setting), 1, fp );
                    fclose( fp );
                    break;
                }
            }
        }
        else
        {
            switch(i)
            {
                case ( FONT_FILE ):
                {
                    break;
                }
                case ( CONFIG_FILE ):
                {
                    FILE* fp = fopen( "setting.config", "rb" );
                    fread( &config, sizeof (setting), 1, fp );
                    fclose( fp );
                    break;
                }
            }
        }
    }


    // init
    SDL_Init( SDL_INIT_EVERYTHING );
    SDL_Window      *win    = SDL_CreateWindow( "五子棋", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                                config.width, config.height, 
                                                SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN );
    if ( win == NULL )
    {
        SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "ERROR", "窗口创建失败", NULL );
        return 1;
    }
    SDL_Renderer    *ren    = SDL_CreateRenderer( win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE );
    if ( ren == NULL )
    {
        SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "ERROR", "渲染器创建失败", NULL );
        return 1;
    }


    // apply setting
    SDL_DisplayMode dm = { 0 };
    SDL_GetDesktopDisplayMode( 0, &dm );
    SDL_SetWindowMinimumSize( win, D_WIDTH, D_WIDTH * SCALE );


    


    // start game
    while ( !quit )
    {
        main_loop( win, ren );
    }



    SDL_DestroyRenderer( ren );
    SDL_DestroyWindow( win );
    SDL_Quit();

    return 0;
}