#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <string>

#pragma comment(lib, "gdiplus.lib")

struct Elf64Header {
    uint32_t magic;
    uint32_t ident_pad1;
    uint64_t ident_pad2;
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

Gdiplus::Image* g_bgImage = nullptr;
std::wstring g_statusText = L"PS5 Compatibility Layer Ready - Drag & Drop a Wallpaper (.png/.jpg) or eboot.bin";

void LoadPS5Binary(const std::wstring& path) {
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file) return;

    Elf64Header header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.magic != 0x464C457F) {
        g_statusText = L"Error: File is not an unencrypted PS5 ELF binary.";
        return;
    }

    file.seekg(header.e_phoff);
    std::vector<Elf64Phdr> segments(header.e_phnum);
    file.read(reinterpret_cast<char*>(segments.data()), header.e_phnum * sizeof(Elf64Phdr));

    int loadedCount = 0;
    for (const auto& seg : segments) {
        if (seg.p_type == 1 && seg.p_memsz > 0) {
            void* mem = VirtualAlloc(nullptr, seg.p_memsz, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (mem) {
                std::memset(mem, 0, seg.p_memsz);
                file.seekg(seg.p_offset);
                file.read(reinterpret_cast<char*>(mem), seg.p_filesz);
                loadedCount++;
            }
        }
    }
    g_statusText = L"Loaded PS5 Binary! Segments mapped into RAM: " + std::to_wstring(loadedCount);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DROPFILES: {
            wchar_t filePath[MAX_PATH];
            DragQueryFileW((HDROP)wParam, 0, filePath, MAX_PATH);
            DragFinish((HDROP)wParam);

            std::wstring path(filePath);
            if (path.ends_with(L".png") || path.ends_with(L".jpg") || path.ends_with(L".jpeg")) {
                delete g_bgImage;
                g_bgImage = new Gdiplus::Image(path.c_str());
                g_statusText = L"Custom Background Updated!";
            } else {
                LoadPS5Binary(path);
            }
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rect;
            GetClientRect(hwnd, &rect);

            Gdiplus::Graphics graphics(hdc);
            if (g_bgImage && g_bgImage->GetLastStatus() == Gdiplus::Ok) {
                graphics.DrawImage(g_bgImage, 0, 0, rect.right, rect.bottom);
            } else {
                Gdiplus::SolidBrush bgBrush(Gdiplus::Color(255, 15, 20, 35));
                graphics.FillRectangle(&bgBrush, 0, 0, rect.right, rect.bottom);
            }

            Gdiplus::Font font(L"Segoe UI", 16);
            Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 240, 245, 255));
            graphics.DrawString(g_statusText.c_str(), -1, &font, Gdiplus::PointF(30.0f, 30.0f), &textBrush);

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"PS5CompatLayerClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        WS_EX_ACCEPTFILES, L"PS5CompatLayerClass", L"PS5 Compatibility Layer",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
        nullptr, nullptr, hInstance, nullptr
    );

    ShowWindow(hwnd, nCmdShow);

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    delete g_bgImage;
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return 0;
}
