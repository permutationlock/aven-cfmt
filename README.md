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
specifiers. It will also parse C++ `extern "C"` blocks to allow for "universal" header files.

The formatter does not perform semantic analysis or try to follow include directives. Therefore
it is impossible to handle all possible uses of the
C preprocessor. Instead, common uses of macros are parsed directly into the AST.

### Macro definitions

Macro `#define A` and `#define A(...)` directives are each parsed into a
separate AST. Each macro AST is rendered when the corresponding portion of the
primary AST is rendered.
A macro definiton may be followed by a single token, a type name, an
initializer list, a list of declaration specifiers, or an expression statement,
a `do` statement, or a declaration, always ommitting terminating `;` characters.
The special `#` and `##` operators are allowed in preprocessor code.

### Macro invocations

A macro invocation is an identifier followed by an optional parenthesised parameter
list of assignment expressions and type names. Macro invocations may appear in
type names, declaration specifiers, and compound string literals. In addition, a
postfix parenthesis expression is allowed to contain type names in its parameter
list to allow type parameterised macro invocations within expressions.

### Conditional directives

Conditional directives (`#if`, `#else`, `#endif`, `#ifdef`, etc.)
are parsed and rendered in exactly the same way as macro definitions.
In the primary translation unit AST, conditional directives are simply ignored.
Therefore a source file must be still parseable when all
preprocessor directive lines are removed. E.g. the following is invalid
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

### Other directives

Include directives (`#include`) are parsed and pretty printed, but
all other directives (`#error`, `#warning`, `#pragma`, etc.) are treated as
comments and rendered unmodified from source. The `_Pragma("...")` variety
of pragma directives are not allowed.

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

### Parse depth

The `aven-cfmt` C parser was written haphazardly with the C11 standard's grammar
open on my left monitor. As a result, a combinatorial explosion of backtracking can
occur with some pathological inputs. My hack to solve this (and satisfy the allmighty fuzzer)
was to simply place a limit on the depth of the parse tree.
If valid source code contains extremely long expressions or
if-else chains, then the `--depth N` command line
argument or the `// aven cfmt depth: N` control comment may be used to expand the limit.

### Is this too restrictive?

I wrote this formatter for my own personal use, and I generally write code that
follows the above rules. In the few places where I needed to make changes to
satisfy `aven-cfmt`, I believe that those changes were for the better.

A surprisingly large portion of the repos I keep cloned on my machine are
formattable out of the box as well. For example, the `aven-cfmt --columns 0` command
will accept most Raylib and GLFW source files. The files it refuses to format
use either unsupported macros (not wrapping blocks in `do` statements, or
terminating macros with `;` or `,`), or unsupported extensions (MSVC
`(__stdcall *fn)` declarators). It would be simple to modify such files
to comply, but, of course, re-formatting code from well established projects is
not a goal of `aven-cfmt`.
```Shell
$ for i in ../raylib/src/*.c; do echo $i && ./build_out/aven-cfmt --columns 128 $i > /dev/null; done
../raylib/src/raudio.c
../raylib/src/rcore.c
../raylib/src/rcore.old.c
../raylib/src/rglfw.c
../raylib/src/rmodels.c
error: error at 5247:9: expected punctuator '(', found:
5247:1:         int n = 0; \
                ^
../raylib/src/rshapes.c
../raylib/src/rtext.c
../raylib/src/rtextures.c
../raylib/src/utils.c
```

## Errors

The formatter will report the first parse error encountered, along with the exact location of
the error in the source file. By default lines are required to render
within 80 columns, with a slight overflow allowance
to simplify the rendering logic. If a line cannot be
broken to fit within 80 columns, e.g. due to a long identifier or excessive indent depth,
then the formatter will report the offending line in the original source file.

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
The project also provides a build system written in C.
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
$ ./build --ccflags "-O3 -march=native -g0"
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

## Performance

