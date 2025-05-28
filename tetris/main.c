#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <windows.h>
#include <string.h> 


#define CANVAS_WIDTH 10
#define CANVAS_HEIGHT 20

#define FALL_DELAY 500
#define RENDER_DELAY 100

// 鍵盤對照表
#define LEFT_KEY 0x25
#define RIGHT_KEY 0x27 
#define ROTATE_KEY 0x26 
#define DOWN_KEY 0x28 
#define FALL_KEY 0x20 

// 判斷按鍵是否有被按下的函式
#define LEFT_FUNC() GetAsyncKeyState(LEFT_KEY) & 0x8000
#define RIGHT_FUNC() GetAsyncKeyState(RIGHT_KEY) & 0x8000
#define ROTATE_FUNC() GetAsyncKeyState(ROTATE_KEY) & 0x8000
#define DOWN_FUNC() GetAsyncKeyState(DOWN_KEY) & 0x8000
#define FALL_FUNC() GetAsyncKeyState(FALL_KEY) & 0x8000


typedef enum{
    RED = 41,
    GREEN,
    YELLOW,
    BLUE,
    PURPLE,
    CYAN,
    WHITE,
    BLACK = 0,
} Color;

typedef enum{
    EMPTY = -1,
    I,
    J,
    L,
    O,
    S,
    T,
    Z
} ShapeId;

typedef struct{
    ShapeId shape;
    Color color;
    int size;
    char rotates[4][4][4];
} Shape;

typedef struct{
    int x;
    int y;
    int score;
    int rotate;
    int fallTime;
    ShapeId queue[4];
    int combo;  // 追蹤連續消行次數

} State;

typedef struct{
    Color color;
    ShapeId shape;
    bool current;
} Block;

Shape shapes[7] = {
    {.shape = I,
     .color = CYAN,
     .size = 4,
     .rotates =
         {
             {{0, 0, 0, 0},
              {1, 1, 1, 1},
              {0, 0, 0, 0},
              {0, 0, 0, 0}},

             {{0, 0, 1, 0},
              {0, 0, 1, 0},
              {0, 0, 1, 0},
              {0, 0, 1, 0}},

             {{0, 0, 0, 0},
              {0, 0, 0, 0},
              {1, 1, 1, 1},
              {0, 0, 0, 0}},

             {{0, 1, 0, 0},
              {0, 1, 0, 0},
              {0, 1, 0, 0},
              {0, 1, 0, 0}}}},
    {.shape = J,
     .color = BLUE,
     .size = 3,
     .rotates =
         {
             {{1, 0, 0},
              {1, 1, 1},
              {0, 0, 0}},

             {{0, 1, 1},
              {0, 1, 0},
              {0, 1, 0}},

             {{0, 0, 0},
              {1, 1, 1},
              {0, 0, 1}},

             {{0, 1, 0},
              {0, 1, 0},
              {1, 1, 0}}}},
    {.shape = L,
     .color = YELLOW,
     .size = 3,
     .rotates =
         {
             {{0, 0, 1},
              {1, 1, 1},
              {0, 0, 0}},

             {{0, 1, 0},
              {0, 1, 0},
              {0, 1, 1}},

             {{0, 0, 0},
              {1, 1, 1},
              {1, 0, 0}},

             {{1, 1, 0},
              {0, 1, 0},
              {0, 1, 0}}}},
    {.shape = O,
     .color = WHITE,
     .size = 2,
     .rotates =
         {
             {{1, 1},
              {1, 1}},

             {{1, 1},
              {1, 1}},

             {{1, 1},
              {1, 1}},

             {{1, 1},
              {1, 1}}}},
    {.shape = S,
     .color = GREEN,
     .size = 3,
     .rotates =
         {
             {{0, 1, 1},
              {1, 1, 0},
              {0, 0, 0}},

             {{0, 1, 0},
              {0, 1, 1},
              {0, 0, 1}},

             {{0, 0, 0},
              {0, 1, 1},
              {1, 1, 0}},

             {{1, 0, 0},
              {1, 1, 0},
              {0, 1, 0}}}},
    {.shape = T,
     .color = PURPLE,
     .size = 3,
     .rotates =
         {
             {{0, 1, 0},
              {1, 1, 1},
              {0, 0, 0}},

             {{0, 1, 0},
              {0, 1, 1},
              {0, 1, 0}},

             {{0, 0, 0},
              {1, 1, 1},
              {0, 1, 0}},

             {{0, 1, 0},
              {1, 1, 0},
              {0, 1, 0}}}},
    {.shape = Z,
     .color = RED,
     .size = 3,
     .rotates =
         {
             {{1, 1, 0},
              {0, 1, 1},
              {0, 0, 0}},

             {{0, 0, 1},
              {0, 1, 1},
              {0, 1, 0}},

             {{0, 0, 0},
              {1, 1, 0},
              {0, 1, 1}},

             {{0, 1, 0},
              {1, 1, 0},
              {1, 0, 0}}}},
};

