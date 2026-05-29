#include "cli.h"
#include "config.h"
#include "engine.h"
#include "ignore.h"
#include "util.h"
#include "walker.h"

int main(int argc, char **argv) {
    CliOptions opts;
    cli_init(&opts);
    int rc = cli_parse(&opts, argc, argv);
    if (rc == 100) {
        cli_free(&opts);
        return EOTE_EXIT_OK;
    }
    if (rc != EOTE_EXIT_OK) {
        cli_free(&opts);
        return rc;
    }
    if (opts.validate) {
        cli_free(&opts);
        return config_validate_all();
    }
    config_resolve(&opts);
    Config cfg;
    config_from_options(&opts, &cfg);
    if (opts.stdin_mode) {
        rc = engine_process_stdin(&opts, &cfg);
        cli_free(&opts);
        return rc;
    }
    IgnoreRules rules;
    ignore_init(&rules);
    ignore_load(&rules);
    rc = walker_process_targets(&opts, &cfg, &rules);
    ignore_free(&rules);
    cli_free(&opts);
    return rc;
}
