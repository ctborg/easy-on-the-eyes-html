#include "test_support.h"

#include "engine_json.h"

#include <stdlib.h>
#include <string.h>

void run_engine_json_tests(void) {
    Config cfg = test_config();
    char err[128] = {0};
    const char *json = "{\"a\":1,\"b\":[true,false,],}";
    char *out = engine_json_format(json, strlen(json), &cfg, err, sizeof(err));
    test_expect("json formats", out != NULL);
    if (out) {
        test_expect_str("json output", out, "{\n  \"a\": 1,\n  \"b\": [\n    true,\n    false\n  ]\n}\n");
        free(out);
    }
    err[0] = '\0';
    json = "{\"a\":}";
    out = engine_json_format(json, strlen(json), &cfg, err, sizeof(err));
    test_expect("json invalid fails", out == NULL && err[0] != '\0');
    free(out);
    err[0] = '\0';
    json = "[1,2]";
    out = engine_json_format(json, strlen(json), &cfg, err, sizeof(err));
    test_expect("json array formats", out != NULL);
    free(out);
}
