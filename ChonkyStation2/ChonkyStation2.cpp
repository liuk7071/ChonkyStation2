#define SDL_MAIN_HANDLED
#include <iostream>
#include <SDL.h>
#include "Third party/opengl.h"

#include "Source/ps2.h"

int main() {
    // SDL2 / OpenGL initialization
    SDL_Init(SDL_INIT_EVERYTHING & ~SDL_INIT_HAPTIC);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

    SDL_Window* window = SDL_CreateWindow("ChonkyStation2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_GLContext context = SDL_GL_CreateContext(window);

    if (gl3wInit()) {
        Helpers::Panic("GL3W Initialization error\n");
    }

    Ps2 ps2;
    ps2.ee.pc = ps2.memory.LoadELF("C:\\Users\\zacse\\Downloads\\3stars\\3stars\\3stars.elf");
    //ps2.ee.pc = ps2.memory.LoadELF("C:\\Users\\zacse\\Downloads\\ps2tut\\ps2tut\\ps2tut_02a\\demo2a.elf");
    //ps2.ee.pc = ps2.memory.LoadELF("C:\\Users\\zacse\\Downloads\\ps2tut\\ps2tut\\ps2tut_02b\\demo2b.elf");
    //ps2.ee.pc = ps2.memory.LoadELF("C:\\Users\\zacse\\Downloads\\ps2tut\\ps2tut\\ps2tut_01\\demo1.elf");
    //ps2.memory.LoadBIOS("C:\\Users\\zacse\\Downloads\\ps2_bios\\scph10000.bin");

    bool quit = false;
    int cycle_count = 0;
    u8* out = new u8[2048 * 2048 * 4];
    while (!quit) {
        if (cycle_count > 100000) { // Temporarily just a random number
            SDL_Event e;

            while (SDL_PollEvent(&e)) {
                switch (e.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                }
            }
            // Render
            ps2.gs.vram.bind();
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);

            SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(out, 2048, 2048, 8 * 4, 2048 * 4, 0xFF, 0xFF00, 0xFF0000, 0xFF000000);
            SDL_Surface* window_surface = SDL_GetWindowSurface(window);
            SDL_BlitSurface(surface, NULL, window_surface, NULL);
            SDL_UpdateWindowSurface(window);

            //SDL_GL_SwapWindow(window);
            cycle_count = 0;
        }

        ps2.ee.Step();
        cycle_count += 2;
    }
}
