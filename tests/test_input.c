/*
 * S1TMS-8: configuration input validation.
 * Boundary tests (min-1, min, max, max+1), error inputs (zero, negative,
 * non-numeric, oversized) and bounded line reading.
 *
 * Uses small temporary files in the working directory instead of tmpfile(),
 * which fails on some Windows setups.
 */
#include <stdio.h>
#include <string.h>

#include "input.h"
#include "test_util.h"

#define TMP_IN "tms_test_input.tmp"
#define TMP_OUT "tms_test_output.tmp"

static FILE *open_input(const char *text)
{
    FILE *f = fopen(TMP_IN, "wb");

    if (f == NULL) {
        return NULL;
    }
    fputs(text, f);
    fclose(f);
    return fopen(TMP_IN, "rb");
}

static void close_input(FILE *f)
{
    if (f != NULL) {
        fclose(f);
    }
    remove(TMP_IN);
}

static void test_parse_accepts_valid_and_boundaries(void)
{
    int v = -1;

    CHECK(parse_int_strict("5", 1, 100, &v) == INPUT_OK && v == 5);
    CHECK(parse_int_strict("1", 1, 100, &v) == INPUT_OK && v == 1);      /* min */
    CHECK(parse_int_strict("100", 1, 100, &v) == INPUT_OK && v == 100);  /* max */
    CHECK(parse_int_strict("  42  ", 1, 100, &v) == INPUT_OK && v == 42);/* spaces */
    CHECK(parse_int_strict("007", 1, 100, &v) == INPUT_OK && v == 7);
    CHECK(parse_int_strict("+9", 1, 100, &v) == INPUT_OK && v == 9);
}

static void test_parse_out_of_range(void)
{
    int v = 123;

    CHECK(parse_int_strict("0", 1, 100, &v) == INPUT_OUT_OF_RANGE);      /* min-1 */
    CHECK(parse_int_strict("101", 1, 100, &v) == INPUT_OUT_OF_RANGE);    /* max+1 */
    CHECK(parse_int_strict("-4", 1, 100, &v) == INPUT_OUT_OF_RANGE);     /* negative */
    CHECK(parse_int_strict("-0", 1, 100, &v) == INPUT_OUT_OF_RANGE);
    CHECK(parse_int_strict("2147483648", 1, 100, &v) == INPUT_OUT_OF_RANGE);   /* INT_MAX+1 */
    CHECK(parse_int_strict("99999999999999999999", 1, 100, &v) == INPUT_OUT_OF_RANGE); /* overflow */
    CHECK(parse_int_strict("-99999999999999999999", 1, 100, &v) == INPUT_OUT_OF_RANGE);
    CHECK(v == 123);                                  /* never written on failure */
}

static void test_parse_not_a_number(void)
{
    int v = 123;

    CHECK(parse_int_strict("abc", 1, 100, &v) == INPUT_NOT_A_NUMBER);
    CHECK(parse_int_strict("5x", 1, 100, &v) == INPUT_NOT_A_NUMBER);     /* trailing junk */
    CHECK(parse_int_strict("x5", 1, 100, &v) == INPUT_NOT_A_NUMBER);
    CHECK(parse_int_strict("3.5", 1, 100, &v) == INPUT_NOT_A_NUMBER);    /* decimal */
    CHECK(parse_int_strict("1 2", 1, 100, &v) == INPUT_NOT_A_NUMBER);    /* two numbers */
    CHECK(parse_int_strict("0x10", 1, 100, &v) == INPUT_NOT_A_NUMBER);   /* hex */
    CHECK(parse_int_strict("--5", 1, 100, &v) == INPUT_NOT_A_NUMBER);
    CHECK(parse_int_strict("", 1, 100, &v) == INPUT_NOT_A_NUMBER);       /* empty */
    CHECK(parse_int_strict("   ", 1, 100, &v) == INPUT_NOT_A_NUMBER);    /* blank */
    CHECK(parse_int_strict(NULL, 1, 100, &v) == INPUT_NOT_A_NUMBER);
    CHECK(parse_int_strict("5", 1, 100, NULL) == INPUT_NOT_A_NUMBER);
    CHECK(v == 123);
}

static void test_signal_duration_boundaries(void)
{
    int v = 0;

    /* green signal duration is 1-120 seconds */
    CHECK(parse_int_strict("0", 1, 120, &v) == INPUT_OUT_OF_RANGE);
    CHECK(parse_int_strict("1", 1, 120, &v) == INPUT_OK);
    CHECK(parse_int_strict("120", 1, 120, &v) == INPUT_OK);
    CHECK(parse_int_strict("121", 1, 120, &v) == INPUT_OUT_OF_RANGE);
}

