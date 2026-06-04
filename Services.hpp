// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.4 — Services.hpp
//
//  C++26: std::shared_ptr, std::shared_from_this, std::stop_token
//  Dependency-injection container for the long-lived modules the
//  MainWindow and its tabs depend on. Phase 3 of the C++26
//  modernisation plan: replace the implicit singleton chain
//  (DatabaseModule::instance, Config::Manager::instance,
//  FlagLoader-as-MainWindow-member, …) with explicit service
//  lifetimes owned by one Services object that every consumer
//  receives through a constructor argument.
//
//  Usage:
//      auto ctx = IPView::Services::create(config);
//      MainWindow w{std::move(ctx)};
//      w.tabs().packet()->setServices(w.ctx().services());
//
//  Lifetime rules:
//    * Services are shared_ptr'd. std::shared_from_this() is the
//      recommended way for a module to obtain its own Services
//      handle.
//    * Modules that need to spawn a background worker receive the
//      shutdown stop_token from create() and forward it to their
//      std::jthread constructor.
//    * Tests construct a Services with mock implementations by
//      calling makeWith<T>(...) for the relevant shared_ptr
//      slots; the rest stay default-constructed.
//
//  This header is purely additive: existing modules can keep
//  their legacy singleton accessors. The new code path is wired
//  in only when the call sites opt in.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef IPVIEW_SERVICES_HPP
#define IPVIEW_SERVICES_HPP

#include <memory>
#include <stop_token>

// Forward declarations in their actual namespaces so the
// shared_ptr<T> members here are well-formed without dragging in
// every module header.
namespace IPView::Config    { class Manager; }
namespace IPView::Storage   { class DatabaseModule; }
namespace IPView::Telemetry { class TelemetryModule;
                              class TelemetryPersistenceModule; }
namespace IPView::Packet    { class PacketModule; }
namespace IPView::Scanner   { class ScannerModule; }
namespace IPView::Auditor   { class AuditorModule; }
namespace IPView::Alert     { class AlertEngine; }
namespace IPView::Speedtest { class ServerSelectionModule; }
class NetworkManager; // global
class WhoisManager;   // global
class FlagLoader;     // global

namespace IPView {

class Services {
public:
    std::shared_ptr<Config::Manager>                config{};
    std::shared_ptr<Storage::DatabaseModule>        database{};
    std::shared_ptr<NetworkManager>                 network{};
    std::shared_ptr<Telemetry::TelemetryModule>     telemetry{};
    std::shared_ptr<Packet::PacketModule>           packet{};
    std::shared_ptr<Scanner::ScannerModule>         scanner{};
    std::shared_ptr<Auditor::AuditorModule>         auditor{};
    std::shared_ptr<Alert::AlertEngine>             alertEngine{};
    std::shared_ptr<WhoisManager>                   whois{};
    std::shared_ptr<FlagLoader>                     flags{};
    std::shared_ptr<Speedtest::ServerSelectionModule> serverSelection{};
    std::shared_ptr<Telemetry::TelemetryPersistenceModule> telemetryPersistence{};

    /// Factory that wires a default set of services and returns
    /// the container plus a stop_token that propagates shutdown
    /// to every module's jthread workers.
    [[nodiscard]]
    static std::unique_ptr<Services>
    create(std::shared_ptr<Config::Manager> cfg,
           std::stop_source shutdown = {}) noexcept;

    /// Stop token for the lifetime of the application. Worker
    /// threads query this in their loop.
    [[nodiscard]] std::stop_token stopToken() const noexcept { return mStopSource.get_token(); }

    /// Request cooperative shutdown of all background workers.
    void requestShutdown() noexcept { mStopSource.request_stop(); }

private:
    std::stop_source mStopSource{};
};

class IServiceContext {
public:
    [[nodiscard]] virtual Services& services() noexcept = 0;
    virtual ~IServiceContext() = default;
};

} // namespace IPView

#endif // IPVIEW_SERVICES_HPP
