#include "test_support.h"

#include "engine_css.h"

#include <stdlib.h>
#include <string.h>

void run_engine_css_tests(void) {
    Config cfg = test_config();
    char err[128] = {0};
    const char *css = "a{color:red;background:blue}";
    char *out = engine_css_format(css, strlen(css), &cfg, err, sizeof(err));
    test_expect("css formats", out != NULL);
    if (out) {
        test_expect_str("css output", out, "a {\n  color:red;\n  background:blue;\n}\n");
        free(out);
    }
    err[0] = '\0';
    css = "a{color:red";
    out = engine_css_format(css, strlen(css), &cfg, err, sizeof(err));
    test_expect("css invalid fails", out == NULL);
    free(out);
    css = "a{margin:0}b{padding:0}";
    out = engine_css_format(css, strlen(css), &cfg, err, sizeof(err));
    test_expect("css multiple rules", out != NULL && strstr(out, "\n\nb {") != NULL);
    free(out);
}
