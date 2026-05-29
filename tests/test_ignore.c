#include "test_support.h"

#include "ignore.h"

void run_ignore_tests(void) {
    test_expect("glob star", glob_match("src/*.js", "src/a.js"));
    test_expect("glob star no slash", !glob_match("src/*.js", "src/a/b.js"));
    test_expect("glob question", glob_match("src/?.js", "src/a.js"));
    test_expect("glob doublestar nested", glob_match("src/**/*.js", "src/a/b/c.js"));
    test_expect("glob doublestar zero", glob_match("src/**/*.js", "src/a.js"));
}
