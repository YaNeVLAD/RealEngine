re_add_module(ReAudio)

target_link_libraries(ReAudio
        PUBLIC
        ReCore

        miniaudio
)

target_compile_definitions(ReAudio PRIVATE RE_AUDIO_EXPORTS)