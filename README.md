# The Aven C source code formatter

This repository contains a C source code lexer, parser, and AST
renderer in `aven/c.h`. All three are combined into a source code formating
application in `src/aven-cfmt.c`. Personally, I am not a fan of any of the
possible configurations of `clang-format` (believe me, I tried).

I did not optimize the code very much or
make any use of SIMD, but benchmarks show that it formats at ~30-40MB/sec,
easily fast enough for my use cases.
Benchmarking several source files using [`poop`][9] showed that
on my machine `aven-cfmt` ran between 20 and 100 times faster than `clang-format`,
around speed of `zig fmt` for similar size source files.

The formatter has rudimentary error reporting. It will report the first parse error
it encounters along with the exact location of the error in the source file. By
default the formatter requires that lines render within 80 columns, with an allowance for
one or two characters over to make the rendering logic easier. If a line cannot be
broken to fit within 80 columns, e.g. due to a long identifier or excessive indent depth,
then the formatter will error and report the offending
line in the original source file.

## Limitations

The formatter is designed to parse code that follows the C99 and C11 standards
with some GNU extensions. E.g. it will parse GNU attribute specifiers and inline
assembly statements. Declaration attributes must occur before all other
declaration specifiers, or after a declarator.

Preprocessor directives and macros will be parsed and pretty printed, but some
heavy restrictions are placed on their use.
A define directive must be an expression statement, a `do` statement,
or a declaration without the terminating ';', or a type-name,
an initializer list, a list of declaration specifiers, or any single token.

The `#` and `##` operators are allowed while in preprocessor mode.
All non-standard directives (standard directives are `#define`, `#if`, `#else`, etc.)
are not parsed and are rendered unmodified as if they were comments.

Source files must be parseable C even with all
preprocessor directive lines removed. E.g. the following is invalid
according to `aven-cfmt`:

```C
#ifdef A
int foo(int n) {
#else
int bar(int n){
#endif
    // body
}
```

In practice, most C files will already follow these rules. E.g.
if the 80 column width requirement is removed, then `aven-cfmt`
can format most Raylib and GLFW source files. The files it can't format
use unsupported macros or non-standard extensions like `(__stdcall *fn)` in declarators,
which would be simple to modify in order to accomodate `aven-fmt` if desired.

I prefer these restrictions to enforce sane
macro practices. In the few places in my own source code where
the formatter reported an issue, the
changes required to comply were clear improvements.

## Building

Ensure that you have pulled the `libaven` submodule dependency.
```Shell
git submodule init
git submodule update
```
To build with your favorite C compiler `cc` you may simply run
```Shell
cc -I deps/libaven/include -I include/ -o aven-cfmt src/aven-cfmt.c
```
I also have my own C build system written in C that I provide for all of my projects.
To build the build system run either
```Shell
make
```
or
```
cc -o build build.c
```
Then to build the `aven-cfmt` binary, run
```Shell
./build
```
The resulting binary will be located in the `build_out` directory.
Compilation flags may be specified with `--ccflags`
```Shell
./build --ccflags "-O3 -march=native -g0 -fstrict-aliasing"
```
To run the test suite, run
```Shell
./build test
```
To clean up all build artifacts, run
```Shell
./build clean
```
To see a full list of available flags, run
```Shell
./build help
```

## Usage

The default behavior is to read from the specified src_file and write to `stdout`.

```Shell
aven-cfmt unformatted.c > formatted.c
```

An output file can be specified with `--out`.
```Shell
aven-cfmt unformatted.c --out formatted.c
```

Files can be formatted in-place (with no changes on parse or render error) using `--in-place`.
```Shell
aven-cfmt --in-place myfile.c
```

The formatter will read from stdin if the `--stdin` flag is specified.
```Shell
cat unformatted.c | aven-cfmt --stdin > formatted.c
```

## Fuzzing

If you have `clang` installed, then I have a basic `libfuzzer` fuzzing setup that may be compiled and
run with
```Shell
./clang_fuzz.sh
```

[9]: https://github.com/andrewrk/poop
