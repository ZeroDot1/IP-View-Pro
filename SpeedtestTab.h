// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — SpeedtestTab.h
//  C++26: [[nodiscard]], default member init, noexcept
//  Professional speedtest via speedtest-cli (Python sivel) with --json.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef SPEEDTESTTAB_H
#define SPEEDTESTTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QProcess>
#include <QTextEdit>
#include <QTimer>
#include <QCheckBox>
#include <QComboBox>
#include <QJsonObject>
#include <QElapsedTimer>
#include <QFrame>
#include <QGroupBox>

#include <vector>

struct ServerInfo {
    int     id{0};
    QString sponsor;
    QString location;
    double  distanceKm{0.0};
};

class SpeedtestTab : public QWidget
{
    Q_OBJECT

public:
    explicit SpeedtestTab(QWidget *parent = nullptr);
    ~SpeedtestTab() override;

private slots:
    void onStartClicked();
    void onStopClicked();
    void onSpeedtestFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onReadyRead();
    void onBrowseServers();
    void onProgressTick();

private:
    // ── UI-Aufbau ─────────────────────────────────────────────────────────
    void setupUI();

    [[nodiscard]] QFrame *createMetricCard(const QString &title,
                                            QLabel *&valueLabel,
                                            const QString &unit,
                                            const QString &color);

    [[nodiscard]] QString appStyleSheet() const noexcept;

    // ── Steuerung ─────────────────────────────────────────────────────────
    void setControlsEnabled(bool enabled) noexcept;
    void resetDisplay() noexcept;
    void updateDisplayFromJson(const QJsonObject &obj) noexcept;

    // ── Server-Verwaltung ─────────────────────────────────────────────────
    void parseServerList(const QString &raw);
    void setSelectedServer(int serverId);

    // ── Prozess-Management ────────────────────────────────────────────────
    [[nodiscard]] QString findSpeedtest() const noexcept;
    void startProcess(const QStringList &args);

    // ── UI-Elemente ───────────────────────────────────────────────────────
    QPushButton *startButton{nullptr};
    QPushButton *stopButton{nullptr};
    QPushButton *browseServersButton{nullptr};
    QComboBox   *serverCombo{nullptr};

    QLabel *pingLabel{nullptr};
    QLabel *downloadLabel{nullptr};
    QLabel *uploadLabel{nullptr};
    QLabel *serverLabel{nullptr};
    QLabel *statusLabel{nullptr};
    QLabel *ispLabel{nullptr};
    QLabel *shareLinkLabel{nullptr};

    QCheckBox *singleConnCheck{nullptr};
    QCheckBox *secureCheck{nullptr};
    QCheckBox *shareCheck{nullptr};

    QProgressBar *progressGauge{nullptr};
    QTextEdit    *logArea{nullptr};

    // ── Prozess & Zustand ─────────────────────────────────────────────────
    QProcess     *process{nullptr};
    QTimer       *progressTimer{nullptr};
    QElapsedTimer elapsedTimer;

    int       progressDot{0};
    QByteArray jsonBuffer;
    bool      isJsonMode{false};
    bool      isServerListMode{false};

    // ── Server-Cache ──────────────────────────────────────────────────────
    std::vector<ServerInfo> serverCache;
};

#endif // SPEEDTESTTAB_H