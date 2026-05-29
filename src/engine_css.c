#include "engine_css.h"

#include "util.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

static size_t trim_start(const char *s, size_t a, size_t b) {
    while (a < b && isspace((unsigned char)s[a])) {
        a++;
    }
    return a;
}

static size_t trim_end(const char *s, size_t a, size_t b) {
    (void)a;
    while (b > 0 && isspace((unsigned char)s[b - 1])) {
        b--;
    }
    return b;
}

static bool append_trimmed(StrBuf *out, const char *s, size_t a, size_t b) {
    a = trim_start(s, a, b);
    b = trim_end(s, a, b);
    return b > a ? sb_append_n(out, s + a, b - a) : true;
}

char *engine_css_format(const char *src, size_t src_len, const Config *cfg, char *err, size_t err_len) {
    StrBuf out;
    sb_init(&out);
    size_t i = 0;
    bool first_rule = true;
    while (i < src_len) {
        i = trim_start(src, i, src_len);
        if (i >= src_len) {
            break;
        }
        size_t sel_start = i;
        bool in_comment = false;
        while (i < src_len) {
            if (!in_comment && i + 1 < src_len && src[i] == '/' && src[i + 1] == '*') {
                in_comment = true;
                i += 2;
                continue;
            }
            if (in_comment && i + 1 < src_len && src[i] == '*' && src[i + 1] == '/') {
                in_comment = false;
                i += 2;
                continue;
            }
            if (!in_comment && src[i] == '{') {
                break;
            }
            i++;
        }
        if (i >= src_len) {
            set_err(err, err_len, "expected '{'");
            sb_free(&out);
            return NULL;
        }
        if (!first_rule && !sb_append(&out, "\n\n")) {
            sb_free(&out);
            return NULL;
        }
        first_rule = false;
        if (!append_trimmed(&out, src, sel_start, i) || !sb_append(&out, " {\n")) {
            sb_free(&out);
            return NULL;
        }
        i++;
        size_t decl_start = i;
        in_comment = false;
        while (i < src_len) {
            if (!in_comment && i + 1 < src_len && src[i] == '/' && src[i + 1] == '*') {
                in_comment = true;
                i += 2;
                continue;
            }
            if (in_comment && i + 1 < src_len && src[i] == '*' && src[i + 1] == '/') {
                in_comment = false;
                i += 2;
                continue;
            }
            if (!in_comment && (src[i] == ';' || src[i] == '}')) {
                size_t end = i;
                size_t a = trim_start(src, decl_start, end);
                size_t b = trim_end(src, a, end);
                if (b > a) {
                    if (!sb_append_indent(&out, 1, cfg->indent_size, cfg->use_tabs == 1) ||
                        !sb_append_n(&out, src + a, b - a) ||
                        !sb_append(&out, ";\n")) {
                        sb_free(&out);
                        return NULL;
                    }
                }
                if (src[i] == '}') {
                    break;
                }
                i++;
                decl_start = i;
                continue;
            }
            i++;
        }
        if (i >= src_len || src[i] != '}') {
            set_err(err, err_len, "expected '}'");
            sb_free(&out);
            return NULL;
        }
        if (!sb_append(&out, "}")) {
            sb_free(&out);
            return NULL;
        }
        i++;
    }
    if (!sb_append_char(&out, '\n')) {
        sb_free(&out);
        return NULL;
    }
    return sb_take(&out);
}
