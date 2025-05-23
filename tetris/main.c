#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

// 鍵盤設定
#define LEFT_KEY 0x25
#define RIGHT_KEY 0x27 
#define ROTATE_KEY 0x26 
#define DOWN_KEY 0x28 
#define FALL_KEY 0x20 

#define LEFT_FUNC() GetAsyncKeyState(LEFT_KEY) & 0x8000
#define RIGHT_FUNC() GetAsyncKeyState(RIGHT_KEY) & 0x8000
#define ROTATE_FUNC() GetAsyncKeyState(ROTATE_KEY) & 0x8000
#define DOWN_FUNC() GetAsyncKeyState(DOWN_KEY) & 0x8000
#define FALL_FUNC() GetAsyncKeyState(FALL_KEY) & 0x8000

#define FALL_DELAY 500
#define RENDER_DELAY 100


#define CANVAS_WIDTH 10
#define CANVAS_HEIGHT 20

typedef enum {
	EMPTY = -1,
	I, J, L, O, S, T, Z
}Shape_id;

typedef enum {
	RED = 41,
	GREEN, YELLOW, BLUE, PURPLE, CYAN, WHITE,
	BLACK = 0
}Color;

typedef struct {
	Shape_id shape;
	Color color;
	int size;
	char rotates[4][4][4];
}Shape;

typedef struct {
	Color color;
	Shape_id shape;
	bool current; // include
}Block;

typedef struct { // 遊戲狀態(方塊狀態、分數)
	int x;
	int y;
	int score;
	int rotate;
	int fall_time;
	Shape_id queue[4];
}State;



Shape shape[7] = { // 各種方塊的基本設定
	{
		.shape = I,
		.color = CYAN,
		.size = 4,
		.rotates =
		{
			{
				{0,0,0,0},
				{1,1,1,1},
				{0,0,0,0},
				{0,0,0,0},
			},
			{
				{0,0,1,0},
				{0,0,1,0},
				{0,0,1,0},
				{0,0,1,0},
			},
			{
				{0,0,0,0},
				{0,0,0,0},
				{1,1,1,1},
				{0,0,0,0},
			},
			{
				{0,1,0,0},
				{0,1,0,0},
				{0,1,0,0},
				{0,1,0,0},
			}
		}
	},

	{
		.shape = J,
		.color = BLUE,
		.size = 3,
		.rotates =
		{
			{
				{1,0,0},
				{1,1,1},
				{0,0,0},
			},

			{
				{0,1,1},
				{0,1,0},
				{0,1,0},
			},

			{
				{0,0,0},
				{1,1,1},
				{0,0,1},
			},

			{
				{0,1,0},
				{0,1,0},
				{1,1,0},
			}
		}
	},

	{
		.shape = L,
		.color = YELLOW,
		.size = 3,
		.rotates =
		{
			{
				{0,0,1},
				{1,1,1},
				{0,0,0},
			},

			{
				{0,1,0},
				{0,1,0},
				{0,1,1},
			},

			{
				{0,0,0},
				{1,1,1},
				{1,0,0},
			},

			{
				{1,1,0},
				{0,1,0},
				{0,1,0},
			}
		}
	},

	{
		.shape = O,
		.color = WHITE,
		.size = 2,
		.rotates =
		{
			{
				{1,1},
				{1,1}
			},

			{
				{1,1},
				{1,1}
			},

			{
				{1,1},
				{1,1}
			},

			{
				{1,1},
				{1,1}
			}
		}
	},

	{
		.shape = S,
		.color = GREEN,
		.size = 3,
		.rotates =
		{
			{
				{0,1,1},
				{1,1,0},
				{0,0,0},
			},

			{
				{0,1,0},
				{0,1,1},
				{0,0,1},
			},

			{
				{0,0,0},
				{0,1,1},
				{1,1,0},
			},

			{
				{1,0,0},
				{1,1,0},
				{0,1,0},
			}
		}
	},

	{
		.shape = T,
		.color = PURPLE,
		.size = 3,
		.rotates =
		{
			{
				{0,1,0},
				{1,1,1},
				{0,0,0},
			},

			{
				{0,1,0},
				{0,1,1},
				{0,1,0},
			},

			{
				{0,0,0},
				{1,1,1},
				{0,1,0},
			},

			{
				{0,1,0},
				{1,1,0},
				{0,1,0},
			}
		}
	},

	{
		.shape = Z,
		.color = RED,
		.size = 3,
		.rotates =
		{
			{
				{1,1,0},
				{0,1,1},
				{0,0,0},
			},

			{
				{0,0,1},
				{0,1,1},
				{0,1,0},
			},

			{
				{0,0,0},
				{1,1,0},
				{0,1,1},
			},

			{
				{0,1,0},
				{1,1,0},
				{1,0,0},
			}
		}
	}
};

void set_block(Block* block, Color color, Shape_id shape, bool current) {
	block->color = color;
	block->shape = shape;
	block->current = current;
}

void reset_block(Block* block) {
	block->color = BLACK;
	block->shape = EMPTY;
	block->current = false;
}

