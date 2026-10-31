// SPDX-License-Identifier: MIT
// Copyright (C) 2026 David Meeker and contributors
//
// script_panel.cpp - see script_panel.h.

#include "script_panel.h"
#include "py_console.h"
#include "py_state.h"
#include "build_info.h"
#include "tablet_mode.h"
#ifdef __ANDROID__
#include "android_support.h"
#endif

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>
#include <QSyntaxHighlighter>
#include <QTextBlock>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#ifdef __ANDROID__
static const int kButtonHeight = 48;
static const int kEditorPt     = 14;
#else
static const int kButtonHeight = 28;
static const int kEditorPt     = 11;
#endif

// ---------------------------------------------------------------------------
// Python highlighting: keywords, FEMM commands, strings, comments, numbers.
// ---------------------------------------------------------------------------
namespace {

class PyHighlighter : public QSyntaxHighlighter
{
public:
    PyHighlighter(QTextDocument *doc, bool dark) : QSyntaxHighlighter(doc)
    {
        // Two palettes: the light one for the desktop default, the dark one
        // for a dark system theme (common on Android tablets).
        QTextCharFormat kw;
        kw.setForeground(dark ? QColor(0x56, 0x9c, 0xd6) : QColor(0x00, 0x33, 0x99));
        kw.setFontWeight(QFont::Bold);
        const char *words[] = {
            "and", "as", "assert", "break", "class", "continue", "def", "del",
            "elif", "else", "except", "finally", "for", "from", "global", "if",
            "import", "in", "is", "lambda", "nonlocal", "not", "or", "pass",
            "raise", "return", "try", "while", "with", "yield", "None", "True",
            "False"};
        for (const char *w : words)
            m_rules.push_back({QRegularExpression(QString("\\b%1\\b").arg(w)), kw});

        QTextCharFormat femm;
        femm.setForeground(dark ? QColor(0x4e, 0xc9, 0xb0) : QColor(0x00, 0x77, 0x66));
        m_rules.push_back({QRegularExpression(
            "\\b(?:femm\\.)?(?:mi|mo|ei|eo|hi|ho|ci|co)_\\w+"), femm});

        QTextCharFormat num;
        num.setForeground(dark ? QColor(0xb5, 0xce, 0xa8) : QColor(0x99, 0x33, 0x00));
        m_rules.push_back({QRegularExpression(
            "\\b[0-9]+(?:\\.[0-9]*)?(?:[eE][-+]?[0-9]+)?\\b"), num});

        m_string.setForeground(dark ? QColor(0xce, 0x91, 0x78) : QColor(0x88, 0x00, 0x88));
        m_comment.setForeground(dark ? QColor(0x6a, 0x99, 0x55) : QColor(0x70, 0x70, 0x70));
        m_comment.setFontItalic(true);
    }

protected:
    void highlightBlock(const QString &text) override
    {
        for (const auto &r : m_rules) {
            auto it = r.re.globalMatch(text);
            while (it.hasNext()) {
                auto m = it.next();
                setFormat(m.capturedStart(), m.capturedLength(), r.fmt);
            }
        }
        // Strings and comments last so they win; a '#' inside a string is
        // not a comment.
        int i = 0;
        const int n = text.size();
        while (i < n) {
            const QChar c = text[i];
            if (c == '#') { setFormat(i, n - i, m_comment); break; }
            if (c == '"' || c == '\'') {
                int j = i + 1;
                while (j < n && text[j] != c) { if (text[j] == '\\') ++j; ++j; }
                setFormat(i, qMin(j + 1, n) - i, m_string);
                i = j + 1;
                continue;
            }
            ++i;
        }
    }

private:
    struct Rule { QRegularExpression re; QTextCharFormat fmt; };
    std::vector<Rule> m_rules;
    QTextCharFormat m_string, m_comment;
};

class Gutter : public QWidget
{
public:
    explicit Gutter(ScriptCodeEdit *e) : QWidget(e), m_edit(e) {}
    QSize sizeHint() const override { return QSize(m_edit->gutterWidth(), 0); }
protected:
    void paintEvent(QPaintEvent *e) override { m_edit->paintGutter(e); }
private:
    ScriptCodeEdit *m_edit;
};

} // namespace

