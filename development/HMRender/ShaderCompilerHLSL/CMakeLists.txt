wd_cmake_init()


wd_requires_d3d()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

wd_create_target(SHARED_LIBRARY ${PROJECT_NAME} NO_WD_PREFIX)

wd_link_target_dx11(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  RendererCore
)
