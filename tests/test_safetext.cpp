// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.5 — tests/test_safetext.cpp
//
//  Validates the HTML and shell escaping helpers in SafeText.hpp.
//
//  We deliberately use plain string literals (const char*) instead of
//  QStringLiteral here. QStringLiteral is a multi-line macro that the
//  preprocessor cannot fully inline inside our IPVIEW_TEST_CASE body
//  wrapper without confusing -Werror; the const-char* form is enough
//  to exercise the escaping logic.
// ═══════════════════════════════════════════════════════════════════════════════

#include "TestFramework.hpp"
#include "../SafeText.hpp"

IPVIEW_TEST_CASE(htmlEscape_neutralises_script_tag,
    QString const input = QString::fromUtf8("<script>alert(1)</script>");
    QString const out   = IPView::SafeText::htmlEscape(input);
    IPVIEW_CHECK(out.contains(QString::fromUtf8("&lt;script&gt;")));
    IPVIEW_CHECK(out.contains(QString::fromUtf8("&lt;/script&gt;")));
    IPVIEW_CHECK(!out.contains(QString::fromUtf8("<script>")))
)

IPVIEW_TEST_CASE(htmlEscape_escapes_all_five_xml_entities,
    QString const input = QString::fromUtf8("a&b<c>d\"e'f");
    QString const out   = IPView::SafeText::htmlEscape(input);
    IPVIEW_CHECK_EQ(out, QString::fromUtf8("a&amp;b&lt;c&gt;d&quot;e&#39;f"))
)

IPVIEW_TEST_CASE(htmlEscape_empty_string_returns_empty,
    IPVIEW_CHECK_EQ(IPView::SafeText::htmlEscape(QString{}), QString{})
)

IPVIEW_TEST_CASE(htmlEscape_passes_plain_text_through,
    QString const input = QString::fromUtf8("hello world 123");
    IPVIEW_CHECK_EQ(IPView::SafeText::htmlEscape(input), input)
)

IPVIEW_TEST_CASE(htmlEscape_handles_onclick_attribute,
    QString const input = QString::fromUtf8("\" onmouseover=\"alert(1)");
    QString const out   = IPView::SafeText::htmlEscape(input);
    IPVIEW_CHECK(!out.contains(QString::fromUtf8("\"")));
    IPVIEW_CHECK(out.contains(QString::fromUtf8("&quot;")))
)

IPVIEW_TEST_CASE(shellEscape_wraps_in_single_quotes,
    QString const input = QString::fromUtf8("hello world");
    IPVIEW_CHECK_EQ(IPView::SafeText::shellEscape(input),
                    QString::fromUtf8("'hello world'"))
)

IPVIEW_TEST_CASE(shellEscape_escapes_embedded_single_quotes,
    QString const input = QString::fromUtf8("don't");
    QString const out   = IPView::SafeText::shellEscape(input);
    // POSIX-safe escape pattern is:  ' → '\''
    IPVIEW_CHECK_EQ(out, QString::fromUtf8("'don'\\''t'"))
)
