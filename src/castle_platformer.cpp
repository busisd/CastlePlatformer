#include <iostream>
#include <string>
#include <unordered_set>
#include <SDL.h>
#include <SDL_image.h>
#include <chrono>
#include <list>
#include "util.h"

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
    std::string exe_path = argv[0];
    std::string build_dir_path = exe_path.substr(0, exe_path.find_last_of("\\"));
    std::string project_dir_path = build_dir_path.substr(0, build_dir_path.find_last_of("\\"));

    SDL_Window *window = SDL_CreateWindow("Castle Platformer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    SDL_Texture *king_texture = LoadTexture(project_dir_path + "/assets/king.png", renderer);
    SDL_Texture *bg_texture = LoadTexture(project_dir_path + "/assets/castlebg.png", renderer);
    SDL_Texture *clouds_texture = LoadTexture(project_dir_path + "/assets/clouds.png", renderer);
    SDL_Texture *moon_texture = LoadTexture(project_dir_path + "/assets/moon.png", renderer);

    SDL_Rect player_rect = {x : SCREEN_W / 2 - (73 / 2), y : 400, w : 73, h : 153};
    bool facing_left = false;

    float player_x = 0.0;
    float player_y = 0.0;
    SDL_Rect bg_rect_left = {x : -SCREEN_W, y : 0, w : SCREEN_W, h : SCREEN_H};
    SDL_Rect bg_rect_right = {x : 0, y : 0, w : SCREEN_W, h : SCREEN_H};

    int fg_rect_x = SCREEN_W / 2 - (300 / 2);
    SDL_Rect fg_rect = {x : fg_rect_x, y : 400 + 153, w : 300, h : SCREEN_H - 400 - 153};

    float cloud_x = 0.0;
    SDL_Rect cloud_rect_left = {x : -SCREEN_W, y : 0, w : SCREEN_W, h : SCREEN_H};
    SDL_Rect cloud_rect_right = {x : 0, y : 0, w : SCREEN_W, h : SCREEN_H};

    bool isRunning = true;
    SDL_Event event;

    std::unordered_set<SDL_KeyCode> keys_pressed;

    auto frame_length = std::chrono::microseconds{(int)(1.0 / 60.0 * 1000.0 * 1000.0)};
    auto current_time = std::chrono::steady_clock::now();

    int frame_number = 0;

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
                    bool inserted = keys_pressed.insert((SDL_KeyCode) event.key.keysym.sym).second;
                    if (inserted)
                        PrintSet(keys_pressed);
                }
                break;

            case SDL_KEYUP:
                keys_pressed.erase((SDL_KeyCode) event.key.keysym.sym);
                PrintSet(keys_pressed);
                break;
            }
        }

        while (std::chrono::steady_clock::now() > current_time + frame_length)
        {
            frame_number++;

            current_time += frame_length;

            if (Contains(keys_pressed, SDLK_UP))
            {
                player_y -= 5;
            }
            if (Contains(keys_pressed, SDLK_DOWN))
            {
                player_y += 5;
            }
            if (Contains(keys_pressed, SDLK_RIGHT))
            {
                player_x += 5;
                facing_left = false;
            }
            if (Contains(keys_pressed, SDLK_LEFT))
            {
                player_x -= 5;
                facing_left = true;
            }

            // player_rect.x = (int)player_x;
            // player_rect.y = (int)player_y;
            fg_rect.x = fg_rect_x - player_x;

            cloud_x -= .35;
            if (cloud_x < 0)
                cloud_x += SCREEN_W;
            cloud_rect_left.x = cloud_x - SCREEN_W;
            cloud_rect_right.x = cloud_x;

            bg_rect_right.x = (int)(-0.22 * player_x) % SCREEN_W;
            if (bg_rect_right.x < 0)
                bg_rect_right.x += SCREEN_W;
            bg_rect_left.x = bg_rect_right.x - SCREEN_W;

            // Render
            SDL_RenderClear(renderer);
            SDL_SetRenderDrawColor(renderer, 120, 140, 70, 255);

            SDL_RenderCopy(renderer, bg_texture, NULL, &bg_rect_left);
            SDL_RenderCopy(renderer, bg_texture, NULL, &bg_rect_right);

            SDL_RenderCopy(renderer, moon_texture, NULL, NULL);

            SDL_RenderCopy(renderer, clouds_texture, NULL, &cloud_rect_left);
            SDL_RenderCopy(renderer, clouds_texture, NULL, &cloud_rect_right);

            SDL_RenderFillRect(renderer, &fg_rect);

            SDL_RenderCopyEx(renderer, king_texture, NULL, &player_rect, 0, NULL, facing_left ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);

            SDL_RenderPresent(renderer);
        }
    }

    DestroyTextures();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
