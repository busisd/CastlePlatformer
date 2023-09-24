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
#include <nlohmann/json.hpp>

using namespace util;

// TODO:
// * Use area grouping to only calculate collision with nearby rects
//   * Before running the game: For every 50x50 sector in the stage, do a Collision check (e.g. check the entire grid of the stage)
//   * Once the game starts, use that data to only compare collisions for sectors the player is actually in
// * Jumping/landing animations
// * Hold UP for longer jumps
// * Double jumps
// * Draw polygons/slopes
// * Walk up slopes with snap-up
// * Option to set controls
// * Save user options
// * Reorganize game into its own class or function
// * Change menu boolean to enum of views (menu, settings, game)
// * Get stage bounds from the stage json
// * Add player starting position to stage json
// * Change the brick texture to be less mossy
// * Support >60 fps
// * Center the screen when the aspect ratio is not perfect
// * Make repeated textures the same regardless of window size
// * Make menu buttons the same regardless of window size
// * Make pixel art always correspond to game size
//   * Or some fraction of game size that can be scaled up in various ways
// * Add setting to "snap" game resolution to the biggest perfect multiple
// * Consider not letter-boxing the screen
//   * Logic to show extra; OR
//   * Reverse logic for bounding the screen so it always shows less
// * Move Texture into Drawable
// * Move rendering into a class so that camera position and renderer don't need to be passed in

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

SizedTexture LoadSizedTexture(std::string path, SDL_Renderer *renderer)
{
    SizedTexture sized_texture;
    sized_texture.sdl_texture = LoadTexture(path, renderer);
    SDL_QueryTexture(sized_texture.sdl_texture, NULL, NULL, &(sized_texture.w), &(sized_texture.h));
    return sized_texture;
}

// TODO: memoize results?
int RenderRepeatedTexture(SDL_Renderer *renderer, SizedTexture sized_texture, int src_w, int src_h, SDL_Rect dstrect)
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

            status_code = SDL_RenderCopy(renderer, sized_texture.sdl_texture, &src_rect, &dest_rect);
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
 * The Camera Area is an 1280 by 720 rectangle (16:9 ratio) centered at 0,0.
 * Bounds: (-640, 640) X; (-360, 360) Y
 * Objects with Game Coordinates are rendered within the Camera Area relative
 * to the Camera Position. For example: If the camera is at (30, 30) and an
 * object is at (40, 40), it would be rendered at a Camera Position of (10, 10),
 * which would be converted to its true coordinate on the screen.
 */

// SCREEN CONSTANTS
int ACTUAL_SCREEN_W = 1280;
int ACTUAL_SCREEN_H = 720;
int SCREEN_W = 1280;
int SCREEN_H = 720;
int SCREEN_X_PADDING = 0;
int SCREEN_Y_PADDING = 0;

const int SCREEN_RATIO_W = 16;
const int SCREEN_RATIO_H = 9;

const int PLAYER_WIDTH = 73;
const int PLAYER_HEIGHT = 153;

// GAME CONSTANTS (Regardless of window size)
// Game box area is the
const int GAME_BOX_W = 1280;
const int GAME_BOX_H = 720;

const int GAME_BOX_BOUND_LEFT = -(GAME_BOX_W / 2);
const int GAME_BOX_BOUND_RIGHT = GAME_BOX_W / 2;
const int GAME_BOX_BOUND_TOP = GAME_BOX_H / 2;
const int GAME_BOX_BOUND_BOTTOM = -(GAME_BOX_H / 2);

// CAMERA CONSTANTS
const int CAMERA_CENTER_VERTICAL_OFFSET = 120;

const double STAGE_BOUND_LEFT = -1000.0;
const double STAGE_BOUND_RIGHT = 1000.0;
const double STAGE_BOUND_BOTTOM = -425.0;
const double STAGE_BOUND_TOP = 600.0;

