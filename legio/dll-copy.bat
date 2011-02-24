@echo off
mkdir lib\Win\x86
mkdir lib\Win\x64

copy ..\api\dll\Win32\Release\indigo.dll lib\Win\x86\
copy ..\api\dll\x64\Release\indigo.dll lib\Win\x64\
copy ..\api\renderer\dll\Win32\Release\indigo-renderer.dll lib\Win\x86\
copy ..\api\renderer\dll\x64\Release\indigo-renderer.dll lib\Win\x64\
copy ..\zlib-src\Win32\ReleaseDll\zlib.dll lib\Win\x86\
copy ..\zlib-src\x64\ReleaseDll\zlib.dll lib\Win\x64\