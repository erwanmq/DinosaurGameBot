#define UNICODE 1

#include <Windows.h>
#include <tchar.h>
#include <iostream>
#include <chrono>
#include <memory>

POINT getDinosaurPosition()
{
    GetAsyncKeyState(1); // function to clean the stack
    POINT pos_dino;
    std::cout << "click on the dino\n";
    while(GetAsyncKeyState(1) == 0);
    GetCursorPos(&pos_dino);
    return pos_dino;
}

void pressSpaceBar(INPUT* ip, int ms)
{
    ip->ki.dwFlags = 0;
    SendInput(1, ip, sizeof(INPUT));
    Sleep(ms);
    ip->ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, ip, sizeof(INPUT));
}

struct PixelsInfo
{
    HBITMAP hBitmap;
    std::unique_ptr<BYTE> bitPointer;
    int MAX_WIDTH;
    int MAX_HEIGHT;

    PixelsInfo() = default;

    PixelsInfo(PixelsInfo& pixelsInfo)
    {
        bitPointer = std::move(pixelsInfo.bitPointer);
        MAX_WIDTH = pixelsInfo.MAX_WIDTH;
        MAX_HEIGHT = pixelsInfo.MAX_HEIGHT;
    }

    ~PixelsInfo() = default;
};

void transformHDCtoBITMAP(HDC& hdc, HWND& hwnd, PixelsInfo& pixelsInfo)
{
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int MAX_WIDTH = rect.right;
    int MAX_HEIGHT = rect.bottom;

    HDC hdcTemp = CreateCompatibleDC(hdc);
    BITMAPINFO bitmap;
    bitmap.bmiHeader.biSize = sizeof(bitmap.bmiHeader);
    bitmap.bmiHeader.biWidth = MAX_WIDTH;
    bitmap.bmiHeader.biHeight = -MAX_HEIGHT;
    bitmap.bmiHeader.biPlanes = 1;
    bitmap.bmiHeader.biBitCount = 32;
    bitmap.bmiHeader.biCompression = BI_RGB;
    bitmap.bmiHeader.biSizeImage = MAX_WIDTH * 4 * MAX_HEIGHT;
    bitmap.bmiHeader.biClrUsed = 0;
    bitmap.bmiHeader.biClrImportant = 0;

    BYTE* bitPointer;
    pixelsInfo.hBitmap = CreateDIBSection(hdcTemp, &bitmap, DIB_RGB_COLORS, (void**)(&bitPointer), NULL, NULL);
    SelectObject(hdcTemp, pixelsInfo.hBitmap);
    BitBlt(hdcTemp, 0, 0, MAX_WIDTH, MAX_HEIGHT, hdc, 0, 0, SRCCOPY);

    DeleteDC(hdcTemp);

    

    std::unique_ptr<BYTE> bitSmartPtr(bitPointer);
    
    pixelsInfo.bitPointer = std::move(bitSmartPtr);
    pixelsInfo.MAX_HEIGHT = MAX_HEIGHT;
    pixelsInfo.MAX_WIDTH = MAX_WIDTH;
}



bool findObstacle(HDC& hdc, HWND& hwnd, PixelsInfo& pixelsInfo, POINT& position, COLORREF& color_dino)
{
    transformHDCtoBITMAP(hdc, hwnd, pixelsInfo);
    int red, green, blue, alpha;

    red = (int)pixelsInfo.bitPointer.get()[(pixelsInfo.MAX_WIDTH * (position.y) + position.x) * 4 + 200 * 4 + 0];
    green = (int)pixelsInfo.bitPointer.get()[(pixelsInfo.MAX_WIDTH * (position.y) + position.x) * 4 + 200 * 4 + 1];
    blue = (int)pixelsInfo.bitPointer.get()[(pixelsInfo.MAX_WIDTH * (position.y) + position.x) * 4 + 200 * 4 + 2];

    if( red == (int)GetRValue(color_dino) && green == (int)GetGValue(color_dino) && blue == (int)GetBValue(color_dino))
    {
        return true;
    }   
    return false;
}

int main()
{
    std::cout << "start\n";

    POINT dino_position = getDinosaurPosition();
    HWND hwnd = GetDesktopWindow();
    HDC windowHDC = GetDC(hwnd);
    
    COLORREF color_dinosaur = GetPixel(windowHDC, dino_position.x, dino_position.y);

    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;
    ip.ki.wVk = VK_SPACE;

    std::cout << "the bot is playing\n";
    
    PixelsInfo pixelsInfo;
    while(GetAsyncKeyState(VK_ESCAPE) == 0)
    {
        if(findObstacle(windowHDC, hwnd, pixelsInfo, dino_position, color_dinosaur))
            pressSpaceBar(&ip, 50);
        
        pixelsInfo.bitPointer.release();
        DeleteObject(pixelsInfo.hBitmap);
    }   
    ReleaseDC(hwnd, windowHDC);
    DeleteObject(hwnd);
    

    return 0;
}