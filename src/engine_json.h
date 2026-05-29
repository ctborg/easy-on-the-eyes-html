#ifndef EOTE_ENGINE_JSON_H
#define EOTE_ENGINE_JSON_H

#include "config.h"
#include <stddef.h>

char *engine_json_format(const char *src, size_t src_len, const Config *cfg, char *err, size_t err_len);

#endif
