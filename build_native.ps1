$ErrorActionPreference = 'Stop'
$compiler = if ($env:BG_CXX) { $env:BG_CXX } else { 'C:\Users\seo\.cache\bg-toolchain\mingw32\bin\g++.exe' }
$source = 'vendor\sc-display'
New-Item -ItemType Directory -Force assets | Out-Null
$parts = @('sc_screen.cpp','sc_console.cpp','sc_menu.cpp','sc_stormpresent.cpp','sc_engine.cpp','sc_log.cpp','sc_hook.cpp','sc_session.cpp','sc_circles.cpp') | ForEach-Object { Join-Path $source $_ }
& $compiler -O2 -static-libgcc -static-libstdc++ -shared native/bg_display.cpp native/bg_gameplay.cpp @parts -I $source -I native -o assets/bg_display.dll
if ($LASTEXITCODE) { throw 'display build failed' }
& $compiler -O2 -static-libgcc -static-libstdc++ -shared native/bg_bridge.cpp -ladvapi32 -o assets/bg_bridge.dll
if ($LASTEXITCODE) { throw 'bridge build failed' }
& $compiler -O2 -static-libgcc -static-libstdc++ -municode native/bg_start.cpp -o assets/bg_start.exe
if ($LASTEXITCODE) { throw 'start build failed' }
