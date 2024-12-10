cmake_minimum_required(VERSION 2.8)

project(RollerCoasters)
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src/)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include/)
set(LIB_DIR ${PROJECT_SOURCE_DIR}/lib/)

set(SRC_SHADER 
    ${PROJECT_SOURCE_DIR}/assets/shaders/simple.vert
    ${PROJECT_SOURCE_DIR}/assets/shaders/simple.frag)

set(SRC_RENDER_UTILITIES
    ${SRC_DIR}RenderUtilities/BufferObject.h
    ${SRC_DIR}RenderUtilities/Shader.h
    ${SRC_DIR}RenderUtilities/Texture.h)

include_directories(${INCLUDE_DIR})
include_directories(${INCLUDE_DIR}glad4.6/include/)
include_directories(${INCLUDE_DIR}glm-0.9.8.5/glm/)
include_directories(${INCLUDE_DIR}assimp/)

add_Definitions("-D_XKEYCHECK_H")
add_definitions(-DPROJECT_DIR="${PROJECT_SOURCE_DIR}")

add_executable(RollerCoasters
    ${SRC_DIR}CallBacks.h
    ${SRC_DIR}ControlPoint.h
    ${SRC_DIR}Object.h
    ${SRC_DIR}Track.h
    ${SRC_DIR}TrainView.h
    ${SRC_DIR}TrainWindow.h

    ${SRC_DIR}main.cpp
    ${SRC_DIR}CallBacks.cpp
    ${SRC_DIR}ControlPoint.cpp
    ${SRC_DIR}Track.cpp
    ${SRC_DIR}TrainView.cpp
    ${SRC_DIR}TrainWindow.cpp

    ${SRC_SHADER}
    ${SRC_RENDER_UTILITIES}

    ${INCLUDE_DIR}glad4.6/src/glad.c
    ${INCLUDE_DIR}stb_image.h
)
source_group("shaders" FILES ${SRC_SHADER})
source_group("RenderUtilities" FILES ${SRC_RENDER_UTILITIES})


add_library(Utilities 
    ${SRC_DIR}Utilities/ArcBallCam.h
    ${SRC_DIR}Utilities/3DUtils.h
    ${SRC_DIR}Utilities/Pnt3f.h
    ${SRC_DIR}Utilities/ArcBallCam.cpp
    ${SRC_DIR}Utilities/3DUtils.cpp
    ${SRC_DIR}Utilities/Pnt3f.cpp)

target_link_libraries(RollerCoasters 
    debug ${LIB_DIR}Debug/fltk_formsd.lib      optimized ${LIB_DIR}Release/fltk_forms.lib
    debug ${LIB_DIR}Debug/fltk_gld.lib         optimized ${LIB_DIR}Release/fltk_gl.lib
    debug ${LIB_DIR}Debug/fltk_imagesd.lib     optimized ${LIB_DIR}Release/fltk_images.lib
    debug ${LIB_DIR}Debug/fltk_jpegd.lib       optimized ${LIB_DIR}Release/fltk_jpeg.lib
    debug ${LIB_DIR}Debug/fltk_pngd.lib        optimized ${LIB_DIR}Release/fltk_png.lib
    debug ${LIB_DIR}Debug/fltk_zd.lib          optimized ${LIB_DIR}Release/fltk_z.lib
    debug ${LIB_DIR}Debug/fltkd.lib            optimized ${LIB_DIR}Release/fltk.lib
    debug ${LIB_DIR}Debug/opencv_world341d.lib optimized ${LIB_DIR}Release/opencv_world341.lib)

target_link_libraries(RollerCoasters 
    ${LIB_DIR}OpenGL32.lib
    ${LIB_DIR}glu32.lib
    ${LIB_DIR}common.lib
    ${LIB_DIR}ex-common.lib
    ${LIB_DIR}OpenAL32.lib
    ${LIB_DIR}alut.lib
    ${LIB_DIR}alut_static.lib
    ${LIB_DIR}assimp-vc142-mt.lib)

target_link_libraries(RollerCoasters Utilities)

# 需要複製到執行檔路徑下的dll
set(DLL_SOURCE_PATHS
    ${LIB_DIR}dll/opencv_world341.dll
    ${LIB_DIR}dll/opencv_world341d.dll
    ${LIB_DIR}dll/alut.dll
    ${LIB_DIR}dll/OpenAL32.dll
    ${LIB_DIR}dll/assimp-vc142-mt.dll
)
# 每次編譯完成後，把dll複製到執行檔所在資料夾(debug/release)
foreach(DLL_PATH ${DLL_SOURCE_PATHS})
    add_custom_command(TARGET RollerCoasters POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DLL_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)
    )
endforeach()

set(ASSETS_SOURCE_PATHS
    ${PROJECT_SOURCE_DIR}/assets
)

# 每次編譯完成後，複製資料夾到執行檔所在資料夾(debug/release)
foreach(SOURCE_DIR ${ASSETS_SOURCE_PATHS})
	get_filename_component(SOURCE_DIR_NAME ${SOURCE_DIR} NAME)
	add_custom_command(TARGET RollerCoasters POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory 
		${SOURCE_DIR}
		${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)/${SOURCE_DIR_NAME}
	)
endforeach()

# 設定visual studio在編輯情況下的工作目錄與輸出目錄相同
set_target_properties(RollerCoasters PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"
)

# 設定起始專案
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT RollerCoasters)