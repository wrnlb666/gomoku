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
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

// #include "helper.h"


#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif  // __EMSCRIPTEN__


#define SCALE                   0.75f
#define D_WIDTH                 800
#define HEADER_SCALE            4
#define LINE                    15
#define D_HOST                  "localhost"
#define D_PORT                  4396
#define D_FONT                  "Minimal5x7.ttf"
#define F_COLOR                 (SDL_Color) { 0, 25, 50, SDL_ALPHA_OPAQUE }//, (SDL_Color) { 155, 125, 200, SDL_ALPHA_OPAQUE }

// type defines
typedef enum game_status
{
    MENU,
    CREATE,
    JOIN,
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
    int32_t     room_id;                // room id, most likely to be the thread id of that room at the server side
    uint8_t     player;                 // to imply which player this client is
    piece       new_piece;              // new piece to send/receive
    bool        timeout;                // if the current player or the opponent timeouts (auto win)
} message;

typedef enum files
{
    CONFIG_FILE,
    FONT_FILE,
    BLACK_FILE,
    WHITE_FILE,
    FILE_MAX,
} files;

typedef enum curr_resolution
{
    P600,
    P720,
    P1080,
    P1440,
} curr_resolution;





// global variable
uint8_t             status              = MENU;         // game status
uint8_t             curr_res            = P600;         // game resolution
int                 p_size              = 0;            // piece size
int                 h_size              = 48;           // header font size
int                 b_size              = 24;           // body font size
int                 room_id_num         = 0;            // room id
SDL_DisplayMode     dm                  = { 0 };        // maybe unused?
SDL_Event           event               = { 0 };        // global event
setting             config              = { 0 };        // game setting
board_t             board               = { 0 };        // local board
SDL_Rect            pieces[LINE][LINE]  = { 0 };        // all the rect of which a piece should be at
char                room_id_text[10]    = { 0 };        // get input for room id
SDL_Window*         win                 = NULL;
SDL_Renderer*       ren                 = NULL;
SDL_Texture*        b_tex               = NULL;         // texture for black piece
SDL_Texture*        w_tex               = NULL;         // texture for white piece
TTF_Font*           h_font              = NULL;         // header font, bigger size
TTF_Font*           b_font              = NULL;         // body font, smaller size
volatile    bool    quit                = false;        // if the game should end or not
_Atomic     uint8_t curr_player         = NONE;         // enum owner, which player is the current player



// surface
SDL_Surface*        menu_sur            = NULL;
SDL_Surface*        con_sur             = NULL;
SDL_Surface*        setting_sur         = NULL;
SDL_Surface*        create_sur          = NULL;
SDL_Surface*        join_sur            = NULL;
SDL_Surface*        exit_sur            = NULL;
SDL_Surface*        res_sur             = NULL;
SDL_Surface*        r600_sur            = NULL;
SDL_Surface*        r720_sur            = NULL;
SDL_Surface*        r1080_sur           = NULL;
SDL_Surface*        r1440_sur           = NULL;
SDL_Surface*        grid_sur            = NULL;
SDL_Surface*        irid_sur            = NULL;
SDL_Surface*        copy_sur            = NULL;
SDL_Surface*        paste_sur           = NULL;

// texture
SDL_Texture*        menu_tex            = NULL;
SDL_Texture*        con_tex             = NULL;
SDL_Texture*        setting_tex         = NULL;
SDL_Texture*        create_tex          = NULL;
SDL_Texture*        join_tex            = NULL;
SDL_Texture*        exit_tex            = NULL;
SDL_Texture*        res_tex             = NULL;
SDL_Texture*        r600_tex            = NULL;
SDL_Texture*        r720_tex            = NULL;
SDL_Texture*        r1080_tex           = NULL;
SDL_Texture*        r1440_tex           = NULL;
SDL_Texture*        grid_tex            = NULL;
SDL_Texture*        irid_tex            = NULL;
SDL_Texture*        copy_tex            = NULL;
SDL_Texture*        paste_tex           = NULL;

