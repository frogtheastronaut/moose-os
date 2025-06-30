/*
    Moose Operating System
    Copyright 2025 Ethan Zhang, All rights reserved.
*/

#include "include/gui.h"
#include "../kernel/include/keyboard.h"
#include "../kernel/include/keydef.h"
#include "../lib/lib.h"

// declares
#define PONG_WIDTH 300
#define PONG_HEIGHT 180
#define PONG_X ((SCREEN_WIDTH - PONG_WIDTH) / 2)
#define PONG_Y ((SCREEN_HEIGHT - PONG_HEIGHT) / 2)

// area
#define GAME_AREA_X (PONG_X + 8)
#define GAME_AREA_Y (PONG_Y + 28)
#define GAME_AREA_WIDTH (PONG_WIDTH - 16)
#define GAME_AREA_HEIGHT (PONG_HEIGHT - 36)

// puddle
#define PADDLE_WIDTH 4
#define PADDLE_HEIGHT 15
#define PADDLE_SPEED 2

// boll 
#define BALL_SIZE 3
#define BALL_SPEED 1

// settings
#define WIN_SCORE 11  // First to 11 wins

// colors
#define PONG_BG_COLOR VGA_COLOR_BLACK
#define PADDLE_COLOR VGA_COLOR_WHITE
#define BALL_COLOR VGA_COLOR_WHITE
#define SCORE_COLOR VGA_COLOR_WHITE

// paddle struct
typedef struct {
    int x, y;
    int width, height;
} Paddle;

// ball struct
typedef struct {
    int x, y;
    int dx, dy;
    int size;
} Ball;

// vars from mars
static Paddle left_paddle;
static Paddle right_paddle;
static Ball ball;
static int left_score = 0;
static int right_score = 0;
static bool pong_active = false;
static bool game_paused = false;
static bool game_over = false;

static int winner = 0; // 1 for left player, 2 for right player
static int frame_counter = 0;
// AI delay counter
static int ai_delay_counter = 0; // controls AI paddle speed

// prev pos
static Ball prev_ball;
static Paddle prev_left_paddle;
static Paddle prev_right_paddle;

// exterm
extern bool dialog_active;
extern bool explorer_active;
extern bool editor_active;
extern bool terminal_active;
extern void dock_return(void);


/**
 * innit
 */
static void pong_init_game() {
    // left paddle
    left_paddle.x = GAME_AREA_X + 10;
    left_paddle.y = GAME_AREA_Y + (GAME_AREA_HEIGHT - PADDLE_HEIGHT) / 2;
    left_paddle.width = PADDLE_WIDTH;
    left_paddle.height = PADDLE_HEIGHT;
    
    // right paddle
    right_paddle.x = GAME_AREA_X + GAME_AREA_WIDTH - 10 - PADDLE_WIDTH;
    right_paddle.y = GAME_AREA_Y + (GAME_AREA_HEIGHT - PADDLE_HEIGHT) / 2;
    right_paddle.width = PADDLE_WIDTH;
    right_paddle.height = PADDLE_HEIGHT;
    
    // ball
    ball.x = GAME_AREA_X + GAME_AREA_WIDTH / 2;
    ball.y = GAME_AREA_Y + GAME_AREA_HEIGHT / 2;
    ball.dx = -BALL_SPEED; // always serve to user, agree to disagree
    ball.dy = -BALL_SPEED;
    ball.size = BALL_SIZE;
    
    // init prev positions
    prev_ball = ball;
    prev_left_paddle = left_paddle;
    prev_right_paddle = right_paddle;

    // reset
    ai_delay_counter = 0;

    // reset
    left_score = 0;
    right_score = 0;
    game_paused = false; 
    game_over = false;
    winner = 0;
    frame_counter = 0;
}

/**
 * center
 */
