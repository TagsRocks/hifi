//
//  Breakpad.cpp
//  interface/src
//
//  Created by Gabriel Calero & Cristian Duarte on 06/06/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Crashpad.h"

#if defined(HAS_BREAKPAD)
#include <QDebug>

#include <common/linux/google_crashdump_uploader.h>
#include <client/linux/handler/exception_handler.h>
#include <client/linux/handler/minidump_descriptor.h>
#include <QtCore/QFileInfo>
#include <QtAndroidExtras/QAndroidJniObject>
#include <QMap>
#include <SettingHelpers.h>
#include <mutex>

google_breakpad::ExceptionHandler *gBreakpadHandler;

std::mutex annotationMutex;
QMap<QString, QString> annotations;

static bool breakpad_dumpCallback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded) {
    return succeeded;
}

QString obbDir() {
    QAndroidJniObject mediaDir = QAndroidJniObject::callStaticObjectMethod("android/os/Environment", "getExternalStorageDirectory", "()Ljava/io/File;");
    QAndroidJniObject mediaPath = mediaDir.callObjectMethod( "getAbsolutePath", "()Ljava/lang/String;" );
    QAndroidJniObject activity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");
    QAndroidJniObject package = activity.callObjectMethod("getPackageName", "()Ljava/lang/String;");
    QString dataAbsPath = mediaPath.toString()+"/Android/obb/" + package.toString();
    return dataAbsPath;
}

bool startCrashHandler() {

    gBreakpadHandler = new google_breakpad::ExceptionHandler(
            google_breakpad::MinidumpDescriptor(obbDir().toStdString()),
            nullptr, breakpad_dumpCallback, nullptr, true, -1);
    return true;
}

void setCrashAnnotation(std::string name, std::string value) {
    std::lock_guard<std::mutex> guard(annotationMutex);
    QString qName = QString::fromStdString(name);
    QString qValue = QString::fromStdString(value);
    if(!annotations.contains(qName)) {
        annotations.insert(qName, qValue);
    } else {
        annotations[qName] = qValue;
    }

    QSettings settings(obbDir() + "/annotations.json", JSON_FORMAT);
    settings.clear();
    settings.beginGroup("Annotations");
    for(auto k : annotations.keys()) {
        settings.setValue(k, annotations.value(k));
    }
    settings.endGroup();
    settings.sync();
}

#endif
