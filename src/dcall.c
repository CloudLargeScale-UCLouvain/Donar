// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include <gst/gst.h>
#include <glib-2.0/glib.h>
#include <glib-2.0/gmodule.h>
#include <glib-2.0/glib-object.h>
#include <glib-2.0/glib-unix.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

struct dcall_elements {
  GstElement *pipeline;
  GstElement *rx_tap, *rx_jitterbuffer, *rx_depay, *rx_opusdec, *rx_resample, *rx_echocancel, *rx_pulse, *rx_fakesink;
  GstElement *tx_pulse, *tx_filesrc, *tx_mpegaudioparse, *tx_mpgaudiodec, *tx_audioconvert, *tx_echocancel, *tx_queue, *tx_resample, *tx_opusenc, *tx_pay, *tx_sink;
  char *local_host, *remote_host, *audio_tap, *audio_sink, *audio_file, *gstreamer_log_path;
  int remote_port, local_port, latency;
  guint64 grtppktlost;
  GMainLoop *loop;
  char* buffering_mode;
  gboolean droplat;
};

int create_rx_chain(struct dcall_elements* de) {
  de->rx_tap = gst_element_factory_make("udpsrc", "rx-tap");
  de->rx_jitterbuffer = gst_element_factory_make("rtpjitterbuffer", "rx-jitterbuffer");
  de->rx_depay = gst_element_factory_make("rtpopusdepay", "rx-depay");
  de->rx_opusdec = gst_element_factory_make("opusdec", "rx-opusdec");
  de->rx_resample = gst_element_factory_make("audioresample", "rx-audioresample");
  de->rx_echocancel = gst_element_factory_make("webrtcechoprobe", "rx-echocancel");
  de->rx_pulse = gst_element_factory_make("pulsesink", "rx-pulse");
  de->rx_fakesink = gst_element_factory_make("fakesink", "rx-fakesink");

  if (!de->rx_tap || !de->rx_jitterbuffer || !de->rx_depay || !de->rx_opusdec || !de->rx_resample || !de->rx_echocancel || !de->rx_pulse || !de->rx_fakesink) {
    g_printerr ("One element of the rx chain could not be created. Exiting.\n");
    return -1;
  }

  g_object_set(G_OBJECT (de->rx_tap), "port", de->local_port, NULL);
  g_object_set(G_OBJECT (de->rx_tap), "address", de->local_host, NULL);
  g_object_set(G_OBJECT (de->rx_tap), "caps", gst_caps_new_simple("application/x-rtp", "media", G_TYPE_STRING, "audio", NULL), NULL);

  g_object_set(G_OBJECT (de->rx_jitterbuffer), "do-lost", TRUE, NULL);
  g_object_set(G_OBJECT (de->rx_jitterbuffer), "do-retransmission", FALSE, NULL);
  g_object_set(G_OBJECT (de->rx_jitterbuffer), "latency", de->latency, NULL);
  g_object_set(G_OBJECT (de->rx_jitterbuffer), "drop-on-latency", de->droplat, NULL);
  gst_util_set_object_arg(G_OBJECT(de->rx_jitterbuffer), "mode", de->buffering_mode);

  g_object_set(G_OBJECT (de->rx_opusdec), "plc", TRUE, NULL);
  g_object_set(G_OBJECT (de->rx_opusdec), "use-inband-fec", FALSE, NULL);


  GstStructure *props;
  props = gst_structure_from_string ("props,media.role=phone", NULL);
  g_object_set (de->rx_pulse, "stream-properties", props, NULL);
  gst_structure_free (props);

  if (strcmp(de->audio_sink, "pulsesink") == 0) {
    gst_bin_add_many (GST_BIN (de->pipeline), de->rx_tap, de->rx_jitterbuffer, de->rx_depay, de->rx_opusdec, de->rx_resample, de->rx_echocancel, de->rx_pulse, NULL);
    gst_element_link_many (de->rx_tap, de->rx_jitterbuffer, de->rx_depay, de->rx_opusdec, de->rx_resample, de->rx_echocancel, de->rx_pulse, NULL);
  } else if (strcmp(de->audio_sink, "fakesink") == 0) {
    gst_bin_add_many (GST_BIN (de->pipeline), de->rx_tap, de->rx_jitterbuffer, de->rx_depay, de->rx_opusdec, de->rx_resample, de->rx_echocancel, de->rx_fakesink, NULL);
    gst_element_link_many (de->rx_tap, de->rx_jitterbuffer, de->rx_depay, de->rx_opusdec, de->rx_resample, de->rx_echocancel, de->rx_fakesink, NULL);
  } else {
    fprintf(stderr, "Wrong audio sink %s, exiting...\n", de->audio_sink);
    exit(EXIT_FAILURE);
  }

  return 0;
}

