cmake_minimum_required(VERSION 3.16)
project(easy_on_the_eyes C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra)

set(EOTE_SOURCES
    src/cli.c
    src/config.c
    src/engine.c
    src/engine_css.c
    src/engine_html.c
    src/engine_js.c
    src/engine_json.c
    src/ignore.c
    src/util.c
    src/walker.c
)

add_executable(easy-on-the-eyes src/main.c ${EOTE_SOURCES})
target_link_libraries(easy-on-the-eyes PRIVATE pthread)

set(EOTE_TEST_SOURCES
    tests/test_runner.c
    tests/test_cli.c
    tests/test_config.c
    tests/test_ignore.c
    tests/test_engine_json.c
    tests/test_engine_css.c
    tests/test_engine_js.c
    tests/test_engine_html.c
)

add_executable(test_runner ${EOTE_TEST_SOURCES} ${EOTE_SOURCES})
target_include_directories(test_runner PRIVATE src)
target_link_libraries(test_runner PRIVATE pthread)

add_custom_target(run_tests
    COMMAND test_runner
    DEPENDS test_runner
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
