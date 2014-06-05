# Copyright (C) 2014 Lars Christensen
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files(the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions :
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

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

