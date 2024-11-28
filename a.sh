glslangValidator -V --target-env vulkan1.3 shader.comp -o shader.spv


rm -rf build/
cmake -H. -Bbuild
cmake --build build/
cp build/vktest .


#export VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_api_dump
#export VK_LAYER_LUNARG_API_DUMP_LOG_FILE=/home/kjoh/vktest/vkcall.txt
#export VK_LAYER_LUNARG_API_DUMP_VERBOSE=ON

