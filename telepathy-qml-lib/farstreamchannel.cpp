/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "farstreamchannel.h"
#include <TelepathyQt4/Connection>
#include <TelepathyQt4Yell/Farstream/Channel>
#include <QtGstQmlSink/qmlvideosurfacegstsink.h>
#include <gst/farsight/fs-conference-iface.h>
#include <gst/farsight/fs-utils.h>

QmlPainterVideoSurface *FarstreamChannel::mIncomingSurface = 0;
QmlPainterVideoSurface *FarstreamChannel::mOutgoingSurface = 0;

#define VIDEO_SOURCE_ELEMENT "v4l2src"
//#define VIDEO_SOURCE_ELEMENT "autovideosrc"
//#define VIDEO_SOURCE_ELEMENT "videotestsrc"

//#define AUDIO_SOURCE_ELEMENT "autoaudiosrc"
//#define AUDIO_SOURCE_ELEMENT "audiotestsrc"
#define AUDIO_SOURCE_ELEMENT "pulsesrc"

//#define AUDIO_SINK_ELEMENT "autoaudiosink"
//#define AUDIO_SINK_ELEMENT "alsasink"
#define AUDIO_SINK_ELEMENT "pulsesink"
//#define AUDIO_SINK_ELEMENT "filesink"

// only used when the audio sink is a filesink
#define AUDIO_FILESINK_OUTPUT_FILE "/tmp/im-output.wav"

#define COLORSPACE_CONVERT_ELEMENT "ffmpegcolorspace"

#define SINK_GHOST_PAD_NAME "sink"
#define SRC_GHOST_PAD_NAME "src"

class LifetimeTracer {
private:
    const char *filename;
    int line;
    const char *function;
public:
    LifetimeTracer(const char *fn, int l, const char *f): filename(fn), line(l), function(f) { qDebug() << filename << ":" << line << ":" << " entering " << function; }
    ~LifetimeTracer() { qDebug() << filename << ":" << line << ":" << " leaving " << function; }
};
#define LIFETIME_TRACER() LifetimeTracer lifetime_tracer(__FILE__,__LINE__,__PRETTY_FUNCTION__)
#define TRACE() qDebug() << __FILE__ << ":" << __LINE__ << ": trace";

FarstreamChannel::FarstreamChannel(TfChannel *tfChannel, QObject *parent) :
    QObject(parent),
    mTfChannel(tfChannel),
    mState(Tp::MediaStreamStateDisconnected),
    mGstPipeline(0),
    mGstBus(0),
    mGstBusSource(0),
    mGstAudioInput(0),
    mGstAudioInputVolume(0),
    mGstAudioOutput(0),
    mGstAudioOutputVolume(0),
    mGstAudioOutputSink(0),
    mGstAudioOutputActualSink(0),
    mGstVideoInput(0),
    mGstVideoSource(0),
    mGstVideoFlip(0),
    mGstVideoOutput(0),
    mGstVideoOutputSink(0),
    mGstVideoTee(0),
    mGstIncomingVideoSink(0),
    mGstOutgoingVideoSink(0),
    mIncomingVideoItem(0),
    mOutgoingVideoItem(0),
    mCurrentCamera(0),
    mCameraCount(-1),
    mCurrentOrientation(1) // TopUp is the default orientation of the camera (for lenovo tablet)
{
    LIFETIME_TRACER();

    if (!mTfChannel) {
        setError("Unable to create Farstream channel");
        return;
    }

    // connect to the glib-style signals in farstream channel
    mSHClosed = g_signal_connect(mTfChannel, "closed",
        G_CALLBACK(&FarstreamChannel::onClosed), this);
    mSHFsConferenceAdded = g_signal_connect(mTfChannel, "fs-conference-added",
        G_CALLBACK(&FarstreamChannel::onFsConferenceAdded), this);
    mSHFsConferenceRemoved = g_signal_connect(mTfChannel, "fs-conference-removed",
        G_CALLBACK(&FarstreamChannel::onFsConferenceRemoved), this);
    mSHContentAdded = g_signal_connect(mTfChannel, "content-added",
        G_CALLBACK(&FarstreamChannel::onContentAdded), this);
    mSHContentRemoved = g_signal_connect(mTfChannel, "content-removed",
        G_CALLBACK(&FarstreamChannel::onContentRemoved), this);
}

FarstreamChannel::~FarstreamChannel()
{
    LIFETIME_TRACER();

    deinitAudioOutput();
    deinitAudioInput();
    deinitVideoInput();
    deinitVideoOutput();
    deinitIncomingVideoWidget();
    deinitOutgoingVideoWidget();
    deinitGstreamer();

    if (mTfChannel) {
        if (g_signal_handler_is_connected(mTfChannel, mSHClosed)) {
            g_signal_handler_disconnect(mTfChannel, mSHClosed);
            mSHClosed = 0;
        }
        if (g_signal_handler_is_connected(mTfChannel, mSHFsConferenceAdded)) {
            g_signal_handler_disconnect(mTfChannel, mSHFsConferenceAdded);
            mSHClosed = 0;
        }
        if (g_signal_handler_is_connected(mTfChannel, mSHFsConferenceRemoved)) {
            g_signal_handler_disconnect(mTfChannel, mSHFsConferenceRemoved);
            mSHClosed = 0;
        }
        if (g_signal_handler_is_connected(mTfChannel, mSHContentAdded)) {
            g_signal_handler_disconnect(mTfChannel, mSHContentAdded);
            mSHClosed = 0;
        }
        if (g_signal_handler_is_connected(mTfChannel, mSHContentRemoved)) {
            g_signal_handler_disconnect(mTfChannel, mSHContentRemoved);
            mSHClosed = 0;
        }
        g_object_unref(mTfChannel);
        mTfChannel = 0;
    }
}

Tp::MediaStreamState FarstreamChannel::state() const
{
    return mState;
}

void FarstreamChannel::setState(Tp::MediaStreamState state)
{
    qDebug() << "FarstreamChannel::setState(" << state << ") current mState=" << mState;

    if (mState != state) {
        mState = state;
        emit stateChanged();
    }
}

void FarstreamChannel::setError(const QString &errorMessage)
{
    qDebug() << "ERROR: " << errorMessage;

    emit error(errorMessage);
}

void FarstreamChannel::init()
{
    LIFETIME_TRACER();
    initGstreamer();

    // surfaces and sinks are created now to avoid problems with threads

    if (!mOutgoingSurface) {
        mOutgoingSurface = new QmlPainterVideoSurface();
        if (!mOutgoingSurface) {
            setError("Unable to create outgoing painter video surface");
        }
    }

    mGstOutgoingVideoSink = GST_ELEMENT(QmlVideoSurfaceGstSink::createSink(mOutgoingSurface));
    if (!mGstOutgoingVideoSink) {
        setError("Unable to create outgoing gst sink");
    }

    if (!mIncomingSurface) {
        mIncomingSurface = new QmlPainterVideoSurface();
        if (!mIncomingSurface) {
            setError("Unable to create incoming painter video surface");
        }
    }

    mGstIncomingVideoSink = GST_ELEMENT(QmlVideoSurfaceGstSink::createSink(mIncomingSurface));
    if (!mGstIncomingVideoSink) {
        setError("Unable to create incoming gst sink");
    }

    if (mGstPipeline) {
        GstStateChangeReturn ret = gst_element_set_state(mGstPipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            setError("GStreamer pipeline cannot be played");
            return;
        }
    }

    setState(Tp::MediaStreamStateConnecting);
}

