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

Short local smoke run on macOS with `BENCH_WARMUPS=1 BENCH_ROUNDS=2`:

```text
## HTML
easy-on-the-eyes   mean 1.76ms | p50 2.20ms | p95 2.20ms | min 1.32ms
prettier (npx)     mean 564.58ms | p50 575.36ms | p95 575.36ms | min 553.79ms
js-beautify (npx)  mean 520.43ms | p50 526.97ms | p95 526.97ms | min 513.89ms

## CSS
easy-on-the-eyes   mean 1.43ms | p50 1.45ms | p95 1.45ms | min 1.41ms
prettier (npx)     mean 540.88ms | p50 560.62ms | p95 560.62ms | min 521.14ms
js-beautify (npx)  mean 527.89ms | p50 533.96ms | p95 533.96ms | min 521.82ms

## JS
easy-on-the-eyes   mean 1.31ms | p50 1.33ms | p95 1.33ms | min 1.28ms
prettier (npx)     mean 565.20ms | p50 595.28ms | p95 595.28ms | min 535.13ms
js-beautify (npx)  mean 523.29ms | p50 525.76ms | p95 525.76ms | min 520.82ms

## JSON
easy-on-the-eyes   mean 1.61ms | p50 1.76ms | p95 1.76ms | min 1.47ms
prettier (npx)     mean 550.64ms | p50 589.14ms | p95 589.14ms | min 512.14ms
js-beautify (npx)  mean 513.57ms | p50 520.03ms | p95 520.03ms | min 507.11ms
```

These numbers include full CLI process startup. The Prettier and js-beautify results above used
`npx`, so they also include package startup/download overhead. Install them locally before drawing
serious conclusions.

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
