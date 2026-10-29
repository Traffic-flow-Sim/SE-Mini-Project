#ifndef INPUT_H
#define INPUT_H

#include <stddef.h>
#include <stdio.h>

/* Longest accepted input line, including the terminator. Longer lines are rejected. */
#define INPUT_MAX_LINE 64

typedef enum {
    INPUT_OK = 0,
    INPUT_NOT_A_NUMBER,
    INPUT_OUT_OF_RANGE
} InputStatus;

/*
 * Reads one line into buf with the newline (and any CR) removed. Never writes
 * more than size bytes.
 * Returns 1 on success, 0 if the line was too long (the rest of that line is
 * discarded so the next read starts cleanly), -1 on end of input or bad arguments.
 */
int read_line_bounded(FILE *in, char *buf, size_t size);

/*
 * Strict whole-number parse: surrounding spaces are allowed; anything else
 * (empty text, letters, decimals, trailing characters) is INPUT_NOT_A_NUMBER.
 * A number that is outside [min, max], including one too large for an int,
 * is INPUT_OUT_OF_RANGE. *out is written only on INPUT_OK.
 */
InputStatus parse_int_strict(const char *text, int min, int max, int *out);

/*
 * Prints the prompt and re-asks until the user enters a whole number in
 * [min, max]. Returns 1 on success, -1 if input ends or arguments are invalid.
 */
int prompt_int(FILE *in, FILE *out, const char *prompt,
               int min, int max, int *value);

#endif
