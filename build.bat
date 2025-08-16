set INCLUDES=/I ..\glfw\include\ /I ..\glfw\deps 
set LIBS=/LIBPATH:..\glfw\build\src\Debug glfw3.lib

cl /Z7 /MD %INCLUDES% rtrd_text_render_sample.c /link %LIBS% User32.lib gdi32.lib Shell32.lib