// texture posiiton
SDL_Rect            menu_rec            = { 0 };
SDL_Rect            con_rec             = { 0 };
SDL_Rect            setting_rec         = { 0 };
SDL_Rect            create_rec          = { 0 };
SDL_Rect            join_rec            = { 0 };
SDL_Rect            exit_rec            = { 0 };
SDL_Rect            res_rec             = { 0 };
SDL_Rect            reso_rec            = { 0 };
SDL_Rect            grid_rec            = { 0 };
SDL_Rect            irid_rec            = { 0 };
SDL_Rect            irid_temp_rec       = { 0 };
SDL_Rect            copy_rec            = { 0 };
SDL_Rect            paste_rec           = { 0 };




// change rect positiona and size
void adjust_size( void )
{
    // menu UI rect
    menu_rec = (SDL_Rect)
    {
        .w = ( config.width - config.height ) / 8 * 4,
        .h = config.height / 17,
    };
    menu_rec.x   = ( config.height ) + ( config.width - config.height - menu_rec.w ) / 2;
    menu_rec.y   = config.height / 5 * 1;

    setting_rec = (SDL_Rect)
    {
        .w = ( config.width - config.height ) / 8 * 7,
        .h = config.height / 17,
    };
    setting_rec.x   = ( config.height ) + ( config.width - config.height - setting_rec.w ) / 2;
    setting_rec.y   = config.height / 5 * 4;

    create_rec = (SDL_Rect)
    {
        .w = ( config.width - config.height ) / 8 * 6,
        .h = config.height / 17,
    };
    create_rec.x    = ( config.height ) + ( config.width - config.height - create_rec.w ) / 2;
    create_rec.y    = config.height / 5 * 2;

    join_rec = (SDL_Rect)
    {
        .w = ( config.width - config.height ) / 8 * 4,
        .h = config.height / 17,
    };
    join_rec.x      = ( config.height ) + ( config.width - config.height - join_rec.w ) / 2;
    join_rec.y      = config.height / 5 * 3;

    exit_rec = (SDL_Rect)
    {
        .w = ( config.height ) / 24 * 4,
        .h = config.height / 17,
    };
    exit_rec.x      = ( config.height - exit_rec.w ) / 2;
    exit_rec.y      = config.height / 3 * 2;

    res_rec = (SDL_Rect)
    {
        .w = ( config.height ) / 24 * 12,
        .h = config.height / 17,
    };
    res_rec.x       = ( config.height - res_rec.w ) / 2;
    res_rec.y       = config.height / 3 * 1;

    reso_rec = (SDL_Rect)
    {
        .w = ( config.height ) / 24 * 9,
        .h = config.height / 17,
    };
    reso_rec.x      = ( config.height - reso_rec.w ) / 2;
    reso_rec.y      = config.height / 3 * 2;

    grid_rec = (SDL_Rect)
    {
        .w = ( config.height ) / 24 * 22,
        .h = config.height / 17,
    };
    grid_rec.x      = ( config.height - grid_rec.w ) / 2;
    grid_rec.y      = config.height / 3 * 1;

    irid_rec = (SDL_Rect)
    {
        .w = ( config.height ) / 24 * 8,
        .h = config.height / 17,
    };
    irid_rec.x      = ( config.height - irid_rec.w ) / 2;
    irid_rec.y      = config.height / 3 * 2;

    irid_temp_rec = (SDL_Rect)
    {
        .w = ( config.height ) / 24 * strlen( room_id_text ),
        .h = config.height / 17,
    };
    irid_temp_rec.x      = ( config.height - irid_temp_rec.w ) / 2;
    irid_temp_rec.y      = config.height / 3 * 2;

    paste_rec = (SDL_Rect)
    {
        .w = ( config.height ) / 24 * 5,
        .h = config.height / 17,
    };
    paste_rec.x     = irid_rec.x + irid_rec.w + config.height / 24;
    paste_rec.y     = irid_rec.y;



    // game UI rect
    con_rec = (SDL_Rect)
    {
        .w = ( config.width - config.height ) / 8 * 7,
        .h = config.height / 17,
    };
    con_rec.x    = ( config.height ) + ( config.width - config.height - con_rec.w ) / 2;
    con_rec.y    = config.height / 4 * 3;





    // set piece size
    p_size = ( config.height / LINE ) / 3 * 2;
    for ( int i = 0; i < LINE ; i++ )
    {
        for ( int j = 0; j < LINE; j++ )
        {
            pieces[i][j] = ( SDL_Rect )
            {
                .h = p_size,
                .w = p_size,
                .x = config.height / LINE * i - p_size / 2,
                .y = config.height / LINE * j - p_size / 2,
            };
        }
    }

    return;
}

