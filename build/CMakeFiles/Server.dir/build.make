# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.18

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /media/wade/WadeGao/WinterVacation/实验楼/C++实现即时通讯软件

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /media/wade/WadeGao/WinterVacation/实验楼/C++实现即时通讯软件/build

# Include any dependencies generated for this target.
include CMakeFiles/Server.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/Server.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/Server.dir/flags.make

CMakeFiles/Server.dir/Server.cpp.o: CMakeFiles/Server.dir/flags.make
CMakeFiles/Server.dir/Server.cpp.o: ../Server.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/media/wade/WadeGao/WinterVacation/实验楼/C++实现即时通讯软件/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/Server.dir/Server.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Server.dir/Server.cpp.o -c /media/wade/WadeGao/WinterVacation/实验楼/C++实现即时通讯软件/Server.cpp

CMakeFiles/Server.dir/Server.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Server.dir/Server.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /media/wade/WadeGao/WinterVacation/实验楼/C++实现即时通讯软件/Server.cpp > CMakeFiles/Server.dir/Server.cpp.i

CMakeFiles/Server.dir/Server.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Server.dir/Server.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /media/wade/WadeGao/WinterVacation/实验楼/C++实现即时通讯软件/Server.cpp -o CMakeFiles/Server.dir/Server.cpp.s

CMakeFiles/Server.dir/ServerMain.cpp.o: CMakeFiles/Server.dir/flags.make
CMakeFiles/Server.dir/ServerMain.cpp.o: ../ServerMain.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/media/wade/WadeGao/WinterVacation/实验楼/C++实现即时通讯软件/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/Server.dir/ServerMain.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Server.dir/ServerMain.cpp.o -c /media/wade/WadeGao/WinterVacation/实验楼/C++实现即时通讯软件/ServerMain.cpp

CMakeFiles/Server.dir/ServerMain.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Server.dir/ServerMain.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /media/wade/WadeGao/WinterVacation/实验楼/C++实现即时通讯软件/ServerMain.cpp > CMakeFiles/Server.dir/ServerMain.cpp.i

CMakeFiles/Server.dir/ServerMain.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Server.dir/ServerMain.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /media/wade/WadeGao/WinterVacation/实验楼/C++实现即时通讯软件/ServerMain.cpp -o CMakeFiles/Server.dir/ServerMain.cpp.s

# Object files for target Server
Server_OBJECTS = \
"CMakeFiles/Server.dir/Server.cpp.o" \
"CMakeFiles/Server.dir/ServerMain.cpp.o"

# External object files for target Server
Server_EXTERNAL_OBJECTS =

Server: CMakeFiles/Server.dir/Server.cpp.o
Server: CMakeFiles/Server.dir/ServerMain.cpp.o
Server: CMakeFiles/Server.dir/build.make
Server: CMakeFiles/Server.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/media/wade/WadeGao/WinterVacation/实验楼/C++实现即时通讯软件/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable Server"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Server.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/Server.dir/build: Server

.PHONY : CMakeFiles/Server.dir/build

CMakeFiles/Server.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/Server.dir/cmake_clean.cmake
.PHONY : CMakeFiles/Server.dir/clean

CMakeFiles/Server.dir/depend:
	cd /media/wade/WadeGao/WinterVacation/实验楼/C++实现即时通讯软件/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /media/wade/WadeGao/WinterVacation/实验楼/C++实现即时通讯软件 /media/wade/WadeGao/WinterVacation/实验楼/C++实现即时通讯软件 /media/wade/WadeGao/WinterVacation/实验楼/C++实现即时通讯软件/build /media/wade/WadeGao/WinterVacation/实验楼/C++实现即时通讯软件/build /media/wade/WadeGao/WinterVacation/实验楼/C++实现即时通讯软件/build/CMakeFiles/Server.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/Server.dir/depend

