# The Aven C source code formatter

This repository contains a C source code lexer, parser, and AST
renderer in `aven/c.h`. All three are combined into a source code formating
application in `src/aven-cfmt.c`.

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
a `do` statement, or a declaration, ommitting the terminating `;`.
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

Source files are assumed to be ascii or utf8 encoded. Each source file is
verified to be valid utf8 prior to tokenization.
The tokenizer only allows non-ascii codepoints in comments, character
constants, and string literals. Each utf8 codepoint is counted as one column
during rendering.

The only whitespace characters that will be rendered are spaces, newlines, and tabs.
Windows `\r\n` line endings will be parsed the same as `\n`, and the formatter
will render all line endings as `\n`.
Carriage return characters `\r` are illegal outside of line endings. All indents
are rendered with space, tabs will only be
rendered if they appear within comments or string literals.

### Parse depth

A combinatorial explosion of backtracking can
occur with some pathological inputs. Fixing the underlying issue proved
too tricky for the time being, so to solve this (and satisfy the allmighty fuzzer)
I simply placed a limit on the depth of the parse tree.
If valid source code contains extremely long expressions or
if-else chains, then the `--depth N` command line
argument or the `// aven cfmt depth: N` control comment may be used to expand the limit.

### Is this too restrictive?

It works for my code, and a surprisingly large portion of the repos I keep cloned on my machine are
formattable out of the box as well. For example, `aven-cfmt --columns 0`
will accept most [musl][4], [Raylib][5], and [GLFW][6] source files. The files it refuses to format
contain either C++ code, unsupported macros (not using `do` statements, including
terminating `;`), or extensions (MSVC
`(__stdcall *fn)` declarators). It would be simple to modify such files
to comply, but, of course, re-formatting code from well established projects is
not a goal of `aven-cfmt`.

```Shell
$ # aven-cfmt all Raylib .c files, only show errors
$ for i in raylib/src/*.c; do echo $i && aven-cfmt --columns 128 $i > /dev/null; done
raylib/src/raudio.c
raylib/src/rcore.c
raylib/src/rglfw.c
raylib/src/rmodels.c
error: expected punctuator '('
5247:9:         int n = 0; \
                ^
raylib/src/rshapes.c
raylib/src/rtext.c
raylib/src/rtextures.c
raylib/src/utils.c
```
```Shell
$ # count number of files in musl-1.2.5/src/stdio
$ ls -1 musl-1.2.5/src/stdio/ | wc -l
     118
$ # aven-cfmt all stdio files, only show errors
$ for i in musl-1.2.5/src/stdio/*; do \
    echo $i && aven-cfmt $i; \
done 2>&1 | grep "error:" -B1 -A2
musl-1.2.5/src/stdio/vfprintf.c
error: expected identifier
47:14: #define S(x) [(x)-'A']
                    ^
--
musl-1.2.5/src/stdio/vfwprintf.c
error: expected identifier
40:14: #define S(x) [(x)-'A']
                    ^
```

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
To build the build system you can either use `make` or
```Shell
$ cc -o build build.c
```
To build `aven-cfmt`, run `./build`.
The resulting binary will be located in the `build_out` directory.
Flags may be specified with `--ccflags` and `--ldflags`
```Shell
$ ./build --ccflags "-O3 -march=native -g0" --ldflags "-O3 -g0"
```
To run the test suite, run `./build test`.
To clean up all build artifacts, run `./build clean`.
To see a full list of available build system flags, run
`./build help`.

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
    --depth N             parse depth, 0 for no limit (default=40)
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

My benchmarks show that `aven-cfmt` formats at ~40MB/sec on my Intel N100 mini pc.
```Shell
$ lscpu | grep "Model name"
Model name:                           Intel(R) N100
$ ./build --ccflags "-O3" --ldflags ""
clang -O3 -I deps/libaven/include -I ./include -c -o build_out/aven-cfmt.o ./src/aven-cfmt.c
clang -o build_out/aven-cfmt build_out/aven-cfmt.o
rm build_out/aven-cfmt.o
$ poop "clang-format ../raylib/src/rcore.c" "astyle --style=google --stdin=../raylib/src/rcore.c" "./build_out/aven-cfmt --columns 128 ../raylib/src/rcore.c"
Benchmark 1 (7 runs): clang-format ../raylib/src/rcore.c
  measurement          mean ± σ            min … max           outliers         delta
  wall_time           759ms ± 7.84ms     747ms …  772ms          0 ( 0%)        0%
  peak_rss           96.4MB ±  158KB    96.2MB … 96.7MB          0 ( 0%)        0%
  cpu_cycles         2.45G  ± 7.66M     2.43G  … 2.46G           0 ( 0%)        0%
  instructions       5.41G  ± 1.12M     5.41G  … 5.41G           0 ( 0%)        0%
  cache_references   65.5M  ±  733K     64.5M  … 66.5M           0 ( 0%)        0%
  cache_misses       24.0M  ±  787K     23.0M  … 24.9M           0 ( 0%)        0%
  branch_misses      10.7M  ± 34.1K     10.6M  … 10.7M           0 ( 0%)        0%
Benchmark 2 (149 runs): astyle --style=google --stdin=../raylib/src/rcore.c
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          33.6ms ±  704us    32.4ms … 38.1ms          1 ( 1%)        ⚡- 95.6% ±  0.2%
  peak_rss           3.54MB ±  100KB    3.30MB … 3.75MB          0 ( 0%)        ⚡- 96.3% ±  0.1%
  cpu_cycles          107M  ±  778K      105M  …  112M           2 ( 1%)        ⚡- 95.6% ±  0.1%
  instructions        282M  ± 54.8K      282M  …  282M           1 ( 1%)        ⚡- 94.8% ±  0.0%
  cache_references   49.0K  ± 7.86K     35.2K  … 82.4K           3 ( 2%)        ⚡- 99.9% ±  0.2%
  cache_misses       15.1K  ± 2.65K     11.8K  … 30.3K           6 ( 4%)        ⚡- 99.9% ±  0.5%
  branch_misses       516K  ± 16.6K      502K  …  688K           3 ( 2%)        ⚡- 95.2% ±  0.1%
Benchmark 3 (1157 runs): ./build_out/aven-cfmt --columns 128 ../raylib/src/rcore.c
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          4.29ms ±  416us    3.92ms … 6.62ms         95 ( 8%)        ⚡- 99.4% ±  0.1%
  peak_rss           1.85MB ± 42.8KB    1.73MB … 1.91MB          0 ( 0%)        ⚡- 98.1% ±  0.0%
  cpu_cycles         11.2M  ±  157K     11.0M  … 12.9M          50 ( 4%)        ⚡- 99.5% ±  0.0%
  instructions       27.5M  ±  203      27.5M  … 27.5M         381 (33%)        ⚡- 99.5% ±  0.0%
  cache_references   15.3K  ± 6.84K     5.58K  … 71.3K          29 ( 3%)        ⚡-100.0% ±  0.1%
  cache_misses       1.81K  ± 1.17K      980   … 30.1K          40 ( 3%)        ⚡-100.0% ±  0.2%
  branch_misses       118K  ± 1.29K      115K  …  124K          75 ( 6%)        ⚡- 98.9% ±  0.0%
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
an input that takes longer than 2 seconds to parse and render.

[1]: https://helix-editor.com/
[2]: https://github.com/andrewrk/poop
[3]: https://llvm.org/docs/LibFuzzer.html
[4]: https://musl.libc.org/
[5]: https://www.raylib.com/
[6]: https://www.glfw.org/
