#define UNICODE 1

#include <Windows.h>
#include <tchar.h>
#include <iostream>
#include <chrono>
#include <memory>
#include <vector>
#include <map>
#include <algorithm> 

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



int findObstacle(HDC& hdc, HWND& hwnd, PixelsInfo& pixelsInfo, POINT& position, COLORREF& color_dino, int offset)
{
    transformHDCtoBITMAP(hdc, hwnd, pixelsInfo);
    int red, green, blue;

    int line_begin = (pixelsInfo.MAX_WIDTH * (position.y) + position.x + 160 + offset) * 4;
    int line_end = (pixelsInfo.MAX_WIDTH * (position.y) + position.x + 210 + offset) * 4;

    red = (int)pixelsInfo.bitPointer.get()[line_begin - pixelsInfo.MAX_WIDTH * 20 * 4];
    green = (int)pixelsInfo.bitPointer.get()[line_begin - pixelsInfo.MAX_WIDTH * 20 * 4 + 1];
    blue = (int)pixelsInfo.bitPointer.get()[line_begin - pixelsInfo.MAX_WIDTH * 20 * 4 + 2];

    int number_same_color{ 0 };
    for(int i{ line_begin }; i < line_end; i += 4)
    {
        if((int)pixelsInfo.bitPointer.get()[i] != red && (int)pixelsInfo.bitPointer.get()[i + 1] != green && (int)pixelsInfo.bitPointer.get()[i + 2] != blue)
            number_same_color++;
    }

    return number_same_color;
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

    
    
    PixelsInfo pixelsInfo;
    int index{ 0 };

    

    std::vector<int> vValue_pixel;
    while(GetAsyncKeyState(VK_SPACE) == 0);

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "the bot is playing\n";
    while(GetAsyncKeyState(VK_ESCAPE) == 0)
    {
        int number = findObstacle(windowHDC, hwnd, pixelsInfo, dino_position, color_dinosaur, (int)(std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() / 2));
        int number_max{ 0 };
        if(number != 0)
        {
            vValue_pixel.push_back(number);
            goto end_while;
        }
        else if(vValue_pixel.size() != 0)
        {
            number_max = *std::max_element(vValue_pixel.begin(), vValue_pixel.end());
            vValue_pixel.clear();
        }
        

        if(5 < number_max && number_max <= 40)
        {
            pressSpaceBar(&ip, 5);
        }
        else if(40 < number_max && number_max <= 60)
        {
            pressSpaceBar(&ip, 30);
        }
        else if(60 < number_max)
        {
            pressSpaceBar(&ip, 70);
        }

        std::cout << "number max = " << number_max << '\n';
        
        end_while:
        pixelsInfo.bitPointer.release();
        DeleteObject(pixelsInfo.hBitmap);

        end = std::chrono::steady_clock::now();
    }
      
    ReleaseDC(hwnd, windowHDC);
    DeleteObject(hwnd);
    

    return 0;
}