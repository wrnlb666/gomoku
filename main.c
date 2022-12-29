#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_net.h>

#include "helper.h"


#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
    #include "helper.c"
#endif  // __EMSCRIPTEN__


#define SCALE               0.75f
#define D_WIDTH             800
#define HEADER_SCALE        4
#define LINE                15
#define FONT_SCALE          50
#define D_HOST              "localhost"

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

typedef struct piece
{
    uint8_t     x;
    uint8_t     y;
    bool        black;
} piece;

typedef struct board_t
{
    size_t      size;
    piece       pieces[ LINE * LINE ];
    bool        black;
} board_t;

typedef enum files
{
    CONFIG_FILE,
    FONT_FILE,
    FILE_MAX,
} files;






// global variable
uint8_t             status      = GAME;
SDL_DisplayMode     dm          = { 0 };
SDL_Event           event       = { 0 };
setting             config      = { 0 };
board_t             board       = { 0 };
SDL_Window*         win         = NULL;
SDL_Renderer*       ren         = NULL;
TTF_Font*           header      = NULL;
TTF_Font*           body        = NULL;
int8_t              font_size   = 0;
volatile    bool    quit        = false;
_Atomic     bool    my_turn     = false;



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
                        config.width        = event.window.data1;
                        config.height       = event.window.data2;
                        if ( config.width > (int) ( (float) dm.h / SCALE ) )
                        {
                            config.height   = dm.h;
                            config.width    = (int) ( (float) config.height / SCALE );
                        }
                        else
                        {
                            config.width    = (int) ( (float) ( config.width + config.height / SCALE ) / 2 );
                            config.height   = (int) ( (float) config.width * SCALE );
                        }
                        SDL_SetWindowSize( win, config.width, config.height );
                        // change font size when window is resized
                        TTF_CloseFont( header );
                        TTF_CloseFont( body );
                        font_size   = config.height / FONT_SCALE;
                        header      = TTF_OpenFont( "font.ttf", font_size * HEADER_SCALE  );
                        body        = TTF_OpenFont( "font.ttf", font_size );
                        // TODO: Destroy old text surface and texture and create new surface and texture

                        break;
                    }
                    default: break;
                }
                break;
            }
            // handle mouse event
            case ( SDL_MOUSEBUTTONUP ):
            {
                switch ( status )
                {
                    // in game click
                    case (GAME):
                    {
                        if ( event.button.button == SDL_BUTTON_LEFT )
                        {
                            // check which box is clicked
                            if ( event.button.x < ( config.height - ( config.height / LINE ) / 2 ) && event.button.x > ( config.height / LINE ) / 2 && 
                                event.button.y < ( config.height - ( config.height / LINE ) / 2 ) && event.button.y > ( config.height / LINE ) / 2)
                            {
                                board.pieces[ board.size ].x = event.button.x / ( config.height / LINE );
                                board.pieces[ board.size ].y = event.button.y / ( config.height / LINE );
                                

                                // debug report
                                #ifdef TEST
                                    char* str = malloc( 50 );
                                    snprintf( str, 49, "You clicked \nx: %d \ny: %d", board.pieces[board.size].x, board.pieces[board.size].y );
                                    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "INFO", str, win );
                                    free(str);
                                #endif // TEST
                                

                                // TODO: check if my_turn then display piece
                            }
                            // check which button of the UI part is clicked
                            else if ( event.button.x > config.height )
                            {
                                
                            }
                        }
                        break;
                    }
                }

                break;
            }
            
            default: break;
        }
    }


    // draw background
    SDL_SetRenderDrawColor( ren, 255, 225, 200, SDL_ALPHA_OPAQUE );
    SDL_RenderClear( ren );

    // draw board background
    SDL_Rect board_back = 
    {
        .x = ( config.height / LINE ) / 2,
        .y = ( config.height / LINE ) / 2,
        .w = config.height - ( config.height / LINE ),
        .h = config.height - ( config.height / LINE ),
    };
    SDL_SetRenderDrawColor( ren, 225, 205, 175, SDL_ALPHA_OPAQUE );
    SDL_RenderFillRect( ren, &board_back );

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

    switch ( status )
    {
        case ( GAME ):
        {
            // draw board
            SDL_SetRenderDrawColor( ren, 25, 25, 25, SDL_ALPHA_OPAQUE );
            for ( int i = 1; i < LINE; i++ )
            {
                SDL_RenderDrawLine( ren, config.height / ( LINE ) * i, config.height / ( LINE ), 
                                    config.height / ( LINE ) * i, config.height / ( LINE ) * ( LINE - 1 ) );
                SDL_RenderDrawLine( ren, config.height / ( LINE ), config.height / ( LINE ) * i, 
                                    config.height / ( LINE ) * ( LINE - 1 ), config.height / ( LINE ) * i );
            }
            // draw UI
            

            break;
        }

        default: break;
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
    bool file_check[ FILE_MAX ] = { 0 };
    struct dirent* file;
    DIR *dir = opendir( "./" );
    while( ( file = readdir(dir) ) != NULL )
    {
        if( !strcmp( file->d_name, "font.ttf" ) ) file_check[ FONT_FILE ] = true;
        else if( !strcmp( file->d_name, "setting.config" ) ) file_check[ CONFIG_FILE ] = true;
    }
    for(int i = 0; i < FILE_MAX; i++)
    {
        if( !file_check[i] )
        {
            switch ( i )
            {
                case ( FONT_FILE ):
                {
                    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Font Missing", "Please add \"font.ttf\" into the current directory", NULL );
                    return 1;
                }
                case ( CONFIG_FILE ):
                {
                    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "config file missing", "config file missing\nwill use default setting", NULL );
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
            switch ( i )
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
                                                SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN );
    if ( win == NULL )
    {
        SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "ERROR", "Failed to create window", NULL );
        return 1;
    }
    ren = SDL_CreateRenderer( win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE );
    if ( ren == NULL )
    {
        SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "ERROR", "Failed to create renderer", NULL );
        return 1;
    }


    // apply setting
    SDL_GetCurrentDisplayMode( 0, &dm );
    SDL_SetWindowMinimumSize( win, D_WIDTH, D_WIDTH * SCALE );


    // init font
    TTF_Init();
    font_size   = config.height / FONT_SCALE;
    header      = TTF_OpenFont( "font.ttf", font_size * HEADER_SCALE );
    body        = TTF_OpenFont( "font.ttf", font_size );


    // init net
    SDLNet_Init();


    // start game
    #ifndef __EMSCRIPTEN__
        while ( !quit )
        {
            main_loop();
        }
    #endif  // __EMSCRIPTEN__
    #ifdef __EMSCRIPTEN__
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
    SDLNet_Quit();
    TTF_Quit();
    SDL_Quit();

    return 0;
}