static void reset_ball() {
    ball.x = GAME_AREA_X + GAME_AREA_WIDTH / 2;
    ball.y = GAME_AREA_Y + GAME_AREA_HEIGHT / 2;
    
    // serve to ME
    ball.dx = -BALL_SPEED;
    ball.dy = -BALL_SPEED;

    // reset
    ai_delay_counter = 0;
}

/**
 *  check colision
 */
static bool check_paddle_collision(Paddle* paddle) {
    return (ball.x < paddle->x + paddle->width &&
            ball.x + ball.size > paddle->x &&
            ball.y < paddle->y + paddle->height &&
            ball.y + ball.size > paddle->y);
}

/**
 * update game
 */
static void update_game() {
    if (game_paused || game_over) return;
    
    // Move 
    ball.x += ball.dx;
    ball.y += ball.dy;
    
    // ball collision with top/bottom walls
    if (ball.y <= GAME_AREA_Y || ball.y + ball.size >= GAME_AREA_Y + GAME_AREA_HEIGHT) {
        ball.dy = -ball.dy;
    }
    
    // ball collision with paddles
    if (check_paddle_collision(&left_paddle) && ball.dx < 0) {
        ball.dx = -ball.dx;
        ball.x = left_paddle.x + left_paddle.width; // Prevent sticking
    }

    if (check_paddle_collision(&right_paddle) && ball.dx > 0) {
        ball.dx = -ball.dx;
        ball.x = right_paddle.x - ball.size; // Prevent sticking
    }
    
    // ball out of bounds (scoring)
    if (ball.x < GAME_AREA_X) {
        right_score++;
        if (right_score >= WIN_SCORE) {
            game_over = true;
            winner = 2; // right player wins (opp)
        } else {
            reset_ball();
        }
    } else if (ball.x > GAME_AREA_X + GAME_AREA_WIDTH) {
        left_score++;
        if (left_score >= WIN_SCORE) {
            game_over = true;
            winner = 1; // left player wins (you)
        } else {
            reset_ball(); // reset
        }
    }
    
    // ai 
    int paddle_center = right_paddle.y + right_paddle.height / 2;
    int ball_center = ball.y + ball.size / 2;

    ai_delay_counter++;
    // ai reacts every 2 frames
    if (ai_delay_counter >= 2) {
        ai_delay_counter = 0;  // reset

        if (ball_center < paddle_center - 8) {  
            right_paddle.y -= 1; 
        } else if (ball_center > paddle_center + 8) {  
            right_paddle.y += 1; 
        }
    }


    
    // keep paddle within game area
    if (right_paddle.y < GAME_AREA_Y) {
        right_paddle.y = GAME_AREA_Y;
    }
    if (right_paddle.y + right_paddle.height > GAME_AREA_Y + GAME_AREA_HEIGHT) {
        right_paddle.y = GAME_AREA_Y + GAME_AREA_HEIGHT - right_paddle.height;
    }
}

/**
 * draw game window
 */
static void pong_draw_window() {
    // clear
    gui_clear_screen(VGA_COLOR_LIGHT_GREY);
    
    // draw border
    gui_draw_window_box(PONG_X, PONG_Y, PONG_WIDTH, PONG_HEIGHT,
                       VGA_COLOR_BLACK, VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY);
    
    // draw title
    gui_draw_title_bar(PONG_X, PONG_Y, PONG_WIDTH, 15, VGA_COLOR_BLUE);
    gui_draw_text(PONG_X + 5, PONG_Y + 3, "MooseOS Pong", VGA_COLOR_WHITE);
    
    // draw game bkg
    gui_draw_rect(GAME_AREA_X, GAME_AREA_Y, GAME_AREA_WIDTH, GAME_AREA_HEIGHT, PONG_BG_COLOR);
    gui_draw_rect_outline(GAME_AREA_X - 1, GAME_AREA_Y - 1, GAME_AREA_WIDTH + 2, GAME_AREA_HEIGHT + 2, VGA_COLOR_WHITE);
}

/**
 * draw center line
 */