void print_canvas(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH], State *state) {
	printf("\033[0;0H\n");
	for (int i = 0; i < CANVAS_HEIGHT; i++) {
		printf("|");
		for (int j = 0; j < CANVAS_WIDTH; j++) {
			printf("\033[%dm\u3000", canvas[i][j].color);
		}

		printf("\033[0m");
		printf("|\n");
	}

	// 印遊戲旁邊的提示區塊
	printf("\033[%d;%dHNext:", 3, CANVAS_WIDTH * 2 + 5);

	for (int i = 1; i < 4; i++) { // queue的1~3
		Shape shape_data = shape[state->queue[i]];
		for (int j = 0; j < 4; j++) {
			printf("\033[%d;%dHNext:", i*4+j, CANVAS_WIDTH * 2 + 5);
			for (int k = 0; k < 4; k++) {
				if (j < shape_data.size && k < shape_data.size && shape_data.rotates[0][j][k]) {
					printf("\x1b[%dm  ", shape_data.color);
				}
				else {
					printf("\x1b[%0m  ");
				}
			}
		}
		
	}
}

bool move(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH], int original_x, int original_y, int original_rotate, int new_x, int new_y, int new_rotate, Shape_id shape_id) {
	Shape shape_data = shape[shape_id]; // 
	int size = shape_data.size;

	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			
			if (shape_data.rotates[new_rotate][i][j]) {
				// 判斷新位置會不會超出邊界
				if (new_x + j < 0 || new_x + j >= CANVAS_WIDTH || new_y + i < 0 || new_y + i >= CANVAS_HEIGHT) {
					return false;
				}
				// 判斷新位置會不會撞到其他方塊
				else if (!canvas[new_y + i][new_x + j].current && canvas[new_y + i][new_x + j].shape != EMPTY) {

				}
			}
		}
	}
	// 刪掉舊方塊
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			if (shape_data.rotates[new_rotate][i][j]) {
				reset_block(&canvas[original_y + i][original_x + j]);
			}
		}
	}

	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			if (shape_data.rotates[new_rotate][i][j]) {
				set_block(&canvas[new_y + i][new_x + j], shape_data.color, shape_id, true);
			}
		}
	}
}

int clearLine(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH])
{
	for (int i = 0; i < CANVAS_HEIGHT; i++)
	{
		for (int j = 0; j < CANVAS_WIDTH; j++)
		{
			if (canvas[i][j].current)
			{
				canvas[i][j].current = false;
			}
		}
	}

	int linesCleared = 0;
	for (int i = CANVAS_HEIGHT - 1; i > 0; i--) {
		bool is_full = true;
		for (int j = 0; j < CANVAS_WIDTH; j++) {
			if (canvas[i][j].shape == EMPTY) {
				is_full = false;
				break;
			}
		}
		if (is_full) {
			linesCleared += 1;

			for (int j = i; j > 0; j--) {
				for (int k = 0; k < CANVAS_WIDTH; k++) {
					
				}
			}
		}
	}
	return linesCleared;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
}


void logic(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH], State* state) {
	if (ROTATE_FUNC()) {
		int new_rotate = (state->rotate + 1) % 4;
		if (move(canvas, state->x, state->y, state->rotate, state->x, state->y + 1, state->rotate, state->queue[0])) {
			state->rotate = new_rotate;
		}
	}
	else if (LEFT_FUNC()) {
		if (move(canvas, state->x, state->y, state->rotate, state->x - 1, state->y + 1, state->rotate, state->queue[0])) {
			state->x -= 1;
		}
	}
	else if (RIGHT_FUNC()) {
		if (move(canvas, state->x, state->y, state->rotate, state->x + 1, state->y + 1, state->rotate, state->queue[0])) {
			state->x += 1;
		}
	}
	else if (DOWN_FUNC()) {
		state->fall_time = FALL_DELAY;
	}
	else if (FALL_FUNC()) {
		state->fall_time += FALL_DELAY * CANVAS_HEIGHT;
	}

	state->fall_time += RENDER_DELAY;

	while (state->fall_time >= FALL_DELAY) {
		state->fall_time -= FALL_DELAY;
		if (move(canvas, state->x, state->y, state->rotate, state->x, state->y + 1, state->rotate, state->queue[0])) {
			state->y++;
		}
		else {
			state->score += clear_line(canvas);

			// 初始化state
			state->x = CANVAS_WIDTH / 2;
			state->y = 0;
			state->rotate = 0;
			state->fall_time = 0;
			state->queue[0] = state->queue[1]; // 東西往前移和新增
			state->queue[1] = state->queue[2];
			state->queue[2] = state->queue[3];
			state->queue[3] = rand() % 7;
		}
	}
	
	return;
}

int main() {
	srand(time(NULL));

	State state = { // 初始化狀態
		.x = CANVAS_WIDTH / 2,
		.y = 0,
		.score = 0,
		.rotate = 0,
		.fall_time = 0
	};

	for (int i = 0; i < 4; i++) {
		state.queue[i] = rand() % 7;
	}

	Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH];
	for (int i = 0; i < CANVAS_HEIGHT; i++) {
		for (int j = 0; j < CANVAS_WIDTH; j++) {
			reset_block(&canvas[i][j]);
		}
	}

	Shape shape_data = shape[state.queue[0]];

	for (int i = 0; i < shape_data.size; i++) {
		for (int j = 0; j < shape_data.size; j++) {
			if (shape_data.rotates[state.rotate][i][j] == 1) {
				set_block(&canvas[state.y + i][state.x + j], shape_data.color, shape_data.shape, true);
			}
		}
	}

	while (true) {
		print_canvas(canvas, &state);
		logic(canvas, &state);
		Sleep(100);
	}

	return 0;
}