
#include "input.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int read_line_bounded(FILE *in, char *buf, size_t size)
{
    size_t length;
    int c;

    if (in == NULL || buf == NULL || size < 2) {
        return -1;
    }

    if (fgets(buf, (int)size, in) == NULL) {
        buf[0] = '\0';
        return -1;
    }

    length = strlen(buf);

    /* A complete line was read, including its newline. */
    if (length > 0 && buf[length - 1] == '\n') {
        buf[--length] = '\0';

        /* Remove a carriage return from Windows-style line endings. */
        if (length > 0 && buf[length - 1] == '\r') {
            buf[length - 1] = '\0';
        }

        return 1;
    }

    /*
     * No newline was read. Check the next character to determine
     * whether the input line ended or exceeded the buffer.
     */
    c = getc(in);

    if (c == EOF) {
        /* The final line can end without a newline. */
        return 1;
    }

    if (c == '\n') {
        /* The input exactly filled the buffer before its newline. */
        return 1;
    }

    /* The line is too long. Discard the remaining characters. */
    while (c != '\n' && c != EOF) {
        c = getc(in);
    }

    return 0;
}

InputStatus parse_int_strict(const char *text, int min, int max, int *out)
{
    char *end;
    long value;

    if (text == NULL || out == NULL) {
        return INPUT_NOT_A_NUMBER;
    }

    while (isspace((unsigned char)*text)) {
        text++;
    }

    if (*text == '\0') {
        return INPUT_NOT_A_NUMBER;
    }

    errno = 0;
    value = strtol(text, &end, 10);

    if (end == text) {
        return INPUT_NOT_A_NUMBER;
    }

    while (isspace((unsigned char)*end)) {
        end++;
    }

    if (*end != '\0') {
        return INPUT_NOT_A_NUMBER;
    }

    if (errno == ERANGE || value < (long)min || value > (long)max) {
        return INPUT_OUT_OF_RANGE;
    }

    *out = (int)value;
    return INPUT_OK;
}

int prompt_int(FILE *in, FILE *out, const char *prompt,
               int min, int max, int *value)
{
    char line[INPUT_MAX_LINE];
    int read_status;
    InputStatus status;

    if (in == NULL || out == NULL || value == NULL) {
        return -1;
    }

    for (;;) {
        fputs(prompt != NULL ? prompt : "", out);
        fflush(out);

        read_status = read_line_bounded(in, line, sizeof line);

        if (read_status < 0) {
            return -1;
        }

        if (read_status == 0) {
            fprintf(out, "Error: Input is too long.\n");
            continue;
        }

        status = parse_int_strict(line, min, max, value);

        if (status == INPUT_OK) {
            return 1;
        }

        if (status == INPUT_NOT_A_NUMBER) {
            fprintf(out, "Error: Enter a valid whole number.\n");
        } else {
            fprintf(out, "Error: Enter a value between %d and %d.\n",
                    min, max);
        }
    }
}
