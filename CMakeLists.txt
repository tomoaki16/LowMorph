cmake_minimum_required(VERSION 3.25)
project(LowMorph VERSION 0.3.0 LANGUAGES CXX)
if(NOT DEFINED VST3_SDK_ROOT)
  message(FATAL_ERROR "Set VST3_SDK_ROOT")
endif()
set(vst3sdk_SOURCE_DIR "${VST3_SDK_ROOT}")
add_subdirectory("${VST3_SDK_ROOT}" "${CMAKE_BINARY_DIR}/vst3sdk")
smtg_enable_vst3_sdk()
smtg_add_vst3plugin(LowMorph
  source/processor.cpp source/processor.h
  source/controller.cpp source/controller.h
  source/entry.cpp source/ids.h
  core/LowMorphCore.h)
target_include_directories(LowMorph PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(LowMorph PRIVATE sdk)
smtg_target_configure_version_file(LowMorph)
