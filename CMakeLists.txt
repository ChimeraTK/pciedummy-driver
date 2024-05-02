project("pciedummy")
cmake_minimum_required(VERSION 3.5)

set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 10)
set(${PROJECT_NAME}_PATCH_VERSION 02)

include(cmake/set_version_numbers.cmake)

file(GLOB dkms_source *.h.in *.h *.c Makefile)
install(FILES ${dkms_source} DESTINATION src/mtcadummy-${${PROJECT_NAME}_VERSION})
install(FILES mtcadummy.conf DESTINATION etc/modules-load.d)
install(FILES 10-mtcadummy.rules DESTINATION etc/udev/rules.d)
enable_testing()
add_test(NAME dummy COMMAND true)
