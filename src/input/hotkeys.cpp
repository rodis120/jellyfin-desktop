#include "hotkeys.h"

#include "input.h"
#include "../common.h"
#include "../platform/platform.h"
#include "../playback/coordinator.h"
#include "../browser/debug_overlay_browser.h"

namespace input {
namespace {

// Fullscreen is only meaningful when the video player is the active content.
// Music playback ignores fullscreen hotkeys; a paused video still counts as
// "active" because the user may want to toggle fullscreen while paused.
bool video_player_active() {
    if (!g_playback_coord_running.load(std::memory_order_acquire)) return false;
    PlaybackSnapshot snap = playback::snapshot();
    return snap.media_type == MediaType::Video
        && snap.presence == PlayerPresence::Present
        && snap.phase != PlaybackPhase::Stopped;
}

}  // namespace

bool hotkey_try_consume(const KeyEvent& e) {
    if (e.action != KeyAction::Down) return false;

    // Alt+F4: close window
    if (e.code == KeyCode::F4 && (e.modifiers & EVENTFLAG_ALT_DOWN)) {
        initiate_shutdown();
        return true;
    }

    if (e.code == KeyCode::F || e.code == KeyCode::F11) {
        if (!video_player_active()) return false;
        g_platform.toggle_fullscreen();
        return true;
    }

    // Ctrl+Shift+D: toggle debug overlay
    if (e.code == KeyCode::D && (e.modifiers & EVENTFLAG_CONTROL_DOWN) && (e.modifiers & EVENTFLAG_SHIFT_DOWN)) {
        DebugOverlayBrowser::toggle();
        return true;
    }
    return false;
}

}  // namespace input
