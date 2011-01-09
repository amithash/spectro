/* GStreamer spectrum analysis toy
 * Copyright (C) 2006 Joseph Rabinoff <bobqwatson@yahoo.com>
 * Some code copyright (C) 2005 Gav Wood
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/**
 * SECTION:element-spectgen
 *
 * <refsect2>
 * <title>Example launch line</title>
 * <para>
 * <programlisting>
 * gst-launch filesrc location=test.mp3 ! mad ! audioconvert ! fftwspectrum ! spectgen height=50 ! pngenc ! filesink location=test.png
 * </programlisting>
 * </para>
 * </refsect2>
 */

/* This plugin is based on the Spectgen code in Amarok version 1.4.0a,
 * written by Gav Wood.  The algorithm is basically the same as the
 * one applied there, and the normalizing code below is taken directly
 * from Gav Wood's Exscalibar package.
 */

/* This plugin takes a frequency-domain stream, does some simple 
 * analysis, and returns a string of (float) 24 bands
 * that represent the magnitude of various sections of the stream.
 * Since we have to perform some normalization, we queue up all
 * of our analysis until we get an EOS event, at which point we 
 * normalize and do the output.  If a max-width is specified, the
 * output is scaled down to the desired width if necessary.
 */

/* More precisely, the analysis performed is as follows:
 *  (1) the spectrum is broken into 24 parts, called "bark bands"
 *      (Gav's terminology), as given in bark_bands below
 *  (3) after receiving an EOS, we normalize all of the analysis
 *      done in (1) and (2) and return a stream of 24 band vectors
 *      (application/x-raw-rgb)
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include "gstspectgen.h"
#include "spectrum.h"

GST_DEBUG_CATEGORY (gst_spectgen_debug);
#define GST_CAT_DEFAULT gst_spectgen_debug

/* Filter signals and args */
enum
{
	/* FILL ME */
	LAST_SIGNAL
};

enum
{
	ARG_0,
	ARG_HEIGHT,
	ARG_MAX_WIDTH
};

static GstStaticPadTemplate sink_factory 
	= GST_STATIC_PAD_TEMPLATE ("sink",
			     GST_PAD_SINK,
			     GST_PAD_ALWAYS,
			     GST_STATIC_CAPS 
			       ( SPECTRUM_FREQ_CAPS )
			     );

static GstStaticPadTemplate src_factory 
	= GST_STATIC_PAD_TEMPLATE ("src",
			     GST_PAD_SRC,
			     GST_PAD_ALWAYS,
			     GST_STATIC_CAPS
			       ( "video/x-raw-rgb, " 
				   "bpp = (int) 24, "
				   "depth = (int) 24, "
				   "height = (int) [ 1, MAX ], "
				   "width = (int) [ 1, MAX ], "
				   "framerate = (fraction) 0/1"
			       )
			     );

GST_BOILERPLATE (GstSpectgen, gst_spectgen, GstElement,
    GST_TYPE_ELEMENT);

static void gst_spectgen_set_property (GObject *object, guint prop_id,
    const GValue *value, GParamSpec *pspec);
static void gst_spectgen_get_property (GObject *object, guint prop_id,
    GValue *value, GParamSpec *pspec);

static gboolean gst_spectgen_set_sink_caps (GstPad *pad, GstCaps *caps);
static gboolean gst_spectgen_sink_event (GstPad *pad, GstEvent *event);

static GstFlowReturn gst_spectgen_chain (GstPad *pad, GstBuffer *buf);
static GstStateChangeReturn gst_spectgen_change_state (GstElement *element,
    GstStateChange transition);

static void gst_spectgen_finish (GstSpectgen *spect);

/* This is a failsafe so we don't eat up all of a computer's memory
 * if we hit an endless stream. */
#define MAX_TRIPLES (1024*1024*5)  

#define NUMFREQS(spect) ((spect)->size/2+1)

/* Allocate spect->r, spect->g, and spect->b in chunks of this many */
#define FRAME_CHUNK 1000