const double CAMERA_BOUND_LEFT = STAGE_BOUND_LEFT + GAME_BOX_W / 2;
const double CAMERA_BOUND_RIGHT = STAGE_BOUND_RIGHT - GAME_BOX_W / 2;
const double CAMERA_BOUND_BOTTOM = STAGE_BOUND_BOTTOM + GAME_BOX_H / 2;
const double CAMERA_BOUND_TOP = STAGE_BOUND_TOP - GAME_BOX_H / 2;

// Conversion from Game to Screen position
double GAME_TO_SCREEN_MULTIPLIER = (double)SCREEN_W / GAME_BOX_W;

// Other
const double BG_SCROLL_SPEED = -0.22;

int TransformGameXToWindowX(double game_x)
{
    return std::round((game_x * GAME_TO_SCREEN_MULTIPLIER) + (SCREEN_W / 2));
}

int TransformGameYToWindowY(double game_y)
{
    return std::round((-game_y) * GAME_TO_SCREEN_MULTIPLIER) + (SCREEN_H / 2);
}

void DrawAtPosition(Point camera_center, Drawable &drawable)
{
    // TODO: Possible optimization, don't recalculate height/width every time
    // (Cache it for each sprite until window size changes)
    drawable.draw_rect.w = drawable.game_rect.w * GAME_TO_SCREEN_MULTIPLIER;
    drawable.draw_rect.x = TransformGameXToWindowX(drawable.game_rect.x - camera_center.x);
    drawable.draw_rect.h = drawable.game_rect.h * GAME_TO_SCREEN_MULTIPLIER;
    drawable.draw_rect.y = TransformGameYToWindowY(drawable.game_rect.y - camera_center.y) - (drawable.game_rect.h * GAME_TO_SCREEN_MULTIPLIER);
}

/**
 * Ignores camera position
 */
void DrawAtPositionStatic(Drawable &drawable)
{
    drawable.draw_rect.w = drawable.game_rect.w * GAME_TO_SCREEN_MULTIPLIER;
    drawable.draw_rect.x = TransformGameXToWindowX(drawable.game_rect.x);
    drawable.draw_rect.h = drawable.game_rect.h * GAME_TO_SCREEN_MULTIPLIER;
    drawable.draw_rect.y = TransformGameYToWindowY(drawable.game_rect.y) - (drawable.game_rect.h * GAME_TO_SCREEN_MULTIPLIER);
}

void RenderAtPosition(SDL_Renderer *renderer, Point camera_center, Drawable &drawable)
{
    DrawAtPosition(camera_center, drawable);
    if (drawable.is_repeating_texture)
    {
        // TODO: Remove hardcoded *4
        RenderRepeatedTexture(renderer, drawable.texture, drawable.texture.w * 4, drawable.texture.h * 4, drawable.draw_rect);
    }
    else
    {
        SDL_RenderCopyEx(renderer, drawable.texture.sdl_texture, NULL, &drawable.draw_rect, 0, NULL, drawable.flip);
    }
}

void RenderAtPositionStatic(SDL_Renderer *renderer, Drawable &drawable)
{
    DrawAtPositionStatic(drawable);
    SDL_RenderCopy(renderer, drawable.texture.sdl_texture, NULL, &drawable.draw_rect);
}

void RenderFullScreenRect(SDL_Renderer *renderer) {
    SDL_Rect full_screen_rect = {
        x: 0,
        y: 0,
        w: SCREEN_W,
        h: SCREEN_H
    };
    SDL_RenderFillRect(renderer, &full_screen_rect);
}

