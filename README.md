# The Aven C source code formatter

This repository contains a C source code lexer, parser, and AST
renderer in `aven/c.h`. All three are combined into a source code formating
application in `src/aven-cfmt.c`.

I am not a fan of any configuration of `clang-format`,
`indent`, or `astyle` (believe me, I tried). The `aven-cfmt` C code formatter
is specifically tailored to my code aesthetic and supports minimal configuration;
it's trying to be `zig fmt` or `go fmt`, not `clang-format`.

## Limitations

The formatter is designed to parse code that follows the C99 or C11 standard
with some GNU extensions. It will parse GNU inline assembly and attribute
specifiers, but attributes must occur either before all other
declaration specifiers, or after a declarator. It will also parse C++ `extern "C"`
blocks to allow for "universal" header files.

### Preprocessor directives

Standard preprocessor directives (`#if`, `#else`, etc) and macros
(`#define A` or `#define A(...)`) will be parsed
and pretty printed, but some heavy restrictions are placed on their use.
A directive/macro may only be followed by a single token, a type name, an
initializer list, a list of declaration specifiers, or an expression statement,
a `do` statement, or a declaration without the terminating `;`.
The `#` and `##` operators are allowed in preprocessor code.

Source files must be parseable C even with all
preprocessor directive lines removed, e.g. the following is invalid
according to `aven-cfmt`.
```C
#ifdef A
int foo(int n) {
#else
int bar(int n){
#endif
    // body
}
```
In practice, most C files will already follow these rules.
If the column width is set to unlimited (`--columns 0`), then `aven-cfmt`
will format most Raylib and GLFW source files. The files it refuses to format
use either unsupported macros or non-standard extensions like the MSVC
`(__stdcall *fn)` declarators. It would be simple to modify these files
to comply, but, of course, formatting code from established projects is not a
goal of `aven-cfmt`.

Include directives (`#include`) will also be parsed and pretty printed. All other
directives will not be parsed, including `#error`, `#warning`, and `#pragma`; all such
directives are rendered unmodified from the original source.

### Character sets and white space

Source files are assumed to be ascii or utf8 encoded. The source file is
verified to be valid utf8 prior to tokenization.
The tokenizer only allows non-ascii codepoints in comments, character
constants, and string literals. With regards to column width, each utf8 codepoint
is counted as one column.

The only whitespace characters that will be rendered are spaces, newlines, and tabs.
Windows `\r\n` line endings will be parsed the same as `\n`, and the formatter
will render all line endings as `\n`.
Carriage return characters `\r` are illegal outside of line endings. All indents
are rendered with spaces.
Tab characters `\t` are legal everywhere whitespace is allowed, but will only be
rendered if they appear within comments or string literals.

## Errors

The formatter will only render code that it can correctly parse.
It will report the first parse error encountered, along with the exact location of
the error in the source file.

By default lines are required to render
within 80 columns, with a slight overflow allowance
to simplify the rendering logic. If a line cannot be
broken to fit within 80 columns, e.g. due to a long identifier or excessive indent depth,
then the formatter will error and report the offending
line in the original source file.

The formatter does not perform any semantic analysis, and it's error messages will
pale in comparison to any real C compiler. The reports are primarily for debugging cases
where a valid C source file doesn't fit within the restrictive
flavor of C that `aven-cfmt` can parse.

## Building

Ensure that you have pulled the `libaven` submodule dependency.
```Shell
$ git submodule init
$ git submodule update
```
To build the project with your favorite C compiler `cc`, run
```Shell
$ cc -I deps/libaven/include -I include/ -o aven-cfmt src/aven-cfmt.c
```
I also have a C build system written in C that I provide for all of my projects.
To build the build system run either
```Shell
$ make
```
or
```Shell
$ cc -o build build.c
```
To build `aven-cfmt`, run
```Shell
$ ./build
```
The resulting binary will be located in the `build_out` directory.
Flags may be specified with `--ccflags`
```Shell
$ ./build --ccflags "-O3 -march=native -g0 -fstrict-aliasing"
```
To run the test suite, run
```Shell
$ ./build test
```
To clean up all build artifacts, run
```Shell
$ ./build clean
```
To see a full list of available build system flags, run
```Shell
$ ./build help
```

## Usage

The default behavior is to read from the specified `src_file` and write to `stdout`.
```Shell
$ aven-cfmt unformatted.c > formatted.c
```
An output file can be specified with `--out`.
```Shell
$ aven-cfmt unformatted.c --out formatted.c
```
Files can be formatted in-place (with no modification on error) using `--in-place`.
```Shell
$ aven-cfmt --in-place myfile.c
```
The formatter will read from stdin if the `--stdin` flag is specified.
```Shell
$ aven-cfmt --stdin < unformatted.c > formatted.c
```
A full list of command line options is available in the help message.
```Shell
$ aven-cfmt --help
overview: Aven C Formatter
usage: aven-cfmt [src_file] [options]
configure:
    comments at the top of files can configure options
        // aven cfmt columns: 128
        // aven cfmt indent: 8
        // aven cfmt depth: 0
    or disable formatting
        // aven cfmt disable
options:
    --out "str"           output file (optional)
    --stdin [false]       read from stdin (default=false)
    --in-place [false]    format src_file in-place (default=false)
    --columns N           column width, 0 for no limit (default=80)
    --indent N            indent width (default=4)
    --depth N             parse depth, 0 for no limit (default=32)
    --help                show this  message
```

### In-editor formatting

As `aven-cfmt --stdin` works in the same way as `clang-format`, `astyle`, and
`zig fmt --stdin`, it should be simple to configure it for use with any editor
that supports those formatters.