/* Default height of the output image */
#define HEIGHT_DEFAULT 1

/* Default max-width of the output image, or 0 for no rescaling */
#define MAX_WIDTH_DEFAULT 0

/* We use this table to break up the incoming spectrum into segments */
static const guint bark_bands[24] 
  = { 100,  200,  300,  400,  510,  630,  770,   920, 
      1080, 1270, 1480, 1720, 2000, 2320, 2700,  3150, 
      3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500 };

const float inv_eq_loudness_coeff[NBANDS]
= { 0.823529, 0.897436, 0.921053, 0.921053, 0.921053, 0.917464, 0.900932, 0.879403,
    0.876772, 0.880981, 0.885633, 0.896203, 0.909091, 0.932678, 0.959176, 0.983516,
    1.000000, 0.974123, 0.927985, 0.864979, 0.819767, 0.818741, 0.736842, 0.628727 };



/***************************************************************/
/* GObject boilerplate stuff                                   */
/***************************************************************/


static void gst_spectgen_base_init (gpointer gclass)
{
	static GstElementDetails element_details = {
			"Spectgen analyzer",
			"Filter/Converter/Spectgen",
			"Convert a spectrum into a stream of (float) 24 bands representing its \"spect\"",
			"Joe Rabinoff <bobqwatson@yahoo.com>"
		};
	GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

	gst_element_class_add_pad_template (element_class,
		gst_static_pad_template_get (&src_factory));
	gst_element_class_add_pad_template (element_class,
		gst_static_pad_template_get (&sink_factory));
	gst_element_class_set_details (element_class, &element_details);
}

/* initialize the plugin's class */
static void gst_spectgen_class_init (GstSpectgenClass * klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;

	gobject_class->set_property = gst_spectgen_set_property;
	gobject_class->get_property = gst_spectgen_get_property;

	g_object_class_install_property (gobject_class, ARG_HEIGHT,
	g_param_spec_int ("height", "Image height", 
			  "The height of the resulting raw image",
			  1, G_MAXINT32, HEIGHT_DEFAULT, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, ARG_MAX_WIDTH,
					 g_param_spec_int ("max-width", "Image maximum width", 
                                         "The maximum width of the resulting raw image, or 0 for no rescaling",
					 0, G_MAXINT32, MAX_WIDTH_DEFAULT, G_PARAM_READWRITE));

	gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_spectgen_change_state);
}

/* initialize the new element
 * instantiate pads and add them to element
 * set functions
 * initialize structure
 */
static void gst_spectgen_init (GstSpectgen *spect, GstSpectgenClass *gclass)
{
	GstElementClass *klass = GST_ELEMENT_GET_CLASS (spect);
	int i;

	spect->sinkpad = gst_pad_new_from_template(
				gst_element_class_get_pad_template (klass, "sink"), "sink");
	gst_pad_set_setcaps_function (spect->sinkpad, 
				      GST_DEBUG_FUNCPTR (gst_spectgen_set_sink_caps));
	gst_pad_set_event_function (spect->sinkpad,
			            GST_DEBUG_FUNCPTR (gst_spectgen_sink_event));
	gst_pad_set_chain_function (spect->sinkpad, 
			            GST_DEBUG_FUNCPTR (gst_spectgen_chain));

	spect->srcpad = gst_pad_new_from_template(
		gst_element_class_get_pad_template (klass, "src"), "src");


	gst_element_add_pad (GST_ELEMENT (spect), spect->sinkpad);
	gst_element_add_pad (GST_ELEMENT (spect), spect->srcpad);

	/* These are set once the (sink) capabilities are determined */
	spect->rate = 0;
	spect->size = 0;
	spect->barkband_table = NULL;
  
	/* These are allocated when we change to PAUSED */
	for(i = 0; i < NBANDS; i++) {
  		spect->bands[i] = NULL;
	}
	spect->numframes = 0;

	/* Property */
	spect->height = HEIGHT_DEFAULT;
	spect->max_width = MAX_WIDTH_DEFAULT;
}


