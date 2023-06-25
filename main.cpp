#define UNICODE 1

#include <Windows.h>
#include <tchar.h>
#include <iostream>
#include <chrono>
#include <memory>
#include <vector>
#include <algorithm> 


// structure to get the HBITMAP, the pointer of pixels, the width and the height of the screen
// the HBITMAP is there to allow the pointer to have the data otherwise it will be a dangling pointer
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

// get the position of the dinosaur to allow to the program to know where it is
POINT getDinosaurPosition()
{
    GetAsyncKeyState(1); // function to clean the stack
    POINT pos_dino;
    std::cout << "click on the dino\n";
    // while the player didn't press left click
    while(GetAsyncKeyState(1) == 0);
    GetCursorPos(&pos_dino);
    return pos_dino;
}

// transform the handle device context to a bitmap and get the pixels of it to the pointer of the structure
void transformHDCtoBITMAP(HDC& hdc, HWND& hwnd, PixelsInfo& pixelsInfo)
{
    // get the size of the window
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int MAX_WIDTH = rect.right;
    int MAX_HEIGHT = rect.bottom;

    // create a temporary handle device context to store the data of the HBITMAP
    HDC hdcTemp = CreateCompatibleDC(hdc);

    // create the information of our bitmap
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

    // create our pointer of pixels
    BYTE* bitPointer;
    pixelsInfo.hBitmap = CreateDIBSection(hdcTemp, &bitmap, DIB_RGB_COLORS, (void**)(&bitPointer), NULL, NULL);
    SelectObject(hdcTemp, pixelsInfo.hBitmap);
    BitBlt(hdcTemp, 0, 0, MAX_WIDTH, MAX_HEIGHT, hdc, 0, 0, SRCCOPY);

    DeleteDC(hdcTemp);

    
    // create a unique_ptr to be safer and to be used by the structure
    std::unique_ptr<BYTE> bitSmartPtr(bitPointer);
    
    pixelsInfo.bitPointer = std::move(bitSmartPtr);
    pixelsInfo.MAX_HEIGHT = MAX_HEIGHT;
    pixelsInfo.MAX_WIDTH = MAX_WIDTH;
}

// function to check if their is a cactus in front of the dinosaur
int findCactus(HDC& hdc, HWND& hwnd, PixelsInfo& pixelsInfo, POINT& position, int offset)
{
    pixelsInfo.bitPointer.release();
    DeleteObject(pixelsInfo.hBitmap);
    // get the pixels of the hdc in a pointer
    transformHDCtoBITMAP(hdc, hwnd, pixelsInfo);
    int red, green, blue;

    // create our line of checking 
    // this line is front of the dinosaur where the user clicked and is 50 pixels long (it will change with the offset during the time)
    int line_begin = (pixelsInfo.MAX_WIDTH * (position.y) + position.x + 160 + offset) * 4;
    int line_end = (pixelsInfo.MAX_WIDTH * (position.y) + position.x + 210 + offset) * 4;

    // get the RGB value of the background. We check a random pixel 50 pixels above our line.
    red = (int)pixelsInfo.bitPointer.get()[line_begin - pixelsInfo.MAX_WIDTH * 20 * 4];
    green = (int)pixelsInfo.bitPointer.get()[line_begin - pixelsInfo.MAX_WIDTH * 20 * 4 + 1];
    blue = (int)pixelsInfo.bitPointer.get()[line_begin - pixelsInfo.MAX_WIDTH * 20 * 4 + 2];

    // keep track of the number of pixels found
    int number_same_color{ 0 };
    // loop on the line
    for(int i{ line_begin }; i < line_end; i += 4)
    {
        // if the pixel the program is checking is different from the random background pixel, we add 1
        if((int)pixelsInfo.bitPointer.get()[i] != red && (int)pixelsInfo.bitPointer.get()[i + 1] != green && (int)pixelsInfo.bitPointer.get()[i + 2] != blue)
            number_same_color++;
    }

    return number_same_color;
}

// function to press space bar (jump) and to press arrow down(stoop)
void pressKeyDown(INPUT* ip, int ms)
{
    ip->ki.dwFlags = 0;
    SendInput(1, ip, sizeof(INPUT));
    Sleep(ms);
    ip->ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, ip, sizeof(INPUT));
}

