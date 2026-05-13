re_add_module(IgniLang)

target_link_libraries(IgniLang
        PUBLIC
        ReCore

        FSM
)

target_compile_definitions(IgniLang PRIVATE IGNI_EXPORT)

enable_testing()
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(IgniTests
        tests/IgniLangTests.cpp
        tests/FrontendTests.cpp
        tests/E2ETests.cpp
)

target_link_libraries(IgniTests
        PRIVATE
        IgniLang
        ReRVM
        ReCore
        gtest_main
)

include(GoogleTest)
gtest_discover_tests(IgniTests)

add_custom_command(TARGET IgniTests PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/scripts"
        "$<TARGET_FILE_DIR:IgniTests>/tests/scripts"
        COMMENT "Copying test scripts to output directory..."
)

# --- 1. ТАРГЕТ КОДОГЕНЕРАТОРА ---
# Предположим, вы создали папку tools/CodeGen в модуле языка
add_executable(SemanticsCodeGen tools/CodeGen/main.cpp)
target_link_libraries(SemanticsCodeGen PRIVATE ReCore nlohmann_json::nlohmann_json)

# --- 2. НАСТРОЙКА ПУТЕЙ ---
set(SEMANTICS_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/semantics.json")
set(GENERATED_CPP "${CMAKE_CURRENT_BINARY_DIR}/GeneratedSemantics.cpp")

# --- 3. АВТОМАТИЧЕСКАЯ ГЕНЕРАЦИЯ ---
add_custom_command(
        OUTPUT ${GENERATED_CPP}
        # Используем скомпилированный таргет как команду
        COMMAND SemanticsCodeGen ${SEMANTICS_CONFIG} ${GENERATED_CPP}
        # Указываем в зависимостях и сам генератор, и конфиг.
        # Если изменится код генератора ИЛИ json-конфиг, CMake перегенерирует файл.
        DEPENDS SemanticsCodeGen ${SEMANTICS_CONFIG}
        COMMENT "Generating semantics logic from JSON using SemanticsCodeGen..."
)

# --- 4. ПОДКЛЮЧЕНИЕ К ОСНОВНОМУ ПРОЕКТУ ---
# Добавляем сгенерированный файл прямо в исходники IgniLang
target_sources(IgniLang PRIVATE ${GENERATED_CPP})

# Добавляем папку сборки в include, чтобы вы могли заинклудить сгенерированный хедер (если он будет)
target_include_directories(IgniLang PRIVATE ${CMAKE_CURRENT_BINARY_DIR})