void FarstreamChannel::initGstreamer()
{
    LIFETIME_TRACER();

    mGstPipeline = gst_pipeline_new(NULL);
    if (!mGstPipeline) {
        setError("Gstreamer pipeline could not be created");
        return;
    }

    mGstBus = gst_pipeline_get_bus(GST_PIPELINE(mGstPipeline));
    if (!mGstBus) {
        setError("Gstreamer bus could not be retrieved");
        return;
    }

    mGstBusSource = gst_bus_add_watch(mGstBus, (GstBusFunc) &FarstreamChannel::onBusWatch, this);
    if (!mGstBusSource) {
        setError("Gstreamer bus add watch failed");
        return;
    }
}

void FarstreamChannel::deinitGstreamer()
{
    LIFETIME_TRACER();

    foreach(FsElementAddedNotifier * notifier, mFsNotifiers) {
        fs_element_added_notifier_remove(notifier, GST_BIN(mGstPipeline));
        g_object_unref(notifier);
    }
    mFsNotifiers.clear();

    if (mGstBusSource) {
        g_source_remove(mGstBusSource);
        mGstBusSource = 0;
    }

    if (mGstBus) {
        gst_object_unref(mGstBus);
        mGstBus = 0;
    }

    if (mGstPipeline) {
        gst_element_set_state(mGstPipeline, GST_STATE_NULL);
        gst_object_unref(mGstPipeline);
        mGstPipeline = 0;
    }
}

static void setPhoneMediaRole(GstElement *element)
{
    GstStructure *props = gst_structure_from_string ("props,media.role=phone", NULL);
    g_object_set(element, "stream-properties", props, NULL);
    gst_structure_free(props);
}

static void releaseGhostPad(GstElement *bin, const char *name, GstElement *sink)
{
    LIFETIME_TRACER();

    // if trying to disconnect while connecting, this will not have been setup yet
    if (!bin)
        return;

    qDebug() << "Releasing ghost pad named " << name << " from bin " << gst_element_get_name(bin) << ", sink " << (sink ? gst_element_get_name(sink) : "<none>");
    if (bin) {
        GstPad *ghostPad = gst_element_get_static_pad(GST_ELEMENT(bin), name);
        if (GST_GHOST_PAD(ghostPad)) {
            GstPad *pad = gst_ghost_pad_get_target(GST_GHOST_PAD(ghostPad));
            if (pad) {
                gst_ghost_pad_set_target(GST_GHOST_PAD(ghostPad), NULL);
                if (sink) {
                    qDebug() << "Releasing request pad under ghost pad";
                    gst_element_release_request_pad(sink, pad);
                }
                gst_object_unref(pad);
            }
        }
    }
}

void FarstreamChannel::initAudioInput()
{
    LIFETIME_TRACER();

    mGstAudioInput = gst_bin_new("audio-input-bin");
    if (!mGstAudioInput) {
        setError("GStreamer audio input bin could not be created");
        return;
    }
    gst_object_ref(mGstAudioInput);
    gst_object_sink(mGstAudioInput);

    GstElement *source = 0;
    source = addElementToBin(mGstAudioInput, source, AUDIO_SOURCE_ELEMENT);
    if (!source) {
        setError("GStreamer audio input source could not be created");
        return;
    }

    /* Pulseaudio enjoys dying. When it does, pulsesrc/pulsesink can't go to
       READY as they can't connect to the pulse daemon. This will cause the
       entire pipeline to fail to set to PLAYING. So we try to get pulsesrc to
       READY temporarily here, and if it doesn't work, replace it with audiotestsrc
       and volume, such that we'll get audio data at zero volume - silence. */
    GstStateChangeReturn scr = gst_element_set_state(source, GST_STATE_READY);
    if (scr == GST_STATE_CHANGE_FAILURE) {
        qDebug() << "Audio source \"" AUDIO_SOURCE_ELEMENT "\" failed to get to READY, replacing with a fake audio source";
        gst_bin_remove(GST_BIN(mGstAudioInput), source);
        source = NULL;
        source = addElementToBin(mGstAudioInput, source, "audiotestsrc");
        g_object_set(source, "is-live", 1, NULL);
        g_object_set(source, "wave", "silence", NULL);
    }
    else {
        // put things back how we found them
        gst_element_set_state(source, GST_STATE_NULL);

        if (!strcmp(AUDIO_SOURCE_ELEMENT, "pulsesrc")) {
            setPhoneMediaRole(source);
        }
    }

    mGstAudioInputVolume = addElementToBin(mGstAudioInput, source, "volume");
    if (!mGstAudioInputVolume) {
        setError("GStreamer audio input volume could not be created");
    } else {
        source = mGstAudioInputVolume;
        gst_object_ref (mGstAudioInputVolume);
    }

    /*
    GstElement *level = addElementToBin(mGstAudioInput, source, "level");
    if (!level) {
        setError("GStreamer audio input level could not be created");
    } else {
        source = level;
    }
    */

    GstPad *src = gst_element_get_static_pad(source, "src");
    if (!src) {
        setError("GStreamer audio volume source pad failed");
        return;
    }

    qDebug() << "Creating ghost pad named " << SRC_GHOST_PAD_NAME << " for bin " << gst_element_get_name(mGstAudioInput);
    GstPad *ghost = gst_ghost_pad_new(SRC_GHOST_PAD_NAME, src);
    gst_object_unref(src);
    if (!ghost) {
        setError("GStreamer ghost src pad failed");
        return;
    }

    gboolean res = gst_element_add_pad(GST_ELEMENT(mGstAudioInput), ghost);
    if (!res) {
        setError("GStreamer add audio sink ghost pad failed");
        return;
    }
}

void FarstreamChannel::deinitAudioInput()
{
    LIFETIME_TRACER();

    if (!mGstAudioInput) {
      qDebug() << "Audio input not initialized, doing nothing";
      return;
    }

    releaseGhostPad(mGstAudioInput, SRC_GHOST_PAD_NAME, NULL);

    if (mGstAudioInput) {
        gst_element_set_state(mGstAudioInput, GST_STATE_NULL);
        gst_bin_remove (GST_BIN(mGstPipeline), mGstAudioInput);
        gst_object_unref(mGstAudioInput);
        mGstAudioInput = 0;
    }

    if (mGstAudioInputVolume) {
        gst_object_unref(mGstAudioInputVolume);
        mGstAudioInputVolume = 0;
    }
}

GstElement *FarstreamChannel::pushElement(GstElement *bin, GstElement *&last, const char *factory, bool optional, GstElement **copy, bool checkLink)
{
    if (copy)
        *copy = NULL;
    GstElement *e = addElementToBin(bin, last, factory, checkLink);
    if (!e) {
        if (optional) {
            qDebug() << "Failed to create or link optional element " << factory;
        }
        else {
            setError(QString("Failed to create or link element ") + factory);
        }
        return NULL;
    }
    last = e;
    if (copy) {
        gst_object_ref(e);
        *copy = e;
    }
    return e;
}

