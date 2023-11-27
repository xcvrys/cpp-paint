#pragma once
constexpr auto LINE_WIDTH_PLUS = 0;
constexpr auto LINE_WIDTH_MINUS = 1;
constexpr auto LINE_WIDTH_CHECK = 2;

constexpr auto MODE_PENCIL = 3;
constexpr auto MODE_RAINBOW = 4;
constexpr auto MODE_FILL = 5;
constexpr auto MODE_RECTANGLE = 6;
constexpr auto MODE_RECTANGLE_FILLED = 7;
constexpr auto MODE_CIRCLE = 8;
constexpr auto MODE_CIRCLE_FILLED = 9;
constexpr auto MODE_STRAIGHT_LINE = 10;

constexpr auto MODE_BRUSH_ROUND = 11;
constexpr auto MODE_BRUSH_SQUARE = 12;

constexpr auto FLIP_SCREEN_HORIZONTAL = 13;
constexpr auto FLIP_SCREEN_VERTICAL = 14;

constexpr auto COLOR_WEEL = 15;

constexpr auto SAVE_IMAGE = 16;

int LineWidth = 2;

enum PencilState {
    DRAW,
    RAINBOW,
    LINE,
    FILL,
    RECTANGLE,
    RECTANGLE_FILLED,
    CIRCLE,
    CIRCLE_FILLED,
    STRAIGHT_LINE
};

enum BrushShape {
    ROUND_BRUSH,
    SQUARE_BRUSH
};

enum LineStyle {
    SOLID_LINE,
    DASHED_LINE,
    DOTTED_LINE
};

struct RGBColor {
    int red;
    int green;
    int blue;
};

float rainbowHue = 0.0f;

size_t BackgroundColor = 0x222222;
size_t color = 0xffffff;

PencilState Pencil = DRAW;

BrushShape CurrentBrushShape = ROUND_BRUSH;

