#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>

struct Elf64Header {
    uint8_t  e_ident;
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
};

void InspectBinary(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return;

    Elf64Header header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.e_ident == 0x7F && header.e_ident == 'E' &&
        header.e_ident == 'L'  && header.e_ident == 'F') {
        std::cout << "[PS5 Loader] Valid ELF loaded! Entry Point: 0x"
                  << std::hex << header.e_entry << "\n";
    } else {
        std::cout << "[PS5 Loader] File is not an unencrypted ELF binary.\n";
    }
}

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) return 1;

    SDL_Window* window = SDL_CreateWindow("PS5 Compatibility Layer", 1280, 720, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    SDL_Texture* bgTexture = nullptr;

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;

            if (event.type == SDL_EVENT_DROP_FILE) {
                std::string droppedFile = event.drop.data;
                if (droppedFile.ends_with(".png") || droppedFile.ends_with(".jpg")) {
                    if (bgTexture) SDL_DestroyTexture(bgTexture);
                    bgTexture = IMG_LoadTexture(renderer, droppedFile.c_str());
                } else {
                    InspectBinary(droppedFile);
                }
            }
        }

        SDL_RenderClear(renderer);
        if (bgTexture) {
            SDL_RenderTexture(renderer, bgTexture, nullptr, nullptr);
        } else {
            SDL_SetRenderDrawColor(renderer, 15, 20, 35, 255);
            SDL_RenderFillRect(renderer, nullptr);
        }
        SDL_RenderPresent(renderer);
    }

    if (bgTexture) SDL_DestroyTexture(bgTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
