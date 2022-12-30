#ifndef __HELPER_H__
#define __HELPER_H__

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

void SDL_RenderFillCircle( SDL_Renderer *renderer, int x, int y, int radius );
void SDL_RenderDrawCircle(SDL_Renderer * renderer, int32_t centreX, int32_t centreY, int32_t radius);




#endif  // __HELPER_H__
