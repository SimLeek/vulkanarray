#include <iostream>
#define SDL_MAIN_HANDLED
#include "SDL.h" /* This one can't be found */

int main(){
    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_Quit();
    return 0;
}