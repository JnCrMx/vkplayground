file(GLOB_RECURSE GLSL_SOURCE_FILES "*.frag" "*.vert" "*.geom")

foreach(GLSL ${GLSL_SOURCE_FILES})
	file(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${GLSL})
	set(output ${CMAKE_CURRENT_BINARY_DIR}/${rel}.spv)

	get_filename_component(output-dir ${output} DIRECTORY)
	file(MAKE_DIRECTORY ${output-dir})

	add_custom_command(
		OUTPUT ${output}
		COMMAND ${CMAKE_COMMAND} -E make_directory "${output-dir}"
		COMMAND ${GLSL_COMPILER} ${GLSL_COMPILER_FLAGS} ${GLSL} -o ${output}
		DEPENDS ${GLSL})
	list(APPEND SPIRV_BINARY_FILES ${output})

	get_filename_component(dst ${rel} DIRECTORY)
	install(FILES ${output} DESTINATION ${CMAKE_INSTALL_BINDIR}/shaders/${dst})
endforeach(GLSL)
add_custom_target(shaders DEPENDS ${SPIRV_BINARY_FILES})
