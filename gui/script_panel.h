// SPDX-License-Identifier: MIT
// Copyright (C) 2026 Fırat Türkerirci and contributors
//
// script_panel.h - a dockable script editor built for the "write the script
// elsewhere (often with an AI assistant), paste it, run it" workflow.
//
//   Paste / Open  -> the editor
//   Run           -> saves the text into the work folder and runs it with
//                    exec_python_file(), the same path File > Run Script uses
//   on an error   -> the failing line is highlighted, and Copy Error puts a
//                    ready-to-paste report (message, line, nearby code, the
//                    output so far) on the clipboard
//   AI Prompt     -> copies a description of FEMM's scripting environment to
//                    paste into an assistant before describing the model
#pragma once

#include <QDockWidget>
#include <QPlainTextEdit>
#include <QString>

class QLabel;
class QToolButton;
class PyConsole;

// Plain-text editor with a line-number gutter and error-line highlighting.
class ScriptCodeEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit ScriptCodeEdit(QWidget *parent = nullptr);

    int gutterWidth() const;
    bool isDark() const;   // dark theme: pick readable colours
    void paintGutter(QPaintEvent *e);
    // 1-based; 0 clears the highlight.
    void setErrorLine(int line);

    // Editing helpers for a touch screen, where the on-screen keyboard has no
    // Tab key: indent or outdent the selected lines (or the current one) by
    // four spaces; make the text larger or smaller.
    void indentLines(bool in);
    void zoomText(int steps);

protected:
    void resizeEvent(QResizeEvent *e) override;
    // Enter keeps the indentation of the line above (one level more after a
    // line ending in ':'); Tab and Shift+Tab indent and outdent.
    void keyPressEvent(QKeyEvent *e) override;

private:
    void updateGutterWidth();
    void updateGutter(const QRect &rect, int dy);
    void updateSelections();

    QWidget *m_gutter = nullptr;
    int m_errorLine = 0;
};

class ScriptPanel : public QDockWidget
{
    Q_OBJECT
public:
    // console: its output signal is how the panel sees what a run prints.
    explicit ScriptPanel(PyConsole *console, QWidget *parent = nullptr);

    QString scriptText() const;
    void setScriptText(const QString &text, const QString &name = QString());
    // Folder the script is saved into and run from.
    QString workDir() const;

    // Text of the last error report (empty after a successful run).
    QString lastErrorReport() const { return m_errorReport; }
    QString statusText() const;
    QString outputText() const { return m_runOutput; }
    // Keep the last run's output and status across an app restart.
    void saveState() const;
    void restoreState();

public slots:
    // Text from the clipboard or shared by another app: strips a ```python
    // fence and replaces the script.
    void acceptScriptText(const QString &text, const QString &how);
    void runScript();
    void stopScript();
    void pasteScript();
    void openScript();
    void saveScript();
    void copyError();
    void copyAiPrompt();

signals:
    // After a run: ok = completed without an error.
    void scriptFinished(bool ok);

private:
    void onConsoleOutput(const QString &text);
    void setRunning(bool running);
    void showStatus(const QString &text, bool error);
    QString buildErrorReport(const QString &msg, int line) const;
    QToolButton *makeButton(const QString &text, const QString &tip,
                            const char *iconName = nullptr);
    void saveDraft() const;

    ScriptCodeEdit *m_edit = nullptr;
    QPlainTextEdit *m_output = nullptr;
    QLabel *m_status = nullptr;
    QToolButton *m_runBtn = nullptr;
    QToolButton *m_stopBtn = nullptr;
    QToolButton *m_copyErrBtn = nullptr;

    QString m_name = "script.py";   // file name inside workDir()
    QString m_dir;                  // set when a script was opened from a folder
    bool m_running = false;
    QString m_runOutput;            // everything printed by the current/last run
    QString m_errorReport;
};
