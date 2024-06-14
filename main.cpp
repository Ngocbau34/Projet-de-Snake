#include <mbed.h>
#include <threadLvgl.h>

#include "demos/lv_demos.h"
#include <vector>
#include <string>
#include "apple.h"

// Pin definitions
const PinName PINS[] = {A0, A1, A2, A3, A4, A5};
const int NUM_PINS = sizeof(PINS) / sizeof(PinName);

// LVGL setup
ThreadLvgl threadLvgl(30);
lv_obj_t *pin_labels[NUM_PINS];
lv_obj_t *start_button;
lv_obj_t *start_label;

// Snake game parameters
const int SNAKE_SIZE = 15;
const int SCREEN_WIDTH = 480;
const int SCREEN_HEIGHT = 270;
const int INITIAL_SPEED = 100; // Initial speed in milliseconds

enum Direction
{
    UP,
    DOWN,
    LEFT,
    RIGHT,
    STOP
};
Direction snake_dir = STOP;
bool game_running = false;
bool gameover = false;
bool paused = false;
int score = 0;
int game_speed = INITIAL_SPEED; // Initial game speed

struct Point
{
    int x, y;
};

std::vector<Point> snake;
Point food;
std::string player_name = "Player";

// Color variables
lv_color_t snake_color;

// Initialize digital input pins globally
DigitalIn up(PINS[2]);
DigitalIn left_btn(PINS[4]);
DigitalIn right_btn(PINS[3]);
DigitalIn down(PINS[5]);
DigitalIn reset_btn(PINS[0]);
DigitalIn pause_btn(PINS[1]);

void place_food()
{
    food.y = (rand() % (SCREEN_HEIGHT / SNAKE_SIZE)) * SNAKE_SIZE;
    food.x = (rand() % (SCREEN_WIDTH / SNAKE_SIZE)) * SNAKE_SIZE;
}

void init_game()
{
    // Reset the game
    snake.clear();
    for (int i = 0; i < 5; ++i)
    {
        snake.push_back({SCREEN_WIDTH / 2 + i * SNAKE_SIZE, SCREEN_HEIGHT / 2});
    }
    place_food();
    snake_dir = LEFT; // Start moving left by default
    game_running = true;
    gameover = false;
    paused = false;
    score = 0;
    game_speed = INITIAL_SPEED; // Reset game speed
    snake_color = lv_color_hex(0x800080); // Initial snake color (Purple)
}

void update_direction()
{
    // Read input pins and update direction accordingly
    if ((up.read() == 0) && (snake_dir != DOWN))
    {
        snake_dir = UP;
    }
    else if ((left_btn.read() == 0) && (snake_dir != RIGHT))
    {
        snake_dir = LEFT;
    }
    else if ((right_btn.read() == 0) && (snake_dir != LEFT))
    {
        snake_dir = RIGHT;
    }
    else if ((down.read() == 0) && (snake_dir != UP))
    {
        snake_dir = DOWN;
    }
}

void move_snake()
{
    if (snake_dir == STOP || gameover || paused)
        return;

    Point head = snake.front();
    Point new_head = head;

    switch (snake_dir)
    {
    case UP:
        new_head.y -= SNAKE_SIZE;
        break;
    case DOWN:
        new_head.y += SNAKE_SIZE;
        break;
    case LEFT:
        new_head.x -= SNAKE_SIZE;
        break;
    case RIGHT:
        new_head.x += SNAKE_SIZE;
        break;
    default:
        break;
    }

    // Wrap around the screen
    if (new_head.x >= SCREEN_WIDTH)
        new_head.x = 0;
    if (new_head.x < 0)
        new_head.x = SCREEN_WIDTH - SNAKE_SIZE;
    if (new_head.y >= SCREEN_HEIGHT)
        new_head.y = 0;
    if (new_head.y < 0)
        new_head.y = SCREEN_HEIGHT - SNAKE_SIZE;

    // Check for collisions with itself
    for (const auto &part : snake)
    {
        if (new_head.x == part.x && new_head.y == part.y)
        {
            game_running = false;
            gameover = true;
            return;
        }
    }

    snake.insert(snake.begin(), new_head);

    // Check for food consumption
    if (new_head.x == food.x && new_head.y == food.y)
    {
        place_food();
        score++;

        // Change snake color
        snake_color = lv_color_hex(rand() % 0xFFFFFF);

        // Increase game speed every time the score increases
        game_speed -= 10; // Adjust this value to control the rate of speed increase
        if (game_speed < 50) // Ensure speed doesn't go below a certain threshold
            game_speed = 50;
    }
    else
    {
        snake.pop_back();
    }
}