void FarstreamChannel::initAudioOutput()
{
    LIFETIME_TRACER();

    mGstAudioOutputSink = NULL;
    mGstAudioOutput = gst_bin_new("audio-output-bin");
    if (!mGstAudioOutput) {
        setError("GStreamer audio output could not be created");
        return;
    }
    gst_object_ref(mGstAudioOutput);
    gst_object_sink(mGstAudioOutput);

    GstElement *source = 0;
    if (strcmp(AUDIO_SINK_ELEMENT, "pulsesink")) {
        pushElement(mGstAudioOutput, source, "audioresample", true, &mGstAudioOutputSink, false);
        pushElement(mGstAudioOutput, source, "volume", true, &mGstAudioOutputVolume, false);
    }
    else {
        mGstAudioOutputVolume = NULL;
    }
    if (!strcmp(AUDIO_SINK_ELEMENT, "filesink")) {
        pushElement(mGstAudioOutput, source, "audioconvert", false, NULL, false);
        pushElement(mGstAudioOutput, source, "wavenc", false, NULL, false);
        pushElement(mGstAudioOutput, source, AUDIO_SINK_ELEMENT, false, &mGstAudioOutputActualSink, false);
        g_object_set(mGstAudioOutputActualSink, "location", AUDIO_FILESINK_OUTPUT_FILE, NULL);
    }
    else {
        GstElement *previous_source = source;
        pushElement(mGstAudioOutput, source, AUDIO_SINK_ELEMENT, false, &mGstAudioOutputActualSink ,false);

        /* Pulseaudio enjoys dying. When it does, pulsesrc/pulsesink can't go to
           READY as they can't connect to the pulse daemon. This will cause the
           entire pipeline to fail to set to PLAYING. So we try to get pulsesink
           to READY temporarily here, and replace it by fakesink if it fails, so
           the pipeline can run happily */
        GstStateChangeReturn scr = gst_element_set_state(mGstAudioOutputActualSink, GST_STATE_READY);
        if (scr == GST_STATE_CHANGE_FAILURE) {
            qDebug() << "Audio sink \"" AUDIO_SINK_ELEMENT "\" failed to get to READY, replacing with a fake audio sink";
            if (previous_source)
              gst_element_unlink(previous_source, mGstAudioOutputActualSink);
            gst_bin_remove(GST_BIN(mGstAudioOutput), mGstAudioOutputActualSink);
            source = previous_source;
            pushElement(mGstAudioOutput, source, "fakesink", false, &mGstAudioOutputActualSink, false);
        }
        else {
            // restore sink as we created it
            gst_element_set_state(mGstAudioOutputActualSink, GST_STATE_NULL);
        }

        if (!mGstAudioOutputSink) {
            mGstAudioOutputSink = mGstAudioOutputActualSink;
            gst_object_ref(mGstAudioOutputSink);
        }

        if (!strcmp(AUDIO_SINK_ELEMENT, "pulsesink")) {
            setPhoneMediaRole(mGstAudioOutputActualSink);
        }
    }
}

void FarstreamChannel::deinitAudioOutput()
{
    LIFETIME_TRACER();

    if (!mGstAudioOutput) {
      qDebug() << "Audio output not initialized, doing nothing";
      return;
    }


    releaseGhostPad(mGstAudioOutput, SINK_GHOST_PAD_NAME, NULL);

    if (mGstAudioOutput) {
        gst_element_set_state(mGstAudioOutput, GST_STATE_NULL);
        gst_object_unref(mGstAudioOutput);
        gst_bin_remove (GST_BIN(mGstPipeline), mGstAudioOutput);
        mGstAudioOutput = 0;
    }

    if (mGstAudioOutputVolume) {
        gst_object_unref(mGstAudioOutputVolume);
        mGstAudioOutputVolume = 0;
    }

    if (mGstAudioOutputSink) {
        gst_object_unref(mGstAudioOutputSink);
        mGstAudioOutputSink = 0;
    }

    if (mGstAudioOutputActualSink) {
        gst_object_unref(mGstAudioOutputActualSink);
        mGstAudioOutputActualSink = 0;
    }
}

void FarstreamChannel::initVideoInput()
{
    LIFETIME_TRACER();

    mGstVideoInput = gst_bin_new("video-input-bin");
    if (!mGstVideoInput) {
        setError("GStreamer video input could not be created");
        return;
    }
    gst_object_ref(mGstVideoInput);
    gst_object_sink(mGstVideoInput);

    GstElement *element = 0;
    GstElement *source = 0;

    pushElement(mGstVideoInput, source, VIDEO_SOURCE_ELEMENT, false, &mGstVideoSource, false);

    /* Prefer videomaxrate if it's available */
    if (pushElement(mGstVideoInput, source, "videomaxrate", true, NULL, false) == NULL)
        pushElement(mGstVideoInput, source, "fsvideoanyrate", true, NULL, false);
    pushElement(mGstVideoInput, source, "videoscale", false, NULL, false);
    pushElement(mGstVideoInput, source, COLORSPACE_CONVERT_ELEMENT, false, NULL, false);

    GstElement *capsfilter = pushElement(mGstVideoInput, source, "capsfilter", false, NULL, false);
    if (capsfilter) {
        GstCaps *caps = gst_caps_new_simple(
            "video/x-raw-yuv",
            "width", G_TYPE_INT, 320,
            "height", G_TYPE_INT, 240,
            "framerate", GST_TYPE_FRACTION, 15, 1,
            "format", GST_TYPE_FOURCC, GST_MAKE_FOURCC('I','4','2','0'),
            NULL);
        /*
        GstCaps *caps = gst_caps_new_simple(
            "video/x-raw-yuv",
            "width", GST_TYPE_INT_RANGE, 320, 352,
            "height", GST_TYPE_INT_RANGE, 240, 288,
            "framerate", GST_TYPE_FRACTION, 15, 1,
            NULL);
        */
        if (caps) {
            g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
        }
    }

    pushElement(mGstVideoInput, source, "videoflip", true, &mGstVideoFlip, false);
    if (mGstVideoFlip) {
        // setup video flip element according to current orientation
        onOrientationChanged(mCurrentOrientation);
    }

    pushElement(mGstVideoInput, source, "tee", false, &mGstVideoTee, false);
    pushElement(mGstVideoInput, source, "videoscale", false, NULL, false);
    pushElement(mGstVideoInput, source, COLORSPACE_CONVERT_ELEMENT, false, NULL, false);

    element = mGstOutgoingVideoSink; // = GST_ELEMENT(QmlVideoSurfaceGstSink::createSink(mOutgoingSurface));
    element = addAndLink(GST_BIN(mGstVideoInput), source, element, false);
    if (!element) {
        setError("GStreamer outgoing video sink could not be created");
    } else {
        source = element;
        gst_object_ref(mGstOutgoingVideoSink);
        g_object_set(G_OBJECT(element), "async", true, "sync", false, NULL); //, "force-aspect-ratio", true, NULL);
    }

    qDebug() << "Requesting pad for bin " << gst_element_get_name(mGstVideoInput) << " to add a src ghost pad to";
    GstPad *src = gst_element_get_request_pad(mGstVideoTee, "src%d");
    if (!src) {
        setError("GStreamer get video input source pad failed");
        return;
    }

    qDebug() << "Creating ghost pad named " << SRC_GHOST_PAD_NAME << " for bin " << gst_element_get_name(mGstVideoInput);
    GstPad *ghost = gst_ghost_pad_new(SRC_GHOST_PAD_NAME, src);
    if (!ghost) {
        setError("GStreamer create new video src pad failed");
        return;
    }

    gboolean res = gst_element_add_pad(GST_ELEMENT(mGstVideoInput), ghost);
    if (!res) {
        setError("GStreamer add video src pad failed");
        return;
    }
}

