#include <SDL2/SDL.h>
#include <stdbool.h>
#include <emscripten/emscripten.h>

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define BLOCK_SIZE 30

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};
bool running = true;
Uint32 last_tick = 0;
Uint32 drop_interval = 500;

typedef struct {
    int x, y;
    int shape[4][4];
} Tetromino;

Tetromino current_piece;

int I_shape[4][4] = {
    {0,0,0,0},
    {1,1,1,1},
    {0,0,0,0},
    {0,0,0,0}
};

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return false;
    }

    // Uproszczone tworzenie okna i renderera w jednym kroku
    if (SDL_CreateWindowAndRenderer(BOARD_WIDTH * BLOCK_SIZE, BOARD_HEIGHT * BLOCK_SIZE, 0, &window, &renderer) < 0) {
        return false;
    }

    return true;
}

void draw_block(int x, int y, SDL_Color color) {
    SDL_Rect rect = { x * BLOCK_SIZE, y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE };
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &rect);
}

bool can_move(int new_x, int new_y) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (current_piece.shape[i][j]) {
                int board_x = new_x + j;
                int board_y = new_y + i;

                if (board_x < 0 || board_x >= BOARD_WIDTH || board_y >= BOARD_HEIGHT) return false;
                if (board_y >= 0 && board[board_y][board_x]) return false;
            }
        }
    }
    return true;
}

void lock_piece() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (current_piece.shape[i][j]) {
                int x = current_piece.x + j;
                int y = current_piece.y + i;
                if (y >= 0 && y < BOARD_HEIGHT && x >= 0 && x < BOARD_WIDTH)
                    board[y][x] = 1;
            }
        }
    }
}

void spawn_piece() {
    current_piece.x = 3;
    current_piece.y = -2;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            current_piece.shape[i][j] = I_shape[i][j];
}

void clear_lines() {
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        bool full = true;
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board[y][x] == 0) {
                full = false;
                break;
            }
        }
        if (full) {
            for (int row = y; row > 0; row--) {
                for (int col = 0; col < BOARD_WIDTH; col++) {
                    board[row][col] = board[row - 1][col];
                }
            }
            for (int col = 0; col < BOARD_WIDTH; col++)
                board[0][col] = 0;
            y++;
        }
    }
}
void rotate_piece() {
    int rotated[4][4] = {0};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            rotated[j][3 - i] = current_piece.shape[i][j];

    Tetromino test = current_piece;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            test.shape[i][j] = rotated[i][j];

    if (can_move(test.x, test.y))
        current_piece = test;
}

void game_loop() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) running = false;
        else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_LEFT:
                    if (can_move(current_piece.x - 1, current_piece.y))
                        current_piece.x--;
                    break;
                case SDLK_RIGHT:
                    if (can_move(current_piece.x + 1, current_piece.y))
                        current_piece.x++;
                    break;
                case SDLK_DOWN:
                    if (can_move(current_piece.x, current_piece.y + 1))
                        current_piece.y++;
                    break;
                case SDLK_UP:
    rotate_piece();
    break;
            }
        }
    }

    Uint32 now = SDL_GetTicks();
    if (now - last_tick > drop_interval) {
        if (can_move(current_piece.x, current_piece.y + 1)) {
            current_piece.y++;
        } else {
            lock_piece();
            clear_lines();
            spawn_piece();
            if (!can_move(current_piece.x, current_piece.y)) {
                running = false;
                emscripten_cancel_main_loop();
            }
        }
        last_tick = now;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board[y][x]) {
                draw_block(x, y, (SDL_Color){0, 255, 255, 255});
            }
        }
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (current_piece.shape[i][j]) {
                int px = current_piece.x + j;
                int py = current_piece.y + i;
                if (py >= 0)
                    draw_block(px, py, (SDL_Color){255, 0, 0, 255});
            }
        }
    }

    SDL_RenderPresent(renderer);
}

int main() {
    if (!init()) return 1;

    spawn_piece();
    last_tick = SDL_GetTicks();

    emscripten_set_main_loop(game_loop, 0, 1);
    return 0;
}
