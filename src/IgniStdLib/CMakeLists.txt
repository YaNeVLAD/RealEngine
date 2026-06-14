add_library(IgniStdLib SHARED
        src/IgniStdLib.cpp
)

target_include_directories(IgniStdLib
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        src
)

target_link_libraries(IgniStdLib
        PUBLIC
        ReCore
        ReRVM
        IgniLang
)

target_compile_definitions(IgniStdLib PRIVATE IGNI_STD_EXPORT)