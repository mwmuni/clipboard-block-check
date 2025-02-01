if (-not (Test-Path clipboard-manager.exe)) {
    & zig cc "${PSScriptRoot}/clipboard-manager.c" -o "${PSScriptRoot}/clipboard-manager.exe" -luser32 -lcomctl32 -luxtheme -lgdi32 "-Wl,/subsystem:windows" -lpsapi -Ofast
}

Start-Process -FilePath "${PSScriptRoot}/clipboard-manager.exe"

