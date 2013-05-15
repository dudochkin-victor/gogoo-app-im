/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef TELEPATHYTYPES_H
#define TELEPATHYTYPES_H

#include <QObject>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4Yell/Constants>

class TelepathyTypes : public QObject
{
    Q_OBJECT
    Q_ENUMS(ConnectionPresenceType)
    Q_ENUMS(ConnectionStatus)
    Q_ENUMS(ConnectionStatusReason)
    Q_ENUMS(PresenceState)
    Q_ENUMS(FileTransferState)
    Q_ENUMS(FileTransferStateChangeReason)
public:

    enum ConnectionPresenceType {
        ConnectionPresenceTypeUnset = Tp::ConnectionPresenceTypeUnset,
        ConnectionPresenceTypeOffline = Tp::ConnectionPresenceTypeOffline,
        ConnectionPresenceTypeAvailable = Tp::ConnectionPresenceTypeAvailable,
        ConnectionPresenceTypeAway = Tp::ConnectionPresenceTypeAway,
        ConnectionPresenceTypeExtendedAway = Tp::ConnectionPresenceTypeExtendedAway,
        ConnectionPresenceTypeHidden = Tp::ConnectionPresenceTypeHidden,
        ConnectionPresenceTypeBusy = Tp::ConnectionPresenceTypeBusy,
        ConnectionPresenceTypeUnknown = Tp::ConnectionPresenceTypeUnknown,
        ConnectionPresenceTypeError = Tp::ConnectionPresenceTypeError
    };

    enum ConnectionStatus {
        ConnectionStatusConnected = Tp::ConnectionStatusConnected,
        ConnectionStatusConnecting = Tp::ConnectionStatusConnecting,
        ConnectionStatusDisconnected = Tp::ConnectionStatusDisconnected
    };

    enum ConnectionStatusReason {
        ConnectionStatusReasonNoneSpecified = Tp::ConnectionStatusReasonNoneSpecified,
        ConnectionStatusReasonRequested = Tp::ConnectionStatusReasonRequested,
        ConnectionStatusReasonNetworkError = Tp::ConnectionStatusReasonNetworkError,
        ConnectionStatusReasonAuthenticationFailed = Tp::ConnectionStatusReasonAuthenticationFailed,
        ConnectionStatusReasonEncryptionError = Tp::ConnectionStatusReasonEncryptionError,
        ConnectionStatusReasonNameInUse = Tp::ConnectionStatusReasonNameInUse,
        ConnectionStatusReasonCertNotProvided = Tp::ConnectionStatusReasonCertNotProvided,
        ConnectionStatusReasonCertUntrusted = Tp::ConnectionStatusReasonCertUntrusted,
        ConnectionStatusReasonCertExpired = Tp::ConnectionStatusReasonCertExpired,
        ConnectionStatusReasonCertNotActivated = Tp::ConnectionStatusReasonCertNotActivated,
        ConnectionStatusReasonCertHostnameMismatch = Tp::ConnectionStatusReasonCertHostnameMismatch,
        ConnectionStatusReasonCertFingerprintMismatch = Tp::ConnectionStatusReasonCertFingerprintMismatch,
        ConnectionStatusReasonCertSelfSigned = Tp::ConnectionStatusReasonCertSelfSigned,
        ConnectionStatusReasonCertOtherError = Tp::ConnectionStatusReasonCertOtherError,
        ConnectionStatusReasonCertRevoked = Tp::ConnectionStatusReasonCertRevoked,
        ConnectionStatusReasonCertInsecure = Tp::ConnectionStatusReasonCertInsecure,
        ConnectionStatusReasonCertLimitExceeded = Tp::ConnectionStatusReasonCertLimitExceeded
    };

    enum PresenceState {
        PresenceStateNo = Tp::Contact::PresenceStateNo,
        PresenceStateAsk = Tp::Contact::PresenceStateAsk,
        PresenceStateYes = Tp::Contact::PresenceStateYes
    };

    enum FileTransferState {
        FileTransferStateNone = Tp::FileTransferStateNone,
        FileTransferStatePending = Tp::FileTransferStatePending,
        FileTransferStateAccepted = Tp::FileTransferStateAccepted,
        FileTransferStateOpen = Tp::FileTransferStateOpen,
        FileTransferStateCompleted = Tp::FileTransferStateCompleted,
        FileTransferStateCancelled = Tp::FileTransferStateCancelled
    };

    enum FileTransferStateChangeReason {
        FileTransferStateChangeReasonNone = Tp::FileTransferStateChangeReasonNone,
        FileTransferStateChangeReasonRequested = Tp::FileTransferStateChangeReasonRequested,
        FileTransferStateChangeReasonLocalStopped = Tp::FileTransferStateChangeReasonLocalStopped,
        FileTransferStateChangeReasonRemoteStopped = Tp::FileTransferStateChangeReasonRemoteStopped,
        FileTransferStateChangeReasonLocalError = Tp::FileTransferStateChangeReasonLocalError,
        FileTransferStateChangeReasonRemoteError = Tp::FileTransferStateChangeReasonRemoteError
    };

    explicit TelepathyTypes(QObject *parent = 0);
};

#endif // TELEPATHYTYPES_H
