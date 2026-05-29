#ifndef EOTE_CLI_H
#define EOTE_CLI_H

#include <stdbool.h>

typedef struct {
    bool write;
    bool check;
    bool quiet;
    bool use_tabs;
    bool validate;
    int indent_size;
    int max_char;
    bool space_conditional;
    char **targets;
    int target_count;
    bool stdin_mode;
    bool lang_set;
    char lang[16];
    bool indent_size_set;
    bool max_char_set;
    bool use_tabs_set;
    bool space_conditional_set;
} CliOptions;

void cli_init(CliOptions *opts);
void cli_free(CliOptions *opts);
int cli_parse(CliOptions *opts, int argc, char **argv);
void cli_print_help(void);
void cli_print_version(void);

#endif
