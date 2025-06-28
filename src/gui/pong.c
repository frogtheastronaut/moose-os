#include "gui.h"
#include "../kernel/include/keyboard.h"
#include "../kernel/include/keydef.h"
#include "../lib/lib.h"

// =============================================================================
// CONSTANTS
// =============================================================================

#define PONG_WIDTH 300
#define PONG_HEIGHT 180
#define PONG_X ((SCREEN_WIDTH - PONG_WIDTH) / 2)
#define PONG_Y ((SCREEN_HEIGHT - PONG_HEIGHT) / 2)

// Game area
#define GAME_AREA_X (PONG_X + 8)
#define GAME_AREA_Y (PONG_Y + 28)
#define GAME_AREA_WIDTH (PONG_WIDTH - 16)
#define GAME_AREA_HEIGHT (PONG_HEIGHT - 36)

// Paddle 
#define PADDLE_WIDTH 4
#define PADDLE_HEIGHT 15
#define PADDLE_SPEED 2

// Ball 
#define BALL_SIZE 3
#define BALL_SPEED 1

// Game settings
#define WIN_SCORE 11  // First to 11 wins

// Colors
#define PONG_BG_COLOR VGA_COLOR_BLACK
#define PADDLE_COLOR VGA_COLOR_WHITE
#define BALL_COLOR VGA_COLOR_WHITE
#define SCORE_COLOR VGA_COLOR_WHITE

typedef struct {
    int x, y;
    int width, height;
} Paddle;

typedef struct {
    int x, y;
    int dx, dy;
    int size;
} Ball;

// Variables
static Paddle left_paddle;
static Paddle right_paddle;
static Ball ball;
static int left_score = 0;
static int right_score = 0;
static bool pong_active = false;
static bool game_paused = false; // Start unpaused
static bool game_over = false;

static int winner = 0; // 1 for left player, 2 for right player
static int frame_counter = 0;
// AI delay counter
static int ai_delay_counter = 0;
static bool ai_wait_for_first_hit = true;

// Previous positions 
static Ball prev_ball;
static Paddle prev_left_paddle;
static Paddle prev_right_paddle;

// External variables
extern bool dialog_active;
extern bool explorer_active;
extern bool editor_active;
extern bool terminal_active;
extern void dock_return(void);

static void draw_pong_game_optimized();

/**
 * Initialize Pong
 */
static void pong_init_game() {
    // Initialize left paddle
    left_paddle.x = GAME_AREA_X + 10;
    left_paddle.y = GAME_AREA_Y + (GAME_AREA_HEIGHT - PADDLE_HEIGHT) / 2;
    left_paddle.width = PADDLE_WIDTH;
    left_paddle.height = PADDLE_HEIGHT;
    
    // Initialize right paddle
    right_paddle.x = GAME_AREA_X + GAME_AREA_WIDTH - 10 - PADDLE_WIDTH;
    right_paddle.y = GAME_AREA_Y + (GAME_AREA_HEIGHT - PADDLE_HEIGHT) / 2;
    right_paddle.width = PADDLE_WIDTH;
    right_paddle.height = PADDLE_HEIGHT;
    
    // Initialize ball
    ball.x = GAME_AREA_X + GAME_AREA_WIDTH / 2;
    ball.y = GAME_AREA_Y + GAME_AREA_HEIGHT / 2;
    ball.dx = -BALL_SPEED; // Always serve to the user (right paddle)
    ball.dy = -BALL_SPEED;
    ball.size = BALL_SIZE;
    
    // Initialize previous positions
    prev_ball = ball;
    prev_left_paddle = left_paddle;
    prev_right_paddle = right_paddle;

    // Reset AI delay countert
    ai_delay_counter = 0;
    // Make sure AI waits for first hit
    ai_wait_for_first_hit = true;

    // Reset scores
    left_score = 0;
    right_score = 0;
    game_paused = false; // Make sure game is NOT paused
    game_over = false;
    winner = 0;
    frame_counter = 0;
}

/**
 * Reset ball to center
 */
