#ifndef PROCASM_CONFIG_H
#define PROCASM_CONFIG_H
constexpr int WIDTH = 800, HEIGHT = 800;

constexpr int MAX_LINE_WIDTH = 46;
constexpr int MAX_LINES = 19;


constexpr int BOX_SIZE = 400;
constexpr int BOX_X = WIDTH / 2 - BOX_SIZE / 2 + 100;
constexpr int BOX_Y = HEIGHT / 2 - BOX_SIZE / 2;
constexpr int BOX_TEXT_MARGIN = 16;
constexpr int BOX_LINE_HEIGHT = 20;
constexpr int BOX_CHAR_WIDTH = 8;
constexpr int BOX_UNDO_BUFFER_SIZE = 1024;

constexpr int SPACES_PER_TAB = 4;

#endif // PROCASM_CONFIG_H
