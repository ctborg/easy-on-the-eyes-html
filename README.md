# easy-on-the-eyes

`easy-on-the-eyes` is a small C command-line formatter for HTML, CSS, JavaScript, and JSON.
It is designed for un-minifying and making source files readable without pulling in third-party
runtime dependencies.

This project is early. The formatters are intentionally conservative and dependency-free, so they
do not aim to replace full language-aware tools such as Prettier, ESLint, or dedicated HTML parsers.

## Build

Requirements:

- C11 compiler
- CMake 3.16+
- POSIX-like system, tested on macOS

```sh
cmake -S . -B build
cmake --build build
```

The binary is written to:

```sh
build/easy-on-the-eyes
```

## Test

```sh
cmake --build build --target run_tests
```

## Usage

```sh
easy-on-the-eyes [options] [files|dirs|globs...]
```

Examples:

```sh
easy-on-the-eyes src/index.html
easy-on-the-eyes -w "src/**/*.js" --quiet
cat package.json | easy-on-the-eyes --lang json
easy-on-the-eyes --validate
```

Options:

```text
-w, --write                    overwrite files in place
-c, --check                    verify formatting without writing
-q, --quiet                    suppress non-error output
-i, --indent-size <int>        set indentation size
    --use-tabs                 indent with tabs
    --max-char <int>           wrap long lines when non-zero
    --space-conditional <bool> control spaces before conditionals
    --lang <html|css|js|json>  language for stdin mode
    --validate                 validate configuration files
-v, --version                  show version
-h, --help                     show help
```

## Configuration

Configuration is read from:

1. `.easy-on-the-eyes.rc` in the current working directory
2. `~/.easy-on-the-eyes.rc`
3. built-in defaults

CLI flags have highest priority.

Example:

```json
{
  "indent-size": 2,
  "use-tabs": false,
  "max-char": 80,
  "space-conditional": true
}
```

## Ignore File

During directory and glob walks, `.easy-on-the-eyes.ignore` may contain glob patterns to skip.

The formatter always ignores:

```text
**/.git/**
**/node_modules/**
```

Explicit files passed on the command line bypass ignore rules.

## Exit Codes

```text
0  clean run / all checks passed
1  --check found unformatted files
2  parse failure / config validation failure / fatal error
```

## License

MIT
