Get started with:
* https://code.visualstudio.com/docs/cpp/config-mingw
  * NOTE: The msys2 site now recommends installing ucrt package versions
    (instead of mingw), so that's how this project is configured.
    * `pacman -S mingw-w64-ucrt-x86_64-gcc`
  * NOTE: For debugging, gdb may need to be installed manually
    * `pacman -S mingw-w64-ucrt-x86_64-gdb`

Required libraries (managed via `pacman` in [MSYS2](https://www.msys2.org/)):
* SDL2 https://github.com/libsdl-org/SDL
  * `pacman -S mingw-w64-ucrt-x86_64-SDL2`
* SDL_image https://github.com/libsdl-org/SDL_image
  * `pacman -S mingw-w64-ucrt-x86_64-SDL2_image`
* nhlomann json https://github.com/nlohmann/json
  * `pacman -S mingw-w64-ucrt-x86_64-nlohmann-json`

Note: Run the VSCode "C/C++: g++.exe build castle_platformer" task to build the project.
You can also use "C/C++: g++.exe build and debug castle_platformer" from `launch.json`.
