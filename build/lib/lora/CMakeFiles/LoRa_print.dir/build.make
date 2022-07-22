# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.23

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/jirka/programovani/pico/Picopod

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/jirka/programovani/pico/Picopod/build

# Include any dependencies generated for this target.
include lib/lora/CMakeFiles/LoRa_print.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include lib/lora/CMakeFiles/LoRa_print.dir/compiler_depend.make

# Include the progress variables for this target.
include lib/lora/CMakeFiles/LoRa_print.dir/progress.make

# Include the compile flags for this target's objects.
include lib/lora/CMakeFiles/LoRa_print.dir/flags.make

lib/lora/CMakeFiles/LoRa_print.dir/Print.cpp.obj: lib/lora/CMakeFiles/LoRa_print.dir/flags.make
lib/lora/CMakeFiles/LoRa_print.dir/Print.cpp.obj: ../lib/lora/Print.cpp
lib/lora/CMakeFiles/LoRa_print.dir/Print.cpp.obj: lib/lora/CMakeFiles/LoRa_print.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jirka/programovani/pico/Picopod/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object lib/lora/CMakeFiles/LoRa_print.dir/Print.cpp.obj"
	cd /home/jirka/programovani/pico/Picopod/build/lib/lora && /usr/sbin/arm-none-eabi-g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT lib/lora/CMakeFiles/LoRa_print.dir/Print.cpp.obj -MF CMakeFiles/LoRa_print.dir/Print.cpp.obj.d -o CMakeFiles/LoRa_print.dir/Print.cpp.obj -c /home/jirka/programovani/pico/Picopod/lib/lora/Print.cpp

lib/lora/CMakeFiles/LoRa_print.dir/Print.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/LoRa_print.dir/Print.cpp.i"
	cd /home/jirka/programovani/pico/Picopod/build/lib/lora && /usr/sbin/arm-none-eabi-g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/jirka/programovani/pico/Picopod/lib/lora/Print.cpp > CMakeFiles/LoRa_print.dir/Print.cpp.i

lib/lora/CMakeFiles/LoRa_print.dir/Print.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/LoRa_print.dir/Print.cpp.s"
	cd /home/jirka/programovani/pico/Picopod/build/lib/lora && /usr/sbin/arm-none-eabi-g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/jirka/programovani/pico/Picopod/lib/lora/Print.cpp -o CMakeFiles/LoRa_print.dir/Print.cpp.s

# Object files for target LoRa_print
LoRa_print_OBJECTS = \
"CMakeFiles/LoRa_print.dir/Print.cpp.obj"

# External object files for target LoRa_print
LoRa_print_EXTERNAL_OBJECTS =

lib/lora/libLoRa_print.a: lib/lora/CMakeFiles/LoRa_print.dir/Print.cpp.obj
lib/lora/libLoRa_print.a: lib/lora/CMakeFiles/LoRa_print.dir/build.make
lib/lora/libLoRa_print.a: lib/lora/CMakeFiles/LoRa_print.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/jirka/programovani/pico/Picopod/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libLoRa_print.a"
	cd /home/jirka/programovani/pico/Picopod/build/lib/lora && $(CMAKE_COMMAND) -P CMakeFiles/LoRa_print.dir/cmake_clean_target.cmake
	cd /home/jirka/programovani/pico/Picopod/build/lib/lora && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/LoRa_print.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
lib/lora/CMakeFiles/LoRa_print.dir/build: lib/lora/libLoRa_print.a
.PHONY : lib/lora/CMakeFiles/LoRa_print.dir/build

lib/lora/CMakeFiles/LoRa_print.dir/clean:
	cd /home/jirka/programovani/pico/Picopod/build/lib/lora && $(CMAKE_COMMAND) -P CMakeFiles/LoRa_print.dir/cmake_clean.cmake
.PHONY : lib/lora/CMakeFiles/LoRa_print.dir/clean

lib/lora/CMakeFiles/LoRa_print.dir/depend:
	cd /home/jirka/programovani/pico/Picopod/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/jirka/programovani/pico/Picopod /home/jirka/programovani/pico/Picopod/lib/lora /home/jirka/programovani/pico/Picopod/build /home/jirka/programovani/pico/Picopod/build/lib/lora /home/jirka/programovani/pico/Picopod/build/lib/lora/CMakeFiles/LoRa_print.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : lib/lora/CMakeFiles/LoRa_print.dir/depend