void FarstreamChannel::deinitVideoInput()
{
    LIFETIME_TRACER();

    if (!mGstVideoInput) {
      qDebug() << "Video input not initialized, doing nothing";
      return;
    }

    releaseGhostPad(mGstVideoInput, SRC_GHOST_PAD_NAME, mGstVideoTee);

    if (mGstVideoTee) {
        gst_object_unref(mGstVideoTee);
        mGstVideoTee = 0;
    }

    if (mGstVideoSource) {
        gst_object_unref(mGstVideoSource);
        mGstVideoSource = 0;
    }

    if (mGstVideoFlip) {
        gst_object_unref(mGstVideoFlip);
        mGstVideoFlip = 0;
    }

    if (mGstOutgoingVideoSink) {
        gst_object_unref(mGstOutgoingVideoSink);
    }

    if (mGstVideoInput) {
        gst_element_set_state(mGstVideoInput, GST_STATE_NULL);
        gst_bin_remove(GST_BIN(mGstVideoInput), mGstOutgoingVideoSink);
        gst_bin_remove (GST_BIN(mGstPipeline), mGstVideoInput);
        gst_object_unref(mGstVideoInput);
        mGstVideoInput = 0;
    }

    /*
    if (mOutgoingSurface) {
      delete mOutgoingSurface;
      mOutgoingSurface = 0;
    }
    */
}

void FarstreamChannel::initVideoOutput()
{
    LIFETIME_TRACER();

    mGstVideoOutput = gst_bin_new("video-output-bin");
    if (!mGstVideoOutput) {
        setError("GStreamer video output could not be created");
        return;
    }
    gst_object_ref(mGstVideoOutput);
    gst_object_sink(mGstVideoOutput);

    GstElement *source = 0;

    mGstVideoOutputSink = pushElement(mGstVideoOutput, source, "fsfunnel", false, &mGstVideoOutputSink, false);
    pushElement(mGstVideoOutput, source, "videoscale", true, NULL, false);
    pushElement(mGstVideoOutput, source, COLORSPACE_CONVERT_ELEMENT, true, NULL, false);

    /*
    if (!mIncomingSurface) {
        mIncomingSurface = new QmlPainterVideoSurface();
        if (!mIncomingSurface) {
            setError("Unable to create incoming painter video surface");
        }
        //QGLContext* context = new QGLContext(g->context()->format());
        QGLContext* context = new QGLContext(QGLFormat::defaultFormat());
        mIncomingSurface->setGLContext(context);
    }*/

    GstElement *element = mGstIncomingVideoSink; // = GST_ELEMENT(QmlVideoSurfaceGstSink::createSink(mIncomingSurface));
    element = addAndLink(GST_BIN(mGstVideoOutput), source, element, false);
    if (!element) {
        setError("GStreamer qt video sink could not be created");
    } else {
        source = element;
        gst_object_ref(mGstIncomingVideoSink);
        //g_object_set(G_OBJECT(element), "async", false, "sync", false, "force-aspect-ratio", true, NULL);
    }

/*
    GstStateChangeReturn resState = gst_element_set_state(mGstVideoOutput, GST_STATE_PLAYING);
    if (resState == GST_STATE_CHANGE_FAILURE) {
        setError("GStreamer input could not be set to playing");
        return;
    }*/
}

void FarstreamChannel::deinitVideoOutput()
{
    LIFETIME_TRACER();

    if (!mGstVideoOutput) {
      qDebug() << "Video output not initialized, doing nothing";
      return;
    }

    releaseGhostPad(mGstVideoOutput, SINK_GHOST_PAD_NAME, mGstVideoOutputSink);

    if (mGstVideoOutputSink) {
        gst_object_unref(mGstVideoOutputSink);
        mGstVideoOutputSink = 0;
    }

    if (mGstIncomingVideoSink) {
        gst_object_unref(mGstIncomingVideoSink);
    }

    if (mGstVideoOutput) {
        gst_element_set_state(mGstVideoOutput, GST_STATE_NULL);
        gst_bin_remove(GST_BIN(mGstVideoOutput), mGstIncomingVideoSink);
        gst_bin_remove (GST_BIN(mGstPipeline), mGstVideoOutput);
        gst_object_unref(mGstVideoOutput);
        mGstVideoOutput = 0;
    }

    /*
    if (mIncomingSurface) {
      delete mIncomingSurface;
      mIncomingSurface = 0;
    }
    */
}

void FarstreamChannel::initOutgoingVideoWidget()
{
    LIFETIME_TRACER();

    if (mOutgoingVideoItem) {
        mOutgoingVideoItem->setSurface(mOutgoingSurface);
    } else {
        setError("Outgoing video item is unknown yet");
    }
}

void FarstreamChannel::deinitOutgoingVideoWidget()
{
    LIFETIME_TRACER();

    if (mOutgoingVideoItem) {
        mOutgoingVideoItem->setSurface(0);
    }
}

void FarstreamChannel::initIncomingVideoWidget()
{
    LIFETIME_TRACER();

    if (mIncomingVideoItem) {
        mIncomingVideoItem->setSurface(mIncomingSurface);
    } else {
        setError("Incoming video item is unknown yet");
    }
}

void FarstreamChannel::deinitIncomingVideoWidget()
{
    LIFETIME_TRACER();

    if (mIncomingVideoItem) {
        mIncomingVideoItem->setSurface(0);
    }
}

void FarstreamChannel::onClosed(TfChannel *tfc, FarstreamChannel *self)
{
    Q_UNUSED(tfc);

    qDebug() << "FarstreamChannel::onClosed:";
    self->setState(Tp::MediaStreamStateDisconnected);
}