static void reset_ball() {
    ball.x = GAME_AREA_X + GAME_AREA_WIDTH / 2;
    ball.y = GAME_AREA_Y + GAME_AREA_HEIGHT / 2;
    
    // Always serve to the user (right paddle)
    ball.dx = -BALL_SPEED;
    ball.dy = -BALL_SPEED;

    // Reset AI delay counter and wait flag so AI does not move until first hit
    ai_delay_counter = 0;
    ai_wait_for_first_hit = true;
}

/**
 * Check collision between ball and paddle
 */
static bool check_paddle_collision(Paddle* paddle) {
    return (ball.x < paddle->x + paddle->width &&
            ball.x + ball.size > paddle->x &&
            ball.y < paddle->y + paddle->height &&
            ball.y + ball.size > paddle->y);
}

/**
 * Update game logic
 */
static void update_game() {
    if (game_paused || game_over) return;
    
    // Move ball
    ball.x += ball.dx;
    ball.y += ball.dy;
    
    // Ball collision with top/bottom walls
    if (ball.y <= GAME_AREA_Y || ball.y + ball.size >= GAME_AREA_Y + GAME_AREA_HEIGHT) {
        ball.dy = -ball.dy;
    }
    
    // Ball collision with paddles
    if (check_paddle_collision(&left_paddle) && ball.dx < 0) {
        ball.dx = -ball.dx;
        ball.x = left_paddle.x + left_paddle.width; // Prevent sticking
        ai_wait_for_first_hit = false; // AI can start moving after first hit
    }

    if (check_paddle_collision(&right_paddle) && ball.dx > 0) {
        ball.dx = -ball.dx;
        ball.x = right_paddle.x - ball.size; // Prevent sticking
        ai_wait_for_first_hit = false; // AI can start moving after first hit
    }
    
    // Ball out of bounds (scoring)
    if (ball.x < GAME_AREA_X) {
        right_score++;
        if (right_score >= WIN_SCORE) {
            game_over = true;
            winner = 2; // Right player wins
        } else {
            reset_ball();
        }
    } else if (ball.x > GAME_AREA_X + GAME_AREA_WIDTH) {
        left_score++;
        if (left_score >= WIN_SCORE) {
            game_over = true;
            winner = 1; // Left player wins
        } else {
            reset_ball();
        }
    }
    
    // Slightly better AI for right paddle - always moves but still beatable
    int paddle_center = right_paddle.y + right_paddle.height / 2;
    int ball_center = ball.y + ball.size / 2;

    // AI only moves after first hit
    if (!ai_wait_for_first_hit) {
        ai_delay_counter++;

        // AI reacts every 2nd frame (slightly better than every 3rd)
        if (ai_delay_counter >= 2) {
            ai_delay_counter = 0;  // Reset counter

            // Moderate dead zone and reasonable movement speed
            if (ball_center < paddle_center - 8) {  // Reduced dead zone to 8 pixels (was 15)
                right_paddle.y -= 1;  // Still slower than player (PADDLE_SPEED = 2)
            } else if (ball_center > paddle_center + 8) {  // Reduced dead zone to 8 pixels (was 15)
                right_paddle.y += 1;  // Still slower than player (PADDLE_SPEED = 2)
            }
        }
    }


    
    // Keep right paddle in bounds
    if (right_paddle.y < GAME_AREA_Y) {
        right_paddle.y = GAME_AREA_Y;
    }
    if (right_paddle.y + right_paddle.height > GAME_AREA_Y + GAME_AREA_HEIGHT) {
        right_paddle.y = GAME_AREA_Y + GAME_AREA_HEIGHT - right_paddle.height;
    }
}

/**
 * Draw game window
 */
static void draw_pong_window() {
    // Clear screen
    gui_clear_screen(VGA_COLOR_LIGHT_GREY);
    
    // Draw window border (file explorer style)
    gui_draw_window_box(PONG_X, PONG_Y, PONG_WIDTH, PONG_HEIGHT,
                       VGA_COLOR_BLACK, VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY);
    
    // Draw title bar
    gui_draw_title_bar(PONG_X, PONG_Y, PONG_WIDTH, 15, VGA_COLOR_BLUE);
    gui_draw_text(PONG_X + 5, PONG_Y + 3, "MooseOS Pong", VGA_COLOR_WHITE);
    
    // Draw game area background
    gui_draw_rect(GAME_AREA_X, GAME_AREA_Y, GAME_AREA_WIDTH, GAME_AREA_HEIGHT, PONG_BG_COLOR);
    gui_draw_rect_outline(GAME_AREA_X - 1, GAME_AREA_Y - 1, GAME_AREA_WIDTH + 2, GAME_AREA_HEIGHT + 2, VGA_COLOR_WHITE);
}

