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

## Benchmarks

The benchmark harness compares `easy-on-the-eyes` against popular Node-based formatters:

- Prettier
- js-beautify

Build the C binary first:

```sh
cmake -S . -B build
cmake --build build
```

Then run:

```sh
cmake --build build --target bench_formatters
```

For cleaner comparisons, install the JavaScript formatters locally so the benchmark does not include
`npx` package startup or download overhead:

```sh
npm i --no-save prettier js-beautify
cmake --build build --target bench_formatters
```

You can tune sample counts with environment variables:

```sh
BENCH_WARMUPS=5 BENCH_ROUNDS=50 cmake --build build --target bench_formatters
```

### Example Results

Local run on macOS with locally installed Prettier/js-beautify and
`BENCH_WARMUPS=5 BENCH_ROUNDS=50`:

```text
## HTML
easy-on-the-eyes  mean 1.39ms | p50 1.27ms | p95 1.78ms | min 1.12ms
prettier          mean 104.23ms | p50 104.02ms | p95 105.62ms | min 102.45ms
js-beautify       mean 75.85ms | p50 75.48ms | p95 77.22ms | min 73.92ms

## CSS
easy-on-the-eyes  mean 1.29ms | p50 1.26ms | p95 1.62ms | min 1.12ms
prettier          mean 91.42ms | p50 91.53ms | p95 92.97ms | min 89.92ms
js-beautify       mean 72.76ms | p50 71.96ms | p95 73.62ms | min 70.75ms

## JS
easy-on-the-eyes  mean 1.27ms | p50 1.24ms | p95 1.56ms | min 1.09ms
prettier          mean 97.02ms | p50 96.68ms | p95 99.35ms | min 95.31ms
js-beautify       mean 74.62ms | p50 74.60ms | p95 76.04ms | min 73.10ms

## JSON
easy-on-the-eyes  mean 1.27ms | p50 1.24ms | p95 1.48ms | min 1.13ms
prettier          mean 90.39ms | p50 90.32ms | p95 92.15ms | min 88.36ms
js-beautify       mean 74.38ms | p50 74.05ms | p95 75.47ms | min 72.87ms
```

These numbers include full CLI process startup. Benchmark results vary by machine, installed
formatter versions, fixture size, and whether JavaScript formatters are run through local binaries
or `npx`.

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
