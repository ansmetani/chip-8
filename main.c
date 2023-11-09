#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <SDL2/SDL.h>

#include <windows.h>

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define PIXEL_SIZE 10

#define TICK_INTERVAL 2

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ...\n");
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");

    if (fp == NULL) {
        printf("Error: Could not open file\n");
        return 1;
    }

    unsigned char gfx[SCREEN_WIDTH * SCREEN_HEIGHT] = {0};
    unsigned char key[16] = {0};

    unsigned short pc = 0x200;
    unsigned short opcode = 0;
    unsigned short I = 0;
    unsigned short sp = 0;

    unsigned char V[16] = {0};
    unsigned short stack[16] = {0};
    unsigned char memory[4096] = {0};

    unsigned char delay_timer = 0;
    unsigned char sound_timer = 0;

    bool draw_flag = true;

    unsigned char chip8_fontset[80] = { 
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };
    
    for (int i = 0; i < 80; i++) {
        memory[i] = chip8_fontset[i];
    }

    fseek(fp, 0, SEEK_END);
    int fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (fsize > 4096 - 512) {
        printf("Error: File size exceeds available memory\n");
        return 1;
    }
    
    unsigned char *buffer = malloc(fsize * sizeof(unsigned char));
    if (buffer == NULL) {
        printf("Error: Failed to allocate memory\n");
        return 1;
    }

    if (fread(buffer, 1, fsize, fp) != fsize) {
        printf("Error: Failed to read file\n");
        return 1;
    }

    for (int i = 0; i < fsize; i++) {
        memory[i + 512] = buffer[i];
    }

    free(buffer);
    fclose(fp);

    srand(time(NULL));


    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Window *window = SDL_CreateWindow(
        argv[1],
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH * PIXEL_SIZE,
        SCREEN_HEIGHT * PIXEL_SIZE,
        SDL_WINDOW_SHOWN
    );
    if (window == NULL) {
        printf("Error: Could not create window (%s)\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Error: Could not create renderer (%s)\n", SDL_GetError());
        return 1;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Event event;

    Uint64 last_ticks = SDL_GetTicks64();
    
    for (;;) {
        if (SDL_GetTicks64() - last_ticks < TICK_INTERVAL) continue;
        last_ticks = SDL_GetTicks64();

        opcode = memory[pc] << 8 | memory[pc + 1];

        //printf("%04X, pc = %d\n", opcode, pc);
        
        switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x00FF) {
            case 0x00E0:
                for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
                    gfx[i] = 0;
                }
                draw_flag = true;
                pc += 2;
                break;
            
            case 0x00EE:
                sp -= 1;
                pc = stack[sp];
                pc += 2;
                break;
            }
            break;

        case 0x1000:
            pc = opcode & 0x0FFF;
            break;
        
        case 0x2000:
            stack[sp] = pc;
            sp += 1;
            pc = opcode & 0x0FFF;
            break;
        
        case 0x3000:
            if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;
        
        case 0x4000:
            if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;

        case 0x5000:
            if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;

        case 0x6000:
            V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            pc += 2;
            break;

        case 0x7000:
            V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            pc += 2;
            break;

        case 0x8000:
            switch (opcode & 0x000F) {
            case 0x0000:
                V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
                break;
            
            case 0x0001:
                V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
                break;

            case 0x0002:
                V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
                break;

            case 0x0003:
                V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
                break;

            case 0x0004:
                V[0xF] = (V[(opcode & 0x0F00) >> 8] + V[(opcode & 0x00F0) >> 4]) >> 8;
                V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
                break;

            case 0x0005:
                V[0xF] = (V[(opcode & 0x0F00) >> 8] >= V[(opcode & 0x00F0) >> 4]) ? 1 : 0;
                V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
                break;
            
            case 0x0006:
                V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x01;
                V[(opcode & 0x0F00) >> 8] >>= 1;
                break;
            
            case 0x0007:
                V[0xF] = (V[(opcode & 0x0F00) >> 8] <= V[(opcode & 0x00F0) >> 4]) ? 1 : 0;
                V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
                break;
            
            case 0x000E:
                V[0xF] = (V[(opcode & 0x0F00) >> 8] & 0x80) >> 7;
                V[(opcode & 0x0F00) >> 8] <<= 1;
                break;
            
            default:
                printf("Error: Unknown opcode %04X\n", opcode);
            }
            pc += 2;
            break;
        
        case 0x9000:
            if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4]) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;
        
        case 0xA000:
            I = opcode & 0x0FFF;
            pc += 2;
            break;
        
        case 0xB000:
            pc = (opcode & 0x0FFF) + V[0x0];
            break;
        
        case 0xC000:
            V[(opcode & 0x0F00) >> 8] = (rand() % 0x100) & (opcode & 0x00FF);
            pc += 2;
            break;
        
        case 0xD000: {
            unsigned char x = V[(opcode & 0x0F00) >> 8];
            unsigned char y = V[(opcode & 0x00F0) >> 4];
            V[0xF] = 0;
            for (int i = 0; i < (opcode & 0x000F); i++) {
                unsigned char row = memory[I + i];
                for (int j = 0; j < 8; j++) {
                    if ((row & (0x80 >> j)) != 0) {
                        if (gfx[x + j + (y + i) * SCREEN_WIDTH] != 0) V[0xF] = 1;
                        gfx[x + j + (y + i) * SCREEN_WIDTH] ^= 1;
                    }
                }
            }
            draw_flag = true;
            pc += 2;
            break;
        }
        
        case 0xE000:
            switch (opcode & 0x00FF) {
            case 0x009E:
                if (key[V[(opcode & 0x0F00) >> 8]] != 0) {
                    pc += 4;
                } else {
                    pc += 2;
                }
                break;
            
            case 0x00A1:
                if (key[V[(opcode & 0x0F00) >> 8]] == 0) {
                    pc += 4;
                } else {
                    pc += 2;
                }
                break;
            
            default:
                printf("Error: Unknown opcode %04X\n", opcode);
            }
            break;
        
        case 0xF000:
            switch (opcode & 0x00FF) {
            case 0x0007:
                V[(opcode & 0x0F00) >> 8] = delay_timer;
                break;
            
            case 0x000A:
                bool key_press = false;
                for (int i = 0; i < 16; i++) {
                    if (key[i] != 0) {
                        V[(opcode & 0x0F00) >> 8] = i;
                        key_press = true;
                    }
                }
                if (key_press) pc += 2;
                break;
            
            case 0x0015:
                delay_timer = V[(opcode & 0x0F00) >> 8];
                break;

            case 0x0018:
                sound_timer = V[(opcode & 0x0F00) >> 8];
                break;

            case 0x001E:
                I += V[(opcode & 0x0F00) >> 8];
                break;

            case 0x0029:
                I = V[(opcode & 0x0F00) >> 8] * 5;
                break;

            case 0x0033:
                memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
                memory[I + 1] = (V[(opcode & 0x0F00) >> 8] % 100) / 10;
                memory[I + 2] = V[(opcode & 0x0F00) >> 8] % 10;
                break;

            case 0x0055:
                for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++) {
                    memory[I + i] = V[i];
                }
                break;

            case 0x0065:
                for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++) {
                    V[i] = memory[I + i];
                }
                break;
            
            default:
                printf("Error: Unknown opcode %04X\n", opcode);
            }
            pc += 2;
            break;
        
        default:
            printf("Error: Unknown opcode %04X\n", opcode);
        }

        if (draw_flag) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            for (int y = 0; y < SCREEN_HEIGHT; y++) {
                for (int x = 0; x < SCREEN_WIDTH; x++) {
                    if (gfx[x + y * SCREEN_WIDTH] != 0) {
                        SDL_Rect rect = {x * PIXEL_SIZE, y * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE};
                        SDL_RenderFillRect(renderer, &rect);
                    }
                }
            }
            SDL_RenderPresent(renderer);
            draw_flag = false;
        }

        SDL_PollEvent(&event);

        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_1) key[0x1] = 1;
            if (event.key.keysym.sym == SDLK_2) key[0x2] = 1;
            if (event.key.keysym.sym == SDLK_3) key[0x3] = 1;
            if (event.key.keysym.sym == SDLK_4) key[0xC] = 1;

            if (event.key.keysym.sym == SDLK_q) key[0x4] = 1;
            if (event.key.keysym.sym == SDLK_w) key[0x5] = 1;
            if (event.key.keysym.sym == SDLK_e) key[0x6] = 1;
            if (event.key.keysym.sym == SDLK_r) key[0xD] = 1;

            if (event.key.keysym.sym == SDLK_a) key[0x7] = 1;
            if (event.key.keysym.sym == SDLK_s) key[0x8] = 1;
            if (event.key.keysym.sym == SDLK_d) key[0x9] = 1;
            if (event.key.keysym.sym == SDLK_f) key[0xE] = 1;

            if (event.key.keysym.sym == SDLK_z) key[0xA] = 1;
            if (event.key.keysym.sym == SDLK_x) key[0x0] = 1;
            if (event.key.keysym.sym == SDLK_c) key[0xB] = 1;
            if (event.key.keysym.sym == SDLK_v) key[0xF] = 1;
        } else if (event.type == SDL_KEYUP) {
            if (event.key.keysym.sym == SDLK_1) key[0x1] = 0;
            if (event.key.keysym.sym == SDLK_2) key[0x2] = 0;
            if (event.key.keysym.sym == SDLK_3) key[0x3] = 0;
            if (event.key.keysym.sym == SDLK_4) key[0xC] = 0;

            if (event.key.keysym.sym == SDLK_q) key[0x4] = 0;
            if (event.key.keysym.sym == SDLK_w) key[0x5] = 0;
            if (event.key.keysym.sym == SDLK_e) key[0x6] = 0;
            if (event.key.keysym.sym == SDLK_r) key[0xD] = 0;

            if (event.key.keysym.sym == SDLK_a) key[0x7] = 0;
            if (event.key.keysym.sym == SDLK_s) key[0x8] = 0;
            if (event.key.keysym.sym == SDLK_d) key[0x9] = 0;
            if (event.key.keysym.sym == SDLK_f) key[0xE] = 0;

            if (event.key.keysym.sym == SDLK_z) key[0xA] = 0;
            if (event.key.keysym.sym == SDLK_x) key[0x0] = 0;
            if (event.key.keysym.sym == SDLK_c) key[0xB] = 0;
            if (event.key.keysym.sym == SDLK_v) key[0xF] = 0;
        }
        if (event.type == SDL_QUIT) {
            SDL_DestroyWindow(window);
            SDL_Quit();
            break;
        }

        if (sound_timer == 1) printf("BEEP\n");
        if (delay_timer > 0) delay_timer--;
        if (sound_timer > 0) sound_timer--;
    }
}