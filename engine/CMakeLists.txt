cmake_minimum_required(VERSION 3.16)
project(ZergChess LANGUAGES CXX)

find_package(Threads REQUIRED)

add_library(chess_engine STATIC
    src/ai.cpp
    src/bishop.cpp
    src/board.cpp
    src/board_copy.cpp
    src/castling.cpp
    src/draw.cpp
    src/game.cpp
    src/game_check.cpp
    src/king.cpp
    src/knight.cpp
    src/pawn.cpp
    src/queen.cpp
    src/rook.cpp
)

target_compile_features(chess_engine PUBLIC cxx_std_17)

target_include_directories(chess_engine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(chess_engine PUBLIC Threads::Threads)

option(ZERGCHESS_BUILD_GUI "Build the Qt desktop application" ON)

if(ZERGCHESS_BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)

    add_executable(chess_gui gui/main.cpp)

    target_link_libraries(chess_gui PRIVATE
        chess_engine
        Qt6::Widgets
    )

    target_compile_definitions(chess_gui PRIVATE
        CHESS_ASSET_PATH="${CMAKE_CURRENT_SOURCE_DIR}/gui/assets"
        CHESS_HISTORY_PATH="${CMAKE_CURRENT_SOURCE_DIR}/data/games.tsv"
    )

    set_target_properties(chess_gui PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
    )
endif()

include(CTest)

if(BUILD_TESTING)
    foreach(test_name IN ITEMS
        castling_test
        draw_test
        move_count_test
        random_rules_test
        ai_test
    )
        set(test_source
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/${test_name}.cpp"
        )

        # Support the AI test's location in the original source snapshot.
        if(test_name STREQUAL "ai_test" AND NOT EXISTS "${test_source}")
            set(test_source
                "${CMAKE_CURRENT_SOURCE_DIR}/src/ai_test.cpp"
            )
        endif()

        if(NOT EXISTS "${test_source}")
            message(FATAL_ERROR
                "Missing test source: ${test_source}"
            )
        endif()

        add_executable(${test_name} "${test_source}")
        target_link_libraries(${test_name} PRIVATE chess_engine)

        if(MSVC)
            target_compile_options(${test_name} PRIVATE /UNDEBUG)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${test_name} PRIVATE -UNDEBUG)
        endif()

        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()