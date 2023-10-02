set(SDL_INCLUDES "../vendor/sdl/include")
set(SDL_BINARY ../vendor/sdl/sdl3.lib)

file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE INCS ../include/*.h)
add_executable( MrDoGame ${SOURCES} ${INCS})
source_group(headers FILES ${INCS}) 

#find_package(nlohmann_json 3.11.2 REQUIRED)


set_target_properties( MrDoGame PROPERTIES RUNTIME_OUTPUT_DIRECTORY 
   ${MrDo_BINARY_DIR}/build_results/bin )

include_directories(${MrDo_SOURCE_DIR}/mrdo/include)
target_include_directories( MrDoGame SYSTEM PRIVATE ${SDL_INCLUDES})
target_link_libraries(MrDoGame PRIVATE SDL3::SDL3)
target_link_libraries(MrDoGame PRIVATE nlohmann_json::nlohmann_json)
 if (WIN32)
    add_custom_command(
        TARGET MrDoGame POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL3::SDL3>" "$<TARGET_FILE_DIR:MrDoGame>"
        VERBATIM
    )
endif()

add_custom_command(TARGET MrDoGame POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/mrdo/assets/ $<TARGET_FILE_DIR:MrDoGame>/assets/)