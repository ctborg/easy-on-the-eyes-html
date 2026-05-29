#include "cli.h"

#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool parse_int_arg(const char *s, int *out) {
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (!s[0] || (end && *end != '\0') || v < -1 || v > 1000000) {
        return false;
    }
    *out = (int)v;
    return true;
}

static bool parse_bool_arg(const char *s, bool *out) {
    if (strcmp(s, "true") == 0) {
        *out = true;
        return true;
    }
    if (strcmp(s, "false") == 0) {
        *out = false;
        return true;
    }
    return false;
}

static bool valid_lang(const char *s) {
    return strcmp(s, "html") == 0 || strcmp(s, "css") == 0 ||
           strcmp(s, "js") == 0 || strcmp(s, "json") == 0;
}

void cli_init(CliOptions *opts) {
    memset(opts, 0, sizeof(*opts));
    opts->indent_size = 4;
    opts->max_char = 0;
    opts->space_conditional = true;
    snprintf(opts->lang, sizeof(opts->lang), "%s", "js");
}

void cli_free(CliOptions *opts) {
    if (opts->targets) {
        for (int i = 0; i < opts->target_count; i++) {
            free(opts->targets[i]);
        }
        free(opts->targets);
    }
    opts->targets = NULL;
    opts->target_count = 0;
}

static int add_target(CliOptions *opts, const char *target) {
    char **next = (char **)realloc(opts->targets, sizeof(char *) * (size_t)(opts->target_count + 1));
    if (!next) {
        return EOTE_EXIT_ERROR;
    }
    opts->targets = next;
    opts->targets[opts->target_count] = eote_strdup(target);
    if (!opts->targets[opts->target_count]) {
        return EOTE_EXIT_ERROR;
    }
    opts->target_count++;
    return EOTE_EXIT_OK;
}

int cli_parse(CliOptions *opts, int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "-w") == 0 || strcmp(arg, "--write") == 0) {
            opts->write = true;
        } else if (strcmp(arg, "-c") == 0 || strcmp(arg, "--check") == 0) {
            opts->check = true;
        } else if (strcmp(arg, "-q") == 0 || strcmp(arg, "--quiet") == 0) {
            opts->quiet = true;
        } else if (strcmp(arg, "--use-tabs") == 0) {
            opts->use_tabs = true;
            opts->use_tabs_set = true;
        } else if (strcmp(arg, "--validate") == 0) {
            opts->validate = true;
        } else if (strcmp(arg, "-v") == 0 || strcmp(arg, "--version") == 0) {
            cli_print_version();
            return 100;
        } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            cli_print_help();
            return 100;
        } else if (strcmp(arg, "-i") == 0 || strcmp(arg, "--indent-size") == 0) {
            if (++i >= argc || !parse_int_arg(argv[i], &opts->indent_size)) {
                fprintf(stderr, "[ERROR] --indent-size requires an integer.\n");
                return EOTE_EXIT_ERROR;
            }
            opts->indent_size_set = true;
        } else if (strcmp(arg, "--max-char") == 0) {
            if (++i >= argc || !parse_int_arg(argv[i], &opts->max_char)) {
                fprintf(stderr, "[ERROR] --max-char requires an integer.\n");
                return EOTE_EXIT_ERROR;
            }
            opts->max_char_set = true;
        } else if (strcmp(arg, "--space-conditional") == 0) {
            if (++i >= argc || !parse_bool_arg(argv[i], &opts->space_conditional)) {
                fprintf(stderr, "[ERROR] --space-conditional requires true or false.\n");
                return EOTE_EXIT_ERROR;
            }
            opts->space_conditional_set = true;
        } else if (strcmp(arg, "--lang") == 0) {
            if (++i >= argc || !valid_lang(argv[i])) {
                fprintf(stderr, "[ERROR] --lang requires html, css, js, or json.\n");
                return EOTE_EXIT_ERROR;
            }
            snprintf(opts->lang, sizeof(opts->lang), "%s", argv[i]);
            opts->lang_set = true;
        } else if (arg[0] == '-' && arg[1] != '\0') {
            fprintf(stderr, "[ERROR] Unknown option: %s\n", arg);
            return EOTE_EXIT_ERROR;
        } else {
            int rc = add_target(opts, arg);
            if (rc != EOTE_EXIT_OK) {
                return rc;
            }
        }
    }
    opts->stdin_mode = opts->target_count == 0 && !isatty(STDIN_FILENO);
    if (opts->write && opts->check) {
        fprintf(stderr, "[WARN] --write and --check options cannot be active at the same time. Defaulting to safe verification mode (--check).\n");
        opts->write = false;
    }
    return EOTE_EXIT_OK;
}

void cli_print_version(void) {
    printf("easy-on-the-eyes v1.0.0\n");
}

void cli_print_help(void) {
    printf("Usage: easy-on-the-eyes [options] [files|dirs|globs...]\n\n");
    printf("Options:\n");
    printf("  -w, --write                 overwrite files in place\n");
    printf("  -c, --check                 verify formatting without writing\n");
    printf("  -q, --quiet                 suppress non-error output\n");
    printf("  -i, --indent-size <int>     set indentation size\n");
    printf("      --use-tabs              indent with tabs\n");
    printf("      --max-char <int>        wrap long lines when non-zero\n");
    printf("      --space-conditional <bool>  control spaces before conditionals\n");
    printf("      --lang <html|css|js|json>   language for stdin mode\n");
    printf("      --validate              validate configuration files\n");
    printf("  -v, --version               show version\n");
    printf("  -h, --help                  show this help\n\n");
    printf("Examples:\n");
    printf("  easy-on-the-eyes src/index.js\n");
    printf("  easy-on-the-eyes -w \"src/**/*.js\" --quiet\n");
    printf("  cat file.json | easy-on-the-eyes --lang json\n");
}
