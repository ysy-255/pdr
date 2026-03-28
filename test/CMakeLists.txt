add_test(NAME サンプル
	COMMAND pdr2svg
	${CMAKE_CURRENT_SOURCE_DIR}/sample/in.pdr
	${CMAKE_CURRENT_SOURCE_DIR}/sample/result.svg
)
add_test(NAME パスのd
	COMMAND pdr2svg
	${CMAKE_CURRENT_SOURCE_DIR}/path_d/in.pdr
	${CMAKE_CURRENT_SOURCE_DIR}/path_d/result.svg
)
