#ifndef EOTE_TEST_SUPPORT_H
#define EOTE_TEST_SUPPORT_H

#include "config.h"

void test_expect(const char *name, int ok);
void test_expect_str(const char *name, const char *got, const char *want);
Config test_config(void);

void run_cli_tests(void);
void run_config_tests(void);
void run_ignore_tests(void);
void run_engine_json_tests(void);
void run_engine_css_tests(void);
void run_engine_js_tests(void);
void run_engine_html_tests(void);

#endif