gboolean FarstreamChannel::onBusWatch(GstBus *bus, GstMessage *message, FarstreamChannel *self)
{
    Q_UNUSED(bus);

    if (!self->mTfChannel) {
        return TRUE;
    }

    const GstStructure *s = gst_message_get_structure(message);
    if (s == NULL) {
        goto error;
    }

    if (gst_structure_has_name (s, "farsight-send-codec-changed")) {

        const GValue *val = gst_structure_get_value(s, "codec");
        if (!val) {
            goto error;
        }

        FsCodec *codec = static_cast<FsCodec *> (g_value_get_boxed(val));
        if (!codec) {
            goto error;
        }

        val = gst_structure_get_value(s, "session");
        if (!val) {
            goto error;
        }

        FsSession *session = static_cast<FsSession *> (g_value_get_object(val));
        if (!session) {
            goto error;
        }

        FsMediaType type;
        g_object_get(session, "media-type", &type, NULL);

        qDebug() << "FarstreamChannel::onBusWatch: farsight-send-codec-changed "
                 << " type=" << type
                 << " codec=" << fs_codec_to_string(codec);

    } else if (gst_structure_has_name(s, "farsight-recv-codecs-changed")) {

        const GValue *val = gst_structure_get_value(s, "codecs");
        if (!val) {
            goto error;
        }

        GList *codecs = static_cast<GList *> (g_value_get_boxed(val));
        if (!codecs) {
            goto error;
        }

        val = gst_structure_get_value(s, "stream");
        if (!val) {
            goto error;
        }

        FsStream *stream = static_cast<FsStream *> (g_value_get_object(val));
        if (!stream) {
            goto error;
        }

        FsSession *session = 0;
        g_object_get(stream, "session", &session, NULL);
        if (!session) {
            goto error;
        }

        FsMediaType type = FS_MEDIA_TYPE_LAST;
        g_object_get(session, "media-type", &type, NULL);

        qDebug() << "FarstreamChannel::onBusWatch: farsight-recv-codecs-changed "
                 << " type=" << type;
        GList *list;
        for (list = codecs; list != NULL; list = g_list_next(list)) {
            FsCodec *codec = static_cast<FsCodec *> (list->data);
            qDebug() << "       codec " << fs_codec_to_string(codec);
        }

        g_object_unref(session);

    } else if (gst_structure_has_name(s, "farsight-new-active-candidate-pair")) {

        const GValue *val = gst_structure_get_value(s, "remote-candidate");
        if (!val) {
            goto error;
        }

        FsCandidate *remote_candidate = static_cast<FsCandidate *> (g_value_get_boxed(val));
        if (!remote_candidate) {
            goto error;
        }

        val = gst_structure_get_value(s, "local-candidate");
        if (!val) {
            goto error;
        }

        FsCandidate *local_candidate = static_cast<FsCandidate *> (g_value_get_boxed(val));
        if (!local_candidate) {
            goto error;
        }

        val = gst_structure_get_value(s, "stream");
        if (!val) {
            goto error;
        }

        FsStream *stream = static_cast<FsStream *> (g_value_get_object(val));
        if (!stream) {
            goto error;
        }

        FsSession *session = 0;
        g_object_get(stream, "session", &session, NULL);
        if (!session) {
            goto error;
        }

        FsMediaType type = FS_MEDIA_TYPE_LAST;
        g_object_get(session, "media-type", &type, NULL);

        qDebug() << "FarstreamChannel::onBusWatch: farsight-new-active-candidate-pair ";

        if (remote_candidate) {
            qDebug() << "   remote candidate mediatype=" << type
                     << " foundation=" << remote_candidate->foundation << " id=" << remote_candidate->component_id
                     << " IP=" << remote_candidate->ip << ":" << remote_candidate->port
                     << " BaseIP=" << remote_candidate->base_ip << ":" << remote_candidate->base_port
                     << " proto=" << remote_candidate->proto
                     << " type=" << remote_candidate->type;
        }

        if (local_candidate) {
            qDebug() << "   local candidate mediatype=" << type
                     << " foundation=" << local_candidate->foundation << " id=" << local_candidate->component_id
                     << " IP=" << local_candidate->ip << ":" << local_candidate->port
                     << " BaseIP=" << local_candidate->base_ip << ":" << local_candidate->base_port
                     << " proto=" << local_candidate->proto
                     << " type=" << local_candidate->type;
        }

        g_object_unref(session);
    }

error:

    tf_channel_bus_message(self->mTfChannel, message);
    return TRUE;
}

void FarstreamChannel::setMute(bool mute)
{
    qDebug() << "FarstreamChannel::setMute: mute=" << mute;

    if (!mGstAudioInputVolume) {
        return;
    }

    g_object_set(G_OBJECT(mGstAudioInputVolume), "mute", mute, NULL);
}

bool FarstreamChannel::mute() const
{
    if (!mGstAudioInputVolume) {
        return false;
    }

    gboolean mute = FALSE;
    g_object_get (G_OBJECT(mGstAudioInputVolume), "mute", &mute, NULL);

    return mute;
}

void FarstreamChannel::setInputVolume(double volume)
{
    qDebug() << "FarstreamChannel::setInputVolume: volume=" << volume;

    if (!mGstAudioInputVolume) {
        return;
    }

    GParamSpec *pspec;
    pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(mGstAudioInputVolume), "volume");
    if (!pspec) {
        return;
    }

    GParamSpecDouble *pspec_double;
    pspec_double = G_PARAM_SPEC_DOUBLE(pspec);
    if (!pspec_double) {
        return;
    }

    volume = CLAMP(volume, pspec_double->minimum, pspec_double->maximum);
    g_object_set(G_OBJECT(mGstAudioInputVolume), "volume", volume, NULL);
}

double FarstreamChannel::inputVolume() const
{
    if (!mGstAudioInputVolume) {
        return 0.0;
    }

    double volume;
    g_object_get(G_OBJECT(mGstAudioInputVolume), "volume", &volume, NULL);

    return volume;
}

void FarstreamChannel::setVolume(double volume)
{
    qDebug() << "FarstreamChannel::setVolume: volume=" << volume;

    GstElement *volumeElement = mGstAudioOutputVolume ? mGstAudioOutputVolume : mGstAudioOutputActualSink;
    if (!volumeElement) {
        return;
    }

    GParamSpec *pspec;
    pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(volumeElement), "volume");
    if (!pspec) {
        return;
    }

    GParamSpecDouble *pspec_double;
    pspec_double = G_PARAM_SPEC_DOUBLE(pspec);
    if (!pspec_double) {
        return;
    }

    volume = CLAMP(volume, pspec_double->minimum, pspec_double->maximum);
    g_object_set(G_OBJECT(volumeElement), "volume", volume, NULL);
}

double FarstreamChannel::volume() const
{
    GstElement *volumeElement = mGstAudioOutputVolume ? mGstAudioOutputVolume : mGstAudioOutputActualSink;
    if (!volumeElement) {
        return 0.0;
    }

    double volume;
    g_object_get(G_OBJECT(volumeElement), "volume", &volume, NULL);

    return volume;
}

void FarstreamChannel::setIncomingVideo(QmlGstVideoItem *item)
{
    qDebug() << "FarstreamChannel::setIncomingVideo: this=" << (void *) this << " item=" << (void *) item;
    mIncomingVideoItem = item;
    if (item) {
        connect(item, SIGNAL(destroyed(QObject*)), SLOT(onGstVideoItemDestroyed(QObject*)));
    }
    initIncomingVideoWidget();
}

void FarstreamChannel::setOutgoingVideo(QmlGstVideoItem *item)
{
    qDebug() << "FarstreamChannel::setOutgoingVideo: this=" << (void *) this << " item=" << (void *) item;
    mOutgoingVideoItem = item;
    if (item) {
        connect(item, SIGNAL(destroyed(QObject*)), SLOT(onGstVideoItemDestroyed(QObject*)));
    }
    initOutgoingVideoWidget();
}

