#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <iostream>
#include <windowsx.h>
#include <vector>
#include <commdlg.h>
#include <stack>
#include <fstream>

#include "main.h"

#define Assert(Expression) if (!(Expression)) { *(int *)0 = 0; }

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

typedef uint32_t u32;

int ClientWidth;
int ClientHeight;
void* Memory;

bool IsShiftPressed = false;

HMENU hSubMenuPencilType;

std::deque<std::vector<u32>> drawingHistory;
size_t historyIndex = 0;

void FlipScreenHorizontal() {
    u32* pixels = (u32*)Memory;
    std::vector<u32> tempRow(ClientWidth);

    for (int y = 0; y < ClientHeight; ++y) {
        int left = 0;
        int right = ClientWidth - 1;

        while (left < right) {
            // Swap pixels horizontally
            std::swap(tempRow[left], pixels[y * ClientWidth + left]);
            std::swap(pixels[y * ClientWidth + left], pixels[y * ClientWidth + right]);
            std::swap(tempRow[left], pixels[y * ClientWidth + right]);

            ++left;
            --right;
        }
    }
}

void FlipScreenVertical() {
    u32* pixels = (u32*)Memory;
    std::vector<u32> tempColumn(ClientHeight);

    for (int x = 0; x < ClientWidth; ++x) {
        int top = 0;
        int bottom = ClientHeight - 1;

        while (top < bottom) {
            // Swap pixels vertically
            std::swap(tempColumn[top], pixels[top * ClientWidth + x]);
            std::swap(pixels[top * ClientWidth + x], pixels[bottom * ClientWidth + x]);
            std::swap(tempColumn[top], pixels[bottom * ClientWidth + x]);

            ++top;
            --bottom;
        }
    }
}

void SaveDrawingState() {
    u32* pixels = (u32*)Memory;
    drawingHistory.resize(historyIndex);
    drawingHistory.push_back(std::vector<u32>(pixels, pixels + ClientWidth * ClientHeight));
    historyIndex = drawingHistory.size();
}

void RestoreDrawingState() {
    u32* pixels = (u32*)Memory;
    std::vector<u32> currentState = drawingHistory[historyIndex - 1];
    std::copy(currentState.begin(), currentState.end(), pixels);
}

void UndoDrawing() {
    if (historyIndex != 0) {
        RestoreDrawingState();
        historyIndex--;
    }
}

void RedoDrawing() { // HORK IN HALF
    if (historyIndex < drawingHistory.size()) {
        historyIndex++;
        RestoreDrawingState();
    }
}

COLORREF HSVToRGB(float hue, float saturation, float value) {
    int hi = static_cast<int>(hue * 6.0f) % 6;
    float f = hue * 6.0f - hi;
    float p = value * (1.0f - saturation);
    float q = value * (1.0f - f * saturation);
    float t = value * (1.0f - (1.0f - f) * saturation);

    switch (hi) {
    case 0:
        return RGB(value * 255, t * 255, p * 255);
    case 1:
        return RGB(q * 255, value * 255, p * 255);
    case 2:
        return RGB(p * 255, value * 255, t * 255);
    case 3:
        return RGB(p * 255, q * 255, value * 255);
    case 4:
        return RGB(t * 255, p * 255, value * 255);
    default:
        return RGB(value * 255, p * 255, q * 255);
    }
}

void DrawPixel(int X, int Y, u32 Color) {
    if (X >= 0 && X < ClientWidth && Y >= 0 && Y < ClientHeight) {
        u32* Pixel = (u32*)Memory;
        Pixel[Y * ClientWidth + X] = Color;
    }
}

void DrawRectangle(int X, int Y, int Width, int Height, u32 Color, int LineWidth, bool isFilled) {
    int startX, endX, startY, endY;

    if (Width >= 0) {
        startX = X;
        endX = X + Width;
    }
    else {
        startX = X + Width;
        endX = X;
    }

    if (Height >= 0) {
        startY = Y;
        endY = Y + Height;
    }
    else {
        startY = Y + Height;
        endY = Y;
    }

    // Draw filled rectangle
    if (isFilled) {
        for (int i = startY; i < endY; i++) {
            for (int j = startX; j < endX; j++) {
                DrawPixel(j, i, Color);
            }
        }
    }
    else {
        // Draw horizontal lines
        for (int i = startY; i < startY + LineWidth; i++) {
            for (int j = startX; j < endX; j++) {
                DrawPixel(j, i, Color);
            }
        }

        for (int i = endY - LineWidth; i < endY; i++) {
            for (int j = startX; j < endX; j++) {
                DrawPixel(j, i, Color);
            }
        }

        // Draw vertical lines
        for (int i = startX; i < startX + LineWidth; i++) {
            for (int j = startY + LineWidth; j < endY - LineWidth; j++) {
                DrawPixel(i, j, Color);
            }
        }

        for (int i = endX - LineWidth; i < endX; i++) {
            for (int j = startY + LineWidth; j < endY - LineWidth; j++) {
                DrawPixel(i, j, Color);
            }
        }
    }
    SaveDrawingState();
}


