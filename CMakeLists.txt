cmake_minimum_required(VERSION 3.20)

project(ProcAsm)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

set(ENGINE_DIR ${PROJECT_SOURCE_DIR}/src/engine)

set(ENGINGE_SRC 
    ${ENGINE_DIR}/engine.cpp
    ${ENGINE_DIR}/game.cpp
    ${ENGINE_DIR}/input.cpp
    ${ENGINE_DIR}/random.cpp
    ${ENGINE_DIR}/texture.cpp
    ${ENGINE_DIR}/ui.cpp
)

add_executable(main src/main.cpp src/assembly.cpp ${ENGINGE_SRC})

set(LIBRARIES 
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>
    $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>
    ZLIB::ZLIB
)

target_link_libraries(main PUBLIC ${LIBRARIES})

target_include_directories(main PUBLIC ${PROJECT_SOURCE_DIR}/src)