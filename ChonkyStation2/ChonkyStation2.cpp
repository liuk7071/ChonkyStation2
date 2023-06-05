#define SDL_MAIN_HANDLED
#include <iostream>
#include <SDL.h>
#include "Third party/opengl.h"

#include "Source/ps2.h"

std::map<int, bool> keyboard;
bool vblank = false;

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
    //ps2.ee.pc = ps2.memory.LoadELF("C:\\Users\\zacse\\Downloads\\3stars\\3stars\\3stars.elf");
    //ps2.ee.pc = ps2.memory.LoadELF("C:\\Users\\zacse\\Downloads\\ps2tut\\ps2tut\\ps2tut_02a\\demo2a.elf");
    //ps2.ee.pc = ps2.memory.LoadELF("C:\\Users\\zacse\\Downloads\\ps2tut\\ps2tut\\ps2tut_02b\\demo2b.elf");
    //ps2.ee.pc = ps2.memory.LoadELF("C:\\Users\\zacse\\Downloads\\ps2tut\\ps2tut\\ps2tut_01\\demo1.elf");
    //ps2.ee.pc = ps2.memory.LoadELF("C:\\Users\\zacse\\Downloads\\OSDSYS");
    //ps2.ee.pc = ps2.memory.LoadELF("C:\\Users\\zacse\\Downloads\\SCUS_973.28");
    //ps2.ee.pc = ps2.memory.LoadELF("C:\\Users\\zacse\\Downloads\\cube.elf");
    ps2.memory.LoadBIOS("C:\\Users\\zacse\\Downloads\\ps2_bios\\scph10000.bin");

    bool quit = false;
    int cycle_count = 0;
    u8* out = new u8[2048 * 2048 * 4];
    while (!quit) {
        if (cycle_count > 500000) { // Temporarily just a random number
            ps2.gs.csr |= (1 << 3); // VSINT
            ps2.gs.csr ^= 1 << 13;
            ps2.gs.csr |= 1 << 1;
            if (!vblank) {
                // VBLANK START
                ps2.memory.intc_stat |= 1 << 2;
                ps2.memory.iop_i_stat |= 1; 
                vblank = !vblank;
            }
            else {
                // VBLANK END
                ps2.memory.intc_stat |= 1 << 3;
                ps2.memory.iop_i_stat |= 1 << 11;
                vblank = !vblank;
            }
            //ps2.memory.intc_stat |= 1 << 9;
            //ps2.memory.intc_stat |= 1 << 10;
            //ps2.memory.intc_stat |= 1 << 11;
            //ps2.memory.intc_stat |= 1 << 12;
            
            //ps2.memory.intc_stat |= 1;

            //ps2.memory.iop_i_stat |= 1 << 4;
            //ps2.memory.iop_i_stat |= 1 << 5;
            //ps2.memory.iop_i_stat |= 1 << 6;
            //if (ps2.memory.iop_i_mask & (1 << 9)) {
            //    ps2.memory.iop_i_stat |= 1 << 9; // SPU2
            //    printf("Firing SPU2 INTERRUPT\n");
            //}
            SDL_Event e;

            while (SDL_PollEvent(&e)) {
                switch (e.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_KEYDOWN:
                    keyboard[e.key.keysym.sym] = true;
                    break;
                case SDL_KEYUP:
                    keyboard[e.key.keysym.sym] = false;
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
            //printf("0x%08x, 0x%08x\n", ps2.ee.pc, ps2.memory.iop_i_mask);
            //std::ofstream file("ram.bin", std::ios::binary);
            //file.write((const char*)ps2.memory.ram, 32 MB);
        }

        ps2.cdvd.Step();
        ps2.ee.Step();
        ps2.ee.Step();
        ps2.ee.Step();
        ps2.ee.Step();
        ps2.ee.Step();
        ps2.ee.Step();
        ps2.ee.Step();
        ps2.ee.Step();
        ps2.iop.step();

        if (ps2.dma.SIF0.sif0_running) {
            if (ps2.sif.sif0_fifo.size() != 0) {
                while (ps2.sif.sif0_fifo.size() != 0) {
                    ps2.dma.SIF0.DoDMA(ps2.memory.ram, ps2.sif.ReadSIF0, &ps2.sif);
                    if (!ps2.dma.SIF0.sif0_running) {
                        ps2.dma.STAT |= 1 << 5;
                        if ((ps2.dma.STAT & 0x3ff) & ((ps2.dma.STAT >> 16) & 0x3ff))
                            ps2.memory.int1 = true;
                        break;
                    }
                }
            }
            //else {
             //   for (int i = 0; i < (ps2.ee.sideload_elf ? 70000 : 55); i++) {
             //       ps2.cdvd.Step(); ps2.iop.step();
            //    }
            //}
        }
        if (ps2.iopdma.SIF1.sif_running) {
            for (int i = 0; i < 20000; i++) ps2.ee.Step();
            ps2.iopdma.SIF1.DoDMA(ps2.memory.iop_ram, ps2.sif.ReadSIF1, &ps2.sif);
            if (!ps2.iopdma.SIF1.sif_running) {
                // IOP IRQ3
                // TODO (kinda important): This is not entirely correct
                ps2.iopdma.DICR2.IF = ps2.iopdma.DICR2.IF | (1 << 3);
                ps2.iopdma.DICR.raw |= (1 << 31);
                if (ps2.iopdma.DICR2.IM & ps2.iopdma.DICR2.IF) {
                    //printf("[INTC] (IOP) Requesting IRQ3\n");
                    ps2.memory.iop_i_stat |= 1 << 3;
                }
            }
        }
        cycle_count += 2;
    }
}
