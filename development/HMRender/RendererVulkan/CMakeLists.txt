wd_cmake_init()


wd_requires_vulkan()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

wd_create_target(SHARED_LIBRARY ${PROJECT_NAME} NO_WD_PREFIX)

if (WD_CMAKE_PLATFORM_WINDOWS)
	target_compile_definitions(${PROJECT_NAME} PRIVATE VK_USE_PLATFORM_WIN32_KHR)
elseif (WD_CMAKE_PLATFORM_LINUX)
	target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-nullability-completeness" "-Wno-enum-compare" "-Wno-switch")
endif()

wd_link_target_vulkan(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
  RendererFoundation
)

if(WD_3RDPARTY_GLFW_SUPPORT)

  target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

endif()
