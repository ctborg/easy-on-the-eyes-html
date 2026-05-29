#ifndef EOTE_UTIL_H
#define EOTE_UTIL_H

#include <stdbool.h>
#include <stddef.h>

#define EOTE_EXIT_OK 0
#define EOTE_EXIT_CHECK_FAILED 1
#define EOTE_EXIT_ERROR 2

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} StrBuf;

void sb_init(StrBuf *sb);
void sb_free(StrBuf *sb);
bool sb_reserve(StrBuf *sb, size_t add);
bool sb_append_n(StrBuf *sb, const char *s, size_t n);
bool sb_append(StrBuf *sb, const char *s);
bool sb_append_char(StrBuf *sb, char c);
bool sb_append_indent(StrBuf *sb, int level, int indent_size, bool use_tabs);
char *sb_take(StrBuf *sb);

char *eote_strdup(const char *s);
char *eote_strndup(const char *s, size_t n);
bool read_all_file(const char *path, char **out, size_t *out_len);
bool write_all_file(const char *path, const char *data, size_t len);
char *read_all_stdin(size_t *out_len);
bool path_is_dir(const char *path);
bool path_is_symlink(const char *path);
bool path_exists(const char *path);
bool has_glob_chars(const char *s);
const char *path_ext(const char *path);
char *path_join(const char *a, const char *b);
void set_err(char *err, size_t err_len, const char *msg);
void set_err_at(char *err, size_t err_len, const char *msg, size_t pos);
char *absolute_path_for(const char *path);

#endif