static void draw_center_line() {
    int center_x = GAME_AREA_X + GAME_AREA_WIDTH / 2;
    for (int y = GAME_AREA_Y; y < GAME_AREA_Y + GAME_AREA_HEIGHT; y += 8) {
        gui_draw_rect(center_x - 1, y, 2, 4, VGA_COLOR_DARK_GREY);
    }
}

/**
 * draw paddles
 */
static void draw_paddles() {
    // left
    gui_draw_rect(left_paddle.x, left_paddle.y, left_paddle.width, left_paddle.height, PADDLE_COLOR);
    
    // right
    gui_draw_rect(right_paddle.x, right_paddle.y, right_paddle.width, right_paddle.height, PADDLE_COLOR);
}

/**
 *  drawll ball
 */
static void draw_ball() {
    gui_draw_rect(ball.x, ball.y, ball.size, ball.size, BALL_COLOR);
}

/**
 * draw scores
 */
static void draw_scores() {
    char left_score_str[4];  // Support up to 2 digits + null terminator
    char right_score_str[4];
    
    // convert scores to strings
    if (left_score >= 10) {
        left_score_str[0] = '0' + (left_score / 10);
        left_score_str[1] = '0' + (left_score % 10);
        left_score_str[2] = '\0';
    } else {
        left_score_str[0] = '0' + left_score;
        left_score_str[1] = '\0';
    }
    
    if (right_score >= 10) {
        right_score_str[0] = '0' + (right_score / 10);
        right_score_str[1] = '0' + (right_score % 10);
        right_score_str[2] = '\0';
    } else {
        right_score_str[0] = '0' + right_score;
        right_score_str[1] = '\0';
    }
    
    // clear..
    gui_draw_rect(GAME_AREA_X + GAME_AREA_WIDTH / 4 - 15, GAME_AREA_Y + 2, 30, 12, VGA_COLOR_BLACK);
    gui_draw_rect(GAME_AREA_X + 3 * GAME_AREA_WIDTH / 4 - 15, GAME_AREA_Y + 2, 30, 12, VGA_COLOR_BLACK);
    
    // ..then draw
    gui_draw_text(GAME_AREA_X + GAME_AREA_WIDTH / 4, GAME_AREA_Y + 5, left_score_str, SCORE_COLOR);
    gui_draw_text(GAME_AREA_X + 3 * GAME_AREA_WIDTH / 4, GAME_AREA_Y + 5, right_score_str, SCORE_COLOR);
}

/**
 *  paws msg
 */
static void pong_drawpause() {
    if (game_paused) {
        const char* pause_msg = "PAUSED - Press SPACE to resume";
        int msg_width = gui_text_width(pause_msg);
        int msg_x = GAME_AREA_X + (GAME_AREA_WIDTH - msg_width) / 2;
        int msg_y = GAME_AREA_Y + GAME_AREA_HEIGHT / 2;
        
        // Draw background for text
        gui_draw_rect(msg_x - 2, msg_y - 2, msg_width + 4, 12, VGA_COLOR_DARK_GREY);
        gui_draw_text(msg_x, msg_y, pause_msg, VGA_COLOR_WHITE);
    }
}

/**
 * draw game over msg
 */
static void pong_gameover() {
    if (game_over) {
        const char* game_over_msg = "GAME OVER!";
        const char* winner_msg = winner == 1 ? "YOU WIN!" : "OPPONENT WINS!";
        const char* restart_msg = "Press R to restart";
        
        int game_over_width = gui_text_width(game_over_msg);
        int winner_width = gui_text_width(winner_msg);
        int restart_width = gui_text_width(restart_msg);
        
        int msg_x1 = GAME_AREA_X + (GAME_AREA_WIDTH - game_over_width) / 2;
        int msg_x2 = GAME_AREA_X + (GAME_AREA_WIDTH - winner_width) / 2;
        int msg_x3 = GAME_AREA_X + (GAME_AREA_WIDTH - restart_width) / 2;
        int msg_y1 = GAME_AREA_Y + GAME_AREA_HEIGHT / 2 - 15;
        int msg_y2 = GAME_AREA_Y + GAME_AREA_HEIGHT / 2;
        int msg_y3 = GAME_AREA_Y + GAME_AREA_HEIGHT / 2 + 15;
        
        // bkg
        gui_draw_rect(GAME_AREA_X + 20, msg_y1 - 5, GAME_AREA_WIDTH - 40, 40, VGA_COLOR_DARK_GREY);
        
        // msgs
        gui_draw_text(msg_x1, msg_y1, game_over_msg, VGA_COLOR_RED);
        gui_draw_text(msg_x2, msg_y2, winner_msg, VGA_COLOR_LIGHT_BROWN);
        gui_draw_text(msg_x3, msg_y3, restart_msg, VGA_COLOR_WHITE);
    }
}
/**
 * draw game
 */
