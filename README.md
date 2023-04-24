Got started with:
* https://code.visualstudio.com/docs/cpp/config-mingw
  * NOTE: The msys2 site now recommends installing ucrt package versions
    (instead of mingw), so that's how this project is configured.
  * NOTE: For debugging, gdb may need to be installed manually
    * `pacman -S mingw-w64-ucrt-x86_64-gdb`

Required libraries:
* SDL2 https://github.com/libsdl-org/SDL/releases/latest
  * Add devel contents to ./libraries/
  * Add SDL2.dll contents to ./build/
* SDL_image https://github.com/libsdl-org/SDL_image/releases
  * Add devel contents to ./libraries/
  * Add SDL2_image.dll contents to ./build/
* nhlomann json https://github.com/nlohmann/json/releases
  * Extract include.zip to `libraries/nlohmann_json`

Note: Run the VSCode "C/C++: g++.exe build castle_platformer" task to build the project.
You can also use the "Run and Debug" button in the debug tab.
