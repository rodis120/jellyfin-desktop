#include "debug_overlay_browser.h"
#include "app_menu.h"
#include "browsers.h"
#include "../common.h"
#include "../mpv/event.h"
#include "logging.h"
#include "../input/dispatch.h"
#include "../platform/platform.h"
#include "include/cef_task.h"
#include "version.h"
#include "settings.h"
#include "mpv/handle.h"

#include <cmath>
#include <functional>

#define MPV_PROPERTY(p) mpv_get_property_std_string(g_mpv.Get(), p)
#define MPV_PROPERTY_BOOL(p) mpv_get_property_bool(g_mpv.Get(), p)


extern Platform g_platform;

namespace {

DebugOverlayBrowser* s_self = nullptr;

class FnTask : public CefTask {
public:
    explicit FnTask(std::function<void()> fn) : fn_(std::move(fn)) {}
    void Execute() override { if (fn_) fn_(); }
private:
    std::function<void()> fn_;
    IMPLEMENT_REFCOUNTING(FnTask);
};

bool mpv_get_property_bool(mpv_handle* mpv, const char* prop) {
    int flag;
    mpv_get_property(mpv, prop, MPV_FORMAT_FLAG, &flag);
    return static_cast<bool>(flag);
}

std::string mpv_get_property_std_string(mpv_handle* mpv, const char* prop) {
    auto str = mpv_get_property_osd_string(mpv, prop);
    if(str == nullptr) {
        return "";
    }

    std::string output(str);
    mpv_free(str);

    return output;
}

std::string qoute_string(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('"');

    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\n': out += "\\n";  break;
            case '\t': out += "\\t";  break;
            case '\r': out += "\\r";  break;
            default:   out.push_back(c);
        }
    }

    out.push_back('"');
    return out;
}

}

CefRefPtr<CefDictionaryValue> DebugOverlayBrowser::injectionProfile() {
    static const char* const kFunctions[] = {
        "update"
    };
    static const char* const kScripts[] = {};
    CefRefPtr<CefListValue> fns = CefListValue::Create();
    for (size_t i = 0; i < sizeof(kFunctions) / sizeof(*kFunctions); i++)
        fns->SetString(i, kFunctions[i]);
    CefRefPtr<CefListValue> scripts = CefListValue::Create();
    for (size_t i = 0; i < sizeof(kScripts) / sizeof(*kScripts); i++)
        scripts->SetString(i, kScripts[i]);
    CefRefPtr<CefDictionaryValue> d = CefDictionaryValue::Create();
    d->SetList("functions", fns);
    d->SetList("scripts", scripts);
    return d;
}

DebugOverlayBrowser::DebugOverlayBrowser()
    : layer_(g_browsers->create(injectionProfile()))
{
    layer_->setName("debug overlay");

    layer_->setMessageHandler([this](const std::string& name,
                                      CefRefPtr<CefListValue> args,
                                      CefRefPtr<CefBrowser> browser) {
        return handleMessage(name, args, browser);
    });

    layer_->setBeforeCloseCallback([]() {
        DebugOverlayBrowser* self = s_self;
        s_self = nullptr;
        if (!self) return;
        CefPostTask(TID_UI, CefRefPtr<CefTask>(new FnTask([self]() { delete self; })));
    });
}

DebugOverlayBrowser::~DebugOverlayBrowser() {
    release_layer(layer_.get());
}

