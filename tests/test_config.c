#include "test_support.h"

#include "config.h"

#include <stdio.h>
#include <string.h>

void run_config_tests(void) {
    Config cfg;
    char err[128] = {0};
    FILE *f = fopen("test.rc", "wb");
    fputs("{\"indent-size\":2,\"use-tabs\":true,\"max-char\":80,\"space-conditional\":false}", f);
    fclose(f);
    test_expect("config load valid", config_load_file("test.rc", &cfg, err, sizeof(err)));
    test_expect("config indent", cfg.indent_size == 2);
    test_expect("config bools", cfg.use_tabs == 1 && cfg.space_conditional == 0);
    test_expect("config max", cfg.max_char == 80);
    remove("test.rc");

    err[0] = '\0';
    f = fopen("bad.rc", "wb");
    fputs("{\"indent-size\":", f);
    fclose(f);
    test_expect("config invalid fails", !config_load_file("bad.rc", &cfg, err, sizeof(err)));
    test_expect("config invalid reason", strlen(err) > 0);
    remove("bad.rc");
}
