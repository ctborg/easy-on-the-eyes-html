#include "config.h"

#include "util.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum {
    JT_INT,
    JT_BOOL,
    JT_STRING,
    JT_NULL,
    JT_OTHER
} JsonType;

typedef struct {
    char key[64];
    JsonType type;
    int int_value;
    int bool_value;
    char text[128];
} JsonKV;

typedef struct {
    JsonKV items[64];
    int count;
} JsonFlat;

void config_init(Config *cfg) {
    cfg->indent_size = -1;
    cfg->use_tabs = -1;
    cfg->max_char = -1;
    cfg->space_conditional = -1;
}

void config_from_options(const CliOptions *opts, Config *cfg) {
    cfg->indent_size = opts->indent_size;
    cfg->use_tabs = opts->use_tabs ? 1 : 0;
    cfg->max_char = opts->max_char;
    cfg->space_conditional = opts->space_conditional ? 1 : 0;
}

static void skip_ws(const char *s, size_t len, size_t *i) {
    while (*i < len && isspace((unsigned char)s[*i])) {
        (*i)++;
    }
}

static bool parse_json_string(const char *s, size_t len, size_t *i, char *out, size_t out_len) {
    if (*i >= len || s[*i] != '"') {
        return false;
    }
    (*i)++;
    size_t pos = 0;
    while (*i < len) {
        char c = s[*i];
        if (c == '"') {
            (*i)++;
            if (pos < out_len) {
                out[pos] = '\0';
            }
            return true;
        }
        if (c == '\\') {
            if (pos + 1 < out_len) {
                out[pos++] = c;
            }
            (*i)++;
            if (*i >= len) {
                return false;
            }
            c = s[*i];
        }
        if (pos + 1 < out_len) {
            out[pos++] = c;
        }
        (*i)++;
    }
    return false;
}

static bool parse_flat_json(const char *src, size_t len, JsonFlat *flat, char *err, size_t err_len) {
    flat->count = 0;
    size_t i = 0;
    skip_ws(src, len, &i);
    if (i >= len || src[i] != '{') {
        set_err_at(err, err_len, "expected object", i);
        return false;
    }
    i++;
    skip_ws(src, len, &i);
    if (i < len && src[i] == '}') {
        return true;
    }
    while (i < len) {
        if (flat->count >= 64) {
            set_err(err, err_len, "too many properties");
            return false;
        }
        JsonKV *kv = &flat->items[flat->count];
        memset(kv, 0, sizeof(*kv));
        if (!parse_json_string(src, len, &i, kv->key, sizeof(kv->key))) {
            set_err_at(err, err_len, "expected property name", i);
            return false;
        }
        skip_ws(src, len, &i);
        if (i >= len || src[i] != ':') {
            set_err_at(err, err_len, "expected ':'", i);
            return false;
        }
        i++;
        skip_ws(src, len, &i);
        if (i >= len) {
            set_err_at(err, err_len, "expected value", i);
            return false;
        }
        if (src[i] == '"') {
            kv->type = JT_STRING;
            if (!parse_json_string(src, len, &i, kv->text, sizeof(kv->text))) {
                set_err_at(err, err_len, "unterminated string", i);
                return false;
            }
        } else if (strncmp(src + i, "true", 4) == 0) {
            kv->type = JT_BOOL;
            kv->bool_value = 1;
            snprintf(kv->text, sizeof(kv->text), "true");
            i += 4;
        } else if (strncmp(src + i, "false", 5) == 0) {
            kv->type = JT_BOOL;
            kv->bool_value = 0;
            snprintf(kv->text, sizeof(kv->text), "false");
            i += 5;
        } else if (strncmp(src + i, "null", 4) == 0) {
            kv->type = JT_NULL;
            snprintf(kv->text, sizeof(kv->text), "null");
            i += 4;
        } else if (src[i] == '-' || isdigit((unsigned char)src[i])) {
            char *end = NULL;
            long v = strtol(src + i, &end, 10);
            kv->type = JT_INT;
            kv->int_value = (int)v;
            size_t n = (size_t)(end - (src + i));
            if (n >= sizeof(kv->text)) {
                n = sizeof(kv->text) - 1;
            }
            memcpy(kv->text, src + i, n);
            kv->text[n] = '\0';
            i = (size_t)(end - src);
        } else {
            kv->type = JT_OTHER;
            set_err_at(err, err_len, "unsupported JSON value", i);
            return false;
        }
        flat->count++;
        skip_ws(src, len, &i);
        if (i < len && src[i] == ',') {
            i++;
            skip_ws(src, len, &i);
            continue;
        }
        if (i < len && src[i] == '}') {
            i++;
            skip_ws(src, len, &i);
            if (i != len) {
                set_err_at(err, err_len, "trailing content", i);
                return false;
            }
            return true;
        }
        set_err_at(err, err_len, "expected ',' or '}'", i);
        return false;
    }
    set_err(err, err_len, "unterminated object");
    return false;
}