static void uridecodebin_newpad (GstElement *src, GstPad *new_pad, gpointer data) {
  struct dcall_elements *de = data;
  GstPad *sink_pad = gst_element_get_static_pad (de->tx_audioconvert, "sink");
  GstPadLinkReturn ret;
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_struct = NULL;
  const gchar *new_pad_type = NULL;

  g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

  /* If our converter is already linked, we have nothing to do here */
  if (gst_pad_is_linked (sink_pad)) {
    g_print ("We are already linked. Ignoring.\n");
    goto exit;
  }

  /* Check the new pad's type */
  new_pad_caps = gst_pad_get_current_caps (new_pad);
  new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
  new_pad_type = gst_structure_get_name (new_pad_struct);
  if (!g_str_has_prefix (new_pad_type, "audio/x-raw")) {
    g_print ("It has type '%s' which is not raw audio. Ignoring.\n", new_pad_type);
    goto exit;
  }

  /* Attempt the link */
  ret = gst_pad_link (new_pad, sink_pad);
  if (GST_PAD_LINK_FAILED (ret)) {
    g_print ("Type is '%s' but link failed.\n", new_pad_type);
  } else {
    g_print ("Link succeeded (type '%s').\n", new_pad_type);
  }

exit:
  /* Unreference the new pad's caps, if we got them */
  if (new_pad_caps != NULL)
    gst_caps_unref (new_pad_caps);

  /* Unreference the sink pad */
  gst_object_unref (sink_pad);
}

static void uridecodebin_drained (GstElement *src, gpointer data) {
  struct dcall_elements *de = data;
  g_main_loop_quit (de->loop);
}

