#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#define AVEN_IMPLEMENTATION
#include <aven.h>
#include <aven/arena.h>
#include <aven/arg.h>
#include <aven/c.h>
#include <aven/fmt.h>
#include <aven/io.h>
#include <aven/str.h>
#include <aven/ts.h>

#include <stdlib.h>

static AvenArg arg_data[] = {
    {
        .name = aven_str_init(""),
        .optional = true,
        .type = AVEN_ARG_TYPE_STRING,
    },
    {
        .name = aven_str_init("--out"),
        .description = aven_str_init("output file"),
        .optional = true,
        .type = AVEN_ARG_TYPE_STRING,
    },
    {
        .name = aven_str_init("--stdin"),
        .description = aven_str_init("read from stdin"),
        .type = AVEN_ARG_TYPE_BOOL,
    },
};

#define error_text(s) "error: " s
#define error_text_colored(s) "" \
        aven_ts(aven_ts_fg(AVEN_TS_RED), AVEN_TS_BRIGHT) \
        "error:" \
        aven_ts(AVEN_TS_PLAIN) \
        " " \
        s
#define aven_cfmt_perr(n, s) (n) ? \
        aven_io_perr(error_text(s)) : \
        aven_io_perr(error_text_colored(s))
#define aven_cfmt_perrf(n, s, ...) (n) ? \
        aven_io_perrf(error_text(s), __VA_ARGS__) : \
        aven_io_perrf(error_text_colored(s), __VA_ARGS__)