void setBlock(Block* block, Color color, ShapeId shape, bool current){
    block->color = color;
    block->shape = shape;
    block->current = current;
}

void resetBlock(Block* block){
    block->color = BLACK;
    block->shape = EMPTY;
    block->current = false;
}

void printCanvas(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH], State* state){
    printf("\033[0;0H\n");
    for (int i = 0; i < CANVAS_HEIGHT; i++){
        printf("|");
        for (int j = 0; j < CANVAS_WIDTH; j++){
            printf("\033[%dm\u3000", canvas[i][j].color);
        }
        printf("\033[0m|\n");
    }

    // 輸出Next:
    printf("\033[%d;%dHNext:", 3, CANVAS_WIDTH * 2 + 5);
    // 輸出有甚麼方塊
    for (int i = 1; i <= 3; i++){
        Shape shapeData = shapes[state->queue[i]];
        for (int j = 0; j < 4; j++){
            printf("\033[%d;%dH", i * 4 + j, CANVAS_WIDTH * 2 + 15);
            for (int k = 0; k < 4; k++){
                if (j < shapeData.size && k < shapeData.size && shapeData.rotates[0][j][k]){
                    printf("\x1b[%dm  ", shapeData.color);
                }
                else{
                    printf("\x1b[0m  ");
                }
            }
        }
    }
    return;
}

void show_timer(int seconds) {
    int minutes = seconds / 60;
    int secs = seconds % 60;
    printf("\033[1;%dH %02d:%02d", CANVAS_WIDTH * 2 + 20, minutes, secs);
}


bool move(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH], int originalX, int originalY, int originalRotate, int newX, int newY, int newRotate, ShapeId shapeId){
    Shape shapeData = shapes[shapeId];
    int size = shapeData.size;

    // 判斷方塊有沒有不符合條件
    for (int i = 0; i < size; i++){
        for (int j = 0; j < size; j++){
            if (shapeData.rotates[newRotate][i][j]){
                // 判斷有沒有出去邊界
                if (newX + j < 0 || newX + j >= CANVAS_WIDTH || newY + i < 0 || newY + i >= CANVAS_HEIGHT){
                    return false;
                }
                // 判斷有沒有碰到別的方塊
                if (!canvas[newY + i][newX + j].current && canvas[newY + i][newX + j].shape != EMPTY){
                    return false;
                }
            }
        }
    }

    // 移除方塊舊的位置
    for (int i = 0; i < size; i++){
        for (int j = 0; j < size; j++){
            if (shapeData.rotates[originalRotate][i][j]){
                resetBlock(&canvas[originalY + i][originalX + j]);
            }
        }
    }

    // 移動方塊至新的位置
    for (int i = 0; i < size; i++){
        for (int j = 0; j < size; j++){
            if (shapeData.rotates[newRotate][i][j]){
                setBlock(&canvas[newY + i][newX + j], shapeData.color, shapeId, true);
            }
        }
    }

    return true;
}

int clearLine(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH]){
    for (int i = 0; i < CANVAS_HEIGHT; i++){
        for (int j = 0; j < CANVAS_WIDTH; j++){
            if (canvas[i][j].current){
                canvas[i][j].current = false;
            }
        }
    }

    int linesCleared = 0;
    for (int i = CANVAS_HEIGHT - 1; i >= 0; i--){
        bool isFull = true;
        for (int j = 0; j < CANVAS_WIDTH; j++){
            if (canvas[i][j].shape == EMPTY) {
                isFull = false;
                break;
            }
        }
        if (isFull) {
            linesCleared += 1;

            for (int j = i; j > 0; j--){
                for (int k = 0; k < CANVAS_WIDTH; k++){
                    setBlock(&canvas[j][k], canvas[j - 1][k].color, canvas[j - 1][k].shape, false);
                    resetBlock(&canvas[j - 1][k]);
                }
            }
            i++;
        }
    }
    return linesCleared;
}

int calculate_score(int lines_cleared, int* combo) {
    int base_score = 0;
    switch (lines_cleared) {
        case 1: base_score = 1; break;
        case 2: base_score = 2; break;
        case 3: base_score = 4; break;
        case 4: base_score = 8; break;
        default: *combo = 0; return 0; // 沒有消行，combo 歸零
    }

    (*combo)++;
    int multiplier = (*combo > 1) ? 2 : 1;
    return base_score * multiplier;
}

