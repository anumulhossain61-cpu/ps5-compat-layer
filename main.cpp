#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

struct Elf64Header {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
};

struct Elf64Phdr {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

void* AllocateExecMemory(size_t size) {
#ifdef _WIN32
    return VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (ptr == MAP_FAILED) ? nullptr : ptr;
#endif
}

void LoadPS5Binary(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return;

    Elf64Header header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.e_ident[0] != 0x7F || header.e_ident[1] != 'E' ||
        header.e_ident[2] != 'L'  || header.e_ident[3] != 'F') {
        std::cout << "[PS5 Loader] Invalid or encrypted binary.\n";
        return;
    }

    std::cout << "[PS5 Loader] Entry Point: 0x" << std::hex << header.e_entry << "\n";

    file.seekg(header.e_phoff);
    std::vector<Elf64Phdr> segments(header.e_phnum);
    file.read(reinterpret_cast<char*>(segments.data()), header.e_phnum * sizeof(Elf64Phdr));

    for (const auto& seg : segments) {
        if (seg.p_type == 1 && seg.p_memsz > 0) {
            void* mem = AllocateExecMemory(seg.p_memsz);
            if (mem) {
                std::memset(mem, 0, seg.p_memsz);
                file.seekg(seg.p_offset);
                file.read(reinterpret_cast<char*>(mem), seg.p_filesz);
                std::cout << "[Memory] Loaded segment -> VAddr: 0x" << std::hex << seg.p_vaddr
                          << " (" << std::dec << seg.p_memsz << " bytes)\n";
            }
        }
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
                std::string dropped = event.drop.data;
                if (dropped.ends_with(".png") || dropped.ends_with(".jpg")) {
                    if (bgTexture) SDL_DestroyTexture(bgTexture);
                    bgTexture = IMG_LoadTexture(renderer, dropped.c_str());
                } else {
                    LoadPS5Binary(dropped);
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
