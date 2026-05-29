#include "test_support.h"

#include "cli.h"

#include <string.h>

void run_cli_tests(void) {
    CliOptions opts;
    char *argv1[] = {"eote", "-w", "-c", "a.js"};
    cli_init(&opts);
    test_expect("cli conflict parses", cli_parse(&opts, 4, argv1) == 0);
    test_expect("cli conflict clears write", !opts.write && opts.check);
    test_expect("cli target captured", opts.target_count == 1 && strcmp(opts.targets[0], "a.js") == 0);
    cli_free(&opts);

    char *argv2[] = {"eote", "--indent-size", "2", "--max-char", "80", "--space-conditional", "false"};
    cli_init(&opts);
    test_expect("cli value flags parse", cli_parse(&opts, 7, argv2) == 0);
    test_expect("cli indent set", opts.indent_size == 2 && opts.indent_size_set);
    test_expect("cli max and bool set", opts.max_char == 80 && opts.max_char_set && !opts.space_conditional);
    cli_free(&opts);

    char *argv3[] = {"eote", "--lang", "json"};
    cli_init(&opts);
    test_expect("cli lang parses", cli_parse(&opts, 3, argv3) == 0);
    test_expect("cli lang set", opts.lang_set && strcmp(opts.lang, "json") == 0);
    test_expect("cli default indent remains", opts.indent_size == 4);
    cli_free(&opts);
}
