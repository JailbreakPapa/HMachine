wd_cmake_init()



wd_requires(WD_3RDPARTY_IMGUI_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

wd_create_target(LIBRARY ${PROJECT_NAME} NO_WD_PREFIX)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_IMGUI_SUPPORT)

