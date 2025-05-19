#ifndef AVEN_CFMT_BUILD_H
    #define AVEN_CFMT_BUILD_H

    static inline AvenStr aven_cfmt_build_include_path(
        AvenStr root_path,
        AvenArena *arena
    ) {
        return aven_path(arena, root_path, aven_str("include"));
    }

    static inline AvenBuildStep aven_cfmt_build_step(
        AvenBuildCommonOpts *opts,
        AvenStr libaven_include_path,
        AvenBuildStepPtrOptional winutf8_obj_step,
        AvenStr root_path,
        AvenBuildStep *out_dir_step,
        AvenArena *arena
    ) {
        AvenStr include_paths[] = {
            libaven_include_path,
            aven_cfmt_build_include_path(root_path, arena),
        };
        AvenStrSlice includes = slice_array(include_paths);

        AvenStrSlice macros = { 0 };
        AvenStrSlice syslibs = { 0 };

        AvenBuildStep *obj_list_data[1];
        List(AvenBuildStep *) obj_list = list_array(obj_list_data);
        if (winutf8_obj_step.valid) {
            list_push(obj_list) = winutf8_obj_step.value;
        }
        AvenBuildStepPtrSlice objs = slice_list(obj_list);

        bool graphical = false;

        return aven_build_common_step_cc_ld_exe_ex(
            opts,
            includes,
            macros,
            syslibs,
            objs,
            aven_path(
                arena,
                root_path,
                aven_str("src"),
                aven_str("aven-cfmt.c")
            ),
            out_dir_step,
            graphical,
            arena
        );
    }
#endif