static void test_read_line_normal_crlf_and_eof(void)
{
    char buf[16];
    FILE *f = open_input("hello\r\nworld\nlast");

    CHECK(f != NULL);
    CHECK(read_line_bounded(f, buf, sizeof buf) == 1 && strcmp(buf, "hello") == 0);
    CHECK(read_line_bounded(f, buf, sizeof buf) == 1 && strcmp(buf, "world") == 0);
    CHECK(read_line_bounded(f, buf, sizeof buf) == 1 && strcmp(buf, "last") == 0);
    CHECK(read_line_bounded(f, buf, sizeof buf) == -1);              /* end of input */
    close_input(f);
}

static void test_read_line_oversized_is_rejected_and_resyncs(void)
{
    char buf[8];
    FILE *f = open_input("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n42\n");

    CHECK(f != NULL);
    CHECK(read_line_bounded(f, buf, sizeof buf) == 0);               /* too long */
    CHECK(strlen(buf) < sizeof buf);                                 /* no overflow */
    CHECK(read_line_bounded(f, buf, sizeof buf) == 1 && strcmp(buf, "42") == 0);
    close_input(f);
}

static void test_read_line_size_boundary(void)
{
    char buf[8];                                    /* holds 7 characters */
    FILE *f = open_input("1234567\n12345678\nok\n");

    CHECK(f != NULL);
    CHECK(read_line_bounded(f, buf, sizeof buf) == 1 && strcmp(buf, "1234567") == 0);  /* fits */
    CHECK(read_line_bounded(f, buf, sizeof buf) == 0);                                 /* one too many */
    CHECK(read_line_bounded(f, buf, sizeof buf) == 1 && strcmp(buf, "ok") == 0);
    close_input(f);
}

static void test_read_line_bad_arguments(void)
{
    char buf[8];

    CHECK(read_line_bounded(NULL, buf, sizeof buf) == -1);
    CHECK(read_line_bounded(stdin, NULL, 8) == -1);
    CHECK(read_line_bounded(stdin, buf, 1) == -1);
    CHECK(read_line_bounded(stdin, buf, 0) == -1);
}

static void test_prompt_retries_until_valid_and_explains(void)
{
    int value = 0;
    char log[1024];
    size_t n;
    char oversized[200];
    char text[400];
    FILE *in, *out;

    memset(oversized, 'Z', sizeof oversized - 1);
    oversized[sizeof oversized - 1] = '\0';
    snprintf(text, sizeof text, "abc\n-1\n0\n101\n99999999999999999999\n\n%s\n5\n", oversized);

    in = open_input(text);
    out = fopen(TMP_OUT, "w+b");
    CHECK(in != NULL && out != NULL);

    CHECK(prompt_int(in, out, "Enter density (1-100): ", 1, 100, &value) == 1);
    CHECK(value == 5);

    rewind(out);
    n = fread(log, 1, sizeof log - 1, out);
    log[n] = '\0';
    CHECK(strstr(log, "Enter density (1-100): ") != NULL);           /* prompt shown */
    CHECK(strstr(log, "Error: Enter a valid whole number.") != NULL);
    CHECK(strstr(log, "Error: Enter a value between 1 and 100.") != NULL);
    CHECK(strstr(log, "Error: Input is too long.") != NULL);

    fclose(out);
    remove(TMP_OUT);
    close_input(in);
}

static void test_prompt_stops_at_end_of_input(void)
{
    int value = 0;
    FILE *in = open_input("7\nnope\n");
    FILE *out = fopen(TMP_OUT, "w+b");

    CHECK(in != NULL && out != NULL);
    CHECK(prompt_int(in, out, "Value: ", 1, 10, &value) == 1 && value == 7);
    CHECK(prompt_int(in, out, "Value: ", 1, 10, &value) == -1);      /* no endless loop */
    CHECK(prompt_int(NULL, out, "Value: ", 1, 10, &value) == -1);
    CHECK(prompt_int(in, out, "Value: ", 1, 10, NULL) == -1);

    fclose(out);
    remove(TMP_OUT);
    close_input(in);
}

int main(void)
{
    printf("test_input\n");
    TEST_RUN(test_parse_accepts_valid_and_boundaries);
    TEST_RUN(test_parse_out_of_range);
    TEST_RUN(test_parse_not_a_number);
    TEST_RUN(test_signal_duration_boundaries);
    TEST_RUN(test_read_line_normal_crlf_and_eof);
    TEST_RUN(test_read_line_oversized_is_rejected_and_resyncs);
    TEST_RUN(test_read_line_size_boundary);
    TEST_RUN(test_read_line_bad_arguments);
    TEST_RUN(test_prompt_retries_until_valid_and_explains);
    TEST_RUN(test_prompt_stops_at_end_of_input);
    return TEST_SUMMARY();
}
