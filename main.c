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

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif  // __EMSCRIPTEN__


#define SCALE               0.75f
#define D_WIDTH             800
#define HEADER_SCALE        4
#define LINE                15

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
uint8_t             status      = GAME;
SDL_Event           event       = { 0 };
setting             config      = { 0 };
volatile    bool    quit        = false;
SDL_Window*         win         = NULL;
SDL_Renderer*       ren         = NULL;
TTF_Font*           header      = NULL;
TTF_Font*           body        = NULL;
int8_t              font_size   = 0;



void main_loop( void )
{
    // handle event
    while ( SDL_PollEvent( &event ) )
    {
        switch ( event.type )
        {
            // quit event
            case ( SDL_QUIT ): 
            {
                quit = true;
                // TODO: add emscripten support
                #ifdef __EMSCRIPTEN__
                    // save config file
                    FILE* fp = fopen( "setting.config", "wb+" );
                    fwrite( &config, sizeof (setting), 1, fp );
                    fclose( fp );

                    // clean up
                    SDL_DestroyRenderer( ren );
                    SDL_DestroyWindow( win );
                    SDL_Quit();
                    TTF_Quit();
                    exit(0);
                #endif  // __EMSCRIPTEN__
                break;
            }
            // resize event
            case ( SDL_WINDOWEVENT ):
            {
                switch ( event.window.event )
                {
                    case ( SDL_WINDOWEVENT_MAXIMIZED ):
                    {
                        // forbid window maximization
                        SDL_RestoreWindow( win );
                        break;
                    }
                    case ( SDL_WINDOWEVENT_RESIZED ):
                    {
                        // get new window size and store to config file
                        config.height   = event.window.data2;
                        config.width    = (int) ( (float) event.window.data2 / SCALE );
                        SDL_SetWindowSize( win, config.width, config.height );
                        // change font size when window is resized
                        TTF_CloseFont( header );
                        TTF_CloseFont( body );
                        font_size   = config.height / 50;
                        header      = TTF_OpenFont( "font.ttf", font_size * HEADER_SCALE  );
                        body        = TTF_OpenFont( "font.ttf", font_size );
                        // TODO: Destroy old text surface and texture and create new surface and texture
                        break;
                    }
                    default: break;
                }
                break;
            }
            
            default: break;
        }
    }


    // draw background
    SDL_SetRenderDrawColor( ren, 255, 225, 200, SDL_ALPHA_OPAQUE );
    SDL_RenderClear( ren );

    if ( status == GAME )
    {
        // draw UI background
        SDL_Rect UI_back = 
        {
            .x = config.height,
            .y = 0,
            .w = config.width - config.height,
            .h = config.height,
        };
        SDL_SetRenderDrawColor( ren, 205, 175, 150, SDL_ALPHA_OPAQUE );
        SDL_RenderFillRect( ren, &UI_back );

        // draw board
        SDL_SetRenderDrawColor( ren, 25, 25, 25, SDL_ALPHA_OPAQUE );
        for ( int i = 1; i < LINE; i++ )
        {
            SDL_RenderDrawLine( ren, config.height / ( LINE ) * i, config.height / ( LINE ), 
                                    config.height / ( LINE ) * i, config.height / ( LINE ) * ( LINE - 1 ) );
            SDL_RenderDrawLine( ren, config.height / ( LINE ), config.height / ( LINE ) * i, 
                                    config.height / ( LINE ) * ( LINE - 1 ), config.height / ( LINE ) * i );
        }
    }

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
    win = SDL_CreateWindow( "五子棋", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                                config.width, config.height, 
                                                SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN );
    if ( win == NULL )
    {
        SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "ERROR", "窗口创建失败", NULL );
        return 1;
    }
    ren = SDL_CreateRenderer( win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE );
    if ( ren == NULL )
    {
        SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "ERROR", "渲染器创建失败", NULL );
        return 1;
    }


    // apply setting
    SDL_DisplayMode dm = { 0 };
    SDL_GetDesktopDisplayMode( 0, &dm );
    SDL_SetWindowMinimumSize( win, D_WIDTH, D_WIDTH * SCALE );


    // init font
    TTF_Init();
    font_size   = config.height / 50;
    header      = TTF_OpenFont( "font.ttf", font_size * HEADER_SCALE );
    body        = TTF_OpenFont( "font.ttf", font_size );



    // start game
    #ifndef __EMSCRIPTEN__
        while ( !quit )
        {
            main_loop();
        }
    #elif
        emscripten_set_main_loop( main_loop, 0, true );
    #endif  // __EMSCRIPTEN__

    // save config file
    FILE* fp = fopen( "setting.config", "wb+" );
    fwrite( &config, sizeof (setting), 1, fp );
    fclose( fp );



    TTF_CloseFont( header );
    TTF_CloseFont( body );
    SDL_DestroyRenderer( ren );
    SDL_DestroyWindow( win );
    SDL_Quit();
    TTF_Quit();

    return 0;
}