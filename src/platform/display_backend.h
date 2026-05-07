#pragma once

enum class DisplayBackend { Wayland, X11, Windows, macOS };

static const char* displayBackendToString(DisplayBackend backend) {
    switch(backend) {
        case DisplayBackend::Wayland:
            return "Wayland";
        case DisplayBackend::X11:
            return "X11";
        case DisplayBackend::Windows:
            return "Windows";
        case DisplayBackend::macOS:
            return "macOS";
    };

    return "";
}