void draw_game()
{
    lv_obj_clean(lv_scr_act());

    // Draw the snake
    for (size_t i = 0; i < snake.size(); ++i)
    {
        lv_obj_t *rect = lv_obj_create(lv_scr_act());
        lv_obj_set_size(rect, SNAKE_SIZE, SNAKE_SIZE);
        lv_obj_set_style_border_width(rect, 0, LV_PART_MAIN);
        
        if (i == 0)
        {
            // Head
            lv_obj_set_style_bg_color(rect, snake_color, LV_PART_MAIN);
        }
        else
        {
            // Body
            lv_obj_set_style_bg_color(rect, snake_color, LV_PART_MAIN);
        }

        lv_obj_align(rect, LV_ALIGN_TOP_LEFT, snake[i].x, snake[i].y);
    }
    // Draw the food
    draw_apple(lv_scr_act(), food.x, food.y);

    // Draw the score
    char score_str[32];
    sprintf(score_str, "Score: %d", score);
    lv_obj_t *score_label = lv_label_create(lv_scr_act());
    lv_label_set_text(score_label, score_str);
    lv_obj_align(score_label, LV_ALIGN_TOP_MID, 0, 5);
}

void start_button_event_handler(lv_event_t *e)
{
    init_game();
}

void create_start_button()
{
    start_button = lv_btn_create(lv_scr_act());
    lv_obj_align(start_button, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(start_button, start_button_event_handler, LV_EVENT_CLICKED, NULL);

    start_label = lv_label_create(start_button);
    lv_label_set_text(start_label, "Start Game");
}

void ok_button_event_handler(lv_event_t *e)
{
    lv_obj_clean(lv_scr_act()); // Xóa màn hình trước khi bắt đầu trò chơi mới
    init_game(); // Khởi tạo lại trò chơi
    create_start_button(); // Tạo nút bắt
}

void handle_gameover()
{
// Display game over screen and prompt for player name input
lv_obj_clean(lv_scr_act());
lv_obj_t *gameover_label = lv_label_create(lv_scr_act());
lv_label_set_text(gameover_label, "Game Over");
lv_obj_align(gameover_label, LV_ALIGN_CENTER, 0, -40);

lv_obj_t *score_label = lv_label_create(lv_scr_act());
char score_str[32];
sprintf(score_str, "Score: %d", score);
lv_label_set_text(score_label, score_str);
lv_obj_align(score_label, LV_ALIGN_CENTER, 0, -10);


// Create an input box for player name

lv_obj_t *ok_btn = lv_btn_create(lv_scr_act());
lv_obj_align(ok_btn, LV_ALIGN_CENTER, 0, 100);
lv_obj_add_event_cb(ok_btn, ok_button_event_handler, LV_EVENT_CLICKED, NULL);

lv_obj_t *ok_label = lv_label_create(ok_btn);
lv_label_set_text(ok_label, "Appuyez sur le bouton vert pour reinitialiser le jeu");
}

int main()
{
threadLvgl.lock();
create_start_button();
threadLvgl.unlock();
while (1)
{
threadLvgl.lock();
    if (game_running)
    {
        if (!paused)
        {
            update_direction();
            move_snake();
            draw_game();
            ThisThread::sleep_for(game_speed); // Sleep for the current game speed
        }
    }
    else if (gameover)
    {
        handle_gameover();
    }

    // Check for reset button
    if (reset_btn.read() == 0)
    {
        ThisThread::sleep_for(100ms);

        init_game();
    }

    // Check for pause button
    if (pause_btn.read() == 0)
    {
        ThisThread::sleep_for(400ms);

        paused = !paused;
    }

    threadLvgl.unlock();
    ThisThread::sleep_for(10ms); // Adjust this delay as needed
}
}
