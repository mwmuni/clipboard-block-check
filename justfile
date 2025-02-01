
build:
    windres clipboard-manager.rc -O coff -o clipboard-manager.res
    zig cc clipboard-manager.c clipboard-manager.res -o clipboard-manager.exe -luser32 -lcomctl32 -luxtheme -lgdi32 -Wl,/subsystem:windows -lpsapi -Ofast

run:
    {{if path_exists("./clipboard-manager.exe") != "true" { \
        'just build' \
    } else { "" } \
    }}
    ./clipboard-manager.exe