/**
 * Draw center line
 */
static void draw_center_line() {
    int center_x = GAME_AREA_X + GAME_AREA_WIDTH / 2;
    for (int y = GAME_AREA_Y; y < GAME_AREA_Y + GAME_AREA_HEIGHT; y += 8) {
        gui_draw_rect(center_x - 1, y, 2, 4, VGA_COLOR_DARK_GREY);
    }
}

/**
 * Draw paddles
 */
static void draw_paddles() {
    // Draw left paddle
    gui_draw_rect(left_paddle.x, left_paddle.y, left_paddle.width, left_paddle.height, PADDLE_COLOR);
    
    // Draw right paddle
    gui_draw_rect(right_paddle.x, right_paddle.y, right_paddle.width, right_paddle.height, PADDLE_COLOR);
}

/**
 * Draw ball
 */
static void draw_ball() {
    gui_draw_rect(ball.x, ball.y, ball.size, ball.size, BALL_COLOR);
}

/**
 * Draw scores
 */
static void draw_scores() {
    char left_score_str[4];  // Support up to 2 digits + null terminator
    char right_score_str[4];
    
    // Convert scores to strings (properly handle double digits)
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
    
    // Always clear score areas completely at the top of game area - use title bar blue color
    gui_draw_rect(GAME_AREA_X + GAME_AREA_WIDTH / 4 - 15, GAME_AREA_Y + 2, 30, 12, VGA_COLOR_BLACK);
    gui_draw_rect(GAME_AREA_X + 3 * GAME_AREA_WIDTH / 4 - 15, GAME_AREA_Y + 2, 30, 12, VGA_COLOR_BLACK);
    
    // Draw scores at the top of the game area
    gui_draw_text(GAME_AREA_X + GAME_AREA_WIDTH / 4, GAME_AREA_Y + 5, left_score_str, SCORE_COLOR);
    gui_draw_text(GAME_AREA_X + 3 * GAME_AREA_WIDTH / 4, GAME_AREA_Y + 5, right_score_str, SCORE_COLOR);
}

/**
 * Draw pause message
 */
static void draw_pause_message() {
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
 * Draw game over message
 */
static void draw_game_over_message() {
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
        
        // Draw background for text
        gui_draw_rect(GAME_AREA_X + 20, msg_y1 - 5, GAME_AREA_WIDTH - 40, 40, VGA_COLOR_DARK_GREY);
        
        // Draw messages
        gui_draw_text(msg_x1, msg_y1, game_over_msg, VGA_COLOR_RED);
        gui_draw_text(msg_x2, msg_y2, winner_msg, VGA_COLOR_LIGHT_BROWN);
        gui_draw_text(msg_x3, msg_y3, restart_msg, VGA_COLOR_WHITE);
    }
}

/**
 * Main drawing function
 */
static void draw_pong_game() {
    draw_pong_window();
    draw_center_line();
    draw_scores();
    draw_paddles();
    draw_ball();
    draw_pause_message();
    draw_game_over_message();
}

/**
 * Optimized drawing function that only redraws the game area
 */
