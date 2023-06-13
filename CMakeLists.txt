 cmake_minimum_required (VERSION 3.15)

 project (myshell)

 # New CMake versions map C++20 to /std:c++20 instead of c++latest, so <format>
 # and other unstable ABI features aren't supported. Use C++23 for such versions
 if (MSVC AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
     set(CMAKE_CXX_STANDARD 23)
 else ()
     set(CMAKE_CXX_STANDARD 20)
 endif ()

 set (CMAKE_CXX_STANDARD_REQUIRED ON)
 set (CMAKE_CXX_EXTENSIONS OFF)

 if (MSVC)
     add_compile_options (/W4 /permissive- /Zc:externConstexpr /Zc:inline)
 endif ()

 add_executable (myshell "src/main.cpp")


 add_subdirectory (include)
 add_subdirectory (src)


target_include_directories(myshell PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(myshell PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_compile_options(myshell PRIVATE -Wall -Wextra -pedantic -Werror)

target_link_libraries(myshell PRIVATE stdc++fs)