static void gst_spectgen_set_property (GObject *object, guint prop_id,
				       const GValue *value, GParamSpec *pspec)
{
	GstSpectgen *spect = GST_SPECTGEN (object);

	switch (prop_id) {
		case ARG_HEIGHT:
			spect->height = (guint) g_value_get_int (value);
			break;
		case ARG_MAX_WIDTH:
			spect->max_width = (guint) g_value_get_int (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}


static void gst_spectgen_get_property (GObject *object, guint prop_id,
    				       GValue *value, GParamSpec *pspec)
{
	GstSpectgen *spect = GST_SPECTGEN (object);

	switch (prop_id) {
		case ARG_HEIGHT:
			g_value_set_int (value, (int) spect->height);
			break;
		case ARG_MAX_WIDTH:
			g_value_set_int (value, (int) spect->max_width);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}


/***************************************************************/
/* Pad handling                                                */
/***************************************************************/


/* This calculates a table that caches which bark band slot each
 * incoming band is supposed to go in. */
static void calc_barkband_table (GstSpectgen *spect)
{
	guint i;
	guint barkband = 0;

	/* Avoid divide-by-zero */
	if (!spect->size  ||  !spect->rate)
		return;

	if (spect->barkband_table)
		g_free (spect->barkband_table);

	spect->barkband_table = g_malloc (NUMFREQS (spect) * sizeof (guint));
  
	for (i = 0; i < NUMFREQS (spect); ++i) {
		if (barkband < 23 &&
			(guint) GST_SPECTRUM_BAND_FREQ (i, spect->size, spect->rate) 
			>= bark_bands[barkband]) {

			barkband++;
		}

		spect->barkband_table[i] = barkband;
	}
}

/* Setting the sink caps just gets the rate and size parameters.
 * Note that we do not support upstream caps renegotiation, since
 * we could only possibly scale the height anyway.
 */

static gboolean gst_spectgen_set_sink_caps (GstPad *pad, GstCaps *caps)
{
	GstSpectgen *spect;
	GstStructure *newstruct;
	gint rate, size;
	gboolean res = FALSE;

	spect = GST_SPECTGEN (gst_pad_get_parent (pad));

	newstruct = gst_caps_get_structure (caps, 0);
	if (!gst_structure_get_int (newstruct, "rate", &rate) ||
	    !gst_structure_get_int (newstruct, "size", &size)) {
		goto out;
	}

	res = TRUE;
  
	spect->rate = rate;
	spect->size = (guint) size;
	calc_barkband_table (spect);
 
	out:
	gst_object_unref (spect);

	return res;
}


static gboolean gst_spectgen_sink_event (GstPad *pad, GstEvent *event)
{
	GstSpectgen *spect;
	gboolean res = TRUE;

	spect = GST_SPECTGEN (gst_pad_get_parent (pad));

	if (GST_EVENT_TYPE (event) == GST_EVENT_EOS)
		gst_spectgen_finish (spect);
  
	res = gst_pad_push_event (spect->srcpad, event);
	gst_object_unref (spect);

	return res;
}


/***************************************************************/
/* Actual analysis                                             */
/***************************************************************/


static GstStateChangeReturn gst_spectgen_change_state (GstElement *element, 
			GstStateChange transition)
{
	GstSpectgen *spect = GST_SPECTGEN (element);
	GstStateChangeReturn res;
	int i;

	switch (transition) {
		case GST_STATE_CHANGE_NULL_TO_READY:
			calc_barkband_table (spect);
			break;
		case GST_STATE_CHANGE_READY_TO_PAUSED:
			for(i = 0; i < NBANDS; i++) {
				spect->bands[i] = (gfloat *) g_malloc (FRAME_CHUNK * sizeof(gfloat));
			}
			spect->numframes = 0;
			break;
		case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
		default:
			break;
	}

	res = parent_class->change_state (element, transition);

	switch (transition) {
		int i;
		case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
			break;
		case GST_STATE_CHANGE_PAUSED_TO_READY:      
			for(i = 0; i < NBANDS; i++) {
				g_free(spect->bands[i]);
				spect->bands[i] = NULL;
			}
			spect->numframes = 0;
			break;
		case GST_STATE_CHANGE_READY_TO_NULL:
			g_free (spect->barkband_table);
			spect->barkband_table = NULL;
			break;
		default:
			break;
	}
	return res;
}


/* We allocate r, g, b frames in chunks of FRAME_CHUNK so we don't
 * have to realloc every time a buffer comes in.
 */
static gboolean
allocate_another_frame (GstSpectgen *spect)
{
  spect->numframes++;

  /* Failsafe */
  if (spect->numframes == MAX_TRIPLES)
    return FALSE;

  if(spect->numframes % FRAME_CHUNK == 0)
    {
	int i;
      guint size = (spect->numframes + FRAME_CHUNK) * sizeof (gfloat);

      for(i = 0; i < NBANDS; i++) {
      	spect->bands[i] = (gfloat *) g_realloc (spect->bands[i], size);
	if(spect->bands[i] == NULL) {
		return FALSE;
	}
      }
    }

  return TRUE;
}


/* This function does most of the analysis on the spectra we
 * get as input and caches them.  We actually push buffers
 * once we receive an EOS signal.
 */
static GstFlowReturn gst_spectgen_chain (GstPad *pad, GstBuffer *buf)
{
	GstSpectgen *spect = GST_SPECTGEN (gst_pad_get_parent (pad));
	guint i;
	gfloat amplitudes[NBANDS] = {0};
	gfloat *out, real, imag;
	guint numfreqs = NUMFREQS (spect);

	if (GST_BUFFER_SIZE (buf) != numfreqs * sizeof (gfloat) * 2) {
		gst_object_unref (spect);
		return GST_FLOW_ERROR;
	}

	out = (gfloat *) GST_BUFFER_DATA (buf);

	if (!allocate_another_frame (spect))
		return GST_FLOW_ERROR;

	/* Calculate total amplitudes for the different bark bands */
	for (i = 0; i < numfreqs; ++i) {
		real = out[2*i];
		imag = out[2*i + 1];
		amplitudes[spect->barkband_table[i]] += real*real + imag*imag;
	}

	for(i = 0; i < NBANDS; i++) {
		spect->bands[i][spect->numframes] = 
		    sqrt(amplitudes[i]) * inv_eq_loudness_coeff[i];
	}
	gst_buffer_unref (buf);
	gst_object_unref (spect);

	return GST_FLOW_OK;
}


/* This function normalizes all of the cached r,g,b data and 
 * finally pushes a monster buffer with all of our output.
 */
static void gst_spectgen_finish (GstSpectgen *spect)
{
	GstBuffer *buf;
	float *data;
	guint output_width;
	guint i, j;
	GstCaps *caps = gst_caps_copy (gst_pad_get_caps (spect->srcpad));
	gboolean res;

	output_width = spect->numframes;

	buf = gst_buffer_new_and_alloc 
		(output_width * NBANDS * sizeof (float));
	if (!buf)
		return;

	/* Don't set the timestamp, duration, etc. since it's irrelevant */
	GST_BUFFER_OFFSET (buf) = 0;
	data = (float *) GST_BUFFER_DATA (buf);

	for (i = 0; i < output_width; ++i) {
		for(j = 0; j < NBANDS; j++) {
			*(data++) = (float)(spect->bands[j][i]);
		}
	}

	/* Now we (finally) know the width of the image we're pushing */
	gst_caps_set_simple (caps, "width", G_TYPE_INT, output_width, NULL);
	gst_caps_set_simple (caps, "height", G_TYPE_INT, spect->height, NULL);
	res = gst_pad_set_caps (spect->srcpad, caps);
	if (res)
		gst_buffer_set_caps (buf, caps);
		gst_caps_unref (caps);
	if (!res)
		return;

	gst_pad_push (spect->srcpad, buf);
}
