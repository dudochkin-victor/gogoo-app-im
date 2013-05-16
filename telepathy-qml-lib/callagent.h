/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef CALLAGENT_H
#define CALLAGENT_H

#include <QAbstractListModel>
#include <TelepathyQt/Account>
#include <TelepathyQt/Contact>
#include <TelepathyQt/Types>
#include <TelepathyQt4Yell/Farstream/Types>
#include <TelepathyQt4Yell/CallChannel>
#include <QtGstQmlSink/qmlgstvideoitem.h>

class FarstreamChannel;

extern "C" {
struct _TfChannel;
typedef struct _TfChannel TfChannel;
};

class CallAgent : public QObject
{
    Q_OBJECT

    Q_PROPERTY(CallStatus callStatus READ callStatus NOTIFY callStatusChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(bool error READ error NOTIFY errorChanged);
    Q_PROPERTY(QDateTime startTime READ startTime NOTIFY startTimeChanged)
    Q_PROPERTY(bool audioSent READ audioSent WRITE setAudioSend NOTIFY audioSentChanged)
    Q_PROPERTY(bool videoSent READ videoSent WRITE setVideoSend NOTIFY videoSentChanged)
    Q_PROPERTY(bool videoSentOrAboutTo READ videoSentOrAboutTo)
    Q_PROPERTY(bool existingCall READ existingCall NOTIFY existingCallChanged)
    Q_PROPERTY(bool incomingAudioCall READ incomingAudioCall NOTIFY incomingAudioCallChanged)
    Q_PROPERTY(bool incomingVideoCall READ incomingVideoCall NOTIFY incomingVideoCallChanged)
    Q_PROPERTY(bool heldCall READ heldCall WRITE setHoldCall NOTIFY heldCallChanged)
    Q_PROPERTY(uint missedAudioCalls READ missedAudioCalls NOTIFY missedAudioCallsChanged)
    Q_PROPERTY(uint missedVideoCalls READ missedVideoCalls NOTIFY missedVideoCallsChanged)
    // indicates if we are showing remote video
    Q_PROPERTY(bool remoteVideoRender READ remoteVideoRender NOTIFY remoteVideoRenderChanged)

    Q_PROPERTY(bool mutedCall READ mutedCall WRITE setMuteCall NOTIFY mutedCallChanged)
    Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged)

    Q_PROPERTY(bool audioReceived READ audioReceived /*WRITE setAudioReceived*/ NOTIFY audioReceivedChanged)
    Q_PROPERTY(bool videoReceived READ videoReceived /*WRITE setVideoReceived*/ NOTIFY videoReceivedChanged)
    Q_PROPERTY(QObject *outgoingVideo READ outgoingVideo WRITE setOutgoingVideo)
    Q_PROPERTY(QObject *incomingVideo READ incomingVideo WRITE setIncomingVideo)

    Q_PROPERTY(bool isRequested READ isRequested)

    Q_ENUMS(CallStatus);

public:
    typedef enum {
        CallStatusNoCall,
        CallStatusIncomingCall,
        CallStatusConnecting,
        CallStatusRinging,
        CallStatusTalking,
        CallStatusHeld,
        CallStatusHangingUp
        //CallStatusRejectedCall
    } CallStatus;

    explicit CallAgent(const Tp::AccountPtr &account, const Tp::ContactPtr &contact, QObject *parent = 0);
    ~CallAgent();

    Tp::AccountPtr account();
    Tp::ContactPtr contact();

    Q_INVOKABLE CallStatus callStatus() const;
    Q_INVOKABLE QString errorString() const;
    Q_INVOKABLE bool error() const;
    Q_INVOKABLE void clearError();
    Q_INVOKABLE QDateTime startTime() const;
    Q_INVOKABLE QDateTime elapsedTime() const;

    Q_INVOKABLE void call(bool withVideo);
    Q_INVOKABLE void audioCall();
    Q_INVOKABLE void videoCall();
    Q_INVOKABLE void endCall();

    Q_INVOKABLE bool existingCall() const;
    Q_INVOKABLE void acceptCall();

    Q_INVOKABLE void setAudioSend(bool send);
    Q_INVOKABLE bool audioSent() const;
    Q_INVOKABLE void setVideoSend(bool send);
    Q_INVOKABLE bool videoSent() const;
    Q_INVOKABLE bool videoSentOrAboutTo() const;
    Q_INVOKABLE bool audioReceived() const;
    Q_INVOKABLE bool videoReceived() const;

    Q_INVOKABLE bool incomingVideoCall() const;
    Q_INVOKABLE bool incomingAudioCall() const;

    Q_INVOKABLE bool remoteVideoRender() const;

    Q_INVOKABLE void setHoldCall(bool hold);
    Q_INVOKABLE void holdCall();
    Q_INVOKABLE void unholdCall();
    Q_INVOKABLE bool heldCall();

    Q_INVOKABLE void setMuteCall(bool mute);
    Q_INVOKABLE void muteCall();
    Q_INVOKABLE void unmuteCall();
    Q_INVOKABLE bool mutedCall() const;

    Q_INVOKABLE double volume() const;
    Q_INVOKABLE void setVolume(double val);

