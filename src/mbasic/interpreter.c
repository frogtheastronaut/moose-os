/**
 * mBASIC interpreter
 * 
 * BASIC interpreter for MooseOS - runs as a terminal command
 * Usage: basic <filename.bas>
 */

#include "interpreter.h"
#include "../gui/include/gui.h"
#include "../gui/include/terminal.h"
#include "../filesys/file.h"
#include "../lib/lib.h"

//=============================================================================
// CONSTANTS AND CONFIGURATION
//=============================================================================

#define MAX_PROGRAM_LINES 100
#define MAX_LINE_LENGTH 80
#define MAX_VARIABLES 26  // A-Z
#define MAX_FOR_STACK 10

//=============================================================================
// TYPE DEFINITIONS
//=============================================================================

typedef enum {
    TOKEN_NUMBER,
    TOKEN_VARIABLE,
    TOKEN_STRING,
    TOKEN_OPERATOR,
    TOKEN_KEYWORD,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef enum {
    KW_PRINT,
    KW_LET,
    KW_INPUT,
    KW_IF,
    KW_THEN,
    KW_GOTO,
    KW_FOR,
    KW_TO,
    KW_NEXT,
    KW_END,
    KW_UNKNOWN
} Keyword;

typedef struct {
    TokenType type;
    union {
        int number;
        char variable;
        char string[MAX_LINE_LENGTH];
        char operator;
        Keyword keyword;
    };
} Token;

typedef struct {
    int line_number;
    char code[MAX_LINE_LENGTH];
} ProgramLine;

typedef struct {
    char variable;
    int start_value;
    int end_value;
    int step;
    int return_line;
} ForLoop;

// Command handler function pointer type
typedef void (*CommandHandler)(void);

typedef struct {
    const char* name;
    Keyword keyword;
    CommandHandler handler;
} CommandInfo;

//=============================================================================
// GLOBAL STATE
//=============================================================================

static ProgramLine program[MAX_PROGRAM_LINES];
static int program_size = 0;
static int variables[MAX_VARIABLES]; // A=0, B=1, ..., Z=25
static int current_line = 0;
static bool running = false;
static ForLoop for_stack[MAX_FOR_STACK];
static int for_stack_top = -1;

// Parser state
static char* parse_pos;
static Token current_token;

//=============================================================================
// FORWARD DECLARATIONS
//=============================================================================

// System integration
static void basic_print_string(const char* str);
static void basic_print_number(int num);
static int basic_input_number(void);
static void basic_show_error(const char* error);

// Core system
static void init_interpreter(void);
static bool load_basic_file(const char* filename);

// Tokenizer
static void tokenize_next(void);
static void skip_whitespace(void);
static Keyword identify_keyword(void);

// Expression parser
static int parse_expression(void);
static int parse_term(void);
static int parse_factor(void);

// Command handlers
static void cmd_print(void);
static void cmd_let(void);
static void cmd_input(void);
static void cmd_if(void);
static void cmd_goto(void);
static void cmd_for(void);
static void cmd_next(void);
static void cmd_end(void);

// Utilities
static bool is_digit(char c);
static bool is_alpha(char c);
static int string_to_int(const char* str);
static void int_to_string(int num, char* str);
static bool match_keyword(const char* keyword, int length);
static int find_line(int line_number);
static void execute_line(int line_index);
static CommandInfo* find_command(Keyword kw);

//=============================================================================
// COMMAND REGISTRY
//=============================================================================

static CommandInfo command_registry[] = {
    {"PRINT",  KW_PRINT,  cmd_print},
    {"LET",    KW_LET,    cmd_let},
    {"INPUT",  KW_INPUT,  cmd_input},
    {"IF",     KW_IF,     cmd_if},
    {"GOTO",   KW_GOTO,   cmd_goto},
    {"FOR",    KW_FOR,    cmd_for},
    {"TO",     KW_TO,     NULL},      // Special token, no handler
    {"NEXT",   KW_NEXT,   cmd_next},
    {"THEN",   KW_THEN,   NULL},      // Special token, no handler
    {"END",    KW_END,    cmd_end},
    {NULL,     KW_UNKNOWN, NULL}      // Sentinel
};

//=============================================================================
// SYSTEM INTEGRATION FUNCTIONS
//=============================================================================

static void basic_print_string(const char* str) {
    terminal_print(str);
}

static void basic_print_number(int num) {
    char str[32];
    int_to_string(num, str);
    basic_print_string(str);
}

static int basic_input_number(void) {
    // For now, return 0 - in full implementation would read from terminal input
    basic_print_string("? ");
    // TODO: Implement actual input reading from terminal
    return 0;
}

static void basic_show_error(const char* error) {
    basic_print_string("BASIC ERROR: ");
    basic_print_string(error);
    basic_print_string("\n");
}

//=============================================================================
// CORE SYSTEM FUNCTIONS
//=============================================================================

static void init_interpreter(void) {
    program_size = 0;
    current_line = 0;
    running = false;
    for_stack_top = -1;
    
    // Initialize variables to 0
    for (int i = 0; i < MAX_VARIABLES; i++) {
        variables[i] = 0;
    }
    
    // Clear program
    for (int i = 0; i < MAX_PROGRAM_LINES; i++) {
        program[i].line_number = 0;
        program[i].code[0] = '\0';
    }
}

static bool load_basic_file(const char* filename) {
    // Find the file in the current directory
    if (!cwd || !cwd->folder.children) {
        basic_show_error("No files in current directory");
        return false;
    }
    
    File* basic_file = NULL;
    for (int i = 0; i < cwd->folder.childCount; i++) {
        if (!cwd->folder.children[i]) continue;
        
        File* child = cwd->folder.children[i];
        if (child->type == FILE_NODE && strEqual(child->name, filename)) {
            basic_file = child;
            break;
        }
    }
    
    if (!basic_file) {
        basic_show_error("File not found");
        return false;
    }
    
    if (!basic_file->file.content) {
        basic_show_error("File is empty");
        return false;
    }
    
    // Parse file content into program lines
    char* content = basic_file->file.content;
    char line_buffer[MAX_LINE_LENGTH];
    int content_pos = 0;
    int line_buffer_pos = 0;
    
    program_size = 0;
    
    while (content_pos < basic_file->file.content_size && program_size < MAX_PROGRAM_LINES) {
        char c = content[content_pos++];
        
        if (c == '\n' || c == '\r' || content_pos >= basic_file->file.content_size) {
            // End of line - process it
            line_buffer[line_buffer_pos] = '\0';
            
            if (line_buffer_pos > 0) {
                // Parse line number and code
                int line_num = 0;
                int i = 0;
                
                // Skip whitespace
                while (i < line_buffer_pos && (line_buffer[i] == ' ' || line_buffer[i] == '\t')) i++;
                
                // Parse line number
                while (i < line_buffer_pos && is_digit(line_buffer[i])) {
                    line_num = line_num * 10 + (line_buffer[i] - '0');
                    i++;
                }
                
                // Skip whitespace after line number
                while (i < line_buffer_pos && (line_buffer[i] == ' ' || line_buffer[i] == '\t')) i++;
                
                if (line_num > 0 && i < line_buffer_pos) {
                    // Store the line
                    program[program_size].line_number = line_num;
                    int j = 0;
                    while (i < line_buffer_pos && j < MAX_LINE_LENGTH - 1) {
                        program[program_size].code[j++] = line_buffer[i++];
                    }
                    program[program_size].code[j] = '\0';
                    program_size++;
                }
            }
            
            line_buffer_pos = 0;
        } else {
            if (line_buffer_pos < MAX_LINE_LENGTH - 1) {
                line_buffer[line_buffer_pos++] = c;
            }
        }
    }
    
    return program_size > 0;
}

static CommandInfo* find_command(Keyword kw) {
    for (int i = 0; command_registry[i].name != NULL; i++) {
        if (command_registry[i].keyword == kw) {
            return &command_registry[i];
        }
    }
    return NULL;
}

//=============================================================================
// UTILITY FUNCTIONS
//=============================================================================

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static void skip_whitespace(void) {
    while (*parse_pos == ' ' || *parse_pos == '\t') {
        parse_pos++;
    }
}

static int string_to_int(const char* str) {
    int result = 0;
    bool negative = false;
    
    while (*str == ' ') str++; // Skip whitespace
    
    if (*str == '-') {
        negative = true;
        str++;
    }
    
    while (is_digit(*str)) {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return negative ? -result : result;
}

static void int_to_string(int num, char* str) {
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    bool negative = false;
    if (num < 0) {
        negative = true;
        num = -num;
    }
    
    char temp[20];
    int i = 0;
    
    while (num > 0) {
        temp[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    int j = 0;
    if (negative) {
        str[j++] = '-';
    }
    
    while (i > 0) {
        str[j++] = temp[--i];
    }
    str[j] = '\0';
}

static bool match_keyword(const char* keyword, int length) {
    for (int i = 0; i < length; i++) {
        char c1 = parse_pos[i];
        char c2 = keyword[i];
        
        // Convert to uppercase for comparison
        if (c1 >= 'a' && c1 <= 'z') c1 = c1 - 'a' + 'A';
        if (c2 >= 'a' && c2 <= 'z') c2 = c2 - 'a' + 'A';
        
        if (c1 != c2) return false;
    }
    
    char next_char = parse_pos[length];
    return (next_char == ' ' || next_char == '\0' || next_char == '\n' || next_char == '\r');
}

static int find_line(int line_number) {
    for (int i = 0; i < program_size; i++) {
        if (program[i].line_number == line_number) {
            return i;
        }
    }
    return -1;
}

//=============================================================================
// TOKENIZER
//=============================================================================

static Keyword identify_keyword(void) {
    // Check against command registry
    for (int i = 0; command_registry[i].name != NULL; i++) {
        const char* name = command_registry[i].name;
        int length = 0;
        
        // Calculate length
        while (name[length]) length++;
        
        if (match_keyword(name, length)) {
            parse_pos += length;
            return command_registry[i].keyword;
        }
    }
    
    return KW_UNKNOWN;
}

static void tokenize_next(void) {
    skip_whitespace();
    
    if (*parse_pos == '\0' || *parse_pos == '\n' || *parse_pos == '\r') {
        current_token.type = TOKEN_EOF;
        return;
    }
    
    // Numbers
    if (is_digit(*parse_pos)) {
        current_token.type = TOKEN_NUMBER;
        current_token.number = 0;
        
        while (is_digit(*parse_pos)) {
            current_token.number = current_token.number * 10 + (*parse_pos - '0');
            parse_pos++;
        }
        return;
    }
    
    // Keywords and variables
    if (is_alpha(*parse_pos)) {
        Keyword kw = identify_keyword();
        
        if (kw != KW_UNKNOWN) {
            current_token.type = TOKEN_KEYWORD;
            current_token.keyword = kw;
            return;
        }
        
        // Single letter variable
        char c = *parse_pos;
        if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
        
        if (c >= 'A' && c <= 'Z') {
            current_token.type = TOKEN_VARIABLE;
            current_token.variable = c;
            parse_pos++;
            return;
        }
    }
    
    // String literals
    if (*parse_pos == '"') {
        current_token.type = TOKEN_STRING;
        parse_pos++; // Skip opening quote
        
        int i = 0;
        while (*parse_pos != '"' && *parse_pos != '\0' && i < MAX_LINE_LENGTH - 1) {
            current_token.string[i++] = *parse_pos++;
        }
        current_token.string[i] = '\0';
        
        if (*parse_pos == '"') parse_pos++; // Skip closing quote
        return;
    }
    
    // Operators and special characters
    current_token.type = TOKEN_OPERATOR;
    current_token.operator = *parse_pos++;
}

//=============================================================================
// EXPRESSION PARSER
//=============================================================================

static int parse_expression(void) {
    int result = parse_term();
    
    while (current_token.type == TOKEN_OPERATOR && 
           (current_token.operator == '+' || current_token.operator == '-')) {
        char op = current_token.operator;
        tokenize_next();
        int right = parse_term();
        
        if (op == '+') {
            result += right;
        } else {
            result -= right;
        }
    }
    
    return result;
}

static int parse_term(void) {
    int result = parse_factor();
    
    while (current_token.type == TOKEN_OPERATOR && 
           (current_token.operator == '*' || current_token.operator == '/')) {
        char op = current_token.operator;
        tokenize_next();
        int right = parse_factor();
        
        if (op == '*') {
            result *= right;
        } else if (right != 0) {
            result /= right;
        } else {
            basic_show_error("Division by zero");
            return 0;
        }
    }
    
    return result;
}

static int parse_factor(void) {
    if (current_token.type == TOKEN_NUMBER) {
        int value = current_token.number;
        tokenize_next();
        return value;
    }
    
    if (current_token.type == TOKEN_VARIABLE) {
        char var = current_token.variable;
        tokenize_next();
        return variables[var - 'A'];
    }
    
    if (current_token.type == TOKEN_OPERATOR && current_token.operator == '(') {
        tokenize_next(); // Skip '('
        int result = parse_expression();
        if (current_token.type == TOKEN_OPERATOR && current_token.operator == ')') {
            tokenize_next(); // Skip ')'
        }
        return result;
    }
    
    return 0;
}

//=============================================================================
// COMMAND HANDLERS
//=============================================================================

static void cmd_print(void) {
    tokenize_next(); // Skip PRINT
    
    if (current_token.type == TOKEN_STRING) {
        basic_print_string(current_token.string);
        tokenize_next();
    } else {
        int value = parse_expression();
        basic_print_number(value);
    }
    
    basic_print_string("\n");
}

static void cmd_let(void) {
    tokenize_next(); // Skip LET
    
    if (current_token.type == TOKEN_VARIABLE) {
        char var = current_token.variable;
        tokenize_next();
        
        if (current_token.type == TOKEN_OPERATOR && current_token.operator == '=') {
            tokenize_next(); // Skip '='
            int value = parse_expression();
            variables[var - 'A'] = value;
        } else {
            basic_show_error("Expected '=' in LET statement");
        }
    } else {
        basic_show_error("Expected variable in LET statement");
    }
}

static void cmd_input(void) {
    tokenize_next(); // Skip INPUT
    
    if (current_token.type == TOKEN_VARIABLE) {
        char var = current_token.variable;
        int value = basic_input_number();
        variables[var - 'A'] = value;
        tokenize_next();
    } else {
        basic_show_error("Expected variable in INPUT statement");
    }
}

static void cmd_if(void) {
    tokenize_next(); // Skip IF
    
    int left = parse_expression();
    
    if (current_token.type == TOKEN_OPERATOR) {
        char op = current_token.operator;
        tokenize_next();
        int right = parse_expression();
        
        bool condition = false;
        switch (op) {
            case '=': condition = (left == right); break;
            case '<': condition = (left < right); break;
            case '>': condition = (left > right); break;
            default:
                basic_show_error("Invalid comparison operator");
                return;
        }
        
        if (current_token.type == TOKEN_KEYWORD && current_token.keyword == KW_THEN) {
            tokenize_next(); // Skip THEN
            
            if (condition) {
                // Execute the rest of the line
                if (current_token.type == TOKEN_KEYWORD) {
                    CommandInfo* cmd = find_command(current_token.keyword);
                    if (cmd && cmd->handler) {
                        cmd->handler();
                    }
                }
            }
        } else {
            basic_show_error("Expected THEN after IF condition");
        }
    } else {
        basic_show_error("Expected comparison operator after IF");
    }
}

static void cmd_goto(void) {
    tokenize_next(); // Skip GOTO
    
    if (current_token.type == TOKEN_NUMBER) {
        int line_num = current_token.number;
        int line_index = find_line(line_num);
        if (line_index >= 0) {
            current_line = line_index - 1; // Will be incremented in main loop
        } else {
            basic_show_error("Line number not found");
        }
    } else {
        basic_show_error("Expected line number after GOTO");
    }
}

static void cmd_for(void) {
    tokenize_next(); // Skip FOR
    
    if (current_token.type == TOKEN_VARIABLE && for_stack_top < MAX_FOR_STACK - 1) {
        char var = current_token.variable;
        tokenize_next();
        
        if (current_token.type == TOKEN_OPERATOR && current_token.operator == '=') {
            tokenize_next(); // Skip '='
            int start_val = parse_expression();
            
            if (current_token.type == TOKEN_KEYWORD && current_token.keyword == KW_TO) {
                tokenize_next(); // Skip TO
                int end_val = parse_expression();
                
                // Push onto FOR stack
                for_stack_top++;
                for_stack[for_stack_top].variable = var;
                for_stack[for_stack_top].start_value = start_val;
                for_stack[for_stack_top].end_value = end_val;
                for_stack[for_stack_top].step = 1;
                for_stack[for_stack_top].return_line = current_line;
                
                variables[var - 'A'] = start_val;
            } else {
                basic_show_error("Expected TO in FOR statement");
            }
        } else {
            basic_show_error("Expected '=' in FOR statement");
        }
    } else {
        if (for_stack_top >= MAX_FOR_STACK - 1) {
            basic_show_error("FOR stack overflow");
        } else {
            basic_show_error("Expected variable in FOR statement");
        }
    }
}

static void cmd_next(void) {
    tokenize_next(); // Skip NEXT
    
    if (for_stack_top >= 0) {
        ForLoop* loop = &for_stack[for_stack_top];
        char var = loop->variable;
        
        variables[var - 'A'] += loop->step;
        
        if (variables[var - 'A'] <= loop->end_value) {
            current_line = loop->return_line - 1; // Will be incremented
        } else {
            for_stack_top--; // Pop from stack
        }
    } else {
        basic_show_error("NEXT without FOR");
    }
}

static void cmd_end(void) {
    running = false;
}

static void execute_line(int line_index) {
    if (line_index < 0 || line_index >= program_size) return;
    
    parse_pos = program[line_index].code;
    tokenize_next();
    
    if (current_token.type == TOKEN_KEYWORD) {
        CommandInfo* cmd = find_command(current_token.keyword);
        if (cmd && cmd->handler) {
            cmd->handler();
        }
    }
}

//=============================================================================
// PUBLIC API FUNCTIONS
//=============================================================================

void basic_run_file(const char* filename) {
    init_interpreter();
    
    if (!load_basic_file(filename)) {
        return; // Error already printed
    }
    
    basic_print_string("Running BASIC program: ");
    basic_print_string(filename);
    basic_print_string("\n");
    
    current_line = 0;
    running = true;
    for_stack_top = -1;
    
    // Initialize all variables to 0
    for (int i = 0; i < MAX_VARIABLES; i++) {
        variables[i] = 0;
    }
    
    while (running && current_line < program_size) {
        execute_line(current_line);
        current_line++;
    }
    
    basic_print_string("Program completed.\n");
}