bool DebugOverlayBrowser::handleMessage(const std::string& name,
                                 CefRefPtr<CefListValue> args,
                                 CefRefPtr<CefBrowser> browser) {
    if (name == "update") {
        std::stringstream debug_info;
        debug_info << "Jellyfin\n";
        debug_info << "  Version: " << APP_VERSION_FULL " built " __DATE__ " " __TIME__ << "\n";
        debug_info << "  CEF Version: " << APP_CEF_VERSION << "\n";
        // debug_info << "  Platform: " << TARGET_PLATFORM "-" TARGET_ARCH << "\n";

        debug_info << "\nFiles:\n";
        // debug_info << "  Config file: " << Settings::instance().getConfigPath() << "\n";

        // debug_info << "\nNetwork addresses:\n";
        // for (auto& addr : networkAddresses()) {
        //     debug_info << "  " << addr << "\n";
        // }

        debug_info << "\nClient settings:\n";
        debug_info << Settings::instance().cliSettingsJson() << "\n";

        debug_info << "\nWindow:\n";
        // debug_info << "  Display backend: " << displayBackendToString(g_platform.display) << "\n";
        debug_info << "  Physical size: " << mpv::osd_pw() << "x" << mpv::osd_ph() << "\n";
        debug_info << "  Scale: " << mpv::display_scale() << "\n";

        std::stringstream video_info;
        video_info << "File:\n";
        video_info << "URL: " << MPV_PROPERTY("path") << "\n";
        video_info << "Container: " << MPV_PROPERTY("file-format") << "\n";
        video_info << "Native seeking: " << ((MPV_PROPERTY_BOOL("seekable") &&
                                        !MPV_PROPERTY_BOOL("partially-seekable"))
                                        ? "yes" : "no") << "\n";
        video_info << "\n";
        video_info << "Video:\n";
        video_info << "Codec: " << MPV_PROPERTY("video-codec") << "\n";
        video_info << "Size: " << MPV_PROPERTY("video-params/dw") << "x"
                        << MPV_PROPERTY("video-params/dh") << "\n";
        video_info << "FPS (container): " << MPV_PROPERTY("container-fps") << "\n";
        video_info << "FPS (filters): " << MPV_PROPERTY("estimated-vf-fps") << "\n";
        video_info << "Aspect: " << MPV_PROPERTY("video-params/aspect") << "\n";
        video_info << "Bitrate: " << MPV_PROPERTY("video-bitrate") << "\n";
        video_info << "Display FPS: " << MPV_PROPERTY("display-fps") << "\n";
        video_info << "Hardware Decoding: " << MPV_PROPERTY("hwdec-current")
                                        << " (" << MPV_PROPERTY("hwdec-interop") << ")\n";
        video_info << "\n";
        video_info << "Audio:\n";
        video_info << "Codec: " << MPV_PROPERTY("audio-codec") << "\n";
        video_info << "Bitrate: " << MPV_PROPERTY("audio-bitrate") << "\n";
        video_info << "Channels: ";
        // appendAudioFormat(video_info, "audio-params");
        video_info << " -> ";
        // appendAudioFormat(video_info, "audio-out-params");
        video_info << "\n";
        video_info << "Output driver: " << MPV_PROPERTY("current-ao") << "\n";
        video_info << "\n";
        video_info << "Performance:\n";
        video_info << "A/V: " << MPV_PROPERTY("avsync") << "\n";
        video_info << "Dropped frames: " << MPV_PROPERTY("vo-drop-frame-count") << "\n";
        bool dispSync = MPV_PROPERTY_BOOL("display-sync-active");
        video_info << "Display Sync: ";
        if (!dispSync)
        {
            video_info << "no\n";
        }
        else
        {
            video_info << "yes (ratio " << MPV_PROPERTY("vsync-ratio") << ")\n";
            video_info << "Mistimed frames: " << MPV_PROPERTY("mistimed-frame-count")
                                        << "/" << MPV_PROPERTY("vo-delayed-frame-count") << "\n";
            video_info << "Measured FPS: " << MPV_PROPERTY("estimated-display-fps")
                                    << " (" << MPV_PROPERTY("vsync-jitter") << ")\n";
            video_info << "V. speed corr.: " << MPV_PROPERTY("video-speed-correction") << "\n";
            video_info << "A. speed corr.: " << MPV_PROPERTY("audio-speed-correction") << "\n";
        }
        video_info << "\n";
        video_info << "Cache:\n";
        video_info << "Seconds: " << MPV_PROPERTY("demuxer-cache-duration") << "\n";
        video_info << "Extra readahead: " << MPV_PROPERTY("cache-used") << "\n";
        video_info << "Buffering: " << MPV_PROPERTY("cache-buffering-state") << "\n";
        video_info << "Speed: " << MPV_PROPERTY("cache-speed") << "\n";
        video_info << "\n";
        video_info << "Misc:\n";
        video_info << "Time: " << MPV_PROPERTY("playback-time") << " / "
                        << MPV_PROPERTY("duration")
                        << " (" << MPV_PROPERTY("percent-pos") << "%)\n";
        video_info << "State: " << (MPV_PROPERTY_BOOL("pause") ? "paused " : "")
                            << (MPV_PROPERTY_BOOL("paused-for-cache") ? "buffering " : "")
                            << (MPV_PROPERTY_BOOL("core-idle") ? "waiting " : "playing ")
                            << (MPV_PROPERTY_BOOL("seeking") ? "seeking " : "")
                            << "\n";

        layer_->execJs("window.set_debug_info(" + qoute_string(debug_info.str()) + ");");
        layer_->execJs("window.set_video_info(" + qoute_string(video_info.str()) + ");");

        return true;
    }
    return false;
}


void DebugOverlayBrowser::show() {
    if (s_self) {
        LOG_INFO(LOG_CEF, "DebugOverlayBrowser::show(): already visible");
        return;
    }
    if (!g_browsers) {
        LOG_WARN(LOG_CEF, "DebugOverlayBrowser::open(): no Browsers instance, ignoring");
        return;
    }
    LOG_INFO(LOG_CEF, "DebugOverlayBrowser::show()");

    s_self = new DebugOverlayBrowser();
    s_self->layer_->setVisible(true);
    s_self->layer_->create("app://resources/debug_overlay.html");
}

void DebugOverlayBrowser::hide() {
    if (!s_self) {
        LOG_INFO(LOG_CEF, "DebugOverlayBrowser::hide(): already hidden");
        return;
    }
    LOG_INFO(LOG_CEF, "DebugOverlayBrowser::hide()");

    s_self->layer_->browser()->GetHost()->CloseBrowser(false);
}

void DebugOverlayBrowser::toggle() {
    if (s_self) {
        DebugOverlayBrowser::hide();
    } else {
        DebugOverlayBrowser::show();
    }
}
