wd_cmake_init()



# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

wd_create_target(SHARED_LIBRARY ${PROJECT_NAME} NO_WD_PREFIX)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
endif()

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
)
