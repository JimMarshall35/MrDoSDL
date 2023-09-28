set(SDL_INCLUDES "../vendor/sdl/include")
set(SDL_BINARY ../vendor/sdl/sdl3.lib)

file(GLOB_RECURSE SOURCES *.cpp)

add_executable( MrDoGame ${SOURCES})

set_target_properties( MrDoGame PROPERTIES RUNTIME_OUTPUT_DIRECTORY 
   ${MrDo_BINARY_DIR}/build_results/bin )

include_directories(${MrDo_SOURCE_DIR}/mrdo/include)
target_include_directories( MrDoGame SYSTEM PRIVATE ${SDL_INCLUDES})
target_link_libraries( MrDoGame SDL3::SDL3)

 if (WIN32)
    add_custom_command(
        TARGET MrDoGame POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL3::SDL3>" "$<TARGET_FILE_DIR:MrDoGame>"
        VERBATIM
    )
endif()