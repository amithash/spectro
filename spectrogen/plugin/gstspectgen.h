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

#ifndef __GST_SPECTGEN_H__
#define __GST_SPECTGEN_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_SPECTGEN \
  (gst_spectgen_get_type())
#define GST_SPECTGEN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SPECTGEN,GstSpectgen))
#define GST_SPECTGEN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SPECTGEN,GstSpectgenClass))

/*
 * NBANDS must evenly divide 24 thus it can take the values:
 * 24, 12, 8, 6, 4, 3, 2, 1
 * default spectgen is with NBANDS = 3
 */

#define NBANDS 24

typedef struct _GstSpectgen      GstSpectgen;
typedef struct _GstSpectgenClass GstSpectgenClass;

struct _GstSpectgen
{
  GstElement element;

  GstPad *sinkpad, *srcpad;

  /* Stream data */
  gint rate, size;
  
  /* Cached band -> bark band table */
  guint *barkband_table;

  /* Queued spectgen data */
  gfloat *bands[NBANDS];
  guint numframes;

  /* Property */
  guint height;
  guint max_width;
};

struct _GstSpectgenClass 
{
  GstElementClass parent_class;
};

GType gst_spectgen_get_type (void);

G_END_DECLS

#endif /* __GST_SPECTGEN_H__ */
