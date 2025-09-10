#include <gst/gst.h>
#include <string.h>

#define SAMPLE_RATE 48000
#define MIXCAP_CHANNELS 2
#define MIC_CHANNELS 1

#define SPEECH_ON_THRESHOLD -30
#define SPEECH_OFF_THRESHOLD -30

typedef struct {
    GstElement *pipeline;
    GstElement *volume_mix_to_hw;
    GstElement *volume_mic_to_mix;
    gboolean speaking;
} AppData;

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer user_data)
{
    AppData *app = (AppData*)user_data;

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_main_loop_quit((GMainLoop*)g_object_get_data(G_OBJECT(bus), "mainloop"));
            break;

        case GST_MESSAGE_ERROR: {
            GError *err = NULL;
            gchar *dbg = NULL;
            gst_message_parse_error(msg, &err, &dbg);
            g_printerr("ERROR: %s\nDebug: %s\n", err->message, dbg ? dbg : "none");
            g_clear_error(&err);
            g_free(dbg);
            g_main_loop_quit((GMainLoop*)g_object_get_data(G_OBJECT(bus), "mainloop"));
            break;
        }

        case GST_MESSAGE_ELEMENT: {
            const GstStructure *s = gst_message_get_structure(msg);
            if (gst_structure_has_name(s, "level")) {
                const GValue *array_val = gst_structure_get_value(s, "peak");
                if (array_val) {
                    GValueArray *peak_arr = (GValueArray *) g_value_get_boxed(array_val);
                    if (peak_arr && peak_arr->n_values > 0) {
                        gdouble peak_db = g_value_get_double(&peak_arr->values[0]);
                        g_print("Peak: %.2f dB\n", peak_db);
                        if (!app->speaking && peak_db > SPEECH_ON_THRESHOLD) {
                            app->speaking = TRUE;
                            g_print("Speech ON (peak=%f)\n", peak_db);
                            g_object_set(app->volume_mic_to_mix, "volume", 3.0, NULL);
                            g_object_set(app->volume_mix_to_hw, "volume", 0.001, NULL); /* duck */
                        } else if (app->speaking && peak_db < SPEECH_OFF_THRESHOLD) {
                            app->speaking = FALSE;
                            g_print("Speech OFF (peak=%f)\n", peak_db);
                            g_object_set(app->volume_mic_to_mix, "volume", 0.0, NULL);
                            g_object_set(app->volume_mix_to_hw, "volume", 1.0, NULL);
                        }
                    }
                }
            }
        }

        default:
            break;
    }

    return TRUE;
}

int main(int argc, char *argv[])
{
    gst_init(&argc, &argv);

    AppData app;
    memset(&app, 0, sizeof(app));
    app.speaking = FALSE;

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    app.pipeline = gst_pipeline_new("duck-pipeline");

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
    GstElement *level = gst_element_factory_make("level", "level");
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

    /* level config */
    g_object_set(level, "message", TRUE, "interval", (guint64)500000000, NULL);

    /* init volumes */
    g_object_set(app.volume_mix_to_hw, "volume", 1.0, NULL);
    g_object_set(app.volume_mic_to_mix, "volume", 0.0, NULL);

    /* add to pipeline */
    gst_bin_add_many(GST_BIN(app.pipeline),
                     mix_src, mix_q, mix_conv, mix_resample, mix_caps, app.volume_mix_to_hw, hw_sink,
                     mic_src, mic_q, mic_conv, mic_resample, mic_caps, level, app.volume_mic_to_mix,
                     mic_up_conv, mic_up_resample, mic_up_caps, mixplayback_sink,
                     NULL);

    if (!gst_element_link_many(mix_src, mix_q, mix_conv, mix_resample, mix_caps, app.volume_mix_to_hw, hw_sink, NULL)) {
        g_printerr("Failed to link mix->hw\n");
        return -1;
    }

    if (!gst_element_link_many(mic_src, mic_q, mic_conv, mic_resample, mic_caps, level, app.volume_mic_to_mix, mic_up_conv, mic_up_resample, mic_up_caps, mixplayback_sink, NULL)) {
        g_printerr("Failed to link mic->mixplayback\n");
        return -1;
    }

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(app.pipeline));
    g_object_set_data(G_OBJECT(bus), "mainloop", loop);
    gst_bus_add_watch(bus, bus_call, &app);
    gst_object_unref(bus);

    gst_element_set_state(app.pipeline, GST_STATE_PLAYING);
    g_print("Pipeline started (volume ducking).\n");

    g_main_loop_run(loop);

    gst_element_set_state(app.pipeline, GST_STATE_NULL);
    gst_object_unref(app.pipeline);
    g_main_loop_unref(loop);

    return 0;
}