static void pong_drawgame() {
    int clear_x = prev_ball.x - 3; 
    int clear_y = prev_ball.y - 3;
    int clear_width = prev_ball.size + 6;  
    int clear_height = prev_ball.size + 6;
    

    if (clear_x < GAME_AREA_X) {
        clear_width -= (GAME_AREA_X - clear_x);
        clear_x = GAME_AREA_X;
    }
    if (clear_y < GAME_AREA_Y) {
        clear_height -= (GAME_AREA_Y - clear_y);
        clear_y = GAME_AREA_Y;
    }
    if (clear_x + clear_width > GAME_AREA_X + GAME_AREA_WIDTH) {
        clear_width = (GAME_AREA_X + GAME_AREA_WIDTH) - clear_x;
    }
    if (clear_y + clear_height > GAME_AREA_Y + GAME_AREA_HEIGHT) {
        clear_height = (GAME_AREA_Y + GAME_AREA_HEIGHT) - clear_y;
    }
    
    // clear if in bounds
    if (clear_width > 0 && clear_height > 0) {
        gui_draw_rect(clear_x, clear_y, clear_width, clear_height, PONG_BG_COLOR);
    }
    
    // clear left paddle
    clear_x = prev_left_paddle.x - 3;  
    clear_y = prev_left_paddle.y - 3;
    clear_width = prev_left_paddle.width + 6;  
    clear_height = prev_left_paddle.height + 6;
    
    if (clear_x < GAME_AREA_X) {
        clear_width -= (GAME_AREA_X - clear_x);
        clear_x = GAME_AREA_X;
    }
    if (clear_y < GAME_AREA_Y) {
        clear_height -= (GAME_AREA_Y - clear_y);
        clear_y = GAME_AREA_Y;
    }
    if (clear_x + clear_width > GAME_AREA_X + GAME_AREA_WIDTH) {
        clear_width = (GAME_AREA_X + GAME_AREA_WIDTH) - clear_x;
    }
    if (clear_y + clear_height > GAME_AREA_Y + GAME_AREA_HEIGHT) {
        clear_height = (GAME_AREA_Y + GAME_AREA_HEIGHT) - clear_y;
    }
    
    if (clear_width > 0 && clear_height > 0) {
        gui_draw_rect(clear_x, clear_y, clear_width, clear_height, PONG_BG_COLOR);
    }
    
    // right paddle, same thing
    clear_x = prev_right_paddle.x - 3;  
    clear_y = prev_right_paddle.y - 3;
    clear_width = prev_right_paddle.width + 6;  
    clear_height = prev_right_paddle.height + 6;
    
    if (clear_x < GAME_AREA_X) {
        clear_width -= (GAME_AREA_X - clear_x);
        clear_x = GAME_AREA_X;
    }
    if (clear_y < GAME_AREA_Y) {
        clear_height -= (GAME_AREA_Y - clear_y);
        clear_y = GAME_AREA_Y;
    }
    if (clear_x + clear_width > GAME_AREA_X + GAME_AREA_WIDTH) {
        clear_width = (GAME_AREA_X + GAME_AREA_WIDTH) - clear_x;
    }
    if (clear_y + clear_height > GAME_AREA_Y + GAME_AREA_HEIGHT) {
        clear_height = (GAME_AREA_Y + GAME_AREA_HEIGHT) - clear_y;
    }
    
    if (clear_width > 0 && clear_height > 0) {
        gui_draw_rect(clear_x, clear_y, clear_width, clear_height, PONG_BG_COLOR);
    }
    
    // redraw center line
    draw_center_line();
    
    // draw current positions
    draw_paddles();
    draw_ball();
    
    // sometimes clears game, just for safety reason, those who know
    static int full_clear_counter = 0;
    full_clear_counter++;
    if (full_clear_counter >= 150) {  
        full_clear_counter = 0;
        gui_draw_rect(GAME_AREA_X, GAME_AREA_Y, GAME_AREA_WIDTH, GAME_AREA_HEIGHT, PONG_BG_COLOR);
        draw_center_line();
        draw_paddles();
        draw_ball();
    }
    
    // redraw scores every few frames
    static int score_redraw_counter = 0;
    score_redraw_counter++;
    if (score_redraw_counter >= 5) { 
        score_redraw_counter = 0;
        draw_scores();
    }
    // draw stuff if stuff
    pong_drawpause();
    pong_gameover();
    
    // update previous positions
    prev_ball = ball;
    prev_left_paddle = left_paddle;
    prev_right_paddle = right_paddle;
}

