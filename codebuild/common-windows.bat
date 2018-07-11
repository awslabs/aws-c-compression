
cd ../

mkdir install

git clone https://github.com/awslabs/aws-c-common.git
mkdir build
cd build
cmake -G %1 -DCMAKE_BUILD_TYPE="Release" -DCMAKE_INSTALL_PREFIX=../../install ../
msbuild.exe aws-c-common.vcxproj /p:Configuration=Release
msbuild.exe INSTALL.vcxproj /p:Configuration=Release
ctest -V

mkdir build
cd build
cmake -G %1 -DCMAKE_BUILD_TYPE="Release" -DCMAKE_INSTALL_PREFIX=../../install ../
msbuild.exe aws-c-compression.vcxproj /p:Configuration=Release
msbuild.exe tests/aws-c-compression-tests.vcxproj /p:Configuration=Release
ctest -V
