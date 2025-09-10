#include <gst/gst.h>
#include <stdio.h> 
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <errno.h>

#define SAMPLE_RATE 48000
#define MIXCAP_CHANNELS 2
#define MIC_CHANNELS 1

typedef struct {
    GstElement *pipeline;
    GstElement *volume_mix_to_hw;
    GstElement *volume_mic_to_mix;
    GMainLoop  *loop;
    gboolean speaking;
    GMainContext *context;
} AppData;

static void set_volumes(AppData *app, gdouble mic_vol, gdouble mix_vol)
{
    g_object_set(app->volume_mic_to_mix, "volume", mic_vol, NULL);
    g_object_set(app->volume_mix_to_hw, "volume", mix_vol, NULL);
}

/* Function called in the main thread via g_idle_add() */
static gboolean ptt_update(gpointer data)
{
    AppData *app = (AppData*)data;

    if (app->speaking) {
        g_print("PTT pressed\n");
        set_volumes(app, 1.0, 0.1);  // microphone activation, mixcapture reduction
    } else {
        g_print("PTT released\n");
        set_volumes(app, 0.0, 1.0);  // microphone cut, mixcapture restoration
    }
    return G_SOURCE_REMOVE;
}

/* Thread that reads input events */
static void *ptt_thread(void *userdata)
{
    AppData *app = (AppData*)userdata;
    int fd = open("/dev/input/event0", O_RDONLY);
    if (fd < 0) {
        perror("open event0");
        return NULL;
    }

    struct input_event ev;
    while (1) {
        ssize_t n = read(fd, &ev, sizeof(ev));
        if (n == (ssize_t)sizeof(ev)) {
            if (ev.type == EV_KEY) {
                if (ev.value == 1) { // pressed
                    app->speaking = TRUE;
                    g_idle_add(ptt_update, app);
                } else if (ev.value == 0) { // released
                    app->speaking = FALSE;
                    g_idle_add(ptt_update, app);
                }
            }
        } else if (n < 0 && errno != EINTR) {
            perror("read event0");
            break;
        }
    }

    close(fd);
    return NULL;
}

int main(int argc, char *argv[])
{
    gst_init(&argc, &argv);

    AppData app;
    memset(&app, 0, sizeof(app));
    app.speaking = FALSE;

    app.loop = g_main_loop_new(NULL, FALSE);
    app.context = g_main_loop_get_context(app.loop);

    app.pipeline = gst_pipeline_new("ptt-pipeline");

    /* mixcapture -> hw */
    GstElement *mix_src = gst_element_factory_make("alsasrc", "mix_src");
    GstElement *mix_q = gst_element_factory_make("queue", "mix_q");
    GstElement *mix_conv = gst_element_factory_make("audioconvert", "mix_conv");
    GstElement *mix_resample = gst_element_factory_make("audioresample", "mix_resample");
    GstElement *mix_caps = gst_element_factory_make("capsfilter", "mix_caps");
    app.volume_mix_to_hw = gst_element_factory_make("volume", "volume_mix_to_hw");
    GstElement *hw_sink = gst_element_factory_make("alsasink", "hw_sink");

    /* mic -> mixplayback */
    GstElement *mic_src = gst_element_factory_make("alsasrc", "mic_src");
    GstElement *mic_q = gst_element_factory_make("queue", "mic_q");
    GstElement *mic_conv = gst_element_factory_make("audioconvert", "mic_conv");
    GstElement *mic_resample = gst_element_factory_make("audioresample", "mic_resample");
    GstElement *mic_caps = gst_element_factory_make("capsfilter", "mic_caps");
    app.volume_mic_to_mix = gst_element_factory_make("volume", "volume_mic_to_mix");
    GstElement *mic_up_conv = gst_element_factory_make("audioconvert", "mic_up_conv");
    GstElement *mic_up_resample = gst_element_factory_make("audioresample", "mic_up_resample");
    GstElement *mic_up_caps = gst_element_factory_make("capsfilter", "mic_up_caps");
    GstElement *mixplayback_sink = gst_element_factory_make("alsasink", "mixplayback_sink");

    if (!app.pipeline || !mix_src || !app.volume_mix_to_hw || !hw_sink ||
        !mic_src || !app.volume_mic_to_mix || !mixplayback_sink) {
        g_printerr("Missing elements\n");
        return -1;
    }

    /* devices */
    g_object_set(mix_src, "device", "mixcapture", NULL);
    g_object_set(mic_src, "device", "hw:1,0", NULL);
    g_object_set(hw_sink,
             "device", "hw:1,0",
             "sync", FALSE,
             "buffer-time", (gint64)40000,
             "latency-time", (gint64)20000,
             NULL);
    g_object_set(mixplayback_sink, 
            "device", "mixplayback", 
            "sync", FALSE,
            "buffer-time", (gint64)40000,
            "latency-time", (gint64)20000,
            NULL);


    /* caps */
    GstCaps *caps_mix = gst_caps_new_simple("audio/x-raw",
                                            "rate", G_TYPE_INT, SAMPLE_RATE,
                                            "channels", G_TYPE_INT, MIXCAP_CHANNELS,
                                            NULL);
    g_object_set(mix_caps, "caps", caps_mix, NULL);
    gst_caps_unref(caps_mix);

    GstCaps *caps_mic = gst_caps_new_simple("audio/x-raw",
                                            "rate", G_TYPE_INT, SAMPLE_RATE,
                                            "channels", G_TYPE_INT, MIC_CHANNELS,
                                            NULL);
    g_object_set(mic_caps, "caps", caps_mic, NULL);
    gst_caps_unref(caps_mic);

    GstCaps *caps_up = gst_caps_new_simple("audio/x-raw",
                                           "rate", G_TYPE_INT, SAMPLE_RATE,
                                           "channels", G_TYPE_INT, MIXCAP_CHANNELS,
                                           NULL);
    g_object_set(mic_up_caps, "caps", caps_up, NULL);
    gst_caps_unref(caps_up);

    /* init volumes */
    g_object_set(app.volume_mix_to_hw, "volume", 1.0, NULL);
    g_object_set(app.volume_mic_to_mix, "volume", 0.0, NULL);

    gst_bin_add_many(GST_BIN(app.pipeline),
                     mix_src, mix_q, mix_conv, mix_resample, mix_caps, app.volume_mix_to_hw, hw_sink,
                     mic_src, mic_q, mic_conv, mic_resample, mic_caps, app.volume_mic_to_mix,
                     mic_up_conv, mic_up_resample, mic_up_caps, mixplayback_sink,
                     NULL);

    if (!gst_element_link_many(mix_src, mix_q, mix_conv, mix_resample, mix_caps, app.volume_mix_to_hw, hw_sink, NULL)) {
        g_printerr("Failed to link mix->hw\n");
        return -1;
    }

    if (!gst_element_link_many(mic_src, mic_q, mic_conv, mic_resample, mic_caps, app.volume_mic_to_mix, mic_up_conv, mic_up_resample, mic_up_caps, mixplayback_sink, NULL)) {
        g_printerr("Failed to link mic->mixplayback\n");
        return -1;
    }

    gst_element_set_state(app.pipeline, GST_STATE_PLAYING);
    g_print("Pipeline started (PTT mode).\n");

    /* starts PTT thread */
    pthread_t tid;
    if (pthread_create(&tid, NULL, ptt_thread, &app) != 0) {
        perror("pthread_create");
        return -1;
    }

    g_main_loop_run(app.loop);

    gst_element_set_state(app.pipeline, GST_STATE_NULL);
    gst_object_unref(app.pipeline);
    g_main_loop_unref(app.loop);

    return 0;
}
