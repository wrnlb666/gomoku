#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#include "func.h"
#include "game.h"


int main(int argc, char **argv)
{
    //unused variable
    (void)argc;
    (void)argv;


    //traverse folder and apply settings
    bool *file_check = malloc(sizeof(bool) * file_max);
    memset(file_check, 0, file_max);
    struct dirent *file;
    DIR *dir = opendir("./");
    while((file = readdir(dir)) != NULL)
    {
        if(!strcmp(file->d_name, "font.ttf")) file_check[font_ttf] = true;
        else if(!strcmp(file->d_name, "setting.config")) file_check[setting_config] = true;
    }
    setting *setting_buf = NULL;
    for(int i = 0; i < file_max; i++)
    {
        if( !file_check[i] )
        {
            switch(i)
            {
                case font_ttf:
                {
                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "字体丢失", "请在文件夹内添加“font.ttf”字体文件", NULL);
                    return 1;
                }
                case setting_config:
                {
                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "游戏配置文件丢失", "游戏配置文件丢失\n将使用默认配置", NULL);
                    setting_buf = malloc(sizeof(setting_buf));
                    *setting_buf = (setting) { .window_type = window_normal };
                    write_setting(setting_buf);
                    break;
                }
            }
        }
        else
        {
            switch(i)
            {
                case font_ttf:
                {
                    break;
                }
                case setting_config:
                {
                    setting_buf = read_setting();
                    break;
                }
            }
        }
    }


    //SDL init
    scz(SDL_Init(SDL_INIT_EVERYTHING));
    SDL_DisplayMode dm = { 0 };
    scz(SDL_GetDesktopDisplayMode(0, &dm));
    SDL_Window *win = scn(SDL_CreateWindow("中国式顶顶🐍", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, dm.w / 2, dm.h / 2, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN));
    SDL_Renderer *ren = scn(SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE));
    SDL_SetWindowMaximumSize(win, dm.w, dm.h);
    SDL_SetWindowMinimumSize(win, dm.w / 4, dm.h / 4);
    SDL_info info = { .win = win, .ren = ren, .width = dm.w / 2, .height = dm.h / 2 };
    SDL_SetWindowMouseGrab(win, SDL_TRUE);

    //apply settings
    switch(setting_buf->window_type)
    {
        case window_normal:
        {
            break;
        }
        case window_full_screen_desktop:
        {
            scz(SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP));
            break;
        }
        case window_maximized:
        {
            SDL_MaximizeWindow(win);
            break;
        }
        default:
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "配置文件出错", "配置文件出错", win);
            return 1;
        }
    }

    //TTF init
    tcz(TTF_Init());
    TTF_Font *header_font = tcn(TTF_OpenFont("font.ttf", 48));
    TTF_Font *body_font = tcn(TTF_OpenFont("font.ttf", 12));


    //Game init
    SDL_Event event = { 0 };
    int should_quit = 0;
    
    game game = 
    { 
        .status = game_menu
    };

    //texture init
    //开始游戏 texture
    SDL_Surface *menu_surface = tcn(TTF_RenderUTF8_LCD(header_font, "开始游戏", (SDL_Color) { 0, 25, 50, SDL_ALPHA_OPAQUE }, (SDL_Color) { 155, 125, 200, SDL_ALPHA_OPAQUE }));
    SDL_Texture *menu_texture = scn(SDL_CreateTextureFromSurface(ren, menu_surface));
    SDL_Surface *enter_setting_surface = tcn(TTF_RenderUTF8_LCD(header_font, "设置", (SDL_Color) { 0, 25, 50, SDL_ALPHA_OPAQUE }, (SDL_Color) { 155, 125, 200, SDL_ALPHA_OPAQUE }));
    SDL_Texture *enter_setting_texture = scn(SDL_CreateTextureFromSurface(ren, enter_setting_surface));


    //设置 texture
    SDL_Surface *window_flag_setting_fullscreen_desktop_surface = tcn(TTF_RenderUTF8_LCD(body_font, "显示模式     无边框", (SDL_Color) { 0, 25, 50, SDL_ALPHA_OPAQUE }, (SDL_Color) { 155, 125, 200, SDL_ALPHA_OPAQUE }));
    SDL_Surface *window_flag_setting_normal_surface             = tcn(TTF_RenderUTF8_LCD(body_font, "显示模式     窗口化", (SDL_Color) { 0, 25, 50, SDL_ALPHA_OPAQUE }, (SDL_Color) { 155, 125, 200, SDL_ALPHA_OPAQUE }));
    SDL_Surface *window_flag_setting_maximized_surface          = tcn(TTF_RenderUTF8_LCD(body_font, "显示模式     最大化", (SDL_Color) { 0, 25, 50, SDL_ALPHA_OPAQUE }, (SDL_Color) { 155, 125, 200, SDL_ALPHA_OPAQUE }));
    SDL_Texture *window_flag_setting_fullscreen_desktop_texture = scn(SDL_CreateTextureFromSurface(ren, window_flag_setting_fullscreen_desktop_surface));
    SDL_Texture *window_flag_setting_normal_texture             = scn(SDL_CreateTextureFromSurface(ren, window_flag_setting_normal_surface));
    SDL_Texture *window_flag_setting_maximized_texture          = scn(SDL_CreateTextureFromSurface(ren, window_flag_setting_maximized_surface));



    //Game
    while(!should_quit)
    {
        //event handling
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_QUIT: 
                {
                    should_quit ++ ;
                    break;
                }
                case SDL_WINDOWEVENT_RESIZED:
                {
                    SDL_GetWindowSize(win, &info.width, &info.height);
                    break;
                }
                case SDL_MOUSEBUTTONUP:
                {
                    switch(game.status)
                    {
                        case game_menu:
                        {
                            if(event.button.button == SDL_BUTTON_LEFT)
                            {
                                if(event.button.x >= ((info.width - menu_surface->w) / 2) && event.button.x <= ((info.width + menu_surface->w) / 2) &&
                                event.button.y >= (((info.height - menu_surface->h) / 2) - (info.height / 8)) && event.button.y <= ((info.height + menu_surface->h) / 2) - (info.height / 8))
                                {
                                    game.status = game_running;
                                }
                                else if(event.button.x >= (info.width - enter_setting_surface->w) / 2 && event.button.x <= (info.width + enter_setting_surface->w) / 2 &&
                                event.button.y >= (((info.height - enter_setting_surface->h) / 2) + (info.height / 8)) && event.button.y <= (((info.height + enter_setting_surface->h) / 2) + (info.height / 8)))
                                {
                                    game.status = game_setting;
                                }
                                else
                                {
                                    continue;
                                }
                            }
                            break;
                        }
                        case game_running:
                        {
                            
                            break;
                        }
                        case game_end:
                        {
                            break;
                        }
                    }
                    break;
                }

                default: break;
            }
        }
        //init frame
        SDL_SetRenderDrawColor(ren, 255, 225, 200, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(ren);

        SDL_GetWindowSize(win, &(info.width), &(info.height));

        //main menu
        if(game.status == game_menu)
        {
            SDL_Rect main_menu_rect = {(info.width - menu_surface->w) / 2, (((info.height - menu_surface->h) / 2) - (info.height / 8)), menu_surface->w, menu_surface->h};
            SDL_RenderCopy(ren, menu_texture, NULL, &main_menu_rect);
            SDL_Rect enter_setting_rect = {(info.width - enter_setting_surface->w) / 2, (((info.height - enter_setting_surface->h) / 2) + (info.height / 8)), enter_setting_surface->w, enter_setting_surface->h};
            SDL_RenderCopy(ren, enter_setting_texture, NULL, &enter_setting_rect);
        }

        if(game.status == game_setting)
        {

        }

        //game running
        if(game.status == game_running)
        {
            //draw background
            draw_background(&info);

        }

        //game ended
        if(game.status == game_end)
        {

        }

        SDL_RenderPresent(ren);
    }



    //clean up
    free(setting_buf);


    SDL_FreeSurface(menu_surface);
    if(menu_texture) SDL_DestroyTexture(menu_texture);

    SDL_FreeSurface(window_flag_setting_fullscreen_desktop_surface);
    SDL_FreeSurface(window_flag_setting_normal_surface);
    SDL_FreeSurface(window_flag_setting_maximized_surface);
    if(window_flag_setting_fullscreen_desktop_texture) SDL_DestroyTexture(window_flag_setting_fullscreen_desktop_texture);
    if(window_flag_setting_normal_texture) SDL_DestroyTexture(window_flag_setting_normal_texture);
    if(window_flag_setting_maximized_texture) SDL_DestroyTexture(window_flag_setting_maximized_texture);


    TTF_CloseFont(header_font);
    TTF_CloseFont(body_font);
    TTF_Quit();



    if(ren) SDL_DestroyRenderer(ren);
    if(win) SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}