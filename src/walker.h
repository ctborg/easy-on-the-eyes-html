#ifndef EOTE_WALKER_H
#define EOTE_WALKER_H

#include "cli.h"
#include "config.h"
#include "ignore.h"

int walker_process_targets(const CliOptions *opts, const Config *cfg, const IgnoreRules *rules);

#endif