int create_tx_chain(struct dcall_elements* de) {
  de->tx_pulse = gst_element_factory_make("pulsesrc", "tx-pulse");
  de->tx_filesrc = gst_element_factory_make("uridecodebin", "tx-filesrc");
  de->tx_mpegaudioparse = gst_element_factory_make("mpegaudioparse", "tx-mpegaudioparse");
  de->tx_mpgaudiodec = gst_element_factory_make("mpg123audiodec", "tx-mpgaudiodec");
  de->tx_audioconvert = gst_element_factory_make("audioconvert", "tx-audioconvert");
  de->tx_resample = gst_element_factory_make("audioresample", "tx-resample");
  de->tx_echocancel = gst_element_factory_make("webrtcdsp", "tx-echocancel");
  de->tx_queue = gst_element_factory_make("queue", "tx-queue");
  de->tx_opusenc = gst_element_factory_make("opusenc", "tx-opusenc");
  de->tx_pay = gst_element_factory_make("rtpopuspay", "tx-rtpopuspay");
  de->tx_sink = gst_element_factory_make("udpsink", "tx-sink");

  if (!de->tx_pulse || !de->tx_filesrc || !de->tx_mpegaudioparse || !de->tx_mpgaudiodec || !de->tx_audioconvert || !de->tx_echocancel || !de->tx_queue || !de->tx_resample || !de->tx_opusenc || !de->tx_pay || !de->tx_sink) {
    g_printerr("One element of the tx chain could not be created. Exiting.\n");
    return -1;
  }

  gst_util_set_object_arg(G_OBJECT(de->tx_opusenc), "audio-type", "voice");
  g_object_set(G_OBJECT(de->tx_opusenc), "inband-fec", FALSE, NULL);
  g_object_set(G_OBJECT(de->tx_opusenc), "frame-size", 40, NULL);
  g_object_set(G_OBJECT(de->tx_opusenc), "bitrate", 32000, NULL);
  g_object_set(G_OBJECT(de->tx_opusenc), "dtx", FALSE, NULL); // gstreamer dtx opus implem. is broken

  g_object_set(G_OBJECT(de->tx_sink), "host", de->remote_host, NULL);
  g_object_set(G_OBJECT(de->tx_sink), "port", de->remote_port, NULL);
  g_object_set(G_OBJECT(de->tx_sink), "async", FALSE, NULL);

  g_object_set(G_OBJECT(de->tx_echocancel), "echo-cancel", TRUE, NULL);
  g_object_set(G_OBJECT(de->tx_echocancel), "extended-filter", TRUE, NULL);
  g_object_set(G_OBJECT(de->tx_echocancel), "gain-control", TRUE, NULL);
  g_object_set(G_OBJECT(de->tx_echocancel), "high-pass-filter", TRUE, NULL);
  g_object_set(G_OBJECT(de->tx_echocancel), "limiter", FALSE, NULL);
  g_object_set(G_OBJECT(de->tx_echocancel), "noise-suppression", TRUE, NULL);
  g_object_set(G_OBJECT(de->tx_echocancel), "probe", "rx-echocancel", NULL);
  g_object_set(G_OBJECT(de->tx_echocancel), "voice-detection", FALSE, NULL);

  GstStructure *props;
  props = gst_structure_from_string ("props,media.role=phone", NULL);
  g_object_set (de->tx_pulse, "stream-properties", props, NULL);
  gst_structure_free (props);

  g_object_set(de->tx_filesrc, "uri", de->audio_file, NULL);

  if (strcmp(de->audio_tap, "pulsesrc") == 0) {
    gst_bin_add_many(GST_BIN(de->pipeline), de->tx_pulse, de->tx_echocancel, de->tx_queue, de->tx_resample, de->tx_opusenc, de->tx_pay, de->tx_sink, NULL);
    gst_element_link_many(de->tx_pulse, de->tx_resample, de->tx_echocancel, de->tx_queue, de->tx_opusenc, de->tx_pay, de->tx_sink, NULL);
  } else if (strcmp(de->audio_tap, "filesrc") == 0) {
    gst_bin_add_many(GST_BIN(de->pipeline), de->tx_filesrc, de->tx_mpegaudioparse, de->tx_mpgaudiodec, de->tx_audioconvert, de->tx_queue, de->tx_resample, de->tx_opusenc, de->tx_pay, de->tx_sink, NULL);
    gst_element_link_many(de->tx_audioconvert, de->tx_resample, de->tx_queue, de->tx_opusenc, de->tx_pay, de->tx_sink, NULL);
    g_signal_connect (de->tx_filesrc, "pad-added", G_CALLBACK (uridecodebin_newpad), de);
    g_signal_connect (de->tx_filesrc, "drained", G_CALLBACK(uridecodebin_drained), de);
  } else {
    fprintf(stderr, "Wrong audio tap %s, exiting...\n", de->audio_tap);
    exit(EXIT_FAILURE);
  }

  return 0;
}

static GstPadProbeReturn jitter_buffer_sink_event(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
  struct dcall_elements *de = user_data;
  GstEvent *event = NULL;

  //g_print("Entering rtpjitterbuffer sink pad handler for events...\n");

  event = gst_pad_probe_info_get_event (info);
  if (event == NULL) return GST_PAD_PROBE_OK;
  //g_print("We successfully extracted an event from the pad... \n");

  const GstStructure *struc = NULL;
  struc = gst_event_get_structure(event);
  if (struc == NULL) return GST_PAD_PROBE_OK;
  //g_print("We successfully extracted a structure from the event... \n");

  const gchar* struc_name = NULL;
  struc_name = gst_structure_get_name(struc);
  if (struc_name == NULL) return GST_PAD_PROBE_OK;
  //g_print("We extracted the structure \"%s\"...\n", struc_name);

  if (strcmp(struc_name, "GstRTPPacketLost") != 0) return GST_PAD_PROBE_OK;
  //g_print("And that's the structure we want !\n");

  guint seqnum = 0, retry = 0;
  guint64 timestamp = 0, duration = 0;
  gst_structure_get_uint(struc, "seqnum", &seqnum);
  gst_structure_get_uint(struc, "retry", &retry);
  gst_structure_get_uint64(struc, "timestamp", &timestamp);
  gst_structure_get_uint64(struc, "duration", &duration);
  g_print("GstRTPPacketLost{seqnum=%d, retry=%d, duration=%ld, timestamp=%ld}\n", seqnum, retry, duration, timestamp);
  de->grtppktlost++;

  return GST_PAD_PROBE_OK;
}

