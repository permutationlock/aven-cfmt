#define AVEN_IMPLEMENTATION
#include <aven.h>
#include <aven/arena.h>
#include <aven/io.h>
#include <aven/c.h>

#include <stdlib.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    size_t mem_size = 100ULL * (size + 16);
    void *mem = malloc(mem_size);
    if (mem == NULL) {
        aven_panic("malloc failed\n");
    }
    AvenArena temp_arena = aven_arena_init(mem, mem_size);
    AvenStr src = aven_arena_create_slice(char, &temp_arena, size + 1);
    memcpy(src.ptr, data, size);
    get(src, size) = 0;
    AvenIoWriter writer = aven_io_writer_init_sink();
    AvenCFmtResult fmt_res = aven_c_fmt(
        src,
        &writer,
        128,
        4,
        40,
        false,
        &temp_arena
    );
    (void)fmt_res;
    free(mem);
    // if (fmt_res.error == AVEN_C_FMT_ERROR_PARSE) {
    //     return -1;
    // }
    return 0;
}
