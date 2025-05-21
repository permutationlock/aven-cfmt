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
        .value = { .type = AVEN_ARG_TYPE_BOOL, .data = { .arg_bool = false } },
    },
    {
        .name = aven_str_init("--in-place"),
        .description = aven_str_init("format src_file in-place"),
        .type = AVEN_ARG_TYPE_BOOL,
        .value = { .type = AVEN_ARG_TYPE_BOOL, .data = { .arg_bool = false } },
    },
    {
        .name = aven_str_init("--columns"),
        .description = aven_str_init("column width, 0 for no limit"),
        .value = { .type = AVEN_ARG_TYPE_UINT, .data = { .arg_uint = 80 } },
        .type = AVEN_ARG_TYPE_UINT,
    },
    {
        .name = aven_str_init("--indent"),
        .description = aven_str_init("indent width"),
        .value = { .type = AVEN_ARG_TYPE_UINT, .data = { .arg_uint = 4 } },
        .type = AVEN_ARG_TYPE_UINT,
    },
    {
        .name = aven_str_init("--depth"),
        .description = aven_str_init("parse depth, 0 for no limit"),
        .value = { .type = AVEN_ARG_TYPE_UINT, .data = { .arg_uint = 12 } },
        .type = AVEN_ARG_TYPE_UINT,
    },
};

// a 4GB virtual memory reserve will handle pathological files up to ~40MB,
// and normal source files up to ~400MB
#define MAX_ARENA_SIZE ((size_t)min(UINT32_MAX, SIZE_MAX))

int main(int argc, char **argv) {
    size_t arena_size = MAX_ARENA_SIZE;
    void *mem = NULL;
    while (render_size >= (1 << 14)) {
        mem = malloc(arena_size);
        if (mem != NULL) {
            break;
        }
        arena_size /= 2;
    }
    if (mem == NULL) {
        aven_panic("malloc failed\n");
    }
    AvenArena arena = aven_arena_init(mem, arena_size);
    size_t render_size = arena_size / 10;

    AvenStr overview = aven_str("Aven C Formatter");
    AvenStr usage = aven_str(
        "aven-cfmt [src_file] [options]\n"
        "configure:\n"
        "    comments at the top of files can configure options\n"
        "        // aven cfmt columns: 128\n"
        "        // aven cfmt indent: 8\n"
        "        // aven cfmt depth: 0\n"
        "    or disable formatting\n"
        "        // aven cfmt disable"
    );
    AvenArgSlice args = slice_array(arg_data);
    size_t arg_cols = aven_arg_col_len(args);
    AvenArgError parse_error = aven_arg_parse(args, argv, argc, overview, usage);
    switch (parse_error) {
        case AVEN_ARG_ERROR_NONE: {
            break;
        }
        case AVEN_ARG_ERROR_HELP: {
            return 0;
        }
        default: {
            return 1;
        }
    }

    uint64_t arg_cwidth = aven_arg_get_uint(args, "--columns");
    uint64_t arg_indent = aven_arg_get_uint(args, "--indent");
    uint64_t arg_depth = aven_arg_get_uint(args, "--depth");
    size_t column_width = (size_t)arg_cwidth;
    size_t indent = (size_t)arg_indent;
    size_t parse_depth = (size_t)arg_depth;

    AvenIoReader reader = aven_io_stdin;
    Optional(AvenIoFd) in_fd = { 0 };
    Optional(AvenStr) in_file = { 0 };
    if (aven_arg_has_arg(args, "")) {
        if (aven_arg_get_bool(args, "--stdin")) {
            aven_io_perr("error: cannot specify both --stdin and src_file\n");
            aven_arg_help(args, overview, usage, arg_cols);
            return 1;
        }
        in_file.valid = true;
        in_file.value = aven_arg_get_str(args, "");
    } else if (!aven_arg_get_bool(args, "--stdin")) {
        aven_io_perr("error: specify a src_file to format or --stdin\n");
        aven_arg_help(args, overview, usage, arg_cols);
        return 1;
    }

    Optional(AvenStr) out_file = { 0 };
    bool in_place = aven_arg_get_bool(args, "--in-place");
    if (aven_arg_has_arg(args, "--out")) {
        if (in_place) {
            aven_io_perr("error: cannot specify both --out and --in-place\n");
            aven_arg_help(args, overview, usage, arg_cols);
            return 1;
        }
        out_file.valid = true;
        out_file.value = aven_arg_get_str(args, "--out");
    }
    if (in_place) {
        if (!in_file.valid) {
            aven_io_perr("error: must specify a src_file to use --in-place\n");
            aven_arg_help(args, overview, usage, arg_cols);
            return 1;
        }
        assert(out_file.valid == false);
        out_file.valid = true;
        out_file.value = in_file.value;
    }

    if (in_file.valid) {
        AvenIoOpenResult in_res = aven_io_open(
            unwrap(in_file),
            AVEN_IO_OPEN_MODE_READ,
            arena
        );
        if (in_res.error != AVEN_IO_OPEN_ERROR_NONE) {
            aven_io_perrf(
                "error: opening '{}' failed with code {}\n",
                aven_fmt_str(unwrap(in_file)),
                aven_fmt_int((int)in_res.error)
            );
            aven_arg_help(args, overview, usage, arg_cols);
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
        aven_io_perrf(
            "error: reader failed with code {}\n",
            aven_fmt_int(rd_res.error)
        );
        return 1;
    }
    AvenStr src = {
        .ptr = (char *)rd_res.payload.ptr,
        .len = rd_res.payload.len,
    };

    ByteSlice bytes = aven_arena_create_slice(
        unsigned char,
        &arena,
        render_size
    );
    AvenIoWriter writer = aven_io_writer_init_bytes(bytes);
    AvenCFmtResult fmt_res = aven_c_fmt(
        src,
        &writer,
        column_width,
        indent,
        parse_depth,
        &arena
    );
    if (fmt_res.error != AVEN_C_FMT_ERROR_NONE) {
        aven_io_perrf("error: {}\n", aven_fmt_str(fmt_res.msg));
        return 1;
    }
    ByteSlice written = slice_head(writer.buffer, writer.index);

    Optional(AvenIoFd) out_fd = { 0 };
    if (out_file.valid) {
        AvenIoOpenResult out_res = aven_io_open(
            unwrap(out_file),
            AVEN_IO_OPEN_MODE_WRITE,
            arena
        );
        if (out_res.error != AVEN_IO_OPEN_ERROR_NONE) {
            aven_io_perrf(
                "error: opening '{}' failed with code {}\n",
                aven_fmt_str(unwrap(out_file)),
                aven_fmt_int((int)out_res.error)
            );
            aven_arg_help(args, overview, usage, arg_cols);
            return 1;
        }
        out_fd.valid = true;
        out_fd.value = out_res.payload;
    }
    AvenIoWriter file_writer;
    if (out_fd.valid) {
        file_writer = aven_io_writer_init_fd(unwrap(out_fd));
    } else {
        file_writer = aven_io_stdout;
    }
    AvenIoResult res = aven_io_writer_push(&file_writer, written);
    if (res.error != 0) {
        aven_io_perrf(
            "error: writing '{}' failed with code {}\n",
            aven_fmt_str(out_file.valid ? unwrap(out_file) : aven_str("stdout")),
            aven_fmt_int(res.error)
        );
        return 1;
    }
    if (res.payload != written.len) {
        aven_io_perrf(
            "error: writing '{}' ran out of space\n",
            aven_fmt_str(out_file.valid ? unwrap(out_file) : aven_str("stdout"))
        );
        return 1;
    }

    return 0;
}