static void draw_pong_game_optimized() {
    // Clear larger areas around previous positions but stay within bounds
    int clear_x = prev_ball.x - 3;  // Increased margin
    int clear_y = prev_ball.y - 3;
    int clear_width = prev_ball.size + 6;  // Increased margin
    int clear_height = prev_ball.size + 6;
    
    // Clamp to game area boundaries
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
    
    // Only clear if within bounds
    if (clear_width > 0 && clear_height > 0) {
        gui_draw_rect(clear_x, clear_y, clear_width, clear_height, PONG_BG_COLOR);
    }
    
    // Clear paddle areas with boundary checks
    // Left paddle
    clear_x = prev_left_paddle.x - 3;  // Increased margin
    clear_y = prev_left_paddle.y - 3;
    clear_width = prev_left_paddle.width + 6;  // Increased margin
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
    
    // Right paddle
    clear_x = prev_right_paddle.x - 3;  // Increased margin
    clear_y = prev_right_paddle.y - 3;
    clear_width = prev_right_paddle.width + 6;  // Increased margin
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
    
    // Always redraw the entire center line to prevent corruption
    draw_center_line();
    
    // Draw current positions
    draw_paddles();
    draw_ball();
    
    // Periodically clear the entire game area to prevent trail accumulation
    static int full_clear_counter = 0;
    full_clear_counter++;
    if (full_clear_counter >= 150) {  // Even less frequent full clears
        full_clear_counter = 0;
        gui_draw_rect(GAME_AREA_X, GAME_AREA_Y, GAME_AREA_WIDTH, GAME_AREA_HEIGHT, PONG_BG_COLOR);
        draw_center_line();
        draw_paddles();
        draw_ball();
    }
    
    // Redraw scores every few frames to prevent corruption but reduce lag
    static int score_redraw_counter = 0;
    score_redraw_counter++;
    if (score_redraw_counter >= 5) {  // Redraw scores every 5 frames
        score_redraw_counter = 0;
        draw_scores();
    }
    
    draw_pause_message();
    draw_game_over_message();
    
    // Update previous positions
    prev_ball = ball;
    prev_left_paddle = left_paddle;
    prev_right_paddle = right_paddle;
}

// =============================================================================
// PUBLIC INTERFACE
// =============================================================================

/**
 * Main pong drawing function
 */
void gui_draw_pong() {
    gui_init();
    draw_pong_game();
    
    pong_active = true;
    dialog_active = false;
    explorer_active = false;
    editor_active = false;
    terminal_active = false;
}

/**
 * Handle pong keyboard input
 */
bool gui_handle_pong_key(unsigned char key, char scancode) {
    if (!pong_active) return false;
    
    switch (scancode) {
        case ESC_KEY_CODE:
            // Exit pong and return to dock
            pong_active = false;
            dock_return();
            return true;
            
        case 0x11: // W key
            // Move left paddle up
            if (!game_paused && !game_over) {
                left_paddle.y -= PADDLE_SPEED;
                if (left_paddle.y < GAME_AREA_Y) {
                    left_paddle.y = GAME_AREA_Y;
                }
            }
            return true;
            
        case 0x1F: // S key
            // Move left paddle down
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
                // Force full redraw for pause message
                gui_draw_rect(GAME_AREA_X, GAME_AREA_Y, GAME_AREA_WIDTH, GAME_AREA_HEIGHT, PONG_BG_COLOR);
                draw_center_line();
                draw_scores();
                draw_paddles();
                draw_ball();
                draw_pause_message();
            }
            return true;
            
        case 0x13: // R key
            // Restart game
            pong_init_game();
            gui_draw_pong();
            return true;
            
        default:
            return false;
    }
}

/**
 * Update pong game (call this from main loop)
 */
void pong_update() {
    if (pong_active) {
        if (!game_paused && !game_over) {
            frame_counter++;
            
            // Only update every 30th frame to slow down the game and reduce lag
            if (frame_counter >= 30) {
                frame_counter = 0;
                
                // Update game logic
                update_game();
                
                // Use optimized drawing to reduce screen corruption
                draw_pong_game_optimized();
            }
        } else {
            // Game is paused or over, still draw occasionally to show messages
            static int static_frame_counter = 0;
            static_frame_counter++;
            if (static_frame_counter >= 30) {
                static_frame_counter = 0;
                draw_pong_game_optimized();
            }
        }
    }
}

/**
 * Initialize pong app
 */
void pong_app_init() {
    pong_init_game();
}

/**
 * Check if pong is active
 */
bool pong_is_active() {
    return pong_active;
}

/**
 * Open pong application
 */
void gui_open_pong() {
    pong_app_init();
    gui_draw_pong();
}
