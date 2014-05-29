cmake_minimum_required(VERSION 2.8)
project(iptool)
set(headers ipaddr.h
  threadqueue.h
  tick_count.h)
set(sources ipaddr.cpp
  iptool.cpp
  threadqueue.cpp
  tick_count.cpp)

if(WIN32)
  list(APPEND sources win32/ping.cpp)
endif()

if(MSVC)
  add_compile_options(/W4 /MP)
endif()

include_directories(.)

add_executable(racert ${sources} ${headers})

if(WIN32)
  target_link_libraries(racert iphlpapi)
  target_link_libraries(racert winmm)
  target_link_libraries(racert ws2_32)
endif()