void logic(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH], State* state){
    if (ROTATE_FUNC()){
        int newRotate = (state->rotate + 1) % 4;
        if (move(canvas, state->x, state->y, state->rotate, state->x, state->y, newRotate, state->queue[0])){
            state->rotate = newRotate;
        }
    }
    else if (LEFT_FUNC()){
        if (move(canvas, state->x, state->y, state->rotate, state->x - 1, state->y, state->rotate, state->queue[0])){
            state->x -= 1;
        }
    }
    else if (RIGHT_FUNC()){
        if (move(canvas, state->x, state->y, state->rotate, state->x + 1, state->y, state->rotate, state->queue[0])){
            state->x += 1;
        }
    }
    else if (DOWN_FUNC()){
        state->fallTime = FALL_DELAY;
    }
    else if (FALL_FUNC()){
        state->fallTime += FALL_DELAY * CANVAS_HEIGHT;
    }

    state->fallTime += RENDER_DELAY;

    while (state->fallTime >= FALL_DELAY){
        state->fallTime -= FALL_DELAY;
        if (move(canvas, state->x, state->y, state->rotate, state->x, state->y + 1, state->rotate, state->queue[0])){
            state->y++;
        }
        else{
            int lines = clearLine(canvas);
            state->score += calculate_score(lines, &state->combo);

            state->x = CANVAS_WIDTH / 2;
            state->y = 0;
            state->rotate = 0;
            state->fallTime = 0;
            state->queue[0] = state->queue[1];
            state->queue[1] = state->queue[2];
            state->queue[2] = state->queue[3];
            state->queue[3] = rand() % 7;

            //結束輸出
            if (!move(canvas, state->x, state->y, state->rotate, state->x, state->y, state->rotate, state->queue[0])){
                printf("\033[%d;%dH\x1b[41m GAME OVER \x1b[0m\033[%d;%dH", CANVAS_HEIGHT - 3, CANVAS_WIDTH * 2 + 5, CANVAS_HEIGHT + 5, 0);
                exit(0);//結束遊戲
            }
        }
    }
    return;
}




void show_welcome_screen(char* player_name) {
    system("cls");
    printf("=====================================\n");
    printf("         Welcome to Tetris!          \n");
    printf("=====================================\n");
    printf("Please enter your name (default: unknown): ");

    fgets(player_name, 50, stdin);
    // 移除換行符號
    size_t len = strlen(player_name);
    if (len > 0 && player_name[len - 1] == '\n') {
        player_name[len - 1] = '\0';
    }

    if (strlen(player_name) == 0) {
        strcpy_s(player_name, sizeof(player_name), "unknown");
    }

    printf("Hello, %s! Press Enter to start the game...\n", player_name);
    getchar(); // 等待 Enter
	system("cls"); // 清除畫面
}

void show_game_over_screen(const char* player_name, int score, int play_time, char* comment) {
    system("cls");
    printf("=====================================\n");
    printf("             GAME OVER               \n");
    printf("=====================================\n");
    printf("Player: %s\n", player_name);
    printf("Time Played: %02d:%02d\n", play_time / 60, play_time % 60);
    printf("Score: %d\n", score);
    printf("-------------------------------------\n");
    printf("Leave a bullet comment (彈幕攻擊): ");
    fgets(comment, 100, stdin);
    // 移除換行符號
    size_t len = strlen(comment);
    if (len > 0 && comment[len - 1] == '\n') {
        comment[len - 1] = '\0';
    }
}

//void save_record(const char* player_name, int score, int play_time, const char* comment) {
//    FILE* file = fopen("record.txt", "a");
//    if (file == NULL) {
//        perror("Failed to open record.txt");
//        return;
//    }
//
//    fprintf(file, "Player: %s\n", player_name);
//    fprintf(file, "Time Played: %02d:%02d\n", play_time / 60, play_time % 60);
//    fprintf(file, "Score: %d\n", score);
//    fprintf(file, "Comment: %s\n", comment);
//    fprintf(file, "-----------------------------\n");
//
//    fclose(file);
//}


int main(){

    srand(time(NULL));
    time_t start_time = time(NULL);

    char player_name[50];
    show_welcome_screen(player_name);

    State state = {
        .x = CANVAS_WIDTH / 2,
        .y = 0,
        .score = 0,
        .rotate = 0,
        .fallTime = 0,
		.combo = 0
    };

    for (int i = 0; i < 4; i++){
        state.queue[i] = rand() % 7;
    }

    Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH];
    for (int i = 0; i < CANVAS_HEIGHT; i++){
        for (int j = 0; j < CANVAS_WIDTH; j++){
            resetBlock(&canvas[i][j]);
        }
    }

    Shape shapeData = shapes[state.queue[0]];

    for (int i = 0; i < shapeData.size; i++){
        for (int j = 0; j < shapeData.size; j++){
            if (shapeData.rotates[0][i][j]){
                setBlock(&canvas[state.y + i][state.x + j], shapeData.color, state.queue[0], true);
            }
        }
    }

    while (1){
        printCanvas(canvas, &state);
        int elapsed_time = (int)(time(NULL) - start_time);
        show_timer(elapsed_time);
        logic(canvas, &state);
        Sleep(100);
    }

    int play_time = (int)(time(NULL) - start_time);
    char comment[100];
	system("cls"); // 清除畫面
    show_game_over_screen(player_name, state.score, play_time, comment);
    /*save_record(player_name, state.score, play_time, comment);*/


    return 0;
}