// ---------------------------------------------------------------------------
// ScriptCodeEdit
// ---------------------------------------------------------------------------
ScriptCodeEdit::ScriptCodeEdit(QWidget *parent) : QPlainTextEdit(parent)
{
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    f.setPointSize(kEditorPt);
    setFont(f);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setTabStopDistance(4 * fontMetrics().horizontalAdvance(' '));
    new PyHighlighter(document(), isDark());

    m_gutter = new Gutter(this);
    connect(this, &QPlainTextEdit::blockCountChanged, this, [this]() { updateGutterWidth(); });
    connect(this, &QPlainTextEdit::updateRequest, this,
            [this](const QRect &r, int dy) { updateGutter(r, dy); });
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, [this]() { updateSelections(); });
    updateGutterWidth();
}

bool ScriptCodeEdit::isDark() const
{
    return palette().color(QPalette::Base).lightness() < 128;
}

int ScriptCodeEdit::gutterWidth() const
{
    int digits = 1;
    for (int n = qMax(1, blockCount()); n >= 10; n /= 10) ++digits;
    return 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * qMax(3, digits);
}

void ScriptCodeEdit::updateGutterWidth()
{
    setViewportMargins(gutterWidth(), 0, 0, 0);
}

void ScriptCodeEdit::updateGutter(const QRect &rect, int dy)
{
    if (dy) m_gutter->scroll(0, dy);
    else    m_gutter->update(0, rect.y(), m_gutter->width(), rect.height());
    if (rect.contains(viewport()->rect())) updateGutterWidth();
}

void ScriptCodeEdit::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    const QRect cr = contentsRect();
    m_gutter->setGeometry(QRect(cr.left(), cr.top(), gutterWidth(), cr.height()));
}

