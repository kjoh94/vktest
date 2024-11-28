glslangValidator -V --target-env vulkan1.3 shader.comp -o shader.spv


rm -rf build/
cmake -H. -Bbuild
cmake --build build/
cp build/vktest .

