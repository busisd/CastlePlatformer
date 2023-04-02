#include <iostream>
#include <string>
#include <unordered_set>
#include <SDL.h>
#include <SDL_image.h>
#include <chrono>
#include <list>
#include <algorithm>
#include "util.h"
#include "player.h"

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

/**
 * Notes on position:
 * Player Position will be a single point that the camera will try to focus on,
 * within bounds.
 * Player hitbox/sprite will be drawn around that Position.
 * Usually that means it's the center of the sprite, but might not be in case of
 * sprite changing size/ratio.
 *
 * The Game Area is an 800 by 450 rectangle (16:9 ratio) centered at 0,0.
 * Bounds: (-400, 400) X; (-225, 225) Y
 */

// SCREEN CONSTANTS (Eventually should adapt to window size)
const int SCREEN_W = 1280;
const int SCREEN_H = 720;

const int PLAYER_WIDTH = 73;
const int PLAYER_HEIGHT = 153;

const int GROUND_HEIGHT = 473;

// GAME CONSTANTS (Regardless of window size)
const int GAME_AREA_W = 800;
const int GAME_AREA_H = 450;

const float STAGE_BOUND_LEFT = -500.0f;
const float STAGE_BOUND_RIGHT = 500.0f;

const float STAGE_BOUND_BOTTOM = 0.0f;
const float STAGE_BOUND_TOP = 400.0f;

const float GAME_GROUND_HEIGHT = -100.0f;

const int PLAYER_HITBOX_WIDTH = 46;

// Conversion from Game to Screen position
// TODO: Game should maintain ratio while displaying as large as possible
float GAME_TO_SCREEN_MULTIPLIER = (float)SCREEN_W / GAME_AREA_W;

int TransformGameXToWindowX(int game_x)
{
    return (game_x * GAME_TO_SCREEN_MULTIPLIER) + (SCREEN_W / 2);
}

int TransformGameYToWindowY(int game_y)
{
    return ((-game_y) * GAME_TO_SCREEN_MULTIPLIER) + (SCREEN_H / 2);
}

void DrawAtPosition(float position_x, float camera_center_x, float width, float offset_x,
                    float position_y, float camera_center_y, float height, float offset_y,
                    SDL_Rect &draw_rect)
{
    // TODO: Don't recalculate height/width every time?
    draw_rect.w = width * GAME_TO_SCREEN_MULTIPLIER;
    draw_rect.x = TransformGameXToWindowX(position_x - camera_center_x) - offset_x;
    draw_rect.h = height * GAME_TO_SCREEN_MULTIPLIER;
    draw_rect.y = TransformGameYToWindowY(position_y - camera_center_y) - offset_y - draw_rect.h;
}

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

    Player player;

    SDL_Rect player_rect = {x : 0, y : GROUND_HEIGHT - PLAYER_HEIGHT, w : (int)(PLAYER_HITBOX_WIDTH * GAME_TO_SCREEN_MULTIPLIER), h : PLAYER_HEIGHT};
    bool facing_left = false;

    // float player_position_x = 0.0;
    // float player_position_y = 0.0;
    // float player_x = 0.0;
    // float player_y = 0.0;
    float player_offset_x = (SCREEN_W / 2 - (PLAYER_WIDTH / 2));
    SDL_Rect bg_rect_left = {x : -SCREEN_W, y : 0, w : SCREEN_W, h : SCREEN_H};
    SDL_Rect bg_rect_right = {x : 0, y : 0, w : SCREEN_W, h : SCREEN_H};

    // int fg_rect_x = SCREEN_W / 2 - (300 / 2);
    int fg_rect_x = 400;
    SDL_Rect fg_rect = {x : fg_rect_x, y : GROUND_HEIGHT, w : (int)(100 * GAME_TO_SCREEN_MULTIPLIER), h : SCREEN_H - GROUND_HEIGHT};

    SDL_Rect greenbox = {x : 0, y : 0, w : 30, h : 30};
    SDL_Rect greenbox2 = {x : 0, y : 0, w : 30, h : 30};

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
                    bool inserted = keys_pressed.insert((SDL_KeyCode)event.key.keysym.sym).second;
                    if (inserted)
                        util::PrintSet(keys_pressed);
                }
                break;

            case SDL_KEYUP:
                keys_pressed.erase((SDL_KeyCode)event.key.keysym.sym);
                util::PrintSet(keys_pressed);
                break;
            }
        }

        while (std::chrono::steady_clock::now() > current_time + frame_length)
        {
            frame_number++;

            current_time += frame_length;

            if (util::Contains(keys_pressed, SDLK_UP))
            {
                if (player.isGrounded)
                {
                    player.yAccel = 13.0f + 0.6f;
                    player.isGrounded = false;
                }
                // player.y += 1;
            }
            if (util::Contains(keys_pressed, SDLK_DOWN))
            {
                // player.y -= 1;
            }
            if (util::Contains(keys_pressed, SDLK_RIGHT))
            {
                player.x += 3.5;
                facing_left = false;
            }
            if (util::Contains(keys_pressed, SDLK_LEFT))
            {
                player.x -= 3.5;
                facing_left = true;
            }
            if (util::Contains(keys_pressed, SDLK_p))
            {
                std::cout << player.x << "\n";
            }

            if (!player.isGrounded)
            {
                player.yAccel -= 0.6f;
                player.y += player.yAccel;
                if (player.y < GAME_GROUND_HEIGHT)
                {
                    player.y = GAME_GROUND_HEIGHT;
                    player.yAccel = 0;
                    player.isGrounded = true;
                }
            }

            float camera_center_x = std::clamp(player.x, STAGE_BOUND_LEFT, STAGE_BOUND_RIGHT);
            float camera_center_y = std::clamp(player.y, STAGE_BOUND_BOTTOM, STAGE_BOUND_TOP);

            DrawAtPosition(player.x, camera_center_x, player.width, PLAYER_WIDTH / 2,
                           player.y, camera_center_y, player.height, 0, player_rect);
            DrawAtPosition(fg_rect_x, camera_center_x, player.width, 0,
                           -225, camera_center_y, 125, 0, fg_rect);
            DrawAtPosition(-50, camera_center_x, 100, 0,
                           -225, camera_center_y, 125, 0, greenbox);
            DrawAtPosition(50, camera_center_x, 100, 0,
                           -100, camera_center_y, 100, 0, greenbox2);

            // std::cout << "Camera: " << camera_center_x << ", " << camera_center_y << "    ";
            // std::cout << "Player: " << player.x << ", " << player.y << "\n";
            // std::cout << "greenbox: " << greenbox.x << ", " << greenbox.y << ", "
            //           << greenbox.x + greenbox.w << ", " << greenbox.y + greenbox.h
            //           << ", camera center y: " << camera_center_y << ", player y: " << player.y << "\n";

            cloud_x -= .35;
            if (cloud_x < 0)
                cloud_x += SCREEN_W;
            cloud_rect_left.x = cloud_x - SCREEN_W;
            cloud_rect_right.x = cloud_x;

            bg_rect_right.x = (int)(-0.22 * camera_center_x) % SCREEN_W + player_offset_x;
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
            SDL_RenderFillRect(renderer, &greenbox);
            SDL_RenderFillRect(renderer, &greenbox2);

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
