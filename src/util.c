#include "util.h"

#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void sb_init(StrBuf *sb) {
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
}

void sb_free(StrBuf *sb) {
    free(sb->data);
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
}

bool sb_reserve(StrBuf *sb, size_t add) {
    if (sb->len > SIZE_MAX - 1 || add > SIZE_MAX - sb->len - 1) {
        return false;
    }
    size_t need = sb->len + add + 1;
    if (need <= sb->cap) {
        return true;
    }
    size_t cap = sb->cap ? sb->cap : 128;
    while (cap < need) {
        if (cap > SIZE_MAX / 2) {
            cap = need;
            break;
        }
        cap *= 2;
    }
    char *next = (char *)realloc(sb->data, cap);
    if (!next) {
        return false;
    }
    sb->data = next;
    sb->cap = cap;
    return true;
}

bool sb_append_n(StrBuf *sb, const char *s, size_t n) {
    if (!sb_reserve(sb, n)) {
        return false;
    }
    memcpy(sb->data + sb->len, s, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
    return true;
}

bool sb_append(StrBuf *sb, const char *s) {
    return sb_append_n(sb, s, strlen(s));
}

bool sb_append_char(StrBuf *sb, char c) {
    return sb_append_n(sb, &c, 1);
}

bool sb_append_indent(StrBuf *sb, int level, int indent_size, bool use_tabs) {
    for (int i = 0; i < level; i++) {
        if (use_tabs) {
            if (!sb_append_char(sb, '\t')) {
                return false;
            }
        } else {
            for (int j = 0; j < indent_size; j++) {
                if (!sb_append_char(sb, ' ')) {
                    return false;
                }
            }
        }
    }
    return true;
}

char *sb_take(StrBuf *sb) {
    if (!sb->data && !sb_append_char(sb, '\0')) {
        return NULL;
    }
    char *data = sb->data;
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
    return data;
}

char *eote_strdup(const char *s) {
    return eote_strndup(s, strlen(s));
}

char *eote_strndup(const char *s, size_t n) {
    if (n == SIZE_MAX) {
        return NULL;
    }
    char *out = (char *)malloc(n + 1);
    if (!out) {
        return NULL;
    }
    memcpy(out, s, n);
    out[n] = '\0';
    return out;
}

bool read_all_file(const char *path, char **out, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    StrBuf sb;
    sb_init(&sb);
    char chunk[8192];
    size_t n;
    while ((n = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        if (!sb_append_n(&sb, chunk, n)) {
            fclose(f);
            sb_free(&sb);
            return false;
        }
    }
    if (ferror(f)) {
        fclose(f);
        sb_free(&sb);
        return false;
    }
    fclose(f);
    *out_len = sb.len;
    *out = sb_take(&sb);
    return *out != NULL;
}

bool write_all_file(const char *path, const char *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        return false;
    }
    bool ok = fwrite(data, 1, len, f) == len;
    if (fclose(f) != 0) {
        ok = false;
    }
    return ok;
}

char *read_all_stdin(size_t *out_len) {
    StrBuf sb;
    sb_init(&sb);
    char chunk[8192];
    size_t n;
    while ((n = fread(chunk, 1, sizeof(chunk), stdin)) > 0) {
        if (!sb_append_n(&sb, chunk, n)) {
            sb_free(&sb);
            return NULL;
        }
    }
    *out_len = sb.len;
    return sb_take(&sb);
}

bool path_is_dir(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

bool path_is_symlink(const char *path) {
    struct stat st;
    return lstat(path, &st) == 0 && S_ISLNK(st.st_mode);
}

bool path_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

bool has_glob_chars(const char *s) {
    return strchr(s, '*') != NULL || strchr(s, '?') != NULL;
}

const char *path_ext(const char *path) {
    const char *slash = strrchr(path, '/');
    const char *dot = strrchr(path, '.');
    if (!dot || (slash && dot < slash)) {
        return "";
    }
    return dot;
}

char *path_join(const char *a, const char *b) {
    size_t alen = strlen(a);
    size_t blen = strlen(b);
    bool slash = alen > 0 && a[alen - 1] == '/';
    size_t extra = slash ? 1 : 2;
    if (alen > SIZE_MAX - blen - extra) {
        return NULL;
    }
    char *out = (char *)malloc(alen + blen + extra);
    if (!out) {
        return NULL;
    }
    memcpy(out, a, alen);
    size_t pos = alen;
    if (!slash) {
        out[pos++] = '/';
    }
    memcpy(out + pos, b, blen);
    out[pos + blen] = '\0';
    return out;
}

void set_err(char *err, size_t err_len, const char *msg) {
    if (!err || err_len == 0 || err[0] != '\0') {
        return;
    }
    snprintf(err, err_len, "%s", msg);
}

void set_err_at(char *err, size_t err_len, const char *msg, size_t pos) {
    if (!err || err_len == 0 || err[0] != '\0') {
        return;
    }
    snprintf(err, err_len, "%s at byte %zu", msg, pos);
}

char *absolute_path_for(const char *path) {
    if (path[0] == '/') {
        return eote_strdup(path);
    }
    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd))) {
        return eote_strdup(path);
    }
    return path_join(cwd, path);
}
