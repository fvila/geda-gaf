/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 1998-2007 Ales Hvezda
 * Copyright (C) 1998-2008 gEDA Contributors (see ChangeLog for details)
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
#include <math.h>

#include "gschem.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif


static void path_to_points_modify (GSCHEM_TOPLEVEL *w_current, PATH *path,
                                   int dx, int dy, int new_x, int new_y, int whichone,
                                   GdkPoint **points, int *num_points)

{
  TOPLEVEL *toplevel = w_current->toplevel;
  PATH_SECTION *section;
  int x1, y1, x2, y2, x3, y3;
  int i;
  int grip_no = 0;

  *points = g_new0 (GdkPoint, path->num_sections);

  *num_points = 0;

  for (i = 0; i <  path->num_sections; i++) {
    section = &path->sections[i];

    x1 = section->x1 + dx; y1 = section->y1 + dy;
    x2 = section->x2 + dx; y2 = section->y2 + dy;
    x3 = section->x3 + dx; y3 = section->y3 + dy;

    switch (section->code) {
      case PATH_CURVETO:
        /* Two control point grips */
        if (whichone == grip_no++) {
          x1 = new_x; y1 = new_y;
        }
        if (whichone == grip_no++) {
          x2 = new_x; y2 = new_y;
        }
        WORLDtoSCREEN (toplevel, x1, y1, &x1, &y1);
        WORLDtoSCREEN (toplevel, x2, y2, &x2, &y2);
        /* Fall through */
      case PATH_MOVETO:
      case PATH_MOVETO_OPEN:
      case PATH_LINETO:
        /* Destination point grip */
        if (whichone == grip_no++) {
          x3 = new_x; y3 = new_y;
        }
        WORLDtoSCREEN (toplevel, x3, y3, &x3, &y3);
      case PATH_END:
        break;
    }

    switch (section->code) {
      case PATH_CURVETO:
        /* Unsupported, just fall through and draw a line */
        /* Fall through */
      case PATH_MOVETO_OPEN:
        /* Unsupported, just fall through and draw a line */
        /* Fall through */
      case PATH_MOVETO:
      case PATH_LINETO:
        (*points)[*num_points].x = x3;
        (*points)[*num_points].y = y3;
        (*num_points)++;
        break;
      case PATH_END:
        break;
    }
  }
}


static void path_to_points (GSCHEM_TOPLEVEL *w_current, PATH *path,
                            int dx, int dy,
                            GdkPoint **points, int *num_points)
{
  path_to_points_modify (w_current, path,
                         dx, dy, 0, 0, -1,
                         points, num_points);
}


static void find_points_bounds (GdkPoint *points, int num_points,
                                int *min_x, int *min_y, int *max_x, int *max_y)
{
  int i;
  int found_bound = FALSE;

  for (i = 0; i < num_points; i++) {
    *min_x = (found_bound) ? min (*min_x, points[i].x) : points[i].x;
    *min_y = (found_bound) ? min (*min_y, points[i].y) : points[i].y;
    *max_x = (found_bound) ? max (*max_x, points[i].x) : points[i].x;
    *max_y = (found_bound) ? max (*max_y, points[i].y) : points[i].y;
    found_bound = TRUE;
  }
}


/*! \brief Draw a path on screen.
 *  \par Function Description
 *  This function is used to draw a path on screen. The path is described
 *  in the object which is referred by <B>o_current</B>. The path is displayed
 *  according to the current state, described in the GSCHEM_TOPLEVEL object pointed
 *  by <B>w_current</B>.
 *
 *  It first checks if the object is valid or not. If not it returns and do
 *  not output anything. That should never happen though.
 *
 *  \param [in] w_current  The GSCHEM_TOPLEVEL object.
 *  \param [in] o_current  The path OBJECT to draw.
 */
