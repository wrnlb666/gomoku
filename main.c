#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_image.h>

// #include "helper.h"


#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif  // __EMSCRIPTEN__


#define SCALE               0.75f
#define D_WIDTH             800
#define HEADER_SCALE        4
#define LINE                15
#define D_HOST              "localhost"
#define D_PORT              4396

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

typedef enum owner
{
    NONE,
    BLACK,
    WHITE,
} owner;

typedef struct piece
{
    uint8_t     x;                      // x axis
    uint8_t     y;                      // y axis
    uint8_t     player;                 // enum owner, which player does the piece belongs to
} piece;

typedef struct board_t
{
    size_t      size;                   // The current count of pieces
    piece       pieces[ LINE * LINE ];  // The struct to store all of the pieces
    uint8_t     player;                 // Which player is taking turn now. 
} board_t;

typedef struct message
{
    uint8_t     player;                 // to imply which player this client is
    piece       new_piece;              // new piece to send/receive
    char        text[48];               // text messgae to the other player
    size_t      text_len;               // length of text
} message;

typedef enum files
{
    CONFIG_FILE,
    BLACK_FILE,
    WHITE_FILE,
    FILE_MAX,
} files;





// global variable
uint8_t             status      = GAME;
int                 p_size      = 27;
SDL_DisplayMode     dm          = { 0 };
SDL_Event           event       = { 0 };
setting             config      = { 0 };
board_t             board       = { 0 };
SDL_Rect*           pieces      = NULL;
SDL_Window*         win         = NULL;
SDL_Renderer*       ren         = NULL;
SDL_Texture*        b_tex       = NULL;
SDL_Texture*        w_tex       = NULL;
volatile    bool    quit        = false;
_Atomic     uint8_t curr_player = NONE;







// game main loop
void main_loop( void )
{
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

                        // TODO: Destroy old text surface and texture and create new surface and texture
                        // set piece size
                        p_size = ( config.height / LINE ) / 2;

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
                                board.pieces[ board.size ].x = ( ( event.button.x + ( config.height / LINE ) / 2 ) ) / ( config.height / LINE );
                                board.pieces[ board.size ].y = ( ( event.button.y + ( config.height / LINE ) / 2 ) ) / ( config.height / LINE );
                                

                                // debug report
                                #ifdef TEST
                                    char* str = malloc( 50 );
                                    snprintf( str, 49, "You clicked \nx: %d \ny: %d", board.pieces[board.size].x, board.pieces[board.size].y );
                                    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "INFO", str, win );
                                    free(str);
                                #endif // TEST
                                

                                
                            }
                            // check which button of the UI part is clicked
                            else if ( event.button.x > config.height )
                            {
                                
                            }
                        }
                        break;
                    }
                    // menu click
                    case (MENU):
                    {
                        
                        break;
                    }
                }

                break;
            }
            
            default: break;
        }
    }


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
            // draw ready piece
            SDL_Rect temp = 
            {
                .h = p_size,
                .w = p_size,
                .x = config.height / LINE * board.pieces[board.size].x - p_size / 2,
                .y = config.height / LINE * board.pieces[board.size].y - p_size / 2,
            };
            if ( board.pieces[board.size].x != 0 && board.pieces[board.size].y != 0 )
            {
                SDL_RenderCopy( ren, w_tex, NULL, &temp );
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
    #ifdef __EMSCRIPTEN__
        DIR *dir = opendir( "assets" );
        chdir( "assets" );
    #endif  // __EMSCRIPTEN__
    #ifndef __EMSCRIPTEN__
        DIR *dir = opendir( "assets" );
        chdir( "assets" );
    #endif  // __EMSCRIPTEN__
    while( ( file = readdir(dir) ) != NULL )
    {
        if ( !strcmp( file->d_name, "setting.config" ) )        file_check[ CONFIG_FILE ]   = true;
        else if ( !strcmp( file->d_name, "black.png" ) )        file_check[ BLACK_FILE ]    = true;
        else if ( !strcmp( file->d_name, "white.png" ) )        file_check[ WHITE_FILE ]    = true;
    }
    for(int i = 0; i < FILE_MAX; i++)
    {
        if( !file_check[i] )
        {
            switch ( i )
            {
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
                case( BLACK_FILE ):
                {
                    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "ERRO", "Missing texture file", NULL );
                    return 1;
                }
                case( WHITE_FILE ):
                {
                    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "ERRO", "Missing texture file", NULL );
                    return 1;
                }
            }
        }
        else
        {
            switch ( i )
            {
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


    // init net
    if ( SDLNet_Init() )
    {
        SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "ERRO", "Can not initialize network", win );
        return 1;
    }

    // init img
    #ifndef __EMSCRIPTEN__
        if ( IMG_Init( IMG_INIT_PNG ) != IMG_INIT_PNG )
        {
            SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "ERRO", "Can not initialize texture", win );
            return 1;
        }
    #endif  // __EMSCRIPTEN__
    
    // load texture
    w_tex = IMG_LoadTexture( ren, "white.png" );
    b_tex = IMG_LoadTexture( ren, "black.png" );
    assert( w_tex && b_tex );
    pieces = malloc( sizeof ( SDL_Rect ) * LINE * LINE );



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

    free( pieces );
    SDL_DestroyTexture( w_tex );
    SDL_DestroyTexture( b_tex );
    SDL_DestroyRenderer( ren );
    SDL_DestroyWindow( win );
    SDLNet_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
