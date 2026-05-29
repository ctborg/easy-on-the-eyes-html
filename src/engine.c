#include "engine.h"

#include "engine_css.h"
#include "engine_html.h"
#include "engine_js.h"
#include "engine_json.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Language engine_detect_path(const char *path) {
    const char *ext = path_ext(path);
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) {
        return LANG_HTML;
    }
    if (strcmp(ext, ".css") == 0) {
        return LANG_CSS;
    }
    if (strcmp(ext, ".js") == 0) {
        return LANG_JS;
    }
    if (strcmp(ext, ".json") == 0) {
        return LANG_JSON;
    }
    return LANG_UNKNOWN;
}

Language engine_detect_stdin(const char *src, size_t len, const CliOptions *opts) {
    if (opts->lang_set) {
        if (strcmp(opts->lang, "html") == 0) {
            return LANG_HTML;
        }
        if (strcmp(opts->lang, "css") == 0) {
            return LANG_CSS;
        }
        if (strcmp(opts->lang, "json") == 0) {
            return LANG_JSON;
        }
        return LANG_JS;
    }
    size_t n = len < 512 ? len : 512;
    if (n >= 5 && (strstr(src, "<!DOCTYPE") || strstr(src, "<html"))) {
        return LANG_HTML;
    }
    if (memchr(src, '{', n) && memchr(src, '"', n)) {
        return LANG_JSON;
    }
    if (memchr(src, '{', n) && memchr(src, ':', n) && memchr(src, ';', n)) {
        return LANG_CSS;
    }
    return LANG_JS;
}

char *engine_format(Language lang, const char *src, size_t src_len, const Config *cfg, char *err, size_t err_len) {
    switch (lang) {
        case LANG_HTML:
            return engine_html_format(src, src_len, cfg, err, err_len);
        case LANG_CSS:
            return engine_css_format(src, src_len, cfg, err, err_len);
        case LANG_JS:
            return engine_js_format(src, src_len, cfg, err, err_len);
        case LANG_JSON:
            return engine_json_format(src, src_len, cfg, err, err_len);
        default:
            set_err(err, err_len, "unknown language");
            return NULL;
    }
}

int engine_process_file(const char *path, const CliOptions *opts, const Config *cfg, bool emit_header, bool quiet_ok) {
    Language lang = engine_detect_path(path);
    if (lang == LANG_UNKNOWN) {
        return EOTE_EXIT_OK;
    }
    char *src = NULL;
    size_t src_len = 0;
    if (!read_all_file(path, &src, &src_len)) {
        fprintf(stderr, "[ERROR] Failed to parse %s: cannot read file\n", path);
        return EOTE_EXIT_ERROR;
    }
    char err[256] = {0};
    char *formatted = engine_format(lang, src, src_len, cfg, err, sizeof(err));
    if (!formatted) {
        fprintf(stderr, "[ERROR] Failed to parse %s: %s\n", path, err[0] ? err : "unknown parse error");
        free(src);
        return EOTE_EXIT_ERROR;
    }
    int rc = EOTE_EXIT_OK;
    if (opts->check) {
        size_t formatted_len = strlen(formatted);
        if (src_len != formatted_len || memcmp(src, formatted, src_len) != 0) {
            fprintf(stderr, "[FAILURE] Codebase styles out of alignment: %s requires alignment.\n", path);
            rc = EOTE_EXIT_CHECK_FAILED;
        } else if (!opts->quiet && quiet_ok) {
            printf("[OK] %s\n", path);
        }
    } else if (opts->write) {
        if (!write_all_file(path, formatted, strlen(formatted))) {
            fprintf(stderr, "[ERROR] Failed to parse %s: cannot write file\n", path);
            rc = EOTE_EXIT_ERROR;
        }
    } else {
        if (emit_header) {
            printf("### %s ###\n", path);
        }
        fputs(formatted, stdout);
    }
    free(src);
    free(formatted);
    return rc;
}

int engine_process_stdin(const CliOptions *opts, const Config *cfg) {
    size_t src_len = 0;
    char *src = read_all_stdin(&src_len);
    if (!src) {
        fprintf(stderr, "[ERROR] Failed to parse <stdin>: cannot read stdin\n");
        return EOTE_EXIT_ERROR;
    }
    Language lang = engine_detect_stdin(src, src_len, opts);
    char err[256] = {0};
    char *formatted = engine_format(lang, src, src_len, cfg, err, sizeof(err));
    if (!formatted) {
        fprintf(stderr, "[ERROR] Failed to parse <stdin>: %s\n", err[0] ? err : "unknown parse error");
        free(src);
        return EOTE_EXIT_ERROR;
    }
    int rc = EOTE_EXIT_OK;
    if (opts->check) {
        if (!opts->quiet) {
            printf("[STATUS] Checking code uniformity constraints...\n");
            fflush(stdout);
        }
        size_t formatted_len = strlen(formatted);
        if (src_len != formatted_len || memcmp(src, formatted, src_len) != 0) {
            fprintf(stderr, "[FAILURE] Codebase styles out of alignment: <stdin> requires alignment.\n");
            rc = EOTE_EXIT_CHECK_FAILED;
        } else if (!opts->quiet) {
            printf("[OK] <stdin>\n");
        }
    } else {
        fputs(formatted, stdout);
    }
    free(src);
    free(formatted);
    return rc;
}