void DrawCircle(int X, int Y, int Radius, u32 Color, int LineWidth, int isFilled) {
    int x = Radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        // Draw the filled circle by connecting the upper and lower parts with horizontal lines
        if (isFilled) {
            for (int i = -x; i <= x; i++) {
                DrawPixel(X + i, Y + y, Color);
                DrawPixel(X + i, Y - y, Color);
            }
            for (int i = -y; i <= y; i++) {
                DrawPixel(X + i, Y + x, Color);
                DrawPixel(X + i, Y - x, Color);
            }
        }
        else {
            // Draw the circle outline
            for (int i = -LineWidth / 2; i <= LineWidth / 2; i++) {
                for (int j = -LineWidth / 2; j <= LineWidth / 2; j++) {
                    DrawPixel(X + x + i, Y + y + j, Color);
                    DrawPixel(X + y + i, Y + x + j, Color);
                    DrawPixel(X - y + i, Y + x + j, Color);
                    DrawPixel(X - x + i, Y + y + j, Color);
                    DrawPixel(X - x + i, Y - y + j, Color);
                    DrawPixel(X - y + i, Y - x + j, Color);
                    DrawPixel(X + y + i, Y - x + j, Color);
                    DrawPixel(X + x + i, Y - y + j, Color);
                }
            }
        }

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }

        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void FloodFill(int x, int y, u32 replacementColor) {
    std::stack<std::pair<int, int>> stack;
    u32* pixels = (u32*)Memory;
    int canvasWidth = ClientWidth;
    int canvasHeight = ClientHeight;

    // Get the target color from the pixel at the starting coordinates (x, y).
    u32 targetColor = pixels[y * canvasWidth + x];

    if (targetColor == replacementColor) {
        return;
    }

    std::vector<std::pair<int, int>> visited; // Keep track of visited pixels to avoid revisiting them.

    stack.push(std::make_pair(x, y));

    int dx[] = { 0, 0, -1, 1 };
    int dy[] = { -1, 1, 0, 0 };

    while (!stack.empty()) {
        std::pair<int, int> current = stack.top();
        stack.pop();
        x = current.first;
        y = current.second;

        if (pixels[y * canvasWidth + x] == targetColor) {
            pixels[y * canvasWidth + x] = replacementColor;

            for (int i = 0; i < 4; i++) {
                int newX = x + dx[i];
                int newY = y + dy[i];

                if (newX >= 0 && newX < canvasWidth && newY >= 0 && newY < canvasHeight) {
                    if (pixels[newY * canvasWidth + newX] == targetColor) {
                        stack.push(std::make_pair(newX, newY));
                    }
                }
            }
        }
    }
}

bool SaveImage(const char* fileName) {
    BITMAPFILEHEADER bmfh{};
    BITMAPINFOHEADER bmih{};

    // Initialize the BITMAPFILEHEADER
    bmfh.bfType = 0x4D42;  // 'BM' for Bitmap
    bmfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + static_cast<uint32_t>(ClientWidth) * ClientHeight * sizeof(uint32_t);
    bmfh.bfReserved1 = 0;
    bmfh.bfReserved2 = 0;
    bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // Initialize the BITMAPINFOHEADER
    bmih.biSize = sizeof(BITMAPINFOHEADER);
    bmih.biWidth = ClientWidth;
    bmih.biHeight = ClientHeight;  // Keep the height as it is
    bmih.biPlanes = 1;
    bmih.biBitCount = 32;
    bmih.biCompression = BI_RGB;
    bmih.biSizeImage = 0;
    bmih.biXPelsPerMeter = 0;
    bmih.biYPelsPerMeter = 0;
    bmih.biClrUsed = 0;
    bmih.biClrImportant = 0;

    std::ofstream file(fileName, std::ios::out | std::ios::binary);

    if (file.is_open()) {
        file.write(reinterpret_cast<char*>(&bmfh), sizeof(BITMAPFILEHEADER));
        file.write(reinterpret_cast<char*>(&bmih), sizeof(BITMAPINFOHEADER));
        for (int y = 0; y < ClientHeight; y++) {
            for (int x = 0; x < ClientWidth; x++) {
                u32* pixel = (u32*)Memory + (ClientHeight - 1 - y) * ClientWidth + x;
                file.write(reinterpret_cast<char*>(pixel), sizeof(u32));
            }
        }
        file.close();
        return true;
    }
    return false;
}

