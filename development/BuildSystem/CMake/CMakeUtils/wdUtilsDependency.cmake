# #####################################
# ## wd_add_dependency(<dstTarget> <srcTarget>)
# #####################################

function(wd_add_dependency DST_TARGET SRC_TARGET)
	if(NOT TARGET ${DST_TARGET})
		# message(STATUS "DST_TARGET '${DST_TARGET}' is unknown")
		return()
	endif()

	if(NOT TARGET ${SRC_TARGET})
		# message(STATUS "SRC_TARGET '${SRC_TARGET}' is unknown")
		return()
	endif()

	add_dependencies(${DST_TARGET} ${SRC_TARGET})
endfunction()

# #####################################
# ## wd_add_as_runtime_dependency(<target>)
# #####################################
function(wd_add_as_runtime_dependency TARGET_NAME)
	# editor
	wd_add_dependency(EditorProcessor ${TARGET_NAME})

	# player
	wd_add_dependency(Player ${TARGET_NAME})
endfunction()