void ScriptCodeEdit::paintGutter(QPaintEvent *e)
{
    QPainter p(m_gutter);
    const bool dark = isDark();
    p.fillRect(e->rect(), dark ? QColor(0x2b, 0x2b, 0x2b) : QColor(0xf0, 0xf0, 0xf0));
    QTextBlock block = firstVisibleBlock();
    int num = block.blockNumber() + 1;
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    while (block.isValid() && top <= e->rect().bottom()) {
        if (block.isVisible() && bottom >= e->rect().top()) {
            p.setPen(num == m_errorLine ? (dark ? QColor(0xff, 0x6b, 0x6b) : QColor(0xcc, 0x00, 0x00))
                                        : QColor(0x90, 0x90, 0x90));
            p.drawText(0, top, m_gutter->width() - 5, fontMetrics().height(),
                       Qt::AlignRight, QString::number(num));
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++num;
    }
}

void ScriptCodeEdit::setErrorLine(int line)
{
    m_errorLine = line;
    if (line > 0) {
        QTextBlock b = document()->findBlockByNumber(line - 1);
        if (b.isValid()) {
            QTextCursor c(b);
            setTextCursor(c);
            centerCursor();
        }
    }
    updateSelections();
    m_gutter->update();
}

void ScriptCodeEdit::updateSelections()
{
    QList<QTextEdit::ExtraSelection> sel;
    if (m_errorLine > 0) {
        QTextBlock b = document()->findBlockByNumber(m_errorLine - 1);
        if (b.isValid()) {
            QTextEdit::ExtraSelection s;
            s.format.setBackground(isDark() ? QColor(0x5a, 0x1d, 0x1d) : QColor(0xff, 0xd8, 0xd8));
            s.format.setProperty(QTextFormat::FullWidthSelection, true);
            s.cursor = QTextCursor(b);
            sel.append(s);
        }
    }
    setExtraSelections(sel);
}

// ---------------------------------------------------------------------------
// ScriptPanel
// ---------------------------------------------------------------------------
ScriptPanel::ScriptPanel(PyConsole *console, QWidget *parent)
    : QDockWidget(tr("Script"), parent)
{
    setObjectName("ScriptPanel");
    auto *root = new QWidget(this);
    auto *v = new QVBoxLayout(root);
    v->setContentsMargins(4, 4, 4, 4);
    v->setSpacing(4);

    auto *buttons = new QGridLayout;
    buttons->setSpacing(4);
    m_runBtn = makeButton(tr("Run"), tr("Save the script into the work folder and run it"), "run");
    m_stopBtn = makeButton(tr("Stop"), tr("Interrupt the running script"), "stop");
    auto *pasteBtn = makeButton(tr("Paste"), tr("Replace the script with the clipboard text"), "paste");
    auto *openBtn = makeButton(tr("Open"), tr("Open a .py file"), "open_file");
    auto *saveBtn = makeButton(tr("Save"), tr("Save the script to a file"), "save");
    m_copyErrBtn = makeButton(tr("Copy Error"),
        tr("Copy the last error, with the nearby lines, to paste into an AI assistant"), "copyerror");
    auto *promptBtn = makeButton(tr("AI Prompt"),
        tr("Copy a description of FEMM's scripting environment to paste into an AI assistant"), "aiprompt");
    QFont bf = m_runBtn->font();
    bf.setBold(true);
    m_runBtn->setFont(bf);
    buttons->addWidget(m_runBtn, 0, 0);
    buttons->addWidget(m_stopBtn, 0, 1);
    buttons->addWidget(pasteBtn, 0, 2);
    buttons->addWidget(openBtn, 0, 3);
    buttons->addWidget(m_copyErrBtn, 1, 0, 1, 2);
    buttons->addWidget(promptBtn, 1, 2);
    buttons->addWidget(saveBtn, 1, 3);
    v->addLayout(buttons);

    m_edit = new ScriptCodeEdit(root);
    m_edit->setPlaceholderText(tr(
        "Paste a FEMM Python script here and press Run.\n\n"
        "Tip: press \"AI Prompt\", paste it into your AI assistant, then describe "
        "the model you want."));

    m_status = new QLabel(root);
    m_status->setWordWrap(true);
    m_status->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_output = new QPlainTextEdit(root);
    m_output->setReadOnly(true);
    m_output->setMaximumBlockCount(5000);
    QFont of = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    of.setPointSize(kEditorPt - 1);
    m_output->setFont(of);
    m_output->setPlaceholderText(tr("Script output appears here."));

    auto *split = new QSplitter(Qt::Vertical, root);
    split->addWidget(m_edit);
    auto *bottom = new QWidget(split);
    auto *bv = new QVBoxLayout(bottom);
    bv->setContentsMargins(0, 0, 0, 0);
    bv->addWidget(m_status);
    bv->addWidget(m_output);
    split->addWidget(bottom);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 1);
    v->addWidget(split, 1);
    setWidget(root);

    connect(m_runBtn, &QToolButton::clicked, this, &ScriptPanel::runScript);
    connect(m_stopBtn, &QToolButton::clicked, this, &ScriptPanel::stopScript);
    connect(pasteBtn, &QToolButton::clicked, this, &ScriptPanel::pasteScript);
    connect(openBtn, &QToolButton::clicked, this, &ScriptPanel::openScript);
    connect(saveBtn, &QToolButton::clicked, this, &ScriptPanel::saveScript);
    connect(m_copyErrBtn, &QToolButton::clicked, this, &ScriptPanel::copyError);
    connect(promptBtn, &QToolButton::clicked, this, &ScriptPanel::copyAiPrompt);
    if (console)
        connect(console, &PyConsole::outputWritten, this, &ScriptPanel::onConsoleOutput);

    // Keep a draft, so an accidental close (or Android killing the app in the
    // background) does not lose a pasted script.
    auto *draftTimer = new QTimer(this);
    draftTimer->setSingleShot(true);
    draftTimer->setInterval(1000);
    connect(draftTimer, &QTimer::timeout, this, [this]() { saveDraft(); });
    connect(m_edit, &QPlainTextEdit::textChanged, draftTimer, qOverload<>(&QTimer::start));
    connect(m_edit, &QPlainTextEdit::textChanged, this, [this]() {
        if (!m_running) m_edit->setErrorLine(0);
    });

    QSettings s;
    const QString draft = s.value("scriptPanel/draft").toString();
    m_name = s.value("scriptPanel/name", "script.py").toString();
    if (!draft.isEmpty()) {
        m_edit->setPlainText(draft);
    } else {
#ifdef __ANDROID__
        QFile demo(workDir() + "/ai_demo_solenoid.py");
        if (demo.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_edit->setPlainText(QString::fromUtf8(demo.readAll()));
            m_name = "ai_demo_solenoid.py";
        }
#endif
    }

    setRunning(false);
    m_copyErrBtn->setEnabled(false);
    showStatus(tr("Work folder: %1").arg(QDir::toNativeSeparators(workDir())), false);
}

QToolButton *ScriptPanel::makeButton(const QString &text, const QString &tip,
                                     const char *iconName)
{
    auto *b = new QToolButton(this);
    b->setText(text);
    b->setToolTip(tip);
    b->setToolButtonStyle(Qt::ToolButtonTextOnly);
    if (femmTabletMode() && iconName) {
        const QIcon ic = femmTabletIcon(QString::fromLatin1(iconName));
        if (!ic.isNull()) {
            b->setIcon(ic);
            b->setIconSize(QSize(22, 22));
            b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        }
    }
    b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    b->setMinimumHeight(kButtonHeight);
    return b;
}

