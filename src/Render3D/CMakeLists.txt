re_add_module(ReRender3D)

target_link_libraries(ReRender3D
        PUBLIC
        ReCore
)

target_compile_definitions(ReRender3D PRIVATE RE_RENDER_3D_EXPORTS)