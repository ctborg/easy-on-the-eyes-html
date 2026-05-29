#include "walker.h"

#include "engine.h"
#include "util.h"

#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char **items;
    int count;
    int cap;
} PathList;

typedef struct {
    const CliOptions *opts;
    const Config *cfg;
    PathList *paths;
    int next;
    int worst;
    pthread_mutex_t mu;
} WorkQueue;

static void paths_init(PathList *list) {
    list->items = NULL;
    list->count = 0;
    list->cap = 0;
}

static void paths_free(PathList *list) {
    for (int i = 0; i < list->count; i++) {
        free(list->items[i]);
    }
    free(list->items);
}

static bool paths_add(PathList *list, const char *path) {
    if (list->count == list->cap) {
        int cap = list->cap ? list->cap * 2 : 32;
        char **next = (char **)realloc(list->items, sizeof(char *) * (size_t)cap);
        if (!next) {
            return false;
        }
        list->items = next;
        list->cap = cap;
    }
    list->items[list->count] = eote_strdup(path);
    if (!list->items[list->count]) {
        return false;
    }
    list->count++;
    return true;
}

static bool should_take(const IgnoreRules *rules, const char *path) {
    return engine_detect_path(path) != LANG_UNKNOWN && !ignore_matches(rules, path);
}

static void walk_dir(PathList *list, const IgnoreRules *rules, const char *dir) {
    DIR *d = opendir(dir);
    if (!d) {
        return;
    }
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        char *path = path_join(dir, ent->d_name);
        if (!path) {
            continue;
        }
        if (ignore_matches(rules, path)) {
            free(path);
            continue;
        }
        if (path_is_symlink(path)) {
            free(path);
            continue;
        }
        if (path_is_dir(path)) {
            walk_dir(list, rules, path);
        } else if (should_take(rules, path)) {
            paths_add(list, path);
        }
        free(path);
    }
    closedir(d);
}

static const char *glob_root(const char *pattern, char *buf, size_t buf_len) {
    size_t last_slash = 0;
    for (size_t i = 0; pattern[i]; i++) {
        if (pattern[i] == '*' || pattern[i] == '?') {
            break;
        }
        if (pattern[i] == '/') {
            last_slash = i;
        }
    }
    if (last_slash == 0) {
        snprintf(buf, buf_len, "%s", ".");
    } else {
        size_t n = last_slash;
        if (n >= buf_len) {
            n = buf_len - 1;
        }
        memcpy(buf, pattern, n);
        buf[n] = '\0';
    }
    return buf;
}

static void walk_glob(PathList *list, const IgnoreRules *rules, const char *root, const char *pattern) {
    DIR *d = opendir(root);
    if (!d) {
        return;
    }
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        char *path = path_join(root, ent->d_name);
        if (!path) {
            continue;
        }
        if (!ignore_matches(rules, path)) {
            if (path_is_symlink(path)) {
                /* Do not follow symlinks during recursive walks. */
            } else if (path_is_dir(path)) {
                walk_glob(list, rules, path, pattern);
            } else if (engine_detect_path(path) != LANG_UNKNOWN && glob_match(pattern, path)) {
                paths_add(list, path);
            }
        }
        free(path);
    }
    closedir(d);
}

static void record_rc(WorkQueue *q, int rc) {
    if (rc == EOTE_EXIT_ERROR) {
        q->worst = EOTE_EXIT_ERROR;
    } else if (rc == EOTE_EXIT_CHECK_FAILED && q->worst == EOTE_EXIT_OK) {
        q->worst = EOTE_EXIT_CHECK_FAILED;
    }
}

static void *worker_main(void *arg) {
    WorkQueue *q = (WorkQueue *)arg;
    while (true) {
        pthread_mutex_lock(&q->mu);
        int idx = q->next++;
        pthread_mutex_unlock(&q->mu);
        if (idx >= q->paths->count) {
            break;
        }
        int rc = engine_process_file(q->paths->items[idx], q->opts, q->cfg, q->paths->count > 1, true);
        pthread_mutex_lock(&q->mu);
        record_rc(q, rc);
        pthread_mutex_unlock(&q->mu);
    }
    return NULL;
}

static int process_parallel(PathList *paths, const CliOptions *opts, const Config *cfg) {
    if (paths->count == 0) {
        return EOTE_EXIT_OK;
    }
    WorkQueue q;
    q.opts = opts;
    q.cfg = cfg;
    q.paths = paths;
    q.next = 0;
    q.worst = EOTE_EXIT_OK;
    pthread_mutex_init(&q.mu, NULL);
    int thread_count = paths->count < 4 ? paths->count : 4;
    pthread_t threads[4];
    for (int i = 0; i < thread_count; i++) {
        pthread_create(&threads[i], NULL, worker_main, &q);
    }
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_destroy(&q.mu);
    return q.worst;
}

int walker_process_targets(const CliOptions *opts, const Config *cfg, const IgnoreRules *rules) {
    PathList bulk;
    paths_init(&bulk);
    int worst = EOTE_EXIT_OK;
    if (opts->check && !opts->quiet) {
        printf("[STATUS] Checking code uniformity constraints...\n");
        fflush(stdout);
    }
    for (int i = 0; i < opts->target_count; i++) {
        const char *target = opts->targets[i];
        if (has_glob_chars(target)) {
            char root[4096];
            walk_glob(&bulk, rules, glob_root(target, root, sizeof(root)), target);
        } else if (path_is_dir(target)) {
            walk_dir(&bulk, rules, target);
        } else {
            int rc = engine_process_file(target, opts, cfg, opts->target_count > 1, true);
            if (rc == EOTE_EXIT_ERROR) {
                worst = EOTE_EXIT_ERROR;
            } else if (rc == EOTE_EXIT_CHECK_FAILED && worst == EOTE_EXIT_OK) {
                worst = EOTE_EXIT_CHECK_FAILED;
            }
        }
    }
    int bulk_rc = process_parallel(&bulk, opts, cfg);
    if (bulk_rc == EOTE_EXIT_ERROR) {
        worst = EOTE_EXIT_ERROR;
    } else if (bulk_rc == EOTE_EXIT_CHECK_FAILED && worst == EOTE_EXIT_OK) {
        worst = EOTE_EXIT_CHECK_FAILED;
    }
    paths_free(&bulk);
    return worst;
}
