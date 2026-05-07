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

    bool handleMessage(const std::string& name,
                       CefRefPtr<CefListValue> args,
                       CefRefPtr<CefBrowser> browser);

    CefRefPtr<CefLayer> layer_;
};