void DrawLine(int X1, int Y1, int X2, int Y2, u32 Color, int LineWidth) {
    int dx = abs(X2 - X1);
    int dy = abs(Y2 - Y1);
    int sx = (X1 < X2) ? 1 : -1;
    int sy = (Y1 < Y2) ? 1 : -1;
    int err = dx - dy;
    int err2;

    for (;;) {
        int BrushSize = LineWidth / 2;
        for (int i = -BrushSize; i <= BrushSize; i++) {
            for (int j = -BrushSize; j <= BrushSize; j++) {
                int distance = (i * i) + (j * j);
                if (CurrentBrushShape == ROUND_BRUSH) {
                    // Draw in a circular pattern if CurrentBrushShape is ROUND_BRUSH
                    if (distance <= (BrushSize * BrushSize)) {
                        DrawPixel(X1 + i, Y1 + j, Color);
                    }
                }
                else {
                    // Draw in a square pattern if CurrentBrushShape is not ROUND_BRUSH
                    DrawPixel(X1 + i, Y1 + j, Color);
                }
            }
        }

        if (X1 == X2 && Y1 == Y2) {
            break;
        }

        err2 = 2 * err;

        if (err2 > -dy) {
            err -= dy;
            X1 += sx;
        }

        if (err2 < dx) {
            err += dx;
            Y1 += sy;
        }
    }
}

void DrawStraightLine(int X1, int Y1, int X2, int Y2, u32 Color, int LineWidth) {
    int dx = abs(X2 - X1);
    int dy = abs(Y2 - Y1);
    int sx = (X1 < X2) ? 1 : -1;
    int sy = (Y1 < Y2) ? 1 : -1;
    int err = dx - dy;
    int err2;

    for (;;) {
        int BrushSize = LineWidth / 2;
        for (int i = -BrushSize; i <= BrushSize; i++) {
            for (int j = -BrushSize; j <= BrushSize; j++) {
                int distance = (i * i) + (j * j);
                if (CurrentBrushShape == ROUND_BRUSH) {
                    // Draw in a circular pattern if CurrentBrushShape is ROUND_BRUSH
                    if (distance <= (BrushSize * BrushSize)) {
                        DrawPixel(X1 + i, Y1 + j, Color);
                    }
                }
                else {
                    // Draw in a square pattern if CurrentBrushShape is not ROUND_BRUSH
                    DrawPixel(X1 + i, Y1 + j, Color);
                }
            }
        }

        if (X1 == X2 && Y1 == Y2) {
            break;
        }

        err2 = 2 * err;

        if (err2 > -dy) {
            err -= dy;
            X1 += sx;
        }

        if (err2 < dx) {
            err += dx;
            Y1 += sy;
        }
    }
}


void ClearScreen(u32 Color) {
    u32* Pixel = (u32*)Memory;
    for (int Index = 0; Index < ClientWidth * ClientHeight; ++Index) {
        Pixel[Index] = Color;
    }
}

