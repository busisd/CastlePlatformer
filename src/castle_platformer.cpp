#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>
#include <SDL.h>
#include <SDL_image.h>
#include <chrono>
#include <list>
#include <algorithm>
#include <cmath>
#include "util.h"
#include "player.h"
#include <json.hpp>

// TODO:
// * Pull in stage data from XML file
// * Draw stage rects with texture
// * Use area grouping to only calculate collision with nearby rects
//   * Before running the game: For every 50x50 sector in the stage, do a Collision check (e.g. check the entire grid of the stage)
//   * Once the game starts, use that data to only compare collisions for sectors the player is actually in
// * Bound camera so that it can't show off-stage stuff at all (instead of just the center being bounded)
// * Main menu, pause menu
// * Custom repeated textures

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

util::SizedTexture LoadSizedTexture(std::string path, SDL_Renderer *renderer)
{
    util::SizedTexture sized_texture;
    sized_texture.texture = LoadTexture(path, renderer);
    SDL_QueryTexture(sized_texture.texture, NULL, NULL, &(sized_texture.w), &(sized_texture.h));
    return sized_texture;
}

// TODO: memoize results?
int RenderRepeatedTexture(SDL_Renderer *renderer, util::SizedTexture sized_texture, int src_w, int src_h, SDL_Rect dstrect)
{
    int status_code = 0;
    SDL_Rect src_rect = {x : 0, y : 0, w : 0, h : 0};
    SDL_Rect dest_rect;
    for (int dest_x = dstrect.x; dest_x < dstrect.x + dstrect.w; dest_x += src_w)
    {
        for (int dest_y = dstrect.y; dest_y < dstrect.y + dstrect.h; dest_y += src_h)
        {
            // TODO: move to helper function
            if (src_w <= (dstrect.x + dstrect.w) - dest_x)
            {
                src_rect.w = sized_texture.w;
            }
            else
            {
                double pct_scale_w = ((double)((dstrect.x + dstrect.w) - dest_x)) / src_w;
                src_rect.w = std::round(sized_texture.w * pct_scale_w);
            }
            if (src_h <= (dstrect.y + dstrect.h) - dest_y)
            {
                src_rect.h = sized_texture.h;
            }
            else
            {
                double pct_scale_h = ((double)((dstrect.y + dstrect.h) - dest_y)) / src_h;
                src_rect.h = std::round(sized_texture.h * pct_scale_h);
            }
            dest_rect = {
                x : dest_x,
                y : dest_y,
                w : std::min(src_w, (dstrect.x + dstrect.w) - dest_x),
                h : std::min(src_h, (dstrect.y + dstrect.h) - dest_y)
            };

            status_code = SDL_RenderCopy(renderer, sized_texture.texture, &src_rect, &dest_rect);
            if (status_code != 0)
                return status_code;
        }
    }
    return status_code;
}

/**
 * Notes on position:
 * Camera Position will be a single point that the camera will try to focus on,
 * within bounds.
 * It's based on Player Position; camera will try to focus on a point centered on
 * the player's X, and above-center in the Y direction.
 *
 * The Camera Area is an 800 by 450 rectangle (16:9 ratio) centered at 0,0.
 * Bounds: (-400, 400) X; (-225, 225) Y
 * Objects with Game Coordinates are rendered within the Camera Area relative
 * to the Camera Position. For example: If the camera is at (30, 30) and an
 * object is at (40, 40), it would be rendered at a Camera Position of (10, 10),
 * which would be converted to its true coordinate on the screen.
 */

// SCREEN CONSTANTS (Eventually should adapt to window size)
const int SCREEN_W = 1280;
const int SCREEN_H = 720;

const int PLAYER_WIDTH = 73;
const int PLAYER_HEIGHT = 153;

const int GROUND_HEIGHT = 473;

// GAME CONSTANTS (Regardless of window size)
const int CAMERA_AREA_W = 800;
const int CAMERA_AREA_H = 450;

const double STAGE_BOUND_LEFT = -800.0;
const double STAGE_BOUND_RIGHT = 800.0;
const double STAGE_BOUND_BOTTOM = -325.0;
const double STAGE_BOUND_TOP = 400.0;

