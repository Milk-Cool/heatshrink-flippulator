/*
 * Copyright (c) 2011-2015 Scott Vokes <vokes.s@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef GREATEST_H
#define GREATEST_H

/* 1.0.0 */
#define GREATEST_VERSION_MAJOR 1
#define GREATEST_VERSION_MINOR 0
#define GREATEST_VERSION_PATCH 0

/* A unit testing system for C, contained in 1 file.
 * It doesn't use dynamic allocation or depend on anything
 * beyond ANSI C89. */


/*********************************************************************
 * Minimal test runner template
 *********************************************************************/
#if 0

#include "greatest.h"

TEST foo_should_foo() {
    PASS();
}

static void setup_cb(void *data) {
    printf("setup callback for each test case\n");
}

static void teardown_cb(void *data) {
    printf("teardown callback for each test case\n");
}

SUITE(suite) {
    /* Optional setup/teardown callbacks which will be run before/after
     * every test case in the suite.
     * Cleared when the suite finishes. */
    SET_SETUP(setup_cb, voidp_to_callback_data);
    SET_TEARDOWN(teardown_cb, voidp_to_callback_data);

    RUN_TEST(foo_should_foo);
}

/* Add definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

/* Set up, run suite(s) of tests, report pass/fail/skip stats. */
int run_tests(void) {
    GREATEST_INIT();            /* init. greatest internals */
    /* List of suites to run. */
    RUN_SUITE(suite);
    GREATEST_REPORT();          /* display results */
    return greatest_all_passed();
}

/* main(), for a standalone command-line test runner.
 * This replaces run_tests above, and adds command line option
 * handling and exiting with a pass/fail status. */
// int main(int argc, char **argv) {
//     GREATEST_MAIN_BEGIN();      /* init & parse command-line args */
//     RUN_SUITE(suite);
//     GREATEST_MAIN_END();        /* display results */
// }

#endif
/*********************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/***********
 * Options *
 ***********/

/* Default column width for non-verbose output. */
#ifndef GREATEST_DEFAULT_WIDTH
#define GREATEST_DEFAULT_WIDTH 72
#endif

/* FILE *, for test logging. */
#ifndef GREATEST_STDOUT
#define GREATEST_STDOUT stdout
#endif

/* Remove GREATEST_ prefix from most commonly used symbols? */
#ifndef GREATEST_USE_ABBREVS
#define GREATEST_USE_ABBREVS 1
#endif

/* Set to 0 to disable all use of setjmp/longjmp. */
#ifndef GREATEST_USE_LONGJMP
#define GREATEST_USE_LONGJMP 1
#endif

#if GREATEST_USE_LONGJMP
#include <setjmp.h>
#endif

/* Set to 0 to disable all use of time.h / clock(). */
#ifndef GREATEST_USE_TIME
#define GREATEST_USE_TIME 1
#endif

#if GREATEST_USE_TIME
#include <time.h>
#endif

/* Floating point type, for ASSERT_IN_RANGE. */
#ifndef GREATEST_FLOAT
#define GREATEST_FLOAT double
#define GREATEST_FLOAT_FMT "%g"
#endif

/*********
 * Types *
 *********/

/* Info for the current running suite. */
typedef struct greatest_suite_info {
    unsigned int tests_run;
    unsigned int passed;
    unsigned int failed;
    unsigned int skipped;

#if GREATEST_USE_TIME
    /* timers, pre/post running suite and individual tests */
    clock_t pre_suite;
    clock_t post_suite;
    clock_t pre_test;
    clock_t post_test;
#endif
} greatest_suite_info;

/* Type for a suite function. */
typedef void (greatest_suite_cb)(void);

/* Types for setup/teardown callbacks. If non-NULL, these will be run
 * and passed the pointer to their additional data. */
typedef void (greatest_setup_cb)(void *udata);
typedef void (greatest_teardown_cb)(void *udata);

/* Type for an equality comparison between two pointers of the same type.
 * Should return non-0 if equal, otherwise 0.
 * UDATA is a closure value, passed through from ASSERT_EQUAL_T[m]. */
typedef int greatest_equal_cb(const void *exp, const void *got, void *udata);

/* Type for a callback that prints a value pointed to by T.
 * Return value has the same meaning as printf's.
 * UDATA is a closure value, passed through from ASSERT_EQUAL_T[m]. */
typedef int greatest_printf_cb(const void *t, void *udata);