void o_path_draw(GSCHEM_TOPLEVEL *w_current, OBJECT *o_current)
{
  TOPLEVEL *toplevel = w_current->toplevel;
  PATH *path = o_current->path;
  int line_width, length, space;
  int wleft, wtop, wright, wbottom;
  GdkPoint *points;
  int num_points;

  GdkColor *color;
  GdkCapStyle path_end;

  if (path == NULL) {
    return;
  }

  world_get_single_object_bounds(toplevel, o_current,
                                 &wleft, &wtop, &wright, &wbottom);
  if ( (toplevel->DONT_REDRAW == 1) ||
       (!visible(toplevel, wleft, wtop, wright, wbottom)) ) {
    return;
  }

  if (toplevel->override_color != -1 )
    color = x_get_color(toplevel->override_color);
  else
    color = x_get_color(o_current->color);

  line_width = SCREENabs( toplevel, o_current->line_width );
  if( line_width <= 0) {
    line_width = 1;
  }

  switch(o_current->line_end) {
    case END_NONE:   path_end = GDK_CAP_BUTT;       break;
    case END_SQUARE: path_end = GDK_CAP_PROJECTING; break;
    case END_ROUND:  path_end = GDK_CAP_ROUND;      break;
    default:
      fprintf(stderr, _("Unknown end for path (%d)\n"),
                      o_current->line_end);
      path_end = GDK_CAP_BUTT;
    break;
  }

  length = SCREENabs( toplevel, o_current->line_length );
  space = SCREENabs( toplevel, o_current->line_space );

  path_to_points (w_current, path, 0, 0, &points, &num_points);

  if (num_points == 0) {
    g_free (points);
    return;
  }

  /* TODO: Currently we only support solid line drawing */
  gdk_gc_set_foreground(w_current->gc, color);
  gdk_gc_set_line_attributes(w_current->gc, line_width, GDK_LINE_SOLID,
                             path_end, GDK_JOIN_MITER);

  /* Stroke */
  if (path->sections[path->num_sections - 1].code == PATH_END)
    gdk_draw_polygon (w_current->backingstore, w_current->gc,
                      FALSE, points, num_points);
  else
    gdk_draw_lines (w_current->backingstore, w_current->gc,
                    points, num_points);

  /* TODO: Currently we only support solid fill drawing */
  /* Fill */
  if (o_current->fill_type != FILLING_HOLLOW)
    gdk_draw_polygon(w_current->backingstore, w_current->gc,
                     TRUE, points, num_points);

  /* reset line width and reset back to default */
  gdk_gc_set_line_attributes(w_current->gc, 0, GDK_LINE_SOLID,
                             GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

  g_free (points);

  if (o_current->draw_grips && w_current->draw_grips == TRUE) {
    if (!o_current->selected) {
      /* object is no more selected, erase the grips */
      o_current->draw_grips = FALSE;
      o_path_erase_grips(w_current, o_current);
    } else {
      /* object is selected, draw the grips */
      o_path_draw_grips(w_current, o_current);
    }
  }

}


/*! \todo Finish function documentation
 *  \brief
 *  \par Function Description
 *
 *  \note
 *  used in button cancel code in x_events.c
 */
void o_path_eraserubber(GSCHEM_TOPLEVEL *w_current)
{
  o_path_rubberpath_xor (w_current);
}


/*! \brief Draw a path object after applying translation.
 *  \par Function Description
 *  This function is used to draw the path object described by
 *  <B>*o_current</B> after applying a translation on the two directions of
 *  <B>dx</B> and <B>dy</B> in world units. It uses and XOR function to draw the
 *  translated path over the current sheet.
 *
 *  \param [in] w_current  The GSCHEM_TOPLEVEL object.
 *  \param [in] dx         Delta x coordinate for path.
 *  \param [in] dy         Delta y coordinate for path.
 *  \param [in] o_current  Line OBJECT to draw.
 */
void o_path_draw_xor(GSCHEM_TOPLEVEL *w_current, int dx, int dy, OBJECT *o_current)
{
  PATH *path = o_current->path;
  int color;
  int num_points;
  GdkPoint *points;

  path_to_points (w_current, path, dx, dy, &points, &num_points);

  if (num_points == 0) {
    g_free (points);
    return;
  }

  if (o_current->saved_color != -1) {
    color = o_current->saved_color;
  } else {
    color = o_current->color;
  }

  gdk_gc_set_foreground(w_current->outline_xor_gc,
                        x_get_darkcolor(color));
  gdk_gc_set_line_attributes(w_current->xor_gc, 0, GDK_LINE_SOLID,
                             GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

  /* Stroke only, no fill for XOR */
  if (path->sections[path->num_sections - 1].code == PATH_END)
    gdk_draw_polygon (w_current->backingstore, w_current->xor_gc,
                      FALSE, points, num_points);
  else
    gdk_draw_lines (w_current->backingstore, w_current->xor_gc,
                    points, num_points);

  g_free (points);
}

/*! \brief Start process to input a new path.
 *  \par Function Description
 *  This function starts the process of interactively adding a path to
 *  the current sheet.
 *
 *  During all the process, the path is internally represented by the two
 *  ends of the path as (<B>w_current->first_wx</B>,<B>w_current->first_wy</B>) and
 *  (<B>w_current->second_wx</B>,<B>w_current->second_wy</B>).
 *
 *  A temporary path is xor-drawn during the process with the selection color
 *  and changed according to the position of the mouse pointer.
 *
 *  \param [in] w_current  The GSCHEM_TOPLEVEL object.
 *  \param [in] w_x        Current x coordinate of pointer in world units.
 *  \param [in] w_y        Current y coordinate of pointer in world units.
 */
void o_path_start(GSCHEM_TOPLEVEL *w_current, int w_x, int w_y)
{
  /* TODO: Implement support for drawing paths from within gschem */
}


/*! \brief End the input of a path.
 *  \par Function Description
 *  This function ends the process of interactively adding a path to the
 *  current sheet.
 *
 *  It first erases the last temporary path displayed, calculates the
 *  corresponding world coordinates of the two ends of the path and finally
 *  adds a new initialized path object to the list of object of the current
 *  sheet.
 *
 *  \param [in] w_current  The GSCHEM_TOPLEVEL object.
 *  \param [in] w_x        (unused)
 *  \param [in] w_y        (unused)
 */
void o_path_end(GSCHEM_TOPLEVEL *w_current, int w_x, int w_y)
{
  /* TODO: Implement support for drawing paths from within gschem */
}


/*! \brief Draw temporary path while dragging end.
 *  \par Function Description
 *  This function manages the erase/update/draw process of temporary path
 *  when modifying one end of the path.
 *  The path is described by four <B>*w_current</B> variables : the first end
 *  of the path is (<B>first_wx</B>,<B>first_wy</B>), the second end is
 *  (<B>second_wx</B>,<B>second_wy</B>).
 *  The first end is constant. The second end is updated to the (<B>w_x</B>,<B>w_y</B>).
 *
 *  \param [in] w_current  The GSCHEM_TOPLEVEL object.
 *  \param [in] w_x        Current x coordinate of pointer in world units.
 *  \param [in] w_y        Current y coordinate of pointer in world units.
 */
void o_path_rubberpath(GSCHEM_TOPLEVEL *w_current, int w_x, int w_y)
{
  if (w_current->rubber_visible)
    o_path_rubberpath_xor (w_current);

  w_current->second_wx = w_x;
  w_current->second_wy = w_y;

  o_path_rubberpath_xor (w_current);
  w_current->rubber_visible = 1;
}


/*! \brief Draw path from GSCHEM_TOPLEVEL object.
 *  \par Function Description
 *  This function draws a path with an exclusive or function over the sheet.
 *  The color of the box is <B>w_current->select_color</B>. The path is
 *  described by the two points (<B>w_current->first_wx</B>,
 *  <B>w_current->first_wy</B>) and (<B>w_current->second_wx</B>,<B>w_current->second_wy</B>).
 *
 *  \param [in] w_current  The GSCHEM_TOPLEVEL object.
 */
void o_path_rubberpath_xor(GSCHEM_TOPLEVEL *w_current)
{
  PATH *path;
  int num_points;
  GdkPoint *points;
  int left = 0, top = 0, right = 0, bottom = 0;

  g_return_if_fail (w_current->which_object != NULL);
  g_return_if_fail (w_current->which_object->path != NULL);

  path = w_current->which_object->path;

  path_to_points_modify (w_current, path, 0, 0,
                         w_current->second_wx, w_current->second_wy,
                         w_current->which_grip, &points, &num_points);

  if (num_points == 0) {
    g_free (points);
    return;
  }

  gdk_gc_set_foreground(w_current->xor_gc,
                        x_get_darkcolor(w_current->select_color));
  gdk_gc_set_line_attributes(w_current->xor_gc, 0, GDK_LINE_SOLID,
                             GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

  /* Stroke only, no fill for rubberbanding */
  if (path->sections[path->num_sections - 1].code == PATH_END)
    gdk_draw_polygon (w_current->backingstore, w_current->xor_gc,
                      FALSE, points, num_points);
  else
    gdk_draw_lines (w_current->backingstore, w_current->xor_gc,
                    points, num_points);

  find_points_bounds (points, num_points, &left, &top, &right, &bottom);
  o_invalidate_rect (w_current, left, top, right, bottom);

  g_free (points);
}


/*! \brief Draw lines between curve segment end-point and their control point.
 *
 *  \par Function Description
 *  This function XOR draws lines between the end-points and respective
 *  control-points of curve segments in the path.
 *
 *  \param [in] w_current  The GSCHEM_TOPLEVEL object.
 *  \param [in] o_current  The path OBJECT.
 */
static void o_path_xor_control_lines (GSCHEM_TOPLEVEL *w_current,
                                      OBJECT *o_current)
{
  TOPLEVEL *toplevel = w_current->toplevel;
  int i;
  int x, y;
  int next_x, next_y;
  int last_x = 0, last_y = 0;
  PATH_SECTION *section;

  gdk_gc_set_foreground(w_current->outline_xor_gc,
                        x_get_darkcolor(w_current->select_color));

  for (i = 0; i <  o_current->path->num_sections; i++) {
    section = &o_current->path->sections[i];

    if (section->code != PATH_END)
      WORLDtoSCREEN (toplevel, section->x3, section->y3, &next_x, &next_y);


    switch (section->code) {
    case PATH_CURVETO:
      /* Two control point grips */
      WORLDtoSCREEN (toplevel, section->x1, section->y1, &x, &y);
      gdk_draw_line (w_current->backingstore, w_current->outline_xor_gc,
                     last_x, last_y, x, y);
      WORLDtoSCREEN (toplevel, section->x2, section->y2, &x, &y);
      gdk_draw_line (w_current->backingstore, w_current->outline_xor_gc,
                     next_x, next_y, x, y);
      /* Fall through */
    case PATH_MOVETO:
    case PATH_MOVETO_OPEN:
    case PATH_LINETO:
      last_x = next_x;
      last_y = next_y;
      break;
    case PATH_END:
      break;
    }
  }
}


/*! \brief Draw grip marks on path.
 *  \par Function Description
 *  This function draws the grips on the path object <B>o_current</B>.
 *
 *  A path has a grip at each end.
 *
 *  \param [in] w_current  The GSCHEM_TOPLEVEL object.
 *  \param [in] o_current  Line OBJECT to draw grip points on.
 */
void o_path_draw_grips(GSCHEM_TOPLEVEL *w_current, OBJECT *o_current)
{
  TOPLEVEL *toplevel = w_current->toplevel;
  PATH_SECTION *section;
  int i;
  int x, y;

  if (w_current->draw_grips == FALSE)
    return;

  o_path_xor_control_lines (w_current, o_current);

  for (i = 0; i <  o_current->path->num_sections; i++) {
    section = &o_current->path->sections[i];

    switch (section->code) {
    case PATH_CURVETO:
      /* Two control point grips */
      WORLDtoSCREEN (toplevel, section->x1, section->y1, &x, &y);
      o_grips_draw (w_current, x, y);
      WORLDtoSCREEN (toplevel, section->x2, section->y2, &x, &y);
      o_grips_draw (w_current, x, y);
      /* Fall through */
    case PATH_MOVETO:
    case PATH_MOVETO_OPEN:
    case PATH_LINETO:
      /* Destination point grip */
      WORLDtoSCREEN (toplevel, section->x3, section->y3, &x, &y);
      o_grips_draw (w_current, x, y);
      break;
    case PATH_END:
      break;
    }
  }
}


/*! \brief Erase grip marks from path.
 *  \par Function Description
 *  This function erases the grips on the path object <B>o_current</B>.
 *
 *  A path has a grip at each end.
 *
 *  \param [in] w_current  The GSCHEM_TOPLEVEL object.
 *  \param [in] o_current  Line OBJECT to erase grip marks from.
 */
void o_path_erase_grips(GSCHEM_TOPLEVEL *w_current, OBJECT *o_current)
{
  TOPLEVEL *toplevel = w_current->toplevel;
  PATH_SECTION *section;
  int i;
  int x, y;

  if (w_current->draw_grips == FALSE)
    return;

  for (i = 0; i <  o_current->path->num_sections; i++) {
    section = &o_current->path->sections[i];

    switch (section->code) {
    case PATH_CURVETO:
      /* Two control point grips */
      WORLDtoSCREEN (toplevel, section->x1, section->y1, &x, &y);
      o_grips_erase (w_current, x, y);
      WORLDtoSCREEN (toplevel, section->x2, section->y2, &x, &y);
      o_grips_erase (w_current, x, y);
      /* Fall through */
    case PATH_MOVETO:
    case PATH_MOVETO_OPEN:
    case PATH_LINETO:
      /* Destination point grip */
      WORLDtoSCREEN (toplevel, section->x3, section->y3, &x, &y);
      o_grips_erase (w_current, x, y);
      break;
    case PATH_END:
      break;
    }
  }

  o_path_xor_control_lines (w_current, o_current);
}