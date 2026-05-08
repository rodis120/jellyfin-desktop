#pragma once

#include "../cef/cef_client.h"


class DebugOverlayBrowser {
public:
    static void show();
    static void hide();
    static void toggle();

    ~DebugOverlayBrowser();

    static CefRefPtr<CefDictionaryValue> injectionProfile();

private:
    DebugOverlayBrowser();

    void update_app();
    void update_misc();
    void update_window();
    void update_video();

    void set_info(const std::string& id, const std::string& info);

    bool handleMessage(const std::string& name,
                       CefRefPtr<CefListValue> args,
                       CefRefPtr<CefBrowser> browser);

    CefRefPtr<CefLayer> layer_;
};
