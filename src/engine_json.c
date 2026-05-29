#include "engine_json.h"

#include "util.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

static void skip_ws_json(const char *s, size_t len, size_t *i) {
    while (*i < len && isspace((unsigned char)s[*i])) {
        (*i)++;
    }
}

static bool copy_string(const char *s, size_t len, size_t *i, StrBuf *out, char *err, size_t err_len) {
    if (*i >= len || s[*i] != '"') {
        set_err_at(err, err_len, "expected string", *i);
        return false;
    }
    if (!sb_append_char(out, s[*i])) {
        return false;
    }
    (*i)++;
    bool esc = false;
    while (*i < len) {
        char c = s[*i];
        if (!sb_append_char(out, c)) {
            return false;
        }
        (*i)++;
        if (esc) {
            esc = false;
        } else if (c == '\\') {
            esc = true;
        } else if (c == '"') {
            return true;
        }
    }
    set_err(err, err_len, "unterminated string");
    return false;
}

static bool parse_value(const char *s, size_t len, size_t *i, StrBuf *out, const Config *cfg, int depth, char *err, size_t err_len);

static bool parse_array(const char *s, size_t len, size_t *i, StrBuf *out, const Config *cfg, int depth, char *err, size_t err_len) {
    if (!sb_append_char(out, '[')) {
        return false;
    }
    (*i)++;
    skip_ws_json(s, len, i);
    if (*i < len && s[*i] == ']') {
        (*i)++;
        return sb_append_char(out, ']');
    }
    if (!sb_append_char(out, '\n')) {
        return false;
    }
    while (*i < len) {
        if (!sb_append_indent(out, depth + 1, cfg->indent_size, cfg->use_tabs == 1)) {
            return false;
        }
        if (!parse_value(s, len, i, out, cfg, depth + 1, err, err_len)) {
            return false;
        }
        skip_ws_json(s, len, i);
        if (*i < len && s[*i] == ',') {
            (*i)++;
            skip_ws_json(s, len, i);
            if (*i < len && s[*i] == ']') {
                (*i)++;
                if (!sb_append_char(out, '\n') || !sb_append_indent(out, depth, cfg->indent_size, cfg->use_tabs == 1)) {
                    return false;
                }
                return sb_append_char(out, ']');
            }
            if (!sb_append(out, ",\n")) {
                return false;
            }
            continue;
        }
        if (*i < len && s[*i] == ']') {
            (*i)++;
            if (!sb_append_char(out, '\n') || !sb_append_indent(out, depth, cfg->indent_size, cfg->use_tabs == 1)) {
                return false;
            }
            return sb_append_char(out, ']');
        }
        set_err_at(err, err_len, "expected ',' or ']'", *i);
        return false;
    }
    set_err(err, err_len, "unterminated array");
    return false;
}

static bool parse_object(const char *s, size_t len, size_t *i, StrBuf *out, const Config *cfg, int depth, char *err, size_t err_len) {
    if (!sb_append_char(out, '{')) {
        return false;
    }
    (*i)++;
    skip_ws_json(s, len, i);
    if (*i < len && s[*i] == '}') {
        (*i)++;
        return sb_append_char(out, '}');
    }
    if (!sb_append_char(out, '\n')) {
        return false;
    }
    while (*i < len) {
        if (!sb_append_indent(out, depth + 1, cfg->indent_size, cfg->use_tabs == 1) ||
            !copy_string(s, len, i, out, err, err_len)) {
            return false;
        }
        skip_ws_json(s, len, i);
        if (*i >= len || s[*i] != ':') {
            set_err_at(err, err_len, "expected ':'", *i);
            return false;
        }
        (*i)++;
        if (!sb_append(out, ": ")) {
            return false;
        }
        skip_ws_json(s, len, i);
        if (!parse_value(s, len, i, out, cfg, depth + 1, err, err_len)) {
            return false;
        }
        skip_ws_json(s, len, i);
        if (*i < len && s[*i] == ',') {
            (*i)++;
            skip_ws_json(s, len, i);
            if (*i < len && s[*i] == '}') {
                (*i)++;
                if (!sb_append_char(out, '\n') || !sb_append_indent(out, depth, cfg->indent_size, cfg->use_tabs == 1)) {
                    return false;
                }
                return sb_append_char(out, '}');
            }
            if (!sb_append(out, ",\n")) {
                return false;
            }
            continue;
        }
        if (*i < len && s[*i] == '}') {
            (*i)++;
            if (!sb_append_char(out, '\n') || !sb_append_indent(out, depth, cfg->indent_size, cfg->use_tabs == 1)) {
                return false;
            }
            return sb_append_char(out, '}');
        }
        set_err_at(err, err_len, "expected ',' or '}'", *i);
        return false;
    }
    set_err(err, err_len, "unterminated object");
    return false;
}

static bool parse_literal_or_number(const char *s, size_t len, size_t *i, StrBuf *out, char *err, size_t err_len) {
    size_t start = *i;
    if (strncmp(s + *i, "true", 4) == 0) {
        *i += 4;
    } else if (strncmp(s + *i, "false", 5) == 0) {
        *i += 5;
    } else if (strncmp(s + *i, "null", 4) == 0) {
        *i += 4;
    } else {
        if (s[*i] == '-') {
            (*i)++;
        }
        if (*i >= len || !isdigit((unsigned char)s[*i])) {
            set_err_at(err, err_len, "expected value", start);
            return false;
        }
        while (*i < len && (isdigit((unsigned char)s[*i]) || s[*i] == '.' || s[*i] == 'e' || s[*i] == 'E' || s[*i] == '+' || s[*i] == '-')) {
            (*i)++;
        }
    }
    return sb_append_n(out, s + start, *i - start);
}

static bool parse_value(const char *s, size_t len, size_t *i, StrBuf *out, const Config *cfg, int depth, char *err, size_t err_len) {
    skip_ws_json(s, len, i);
    if (*i >= len) {
        set_err(err, err_len, "expected value");
        return false;
    }
    if (s[*i] == '{') {
        return parse_object(s, len, i, out, cfg, depth, err, err_len);
    }
    if (s[*i] == '[') {
        return parse_array(s, len, i, out, cfg, depth, err, err_len);
    }
    if (s[*i] == '"') {
        return copy_string(s, len, i, out, err, err_len);
    }
    return parse_literal_or_number(s, len, i, out, err, err_len);
}

char *engine_json_format(const char *src, size_t src_len, const Config *cfg, char *err, size_t err_len) {
    StrBuf out;
    sb_init(&out);
    size_t i = 0;
    if (!parse_value(src, src_len, &i, &out, cfg, 0, err, err_len)) {
        sb_free(&out);
        return NULL;
    }
    skip_ws_json(src, src_len, &i);
    if (i != src_len) {
        set_err_at(err, err_len, "trailing content", i);
        sb_free(&out);
        return NULL;
    }
    if (!sb_append_char(&out, '\n')) {
        sb_free(&out);
        return NULL;
    }
    return sb_take(&out);
}
