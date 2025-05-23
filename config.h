// config.h can be used to defines custom defaults for flags

#if !defined(_WIN32) && defined(__GNUC__)
   #define AVEN_BUILD_COMMON_DEFAULT_CCFLAGS \
       "-std=c99 -pedantic -fstrict-aliasing -O0 -g3 -Werror -Wall -Wextra " \
       "-Wshadow -Wconversion -Wdouble-promotion -Winit-self " \
       "-Wcast-align -Wstrict-prototypes -Wold-style-definition " \
       "-fsanitize-trap -fsanitize=unreachable -fsanitize=undefined " \
       "-fsanitize=address"
    #define AVEN_BUILD_COMMON_DEFAULT_LDFLAGS \
       "-fsanitize=address"
#endif


