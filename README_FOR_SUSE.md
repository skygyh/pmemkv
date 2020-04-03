You may hit following TBB problem to run cmake.

 package 'tbb' found
CMake Warning at cmake/tbb.cmake:11 (find_package):
  By not providing "FindTBB.cmake" in CMAKE_MODULE_PATH this project has
  asked CMake to find a package configuration file provided by "TBB", but
  CMake did not find one.

  Could not find a package configuration file provided by "TBB" with any of
  the following names:

    TBBConfig.cmake
    tbb-config.cmake

  Add the installation prefix of "TBB" to CMAKE_PREFIX_PATH or set "TBB_DIR"
  to a directory containing one of the above files.  If "TBB" provides a
  separate development package or SDK, be sure it has been installed.
Call Stack (most recent call first):
  CMakeLists.txt:195 (include)

=====================================================
Here is one solution verified on suse12sp4:
1. download tbb bin tar ball from  https://github.com/oneapi-src/oneTBB/releases
wget https://github.com/oneapi-src/oneTBB/releases/download/v2020.2/tbb-2020.2-lin.tgz

2. extract the above tar to wherever you want. Here saying  /home/admin/opensources
tar zxvf tbb-2020.2-lin.tgz

3. pass -DTBB_DIR=<tbb_cmake_path> when you are running CMake.
cd pmemkv/build
cmake -DTBB_DIR=/home/admin/opensources/tbb/cmake ..
