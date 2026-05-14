// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — Iperf3Window.h
//  C++26: default member init, [[nodiscard]], const-correctness
//  iPerf3 network test dialog with client/server mode and real-time metrics.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef IPERF3WINDOW_H
#define IPERF3WINDOW_H

#include <QDialog>
#include <QProcess>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QProgressBar>
#include <QTimer>
#include <QFrame>

class Iperf3Window : public QDialog
{
    Q_OBJECT

public:
    explicit Iperf3Window(QWidget *parent = nullptr);
    ~Iperf3Window() override;

private slots:
    void onStartClicked();
    void onStopClicked();
    void onIperf3ReadyRead();
    void onIperf3Finished(int exitCode, QProcess::ExitStatus status);
    void updateStats();
    void handleSpeedUpdated(double speed);

signals:
    void speedUpdated(double speed);

private:
    // ── UI Setup ────────────────────────────────────────────────────────────
    void setupUI();

    // ── Data Processing ─────────────────────────────────────────────────────
    void parseLine(const QString &line) noexcept;
    void updateVisualization(double speed) noexcept;

    // ── Process & Timer ─────────────────────────────────────────────────────
    QProcess *iperf3Process{nullptr};
    QTimer   *statsTimer{nullptr};

    // ── UI Elements ─────────────────────────────────────────────────────────
    QLabel      *ipLabel{nullptr};
    QLineEdit   *hostEdit{nullptr};
    QComboBox   *roleCombo{nullptr};
    QPushButton *startButton{nullptr};
    QPushButton *stopButton{nullptr};
    QTextEdit   *outputEdit{nullptr};
    QProgressBar *speedBar{nullptr};
    QLabel      *speedLabel{nullptr};
    QLabel      *transferLabel{nullptr};
    QLabel      *timeLabel{nullptr};

    // ── State ───────────────────────────────────────────────────────────────
    bool    isRunning{false};
    double  totalTransfer{0.0};
    qint64  startTime{0};
};

#endif // IPERF3WINDOW_H
