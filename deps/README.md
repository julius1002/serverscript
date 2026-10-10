# Dependencies

## What you need

1. Clone https://github.com/julius1002/blocking-queue

2. wget https://github.com/civetweb/civetweb/archive/refs/tags/v1.16.zip && unzip v1.16.zip

3. After unzip you should build civetweb

4. You need to install libpq for postgresql and set it correctly in the CMakeLists.txt (find it with strg + f and replace the include path and the dylib path with yours)

5. Then you should be able to build the whole project with make