void render_ui( void )
{
    SDL_RenderCopy( ren, menu_tex, NULL, &menu_rec );
    SDL_RenderCopy( ren, setting_tex, NULL, &setting_rec );
    SDL_RenderCopy( ren, create_tex, NULL, &create_rec );
    SDL_RenderCopy( ren, join_tex, NULL, &join_rec );

    return;
}

void ui_click( void )
{
    if ( event.button.x >= menu_rec.x && event.button.y >= menu_rec.y && 
    event.button.x <= menu_rec.x + menu_rec.w && event.button.y <= menu_rec.y + menu_rec.h )
    {
        status = MENU;
        if ( SDL_IsTextInputActive() )
        {
            SDL_StopTextInput();
        }
    }
    // create button
    else if ( event.button.x >= create_rec.x && event.button.y >= create_rec.y && 
    event.button.x <= create_rec.x + create_rec.w && event.button.y <= create_rec.y + create_rec.h )
    {
        status = CREATE;
        if ( SDL_IsTextInputActive() )
        {
            SDL_StopTextInput();
        }
    }
    // join button
    else if ( event.button.x >= join_rec.x && event.button.y >= join_rec.y && 
    event.button.x <= join_rec.x + join_rec.w && event.button.y <= join_rec.y + join_rec.h )
    {
        status = JOIN;
        if ( !SDL_IsTextInputActive() )
        {
            SDL_StartTextInput();
        }
    }
    // setting button
    else if ( event.button.x >= setting_rec.x && event.button.y >= setting_rec.y && 
    event.button.x <= setting_rec.x + setting_rec.w && event.button.y <= setting_rec.y + setting_rec.h )
    {
        status = SETTING;
        if ( SDL_IsTextInputActive() )
        {
            SDL_StopTextInput();
        }
    }
    
    return;
}

void clean_up( void )
{
    // save config file
    FILE* fp = fopen( "setting.config", "wb+" );
    fwrite( &config, sizeof (setting), 1, fp );
    fclose( fp );

    // free surface
    SDL_FreeSurface( irid_sur );

    // free texture
    SDL_DestroyTexture( w_tex );
    SDL_DestroyTexture( b_tex );
    SDL_DestroyTexture( menu_tex );
    SDL_DestroyTexture( con_tex );
    SDL_DestroyTexture( setting_tex );
    SDL_DestroyTexture( create_tex );
    SDL_DestroyTexture( join_tex );
    SDL_DestroyTexture( exit_tex );
    SDL_DestroyTexture( res_tex );
    SDL_DestroyTexture( r600_tex );
    SDL_DestroyTexture( r720_tex );
    SDL_DestroyTexture( r1080_tex );
    SDL_DestroyTexture( r1440_tex );
    SDL_DestroyTexture( grid_tex );
    SDL_DestroyTexture( irid_tex );
    SDL_DestroyTexture( copy_tex );
    SDL_DestroyTexture( paste_tex );

    // close font
    TTF_CloseFont( h_font );
    TTF_CloseFont( b_font );

    // free renderer and window
    SDL_DestroyRenderer( ren );
    SDL_DestroyWindow( win );

    // quit SDL
    SDLNet_Quit();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

}

void adjust_rid( void )
{
    SDL_FreeSurface( irid_sur );
    SDL_DestroyTexture( irid_tex );
    adjust_size();
    irid_sur    = TTF_RenderText_Blended( h_font, room_id_text, F_COLOR );
    irid_tex    = SDL_CreateTextureFromSurface( ren, irid_sur );

    return;
}