const double CAMERA_BOUND_LEFT = STAGE_BOUND_LEFT + CAMERA_AREA_W / 2;
const double CAMERA_BOUND_RIGHT = STAGE_BOUND_RIGHT - CAMERA_AREA_W / 2;
const double CAMERA_BOUND_BOTTOM = STAGE_BOUND_BOTTOM + CAMERA_AREA_H / 2;
const double CAMERA_BOUND_TOP = STAGE_BOUND_TOP - CAMERA_AREA_H / 2;

const int PLAYER_HITBOX_WIDTH = 46;

// Conversion from Game to Screen position
// TODO: Game should maintain ratio while displaying as large as possible
double GAME_TO_SCREEN_MULTIPLIER = (double)SCREEN_W / CAMERA_AREA_W;

int TransformGameXToWindowX(double game_x)
{
    return std::round((game_x * GAME_TO_SCREEN_MULTIPLIER) + (SCREEN_W / 2));
}

int TransformGameYToWindowY(double game_y)
{
    return std::round((-game_y) * GAME_TO_SCREEN_MULTIPLIER) + (SCREEN_H / 2);
}

void DrawAtPosition(util::Rect rect, util::Point cameraCenter, SDL_Rect &drawRect)
{
    // TODO: Don't recalculate height/width every time?
    drawRect.w = rect.w * GAME_TO_SCREEN_MULTIPLIER;
    drawRect.x = TransformGameXToWindowX(rect.x - cameraCenter.x);
    drawRect.h = rect.h * GAME_TO_SCREEN_MULTIPLIER;
    drawRect.y = TransformGameYToWindowY(rect.y - cameraCenter.y) - drawRect.h;
}

void PrintDrawRect(SDL_Rect rect)
{
    std::cout << "x: " << rect.x << " y: " << rect.y << " w: " << rect.w << " h: " << rect.h << "\n";
}

struct DrawableTerrain
{
    SDL_Rect drawRect;
    util::Rect rect;
};