bool config_load_file(const char *path, Config *cfg, char *err, size_t err_len) {
    config_init(cfg);
    char *src = NULL;
    size_t len = 0;
    if (!read_all_file(path, &src, &len)) {
        set_err(err, err_len, "cannot read file");
        return false;
    }
    JsonFlat flat;
    bool ok = parse_flat_json(src, len, &flat, err, err_len);
    free(src);
    if (!ok) {
        return false;
    }
    for (int i = 0; i < flat.count; i++) {
        JsonKV *kv = &flat.items[i];
        if (strcmp(kv->key, "indent-size") == 0 && kv->type == JT_INT) {
            cfg->indent_size = kv->int_value;
        } else if (strcmp(kv->key, "use-tabs") == 0 && kv->type == JT_BOOL) {
            cfg->use_tabs = kv->bool_value;
        } else if (strcmp(kv->key, "max-char") == 0 && kv->type == JT_INT) {
            cfg->max_char = kv->int_value;
        } else if (strcmp(kv->key, "space-conditional") == 0 && kv->type == JT_BOOL) {
            cfg->space_conditional = kv->bool_value;
        }
    }
    return true;
}

static void merge_config(CliOptions *opts, const Config *cfg) {
    if (!opts->indent_size_set && cfg->indent_size != -1) {
        opts->indent_size = cfg->indent_size;
    }
    if (!opts->use_tabs_set && cfg->use_tabs != -1) {
        opts->use_tabs = cfg->use_tabs == 1;
    }
    if (!opts->max_char_set && cfg->max_char != -1) {
        opts->max_char = cfg->max_char;
    }
    if (!opts->space_conditional_set && cfg->space_conditional != -1) {
        opts->space_conditional = cfg->space_conditional == 1;
    }
}

void config_resolve(CliOptions *opts) {
    Config local;
    Config global;
    char err[256] = {0};
    char *local_path = absolute_path_for(".easy-on-the-eyes.rc");
    const char *home = getenv("HOME");
    char *global_path = home ? path_join(home, ".easy-on-the-eyes.rc") : NULL;

    if (local_path && path_exists(local_path)) {
        err[0] = '\0';
        if (config_load_file(local_path, &local, err, sizeof(err))) {
            merge_config(opts, &local);
        } else {
            fprintf(stderr, "[WARN] Cannot parse %s: %s. Skipping.\n", local_path, err[0] ? err : "invalid JSON");
        }
    }
    if (global_path && path_exists(global_path)) {
        err[0] = '\0';
        if (config_load_file(global_path, &global, err, sizeof(err))) {
            merge_config(opts, &global);
        } else {
            fprintf(stderr, "[WARN] Cannot parse %s: %s. Skipping.\n", global_path, err[0] ? err : "invalid JSON");
        }
    }
    free(local_path);
    free(global_path);
}

static const char *type_name(JsonType type) {
    switch (type) {
        case JT_INT: return "integer";
        case JT_BOOL: return "boolean";
        case JT_STRING: return "string";
        case JT_NULL: return "null";
        default: return "value";
    }
}

static void print_received(const JsonKV *kv) {
    if (kv->type == JT_STRING) {
        fprintf(stderr, "string (\"%s\")", kv->text);
    } else {
        fprintf(stderr, "%s (%s)", type_name(kv->type), kv->text);
    }
}

static int validate_one(const char *path) {
    if (!path_exists(path)) {
        return 0;
    }
    char *src = NULL;
    size_t len = 0;
    if (!read_all_file(path, &src, &len)) {
        fprintf(stderr, "[ERROR] Cannot parse %s: invalid JSON. Skipping schema check.\n", path);
        return 1;
    }
    JsonFlat flat;
    char err[256] = {0};
    bool ok = parse_flat_json(src, len, &flat, err, sizeof(err));
    free(src);
    if (!ok) {
        fprintf(stderr, "[ERROR] Cannot parse %s: invalid JSON. Skipping schema check.\n", path);
        return 1;
    }
    bool header = false;
    int failures = 0;
    for (int i = 0; i < flat.count; i++) {
        JsonKV *kv = &flat.items[i];
        const char *expected = NULL;
        bool bad = false;
        if (strcmp(kv->key, "indent-size") == 0) {
            expected = "integer";
            bad = kv->type != JT_INT || kv->int_value <= 0;
        } else if (strcmp(kv->key, "use-tabs") == 0) {
            expected = "boolean";
            bad = kv->type != JT_BOOL;
        } else if (strcmp(kv->key, "max-char") == 0) {
            expected = "integer";
            bad = kv->type != JT_INT || kv->int_value < 0;
        } else if (strcmp(kv->key, "space-conditional") == 0) {
            expected = "boolean";
            bad = kv->type != JT_BOOL;
        } else {
            fprintf(stderr, "[WARN] Unknown property \"%s\" in %s.\n", kv->key, path);
            continue;
        }
        if (bad) {
            if (!header) {
                fprintf(stderr, "[ERROR] Invalid Configuration detected in %s:\n", path);
                header = true;
            }
            fprintf(stderr, "    - Property \"%s\" must be an %s. Received ", kv->key, expected);
            print_received(kv);
            fprintf(stderr, ".\n");
            failures++;
        }
    }
    return failures > 0 ? 1 : 0;
}

int config_validate_all(void) {
    int failures = 0;
    char *local_path = absolute_path_for(".easy-on-the-eyes.rc");
    const char *home = getenv("HOME");
    char *global_path = home ? path_join(home, ".easy-on-the-eyes.rc") : NULL;
    if (local_path) {
        failures += validate_one(local_path);
    }
    if (global_path) {
        failures += validate_one(global_path);
    }
    free(local_path);
    free(global_path);
    if (failures == 0) {
        printf("[OK] Configuration is valid.\n");
        return EOTE_EXIT_OK;
    }
    return EOTE_EXIT_ERROR;
}
