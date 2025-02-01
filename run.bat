if not exist clipboard-manager.exe (
    zig cc clipboard-manager.c -o clipboard-manager.exe -luser32 -lcomctl32 -luxtheme -lgdi32 -Wl,/subsystem:windows -lpsapi -Ofast
)

start "" clipboard-manager.exe
