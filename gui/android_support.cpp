// SPDX-License-Identifier: MIT
// Copyright (C) 2026 Fırat Türker Ircı and contributors
//
// android_support.cpp - Android start-up for femm_gui.
//
// On the desktop everything FEMM ships sits next to the executable, and
// exeDir() finds it. On Android the "executable" is app_process64, so:
//
//  * the read-only data (material libraries, python/ stdlib) is packed into
//    the APK's assets under femm/app/ and extracted into private storage, and
//    exeDir() is pointed there;
//  * the solvers ship as lib<name>.so in the APK's native library directory,
//    which is the only place an app may execute a file from, and libfemm is
//    told to look for them there;
//  * the example models and scripts go into a "work" folder, which becomes
//    the current directory so scripts' relative paths ("temp.fem") resolve.

#include "android_support.h"

#ifdef __ANDROID__

#include "script_callbacks.h"
#include "build_info.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJniEnvironment>
#include <QJniObject>
#include <QStandardPaths>
#include <QtCore/qnativeinterface.h>

static QString s_workDir;

// Copy every file under srcRoot (an "assets:/..." path) into dstRoot.
// Existing files are kept unless overwrite is set.
static void copyTree(const QString &srcRoot, const QString &dstRoot, bool overwrite)
{
    QDirIterator it(srcRoot, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString src = it.next();
        const QString rel = QDir(srcRoot).relativeFilePath(src);
        const QString dst = dstRoot + "/" + rel;
        if (QFile::exists(dst)) {
            if (!overwrite) continue;
            QFile::remove(dst);
        }
        QDir().mkpath(QFileInfo(dst).absolutePath());
        if (QFile::copy(src, dst))
            QFile::setPermissions(dst, QFile::ReadOwner | QFile::WriteOwner);
    }
}

static QString nativeLibraryDir()
{
    QJniObject ctx = QNativeInterface::QAndroidApplication::context();
    if (!ctx.isValid()) return QString();
    QJniObject info = ctx.callObjectMethod(
        "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");
    if (!info.isValid()) return QString();
    return info.getObjectField<jstring>("nativeLibraryDir").toString();
}

void femmAndroidSetup()
{
    const QString base =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString appDir  = base + "/app";
    s_workDir = base + "/work";
    QDir().mkpath(appDir);
    QDir().mkpath(s_workDir);

    // The shipped data is replaced whenever the build changes; the user's
    // work folder only gains example files that are missing.
    const QString stampPath = appDir + "/.build";
    const QByteArray stamp = QByteArray(femm_version()) + "-" + femm_build()
                           + " " __DATE__ " " __TIME__;
    QByteArray old;
    {
        QFile f(stampPath);
        if (f.open(QIODevice::ReadOnly)) old = f.readAll();
    }
    if (old != stamp) {
        copyTree("assets:/femm/app", appDir, true);
        QFile f(stampPath);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) f.write(stamp);
    }
    copyTree("assets:/femm/examples", s_workDir, false);

    set_exe_dir_override(appDir.toStdString());
    const QString libDir = nativeLibraryDir();
    if (!libDir.isEmpty())
        set_solver_dir_override(libDir.toStdString());

    QDir::setCurrent(s_workDir);
}

QString femmAndroidWorkDir()
{
    return s_workDir;
}

QString femmAndroidTakeSharedText()
{
    QJniObject t = QJniObject::callStaticObjectMethod(
        "io/github/firatturkerirci/femm/FemmActivity", "takeSharedText",
        "()Ljava/lang/String;");
    QJniEnvironment env;
    if (env.checkAndClearExceptions()) return QString();
    return t.isValid() ? t.toString() : QString();
}

void femmAndroidHideActionBar()
{
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]() {
        QJniObject activity = QNativeInterface::QAndroidApplication::context();
        if (!activity.isValid()) return;
        QJniObject bar = activity.callObjectMethod("getActionBar", "()Landroid/app/ActionBar;");
        QJniEnvironment env;
        if (env.checkAndClearExceptions() || !bar.isValid()) return;
        bar.callMethod<void>("hide");
        env.checkAndClearExceptions();
    });
}

QString femmAndroidExternalDir()
{
    // getExternalFilesDir(null) also creates the folder. adb can read it
    // without root, which is what the CI self-test relies on.
    QJniObject ctx = QNativeInterface::QAndroidApplication::context();
    if (!ctx.isValid()) return QString();
    QJniObject dir = ctx.callObjectMethod("getExternalFilesDir",
        "(Ljava/lang/String;)Ljava/io/File;", nullptr);
    if (!dir.isValid()) return QString();
    return dir.callObjectMethod("getAbsolutePath", "()Ljava/lang/String;").toString();
}

#endif // __ANDROID__
