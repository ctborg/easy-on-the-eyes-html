#ifndef EOTE_IGNORE_H
#define EOTE_IGNORE_H

#include <stdbool.h>

typedef struct {
    char **patterns;
    int count;
} IgnoreRules;

void ignore_init(IgnoreRules *rules);
void ignore_free(IgnoreRules *rules);
void ignore_load(IgnoreRules *rules);
bool ignore_matches(const IgnoreRules *rules, const char *path);
bool glob_match(const char *pattern, const char *path);

#endif
