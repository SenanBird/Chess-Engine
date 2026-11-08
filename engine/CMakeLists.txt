cmake_minimum_required(VERSION 3.16)
project(ChessEngine LANGUAGES CXX)

# Both front ends and all tests link the same engine.
add_library(chess_engine STATIC
    src/bishop.cpp src/board.cpp src/castling.cpp src/draw.cpp
    src/game.cpp src/game_check.cpp src/king.cpp src/knight.cpp
    src/pawn.cpp src/queen.cpp src/rook.cpp
)
target_compile_features(chess_engine PUBLIC cxx_std_17)
target_include_directories(chess_engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")



find_package(Qt6 REQUIRED COMPONENTS Widgets)
add_executable(chess_gui gui/main.cpp)
target_link_libraries(chess_gui PRIVATE chess_engine Qt6::Widgets)

include(CTest)
if(BUILD_TESTING)
    foreach(test_name IN ITEMS castling_test draw_test move_count_test)
        add_executable(${test_name} tests/${test_name}.cpp)
        target_link_libraries(${test_name} PRIVATE chess_engine)
        # Keep assertions enabled in these tests in Release builds.
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${test_name} PRIVATE -UNDEBUG)
        endif()
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()