int main(int argc, char **argv) {
    // a 4GB virtual memory reserve will handle pathological files up to ~40MB,
    // and normal source files up to ~400MB
    size_t arena_size = (size_t)min((1ULL << 32), SIZE_MAX);
    void *mem = NULL;
    while (arena_size >= (1ULL << 14)) {
        mem = malloc(arena_size);
        if (mem != NULL) {
            break;
        }
        arena_size >>= 1;
    }
    if (mem == NULL) {
        aven_panic("malloc failed\n");
    }
    AvenArena arena = aven_arena_init(mem, arena_size);

    bool no_color = false;
    {
        char *no_color_env = getenv("NO_COLOR");
        if (no_color_env != NULL and aven_str_cstr(no_color_env).len > 0) {
            no_color = true;
        }
    }

    AvenStr overview = aven_str("Aven C Tokenizer");
    AvenStr usage = aven_str("aven-ctokenize [src_file] [options]");
    AvenArgSlice args = slice_array(arg_data);
    size_t arg_cols = aven_arg_col_len(args);
    AvenArgError parse_error = aven_arg_parse(args, argv, argc, overview, usage);
    switch (parse_error) {
        case AVEN_ARG_ERROR_NONE: {
            break;
        }
        case AVEN_ARG_ERROR_HELP: {
            free(mem);
            return 0;
        }
        default: {
            free(mem);
            return 1;
        }
    }

    Optional(AvenIoFd) in_fd = { 0 };
    Optional(AvenStr) in_file = { 0 };
    if (aven_arg_has_arg(args, "")) {
        if (aven_arg_get_bool(args, "--stdin")) {
            aven_cfmt_perr(
                no_color,
                "cannot specify both --stdin and src_file\n"
            );
            aven_arg_help(args, overview, usage, arg_cols);
            free(mem);
            return 1;
        }
        in_file.valid = true;
        in_file.value = aven_arg_get_str(args, "");
    } else if (!aven_arg_get_bool(args, "--stdin")) {
        aven_cfmt_perr(no_color, "specify a src_file to format or --stdin\n");
        aven_arg_help(args, overview, usage, arg_cols);
        free(mem);
        return 1;
    }

    Optional(AvenStr) out_file = { 0 };
    if (aven_arg_has_arg(args, "--out")) {
        out_file.valid = true;
        out_file.value = aven_arg_get_str(args, "--out");
    }

    AvenIoReader reader = aven_io_stdin;
    if (in_file.valid) {
        AvenIoOpenResult in_res = aven_io_open(
            unwrap(in_file),
            AVEN_IO_OPEN_MODE_READ,
            arena
        );
        if (in_res.error != AVEN_IO_OPEN_ERROR_NONE) {
            aven_cfmt_perrf(
                no_color,
                "opening '{}' failed with {}\n",
                aven_fmt_str(unwrap(in_file)),
                aven_fmt_str(aven_io_open_error_str(in_res.error))
            );
            aven_arg_help(args, overview, usage, arg_cols);
            free(mem);
            return 1;
        }
        in_fd.valid = true;
        in_fd.value = in_res.payload;
        reader = aven_io_reader_init_fd(in_fd.value);
    }
    size_t block_size = 8192;
    AvenIoBytesResult rd_res = aven_io_reader_pop_all(
        &reader,
        block_size,
        &arena
    );
    if (rd_res.error != 0) {
        aven_cfmt_perrf(
            no_color,
            "reading '{}' failed with {}\n",
            aven_fmt_str(in_file.valid ? unwrap(in_file) : aven_str("stdin")),
            aven_fmt_str(aven_io_error_str(rd_res.error))
        );
        free(mem);
        return 1;
    }
    AvenStr src = {
        .ptr = (char *)rd_res.payload.ptr,
        .len = rd_res.payload.len,
    };

    AvenStrCodepointsResult cpt_res = aven_str_codepoints(src);
    if (cpt_res.error != 0) {
        AvenCTokenLoc inv_loc = aven_c_byte_loc(src, (uint32_t)cpt_res.payload);
        aven_cfmt_perrf(
            no_color,
            "invalid utf8 in source at {}:{}",
            aven_fmt_uint(inv_loc.line),
            aven_fmt_uint(inv_loc.col)
        );
        free(mem);
        return 1;
    }
    AvenCTokenSet tset = aven_c_lex(src, &arena);

    Optional(AvenIoFd) out_fd = { 0 };
    if (out_file.valid) {
        AvenIoOpenResult out_res = aven_io_open(
            unwrap(out_file),
            AVEN_IO_OPEN_MODE_WRITE,
            arena
        );
        if (out_res.error != AVEN_IO_OPEN_ERROR_NONE) {
            aven_cfmt_perrf(
                no_color,
                "opening '{}' failed with {}\n",
                aven_fmt_str(unwrap(out_file)),
                aven_fmt_str(aven_io_open_error_str(out_res.error))
            );
            aven_arg_help(args, overview, usage, arg_cols);
            free(mem);
            return 1;
        }
        out_fd.valid = true;
        out_fd.value = out_res.payload;
    }
    AvenIoWriter file_writer;
    if (out_fd.valid) {
        file_writer = aven_io_writer_init_fd_buffered(
            unwrap(out_fd),
            8192,
            &arena
        );
    } else {
        file_writer = aven_io_writer_init_stdout_buffered(8192, &arena);
    }
    for (uint32_t i = 0; i < tset.tokens.len; i += 1) {
        AvenCToken token = get(tset.tokens, i);
        AvenStr type_str = aven_c_token_type_str(token.type);
        AvenStr str = aven_c_token_str(tset, i);
        AvenIoResult res = aven_io_writer_printf(
            &file_writer,
            "{ .type = {}, .str = \"{}\"}\n",
            aven_fmt_str(type_str),
            aven_fmt_str(str)
        );
        if (res.error != 0) {
            aven_cfmt_perrf(
                no_color,
                "writing '{}' failed with {}\n",
                aven_fmt_str(
                    out_file.valid ? unwrap(out_file) : aven_str("stdout")
                ),
                aven_fmt_str(aven_io_error_str(res.error))
            );
            free(mem);
            return 1;
        }
    }
    int error = aven_io_writer_flush(&file_writer);
    if (error != 0) {
        aven_cfmt_perrf(
            no_color,
            "writing '{}' failed with {}\n",
            aven_fmt_str(out_file.valid ? unwrap(out_file) : aven_str("stdout")),
            aven_fmt_str(aven_io_error_str(error))
        );
        free(mem);
        return 1;
    }
    free(mem);
    return 0;
}