bool FarstreamChannel::canSwapVideos() const
{
    return (mGstVideoOutput != NULL);
}

void FarstreamChannel::onGstVideoItemDestroyed(QObject *obj)
{
    qDebug() << "FarstreamChannel::onGstVideoItemDestroyed: " << obj;

    if (obj == mOutgoingVideoItem) {
        qDebug() << "FarstreamChannel::onGstVideoItemDestroyed: reset outgoing video item";
        mOutgoingVideoItem = 0;
    }

    if (obj == mIncomingVideoItem) {
        qDebug() << "FarstreamChannel::onGstVideoItemDestroyed: reset incoming video item";
        mIncomingVideoItem = 0;
    }
}

GstElement *FarstreamChannel::addElementToBin(GstElement *bin, GstElement *src, const char *factoryName, bool checkLink)
{
    qDebug() << "FarstreamChannel::addElementToBin: bin=" << bin << " src=" << src << " factoryName=" << factoryName;

    GstBin *binobj = GST_BIN(bin);
    if (!binobj) {
        setError(QLatin1String("Element factory not found ") + factoryName);
        return 0;
    }

    GstElement *ret;
    if ((ret = gst_element_factory_make(factoryName, 0)) == 0) {
        setError(QLatin1String("Element factory not found ") + factoryName);
        return 0;
    }
    
    return addAndLink(binobj, src, ret, checkLink);
}

GstElement *FarstreamChannel::addAndLink(GstBin *binobj, GstElement *src, GstElement *ret, bool checkLink)
{
    gboolean res;

    qDebug() << "FarstreamChannel::addAndLink: binobj="
	     << gst_element_get_name(GST_ELEMENT(binobj)) 
	     << " src="
	     << (src ? gst_element_get_name(src) : "(NULL)")
             << " dst="
	     << gst_element_get_name(ret);

    if (!gst_bin_add(binobj, ret)) {
        setError(QLatin1String("Could not add to bin "));
        gst_object_unref(ret);
        return 0;
    }

    if (!src) {
        return ret;
    }

    if (checkLink) {
      res = gst_element_link(src, ret);
    }
    else {
      res = gst_element_link_pads_full(src, NULL, ret, NULL, GST_PAD_LINK_CHECK_NOTHING);
    }
    if (!res) {
        setError(QLatin1String("Failed to link "));
        gst_bin_remove(binobj, ret);
        return 0;
    }

    return ret;
}

void FarstreamChannel::swapCamera()
{
    mCurrentCamera++;
    if (mCurrentCamera >= cameraCount()) {
        mCurrentCamera = 0;
    }

    if (mGstVideoInput && mGstVideoSource) {
        GstState state;
        GstState pending;
        gst_element_get_state(mGstVideoSource, &state, &pending, 0);
        qDebug() << "FarstreamChannel::swapCamera:: state=" << state << " pending=" << pending;

        GstStateChangeReturn ret = gst_element_set_state(mGstVideoInput, GST_STATE_PAUSED);
        qDebug() << "FarstreamChannel::swapCamera: camera paused " << ret;

        g_object_set(G_OBJECT(mGstVideoSource), "device", currentCameraDevice().data(), NULL);
        qDebug() << "FarstreamChannel::swapCamera: changing device to " << currentCameraDevice();

        ret = gst_element_set_state(mGstVideoInput, state);
        qDebug() << "FarstreamChannel::swapCamera: camera playing " << ret;
    }
}

bool FarstreamChannel::cameraSwappable() const
{
    return (cameraCount() > 1);
}

int FarstreamChannel::currentCamera() const
{
    return mCurrentCamera;
}

QString FarstreamChannel::currentCameraDevice() const
{
    QString filename("/dev/video");
    return filename + QString::number(mCurrentCamera);
}

int FarstreamChannel::countCameras() const
{
    int count = 0;
    QString filename("/dev/video");
    while (QFile::exists(filename + QString::number(count))) {
        count++;
    }
    return count;
}

int FarstreamChannel::cameraCount() const
{
    if (mCameraCount < 0) {
        mCameraCount = countCameras();
        mCurrentCamera = 0;
    }
    return mCameraCount;
}

void FarstreamChannel::setVideoFlipMethod(uint deg, uint mirrored)
{
    if (!mGstVideoFlip) {
        return;
    }

    if (mirrored == UINT_MAX) {
        mirrored = this->mirrored();
    }

    uint method = 0;
    switch (deg % 360) {
    case 0:
        // 0 : GST_VIDEO_FLIP_METHOD_IDENTITY
        // 4 : GST_VIDEO_FLIP_METHOD_HORIZ,
        method = mirrored ? 4 : 0;
        break;
    case 90:
        // 1 : GST_VIDEO_FLIP_METHOD_90R
        // 5 : GST_VIDEO_FLIP_METHOD_VERT,
        method = mirrored ? 5 : 1;
        break;
    case 180:
        // 2 : GST_VIDEO_FLIP_METHOD_180
        // 6 : GST_VIDEO_FLIP_METHOD_TRANS
        method = mirrored ? 6 : 2;
        break;
    case 270:
        // 3 : GST_VIDEO_FLIP_METHOD_90L
        // 7 : GST_VIDEO_FLIP_METHOD_OTHER
        method = mirrored ? 7 : 3;
        break;
    default:
        qWarning() << "FarstreamChannel::setVideoFlipMethod: invalid rotation";
        return;
    }

    qDebug() << "FarstreamChannel::setVideoFlipMethod: deg=" << deg << "mirrored=" << mirrored << "method=" << method;

    g_object_set(G_OBJECT(mGstVideoFlip), "method", method, NULL);
}

uint FarstreamChannel::cameraRotation() const
{
    if (!mGstVideoFlip) {
        return 0;
    }

    uint method = 0;
    g_object_get(G_OBJECT(mGstVideoFlip), "method", &method, NULL);
    return (method % 4) * 90;
}

uint FarstreamChannel::mirrored() const
{
    if (!mGstVideoFlip) {
        return 0;
    }

    /*
    uint method = 0;
    g_object_get(G_OBJECT(mGstVideoFlip), "method", &method, NULL);
    return (method >= 4) ? 1 : 0;
    */
    return 0;
}

void FarstreamChannel::onOrientationChanged(uint orientation)
{
    qDebug() << "FarstreamChannel::onOrientationChanged: orientation:" << orientation;

    mCurrentOrientation = orientation;
    if (!mGstVideoFlip) {
        return;
    }

    uint rotation = 0;
    switch (orientation) {
    case 0: // Right Up
        rotation = 90;
        break;
    case 1: // Top Up
        rotation = 0;
        break;
    case 2: // Left Up
        rotation = 270;
        break;
    case 3: // Top Down
        rotation = 180;
        break;
    }

    setVideoFlipMethod(rotation);
}

