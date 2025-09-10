#include <gst/gst.h>
#include <glib.h>

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    GstElement *pipeline = gst_parse_launch(
        "alsasrc device=hw:1 ! "
        "audio/x-raw,format=S16LE,channels=1,rate=48000 ! "
        "audioconvert ! level interval=1000000000 ! fakesink",
        NULL
    );

    if (!pipeline) {
        g_printerr("Error: unable to create pipeline\n");
        return -1;
    }

    GstBus *bus = gst_element_get_bus(pipeline);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    g_print("Pipeline level launched on hw:1 (1 channel, 48kHz)\n");

    while (1) {
        GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
            GST_MESSAGE_ELEMENT | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

        if (!msg) continue;

        if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ELEMENT) {
            const GstStructure *s = gst_message_get_structure(msg);
            if (gst_structure_has_name(s, "level")) {
                const GValue *array_val = gst_structure_get_value(s, "peak");
                if (array_val) {
                    GValueArray *peak_arr = (GValueArray *) g_value_get_boxed(array_val);
                    if (peak_arr && peak_arr->n_values > 0) {
                        gdouble peak_db = g_value_get_double(&peak_arr->values[0]);
                        g_print("Peak: %.2f dB\n", peak_db);
                    }
                }
            }
        } else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
            GError *err;
            gchar *dbg;
            gst_message_parse_error(msg, &err, &dbg);
            g_printerr("Error: %s\n", err->message);
            g_error_free(err);
            g_free(dbg);
            break;
        } else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS) {
            g_print("End of stream\n");
            break;
        }

        gst_message_unref(msg);
    }

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    return 0;
}