void pressSpaceBar(INPUT* ip_spaceBar, INPUT* ip_downArray, int ms, HDC& hdc, HWND& hwnd, PixelsInfo& pixelsInfo, POINT& point, int offset)
{
    ip_spaceBar->ki.dwFlags = 0;
    SendInput(1, ip_spaceBar, sizeof(INPUT));
    std::chrono::time_point begin = std::chrono::steady_clock::now();
    while(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count() < ms)
    {
        int valtemp = findCactus(hdc, hwnd, pixelsInfo, point, offset);
        std::cout << valtemp;
        if(valtemp > 0)
        {
            std::cout << "je baisse\n";
            ip_spaceBar->ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, ip_spaceBar, sizeof(INPUT));
            pressKeyDown(ip_downArray, ms);
            
            return;
        }
    }
    ip_spaceBar->ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, ip_spaceBar, sizeof(INPUT));
}

int main()
{
    std::cout << "start\n";

    // get the dinosaur position on the screen thanks to the mouse left click
    POINT dino_position = getDinosaurPosition();
    HWND hwnd = GetDesktopWindow(); // get a handle window to the entire screen
    HDC windowHDC = GetDC(hwnd); // get the device context from the window handle

    // INPUT structures filled for the keyboard event space bar
    INPUT ip_space;
    ip_space.type = INPUT_KEYBOARD;
    ip_space.ki.wScan = 0;
    ip_space.ki.time = 0;
    ip_space.ki.dwExtraInfo = 0;
    ip_space.ki.wVk = VK_SPACE;

    // INPUT structure filled for the keyboard event arrow down
    INPUT ip_down_arrow;
    ip_down_arrow.type = INPUT_KEYBOARD;
    ip_down_arrow.ki.wScan = 0;
    ip_down_arrow.ki.time = 0;
    ip_down_arrow.ki.dwExtraInfo = 0;
    ip_down_arrow.ki.wVk = VK_DOWN;

    // structure to keep track of the cactus' pixels, the width and the height of the screen 
    PixelsInfo pixelsInfo;
    
    // creation of a vector to keep track of the maximum amount of pixels the line we checked will have
    std::vector<int> vValue_pixel;

    // while the player didn't press the space bar, the bot will not play
    while(GetAsyncKeyState(VK_SPACE) == 0);

    // keep track of the time to increase the offset of the line to check
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    std::cout << "the bot is playing\n";

    // the player has to press escape to exit the program
    while(GetAsyncKeyState(VK_ESCAPE) == 0)
    {
        // check the numbers of the cactus pixel front of the dinosaur in a straight line in the same y coordinates than the first mouse click
        int offset{(int)(std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() / 2)};
        int number_pixels = findCactus(windowHDC, hwnd, pixelsInfo, dino_position, offset);
        int number_max_pixels{ 0 };

        // if the number of pixels found is not equal to 0 it means that it left some cactus' pixels
        if(number_pixels != 0)
        {
            // push back these values
            vValue_pixel.push_back(number_pixels);
        }
        // if the numbers of pixels is 0 and the vector is filled
        else if(vValue_pixel.size() != 0)
        {
            // find the maximum element
            number_max_pixels = *std::max_element(vValue_pixel.begin(), vValue_pixel.end());
            // and clear the vector to be used again
            vValue_pixel.clear();
        }
        
        // if the maximum numbers of pixels is between 5 and 40, it will be a little jump and increase with more pixels
        if(5 < number_max_pixels && number_max_pixels <= 20)
        {
            pressSpaceBar(&ip_space, &ip_down_arrow, 30, windowHDC, hwnd, pixelsInfo, dino_position, offset);
        }
        else if(20 < number_max_pixels && number_max_pixels <= 40)
        {
            pressSpaceBar(&ip_space, &ip_down_arrow, 100, windowHDC, hwnd, pixelsInfo, dino_position, offset);
        }
        else if(40 < number_max_pixels)
        {
            pressSpaceBar(&ip_space, &ip_down_arrow, 150, windowHDC, hwnd, pixelsInfo, dino_position, offset);
        }

        //std::cout << "number max = " << number_max_pixels << '\n';

        // keep track of the time
        end = std::chrono::steady_clock::now();
    }
      
    // release the device context and delete the content of the window handle
    ReleaseDC(hwnd, windowHDC);
    DeleteObject(hwnd);
    pixelsInfo.bitPointer.release();
    DeleteObject(pixelsInfo.hBitmap);

    return 0;
}