int main(int argc, char **argv)
{
    std::string exe_path = argv[0];
    std::string build_dir_path = exe_path.substr(0, exe_path.find_last_of("\\"));
    std::string project_dir_path = build_dir_path.substr(0, build_dir_path.find_last_of("\\"));

    SDL_Window *window = SDL_CreateWindow("Castle Platformer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SizedTexture text_start = LoadSizedTexture(project_dir_path + "/assets/text_start.png", renderer);
    SizedTexture text_settings = LoadSizedTexture(project_dir_path + "/assets/text_settings.png", renderer);
    SizedTexture text_exit = LoadSizedTexture(project_dir_path + "/assets/text_exit.png", renderer);
    SizedTexture button_selected = LoadSizedTexture(project_dir_path + "/assets/button_selected.png", renderer);
    SizedTexture button_unselected = LoadSizedTexture(project_dir_path + "/assets/button_unselected.png", renderer);

    SizedTexture king_texture = LoadSizedTexture(project_dir_path + "/assets/king.png", renderer);
    SizedTexture bg_texture = LoadSizedTexture(project_dir_path + "/assets/castlebg.png", renderer);
    SizedTexture clouds_texture = LoadSizedTexture(project_dir_path + "/assets/clouds.png", renderer);
    SizedTexture moon_texture = LoadSizedTexture(project_dir_path + "/assets/moon.png", renderer);
    SizedTexture brick_texture = LoadSizedTexture(project_dir_path + "/assets/bricktexture.png", renderer);
    SizedTexture paused_texture = LoadSizedTexture(project_dir_path + "/assets/paused.png", renderer);

    std::ifstream f(project_dir_path + "/data/stage1.json");
    nlohmann::json stageData = nlohmann::json::parse(f);

    Player player;
    player.rects.texture = king_texture;
    Point camera_center;

    bool menu = true;
    int menu_hovered_index = 0;
    const std::vector<SizedTexture> menu_buttons = {text_start, text_settings, text_exit};
    std::vector<Drawable> menu_buttons_text = {
        {game_rect : {x : -200, y : -48, w : 400, h : 96}, texture : text_start},
        {game_rect : {x : -200, y : -193, w : 400, h : 96}, texture : text_settings},
        {game_rect : {x : -200, y : -338, w : 400, h : 96}, texture : text_exit},
    };
    Drawable menu_buttons_bg = {};
    const int MENU_START_INDEX = 0;
    const int MENU_EXIT_INDEX = 2;

    std::list<Drawable> drawable_terrains;
    for (auto terrainPiece : stageData["terrain"])
    {
        drawable_terrains.push_back({
            game_rect : {x : terrainPiece["l"],
                         y : terrainPiece["b"],
                         w : (double)terrainPiece["r"] - (double)terrainPiece["l"] + 1,
                         h : (double)terrainPiece["t"] - (double)terrainPiece["b"] + 1},
            texture : brick_texture,
            is_repeating_texture : true
        });
    }

    Drawable bg_left = {{x : -GAME_BOX_W, y : GAME_BOX_BOUND_BOTTOM, w : GAME_BOX_W, h : GAME_BOX_H}, texture : bg_texture};
    Drawable bg_right = {{x : 0, y : GAME_BOX_BOUND_BOTTOM, w : GAME_BOX_W, h : GAME_BOX_H}, texture : bg_texture};

    bool paused = false;
    Drawable paused_text = {{x : -188, y : -32, w : 376, h : 64}, texture : paused_texture};

    double cloud_x = 0.0;
    Drawable clouds_left = {{x : -GAME_BOX_W, y : GAME_BOX_BOUND_BOTTOM, w : GAME_BOX_W, h : GAME_BOX_H}, texture : clouds_texture};
    Drawable clouds_right = {{x : 0, y : GAME_BOX_BOUND_BOTTOM, w : GAME_BOX_W, h : GAME_BOX_H}, texture : clouds_texture};

    Drawable moon = {{x : 480, y : 253, w : 67, h : 67}, texture : moon_texture};

    SDL_Rect screen_bar_right = {};
    SDL_Rect screen_bar_bottom = {};

    bool isRunning = true;
    SDL_Event event;

    int keyboard_size;
    const Uint8 *keyboard_state = SDL_GetKeyboardState(&keyboard_size);
    Uint8 newly_pressed_keys[keyboard_size];
    std::fill(newly_pressed_keys, newly_pressed_keys + keyboard_size, 0);

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
                if (event.key.repeat == 0)
                {
                    newly_pressed_keys[event.key.keysym.scancode] = 1;
                }
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    ACTUAL_SCREEN_W = event.window.data1;
                    ACTUAL_SCREEN_H = event.window.data2;
                    if (ACTUAL_SCREEN_W * SCREEN_RATIO_H >= ACTUAL_SCREEN_H * SCREEN_RATIO_W)
                    { // Display area is height-bounded
                        SCREEN_H = ACTUAL_SCREEN_H;
                        SCREEN_W = ACTUAL_SCREEN_H * SCREEN_RATIO_W / SCREEN_RATIO_H;
                        screen_bar_right = {x : SCREEN_W, y : 0, w : ACTUAL_SCREEN_W - SCREEN_W, h : SCREEN_H};
                        screen_bar_bottom = {};
                    }
                    else
                    { // Display area is width-bounded
                        SCREEN_W = ACTUAL_SCREEN_W;
                        SCREEN_H = ACTUAL_SCREEN_W * SCREEN_RATIO_H / SCREEN_RATIO_W;
                        screen_bar_right = {};
                        screen_bar_bottom = {x : 0, y : SCREEN_H, w : SCREEN_W, h : ACTUAL_SCREEN_H - SCREEN_H};
                    }
                    SCREEN_X_PADDING = SCREEN_W - ACTUAL_SCREEN_W;
                    SCREEN_Y_PADDING = SCREEN_H - ACTUAL_SCREEN_H;
                    // prettyLog("New window size", SCREEN_W, SCREEN_H);

                    // Recalculate anything depending on SCREEN_W or SCREEN_H
                    GAME_TO_SCREEN_MULTIPLIER = (double)SCREEN_W / GAME_BOX_W;
                }
            }
        }
        if (!isRunning)
        {
            break;
        }

        while (std::chrono::steady_clock::now() > current_time + frame_length)
        {
            frame_number++;
            current_time += frame_length;

            if (menu)
            {
                // TODO: Should only need to call RenderClear in one location
                SDL_RenderClear(renderer);
                bg_right.game_rect.x = GAME_BOX_BOUND_LEFT;
                RenderAtPositionStatic(renderer, bg_right);

                if (newly_pressed_keys[SDL_SCANCODE_ESCAPE])
                {
                    SDL_Event quit_event = {type : SDL_QUIT};
                    SDL_PushEvent(&quit_event);
                }

                if (newly_pressed_keys[SDL_SCANCODE_DOWN])
                {
                    menu_hovered_index++;
                    menu_hovered_index = PositiveModulo(menu_hovered_index, menu_buttons.size());
                }
                if (newly_pressed_keys[SDL_SCANCODE_UP])
                {
                    menu_hovered_index--;
                    menu_hovered_index = PositiveModulo(menu_hovered_index, menu_buttons.size());
                }

                if (newly_pressed_keys[SDL_SCANCODE_Z] || newly_pressed_keys[SDL_SCANCODE_SPACE] || newly_pressed_keys[SDL_SCANCODE_RETURN])
                {
                    if (menu_hovered_index == MENU_START_INDEX)
                    {
                        menu = false;
                    }
                    else if (menu_hovered_index == MENU_EXIT_INDEX)
                    {
                        SDL_Event quit_event = {type : SDL_QUIT};
                        SDL_PushEvent(&quit_event);
                    }
                }

                for (int current_button_index = 0; current_button_index < menu_buttons.size(); current_button_index++)
                {
                    Drawable current_button = menu_buttons_text.at(current_button_index);
                    menu_buttons_bg.game_rect = current_button.game_rect;
                    menu_buttons_bg.texture = (current_button_index == menu_hovered_index) ? button_selected : button_unselected;
                    RenderAtPositionStatic(renderer, menu_buttons_bg);
                    RenderAtPositionStatic(renderer, current_button);
                }
            }
            else
            {
                if (newly_pressed_keys[SDL_SCANCODE_ESCAPE])
                {
                    menu = true;
                }
                if (newly_pressed_keys[SDL_SCANCODE_P])
                {
                    paused = !paused;
                }
                if (!paused)
                {
                    if (keyboard_state[SDL_SCANCODE_UP])
                    {
                        if (player.is_grounded)
                        {
                            player.y_velocity = 13.0 + 0.6;
                            player.is_grounded = false;
                        }
                    }
                    if (keyboard_state[SDL_SCANCODE_DOWN])
                    {
                    }
                    if (keyboard_state[SDL_SCANCODE_RIGHT])
                    {
                        player.rects.game_rect.x += 3.5;
                        for (Drawable &drawableTerrain : drawable_terrains)
                        {
                            if (Collides(player.rects.game_rect, drawableTerrain.game_rect))
                            {
                                player.rects.game_rect.x = drawableTerrain.game_rect.x - player.rects.game_rect.w;
                                break;
                            }
                        }
                        player.rects.flip = SDL_FLIP_NONE;
                    }
                    if (keyboard_state[SDL_SCANCODE_LEFT])
                    {
                        player.rects.game_rect.x -= 3.5;
                        for (Drawable &drawableTerrain : drawable_terrains)
                        {
                            if (Collides(player.rects.game_rect, drawableTerrain.game_rect))
                            {
                                player.rects.game_rect.x = drawableTerrain.game_rect.x + drawableTerrain.game_rect.w;
                                break;
                            }
                        }
                        player.rects.flip = SDL_FLIP_HORIZONTAL;
                    }
                    if (keyboard_state[SDL_SCANCODE_I])
                    {
                        prettyLog("x:", player.rects.game_rect.x, "y:", player.rects.game_rect.y);
                    }

                    player.y_velocity -= 0.6;
                    player.y_velocity = std::max(player.y_velocity, player.max_fall_speed);
                    player.rects.game_rect.y += player.y_velocity;
                    player.is_grounded = false;
                    for (Drawable &drawableTerrain : drawable_terrains)
                    {
                        if (Collides(player.rects.game_rect, drawableTerrain.game_rect))
                        {
                            if (player.y_velocity < 0)
                            {
                                player.rects.game_rect.y = drawableTerrain.game_rect.y + drawableTerrain.game_rect.h;
                                player.y_velocity = 0;
                                player.is_grounded = true;
                                break;
                            }
                            else
                            {
                                player.rects.game_rect.y = drawableTerrain.game_rect.y - player.rects.game_rect.h;
                                player.y_velocity = 0;
                                break;
                            }
                        }
                    }

                    cloud_x -= .35;
                    if (cloud_x < GAME_BOX_BOUND_LEFT)
                    {
                        cloud_x += GAME_BOX_W;
                    }
                    clouds_left.game_rect.x = cloud_x - GAME_BOX_W;
                    clouds_right.game_rect.x = cloud_x;
                }

                camera_center.x = std::clamp(player.rects.game_rect.x + (player.rects.game_rect.w / 2.0), CAMERA_BOUND_LEFT, CAMERA_BOUND_RIGHT);
                camera_center.y = std::clamp(player.rects.game_rect.y + CAMERA_CENTER_VERTICAL_OFFSET, CAMERA_BOUND_BOTTOM, CAMERA_BOUND_TOP);

                int bg_position = PositiveModulo((int)(BG_SCROLL_SPEED * camera_center.x) + (GAME_BOX_W / 2), GAME_BOX_W) - (GAME_BOX_W / 2);
                bg_right.game_rect.x = bg_position;
                bg_left.game_rect.x = bg_position - GAME_BOX_W;

                // Render
                SDL_RenderClear(renderer);

                RenderAtPositionStatic(renderer, bg_left);
                RenderAtPositionStatic(renderer, bg_right);

                RenderAtPositionStatic(renderer, moon);

                RenderAtPositionStatic(renderer, clouds_left);
                RenderAtPositionStatic(renderer, clouds_right);

                for (Drawable &drawableTerrain : drawable_terrains)
                {
                    RenderAtPosition(renderer, camera_center, drawableTerrain);
                }

                RenderAtPosition(renderer, camera_center, player.rects);

                if (paused)
                {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 120);
                    RenderFullScreenRect(renderer);
                    RenderAtPositionStatic(renderer, paused_text);
                }

                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderFillRect(renderer, &screen_bar_right);
                SDL_RenderFillRect(renderer, &screen_bar_bottom);
            }

            SDL_RenderPresent(renderer);
            std::fill(newly_pressed_keys, newly_pressed_keys + keyboard_size, 0);
        }
    }

    DestroyTextures();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
