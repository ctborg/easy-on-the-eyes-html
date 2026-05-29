#include "engine_js.h"

#include "util.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

static bool keyword_before(const char *src, size_t pos, const char *kw) {
    size_t klen = strlen(kw);
    if (pos < klen || strncmp(src + pos - klen, kw, klen) != 0) {
        return false;
    }
    if (pos > klen && (isalnum((unsigned char)src[pos - klen - 1]) || src[pos - klen - 1] == '_')) {
        return false;
    }
    return true;
}

static void trim_trailing_spaces(StrBuf *out) {
    while (out->len > 0 && (out->data[out->len - 1] == ' ' || out->data[out->len - 1] == '\t')) {
        out->len--;
        out->data[out->len] = '\0';
    }
}

static bool newline_indent(StrBuf *out, int depth, const Config *cfg) {
    trim_trailing_spaces(out);
    if (out->len == 0 || out->data[out->len - 1] != '\n') {
        if (!sb_append_char(out, '\n')) {
            return false;
        }
    }
    return sb_append_indent(out, depth, cfg->indent_size, cfg->use_tabs == 1);
}

static bool copy_string_js(const char *src, size_t len, size_t *i, StrBuf *out, char quote, char *err, size_t err_len) {
    if (!sb_append_char(out, src[*i])) {
        return false;
    }
    (*i)++;
    bool esc = false;
    while (*i < len) {
        char c = src[*i];
        if (!sb_append_char(out, c)) {
            return false;
        }
        (*i)++;
        if (esc) {
            esc = false;
        } else if (c == '\\') {
            esc = true;
        } else if (c == quote) {
            return true;
        }
    }
    set_err(err, err_len, "unterminated string literal");
    return false;
}

char *engine_js_format(const char *src, size_t src_len, const Config *cfg, char *err, size_t err_len) {
    StrBuf out;
    sb_init(&out);
    int depth = 0;
    bool at_line_start = true;
    for (size_t i = 0; i < src_len;) {
        char c = src[i];
        if (c == '\'' || c == '"' || c == '`') {
            if (!copy_string_js(src, src_len, &i, &out, c, err, err_len)) {
                sb_free(&out);
                return NULL;
            }
            at_line_start = false;
            continue;
        }
        if (c == '/' && i + 1 < src_len && src[i + 1] == '/') {
            while (i < src_len && src[i] != '\n') {
                if (!sb_append_char(&out, src[i++])) {
                    sb_free(&out);
                    return NULL;
                }
            }
            continue;
        }
        if (c == '/' && i + 1 < src_len && src[i + 1] == '*') {
            while (i < src_len) {
                if (!sb_append_char(&out, src[i])) {
                    sb_free(&out);
                    return NULL;
                }
                if (i + 1 < src_len && src[i] == '*' && src[i + 1] == '/') {
                    i++;
                    if (!sb_append_char(&out, src[i++])) {
                        sb_free(&out);
                        return NULL;
                    }
                    break;
                }
                i++;
            }
            continue;
        }
        if (isspace((unsigned char)c)) {
            if (!at_line_start && out.len > 0 && out.data[out.len - 1] != ' ') {
                if (!sb_append_char(&out, ' ')) {
                    sb_free(&out);
                    return NULL;
                }
            }
            i++;
            continue;
        }
        if (c == '{' || c == '[' || c == '(') {
            if (cfg->space_conditional == 1 && c == '(' &&
                (keyword_before(out.data ? out.data : "", out.len, "if") ||
                 keyword_before(out.data ? out.data : "", out.len, "for") ||
                 keyword_before(out.data ? out.data : "", out.len, "while") ||
                keyword_before(out.data ? out.data : "", out.len, "switch"))) {
                if (out.len > 0 && out.data[out.len - 1] != ' ') {
                    if (!sb_append_char(&out, ' ')) {
                        sb_free(&out);
                        return NULL;
                    }
                }
            }
            if (!sb_append_char(&out, c)) {
                sb_free(&out);
                return NULL;
            }
            depth++;
            if (c != '(') {
                if (!newline_indent(&out, depth, cfg)) {
                    sb_free(&out);
                    return NULL;
                }
                at_line_start = true;
            } else {
                at_line_start = false;
            }
            i++;
            continue;
        }
        if (c == '}' || c == ']' || c == ')') {
            if (depth == 0) {
                set_err_at(err, err_len, "unmatched closing delimiter", i);
                sb_free(&out);
                return NULL;
            }
            depth--;
            if (c != ')') {
                if (!newline_indent(&out, depth, cfg)) {
                    sb_free(&out);
                    return NULL;
                }
            }
            if (!sb_append_char(&out, c)) {
                sb_free(&out);
                return NULL;
            }
            at_line_start = false;
            i++;
            continue;
        }
        if (c == ';' || c == ',') {
            if (!sb_append_char(&out, c)) {
                sb_free(&out);
                return NULL;
            }
            if (c == ';') {
                if (!newline_indent(&out, depth, cfg)) {
                    sb_free(&out);
                    return NULL;
                }
                at_line_start = true;
            } else {
                if (!sb_append_char(&out, ' ')) {
                    sb_free(&out);
                    return NULL;
                }
            }
            i++;
            continue;
        }
        if (at_line_start) {
            if (!sb_append_indent(&out, depth, cfg->indent_size, cfg->use_tabs == 1)) {
                sb_free(&out);
                return NULL;
            }
            at_line_start = false;
        }
        if (!sb_append_char(&out, c)) {
            sb_free(&out);
            return NULL;
        }
        i++;
    }
    if (depth != 0) {
        set_err(err, err_len, "unclosed block delimiter");
        sb_free(&out);
        return NULL;
    }
    trim_trailing_spaces(&out);
    if (!sb_append_char(&out, '\n')) {
        sb_free(&out);
        return NULL;
    }
    return sb_take(&out);
}
