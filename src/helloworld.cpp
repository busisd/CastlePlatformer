#include <iostream>
#include <string>
#include <unordered_set>
#include <SDL.h>
#include <SDL_image.h>
#include <chrono>
#include <list>

template <typename T>
void PrintSet(std::unordered_set<T> const &s)
{
    std::cout << "set{ ";
    for (const auto &elem : s)
    {
        std::cout << elem << " ";
    }
    std::cout << "}\n";
}

std::list<SDL_Texture *> g_textures;

SDL_Texture *LoadTexture(std::string path, SDL_Renderer *renderer)
{
    SDL_Texture *new_texture = IMG_LoadTexture(renderer, path.c_str());
    if (new_texture == NULL)
    {
        printf("Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
    }
    else
    {
        g_textures.push_back(new_texture);
    }

    return new_texture;
}

void DestroyTextures()
{
    for (SDL_Texture *texture : g_textures)
    {
        SDL_DestroyTexture(texture);
    }
    g_textures.clear();
}

const int SCREEN_W = 1280;
const int SCREEN_H = 720;

int main(int argc, char **argv)
{
    SDL_Window *window = SDL_CreateWindow("Hello SDL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    SDL_Texture *man_texture = LoadTexture("assets/man.png", renderer);
    SDL_Texture *bg_texture = LoadTexture("assets/castlebg.png", renderer);
    SDL_Texture *clouds_texture = LoadTexture("assets/clouds.png", renderer);
    SDL_Texture *moon_texture = LoadTexture("assets/moon.png", renderer);
    SDL_Rect player_rect = {x : 0, y : 0, w : 64, h : 64};
    SDL_Rect bg_rect_left = {x : -SCREEN_W, y : 0, w : SCREEN_W, h : SCREEN_H};
    SDL_Rect bg_rect_right = {x : 0, y : 0, w : SCREEN_W, h : SCREEN_H};
    SDL_Rect cloud_rect_left = {x : -SCREEN_W, y : 0, w : SCREEN_W, h : SCREEN_H};
    SDL_Rect cloud_rect_right = {x : 0, y : 0, w : SCREEN_W, h : SCREEN_H};

    bool isRunning = true;
    SDL_Event event;

    std::unordered_set<SDL_Keycode> keys_pressed;

    auto frame_length = std::chrono::microseconds{(int)(1.0 / 60.0 * 1000.0 * 1000.0)};
    auto current_time = std::chrono::steady_clock::now();

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
                        PrintSet(keys_pressed);
                }
                break;

            case SDL_KEYUP:
                keys_pressed.erase(event.key.keysym.sym);
                PrintSet(keys_pressed);
                break;
            }
        }

        while (std::chrono::steady_clock::now() > current_time + frame_length)
        {
            current_time += frame_length;

            if (keys_pressed.find(SDLK_UP) != keys_pressed.end())
            {
                player_rect.y -= 5;
            }
            if (keys_pressed.find(SDLK_DOWN) != keys_pressed.end())
            {
                player_rect.y += 5;
            }
            if (keys_pressed.find(SDLK_RIGHT) != keys_pressed.end())
            {
                player_rect.x += 5;
                bg_rect_left.x -= 3;
                bg_rect_right.x -= 3;
                if (bg_rect_left.x < -SCREEN_W)
                    bg_rect_left.x += SCREEN_W;
                if (bg_rect_right.x < 0)
                    bg_rect_right.x += SCREEN_W;
            }
            if (keys_pressed.find(SDLK_LEFT) != keys_pressed.end())
            {
                player_rect.x -= 5;
                bg_rect_left.x += 3;
                bg_rect_right.x += 3;
                if (bg_rect_left.x > 0)
                    bg_rect_left.x -= SCREEN_W;
                if (bg_rect_right.x > SCREEN_W)
                    bg_rect_right.x -= SCREEN_W;
            }
            if (keys_pressed.find(SDLK_SPACE) != keys_pressed.end())
            {
                player_rect.x = 0;
                player_rect.y = 0;
            }

            cloud_rect_left.x -= 1;
            cloud_rect_right.x -= 1;
            if (cloud_rect_left.x < -SCREEN_W)
                cloud_rect_left.x += SCREEN_W;
            if (cloud_rect_right.x < 0)
                cloud_rect_right.x += SCREEN_W;
        }

        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 120, 140, 70, 255);

        SDL_RenderCopy(renderer, bg_texture, NULL, &bg_rect_left);
        SDL_RenderCopy(renderer, bg_texture, NULL, &bg_rect_right);
        SDL_RenderCopy(renderer, moon_texture, NULL, NULL);
        SDL_RenderCopy(renderer, clouds_texture, NULL, &cloud_rect_left);
        SDL_RenderCopy(renderer, clouds_texture, NULL, &cloud_rect_right);
        // SDL_RenderCopy(renderer, man_texture, NULL, &player_rect);

        SDL_RenderPresent(renderer);
    }

    DestroyTextures();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