/* Callbacks for an arbitrary type; needed for type-specific
 * comparisons via GREATEST_ASSERT_EQUAL_T[m].*/
typedef struct greatest_type_info {
    greatest_equal_cb *equal;
    greatest_printf_cb *print;
} greatest_type_info;

/* Callbacks for string type. */
extern greatest_type_info greatest_type_info_string;

typedef enum {
    GREATEST_FLAG_VERBOSE = 0x01,
    GREATEST_FLAG_FIRST_FAIL = 0x02,
    GREATEST_FLAG_LIST_ONLY = 0x04
} GREATEST_FLAG;

/* Struct containing all test runner state. */
typedef struct greatest_run_info {
    unsigned int flags;
    unsigned int tests_run;     /* total test count */

    /* overall pass/fail/skip counts */
    unsigned int passed;
    unsigned int failed;
    unsigned int skipped;
    unsigned int assertions;

    /* currently running test suite */
    greatest_suite_info suite;

    /* info to print about the most recent failure */
    const char *fail_file;
    unsigned int fail_line;
    const char *msg;

    /* current setup/teardown hooks and userdata */
    greatest_setup_cb *setup;
    void *setup_udata;
    greatest_teardown_cb *teardown;
    void *teardown_udata;

    /* formatting info for ".....s...F"-style output */
    unsigned int col;
    unsigned int width;

    /* only run a specific suite or test */
    char *suite_filter;
    char *test_filter;

#if GREATEST_USE_TIME
    /* overall timers */
    clock_t begin;
    clock_t end;
#endif

#if GREATEST_USE_LONGJMP
    jmp_buf jump_dest;
#endif
} greatest_run_info;

/* Global var for the current testing context.
 * Initialized by GREATEST_MAIN_DEFS(). */
extern greatest_run_info greatest_info;


/**********************
 * Exported functions *
 **********************/

/* These are used internally by greatest. */
void greatest_do_pass(const char *name);
void greatest_do_fail(const char *name);
void greatest_do_skip(const char *name);
int greatest_pre_test(const char *name);
void greatest_post_test(const char *name, int res);
void greatest_usage(const char *name);
int greatest_do_assert_equal_t(const void *exp, const void *got,
    greatest_type_info *type_info, void *udata);

/* These are part of the public greatest API. */
void GREATEST_SET_SETUP_CB(greatest_setup_cb *cb, void *udata);
void GREATEST_SET_TEARDOWN_CB(greatest_teardown_cb *cb, void *udata);
int greatest_all_passed(void);


/********************
* Language Support *
********************/

/* If __VA_ARGS__ (C99) is supported, allow parametric testing
* without needing to manually manage the argument struct. */
#if __STDC_VERSION__ >= 19901L || _MSC_VER >= 1800
#define GREATEST_VA_ARGS
#endif


/**********
 * Macros *
 **********/

/* Define a suite. */
#define GREATEST_SUITE(NAME) void NAME(void); void NAME(void)

/* Start defining a test function.
 * The arguments are not included, to allow parametric testing. */
#define GREATEST_TEST static greatest_test_res

/* PASS/FAIL/SKIP result from a test. Used internally. */
typedef enum {
    GREATEST_TEST_RES_PASS = 0,
    GREATEST_TEST_RES_FAIL = -1,
    GREATEST_TEST_RES_SKIP = 1
} greatest_test_res;

/* Run a suite. */
#define GREATEST_RUN_SUITE(S_NAME) greatest_run_suite(S_NAME, #S_NAME)

