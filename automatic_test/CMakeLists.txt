PROJECT(DummyDriverTest)
cmake_minimum_required(VERSION 2.8)

ENABLE_TESTING()

IF(NOT MtcaDummy_DIR)
  set(MtcaDummy_DIR "${CMAKE_SOURCE_DIR}/..")
ENDIF(NOT MtcaDummy_DIR)
  
INCLUDE_DIRECTORIES( ${MtcaDummy_DIR} include )
#message("{MtcaDummy_DIR} ${MtcaDummy_DIR}")

#create a library with the test tools
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/src ${PROJECT_NAME}_SOURCES )
add_library(${PROJECT_NAME}_TEST_LIBRARY ${${PROJECT_NAME}_SOURCES} )

#add the executables
aux_source_directory(${CMAKE_SOURCE_DIR}/executables_src testExecutables)
foreach( testExecutableSrcFile ${testExecutables})
  #NAME_WE means the base name without path and (longest) extension
  get_filename_component(excutableName ${testExecutableSrcFile} NAME_WE)
  add_executable(${excutableName} ${testExecutableSrcFile})
  target_link_libraries(${excutableName} ${PROJECT_NAME}_TEST_LIBRARY)    
  add_test(${excutableName} ${excutableName})
endforeach( testExecutableSrcFile )


