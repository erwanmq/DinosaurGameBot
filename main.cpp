#define UNICODE 1

#include <Windows.h>
#include <tchar.h>
#include <iostream>
#include <chrono>

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

struct bm
{
    BITMAP bitmap;
    bool val = false;
};

bm transformHDCtoBITMAP(HDC& hdc, HWND& hwnd)
{
    bm bm;

    WCHAR* wPath = L"image.bmp";

    BITMAPFILEHEADER bfHeader;
    BITMAPINFOHEADER biHeader;
    BITMAPINFO bInfo;
    HGDIOBJ hTempBitmap;
    HBITMAP hBitmap;
    BITMAP bAllDesktops;
    HDC hDC, hMemDC;
    LONG lWidth, lHeight;
    BYTE *bBits = NULL;
    HANDLE hHeap = GetProcessHeap();
    DWORD cbBits, dwWritten = 0;
    HANDLE hFile;
    INT x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    INT y = GetSystemMetrics(SM_YVIRTUALSCREEN);

    std::cout << x << " " << y << '\n';

    ZeroMemory(&bfHeader, sizeof(BITMAPFILEHEADER));
    ZeroMemory(&biHeader, sizeof(BITMAPINFOHEADER));
    ZeroMemory(&bInfo, sizeof(BITMAPINFO));
    ZeroMemory(&bAllDesktops, sizeof(BITMAP));

    hDC = GetDC(NULL);
    hTempBitmap = GetCurrentObject(hDC, OBJ_BITMAP);
    GetObjectW(hTempBitmap, sizeof(BITMAP), &bAllDesktops);

    lWidth = bAllDesktops.bmWidth;
    lHeight = bAllDesktops.bmHeight;

    DeleteObject(hTempBitmap);

    bfHeader.bfType = (WORD)('B' | ('M' << 8));
    bfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    biHeader.biSize = sizeof(BITMAPINFOHEADER);
    biHeader.biBitCount = 24;
    biHeader.biCompression = BI_RGB;
    biHeader.biPlanes = 1;
    biHeader.biWidth = lWidth;
    biHeader.biHeight = lHeight;

    bInfo.bmiHeader = biHeader;

    cbBits = (((24 * lWidth + 31)&~31) / 8) * lHeight;

    hMemDC = CreateCompatibleDC(hDC);
    hBitmap = CreateDIBSection(hDC, &bInfo, DIB_RGB_COLORS, (VOID **)&bBits, NULL, 0);
    SelectObject(hMemDC, hBitmap);
    BitBlt(hMemDC, 0, 0, lWidth, lHeight, hDC, x, y, SRCCOPY);

    
    hFile = CreateFileW(wPath, GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(INVALID_HANDLE_VALUE == hFile)
    {
        DeleteDC(hMemDC);
        ReleaseDC(NULL, hDC);
        DeleteObject(hBitmap);
        return bm;
    }
    WriteFile(hFile, &bfHeader, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
    WriteFile(hFile, &biHeader, sizeof(BITMAPINFOHEADER), &dwWritten, NULL);
    WriteFile(hFile, bBits, cbBits, &dwWritten, NULL);
    FlushFileBuffers(hFile);
    CloseHandle(hFile);

    DeleteDC(hMemDC);
    ReleaseDC(NULL, hDC);
    DeleteObject(hBitmap);

    bm.val = true;
    return bm;
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

    bm bm = transformHDCtoBITMAP(windowHDC, hwnd);
    if(!bm.val)
    {
        std::cout << "error bitmap\n";
        return -1;
    }

    while(GetAsyncKeyState(VK_ESCAPE) == 0)
    {
        
        if(GetPixel(windowHDC, dino_position.x + 100, dino_position.y) == color_dinosaur && GetPixel(windowHDC, dino_position.x + 150, dino_position.y) == color_dinosaur)
            pressSpaceBar(&ip, 100);
        else if(GetPixel(windowHDC, dino_position.x + 100, dino_position.y) == color_dinosaur)
            pressSpaceBar(&ip, 50);
    }
    ReleaseDC(hwnd, windowHDC);
    DeleteObject(hwnd);
    return 0;
}