LRESULT CALLBACK WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
    static int IsDrawing = false;
    static int PrevX, PrevY;

    switch (Message) {
        case WM_CREATE: {
            HMENU hMenu = CreateMenu();
            HMENU hSubMenuPencil = CreatePopupMenu();
            HMENU hSubMenuPencilType = CreatePopupMenu();
            HMENU hSubMenuBrush = CreatePopupMenu();
            HMENU hSubMenuCanva = CreatePopupMenu();

            AppendMenuW(hSubMenuPencil, MF_STRING, LINE_WIDTH_PLUS, L"Plus");
            AppendMenuW(hSubMenuPencil, MF_STRING, LINE_WIDTH_MINUS, L"Minus");
            AppendMenuW(hSubMenuPencil, MF_STRING, LINE_WIDTH_CHECK, L"Check Line Width");

            AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hSubMenuPencil, L"Pencil");

            AppendMenuW(hSubMenuPencilType, MF_STRING, MODE_PENCIL, L"Pencil");
            AppendMenuW(hSubMenuPencilType, MF_OWNERDRAW, MODE_RAINBOW, L"Rainbow");
            AppendMenuW(hSubMenuPencilType, MF_STRING, MODE_FILL, L"Filler");
            AppendMenuW(hSubMenuPencilType, MF_STRING, MODE_RECTANGLE, L"Rectangle");
            AppendMenuW(hSubMenuPencilType, MF_STRING, MODE_RECTANGLE_FILLED, L"Filled Rectangle");
            AppendMenuW(hSubMenuPencilType, MF_STRING, MODE_CIRCLE, L"Circle");
            AppendMenuW(hSubMenuPencilType, MF_STRING, MODE_CIRCLE_FILLED, L"Filled Circle");
            AppendMenuW(hSubMenuPencilType, MF_STRING, MODE_STRAIGHT_LINE, L"Straight Line");

            AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hSubMenuPencilType, L"Pencil Type");

            AppendMenuW(hSubMenuBrush, MF_STRING, MODE_BRUSH_ROUND, L"Round Brush");
            AppendMenuW(hSubMenuBrush, MF_STRING, MODE_BRUSH_SQUARE, L"Square Brush");

            AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hSubMenuBrush, L"Brush Type");

            AppendMenuW(hSubMenuCanva, MF_STRING, FLIP_SCREEN_HORIZONTAL, L"Flip Horizontal");
            AppendMenuW(hSubMenuCanva, MF_STRING, FLIP_SCREEN_VERTICAL, L"Flip Vertical");

            AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hSubMenuCanva, L"Canva options");

            AppendMenuW(hMenu, MF_STRING, COLOR_WEEL, L"Color Weel");
            AppendMenuW(hMenu, MF_STRING, SAVE_IMAGE, L"Save Image");

            SetMenu(Window, hMenu);
            break;
        }
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* drawItem = (DRAWITEMSTRUCT*)LParam;
            if (drawItem->CtlType == ODT_MENU) {
                if (drawItem->itemID == MODE_RAINBOW) {
                    wchar_t rainbowText[] = L"Rainbow";
                    int textLength = wcslen(rainbowText);

                    int colorStep = 255 / 4;
                    int leftAdjustment = 19;

                    for (int i = 0; i < textLength; ++i) {
                        int red = i * colorStep;
                        int green = 255 - i * colorStep;
                        SetTextColor(drawItem->hDC, RGB(red, green, 0));
                        wchar_t letter[2] = { rainbowText[i], L'\0' };
                        RECT adjustedRect = drawItem->rcItem;
                        adjustedRect.left += leftAdjustment + i * 10;
                        DrawTextW(drawItem->hDC, letter, 1, &adjustedRect, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOCLIP);
                    }
                    return TRUE;
                }
            }
            break;
        }
        case WM_COMMAND: {
            switch (WParam) {
            case LINE_WIDTH_PLUS: {
                if (LineWidth >= 50) {
                    MessageBox(NULL, L"Line width can't be more than 50", L"Error", MB_OK | MB_ICONERROR);
                }
                else {
                    LineWidth++;
                }
                break;
            }
            case LINE_WIDTH_MINUS: {
                if (LineWidth > 1) {
                    LineWidth--;
                }
                else {
                    MessageBox(NULL, L"Line width can't be less than 1", L"Error", MB_OK | MB_ICONERROR);
                }
                break;
            }
            case MODE_PENCIL: {
                Pencil = DRAW;
                break;
            }
            case MODE_RAINBOW: {
                Pencil = RAINBOW;
                break;
            }
            case MODE_FILL: {
                Pencil = FILL;
                break;
            }
            case MODE_RECTANGLE: {
                Pencil = RECTANGLE;
                break;
            }
            case MODE_RECTANGLE_FILLED: {
                Pencil = RECTANGLE_FILLED;
                break;
            }
            case MODE_CIRCLE: {
                Pencil = CIRCLE;
                break;
            }
            case MODE_CIRCLE_FILLED: {
                Pencil = CIRCLE_FILLED;
                break;
            }
            case MODE_STRAIGHT_LINE: {
                Pencil = STRAIGHT_LINE;
			    break;
            }
            case MODE_BRUSH_ROUND: {
                CurrentBrushShape = ROUND_BRUSH;
                break;
            }
            case MODE_BRUSH_SQUARE: {
                CurrentBrushShape = SQUARE_BRUSH;
                break;
            }
            case FLIP_SCREEN_HORIZONTAL: {
			    FlipScreenHorizontal();
			    break;
		    }
            case FLIP_SCREEN_VERTICAL: {
                FlipScreenVertical();
                break;
            }
            case LINE_WIDTH_CHECK: {
                wchar_t message[21];
                swprintf(message, sizeof(message), LR"(Line width is: %d)", LineWidth);
                MessageBox(NULL, message, L"Info", MB_OK | MB_ICONERROR);
                break;
            }
            case COLOR_WEEL: {
                // Show color wheel
                CHOOSECOLOR cc;
                static COLORREF acrCustClr[16];
                HBRUSH hbrush = nullptr;
                static COLORREF rgbCurrent;
                ZeroMemory(&cc, sizeof(cc));
                cc.lStructSize = sizeof(cc);
                cc.hwndOwner = Window;
                cc.lpCustColors = (LPDWORD)acrCustClr;
                cc.rgbResult = rgbCurrent;
                cc.Flags = CC_FULLOPEN | CC_RGBINIT;

                if (ChooseColor(&cc) == TRUE) {
                    DWORD colorRGB = cc.rgbResult;
                    int red = GetRValue(colorRGB);
                    int green = GetGValue(colorRGB);
                    int blue = GetBValue(colorRGB);
                    color = (static_cast<unsigned long long>((DWORD)red) << 16) | (static_cast<size_t>((DWORD)green) << 8) | blue;
                }
                break;
            }
            case SAVE_IMAGE: {
                if (SaveImage("saved_image.bmp")) {
                    MessageBox(NULL, L"Image saved", L"Info", MB_OK);
                }
                break;
            }
            }
            break;
        }
        case WM_DESTROY: {
        PostQuitMessage(0);
    }
                   break;
    case WM_KEYDOWN: {
        static bool isF2Pressed = false;
        switch (WParam) {
        case VK_ESCAPE: {
            DestroyWindow(Window);
        }
        case VK_OEM_PLUS: {
            if (LineWidth >= 50) {
                MessageBox(NULL, L"Line width can't be more than 50", L"Error", MB_OK | MB_ICONERROR);
            }
            else {
                LineWidth++;
            }
            break;
        }
        case VK_OEM_MINUS: {
            if (LineWidth > 1) {
                LineWidth--;
            }
            else {
                MessageBox(NULL, L"Line width can't be less than 1", L"Error", MB_OK | MB_ICONERROR);
            }
            break;
        }
        case VK_SHIFT: {
            IsShiftPressed = true;
            break;
        }
        case VK_F2: {
            UndoDrawing();
            break;
        }
        case VK_F3: {
            RedoDrawing();
            break;
        }
        case VK_F4: { // TEST FUNC
            DrawStraightLine(300, 100, 500, 700, color, LineWidth);
            break;
        }
        break;
        }
    }
    case WM_KEYUP: {
        switch (WParam) {
        case VK_SHIFT: {
            IsShiftPressed = false;
            break;
        }
        }
        break;
    }
    case WM_LBUTTONDOWN: {
        SaveDrawingState();
        if (Pencil == FILL) {
            int X = LOWORD(LParam);
            int Y = HIWORD(LParam);
            FloodFill(X, Y, color);
        }
        if (Pencil == RECTANGLE || Pencil == CIRCLE || Pencil == RECTANGLE_FILLED || Pencil == CIRCLE_FILLED) {
            int PrevX = LOWORD(LParam);
            int PrevY = HIWORD(LParam);
        }
        if (Pencil == DRAW) {
            int X = LOWORD(LParam);
            int Y = HIWORD(LParam);

            if (IsShiftPressed) {
                PrevX = X;
                PrevY = Y;
            }
        }
        IsDrawing = true;
        PrevX = LOWORD(LParam);
        PrevY = HIWORD(LParam);
    }
    break;
    case WM_RBUTTONUP: {
        SaveDrawingState();
        int X = LOWORD(LParam);
        int Y = HIWORD(LParam);

        DrawStraightLine(PrevX, PrevY, X, Y, color, LineWidth);
    
        IsDrawing = false;
    }
    break;
    case WM_RBUTTONDOWN: {
        int X = LOWORD(LParam);
        int Y = HIWORD(LParam);

        PrevX = X;
        PrevY = Y;
    }
    break;

    case WM_LBUTTONUP: {
        if (Pencil == RECTANGLE || Pencil == RECTANGLE_FILLED || Pencil == CIRCLE || Pencil == CIRCLE_FILLED) {
            int X = LOWORD(LParam);
            int Y = HIWORD(LParam);

            if (Pencil == RECTANGLE) {
                DrawRectangle(PrevX, PrevY, X - PrevX, Y - PrevY, color, LineWidth, false);
            }
            else if (Pencil == RECTANGLE_FILLED) {
                DrawRectangle(PrevX, PrevY, X - PrevX, Y - PrevY, color, LineWidth, true);
            }
            else if (Pencil == CIRCLE) {
                int Radius = static_cast<int>(sqrt(pow(X - PrevX, 2) + pow(Y - PrevY, 2)));
                DrawCircle(PrevX, PrevY, Radius, color, LineWidth, false);
            }
            else if (Pencil == CIRCLE_FILLED) {
                int Radius = static_cast<int>(sqrt(pow(X - PrevX, 2) + pow(Y - PrevY, 2)));
                DrawCircle(PrevX, PrevY, Radius, color, LineWidth, true);
            }
        } else if (Pencil == DRAW && IsShiftPressed) {
            int X = LOWORD(LParam);
            int Y = HIWORD(LParam);

            DrawStraightLine(PrevX, PrevY, X, Y, color, LineWidth);
        }
        IsDrawing = false;
    }
    break;
    case WM_MOUSEMOVE: {
        if (IsDrawing && Pencil == DRAW && !IsShiftPressed) {
            int X = LOWORD(LParam);
            int Y = HIWORD(LParam);
            DrawLine(PrevX, PrevY, X, Y, color, LineWidth);
            PrevX = X;
            PrevY = Y;
        }
        if (IsDrawing && Pencil == RAINBOW) {
            int X = LOWORD(LParam);
            int Y = HIWORD(LParam);
            rainbowHue += 0.01f;
            if (rainbowHue > 1.0f) {
                rainbowHue = 0.0f;
            }
            COLORREF rainbowColor = HSVToRGB(rainbowHue, 1.0f, 1.0f);
            DrawLine(PrevX, PrevY, X, Y, rainbowColor, LineWidth);
            PrevX = X;
            PrevY = Y;
        }
    }
    break;
        default: {
            return DefWindowProcW(Window, Message, WParam, LParam);
        }
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR Cmd, int CmdShow) {

    WNDCLASSW WindowClass = {
        .lpfnWndProc = WindowProc,
        .hInstance = Instance,
        .hCursor = LoadCursor(NULL, IDC_CROSS),
        .lpszClassName = L"MyWindowClass",
    };

    ATOM Atom = RegisterClassW(&WindowClass);

    if (!Atom) {
        return 1;
    }

    HWND Window = CreateWindowW(WindowClass.lpszClassName, L"Drawing Pixels", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, Instance, NULL);

    if (!Window) {
        return 1;
    }

    ShowWindow(Window, CmdShow);

    RECT Rect;
    GetClientRect(Window, &Rect);
    ClientWidth = Rect.right - Rect.left;
    ClientHeight = Rect.bottom - Rect.top;

    Memory = VirtualAlloc(0, static_cast<unsigned long long>(ClientWidth) * ClientHeight * sizeof(u32), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    BITMAPINFO BitmapInfo;
    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = ClientWidth;
    BitmapInfo.bmiHeader.biHeight = -ClientHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    HDC DeviceContext = GetDC(Window);

    ClearScreen(BackgroundColor);
    for (;;) {
        MSG Message;
        if (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE)) {
            if (Message.message == WM_QUIT)
                break;
            TranslateMessage(&Message);
            DispatchMessage(&Message);
            continue;
        }

        // Render the drawing from the current state
        StretchDIBits(DeviceContext, 0, 0, ClientWidth, ClientHeight, 0, 0, ClientWidth, ClientHeight, Memory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
    }
    return 0;
}

int main() {
    WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOW);
    return 0;
}