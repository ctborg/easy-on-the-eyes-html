#ifndef EOTE_ENGINE_H
#define EOTE_ENGINE_H

#include "cli.h"
#include "config.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    LANG_UNKNOWN,
    LANG_HTML,
    LANG_CSS,
    LANG_JS,
    LANG_JSON
} Language;

Language engine_detect_path(const char *path);
Language engine_detect_stdin(const char *src, size_t len, const CliOptions *opts);
char *engine_format(Language lang, const char *src, size_t src_len, const Config *cfg, char *err, size_t err_len);
int engine_process_file(const char *path, const CliOptions *opts, const Config *cfg, bool emit_header, bool quiet_ok);
int engine_process_stdin(const CliOptions *opts, const Config *cfg);

#endif
