cmake_minimum_required(VERSION 2.8)
project(qvdpautest)

find_package(Threads REQUIRED)
find_package(Qt4 REQUIRED QtCore QtGui)
find_package(PkgConfig REQUIRED)

add_subdirectory(src)

#enable_testing()
#add_test(NAME ServerTests COMMAND server-test/server-test)