gboolean stop_handler(gpointer user_data) {
  GMainLoop *loop = user_data;
  g_main_loop_quit(loop);
  return TRUE;
}

int main(int argc, char *argv[]) {

  GstBus *bus;
  struct dcall_elements de = {
    .audio_file = "file://./voice.mp3",
    .gstreamer_log_path = "dcall.log",
    .latency = 150,
    .remote_host = "127.13.3.7",
    .remote_port = 5000,
    .local_host = "0.0.0.0",
    .local_port = 5000,
    .audio_sink = "pulsesink",
    .audio_tap = "pulsesrc",
    .grtppktlost = 0,
    .droplat = TRUE,
    .buffering_mode = "slave"
  };
  int opt = 0;

  while ((opt = getopt(argc, argv, "t:s:r:p:l:d:a:hb:c:m:o")) != -1) {
    switch(opt) {
    case 'a':
      de.audio_file = optarg;
      break;
    case 'b':
      de.local_host = optarg;
      break;
    case 'c':
      de.local_port = atoi(optarg);
      break;
    case 'd':
      de.gstreamer_log_path = optarg;
      break;
    case 'l':
      de.latency = atoi(optarg);
      break;
    case 'p':
      de.remote_port = atoi(optarg);
      break;
    case 'o':
      de.droplat = TRUE;
      break;
    case 'm':
      de.buffering_mode = optarg;
    case 'r':
      de.remote_host = optarg;
      break;
    case 's':
      de.audio_sink = optarg;
      break;
    case 't':
      de.audio_tap = optarg;
      break;
    case 'h':
    default:
      g_print("Usage: %s [-a <audio file>] [-d <gstreamer debug file>] [-l <jitter buffer latency in ms>] [-r <remote host>] [-p <remote port>] [-t <audio tap>] [-s <audio sink>] [-h]\n", argv[0]);
      exit(EXIT_SUCCESS);
      break;
    }
  }
  printf("dcall configuration:\n\tnetwork in: %s:%d, out: %s:%d\n\taudio in: %s, out: %s\n\tmisc latency: %dms, audio_file: %s\n",
         de.local_host, de.local_port, de.remote_host, de.remote_port, de.audio_tap, de.audio_sink, de.latency, de.audio_file);

  setenv("GST_DEBUG_FILE", de.gstreamer_log_path, 0);
  setenv("GST_DEBUG", "3,opusdec:5", 0);

  gst_init (&argc, &argv);
  de.loop = g_main_loop_new (NULL, FALSE);

  de.pipeline = gst_pipeline_new ("pipeline");
  if (!de.pipeline) {
    g_printerr ("Pipeline could not be created. Exiting.\n");
    return -1;
  }

  if (create_rx_chain (&de) != 0) return -1;
  if (create_tx_chain (&de) != 0) return -1;

  gst_element_set_state (de.pipeline, GST_STATE_PLAYING);

  g_unix_signal_add (SIGTERM, stop_handler, de.loop);
  g_unix_signal_add (SIGINT, stop_handler, de.loop);

  g_print ("Running...\n");
  g_main_loop_run (de.loop);

  g_print ("Main loop stopped...\n");

  GstStructure *stats;
  guint64 num_pushed, num_lost, num_late, num_duplicates;
  g_object_get(de.rx_jitterbuffer, "stats", &stats, NULL);

  gst_structure_get_uint64(stats, "num-pushed", &num_pushed);
  gst_structure_get_uint64(stats, "num-lost", &num_lost);
  gst_structure_get_uint64(stats, "num-late", &num_late);
  gst_structure_get_uint64(stats, "num-duplicates", &num_duplicates);
  g_print("pkt_delivered=%ld, pkt_lost=%ld, pkt_late=%ld, pkt_duplicates=%ld\n", num_pushed, num_lost, num_late, num_duplicates);

  gst_element_set_state (de.pipeline, GST_STATE_NULL);

  gst_object_unref (GST_OBJECT (de.pipeline));
  g_main_loop_unref (de.loop);

  return 0;
}
