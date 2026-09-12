re_add_module(ReGUI)

target_link_libraries(ReGUI
        PUBLIC
        ReCore

        ReRenderCore
        imgui
        filagui
)

target_compile_definitions(ReGUI PRIVATE RE_GUI_EXPORTS)