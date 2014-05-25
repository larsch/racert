cmake_minimum_required(VERSION 2.8)
project(iptool)
set(headers ipaddr.h
  threadqueue.h
  tick_count.h)
set(sources ipaddr.cpp
  iptool.cpp
  threadqueue.cpp
  tick_count.cpp)

if(MSVC)
  add_compile_options(/W4 /MP)
endif()

add_subdirectory(WpdPack)
add_executable(iptool ${sources} ${headers})
get_target_property(out pcap INTERFACE_INCLUDE_DIRECTORIES)
target_link_libraries(iptool pcap)
target_link_libraries(iptool iphlpapi)
target_link_libraries(iptool winmm)
target_link_libraries(iptool ws2_32)

