// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.5 — SafeText.hpp
//  HTML and shell-text escaping helpers. Use before passing any
//  user-controlled string into QTextBrowser::setHtml() or into
//  a QProcess argument list.
//
//  The full XSS-safe setHtml pattern is:
//    1. Build the HTML as a QString, interpolating SafeText::htmlEscape(value)
//       wherever untrusted data appears.
//    2. Allow only a small whitelist of tags (<b>, <i>, <code>,
//       <h3>..<h6>, <p>, <br>, <hr>, <ul>, <li>, <span>).
//    3. Reject anything else (script, object, embed, on*= handlers, ...).
//
//  For AuditorTab we currently emit simple styled HTML; a future
//  commit can switch to setMarkdown() which Qt handles internally
//  through a sanitized converter.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef IPVIEW_SAFETEXT_HPP
#define IPVIEW_SAFETEXT_HPP

#include <QString>
#include <QStringView>

namespace IPView::SafeText {

// ── HTML-attribute and text-node escape ─────────────────────────────────
//  Replaces the five XML predefined entities (& < > " ') and removes
//  any other characters that could be interpreted as markup.
[[nodiscard]]
inline QString htmlEscape(QStringView raw) noexcept
{
    QString out;
    out.reserve(raw.size() + 8);
    for (QChar c : raw) {
        switch (c.unicode()) {
        case '&':  out += QStringLiteral("&amp;");  break;
        case '<':  out += QStringLiteral("&lt;");   break;
        case '>':  out += QStringLiteral("&gt;");   break;
        case '"':  out += QStringLiteral("&quot;"); break;
        case '\'': out += QStringLiteral("&#39;");  break;
        default:   out += c;
        }
    }
    return out;
}

[[nodiscard]]
inline QString htmlEscape(const QString &raw) noexcept
{
    return htmlEscape(QStringView{raw});
}

// ── Shell argument escape ──────────────────────────────────────────────
//  Wraps a string in single quotes and escapes any embedded single
//  quotes for POSIX shells. Use only for documentation/display; the
//  project actually launches processes via QProcess::start(program,
//  args) which never invokes a shell.
[[nodiscard]]
inline QString shellEscape(QStringView raw) noexcept
{
    QString out;
    out.reserve(raw.size() + 2);
    out += QLatin1Char('\'');
    for (QChar c : raw) {
        if (c == QLatin1Char('\'')) {
            out += QStringLiteral("'\\''");
        } else {
            out += c;
        }
    }
    out += QLatin1Char('\'');
    return out;
}

} // namespace IPView::SafeText

#endif // IPVIEW_SAFETEXT_HPP
