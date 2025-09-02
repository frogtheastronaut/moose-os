/**
 * mBASIC Interpreter for MooseOS
 * 
 * A simple line-by-line BASIC interpreter supporting common commands.
 * 
 * BASIC COMMAND REFERENCE:
 * 
 * PROGRAM STRUCTURE:
 * - Programs consist of numbered lines (10, 20, 30, etc.)
 * - Lines are executed sequentially unless redirected by GOTO/IF
 * - Line numbers allow for easy insertion and modification
 * 
 * VARIABLES:
 * - Single letter variables A-Z
 * - All variables are integers
 * - Automatically initialized to 0
 * 
 * BASIC COMMANDS:
 * 
 * LET variable = expression
 *   - Assigns a value to a variable
 *   - Example: LET A = 5
 *   - Example: LET B = A + 10
 *   - Note: LET keyword is optional in many BASIC dialects
 * 
 * PRINT expression|"string"
 *   - Outputs numbers, variables, or text strings
 *   - Example: PRINT "Hello World"
 *   - Example: PRINT A
 *   - Example: PRINT A + B
 *   - Automatically adds newline after output
 * 
 * INPUT variable
 *   - Prompts user for input and stores in variable
 *   - Example: INPUT A
 *   - Displays "? " prompt and waits for number entry
 * 
 * IF condition THEN statement
 *   - Conditional execution of statements
 *   - Example: IF A > 5 THEN PRINT "Big"
 *   - Example: IF A = 0 THEN GOTO 100
 *   - Supported operators: = (equal), < (less), > (greater)
 * 
 * GOTO line_number
 *   - Jumps program execution to specified line
 *   - Example: GOTO 50
 *   - Commonly used for loops and conditional branching
 * 
 * FOR variable = start TO end
 *   - Begins a counting loop
 *   - Example: FOR I = 1 TO 10
 *   - Variable increments by 1 each iteration
 *   - Must be paired with NEXT
 * 
 * NEXT variable
 *   - Marks end of FOR loop
 *   - Example: NEXT I
 *   - Returns to FOR line if counter hasn't reached end value
 * 
 * END
 *   - Terminates program execution
 *   - Example: END
 *   - Should be placed at logical program end
 * 
 * ARITHMETIC OPERATORS:
 * - + (addition)
 * - - (subtraction)  
 * - * (multiplication)
 * - / (division)
 * - ( ) (parentheses for grouping)
 * 
 * EXAMPLE PROGRAMS:
 * 
 * Hello World:
 * 10 PRINT "Hello, World!"
 * 20 END
 * 
 * Simple Calculator:
 * 10 PRINT "Enter two numbers:"
 * 20 INPUT A
 * 30 INPUT B
 * 40 PRINT "Sum is: "
 * 50 PRINT A + B
 * 60 END
 * 
 * Counting Loop:
 * 10 FOR I = 1 TO 10
 * 20 PRINT I
 * 30 NEXT I
 * 40 END
 * 
 * Conditional Logic:
 * 10 INPUT A
 * 20 IF A > 10 THEN GOTO 50
 * 30 PRINT "Small number"
 * 40 GOTO 60
 * 50 PRINT "Big number"
 * 60 END
 * 
 * Number Guessing Game:
 * 10 LET S = 7
 * 20 PRINT "Guess a number (1-10):"
 * 30 INPUT G
 * 40 IF G = S THEN GOTO 70
 * 50 PRINT "Wrong! Try again."
 * 60 GOTO 30
 * 70 PRINT "Correct!"
 * 80 END
 * 
 * PROGRAMMING CONVENTIONS:
 * - Use line numbers in increments of 10 (10, 20, 30...)
 * - This allows easy insertion of lines (15, 25, 35...)
 * - Use meaningful variable names within A-Z constraint
 * - Always end programs with END statement
 * - Use indentation for readability (though not required)
 * - Comment complex logic with PRINT statements for debugging
 * 
 * LIMITATIONS:
 * - Only integer arithmetic (no floating point)
 * - Single character variables only (A-Z)
 * - No string variables or string manipulation
 * - No arrays or complex data structures
 * - No subroutines or functions (GOSUB/RETURN)
 * - No file I/O operations
 * - Limited to 100 program lines maximum
 * - No nested FOR loops currently supported
 */

#ifndef MBASIC_INTERPRETER_H
#define MBASIC_INTERPRETER_H

/**
 * Run a BASIC program from a file in the current directory
 * Usage: basic filename.bas
 * 
 * @param filename Name of the BASIC file to execute
 */
void basic_run_file(const char* filename);

#endif // MBASIC_INTERPRETER_H