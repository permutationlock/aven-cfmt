# The Aven C source code formatter

This repository contains a C source code lexer, parser, and AST
renderer in `aven/c.h`. All three are combined into a source code formating
application in `src/aven-cfmt.c`.

I am not a fan of any configuration of `clang-format`,
`indent`, or `astyle` (believe me, I tried). The `aven-cfmt` C code formatter
is specifically tailored to my code aesthetic and supports minimal configuration;
it's trying to be `zig fmt` or `go fmt`, not `clang-format`.

## Errors

The formatter will only parse and render code that it considers to be correct.
It will report the first parse error encountered, along with the exact location of
the error in the source file. By default lines are required to render
within 80 columns, with a slight overflow allowance
to simplify the rendering logic. If a line cannot be
broken to fit within 80 columns, e.g. due to a long identifier or excessive indent depth,
then the formatter will error and report the offending
line in the original source file.

The formatter does not perform any semantic analysis, and it's error messages will
pale in comparison to a real C compiler. The reports are primarily for debugging cases
where a valid C source file doesn't fit within the restrictive
flavor of C that `aven-cfmt` can parse.

## Limitations

The formatter is designed to parse code that follows the C99 or C11 standard
with some GNU extensions. It will parse GNU inline assembly and attribute
specifiers, but attributes must occur either before all other
declaration specifiers, or after a declarator. It will also parse C++ `extern "C"`
blocks to allow for "universal" header files.

Standard preprocessor directives (`#if`, `#else`, etc) and macros
(`#define A` or `#define A(...)`) will be parsed
and pretty printed, but some heavy restrictions are placed on their use.
A directive/macro may only be followed by a single token, a type name, an
initializer list, a list of declaration specifiers, or an expression statement,
a `do` statement, or a declaration without the terminating ';'.
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
to comply, but, of course, formatting established projects is not the goal of
`aven-cfmt`.

Include directives (`#include`) will also be parsed and pretty printed. Common
directives that will not be parsed are `#error`, `#warning`, and `#pragma`; all such
directives are rendered unmodified from the original source.

## Characters and white space

The only whitespace characters that will be rendered are spaces and newlines. Windows
'\r\n' line endings will be parsed, but they will be rendered back as newlines.
Carriage return characters are illegal everywhere outside of line endings.
Tab characters are legal everywhere whitespace is allowed, but they will only be
rendered if they appear within comments or string literals.
The parser only considers ascii characters, but unicode characters that
appear within comments will be rendered back unmodified.

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
I have a C build system written in C that I provide for all of my projects.
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

The default behavior is to read from the specified src_file and write to `stdout`.
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
  wall_time           520ms ± 2.18ms     518ms …  526ms          1 (10%)        0%
  peak_rss           94.3MB ±  146KB    94.2MB … 94.6MB          0 ( 0%)        0%
  cpu_cycles         1.69G  ± 3.09M     1.69G  … 1.70G           0 ( 0%)        0%
  instructions       3.88G  ± 1.20M     3.88G  … 3.88G           0 ( 0%)        0%
  cache_references   39.5M  ± 89.8K     39.4M  … 39.7M           0 ( 0%)        0%
  cache_misses       12.5M  ±  133K     12.3M  … 12.7M           0 ( 0%)        0%
  branch_misses      7.75M  ± 50.9K     7.70M  … 7.88M           1 (10%)        0%
Benchmark 2 (152 runs): astyle --style=google --stdin=../raylib/src/rcore.c
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          32.9ms ±  512us    32.2ms … 35.7ms          4 ( 3%)        ⚡- 93.7% ±  0.1%
  peak_rss           3.54MB ±  107KB    3.31MB … 3.77MB          0 ( 0%)        ⚡- 96.3% ±  0.1%
  cpu_cycles          106M  ±  958K      105M  …  112M           7 ( 5%)        ⚡- 93.7% ±  0.0%
  instructions        282M  ± 54.4K      282M  …  283M           2 ( 1%)        ⚡- 92.7% ±  0.0%
  cache_references   29.5K  ± 7.85K     19.3K  … 89.4K           8 ( 5%)        ⚡- 99.9% ±  0.0%
  cache_misses       7.93K  ± 3.84K     5.11K  … 42.3K          15 (10%)        ⚡- 99.9% ±  0.2%
  branch_misses       516K  ± 23.0K      502K  …  692K          12 ( 8%)        ⚡- 93.3% ±  0.2%
Benchmark 3 (1065 runs): ./build_out/aven-cfmt ../raylib/src/rcore.c --columns 128
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          4.67ms ±  596us    4.10ms … 7.47ms        100 ( 9%)        ⚡- 99.1% ±  0.1%
  peak_rss           1.86MB ± 59.6KB    1.66MB … 1.97MB          3 ( 0%)        ⚡- 98.0% ±  0.0%
  cpu_cycles         12.0M  ±  207K     11.6M  … 13.9M          23 ( 2%)        ⚡- 99.3% ±  0.0%
  instructions       28.9M  ±  211      28.9M  … 28.9M           0 ( 0%)        ⚡- 99.3% ±  0.0%
  cache_references   9.50K  ± 4.61K     5.20K  … 60.7K          26 ( 2%)        ⚡-100.0% ±  0.0%
  cache_misses       1.35K  ±  621       938   … 16.4K          23 ( 2%)        ⚡-100.0% ±  0.1%
  branch_misses       132K  ± 5.48K      121K  …  151K          34 ( 3%)        ⚡- 98.3% ±  0.1%
```
I compiled a release build of `astyle` from upstream source, but the `clang-format` binary was from
the Chimera Linux package manager.
The [poop][2] benchmark was only provided to show that `aven-cfmt` turned out surprisingly fast with
hardly any deliberate optimization. The `clang-format` and `astyle` projects
have very different goals, are highly configurable, and format far more kinds of C code.

[1]: https://helix-editor.com/
[2]: https://github.com/andrewrk/poop
