#include <stdio.h>
#include <gst/gst.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline;
    GstStateChangeReturn ret;

    gst_init(&argc, &argv);

    // gst-launch-1.0 audiotestsrc freq=1000 wave=sine ! audio/x-raw,format=S16LE,channels=1,rate=48000 ! audioconvert ! audioresample ! alsasink device=hw:1
    const char *pipeline_desc =
        "audiotestsrc freq=1000 wave=sine ! "
        "audio/x-raw,format=S16LE,channels=1,rate=48000 ! "
        "audioconvert ! audioresample ! "
        "alsasink device=hw:1";

    pipeline = gst_parse_launch(pipeline_desc, NULL);
    if (!pipeline) {
        printf("[ERROR] Unable to create the pipeline\n");
        return -1;
    }
    printf("[DEBUG] Pipeline created : %s\n", pipeline_desc);

    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        printf("[ERROR] Unable to switch the pipeline to PLAYING\n");
        gst_object_unref(pipeline);
        return -1;
    }
    printf("[DEBUG] Pipeline in PLAYING\n");

    // Let it run for a few seconds
    g_usleep(5 * G_USEC_PER_SEC);
    printf("[DEBUG] Reading completed\n");

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    printf("[DEBUG] Pipeline destroyed, application terminated\n");

    return 0;
}

