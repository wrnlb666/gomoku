#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <gc.h>
#include <time.h>
#include <signal.h>
#include <dirent.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_audio.h>
#include <zmq.h>


#define gmalloc( sz )       GC_MALLOC( sz )
#define grealloc( ptr, sz ) GC_REALLOC( ptr, sz )
#define gfree( ptr )        GC_FREE( ptr )

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
    bool        fullscreen;
} setting;

typedef enum files
{
    CONFIG_FILE,
    FONT_FILE,
    FILE_MAX,
} files;





// global variable
uint8_t     status  = MENU;
SDL_Event   event   = { 0 };
setting     config  = { 0 };



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
                        .width      = 800,
                        .height     = 600,
                        .fullscreen = false,
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
    SDL_Window      *win    = SDL_CreateWindow( "五子棋", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_GRABBED | SDL_WINDOW_OPENGL );
    if ( win == NULL )
    {
        return 1;
    }
    SDL_Renderer    *ren    = SDL_CreateRenderer( win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
    if ( ren == NULL )
    {
        return 1;
    }


    // apply setting
    if ( config.fullscreen == true )
    {
        SDL_SetWindowFullscreen( win, SDL_WINDOW_FULLSCREEN );
    }
    SDL_DisplayMode dm = { 0 };
    SDL_SetWindowDisplayMode( win, )






    SDL_DestroyRenderer( ren );
    SDL_DestroyWindow( win );
    SDL_Quit();

    return 0;
}