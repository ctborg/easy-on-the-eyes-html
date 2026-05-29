#include "test_support.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

void test_expect(const char *name, int ok) {
    if (!ok) {
        fprintf(stderr, "[FAIL] %s\n", name);
        failures++;
    }
}

void test_expect_str(const char *name, const char *got, const char *want) {
    if (strcmp(got, want) != 0) {
        fprintf(stderr, "[FAIL] %s\nGot:  %s\nWant: %s\n", name, got, want);
        failures++;
    }
}

Config test_config(void) {
    Config cfg;
    cfg.indent_size = 2;
    cfg.use_tabs = 0;
    cfg.max_char = 0;
    cfg.space_conditional = 1;
    return cfg;
}

int main(void) {
    run_cli_tests();
    run_config_tests();
    run_ignore_tests();
    run_engine_json_tests();
    run_engine_css_tests();
    run_engine_js_tests();
    run_engine_html_tests();
    if (failures) {
        fprintf(stderr, "%d test failure(s)\n", failures);
        return 1;
    }
    printf("[OK] all tests passed\n");
    return 0;
}