If you use [helix][1] like I do, you can format on save with `aven-cfmt` by adding the following
lines to your `languages.toml` file.
```TOML
# ...
[[language]]
name = "c"
formatter = { command = "aven-cfmt", args = [ "--stdin" ] }
autoformat = true
# ...
```
If you use a (neo)vim variant, then you can format the active buffer with
`%!aven-fmt --stdin`. With this basic command the buffer will be overwritten with the
`stderr` error message if a parse or render error occurs, but that is simple to undo.

## Fuzzing

If you have `clang` installed, the repo contains a basic `libfuzzer` fuzzing setup that may be compiled and
run with
```Shell
$ ./clang_fuzz.sh
```
The fuzzer runs indefinitely, halting upon encountering a crash, failed assert, sanitizer trap, or
an input that takes longer than 1 second to parse and render.

## Parse depth

The `aven-cfmt` C parser is not particularly well designed, and a combinatorial explosion of backtracking can
occur with some pathological inputs. The hack to solve this (and satisfy the fuzzer) was to put a limit on the
depth of the parse tree. If valid source code contains extremely long expressions or
if-else chains, then this depth limit may become a problem. The `--depth N` command line
argument or the `// aven cfmt depth: N` control comment may be used to change the parse depth limit
in such cases.

## Performance

My benchmarks show that `aven-cfmt` formats at ~30-40MB/sec on my Intel N100 mini pc.
Through fuzzing, I am reasonably confident that it won't hang or crash, even
in the face of pathologies.
```Shell
$ lscpu | grep "Model name"
Model name:                           Intel(R) N100
$ ./build --ccflags "-O3 --target=x86_64-linux"
clang -O3 --target=x86_64-linux -I deps/libaven/include -I ./include -c -o build_out/aven-cfmt.o ./src/aven-cfmt.c
clang -o build_out/aven-cfmt build_out/aven-cfmt.o
rm build_out/aven-cfmt.o
$ poop "clang-format ../raylib/src/rcore.c" "astyle --style=google --stdin=../raylib/src/rcore.c" "./build_out/aven-cfmt ../raylib/src/rcore.c --columns 128"
Benchmark 1 (10 runs): clang-format ../raylib/src/rcore.c
  measurement          mean ± σ            min … max           outliers         delta
  wall_time           520ms ± 2.06ms     518ms …  525ms          1 (10%)        0%
  peak_rss           94.3MB ±  108KB    94.1MB … 94.4MB          0 ( 0%)        0%
  cpu_cycles         1.69G  ± 2.96M     1.69G  … 1.70G           0 ( 0%)        0%
  instructions       3.88G  ±  623K     3.88G  … 3.88G           0 ( 0%)        0%
  cache_references   39.5M  ±  154K     39.3M  … 39.8M           0 ( 0%)        0%
  cache_misses       12.7M  ± 61.5K     12.7M  … 12.8M           0 ( 0%)        0%
  branch_misses      7.74M  ± 17.4K     7.71M  … 7.76M           0 ( 0%)        0%
Benchmark 2 (153 runs): astyle --style=google --stdin=../raylib/src/rcore.c
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          32.8ms ±  480us    32.3ms … 37.2ms          2 ( 1%)        ⚡- 93.7% ±  0.1%
  peak_rss           3.55MB ±  104KB    3.31MB … 3.79MB          0 ( 0%)        ⚡- 96.2% ±  0.1%
  cpu_cycles          106M  ±  567K      105M  …  108M           3 ( 2%)        ⚡- 93.7% ±  0.0%
  instructions        282M  ± 52.1K      282M  …  283M           1 ( 1%)        ⚡- 92.7% ±  0.0%
  cache_references   30.5K  ± 3.10K     22.7K  … 38.5K           0 ( 0%)        ⚡- 99.9% ±  0.1%
  cache_misses       7.69K  ± 1.36K     5.95K  … 19.9K          10 ( 7%)        ⚡- 99.9% ±  0.1%
  branch_misses       515K  ± 14.6K      500K  …  610K          11 ( 7%)        ⚡- 93.3% ±  0.1%
Benchmark 3 (1087 runs): ./build_out/aven-cfmt ../raylib/src/rcore.c --columns 128
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          4.57ms ±  544us    4.13ms … 7.03ms        132 (12%)        ⚡- 99.1% ±  0.1%
  peak_rss           1.86MB ± 58.5KB    1.65MB … 1.97MB          1 ( 0%)        ⚡- 98.0% ±  0.0%
  cpu_cycles         11.9M  ±  158K     11.7M  … 13.4M          69 ( 6%)        ⚡- 99.3% ±  0.0%
  instructions       28.9M  ±  208      28.9M  … 28.9M           0 ( 0%)        ⚡- 99.3% ±  0.0%
  cache_references   9.97K  ± 4.54K     5.37K  … 60.4K          20 ( 2%)        ⚡-100.0% ±  0.0%
  cache_misses       1.39K  ±  640       946   … 17.0K          24 ( 2%)        ⚡-100.0% ±  0.0%
  branch_misses       125K  ± 2.92K      122K  …  138K         163 (15%)        ⚡- 98.4% ±  0.0%
```
I compiled a release build of `astyle` from upstream source, but the `clang-format` binary was from
my package manager.
This [poop][2] benchmark was only provided to show that, due to its simplicity, `aven-cfmt`
seems to run very fast, even though I did very little deliberate optimization.

[1]: https://helix-editor.com/
[2]: https://github.com/andrewrk/poop