QString ScriptPanel::workDir() const
{
    if (!m_dir.isEmpty()) return m_dir;
#ifdef __ANDROID__
    const QString d = femmAndroidWorkDir();
    if (!d.isEmpty()) return d;
#endif
    const QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    return (docs.isEmpty() ? QDir::homePath() : docs) + "/FEMM Scripts";
}

QString ScriptPanel::statusText() const
{
    return m_status->text();
}

QString ScriptPanel::scriptText() const
{
    return m_edit->toPlainText();
}

void ScriptPanel::setScriptText(const QString &text, const QString &name)
{
    m_edit->setPlainText(text);
    m_edit->setErrorLine(0);
    if (!name.isEmpty()) m_name = name;
    saveDraft();
}

void ScriptPanel::saveDraft() const
{
    QSettings s;
    s.setValue("scriptPanel/draft", m_edit->toPlainText());
    s.setValue("scriptPanel/name", m_name);
}

void ScriptPanel::saveState() const
{
    saveDraft();
    QSettings s;
    s.setValue("scriptPanel/output", m_runOutput);
    s.setValue("scriptPanel/status", m_status->text());
    s.setValue("scriptPanel/errorReport", m_errorReport);
}

void ScriptPanel::restoreState()
{
    QSettings s;
    m_runOutput = s.value("scriptPanel/output").toString();
    m_output->setPlainText(m_runOutput);
    m_errorReport = s.value("scriptPanel/errorReport").toString();
    m_copyErrBtn->setEnabled(!m_errorReport.isEmpty());
    const QString status = s.value("scriptPanel/status").toString();
    if (!status.isEmpty()) showStatus(status, !m_errorReport.isEmpty());
}

void ScriptPanel::setRunning(bool running)
{
    m_running = running;
    m_runBtn->setEnabled(!running);
    m_stopBtn->setEnabled(running);
}

void ScriptPanel::showStatus(const QString &text, bool error)
{
    m_status->setText(text);
    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    if (error)
        m_status->setStyleSheet(dark ? "color: #ff6b6b; font-weight: bold;"
                                     : "color: #b00000; font-weight: bold;");
    else
        m_status->setStyleSheet(dark ? "color: #8fd88f;" : "color: #305030;");
}

void ScriptPanel::onConsoleOutput(const QString &text)
{
    if (!m_running) return;
    m_runOutput += text;
    QTextCursor c(m_output->document());
    c.movePosition(QTextCursor::End);
    c.insertText(text);
    m_output->verticalScrollBar()->setValue(m_output->verticalScrollBar()->maximum());
}

void ScriptPanel::runScript()
{
    if (m_running || py_is_running()) {
        showStatus(tr("A script is already running."), true);
        return;
    }
    const QString src = m_edit->toPlainText();
    if (src.trimmed().isEmpty()) {
        showStatus(tr("The editor is empty: paste or open a script first."), true);
        return;
    }

    const QString dir = workDir();
    QDir().mkpath(dir);
    if (!m_name.endsWith(".py", Qt::CaseInsensitive)) m_name += ".py";
    const QString path = dir + "/" + m_name;
    {
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            showStatus(tr("Cannot write %1").arg(QDir::toNativeSeparators(path)), true);
            return;
        }
        f.write(src.toUtf8());
    }
    saveDraft();

    m_edit->setErrorLine(0);
    m_output->clear();
    m_runOutput.clear();
    m_errorReport.clear();
    m_copyErrBtn->setEnabled(false);
    showStatus(tr("Running %1 ...").arg(m_name), false);
    setRunning(true);

    // Same as File > Run Script: the script's folder becomes FEMM's current
    // directory so relative file names resolve there.
    QString d = QDir::fromNativeSeparators(dir);
    d.replace("\\", "\\\\").replace("\"", "\\\"");
    char errmsg[4096] = {};
    exec_python(("setcurrentdirectory(\"" + d + "\")").toUtf8().constData(),
                errmsg, sizeof(errmsg));
    errmsg[0] = '\0';

    const QByteArray p8 = QDir::fromNativeSeparators(path).toUtf8();
    const int rc = exec_python_file(p8.constData(), errmsg, sizeof(errmsg));
    const int line = rc ? py_last_error_line() : 0;
    setRunning(false);

    if (rc == 0) {
        showStatus(tr("Completed."), false);
        emit scriptFinished(true);
        return;
    }
    const QString msg = QString::fromUtf8(errmsg);
    if (msg.contains("KeyboardInterrupt")) {
        showStatus(tr("Stopped."), false);
        emit scriptFinished(false);
        return;
    }
    m_errorReport = buildErrorReport(msg, line);
    m_copyErrBtn->setEnabled(true);
    m_edit->setErrorLine(line);
    showStatus(line > 0 ? tr("Error on line %1: %2").arg(line).arg(msg)
                        : tr("Error: %1").arg(msg), true);
    emit scriptFinished(false);
}