/* Run a test in the current suite. */
#define GREATEST_RUN_TEST(TEST)                                         \
    do {                                                                \
        if (greatest_pre_test(#TEST) == 1) {                            \
            greatest_test_res res = GREATEST_SAVE_CONTEXT();            \
            if (res == GREATEST_TEST_RES_PASS) {                        \
                res = TEST();                                           \
            }                                                           \
            greatest_post_test(#TEST, res);                             \
        } else if (GREATEST_LIST_ONLY()) {                              \
            fprintf(GREATEST_STDOUT, "  %s\n", #TEST);                  \
        }                                                               \
    } while (0)

/* Run a test in the current suite with one void * argument,
 * which can be a pointer to a struct with multiple arguments. */
#define GREATEST_RUN_TEST1(TEST, ENV)                                   \
    do {                                                                \
        if (greatest_pre_test(#TEST) == 1) {                            \
            int res = TEST(ENV);                                        \
            greatest_post_test(#TEST, res);                             \
        } else if (GREATEST_LIST_ONLY()) {                              \
            fprintf(GREATEST_STDOUT, "  %s\n", #TEST);                  \
        }                                                               \
    } while (0)

#ifdef GREATEST_VA_ARGS
#define GREATEST_RUN_TESTp(TEST, ...)                                   \
    do {                                                                \
        if (greatest_pre_test(#TEST) == 1) {                            \
            int res = TEST(__VA_ARGS__);                                \
            greatest_post_test(#TEST, res);                             \
        } else if (GREATEST_LIST_ONLY()) {                              \
            fprintf(GREATEST_STDOUT, "  %s\n", #TEST);                  \
        }                                                               \
    } while (0)
#endif


/* Check if the test runner is in verbose mode. */
#define GREATEST_IS_VERBOSE() (greatest_info.flags & GREATEST_FLAG_VERBOSE)
#define GREATEST_LIST_ONLY() (greatest_info.flags & GREATEST_FLAG_LIST_ONLY)
#define GREATEST_FIRST_FAIL() (greatest_info.flags & GREATEST_FLAG_FIRST_FAIL)
#define GREATEST_FAILURE_ABORT() (greatest_info.suite.failed > 0 && GREATEST_FIRST_FAIL())

/* Message-less forms of tests defined below. */
#define GREATEST_PASS() GREATEST_PASSm(NULL)
#define GREATEST_FAIL() GREATEST_FAILm(NULL)
#define GREATEST_SKIP() GREATEST_SKIPm(NULL)
#define GREATEST_ASSERT(COND)                                           \
    GREATEST_ASSERTm(#COND, COND)
#define GREATEST_ASSERT_OR_LONGJMP(COND)                                \
    GREATEST_ASSERT_OR_LONGJMPm(#COND, COND)
#define GREATEST_ASSERT_FALSE(COND)                                     \
    GREATEST_ASSERT_FALSEm(#COND, COND)
#define GREATEST_ASSERT_EQ(EXP, GOT)                                    \
    GREATEST_ASSERT_EQm(#EXP " != " #GOT, EXP, GOT)
#define GREATEST_ASSERT_EQ_FMT(EXP, GOT, FMT)                           \
    GREATEST_ASSERT_EQ_FMTm(#EXP " != " #GOT, EXP, GOT, FMT)
#define GREATEST_ASSERT_IN_RANGE(EXP, GOT, TOL)                         \
    GREATEST_ASSERT_IN_RANGEm(#EXP " != " #GOT " +/- " #TOL, EXP, GOT, TOL)
#define GREATEST_ASSERT_EQUAL_T(EXP, GOT, TYPE_INFO, UDATA)             \
    GREATEST_ASSERT_EQUAL_Tm(#EXP " != " #GOT, EXP, GOT, TYPE_INFO, UDATA)
#define GREATEST_ASSERT_STR_EQ(EXP, GOT)                                \
    GREATEST_ASSERT_STR_EQm(#EXP " != " #GOT, EXP, GOT)

/* The following forms take an additional message argument first,
 * to be displayed by the test runner. */

/* Fail if a condition is not true, with message. */
#define GREATEST_ASSERTm(MSG, COND)                                     \
    do {                                                                \
        greatest_info.assertions++;                                     \
        if (!(COND)) { GREATEST_FAILm(MSG); }                           \
    } while (0)

/* Fail if a condition is not true, longjmping out of test. */
#define GREATEST_ASSERT_OR_LONGJMPm(MSG, COND)                          \
    do {                                                                \
        greatest_info.assertions++;                                     \
        if (!(COND)) { GREATEST_FAIL_WITH_LONGJMPm(MSG); }              \
    } while (0)

/* Fail if a condition is not false, with message. */
#define GREATEST_ASSERT_FALSEm(MSG, COND)                               \
    do {                                                                \
        greatest_info.assertions++;                                     \
        if ((COND)) { GREATEST_FAILm(MSG); }                            \
    } while (0)

/* Fail if EXP != GOT (equality comparison by ==). */
#define GREATEST_ASSERT_EQm(MSG, EXP, GOT)                              \
    do {                                                                \
        greatest_info.assertions++;                                     \
        if ((EXP) != (GOT)) { GREATEST_FAILm(MSG); }                    \
    } while (0)

/* Fail if EXP != GOT (equality comparison by ==). */
#define GREATEST_ASSERT_EQ_FMTm(MSG, EXP, GOT, FMT)                     \
    do {                                                                \
        greatest_info.assertions++;                                     \
        const char *fmt = ( FMT );                                      \
        if ((EXP) != (GOT)) {                                           \
            fprintf(GREATEST_STDOUT, "\nExpected: ");                   \
            fprintf(GREATEST_STDOUT, fmt, EXP);                         \
            fprintf(GREATEST_STDOUT, "\nGot: ");                        \
            fprintf(GREATEST_STDOUT, fmt, GOT);                         \
            fprintf(GREATEST_STDOUT, "\n");                             \
            GREATEST_FAILm(MSG);                                        \
        }                                                               \
    } while (0)

/* Fail if GOT not in range of EXP +|- TOL. */
#define GREATEST_ASSERT_IN_RANGEm(MSG, EXP, GOT, TOL)                   \
    do {                                                                \
        greatest_info.assertions++;                                     \
        GREATEST_FLOAT exp = (EXP);                                     \
        GREATEST_FLOAT got = (GOT);                                     \
        GREATEST_FLOAT tol = (TOL);                                     \
        if ((exp > got && exp - got > tol) ||                           \
            (exp < got && got - exp > tol)) {                           \
            fprintf(GREATEST_STDOUT,                                    \
                "\nExpected: " GREATEST_FLOAT_FMT                       \
                " +/- " GREATEST_FLOAT_FMT "\n"                         \
                "Got: " GREATEST_FLOAT_FMT "\n",                        \
                exp, tol, got);                                         \
            GREATEST_FAILm(MSG);                                        \
        }                                                               \
    } while (0)

/* Fail if EXP is not equal to GOT, according to strcmp. */
#define GREATEST_ASSERT_STR_EQm(MSG, EXP, GOT)                          \
    do {                                                                \
        GREATEST_ASSERT_EQUAL_Tm(MSG, EXP, GOT,                         \
            &greatest_type_info_string, NULL);                          \
    } while (0)                                                         \

/* Fail if EXP is not equal to GOT, according to a comparison
 * callback in TYPE_INFO. If they are not equal, optionally use a
 * print callback in TYPE_INFO to print them. */
#define GREATEST_ASSERT_EQUAL_Tm(MSG, EXP, GOT, TYPE_INFO, UDATA)       \
    do {                                                                \
        greatest_type_info *type_info = (TYPE_INFO);                    \
        greatest_info.assertions++;                                     \
        if (!greatest_do_assert_equal_t(EXP, GOT,                       \
                type_info, UDATA)) {                                    \
            if (type_info == NULL || type_info->equal == NULL) {        \
                GREATEST_FAILm("type_info->equal callback missing!");   \
            } else {                                                    \
                GREATEST_FAILm(MSG);                                    \
            }                                                           \
        }                                                               \
    } while (0)                                                         \

/* Pass. */
#define GREATEST_PASSm(MSG)                                             \
    do {                                                                \
        greatest_info.msg = MSG;                                        \
        return GREATEST_TEST_RES_PASS;                                  \
    } while (0)

/* Fail. */
#define GREATEST_FAILm(MSG)                                             \
    do {                                                                \
        greatest_info.fail_file = __FILE__;                             \
        greatest_info.fail_line = __LINE__;                             \
        greatest_info.msg = MSG;                                        \
        return GREATEST_TEST_RES_FAIL;                                  \
    } while (0)

/* Optional GREATEST_FAILm variant that longjmps. */
#if GREATEST_USE_LONGJMP
#define GREATEST_FAIL_WITH_LONGJMP() GREATEST_FAIL_WITH_LONGJMPm(NULL)
#define GREATEST_FAIL_WITH_LONGJMPm(MSG)                                \
    do {                                                                \
        greatest_info.fail_file = __FILE__;                             \
        greatest_info.fail_line = __LINE__;                             \
        greatest_info.msg = MSG;                                        \
        longjmp(greatest_info.jump_dest, GREATEST_TEST_RES_FAIL);       \
    } while (0)
#endif

/* Skip the current test. */
#define GREATEST_SKIPm(MSG)                                             \
    do {                                                                \
        greatest_info.msg = MSG;                                        \
        return GREATEST_TEST_RES_SKIP;                                  \
    } while (0)

/* Check the result of a subfunction using ASSERT, etc. */
#define GREATEST_CHECK_CALL(RES)                                        \
    do {                                                                \
        int _check_call_res = RES;                                      \
        if (_check_call_res != GREATEST_TEST_RES_PASS) {                \
            return _check_call_res;                                     \
        }                                                               \
    } while (0)                                                         \

#if GREATEST_USE_TIME
#define GREATEST_SET_TIME(NAME)                                         \
    NAME = clock();                                                     \
    if (NAME == (clock_t) -1) {                                         \
        fprintf(GREATEST_STDOUT,                                        \
            "clock error: %s\n", #NAME);                                \
        exit(EXIT_FAILURE);                                             \
    }

#define GREATEST_CLOCK_DIFF(C1, C2)                                     \
    fprintf(GREATEST_STDOUT, " (%lu ticks, %.3f sec)",                  \
        (long unsigned int) (C2) - (long unsigned int)(C1),             \
        (double)((C2) - (C1)) / (1.0 * (double)CLOCKS_PER_SEC))
#else
#define GREATEST_SET_TIME(UNUSED)
#define GREATEST_CLOCK_DIFF(UNUSED1, UNUSED2)
#endif

#if GREATEST_USE_LONGJMP
#define GREATEST_SAVE_CONTEXT()                                         \
        /* setjmp returns 0 (GREATEST_TEST_RES_PASS) on first call */   \
        /* so the test runs, then RES_FAIL from FAIL_WITH_LONGJMP. */   \
        ((greatest_test_res)(setjmp(greatest_info.jump_dest)))
#else
#define GREATEST_SAVE_CONTEXT()                                         \
    /*a no-op, since setjmp/longjmp aren't being used */                \
    GREATEST_TEST_RES_PASS
#endif

/* Include several function definitions in the main test file. */
#define GREATEST_MAIN_DEFS()                                            \
                                                                        \
/* Is FILTER a subset of NAME? */                                       \
static int greatest_name_match(const char *name,                        \
    const char *filter) {                                               \
    size_t offset = 0;                                                  \
    size_t filter_len = strlen(filter);                                 \
    while (name[offset] != '\0') {                                      \
        if (name[offset] == filter[0]) {                                \
            if (0 == strncmp(&name[offset], filter, filter_len)) {      \
                return 1;                                               \
            }                                                           \
        }                                                               \
        offset++;                                                       \
    }                                                                   \
                                                                        \
    return 0;                                                           \
}                                                                       \
                                                                        \
int greatest_pre_test(const char *name) {                               \
    if (!GREATEST_LIST_ONLY()                                           \
        && (!GREATEST_FIRST_FAIL() || greatest_info.suite.failed == 0)  \
        && (greatest_info.test_filter == NULL ||                        \
            greatest_name_match(name, greatest_info.test_filter))) {    \
        GREATEST_SET_TIME(greatest_info.suite.pre_test);                \
        if (greatest_info.setup) {                                      \
            greatest_info.setup(greatest_info.setup_udata);             \
        }                                                               \
        return 1;               /* test should be run */                \
    } else {                                                            \
        return 0;               /* skipped */                           \
    }                                                                   \
}                                                                       \
                                                                        \
void greatest_post_test(const char *name, int res) {                    \
    GREATEST_SET_TIME(greatest_info.suite.post_test);                   \
    if (greatest_info.teardown) {                                       \
        void *udata = greatest_info.teardown_udata;                     \
        greatest_info.teardown(udata);                                  \
    }                                                                   \
                                                                        \
    if (res <= GREATEST_TEST_RES_FAIL) {                                \
        greatest_do_fail(name);                                         \
    } else if (res >= GREATEST_TEST_RES_SKIP) {                         \
        greatest_do_skip(name);                                         \
    } else if (res == GREATEST_TEST_RES_PASS) {                         \
        greatest_do_pass(name);                                         \
    }                                                                   \
    greatest_info.suite.tests_run++;                                    \
    greatest_info.col++;                                                \
    if (GREATEST_IS_VERBOSE()) {                                        \
        GREATEST_CLOCK_DIFF(greatest_info.suite.pre_test,               \
            greatest_info.suite.post_test);                             \
        fprintf(GREATEST_STDOUT, "\n");                                 \
    } else if (greatest_info.col % greatest_info.width == 0) {          \
        fprintf(GREATEST_STDOUT, "\n");                                 \
        greatest_info.col = 0;                                          \
    }                                                                   \
    if (GREATEST_STDOUT == stdout) fflush(stdout);                      \
}                                                                       \
                                                                        \
void greatest_run_suite(greatest_suite_cb *suite_cb,             \
                               const char *suite_name) {                \
    if (greatest_info.suite_filter &&                                   \
        !greatest_name_match(suite_name, greatest_info.suite_filter)) { \
        return;                                                         \
    }                                                                   \
    if (GREATEST_FIRST_FAIL() && greatest_info.failed > 0) { return; }  \
    memset(&greatest_info.suite, 0, sizeof(greatest_info.suite));       \
    greatest_info.col = 0;                                              \
    fprintf(GREATEST_STDOUT, "\n* Suite %s:\n", suite_name);            \
    GREATEST_SET_TIME(greatest_info.suite.pre_suite);                   \
    suite_cb();                                                         \
    GREATEST_SET_TIME(greatest_info.suite.post_suite);                  \
    if (greatest_info.suite.tests_run > 0) {                            \
        fprintf(GREATEST_STDOUT,                                        \
            "\n%u tests - %u pass, %u fail, %u skipped",                \
            greatest_info.suite.tests_run,                              \
            greatest_info.suite.passed,                                 \
            greatest_info.suite.failed,                                 \
            greatest_info.suite.skipped);                               \
        GREATEST_CLOCK_DIFF(greatest_info.suite.pre_suite,              \
            greatest_info.suite.post_suite);                            \
        fprintf(GREATEST_STDOUT, "\n");                                 \
    }                                                                   \
    greatest_info.setup = NULL;                                         \
    greatest_info.setup_udata = NULL;                                   \
    greatest_info.teardown = NULL;                                      \
    greatest_info.teardown_udata = NULL;                                \
    greatest_info.passed += greatest_info.suite.passed;                 \
    greatest_info.failed += greatest_info.suite.failed;                 \
    greatest_info.skipped += greatest_info.suite.skipped;               \
    greatest_info.tests_run += greatest_info.suite.tests_run;           \
}                                                                       \
                                                                        \
void greatest_do_pass(const char *name) {                               \
    if (GREATEST_IS_VERBOSE()) {                                        \
        fprintf(GREATEST_STDOUT, "PASS %s: %s",                         \
            name, greatest_info.msg ? greatest_info.msg : "");          \
    } else {                                                            \
        fprintf(GREATEST_STDOUT, ".");                                  \
    }                                                                   \
    greatest_info.suite.passed++;                                       \
}                                                                       \
                                                                        \
void greatest_do_fail(const char *name) {                               \
    if (GREATEST_IS_VERBOSE()) {                                        \
        fprintf(GREATEST_STDOUT,                                        \
            "FAIL %s: %s (%s:%u)",                                      \
            name, greatest_info.msg ? greatest_info.msg : "",           \
            greatest_info.fail_file, greatest_info.fail_line);          \
    } else {                                                            \
        fprintf(GREATEST_STDOUT, "F");                                  \
        greatest_info.col++;                                            \
        /* add linebreak if in line of '.'s */                          \
        if (greatest_info.col != 0) {                                   \
            fprintf(GREATEST_STDOUT, "\n");                             \
            greatest_info.col = 0;                                      \
        }                                                               \
        fprintf(GREATEST_STDOUT, "FAIL %s: %s (%s:%u)\n",               \
            name,                                                       \
            greatest_info.msg ? greatest_info.msg : "",                 \
            greatest_info.fail_file, greatest_info.fail_line);          \
    }                                                                   \
    greatest_info.suite.failed++;                                       \
}                                                                       \
                                                                        \
void greatest_do_skip(const char *name) {                               \
    if (GREATEST_IS_VERBOSE()) {                                        \
        fprintf(GREATEST_STDOUT, "SKIP %s: %s",                         \
            name,                                                       \
            greatest_info.msg ?                                         \
            greatest_info.msg : "" );                                   \
    } else {                                                            \
        fprintf(GREATEST_STDOUT, "s");                                  \
    }                                                                   \
    greatest_info.suite.skipped++;                                      \
}                                                                       \
                                                                        \
int greatest_do_assert_equal_t(const void *exp, const void *got,        \
        greatest_type_info *type_info, void *udata) {                   \
    int eq = 0;                                                         \
    if (type_info == NULL || type_info->equal == NULL) {                \
        return 0;                                                       \
    }                                                                   \
    eq = type_info->equal(exp, got, udata);                             \
    if (!eq) {                                                          \
        if (type_info->print != NULL) {                                 \
            fprintf(GREATEST_STDOUT, "\nExpected: ");                   \
            (void)type_info->print(exp, udata);                         \
            fprintf(GREATEST_STDOUT, "\nGot: ");                        \
            (void)type_info->print(got, udata);                         \
            fprintf(GREATEST_STDOUT, "\n");                             \
        } else {                                                        \
            fprintf(GREATEST_STDOUT,                                    \
                "GREATEST_ASSERT_EQUAL_T failure at %s:%dn",            \
                greatest_info.fail_file,                                \
                greatest_info.fail_line);                               \
        }                                                               \
    }                                                                   \
    return eq;                                                          \
}                                                                       \
                                                                        \
void greatest_usage(const char *name) {                                 \
    fprintf(GREATEST_STDOUT,                                            \
        "Usage: %s [-hlfv] [-s SUITE] [-t TEST]\n"                      \
        "  -h        print this Help\n"                                 \
        "  -l        List suites and their tests, then exit\n"          \
        "  -f        Stop runner after first failure\n"                 \
        "  -v        Verbose output\n"                                  \
        "  -s SUITE  only run suite named SUITE\n"                      \
        "  -t TEST   only run test named TEST\n",                       \
        name);                                                          \
}                                                                       \
                                                                        \
int greatest_all_passed() { return (greatest_info.failed == 0); }       \
                                                                        \
void GREATEST_SET_SETUP_CB(greatest_setup_cb *cb, void *udata) {        \
    greatest_info.setup = cb;                                           \
    greatest_info.setup_udata = udata;                                  \
}                                                                       \
                                                                        \
void GREATEST_SET_TEARDOWN_CB(greatest_teardown_cb *cb,                 \
                                    void *udata) {                      \
    greatest_info.teardown = cb;                                        \
    greatest_info.teardown_udata = udata;                               \
}                                                                       \
                                                                        \
static int greatest_string_equal_cb(const void *exp, const void *got,   \
    void *udata) {                                                      \
    (void)udata;                                                        \
    return (0 == strcmp((const char *)exp, (const char *)got));         \
}                                                                       \
                                                                        \
static int greatest_string_printf_cb(const void *t, void *udata) {      \
    (void)udata;                                                        \
    return fprintf(GREATEST_STDOUT, "%s", (const char *)t);             \
}                                                                       \
                                                                        \
greatest_type_info greatest_type_info_string = {                        \
    greatest_string_equal_cb,                                           \
    greatest_string_printf_cb,                                          \
};                                                                      \
                                                                        \
greatest_run_info greatest_info

/* Init internals. */
#define GREATEST_INIT()                                                 \
    do {                                                                \
        memset(&greatest_info, 0, sizeof(greatest_info));               \
        greatest_info.width = GREATEST_DEFAULT_WIDTH;                   \
        GREATEST_SET_TIME(greatest_info.begin);                         \
    } while (0)                                                         \

/* Handle command-line arguments, etc. */
#define GREATEST_MAIN_BEGIN()                                           \
    do {                                                                \
        int i = 0;                                                      \
        GREATEST_INIT();                                                \
        for (i = 1; i < argc; i++) {                                    \
            if (0 == strcmp("-t", argv[i])) {                           \
                if (argc <= i + 1) {                                    \
                    greatest_usage(argv[0]);                            \
                    exit(EXIT_FAILURE);                                 \
                }                                                       \
                greatest_info.test_filter = argv[i+1];                  \
                i++;                                                    \
            } else if (0 == strcmp("-s", argv[i])) {                    \
                if (argc <= i + 1) {                                    \
                    greatest_usage(argv[0]);                            \
                    exit(EXIT_FAILURE);                                 \
                }                                                       \
                greatest_info.suite_filter = argv[i+1];                 \
                i++;                                                    \
            } else if (0 == strcmp("-f", argv[i])) {                    \
                greatest_info.flags |= GREATEST_FLAG_FIRST_FAIL;        \
            } else if (0 == strcmp("-v", argv[i])) {                    \
                greatest_info.flags |= GREATEST_FLAG_VERBOSE;           \
            } else if (0 == strcmp("-l", argv[i])) {                    \
                greatest_info.flags |= GREATEST_FLAG_LIST_ONLY;         \
            } else if (0 == strcmp("-h", argv[i])) {                    \
                greatest_usage(argv[0]);                                \
                exit(EXIT_SUCCESS);                                     \
            } else {                                                    \
                fprintf(GREATEST_STDOUT,                                \
                    "Unknown argument '%s'\n", argv[i]);                \
                greatest_usage(argv[0]);                                \
                exit(EXIT_FAILURE);                                     \
            }                                                           \
        }                                                               \
    } while (0)

/* Report passes, failures, skipped tests, the number of
 * assertions, and the overall run time. */
#define GREATEST_REPORT()                                               \
    do {                                                                \
        if (!GREATEST_LIST_ONLY()) {                                    \
            GREATEST_SET_TIME(greatest_info.end);                       \
            fprintf(GREATEST_STDOUT,                                    \
                "\nTotal: %u tests", greatest_info.tests_run);          \
            GREATEST_CLOCK_DIFF(greatest_info.begin,                    \
                greatest_info.end);                                     \
            fprintf(GREATEST_STDOUT, ", %u assertions\n",               \
                greatest_info.assertions);                              \
            fprintf(GREATEST_STDOUT,                                    \
                "Pass: %u, fail: %u, skip: %u.\n",                      \
                greatest_info.passed,                                   \
                greatest_info.failed, greatest_info.skipped);           \
        }                                                               \
    } while (0)

/* Report results, exit with exit status based on results. */
#define GREATEST_MAIN_END()                                             \
    do {                                                                \
        GREATEST_REPORT();                                              \
        return (greatest_all_passed() ? EXIT_SUCCESS : EXIT_FAILURE);   \
    } while (0)

/* Make abbreviations without the GREATEST_ prefix for the
 * most commonly used symbols. */
#if GREATEST_USE_ABBREVS
#define TEST           GREATEST_TEST
#define SUITE          GREATEST_SUITE
#define RUN_TEST       GREATEST_RUN_TEST
#define RUN_TEST1      GREATEST_RUN_TEST1
#define RUN_SUITE      GREATEST_RUN_SUITE
#define ASSERT         GREATEST_ASSERT
#define ASSERTm        GREATEST_ASSERTm
#define ASSERT_FALSE   GREATEST_ASSERT_FALSE
#define ASSERT_EQ      GREATEST_ASSERT_EQ
#define ASSERT_EQ_FMT  GREATEST_ASSERT_EQ_FMT
#define ASSERT_IN_RANGE GREATEST_ASSERT_IN_RANGE
#define ASSERT_EQUAL_T GREATEST_ASSERT_EQUAL_T
#define ASSERT_STR_EQ  GREATEST_ASSERT_STR_EQ
#define ASSERT_FALSEm  GREATEST_ASSERT_FALSEm
#define ASSERT_EQm     GREATEST_ASSERT_EQm
#define ASSERT_EQ_FMTm GREATEST_ASSERT_EQ_FMTm
#define ASSERT_IN_RANGEm GREATEST_ASSERT_IN_RANGEm
#define ASSERT_EQUAL_Tm GREATEST_ASSERT_EQUAL_Tm
#define ASSERT_STR_EQm GREATEST_ASSERT_STR_EQm
#define PASS           GREATEST_PASS
#define FAIL           GREATEST_FAIL
#define SKIP           GREATEST_SKIP
#define PASSm          GREATEST_PASSm
#define FAILm          GREATEST_FAILm
#define SKIPm          GREATEST_SKIPm
#define SET_SETUP      GREATEST_SET_SETUP_CB
#define SET_TEARDOWN   GREATEST_SET_TEARDOWN_CB
#define CHECK_CALL     GREATEST_CHECK_CALL

#ifdef GREATEST_VA_ARGS
#define RUN_TESTp      GREATEST_RUN_TESTp
#endif

#if GREATEST_USE_LONGJMP
#define ASSERT_OR_LONGJMP  GREATEST_ASSERT_OR_LONGJMP
#define ASSERT_OR_LONGJMPm GREATEST_ASSERT_OR_LONGJMPm
#define FAIL_WITH_LONGJMP  GREATEST_FAIL_WITH_LONGJMP
#define FAIL_WITH_LONGJMPm GREATEST_FAIL_WITH_LONGJMPm
#endif

#endif /* USE_ABBREVS */

#endif
