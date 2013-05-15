/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef SETTINGSHELPER_H
#define SETTINGSHELPER_H

#include <QObject>
#include <QSettings>

class SettingsHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool showOfflineContacts READ showOfflineContacts
                                         WRITE setShowOfflineContacts
                                         NOTIFY showOfflineContactsChanged)
    Q_PROPERTY(bool enableAudioAlerts READ enableAudioAlerts
                                WRITE setEnableAudioAlerts
                                NOTIFY enableAudioAlertsChanged)
    Q_PROPERTY(bool enableNotifications READ enableNotifications
                                         WRITE setEnableNotifications
                                         NOTIFY enableNotificationsChanged)
    Q_PROPERTY(bool enableVibrate READ enableVibrate
                                   WRITE setEnableVibrate
                                   NOTIFY enableVibrateChanged)

public:
    explicit SettingsHelper(QObject *parent = 0);

    static SettingsHelper *self();

    bool showOfflineContacts() const;
    bool enableAudioAlerts() const;
    bool enableNotifications() const;
    bool enableVibrate() const;

    Q_INVOKABLE QVariant value(const QString &prop, const QVariant &defaultValue) const;
    Q_INVOKABLE void setValue(const QString &prop, const QVariant &value);

public Q_SLOTS:
    void setShowOfflineContacts(bool show);
    void setEnableAudioAlerts(bool enable);
    void setEnableNotifications(bool enable);
    void setEnableVibrate(bool enable);

Q_SIGNALS:
    void showOfflineContactsChanged();
    void enableAudioAlertsChanged();
    void enableNotificationsChanged();
    void enableVibrateChanged();
    void valueUpdated(const QString &prop);

private:
    QSettings mSettings;
    static SettingsHelper *mSelf;
};

#endif // SETTINGSHELPER_H
