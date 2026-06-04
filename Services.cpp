// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.0 — Services.cpp
//
//  Factory implementation. Kept in a .cpp file (rather than the
//  header) so the heavy includes for the concrete module types
//  do not leak into every translation unit that only needs the
//  forward-declared Services pointer.
// ═══════════════════════════════════════════════════════════════════════════════

#include "Services.hpp"

#include "ConfigManager.h"
#include "DatabaseModule.h"
#include "NetworkManager.h"
#include "TelemetryModule.h"
#include "PacketModule.h"
#include "ScannerModule.h"
#include "AuditorModule.h"
#include "AlertEngine.h"
#include "WhoisManager.h"
#include "FlagLoader.h"
#include "ServerSelectionModule.h"
#include "TelemetryPersistenceModule.h"

#include <utility>

namespace IPView {

// Singletons live forever; wrap them in a shared_ptr with a
// no-op deleter so Services::create can hand them out the same
// way it hands out the non-singleton modules. shared_ptr's
// deleter is invoked when the last copy dies, but for a Meyers
// singleton that "destruction" is a no-op.
template <typename T>
[[nodiscard]] std::shared_ptr<T> wrapSingleton(T& ref) noexcept {
    return std::shared_ptr<T>(std::addressof(ref), [](T*) {});
}

std::unique_ptr<Services>
Services::create(std::shared_ptr<Config::Manager> cfg,
                 std::stop_source shutdown) noexcept
{
    auto s = std::make_unique<Services>();
    s->mStopSource = shutdown;

    // Config and Database are Meyers singletons; the caller may
    // pass an explicit instance but otherwise we just take the
    // existing global one. Every other module is freely
    // constructible and gets a freshly allocated instance.
    s->config = cfg ? std::move(cfg)
                    : wrapSingleton(Config::Manager::instance());

    s->database             = wrapSingleton(Storage::DatabaseModule::instance());
    s->network              = std::make_shared<NetworkManager>();
    s->telemetry            = std::make_shared<Telemetry::TelemetryModule>();
    s->packet               = std::make_shared<Packet::PacketModule>();
    s->scanner              = std::make_shared<Scanner::ScannerModule>();
    s->auditor              = std::make_shared<Auditor::AuditorModule>();
    s->alertEngine          = std::make_shared<Alert::AlertEngine>();
    s->whois                = std::make_shared<WhoisManager>();
    s->flags                = std::make_shared<FlagLoader>();
    s->serverSelection      = std::make_shared<Speedtest::ServerSelectionModule>();
    s->telemetryPersistence = std::make_shared<Telemetry::TelemetryPersistenceModule>();

    return s;
}

} // namespace IPView
