#ifndef EOTE_CONFIG_H
#define EOTE_CONFIG_H

#include "cli.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int indent_size;
    int use_tabs;
    int max_char;
    int space_conditional;
} Config;

void config_init(Config *cfg);
void config_from_options(const CliOptions *opts, Config *cfg);
bool config_load_file(const char *path, Config *cfg, char *err, size_t err_len);
void config_resolve(CliOptions *opts);
int config_validate_all(void);

#endif
