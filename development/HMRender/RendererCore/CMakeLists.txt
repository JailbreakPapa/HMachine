wd_cmake_init()



# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

wd_create_target(SHARED_LIBRARY ${PROJECT_NAME} NO_WD_PREFIX)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Core
  RendererFoundation
  Texture
)

if (WD_3RDPARTY_OZZ_SUPPORT)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ozz
  )
endif()
