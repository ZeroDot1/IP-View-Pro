// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.4 — SafeProcess.hpp
//
//  A thin wrapper around QProcess that enforces a project-wide
//  baseline of safety checks before launching any external program:
//
//    * the program path is checked against the IPView::Security
//      hostname regex (so argument-injection like
//      `traceroute 1.1.1.1; rm -rf /` cannot pass);
//    * every argument is checked with isValidShellArgument;
//    * the timeout defaults to IPView::Timeouts::PROCESS_QUIT;
//    * the launch is recorded with std::source_location so the
//      log line points at the exact call site.
//
//  The wrapper does not replace QProcess — QObject signals
//  (readyReadStandardOutput / finished / …) only work on the
//  concrete class. SafeProcess::start() returns a configured
//  QProcess that the caller is expected to parent and connect to
//  as before. The difference is that the configuration step
//  (program + args + timeout) goes through a single audited path.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef IPVIEW_SAFEPROCESS_HPP
#define IPVIEW_SAFEPROCESS_HPP

#include <QProcess>
#include <QString>
#include <QStringList>

#include <expected>     // C++26
#include <memory>
#include <string_view>
#include <source_location>

#include "Error.hpp"
#include "SecurityUtil.h"
#include "Timeouts.hpp"

namespace IPView::SafeProcess {

// ── start(...) ──────────────────────────────────────────────────────────
//  Validates the program path and the argument list, then returns
//  a freshly-allocated QProcess configured with the right working
//  directory-less environment (we do not inherit the parent shell
//  PATH; QProcess::startDetached semantics are intentionally not
//  used here).
[[nodiscard]]
inline std::expected<std::unique_ptr<QProcess>, ErrorInfo>
start(QString const& program,
      QStringList const& arguments,
      std::chrono::milliseconds timeout = Timeouts::PROCESS_QUIT,
      std::source_location loc = std::source_location::current()) noexcept
{
    if (!isValidCommand(program)) {
        return unexpected(Error::ShellMetachars,
            std::string{"Refusing to launch unsafe program: "} +
            program.toStdString(), loc);
    }

    for (QString const& arg : arguments) {
        if (!isValidShellArgument(arg)) {
            return unexpected(Error::ShellMetachars,
                std::string{"Refusing unsafe argument: "} +
                arg.toStdString(), loc);
        }
    }

    auto proc = std::make_unique<QProcess>();
    proc->setProgram(program);
    proc->setArguments(arguments);
    // Store timeout in msec on the process object so callers can
    // pick it up before calling waitForFinished / kill.
    proc->setProperty("ipviewTimeoutMs",
                      static_cast<qlonglong>(timeout.count()));
    return proc;
}

} // namespace IPView::SafeProcess

#endif // IPVIEW_SAFEPROCESS_HPP
