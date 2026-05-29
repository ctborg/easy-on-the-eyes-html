#include "test_support.h"

#include "engine_js.h"

#include <stdlib.h>
#include <string.h>

void run_engine_js_tests(void) {
    Config cfg = test_config();
    char err[128] = {0};
    const char *js = "if(true){x();}";
    char *out = engine_js_format(js, strlen(js), &cfg, err, sizeof(err));
    test_expect("js formats", out != NULL);
    test_expect("js conditional space", out && strstr(out, "if (") != NULL);
    free(out);
    err[0] = '\0';
    js = "if(true){";
    out = engine_js_format(js, strlen(js), &cfg, err, sizeof(err));
    test_expect("js invalid fails", out == NULL);
    free(out);
    js = "call('}');";
    out = engine_js_format(js, strlen(js), &cfg, err, sizeof(err));
    test_expect("js string preserved", out && strstr(out, "'}'") != NULL);
    free(out);
}