/**
 * draw
 */
void gui_draw_pong() {
    gui_init();
    pong_draw_window();
    pong_drawgame();
    
    pong_active = true;
    dialog_active = false;
    explorer_active = false;
    editor_active = false;
    terminal_active = false;
}

/**
 * input
 */
bool pong_handlekey(unsigned char key, char scancode) {
    if (!pong_active) return false;
    
    switch (scancode) {
        case ESC_KEY_CODE:
            // exit
            pong_active = false;
            dock_return();
            return true;
            
        case 0x11: // W key
            // up
            if (!game_paused && !game_over) {
                left_paddle.y -= PADDLE_SPEED;
                if (left_paddle.y < GAME_AREA_Y) {
                    left_paddle.y = GAME_AREA_Y;
                }
            }
            return true;
            
        case 0x1F: // S key
            // down
            if (!game_paused && !game_over) {
                left_paddle.y += PADDLE_SPEED;
                if (left_paddle.y + left_paddle.height > GAME_AREA_Y + GAME_AREA_HEIGHT) {
                    left_paddle.y = GAME_AREA_Y + GAME_AREA_HEIGHT - left_paddle.height;
                }
            }
            return true;
            
        case 0x39: // Space bar
            // Toggle pause (only if game is not over)
            if (!game_over) {
                game_paused = !game_paused;
                gui_draw_rect(GAME_AREA_X, GAME_AREA_Y, GAME_AREA_WIDTH, GAME_AREA_HEIGHT, PONG_BG_COLOR);
                draw_center_line();
                draw_scores();
                draw_paddles();
                draw_ball();
                pong_drawpause();
            }
            return true;
            
        case 0x13: // R key
            // restart game
            pong_init_game();
            gui_draw_pong();
            return true;
            
        default:
            return false;
    }
}


// Called from kernel or GUI to start Pong in single-task mode
void pong_start(void) {
    pong_init_game();
    gui_draw_pong();
}

// Pong update function for single-task mode
void pong_update(void) {
    static int frame_counter = 0;
    if (!pong_active || game_paused || game_over) return;
    frame_counter++;
    if (frame_counter >= 4) { // update every 8 frames (slower ball)
        frame_counter = 0;
        update_game();
        pong_drawgame();
    }
}

// Pong task for pre-emptive multitasking
void pong_task(void) {
    while (1) {
        if (pong_active && !game_paused && !game_over) {
            update_game();
            pong_drawgame();
        }
        // Let other tasks run
        // No need for delay, pre-emption will handle fairness
    }
}
