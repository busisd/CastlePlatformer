#include <iostream>
#include <string>
#include <unordered_set>
#include <SDL.h>
#include <SDL_image.h>
#include <chrono>

template <typename T>
void print_set(std::unordered_set<T> const &s)
{
    std::cout << "set{ ";
    for (const auto &elem : s)
    {
        std::cout << elem << " ";
    }
    std::cout << "}\n";
}

/**
 * https://lazyfoo.net/tutorials/SDL/07_texture_loading_and_rendering/index.php
 */
SDL_Texture *loadTexture(std::string path, SDL_Renderer *renderer)
{
    // The final texture
    SDL_Texture *newTexture = IMG_LoadTexture(renderer, path.c_str());
    if (newTexture == NULL)
    {
        printf("Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
    }

    return newTexture;
}

int main(int argc, char **argv)
{
    SDL_Window *window = SDL_CreateWindow("Hello SDL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    SDL_Texture *manTexture = loadTexture("assets/man.png", renderer);
    SDL_Rect rectangle = {x : 0, y : 0, w : 64, h : 64};

    bool isRunning = true;
    SDL_Event event;

    std::unordered_set<SDL_Keycode> keys_pressed;

    auto frame_length = std::chrono::microseconds{(int)(1.0 / 60.0 * 1000.0 * 1000.0)};

    auto current_time = std::chrono::steady_clock::now();
    int y_accel = 0;

    while (isRunning)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                isRunning = false;
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    isRunning = false;
                }
                else
                {
                    bool inserted = keys_pressed.insert(event.key.keysym.sym).second;
                    if (inserted)
                        print_set(keys_pressed);
                }
                break;

            case SDL_KEYUP:
                keys_pressed.erase(event.key.keysym.sym);
                print_set(keys_pressed);
                break;
            }
        }

        while (std::chrono::steady_clock::now() > current_time + frame_length)
        {
            current_time += frame_length;

            rectangle.y -= y_accel;
            y_accel -= 1;

            if (keys_pressed.find(SDLK_UP) != keys_pressed.end())
            {
                if(y_accel < 0) y_accel = 20;
                // rectangle.y -= 5;
            }
            if (keys_pressed.find(SDLK_DOWN) != keys_pressed.end())
            {
                // rectangle.y += 5;
            }
            if (keys_pressed.find(SDLK_RIGHT) != keys_pressed.end())
            {
                rectangle.x += 5;
            }
            if (keys_pressed.find(SDLK_LEFT) != keys_pressed.end())
            {
                rectangle.x -= 5;
            }
            if (keys_pressed.find(SDLK_SPACE) != keys_pressed.end())
            {
                rectangle.x = 0;
                rectangle.y = 0;
                y_accel = 0;
            }
        }

        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 120, 140, 70, 255);

        SDL_RenderCopy(renderer, manTexture, NULL, &rectangle);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(manTexture);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