void FarstreamChannel::onFsConferenceAdded(TfChannel *tfc, FsConference * conf, FarstreamChannel *self)
{
    Q_UNUSED(tfc);
    Q_ASSERT(conf);

    qDebug() << "FarstreamChannel::onFsConferenceAdded: tfc=" << tfc << " conf=" << conf << " self=" << self;

    if (!self->mGstPipeline) {
        self->setError("GStreamer pipeline not setup");
        return;
    }

    /* Add notifier to set the various element properties as needed */
    GKeyFile *keyfile = fs_utils_get_default_element_properties(GST_ELEMENT(conf));
    if (keyfile != NULL)
    {
        qDebug() << "Loaded default codecs for " << GST_ELEMENT_NAME(conf);
        FsElementAddedNotifier *notifier = fs_element_added_notifier_new();
        fs_element_added_notifier_set_properties_from_keyfile(notifier, keyfile);
        fs_element_added_notifier_add (notifier, GST_BIN (self->mGstPipeline));
        self->mFsNotifiers.append(notifier);
    }

    // add conference to the pipeline
    gboolean res = gst_bin_add(GST_BIN(self->mGstPipeline), GST_ELEMENT(conf));
    if (!res) {
        self->setError("GStreamer farsight conference could not be added to the bin");
        return;
    }

    GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT(conf), GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        self->setError("GStreamer farsight conference cannot be played");
        return;
    }
}

void FarstreamChannel::onFsConferenceRemoved(TfChannel *tfc, FsConference * conf, FarstreamChannel *self)
{
    Q_UNUSED(tfc);
    Q_ASSERT(conf);

    qDebug() << "FarstreamChannel::onFsConferenceRemoved: tfc=" << tfc << " conf=" << conf << " self=" << self;

    if (!self->mGstPipeline) {
        self->setError("GStreamer pipeline not setup");
        return;
    }

    GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT(conf), GST_STATE_NULL);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        self->setError("GStreamer farsight conference cannot be set to null");
        return;
    }

    if (self->mGstPipeline) {
        // remove conference to the pipeline
        gboolean res = gst_bin_remove(GST_BIN(self->mGstPipeline), GST_ELEMENT(conf));
        if (!res) {
            self->setError("GStreamer farsight conference could not be added to the bin");
            return;
        }
    }
}

void FarstreamChannel::onContentAdded(TfChannel *tfc, TfContent * content, FarstreamChannel *self)
{
    Q_UNUSED(tfc);

    LIFETIME_TRACER();

    /*
    * Tells the application that a content has been added. In the callback for
    * this signal, the application should set its preferred codecs, and hook
    * up to any signal from #TfContent it cares about. Special care should be
    * made to connect #TfContent::src-pad-added as well
    * as the #TfContent::start-sending and #TfContent::stop-sending signals.
    */

    if (!self || !self->mGstPipeline) {
        self->setError("GStreamer pipeline not setup");
        return;
    }

    if (!content) {
        self->setError("Invalid content received");
        return;
    }

    g_signal_connect(content, "start-sending",
                     G_CALLBACK(&FarstreamChannel::onStartSending), self);
    g_signal_connect(content, "stop-sending",
                     G_CALLBACK(&FarstreamChannel::onStopSending), self);
    g_signal_connect(content, "src-pad-added",
                     G_CALLBACK(&FarstreamChannel::onSrcPadAddedContent), self);

    guint media_type;
    GstPad *sink;
    g_object_get(content, "media-type", &media_type, "sink-pad", &sink, NULL);
    if (!sink) {
        self->setError("GStreamer cannot get sink pad from content");
        return;
    }
    qDebug() << "FarstreamChannel::onContentAdded: content=" << content << " type=" << media_type;

    /*
    FsSession *session;
    g_object_get(content, "fs-session", &session, NULL);
    if (session) {

        FsCodec *codec;
        g_object_get(session, "current-send-codec", &codec, NULL);
        if (codec) {
            qDebug() << "FarstreamChannel::onStreamCreated: current send codec=" << fs_codec_to_string(codec);
        }

        FsStream *fs_stream = 0;
        g_object_get(content, "farsight-stream", &fs_stream, NULL);
        if (fs_stream) {
            GList *codecs;
            g_object_get(fs_stream, "current-recv-codecs", &codecs, NULL);
            if (codecs) {
                qDebug() << " current received codecs: ";
                GList *list;
                for (list = codecs; list != NULL; list = g_list_next (list)) {
                    FsCodec *codec = static_cast<FsCodec *> (list->data);
                    qDebug() << "       codec " << fs_codec_to_string(codec);
                }

                fs_codec_list_destroy(codecs);
            }
        }
    }*/

    GstElement *sourceElement = 0;
    if (media_type == TP_MEDIA_STREAM_TYPE_AUDIO) {
        qDebug() << "Got audio sink, initializing audio input";
        if (!self->mGstAudioInput) {
            self->initAudioInput();
        }
        sourceElement = self->mGstAudioInput;
    } else if (media_type == TP_MEDIA_STREAM_TYPE_VIDEO) {
        qDebug() << "Got video sink, initializing video intput";
        if (!self->mGstVideoInput) {
            self->initVideoInput();
            self->initOutgoingVideoWidget();
        }
        sourceElement = self->mGstVideoInput;
    } else {
        Q_ASSERT(false);
    }

    if (!sourceElement) {
        self->setError("GStreamer source element not found");
        return;
    }

    gboolean res = gst_bin_add(GST_BIN(self->mGstPipeline), sourceElement);
    if (!res) {
        self->setError("GStreamer could not add source to the pipeline");
        return;
    }

    GstPad *pad = gst_element_get_static_pad(sourceElement, "src");
    if (!pad) {
        self->setError("GStreamer get source element source pad failed");
        return;
    }

    GstPadLinkReturn resLink = gst_pad_link(pad, sink);
    gst_object_unref(pad);
    gst_object_unref(sink);
    if (resLink != GST_PAD_LINK_OK && resLink != GST_PAD_LINK_WAS_LINKED) {
        self->setError("GStreamer could not link input source pad to sink");
        return;
    }

    GstStateChangeReturn resState = gst_element_set_state(sourceElement, GST_STATE_PLAYING);
    if (resState == GST_STATE_CHANGE_FAILURE) {
        self->setError("GStreamer input could not be set to playing");
        return;
    }

    //GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(self->mGstPipeline), GST_DEBUG_GRAPH_SHOW_ALL, "impipeline1");
}