int main(int argc, char **argv)
{
    std::string exe_path = argv[0];
    std::string build_dir_path = exe_path.substr(0, exe_path.find_last_of("\\"));
    std::string project_dir_path = build_dir_path.substr(0, build_dir_path.find_last_of("\\"));

    SDL_Window *window = SDL_CreateWindow("Castle Platformer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    util::SizedTexture king_texture = LoadSizedTexture(project_dir_path + "/assets/king.png", renderer);
    util::SizedTexture bg_texture = LoadSizedTexture(project_dir_path + "/assets/castlebg.png", renderer);
    util::SizedTexture clouds_texture = LoadSizedTexture(project_dir_path + "/assets/clouds.png", renderer);
    util::SizedTexture moon_texture = LoadSizedTexture(project_dir_path + "/assets/moon.png", renderer);

    std::ifstream f(project_dir_path + "/data/stage1.json");
    nlohmann::json stageData = nlohmann::json::parse(f);
    // std::cout << stageData << std::endl;

    Player player;
    util::Point cameraCenter;

    SDL_Rect player_rect = {x : 0, y : GROUND_HEIGHT - PLAYER_HEIGHT, w : (int)(PLAYER_HITBOX_WIDTH * GAME_TO_SCREEN_MULTIPLIER), h : PLAYER_HEIGHT};
    bool facing_left = false;

    double player_offset_x = (SCREEN_W / 2 - (PLAYER_WIDTH / 2));
    SDL_Rect bg_rect_left = {x : -SCREEN_W, y : 0, w : SCREEN_W, h : SCREEN_H};
    SDL_Rect bg_rect_right = {x : 0, y : 0, w : SCREEN_W, h : SCREEN_H};

    std::list<DrawableTerrain> drawableTerrains;
    for (auto terrainPiece : stageData["terrain"])
    {
        drawableTerrains.push_back({{}, {x : terrainPiece["l"],
                                         y : terrainPiece["b"],
                                         w : (double)terrainPiece["r"] - (double)terrainPiece["l"],
                                         h : (double)terrainPiece["t"] - (double)terrainPiece["b"]}});
    }

    double cloud_x = 0.0;
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
                    player.yVelocity = 13.0 + 0.6;
                    player.isGrounded = false;
                }
                // player.position.y += 1;
            }
            if (util::Contains(keys_pressed, SDLK_DOWN))
            {
                // player.position.y -= 1;
            }
            if (util::Contains(keys_pressed, SDLK_RIGHT))
            {
                player.rect.x += 3.5;
                for (DrawableTerrain &drawableTerrain : drawableTerrains)
                {
                    if (util::Collides(player.rect, drawableTerrain.rect))
                    {
                        player.rect.x = drawableTerrain.rect.x - player.rect.w;
                        break;
                    }
                }
                facing_left = false;
            }
            if (util::Contains(keys_pressed, SDLK_LEFT))
            {
                player.rect.x -= 3.5;
                for (DrawableTerrain &drawableTerrain : drawableTerrains)
                {
                    if (util::Collides(player.rect, drawableTerrain.rect))
                    {
                        player.rect.x = drawableTerrain.rect.x + drawableTerrain.rect.w;
                        break;
                    }
                }
                facing_left = true;
            }
            if (util::Contains(keys_pressed, SDLK_p))
            {
                util::prettyLog("x:", player.rect.x, "y:", player.rect.y);
            }

            player.yVelocity -= 0.6;
            player.yVelocity = std::max(player.yVelocity, player.maxFallSpeed);
            player.rect.y += player.yVelocity;
            player.isGrounded = false;
            for (DrawableTerrain &drawableTerrain : drawableTerrains)
            {
                if (util::Collides(player.rect, drawableTerrain.rect))
                {
                    if (player.yVelocity < 0) {
                        player.rect.y = drawableTerrain.rect.y + drawableTerrain.rect.h;
                        player.yVelocity = 0;
                        player.isGrounded = true;
                        break;
                    } else {
                        player.rect.y = drawableTerrain.rect.y - player.rect.h;
                        player.yVelocity = 0;
                        break;
                    }
                }
            }

            cameraCenter.x = std::clamp(player.rect.x + (player.rect.w / 2.0), CAMERA_BOUND_LEFT, CAMERA_BOUND_RIGHT);
            cameraCenter.y = std::clamp(player.rect.y + 80, CAMERA_BOUND_BOTTOM, CAMERA_BOUND_TOP);

            DrawAtPosition(player.rect, cameraCenter, player_rect);
            for (DrawableTerrain &drawableTerrain : drawableTerrains)
            {
                DrawAtPosition(drawableTerrain.rect, cameraCenter, drawableTerrain.drawRect);
            }
            // DrawAtPosition(greenbox1Rect, cameraCenter, greenbox1);
            // DrawAtPosition(greenbox2Rect, cameraCenter, greenbox2);
            // DrawAtPosition(greenbox3Rect, cameraCenter, greenbox3);
            // DrawAtPosition(fgRect, cameraCenter, fg_rect);

            cloud_x -= .35;
            if (cloud_x < 0)
                cloud_x += SCREEN_W;
            cloud_rect_left.x = cloud_x - SCREEN_W;
            cloud_rect_right.x = cloud_x;

            bg_rect_right.x = (int)(-0.22 * cameraCenter.x) % SCREEN_W + player_offset_x;
            if (bg_rect_right.x < 0)
                bg_rect_right.x += SCREEN_W;
            bg_rect_left.x = bg_rect_right.x - SCREEN_W;

            // Render
            SDL_RenderClear(renderer);
            SDL_SetRenderDrawColor(renderer, 90, 90, 115, 255);

            SDL_RenderCopy(renderer, bg_texture.texture, NULL, &bg_rect_left);
            SDL_RenderCopy(renderer, bg_texture.texture, NULL, &bg_rect_right);

            SDL_RenderCopy(renderer, moon_texture.texture, NULL, NULL);

            SDL_RenderCopy(renderer, clouds_texture.texture, NULL, &cloud_rect_left);
            SDL_RenderCopy(renderer, clouds_texture.texture, NULL, &cloud_rect_right);

            for (DrawableTerrain &drawableTerrain : drawableTerrains)
            {
                SDL_RenderFillRect(renderer, &(drawableTerrain.drawRect));
            }

            RenderRepeatedTexture(renderer, king_texture, king_texture.w*4, king_texture.h*4, drawableTerrains.back().drawRect);

            SDL_RenderCopyEx(renderer, king_texture.texture, NULL, &player_rect, 0, NULL, facing_left ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);

            SDL_RenderPresent(renderer);
        }
    }

    DestroyTextures();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