// game main loop
void main_loop( void )
{
    // draw background
    SDL_SetRenderDrawColor( ren, 255, 225, 200, SDL_ALPHA_OPAQUE );
    SDL_RenderClear( ren );

    // draw board background
    if ( status == GAME )
    {
        SDL_Rect board_back = 
        {
            .x = ( config.height / LINE ) / 2,
            .y = ( config.height / LINE ) / 2,
            .w = config.height - ( config.height / LINE ),
            .h = config.height - ( config.height / LINE ),
        };
        SDL_SetRenderDrawColor( ren, 225, 205, 175, SDL_ALPHA_OPAQUE );
        SDL_RenderFillRect( ren, &board_back );
    }

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
                    clean_up();
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


                        // change rect position and size
                        adjust_size();


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
                    case ( MENU ):
                    {
                        // menu button
                        ui_click();

                        // exit button click detect
                        if ( event.button.x >= exit_rec.x && event.button.y >= exit_rec.y && 
                        event.button.x <= exit_rec.x + exit_rec.w && event.button.y <= exit_rec.y + exit_rec.y )
                        {
                            quit = true;
                            #ifdef __EMSCRIPTEN__
                                clean_up();
                                emscripten_force_exit( 0 );
                            #endif  // __EMSCRIPTEN__
                        }
                        
                        
                        break;
                    }
                    case ( CREATE ):
                    {
                        ui_click();



                        break;
                    }
                    case ( JOIN ):
                    {
                        ui_click();

                        // paste button
                        if ( event.button.x >= paste_rec.x && event.button.y >= paste_rec.y && 
                        event.button.x <= paste_rec.x + paste_rec.w && event.button.y <= paste_rec.y + paste_rec.y )
                        {
                            memset( room_id_text, 0, 10 );
                            char* rid = SDL_GetClipboardText();
                            strncpy( room_id_text, rid, 8 );
                            SDL_free( rid );
                            adjust_rid();
                            #ifdef __EMSCRIPTEN__
                                SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "INFO", "Paste doesn't work in the browser.", win );
                            #endif  // __EMSCRIPTEN__
                        }

                        break;
                    }
                    case ( SETTING ):
                    {
                        ui_click();

                        // resolution clicked detect
                        if ( event.button.x >= reso_rec.x && event.button.y >= reso_rec.y && 
                        event.button.x <= reso_rec.x + reso_rec.w && event.button.y <= reso_rec.y + reso_rec.y )
                        {
                            switch ( curr_res )
                            {
                                case ( P600 ):
                                {
                                    curr_res            = P720;
                                    if ( dm.w >= 960 && dm.h >= 720 )
                                    {
                                        config.width    = 960;
                                        config.height   = 720;
                                    }
                                    break;
                                }
                                case ( P720 ):
                                {
                                    curr_res            = P1080;
                                    if ( dm.w >= 1440 && dm.h >= 1080 )
                                    {
                                        config.width    = 1440;
                                        config.height   = 1080;
                                    }
                                    break;
                                }
                                case ( P1080 ):
                                {
                                    curr_res            = P1440;
                                    if ( dm.w >= 1920 && dm.h >= 1440 )
                                    {
                                        config.width    = 1920;
                                        config.height   = 1440;
                                    }
                                    break;
                                }
                                case ( P1440 ):
                                {
                                    curr_res        = P600;
                                    config.width    = 800;
                                    config.height   = 600;
                                    break;
                                }
                                default: break;
                            }
                            if ( config.width <= dm.w && config.height <= dm.h )
                            {
                                SDL_SetWindowSize( win, config.width, config.height );
                                adjust_size();
                            }
                        }

                        break;
                    }
                }

                break;
            }
            case ( SDL_TEXTINPUT ):
            {
                if ( status == JOIN )
                {
                    int len = strlen( room_id_text );
                    if ( len < 8 )
                    {
                        strcat( room_id_text, event.text.text );
                        adjust_rid();
                    }
                }
                break;
            }
            case ( SDL_KEYDOWN ):
            {
                if ( status == JOIN )
                {
                    if ( event.key.keysym.sym == SDLK_BACKSPACE )
                    {
                        int len = strlen( room_id_text );
                        if ( len )
                        {
                            room_id_text[len-1] = 0;
                            adjust_rid();
                        }
                        adjust_size();
                    }
                    if ( event.key.keysym.sym == SDLK_RETURN )
                    {
                        room_id_num = atoi( room_id_text );
                        if ( !room_id_num )
                        {
                            SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "ERRO", "Invalid Room ID", win );
                            memset( room_id_text, 0, 10 );
                            adjust_rid();
                        }
                    }
                }
                break;
            }
            
            default: break;
        }
    }


    // Drawing
    switch ( status )
    {
        case ( MENU ):
        {
            // draw start button
            render_ui();
            
            SDL_RenderCopy( ren, exit_tex, NULL, &exit_rec );


            break;
        }
        case ( SETTING ):
        {
            render_ui();

            SDL_RenderCopy( ren, res_tex, NULL, &res_rec );
            switch ( curr_res )
            {
                case ( P600 ):
                {
                    SDL_RenderCopy( ren, r600_tex, NULL, &reso_rec );
                    break;
                }
                case ( P720 ):
                {
                    SDL_RenderCopy( ren, r720_tex, NULL, &reso_rec );
                    break;
                }
                case ( P1080 ):
                {
                    SDL_RenderCopy( ren, r1080_tex, NULL, &reso_rec );
                    break;
                }
                case ( P1440 ):
                {
                    SDL_RenderCopy( ren, r1440_tex, NULL, &reso_rec );
                    break;
                }
                default: break;
            }

            break;
        }
        case ( CREATE ):
        {
            render_ui();

            break;
        }
        case ( JOIN ):
        {
            render_ui();
            
            SDL_SetRenderDrawColor( ren, 255, 255, 255, SDL_ALPHA_OPAQUE );
            SDL_RenderFillRect( ren, &irid_rec );
            SDL_SetRenderDrawColor( ren, 25, 25, 25, SDL_ALPHA_OPAQUE );
            SDL_RenderDrawRect( ren, &irid_rec );
            SDL_RenderCopy( ren, grid_tex, NULL, &grid_rec );
            SDL_RenderCopy( ren, irid_tex, NULL, &irid_temp_rec );
            SDL_RenderCopy( ren, paste_tex, NULL, &paste_rec );
            

            // text input box for room id

            break;
        }
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
            // draw temp piece
            // TODO: change texture accordingly
            if ( board.pieces[board.size].x != 0 && board.pieces[board.size].y != 0 )
            {
                SDL_RenderCopy( ren, w_tex, NULL, &pieces[ board.pieces[board.size].x ][ board.pieces[board.size].y ] );
            }

            // draw UI
            SDL_RenderCopy( ren, con_tex, NULL, &con_rec );
            

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
    DIR *dir = opendir( "assets" );
    chdir( "assets" );
    while( ( file = readdir(dir) ) != NULL )
    {
        if ( !strcmp( file->d_name, "setting.config" ) )        file_check[ CONFIG_FILE ]   = true;
        else if ( !strcmp( file->d_name, D_FONT ) )             file_check[ FONT_FILE ]     = true;
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
                case ( FONT_FILE ):
                {
                    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "ERRO", "Missing font file", NULL );
                    return 1;
                }
                case ( BLACK_FILE ):
                {
                    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "ERRO", "Missing texture file", NULL );
                    return 1;
                }
                case ( WHITE_FILE ):
                {
                    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "ERRO", "Missing texture file", NULL );
                    return 1;
                }

                default: break;
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
                    // in wasm, config file cannot be stored to local disk, use default value every times
                    #ifdef __EMSCRIPTEN__
                        config.width    = D_WIDTH;
                        config.height   = D_WIDTH * SCALE;
                    #endif  // __EMSCRIPTEN__
                    break;
                }
            }
        }
    }


    // init
    SDL_Init( SDL_INIT_EVERYTHING );
    win = SDL_CreateWindow( "GOMOKU", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
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
    SDL_SetWindowMaximumSize( win, (int) ( (float) dm.h / SCALE ), dm.h );

    // report error if display resolution is not valid
    #ifndef __EMSCRIPTEN__
        if ( dm.w < 800 || dm.h < 600 )
        {
            SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_INFORMATION, "ERRO", "Display resolution not suitable", win );
            return 1;
        }
    #endif  // __EMSCRIPTEN__


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
    w_tex   = IMG_LoadTexture( ren, "white.png" );
    b_tex   = IMG_LoadTexture( ren, "black.png" );
    assert( w_tex && b_tex );

    // set the position of where each pieces should be at
    p_size = ( config.height / LINE ) / 3 * 2;
    for ( int i = 0; i < LINE ; i++ )
    {
        for ( int j = 0; j < LINE; j++ )
        {
            pieces[i][j] = ( SDL_Rect )
            {
                .h = p_size,
                .w = p_size,
                .x = config.height / LINE * i - p_size / 2,
                .y = config.height / LINE * j - p_size / 2,
            };
        }
    }

    // load font
    TTF_Init();
    b_size = config.height / 25;
    h_size = b_size * 2;
    h_font = TTF_OpenFont( D_FONT, h_size );
    b_font = TTF_OpenFont( D_FONT, b_size );

    // create surface from font
    menu_sur    = TTF_RenderUTF8_Blended( h_font, "MENU", F_COLOR );
    con_sur     = TTF_RenderUTF8_Blended( h_font, "CONFIRM", F_COLOR );
    setting_sur = TTF_RenderUTF8_Blended( h_font, "SETTING", F_COLOR );
    create_sur  = TTF_RenderUTF8_Blended( h_font, "CREATE", F_COLOR );
    join_sur    = TTF_RenderUTF8_Blended( h_font, "JOIN" , F_COLOR );
    exit_sur    = TTF_RenderUTF8_Blended( h_font, "EXIT" , F_COLOR );
    res_sur     = TTF_RenderUTF8_Blended( h_font, "RESOLUTION: " , F_COLOR );
    r600_sur    = TTF_RenderUTF8_Blended( h_font, "800 X 600" , F_COLOR );
    r720_sur    = TTF_RenderUTF8_Blended( h_font, "960 X 720" , F_COLOR );
    r1080_sur   = TTF_RenderUTF8_Blended( h_font, "1440X1080" , F_COLOR );
    r1440_sur   = TTF_RenderUTF8_Blended( h_font, "1920X1440" , F_COLOR );
    grid_sur    = TTF_RenderUTF8_Blended( h_font, "Please Enter Room ID: ", F_COLOR );
    copy_sur    = TTF_RenderUTF8_Blended( h_font, "copy", F_COLOR );
    paste_sur   = TTF_RenderUTF8_Blended( h_font, "paste", F_COLOR );
    


    // create texture from surface
    menu_tex    = SDL_CreateTextureFromSurface( ren, menu_sur );
    con_tex     = SDL_CreateTextureFromSurface( ren, con_sur );
    setting_tex = SDL_CreateTextureFromSurface( ren, setting_sur );
    create_tex  = SDL_CreateTextureFromSurface( ren, create_sur );
    join_tex    = SDL_CreateTextureFromSurface( ren, join_sur );
    exit_tex    = SDL_CreateTextureFromSurface( ren, exit_sur );
    res_tex     = SDL_CreateTextureFromSurface( ren, res_sur );
    r600_tex    = SDL_CreateTextureFromSurface( ren, r600_sur );
    r720_tex    = SDL_CreateTextureFromSurface( ren, r720_sur );
    r1080_tex   = SDL_CreateTextureFromSurface( ren, r1080_sur );
    r1440_tex   = SDL_CreateTextureFromSurface( ren, r1440_sur );
    grid_tex    = SDL_CreateTextureFromSurface( ren, grid_sur );
    copy_tex    = SDL_CreateTextureFromSurface( ren, copy_sur );
    paste_tex   = SDL_CreateTextureFromSurface( ren, paste_sur );


    // destroy surface after texture is created
    SDL_FreeSurface( menu_sur );
    SDL_FreeSurface( con_sur );
    SDL_FreeSurface( setting_sur );
    SDL_FreeSurface( create_sur );
    SDL_FreeSurface( join_sur );
    SDL_FreeSurface( exit_sur );
    SDL_FreeSurface( res_sur );
    SDL_FreeSurface( r600_sur );
    SDL_FreeSurface( r720_sur );
    SDL_FreeSurface( r1080_sur );
    SDL_FreeSurface( r1440_sur );
    SDL_FreeSurface( grid_sur );
    SDL_FreeSurface( copy_sur );
    SDL_FreeSurface( paste_sur );


    // calculate rect position
    adjust_size();



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

    // clean up
    clean_up();

    return 0;
}