My benchmarks show that `aven-cfmt` formats at ~30-40MB/sec on my Intel N100 mini pc.
```Shell
$ lscpu | grep "Model name"
Model name:                           Intel(R) N100
$ ./build --ccflags "-O3"
clang -O3 -I deps/libaven/include -I ./include -c -o build_out/aven-cfmt.o ./src/aven-cfmt.c
clang -o build_out/aven-cfmt build_out/aven-cfmt.o
rm build_out/aven-cfmt.o
$ poop "clang-format ../raylib/src/rcore.c" "astyle --style=google --stdin=../raylib/src/rcore.c" "./build_out/aven-cfmt --columns 128 ../raylib/src/rcore.c"
Benchmark 1 (10 runs): clang-format ../raylib/src/rcore.c
  measurement          mean ± σ            min … max           outliers         delta
  wall_time           527ms ± 4.34ms     519ms …  531ms          0 ( 0%)        0%
  peak_rss           94.3MB ±  154KB    94.1MB … 94.6MB          0 ( 0%)        0%
  cpu_cycles         1.70G  ± 6.84M     1.69G  … 1.71G           0 ( 0%)        0%
  instructions       3.88G  ± 1.02M     3.88G  … 3.88G           0 ( 0%)        0%
  cache_references   39.9M  ±  395K     39.5M  … 40.7M           0 ( 0%)        0%
  cache_misses       13.3M  ±  530K     12.6M  … 14.1M           0 ( 0%)        0%
  branch_misses      7.76M  ± 30.6K     7.71M  … 7.81M           0 ( 0%)        0%
Benchmark 2 (152 runs): astyle --style=google --stdin=../raylib/src/rcore.c
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          33.0ms ±  626us    32.1ms … 38.5ms          6 ( 4%)        ⚡- 93.7% ±  0.1%
  peak_rss           3.55MB ±  109KB    3.31MB … 3.79MB          0 ( 0%)        ⚡- 96.2% ±  0.1%
  cpu_cycles          106M  ±  726K      105M  …  110M           6 ( 4%)        ⚡- 93.8% ±  0.1%
  instructions        282M  ± 56.6K      282M  …  283M           1 ( 1%)        ⚡- 92.7% ±  0.0%
  cache_references   31.7K  ± 6.92K     18.3K  … 61.9K           1 ( 1%)        ⚡- 99.9% ±  0.2%
  cache_misses       7.76K  ± 2.18K     3.93K  … 19.5K           9 ( 6%)        ⚡- 99.9% ±  0.6%
  branch_misses       515K  ± 16.9K      500K  …  665K          11 ( 7%)        ⚡- 93.4% ±  0.1%
Benchmark 3 (1023 runs): ./build_out/aven-cfmt --columns 128 ../raylib/src/rcore.c
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          4.86ms ±  623us    4.31ms … 8.00ms        112 (11%)        ⚡- 99.1% ±  0.1%
  peak_rss           1.87MB ± 59.4KB    1.72MB … 1.97MB          0 ( 0%)        ⚡- 98.0% ±  0.0%
  cpu_cycles         12.8M  ±  225K     12.3M  … 14.5M           6 ( 1%)        ⚡- 99.2% ±  0.0%
  instructions       31.3M  ±  212      31.3M  … 31.3M           0 ( 0%)        ⚡- 99.2% ±  0.0%
  cache_references   9.62K  ± 4.19K     5.41K  … 61.9K          17 ( 2%)        ⚡-100.0% ±  0.1%
  cache_misses       1.45K  ±  629      1.04K  … 16.6K          14 ( 1%)        ⚡-100.0% ±  0.2%
  branch_misses       136K  ± 5.67K      121K  …  150K           0 ( 0%)        ⚡- 98.2% ±  0.1%
```
I compiled a release build of `astyle` from upstream source, but the `clang-format` binary was from
my package manager.
This [poop][2] benchmark was only provided to show that, due to its simplicity, `aven-cfmt`
seems to be very fast, even though I did very little deliberate optimization.

## Fuzzing

The repo contains a basic [libfuzzer][3] fuzzing setup. If you have `clang` installed, then
the fuzzer can be compiled and run with
```Shell
$ ./clang_fuzz.sh
```
The fuzzer runs indefinitely, halting upon encountering a crash, failed assert, sanitizer trap, or
an input that takes longer than 1 second to parse and render.

[1]: https://helix-editor.com/
[2]: https://github.com/andrewrk/poop
[3]: https://llvm.org/docs/LibFuzzer.html