    Q_INVOKABLE uint missedCalls() const;
    Q_INVOKABLE uint missedAudioCalls() const;
    Q_INVOKABLE uint missedVideoCalls() const;
    Q_INVOKABLE void resetMissedCalls();
    Q_INVOKABLE QDateTime lastMissedVideoCall() const;
    Q_INVOKABLE QDateTime lastMissedAudioCall() const;

    Q_INVOKABLE QObject *incomingVideo() const;
    Q_INVOKABLE void setIncomingVideo(QObject *item);
    Q_INVOKABLE QObject *outgoingVideo() const;
    Q_INVOKABLE void setOutgoingVideo(QObject *item);

    Q_INVOKABLE void swapCamera();
    Q_INVOKABLE bool cameraSwappable() const;

    Q_INVOKABLE bool canSwapVideos() const;

    Q_INVOKABLE bool isRequested() const;
    Q_INVOKABLE QTime updateCallDuration();

    Tpy::CallStateReason stateReason() const;

Q_SIGNALS:
    // property notification signals
    void callStatusChanged(CallAgent::CallStatus oldStatus, CallAgent::CallStatus newStatus);
    void errorStringChanged();
    void errorChanged();
    void startTimeChanged();
    void audioSentChanged();
    void videoSentChanged();
    void audioReceivedChanged();
    void videoReceivedChanged();
    void existingCallChanged();
    void incomingAudioCallChanged();
    void incomingVideoCallChanged();
    void heldCallChanged();
    void mutedCallChanged();
    void volumeChanged();
    void missedAudioCallsChanged();
    void missedVideoCallsChanged();
    void remoteVideoRenderChanged();
    void error(const QString &errorMessage);

public Q_SLOTS:
    // to be called for new channels from channel handler
    void onChannelAvailable(Tp::ChannelPtr channel);
    // to be called When orientation changes, we must change the videoflip rotation.
    // We support four orientations:
    // 0 - Right Up
    // 1 - Top Up
    // 2 - Left Up
    // 3 - Top Down
    void onOrientationChanged(uint orientation);

protected Q_SLOTS:
    void onFarstreamStateChanged();
    void onRemoteVideoRender(bool enabled);
    void onConnectionStatusChanged(Tp::ConnectionStatus status);
    void onConnectionInvalidated(Tp::DBusProxy *proxy);
    void onAccountConnectionChanged(const Tp::ConnectionPtr &conn);
    void onPendingChanelRequestFinished(Tp::PendingOperation *op);
    void onPendingChanelRequestCreated(const Tp::ChannelRequestPtr &channelRequest);
    void onCallChannelReady(Tp::PendingOperation *op);
    void onChannelInvalidated(Tp::DBusProxy *, const QString &errorName, const QString &errorMessage);
    void onContentAdded(const Tpy::CallContentPtr &content);
    void onContentRemoved(const Tpy::CallContentPtr &content);
    void onLocalHoldStateChanged(Tp::LocalHoldState state, Tp::LocalHoldStateReason reason);
    void onStateChanged(Tpy::CallState state);
    void onStreamAdded(const Tpy::CallStreamPtr &stream);
    void onStreamRemoved(const Tpy::CallStreamPtr &stream);
    void onLocalSendingStateChanged(Tpy::SendingState localSendingState);
    void onRemoteSendingStateChanged(const QHash< Tp::ContactPtr, Tpy::SendingState > &remoteSendingStates);
    void onRemoteMembersRemoved(const Tp::Contacts &remoteMembers);
    void onRequestContentFinished(Tp::PendingOperation *op);
    void onRemoveContentFinished(Tp::PendingOperation *op);
    void onRequestCloseFinished(Tp::PendingOperation *op);
    void onAcceptCallFinished(Tp::PendingOperation *op);
    void onRequestHoldFinished(Tp::PendingOperation *op);
    void onFarstreamChannelCreated(Tp::PendingOperation *op);

protected:
    Tp::AccountPtr mAccount;
    Tp::ContactPtr mContact;
    CallStatus mCallStatus;
    QString mErrorString;
    bool mError;
    QDateTime mStartTime;
    uint mMissedVideoCalls;
    uint mMissedAudioCalls;
    QDateTime mLastMissedVideoCall;
    QDateTime mLastMissedAudioCall;
    bool mRemoteVideoRender;

    FarstreamChannel *mFarstreamChannel;
    QmlGstVideoItem *mIncomingVideo;
    QmlGstVideoItem *mOutgoingVideo;
    uint mCurrentOrientation;

    Tpy::FarstreamChannelFactoryPtr mChannelFactory;
    Tp::PendingChannelRequest *mPendingChannelRequest;
    Tpy::CallChannelPtr mCallChannel;
    bool mIsRequested;
    QTime mCallDuration;

    explicit CallAgent(QObject *parent = 0);

    void setCallStatus(CallStatus callStatus);
    void setErrorString(const QString &errorString);
    void setStartTime(const QDateTime &startTime);
    void setupFarstreamChannel(FarstreamChannel *farstreamChannel);
    void handleReadyChannel();
    Tpy::SendingState getVideoSentState() const;

    static const char *streamType(Tp::MediaStreamType streamType);
};

#endif // CALLAGENT_H
