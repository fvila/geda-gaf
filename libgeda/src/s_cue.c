/* gEDA - GPL Electronic Design Automation
 * libgeda - gEDA's library
 * Copyright (C) 1998-2000 Ales V. Hvezda
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
 */
#include <config.h>

#include <stdio.h>
#include <ctype.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifndef HAVE_VSNPRINTF
#include <stdarg.h>
#endif

#include <gtk/gtk.h>
#include <libguile.h>

#include "defines.h"
#include "struct.h"
#include "defines.h"
#include "globals.h"
#include "o_types.h"
#include "colors.h"

#include "../include/prototype.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void s_cue_postscript_fillbox(TOPLEVEL * w_current, FILE * fp, int x,
			      int y)
{
  int offset;
  int offset2;

  /* hard coded values */
  offset = CUE_BOX_SIZE;
  offset2 = offset*2;

  if (w_current->print_color) {
    f_print_set_color(fp, w_current->net_endpoint_color);
  }

  fprintf(fp, "%d %d %d %d fbox\n", 
	   offset2, offset2, x-offset, y-offset);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void s_cue_postscript_fillcircle(TOPLEVEL * w_current, FILE * fp,
                                 int x, int y, int size_flag)
{
  int offset2;

  if (!size_flag) {
    offset2 = CUE_CIRCLE_LARGE_SIZE;
  } else {
    offset2 = CUE_CIRCLE_SMALL_SIZE;
  }

  if (w_current->print_color) {
    f_print_set_color(fp, w_current->net_endpoint_color);
  }

  fprintf(fp, "newpath\n");
  fprintf(fp, "%d %d\n", x, y);
  fprintf(fp, "%d\n", offset2 / 2);
  fprintf(fp, "0 360 arc\n");
  fprintf(fp, "fill\n");
}

#ifdef HAS_LIBGDGEDA
/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void s_cue_image_fillbox(TOPLEVEL * w_current, OBJECT * object, int world_x,
			 int world_y)
{
  int endpoint_color;
  int offset, offset2;
  int x, y;

  if (w_current->image_color == TRUE) {
    endpoint_color =
      o_image_geda2gd_color(w_current->net_endpoint_color);
  } else {
    endpoint_color = image_black;
  }

  WORLDtoSCREEN(w_current, world_x, world_y, &x, &y);

  offset = SCREENabs(w_current, CUE_BOX_SIZE);
  offset2 = offset * 2;

  gdImageFilledRectangle(current_im_ptr,
                         x - offset, y - offset, x - offset + offset2,
                         y - offset + offset2, endpoint_color);

}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void s_cue_image_fillcircle(TOPLEVEL * w_current, int world_x, int world_y,
                            int size_flag)
{
  int endpoint_color;
  int offset, offset2;
  int i;
  int x, y;

  if (w_current->image_color == TRUE) {
    endpoint_color =
      o_image_geda2gd_color(w_current->net_endpoint_color);
  } else {
    endpoint_color = image_black;
  }

  WORLDtoSCREEN(w_current, world_x, world_y, &x, &y);

  /* this needs to be rewritten to be much cleaner */
  if (!size_flag) {
    offset = SCREENabs(w_current, 30); /* large size */
  } else {
    offset = SCREENabs(w_current, 10);  /* small size */
  }
  offset2 = offset * 2;

  gdImageArc(current_im_ptr, x, y,
             offset2 * 1.25, offset2 * 1.25, 0, 360, endpoint_color);

  for (i = 0; i < offset2 * 1.25; i++) {
    gdImageArc(current_im_ptr, x, y, i, i, 0, 360, endpoint_color);
  }

}
#endif

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void s_cue_output_all(TOPLEVEL * w_current, OBJECT * head, FILE * fp,
		      int type)
{
  OBJECT *o_current;

  o_current = head;
  while (o_current != NULL) {
    switch (o_current->type) {
      case (OBJ_NET):
      case (OBJ_BUS):
      case (OBJ_PIN):
        s_cue_output_single(w_current, o_current, fp, type);
        break;

      case (OBJ_COMPLEX):
      case (OBJ_PLACEHOLDER):
        s_cue_output_all(w_current, o_current->complex->prim_objs, fp,
                         type);
        break;

    }

    o_current = o_current->next;
  }
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void s_cue_output_lowlevel(TOPLEVEL * w_current, OBJECT * object, int whichone,
			   FILE * fp, int output_type)
{
  int x, y;
  GList *cl_current;
  CONN *conn;
  int type, count = 0;
  int done = FALSE;
  int bus_involved = FALSE;

  x = object->line->x[whichone];
  y = object->line->y[whichone];

  type = CONN_ENDPOINT;

  cl_current = object->conn_list;
  while (cl_current != NULL && !done) {
    conn = (CONN *) cl_current->data;

    if (conn->x == x && conn->y == y) {
      switch (conn->type) {

        case (CONN_ENDPOINT):
          count++;
          if (conn->other_object &&
              ((object->type == OBJ_NET &&
                conn->other_object->type == OBJ_BUS) ||
               (object->type == OBJ_BUS &&
                conn->other_object->type == OBJ_NET))) {
            bus_involved=TRUE;
          }
          break;

        case (CONN_MIDPOINT):
          type = CONN_MIDPOINT;
          done = TRUE;
          count = 0;
          if (conn->other_object &&
              ((object->type == OBJ_NET &&
                conn->other_object->type == OBJ_BUS) ||
               (object->type == OBJ_BUS &&
                conn->other_object->type == OBJ_NET))) {
            bus_involved=TRUE;
          }
          break;
      }
    }

    cl_current = cl_current->next;
  }

#if DEBUG
  printf("type: %d count: %d\n", type, count);
#endif

  switch (type) {

    case (CONN_ENDPOINT):
      if (object->type == OBJ_NET) {	/* only nets have these cues */
        if (count < 1) {	/* Didn't find anything connected there */
          if (output_type == POSTSCRIPT) {
            s_cue_postscript_fillbox(w_current, fp, x, y);
#ifdef HAS_LIBGDGEDA
          } else if (output_type == PNG) {
            s_cue_image_fillbox(w_current, object, x, y);
#endif
          }


        } else if (count >= 2) {
          if (output_type == POSTSCRIPT) {
            if (!bus_involved) {
              s_cue_postscript_fillcircle(w_current, fp, x, y, FALSE);
            } else {
              s_cue_postscript_fillcircle(w_current, fp, x, y, TRUE);
            }
#ifdef HAS_LIBGDGEDA
          } else if (output_type == PNG) {
            if (!bus_involved) {
              s_cue_image_fillcircle(w_current, x, y, FALSE);
            } else {
              s_cue_image_fillcircle(w_current, x, y, TRUE);
            }
#endif
          }
        }
      }
      break;

    case (CONN_MIDPOINT):
      if (output_type == POSTSCRIPT) {
        if (!bus_involved) {
          s_cue_postscript_fillcircle(w_current, fp, x, y, FALSE);
        } else {
          s_cue_postscript_fillcircle(w_current, fp, x, y, TRUE);
        }
#ifdef HAS_LIBGDGEDA
      } else if (output_type == PNG) {
        if (!bus_involved) {
          s_cue_image_fillcircle(w_current, x, y, FALSE);
        } else {
          s_cue_image_fillcircle(w_current, x, y, TRUE);
        }
#endif
      }
  }

}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void s_cue_output_lowlevel_midpoints(TOPLEVEL * w_current, OBJECT * object,
				     FILE * fp, int output_type)
{
  int x, y;
  GList *cl_current;
  CONN *conn;
  int size_flag;

  cl_current = object->conn_list;
  while (cl_current != NULL) {
    conn = (CONN *) cl_current->data;

    switch (conn->type) {
      case (CONN_MIDPOINT):

        x = conn->x;
        y = conn->y;

        if (conn->other_object &&
          ( (object->type == OBJ_BUS &&
             conn->other_object->type == OBJ_NET) ||
            (object->type == OBJ_NET &&
             conn->other_object->type == OBJ_BUS))) {
        size_flag = TRUE;
      } else {
        size_flag = FALSE;
      }

        
        if (output_type == POSTSCRIPT) {
          s_cue_postscript_fillcircle(w_current, fp, x, y, size_flag);
#ifdef HAS_LIBGDGEDA
        } else if (output_type == PNG) {
          s_cue_image_fillcircle(w_current, x, y, size_flag);
#endif
        }
        break;
    }


    cl_current = cl_current->next;
  }
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void s_cue_output_single(TOPLEVEL * w_current, OBJECT * object, FILE * fp,
			 int type)
{
  if (!object) {
    return;
  }

  if (object->type != OBJ_NET && object->type != OBJ_PIN &&
      object->type != OBJ_BUS) {
	return;
      }

  s_cue_output_lowlevel(w_current, object, 0, fp, type);
  s_cue_output_lowlevel(w_current, object, 1, fp, type);
  s_cue_output_lowlevel_midpoints(w_current, object, fp, type);
}


