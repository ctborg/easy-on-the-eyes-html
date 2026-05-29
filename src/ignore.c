#include "ignore.h"

#include "util.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ignore_init(IgnoreRules *rules) {
    rules->patterns = NULL;
    rules->count = 0;
}

void ignore_free(IgnoreRules *rules) {
    for (int i = 0; i < rules->count; i++) {
        free(rules->patterns[i]);
    }
    free(rules->patterns);
    rules->patterns = NULL;
    rules->count = 0;
}

static bool add_pattern(IgnoreRules *rules, const char *pattern) {
    char **next = (char **)realloc(rules->patterns, sizeof(char *) * (size_t)(rules->count + 1));
    if (!next) {
        return false;
    }
    rules->patterns = next;
    rules->patterns[rules->count] = eote_strdup(pattern);
    if (!rules->patterns[rules->count]) {
        return false;
    }
    rules->count++;
    return true;
}

void ignore_load(IgnoreRules *rules) {
    add_pattern(rules, "**/.git/**");
    add_pattern(rules, ".git/**");
    add_pattern(rules, "**/node_modules/**");
    add_pattern(rules, "node_modules/**");
    char *src = NULL;
    size_t len = 0;
    if (!read_all_file(".easy-on-the-eyes.ignore", &src, &len)) {
        return;
    }
    size_t start = 0;
    for (size_t i = 0; i <= len; i++) {
        if (i == len || src[i] == '\n') {
            size_t end = i;
            while (end > start && (src[end - 1] == '\r' || isspace((unsigned char)src[end - 1]))) {
                end--;
            }
            while (start < end && isspace((unsigned char)src[start])) {
                start++;
            }
            if (end > start && src[start] != '#') {
                char *line = eote_strndup(src + start, end - start);
                if (line) {
                    add_pattern(rules, line);
                    free(line);
                }
            }
            start = i + 1;
        }
    }
    free(src);
}

static bool glob_match_here(const char *p, const char *s) {
    if (*p == '\0') {
        return *s == '\0';
    }
    if (p[0] == '*' && p[1] == '*') {
        p += 2;
        if (*p == '/') {
            p++;
            if (glob_match_here(p, s)) {
                return true;
            }
        }
        for (const char *t = s; ; t++) {
            if (glob_match_here(p, t)) {
                return true;
            }
            if (*t == '\0') {
                break;
            }
        }
        return false;
    }
    if (*p == '*') {
        p++;
        const char *t = s;
        while (true) {
            if (glob_match_here(p, t)) {
                return true;
            }
            if (*t == '\0' || *t == '/') {
                return false;
            }
            t++;
        }
    }
    if (*p == '?') {
        return *s != '\0' && *s != '/' && glob_match_here(p + 1, s + 1);
    }
    if (*p == *s) {
        return glob_match_here(p + 1, s + 1);
    }
    return false;
}

bool glob_match(const char *pattern, const char *path) {
    return glob_match_here(pattern, path);
}

bool ignore_matches(const IgnoreRules *rules, const char *path) {
    for (int i = 0; i < rules->count; i++) {
        if (glob_match(rules->patterns[i], path)) {
            return true;
        }
    }
    return false;
}
