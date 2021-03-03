cmake_minimum_required(VERSION 3.9)

Project(NvidiaDecoderPlugin VERSION 1.0
        LANGUAGES CXX CUDA)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)       #设置可执行文件的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)           #设置库文件的输出目录
set(CMAKE_BUILD_TYPE "Release")
# set(CMAKE_BUILD_TYPE "Debug")

find_package(CUDA REQUIRED)
include_directories(
    ${CUDA_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/Video_Codec_SDK_10.0.26/Interface
    E:/ffmpeg/x64/include
)
link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Video_Codec_SDK_10.0.26/Lib/x64
    E:/ffmpeg/x64/lib
)

aux_source_directory(. SRCS)
add_library(${PROJECT_NAME} SHARED ${SRCS})
target_link_libraries(${PROJECT_NAME} ${CUDA_CUDA_LIBRARY} ${CUDA_CUDART_LIBRARY} nvcuvid)
target_link_libraries(${PROJECT_NAME} avcodec avformat avutil swresample)