void FarstreamChannel::onContentRemoved(TfChannel *tfc, TfContent * content, FarstreamChannel *self)
{
    Q_UNUSED(tfc);
    Q_UNUSED(content);
    Q_UNUSED(self);

    LIFETIME_TRACER();

    if (!self || !self->mGstPipeline) {
        self->setError("GStreamer pipeline not setup");
        return;
    }

    if (!content) {
        self->setError("Invalid content received");
        return;
    }

    guint media_type;
    GstPad *sink;
    g_object_get(content, "media-type", &media_type, "sink-pad", &sink, NULL);
    if (!sink) {
        self->setError("GStreamer cannot get sink pad from content");
        return;
    }
    qDebug() << "FarstreamChannel::onContentRemoved: content=" << content << " type=" << media_type;

    TRACE();
    GstElement *sourceElement = 0;
    if (media_type == TP_MEDIA_STREAM_TYPE_AUDIO) {
        qDebug() << "Got audio sink";
        if (!self->mGstAudioInput) {
            qDebug() << "Audio input is not initialized";
        }
        sourceElement = self->mGstAudioInput;
    } else if (media_type == TP_MEDIA_STREAM_TYPE_VIDEO) {
        qDebug() << "Got video sink";
        if (!self->mGstVideoInput) {
            qDebug() << "Video input is not initialized";
        }
        sourceElement = self->mGstVideoInput;
    } else {
        Q_ASSERT(false);
    }

    TRACE();
    if (!sourceElement) {
        self->setError("GStreamer source element not found");
        return;
    }

    TRACE();
    GstPad *pad = gst_element_get_static_pad(sourceElement, "src");
    if (!pad) {
        self->setError("GStreamer get source element source pad failed");
        return;
    }

    TRACE();
    bool resUnlink = gst_pad_unlink(pad, sink);
    if (!resUnlink) {
        self->setError("GStreamer could not unlink input source pad from sink");
        return;
    }

    TRACE();
    GstStateChangeReturn resState = gst_element_set_state(sourceElement, GST_STATE_NULL);
    if (resState == GST_STATE_CHANGE_FAILURE) {
        self->setError("GStreamer input could not be set to null");
        return;
    }

    TRACE();
    gboolean res = gst_bin_remove(GST_BIN(self->mGstPipeline), sourceElement);
    if (!res) {
        self->setError("GStreamer could not remove source from the pipeline");
        return;
    }
    TRACE();

}

bool FarstreamChannel::onStartSending(TfContent *tfc, FarstreamChannel *self)
{
    Q_UNUSED(tfc);
    Q_UNUSED(self);

    qDebug() << "FarstreamChannel::onStartSending: ";

    return true;
}

void FarstreamChannel::onStopSending(TfContent *tfc, FarstreamChannel *self)
{
    Q_UNUSED(tfc);
    Q_UNUSED(self);

    qDebug() << "FarstreamChannel::onStopSending: ";

}

void FarstreamChannel::onSrcPadAddedContent(TfContent *content, uint handle, FsStream *stream, GstPad *src, FsCodec *codec, FarstreamChannel *self)
{
    Q_UNUSED(content);
    Q_UNUSED(handle);
    Q_UNUSED(codec);
    Q_ASSERT(stream);

    LIFETIME_TRACER();

    guint media_type;
    g_object_get(content, "media-type", &media_type, NULL);

    qDebug() << "FarstreamChannel::onSrcPadAddedContent: stream=" << stream << " type=" << media_type << "pad = " << src;

    GstElement *bin = 0;
    GstElement *sinkElement = 0;
    GstPad *pad = 0;
    gboolean res = FALSE;
    GstPad *sinkPad = NULL;
    GstStateChangeReturn ret;

    switch (media_type) {
    case TP_MEDIA_STREAM_TYPE_AUDIO:
        if (self->mGstAudioOutput) {
          qDebug() << "Audio output already exists, relinking only";
          pad = gst_element_get_static_pad(self->mGstAudioOutput, SINK_GHOST_PAD_NAME);
          if (!gst_pad_unlink (gst_pad_get_peer(pad), pad)) {
            qWarning() << "Ghost pad was not linked, but audio output bin existed";
          }
          bin = self->mGstAudioOutput;
          goto link_only;
        }
        self->initAudioOutput();
        bin = self->mGstAudioOutput;
        sinkElement = self->mGstAudioOutputSink;
        sinkPad = gst_element_get_static_pad(sinkElement, "sink");
        break;
    case TP_MEDIA_STREAM_TYPE_VIDEO:
        if (self->mGstVideoOutput) {
          qDebug() << "Video output already exists, relinking only";
          gst_element_set_state (self->mGstVideoOutput, GST_STATE_READY);
          pad = gst_element_get_static_pad(self->mGstVideoOutput, SINK_GHOST_PAD_NAME);
          if (!gst_pad_unlink (gst_pad_get_peer(pad), pad)) {
            qWarning() << "Ghost pad was not linked, but video output bin existed";
          }
          bin = self->mGstVideoOutput;
          goto link_only;
        }
        self->initIncomingVideoWidget();
        self->initVideoOutput();
        bin = self->mGstVideoOutput;
        sinkElement = self->mGstVideoOutputSink;
        sinkPad = gst_element_get_request_pad(sinkElement, "sink%d");
        break;

    default:
        Q_ASSERT(false);
    }

    if (!bin) {
        //tf_content_error(content, TF_FUTURE_CONTENT_REMOVAL_REASON_ERROR, "", "Could not link sink");
        self->setError("GStreamer output element bin not found");
        return;
    }

    res = gst_bin_add(GST_BIN(self->mGstPipeline), bin);
    if (!res) {
        self->setError("GStreamer audio output output element could not be added to the pipeline bin");
        // maybe was already added
    } else {
        gst_object_ref(bin);
    }

    if (!sinkElement) {
        //tf_content_error(content, TP_MEDIA_STREAM_ERROR_MEDIA_ERROR, "Could not link sink");
        self->setError("GStreamer output sink element not found");
        return;
    }

    if (!sinkPad) {
        //tf_content_error(content, TP_MEDIA_STREAM_ERROR_MEDIA_ERROR, "Could not link sink");
        self->setError("GStreamer sink pad request failed");
        return;
    }

    qDebug() << "Creating ghost pad named " << SINK_GHOST_PAD_NAME << " for bin " << gst_element_get_name(bin);
    pad = gst_ghost_pad_new(SINK_GHOST_PAD_NAME, sinkPad);
    if (!pad) {
        //tf_content_error(content, TP_MEDIA_STREAM_ERROR_MEDIA_ERROR, "Could not link sink");
        self->setError("GStreamer ghost sink pad failed");
        return;
    }

    res = gst_element_add_pad(GST_ELEMENT(bin), pad);
    if (!res) {
        //tf_content_error(content, TP_MEDIA_STREAM_ERROR_MEDIA_ERROR, "Could not link sink");
        self->setError("GStreamer add video sink ghost pad failed");
        return;
    }

    gst_object_unref(sinkPad);

    ret = gst_element_set_state(bin, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        //tf_content_error(content, TP_MEDIA_STREAM_ERROR_MEDIA_ERROR, "Could not link sink");
        self->setError("GStreamer output element cannot be played");
        return;
    }

link_only:
    GstPadLinkReturn resLink = gst_pad_link(src, pad);
    if (resLink != GST_PAD_LINK_OK && resLink != GST_PAD_LINK_WAS_LINKED) {
        //tf_content_error(content, TP_MEDIA_STREAM_ERROR_MEDIA_ERROR, "Could not link sink");
        self->setError("GStreamer could not link sink pad to source");
        return;
    }

    gst_element_set_state (bin, GST_STATE_PLAYING);

    self->setState(Tp::MediaStreamStateConnected);

    if (media_type == TP_MEDIA_STREAM_TYPE_VIDEO) {
        emit self->remoteVideoRender(true);
    }

    // todo If no sink could be linked, try to add fakesink to prevent the whole call

    //GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(self->mGstPipeline), GST_DEBUG_GRAPH_SHOW_ALL, "impipeline2");
}