void ScriptPanel::stopScript()
{
    if (py_is_running()) py_request_interrupt();
}

QString ScriptPanel::buildErrorReport(const QString &msg, int line) const
{
    QString r;
    r += QString("FEMM %1 reported an error while running my script (embedded MicroPython).\n")
             .arg(femm_version());
    r += line > 0 ? QString("Line %1: %2\n").arg(line).arg(msg) : QString("Error: %1\n").arg(msg);
    if (line > 0) {
        r += "\nCode around the error:\n";
        const QStringList lines = m_edit->toPlainText().split('\n');
        for (int i = qMax(1, line - 3); i <= qMin(int(lines.size()), line + 2); ++i)
            r += QString("%1 %2 | %3\n").arg(i == line ? ">" : " ")
                     .arg(i, 4).arg(lines[i - 1]);
    }
    QString out = m_runOutput.trimmed();
    if (!out.isEmpty()) {
        QStringList ol = out.split('\n');
        if (ol.size() > 20) ol = ol.mid(ol.size() - 20);
        r += "\nOutput before the error:\n" + ol.join('\n') + "\n";
    }
    r += "\nPlease fix this and send the complete corrected script.\n";
    return r;
}

void ScriptPanel::copyError()
{
    if (m_errorReport.isEmpty()) return;
    QApplication::clipboard()->setText(m_errorReport);
    showStatus(tr("Error report copied. Paste it into your AI assistant."), false);
}

void ScriptPanel::copyAiPrompt()
{
    QFile f(":/femm/ai_prompt.md");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QApplication::clipboard()->setText(QString::fromUtf8(f.readAll()));
    showStatus(tr("AI prompt copied. Paste it into your AI assistant, then describe "
                  "your model after it."), false);
}

void ScriptPanel::acceptScriptText(const QString &t, const QString &how)
{
    if (t.trimmed().isEmpty()) {
        showStatus(tr("No text to use as a script."), true);
        return;
    }
    // Assistants usually wrap code in a ```python fence; keep what is inside.
    QString code = t;
    static const QRegularExpression fence("```(?:python|py)?[^\\n]*\\n(.*?)```",
        QRegularExpression::DotMatchesEverythingOption);
    auto m = fence.match(t);
    if (m.hasMatch()) code = m.captured(1);
    m_dir.clear();
    setScriptText(code, "script.py");
    show();
    raise();
    showStatus(tr("%1 %2 lines. Press Run.").arg(how).arg(code.count('\n') + 1), false);
}

void ScriptPanel::pasteScript()
{
    const QString t = QApplication::clipboard()->text();
    if (t.trimmed().isEmpty()) {
        showStatus(tr("The clipboard has no text."), true);
        return;
    }
    acceptScriptText(t, tr("Pasted"));
}

void ScriptPanel::openScript()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Open Python Script"),
        workDir(), tr("Python Scripts (*.py);;All Files (*)"));
    if (path.isEmpty()) return;
    QFile f(path);   // also reads Android content:// URIs
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        showStatus(tr("Cannot open %1").arg(path), true);
        return;
    }
    QString name = QFileInfo(path).fileName();
    if (path.startsWith("content:", Qt::CaseInsensitive)) {
        // A content URI says nothing useful about a folder FEMM can write in;
        // run it from the work folder.
        name = QUrl::fromPercentEncoding(name.toUtf8()).section('/', -1).section(':', -1);
        m_dir.clear();
    } else {
        m_dir = QFileInfo(path).absolutePath();
    }
    if (!name.endsWith(".py", Qt::CaseInsensitive)) name = "script.py";
    setScriptText(QString::fromUtf8(f.readAll()), name);
    showStatus(tr("Opened %1. Press Run.").arg(name), false);
}

void ScriptPanel::saveScript()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Save Python Script"),
        workDir() + "/" + m_name, tr("Python Scripts (*.py)"));
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        showStatus(tr("Cannot write %1").arg(path), true);
        return;
    }
    f.write(m_edit->toPlainText().toUtf8());
    showStatus(tr("Saved."), false);
}
