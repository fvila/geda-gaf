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
#include <math.h>

#include <gtk/gtk.h>
#include <libguile.h>

#ifdef HAS_LIBGDGEDA
#include <gdgeda/gd.h>
#endif

#include "defines.h"
#include "struct.h"
#include "globals.h"
#include "o_types.h"

#include "colors.h"
#include "funcs.h"

#include "../include/prototype.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/*! \brief
 *  \par Function Description
 *
 */
int dist(int x1, int y1, int x2, int y2)
{
  int dx1, dy1;
  int dx2, dy2;
  int ret;

  dx1 = x1;
  dy1 = y1;
  dx2 = x2;
  dy2 = y2;

  ret =  sqrt(pow(dx1-dx2,2)+pow(dy1-dy2,2)) ;
  return( ret );
}

/*! \brief Create and add circle OBJECT to list.
 *  \par Function Description
 *  This function creates a new object representing a circle. This object is
 *  added to the end of the list <B>object_list</B> pointed object belongs to.
 *  The circle is described by its center (<B>x</B>,<B>y</B>) and its radius
 *  <B>radius</B>.
 *  The <B>type</B> parameter must be equal to <B>OBJ_CIRCLE</B>. The <B>color</B>
 *  corresponds to the color the box will be drawn with.
 *
 *  The <B>OBJECT</B> structure is allocated with the #s_basic_init_object()
 *  function. The structure describing the circle is allocated and initialized
 *  with the parameters given to the function.
 *
 *  Both the line type and the filling type are set to default values : solid
 *  line type with a width of 0, and no filling. It can be changed after
 *  with #o_set_line_options() and #o_set_fill_options().
 *
 *  The object is added to the end of the list described by the
 *  <B>object_list</B> parameter with #s_basic_link_object().
 *
 *  \param [in]     w_current    The TOPLEVEL object.
 *  \param [in,out] object_list  OBJECT list to add circle to.
 *  \param [in]     type         Must be OBJ_CIRCLE.
 *  \param [in]     color        Circle line color.
 *  \param [in]     x            Center x coordinate.
 *  \param [in]     y            Center y coordinate.
 *  \param [in]     radius       Radius of new circle.
 *  \return A pointer to the new end of the object list.
 */
OBJECT *o_circle_add(TOPLEVEL *w_current, OBJECT *object_list,
		     char type, int color,
		     int x, int y, int radius)
{
  OBJECT *new_node;	

  /* create the object */
  new_node         = s_basic_init_object("circle");
  new_node->type   = type;
  new_node->color  = color;
  
  new_node->circle = (CIRCLE *) g_malloc(sizeof(CIRCLE));
  
  /* describe the circle with its center and radius */
  new_node->circle->center_x = x;
  new_node->circle->center_y = y;
  new_node->circle->radius   = radius;
  
  /* line type and filling initialized to default */
  o_set_line_options(w_current, new_node,
		     END_NONE, TYPE_SOLID, 0, -1, -1);
  o_set_fill_options(w_current, new_node,
		     FILLING_HOLLOW, -1, -1, -1, -1, -1);
  
  /* \todo questionable cast */
  new_node->draw_func = (void *) circle_draw_func;  
  /* \todo questionable cast */
  new_node->sel_func = (void *) select_func;  
  
  /* compute the bounding box and screen coords */
  o_circle_recalc(w_current, new_node);
  
  /* add the object to the list */
  object_list = (OBJECT *) s_basic_link_object(new_node, object_list);

  return(object_list);
}

/*! \brief Create a copy of a circle.
 *  \par Function Description
 *  The function #o_circle_copy() creates a verbatim copy of the object
 *  pointed by <B>o_current</B> describing a circle. The new object is added at
 *  the end of the list, following the <B>list_tail</B> pointed object.
 *
 *
 *  \param [in]  w_current  The TOPLEVEL object.
 *  \param [out] list_tail  OBJECT list to copy to.
 *  \param [in]  o_current  Circle OBJECT to copy.
 *  \return A new pointer to the end of the object list.
 */
OBJECT *o_circle_copy(TOPLEVEL *w_current, OBJECT *list_tail,
		      OBJECT *o_current)
{
  OBJECT *new_obj;
  ATTRIB *a_current;
  int color;

  if (o_current->saved_color == -1) {
    color = o_current->color;
  } else {
    color = o_current->saved_color;
  }

  /*
   * A new circle object is added at the end of the object list with
   * #o_circle_add(). Values for its fields are default and need to be
   * modified.
   */
  /* create and link a new circle object */
  new_obj = o_circle_add(w_current, list_tail, OBJ_CIRCLE, 
			 color, 
			 0, 0, 0);
  
  /*
   * The parameters of the new circle are set with the ones of the original
   * circle. The two circle have the same line type and the same filling
   * options.
   *
   * The coordinates and the values in screen unit are computed with
   * #o_circle_recalc().
   */
  /* modify */
  new_obj->circle->center_x = o_current->circle->center_x;
  new_obj->circle->center_y = o_current->circle->center_y;
  new_obj->circle->radius   = o_current->circle->radius;
  
  o_set_line_options(w_current, new_obj, o_current->line_end,
		     o_current->line_type, o_current->line_width,
		     o_current->line_length, o_current->line_space);
  o_set_fill_options(w_current, new_obj,
		     o_current->fill_type, o_current->fill_width,
		     o_current->fill_pitch1, o_current->fill_angle1,
		     o_current->fill_pitch2, o_current->fill_angle2);
  
  o_circle_recalc(w_current, new_obj);

  /*	new_obj->attribute = 0;*/
  a_current = o_current->attribs;
  if (a_current) {
    while ( a_current ) {
      
      /* head attrib node has prev = NULL */
      if (a_current->prev != NULL) {
	a_current->copied_to = new_obj;
      }
      a_current = a_current->next;
    }
  }
  
  return(new_obj);
}

/*! \brief Modify the description of a circle OBJECT.
 *  \par Function Description
 *  This function modifies the description of the circle object <B>*object</B>
 *  depending on <B>whichone</B> that give the meaning of the <B>x</B> and <B>y</B>
 *  parameters.
 *
 *  If <B>whichone</B> is equal to <B>CIRCLE_CENTER</B>, the new center of the
 *  circle is given by (<B>x</B>,<B>y</B>) where <B>x</B> and <B>y</B> are in world units.
 *
 *  If <B>whichone</B> is equal to <B>CIRCLE_RADIUS</B>, the radius is given by
 *  <B>x</B> - in world units. <B>y</B> is ignored.
 *
 *  The screen coords and the bounding box of the circle object are updated
 *  after the modification of its parameters.
 *
 *  \param [in]     w_current  The TOPLEVEL object.
 *  \param [in,out] object     Circle OBJECT to modify.
 *  \param [in]     x          New center x coordinate, or radius value.
 *  \param [in]     y          New center y coordinate.
 *                             Unused if radius is being modified.
 *  \param [in]     whichone   Which circle parameter to modify.
 *
 *  <B>whichone</B> can have the following values:
 *  <DL>
 *    <DT>*</DT><DD>CIRCLE_CENTER
 *    <DT>*</DT><DD>CIRCLE_RADIUS
 *  </DL>
 */
void o_circle_modify(TOPLEVEL *w_current, OBJECT *object, 
		     int x, int y, int whichone)
{
  switch(whichone) {
    case CIRCLE_CENTER:
      /* modify the center of the circle */
      object->circle->center_x = x;
      object->circle->center_y = y;
      break;
    case CIRCLE_RADIUS:
      /* modify the radius of the circle */
      if (x == 0) {
	s_log_message("Null radius circles are not allowed\n");
	return;
      }
      object->circle->radius = x;
      break;
    default:
      break;
  }

  /* recalculate the screen coords and the boundings */
  o_circle_recalc(w_current, object);
  
}

/*! \brief Create circle OBJECT from character string.
 *  \par Function Description
 *  The #o_circle_read() function gets from the character string <B>*buff</B> the
 *  description of a circle. The new circle is then added to the list of
 *  objects of which <B>*object_list</B> is the last element before the call.
 *
 *  Depending on <B>*version</B>, the right file format is considered.
 *  Currently two file format revisions are supported :
 *  <DL>
 *    <DT>*</DT><DD>the file format used until 2000704 release.
 *    <DT>*</DT><DD>the file format used for the releases after 20000704.
 *  </DL>
 *
 *  \param [in]  w_current       The TOPLEVEL object.
 *  \param [out] object_list     OBJECT list to create circle in.
 *  \param [in]  buf             Character string with circle description.
 *  \param [in]  release_ver     libgeda release version number.
 *  \param [in]  fileformat_ver  libgeda file format version number.
 *  \return A pointer to the new circle object.
 */
OBJECT *o_circle_read(TOPLEVEL *w_current, OBJECT *object_list, char buf[],
		      unsigned int release_ver, unsigned int fileformat_ver)
{
  char type; 
  int x1, y1;
  int radius;
  int color;
  int circle_width, circle_space, circle_length;
  int fill_width, angle1, pitch1, angle2, pitch2;
  int circle_end;
  int circle_type;
  int circle_fill;

  if(release_ver <= VERSION_20000704) {
    /*
     * The old geda file format, i.e. releases 20000704 and older, does not
     * handle the line type and the filling of the box object. They are set
     * to default.
     */
    sscanf(buf, "%c %d %d %d %d\n", &type, &x1, &y1, &radius, &color);

    circle_width = 0;
    circle_end   = END_NONE;
    circle_type  = TYPE_SOLID;
    circle_length= -1;
    circle_space = -1;
    
    circle_fill  = FILLING_HOLLOW;
    fill_width  = 0;
    angle1      = -1;
    pitch1      = -1;
    angle2      = -1;
    pitch2      = -1;
			
  } else {
	
    /*
     * The current line format to describe a circle is a space separated
     * list of characters and numbers in plain ASCII on a single line. The
     * meaning of each item is described in the file format documentation.
     */  
    sscanf(buf, "%c %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
	   &type, &x1, &y1, &radius, &color,
	   &circle_width, &circle_end, &circle_type,
	   &circle_length, &circle_space, &circle_fill,
	   &fill_width, &angle1, &pitch1, &angle2, &pitch2);
  }


  if (radius == 0) {
    fprintf(stderr, "Found a zero radius circle [ %c %d %d %d %d ]\n",
            type, x1, y1, radius, color);
    s_log_message("Found a zero radius circle [ %c %d %d %d %d ]\n",
                  type, x1, y1, radius, color);
	
  }
  
  if (color < 0 || color > MAX_COLORS) {
    fprintf(stderr, "Found an invalid color [ %s ]\n", buf);
    s_log_message("Found an invalid color [ %s ]\n", buf);
    s_log_message("Setting color to WHITE\n");
    color = WHITE;
  }

  /* 
   * A circle is internally described by its center and its radius.
   *
   * A new object is allocated, initialized and added to the object list.
   * Its filling and line type are set according to the values of the field
   * on the line.
   */
  object_list = (OBJECT *) o_circle_add(w_current, object_list,
					type, color, x1, y1, radius);
  o_set_line_options(w_current, object_list,
		     circle_end, circle_type, circle_width, 
		     circle_length, circle_space);
  o_set_fill_options(w_current, object_list,
		     circle_fill, fill_width, pitch1, angle1, pitch2, angle2);
  
  return(object_list);
}

/*! \brief Create a character string representation of a circle OBJECT.
 *  \par Function Description
 *  This function formats a string in the buffer <B>*buff</B> to describe the
 *  circle object <B>*object</B>.
 *  It follows the post-20000704 release file format that handle the line
 *  type and fill options.
 *
 *  \param [in] object  Circle OBJECT to create string from.
 *  \return A pointer to the circle OBJECT character string.
 *
 *  \note
 *  Caller must free returned character string.
 *
 */
char *o_circle_save(OBJECT *object)
{
  int x,y;
  int radius;
  int color;
  int circle_width, circle_space, circle_length;
  int fill_width, angle1, pitch1, angle2, pitch2;
  char *buf;
  OBJECT_END circle_end;
  OBJECT_TYPE circle_type;
  OBJECT_FILLING circle_fill;

  /* circle center and radius */
  x = object->circle->center_x;
  y = object->circle->center_y;
  radius = object->circle->radius;
  
  /* line type parameters */
  circle_width = object->line_width;
  circle_end   = object->line_end;
  circle_type  = object->line_type;
  circle_length= object->line_length;
  circle_space = object->line_space;
  
  /* filling parameters */
  circle_fill  = object->fill_type;
  fill_width   = object->fill_width;
  angle1       = object->fill_angle1;
  pitch1       = object->fill_pitch1;
  angle2       = object->fill_angle2;
  pitch2       = object->fill_pitch2;
  
  /* Use the right color */
  if (object->saved_color == -1) {
    color = object->color;
  } else {
    color = object->saved_color;
  }
  
#if 0 /* old system */
  radius = abs(x2 - x1)/2;
  if (radius == 0) {
    radius = abs(y2 - y1)/2;
  }
  
  x = x1 + radius; 
  y = y1 - radius; /* careful */
#endif
  
  buf = g_strdup_printf("%c %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", 
			object->type, x, y, radius, color,
			circle_width, circle_end, circle_type, circle_length, 
			circle_space, circle_fill,
			fill_width, angle1, pitch1, angle2, pitch2);
  return(buf);
}
           
/*! \brief Translate a circle position by a delta.
 *  \par Function Description
 *  This function applies a translation of (<B>dx</B>,<B>dy</B> to the circle
 *  described by <B>*object</B>. <B>dx</B> and <B>dy</B> are in screen unit.
 *
 *  The translation vector is converted in world unit. The translation is
 *  made with #o_circle_translate_world().
 *
 *  \param [in]     w_current   The TOPLEVEL object.
 *  \param [in]     dx          x distance to move.
 *  \param [in]     dy          y distance to move.
 *  \param [in,out] object      Circle OBJECT to translate.
 */
void o_circle_translate(TOPLEVEL *w_current, int dx, int dy, OBJECT *object)
{
	int world_dx, world_dy;
	
	if (object == NULL) printf("ct NO!\n");
	
	/* convert the translation vector in world unit */
	world_dx = SCREENabs(w_current, dx);
	world_dy = SCREENabs(w_current, dy);
	
	/* translate the circle */
	o_circle_translate_world(w_current, world_dx, world_dy, object);
	
	/* screen coords and boundings are updated by _translate_world() */
  
}

/*! \brief Translate a circle position in WORLD coordinates by a delta.
 *  \par Function Description
 *  This function applies a translation of (<B>x1</B>,<B>y1</B>) to the circle
 *  described by <B>*object</B>. <B>x1</B> and <B>y1</B> are in world unit. 
 *
 *  \param [in]     w_current  The TOPLEVEL object.
 *  \param [in]     x1         x distance to move.
 *  \param [in]     y1         y distance to move.
 *  \param [in,out] object     Circle OBJECT to translate.
 */
void o_circle_translate_world(TOPLEVEL *w_current,
			      int x1, int y1, OBJECT *object)
{
  if (object == NULL) printf("ctw NO!\n");

  /* Do world coords */
  object->circle->center_x = object->circle->center_x + x1;
  object->circle->center_y = object->circle->center_y + y1;
  
  /* recalc the screen coords and the bounding box */
  o_circle_recalc(w_current, object);
  
}

/*! \brief Rotate a Circle OBJECT.
 *  \par Function Description
 *  The function #o_circle_rotate_world() rotate the circle described by
 *  <B>*object</B> around the (<B>world_centerx</B>,<B>world_centery</B>) point by
 *  angle <B>angle</B> degrees.
 *  The center of rotation is in world unit.
 *
 *  \param [in]     w_current  The TOPLEVEL object.
 *  \param [in]     centerx    Rotation center x coordinate in SCREEN units.
 *  \param [in]     centery    Rotation center y coordinate in SCREEN units.
 *  \param [in]     angle      Rotation angle in degrees (unused).
 *  \param [in,out] object     Circle OBJECT to rotate.
 *
 *  \note
 *  takes in screen coordinates for the centerx,y, and then does the rotate 
 *  in world space
 *  also ignores angle argument... for now, rotate only in 90 degree 
 *  increments
 */
void o_circle_rotate(TOPLEVEL *w_current,
		     int centerx, int centery, int angle,
		     OBJECT *object)
{
  int world_centerx, world_centery;

  /* convert the center of rotation to world unit */
  SCREENtoWORLD(w_current,
				centerx, centery, 
                &world_centerx, &world_centery);  

  /* rotate the circle */
  o_circle_rotate_world(w_current,
			world_centerx, world_centery, angle,
			object);
  
  /* screen coords and boundings are updated by _rotate_world() */
  
}

/*! \brief Rotate Circle OBJECT using WORLD coordinates. 
 *  \par Function Description
 *  The function #o_circle_rotate_world() rotate the circle described by
 *  <B>*object</B> around the (<B>world_centerx</B>,<B>world_centery</B>) point by
 *  angle <B>angle</B> degrees.
 *  The center of rotation is in world unit.
 *
 *  \param [in]      w_current      The TOPLEVEL object.
 *  \param [in]      world_centerx  Rotation center x coordinate in WORLD units.
 *  \param [in]      world_centery  Rotation center y coordinate in WORLD units.
 *  \param [in]      angle          Rotation angle in degrees (See note below).
 *  \param [in,out]  object         Circle OBJECT to rotate.
 */
void o_circle_rotate_world(TOPLEVEL *w_current, 
			   int world_centerx, int world_centery, int angle,
			   OBJECT *object)
{
  int newx, newy;
  int x, y;

  /* Only 90 degree multiple and positive angles are allowed. */
  /* angle must be positive */
  if(angle < 0) angle = -angle;
  /* angle must be a 90 multiple or no rotation performed */
  if((angle % 90) != 0) return;
  
  /*
   * The center of rotation (<B>world_centerx</B>,<B>world_centery</B>) is
   * translated to the origin. The rotation of the center around the origin
   * is then performed. Finally, the rotated circle is translated back to
   * its previous location.
   */

  /* translate object to origin */
  object->circle->center_x -= world_centerx;
  object->circle->center_y -= world_centery;
  
  /* rotate the center of the circle around the origin */
  x = object->circle->center_x;
  y = object->circle->center_y;
  rotate_point_90(x, y, angle, &newx, &newy);
  object->circle->center_x = newx;
  object->circle->center_y = newy;
  
  /* translate back in position */
  object->circle->center_x += world_centerx;
  object->circle->center_y += world_centery;

  o_circle_recalc(w_current, object);
  
}

/*! \brief Mirror a Circle.
 *  \par Function Description
 *  This function mirrors the circle from the point (<B>centerx</B>,<B>centery</B>)
 *  in screen unit.
 *
 *  The origin of the mirror in screen unit is converted in world unit.
 *  The circle is mirrored with the function #o_circle_mirror_world()
 *  for which the origin of the mirror must be given in world unit.
 *
 *  \param [in]     w_current  The TOPLEVEL object.
 *  \param [in]     centerx    Origin x coordinate in WORLD units.
 *  \param [in]     centery    Origin y coordinate in WORLD units.
 *  \param [in,out] object     Circle OBJECT to mirror.
 */
void o_circle_mirror(TOPLEVEL *w_current,
		     int centerx, int centery,
		     OBJECT *object)
{
  int world_centerx, world_centery;

  /* convert the origin of mirror */
  SCREENtoWORLD(w_current,
		centerx, centery, 
                &world_centerx, &world_centery);  

  /* apply the mirror in world coords */
  o_circle_mirror_world(w_current,
			world_centerx, world_centery,
			object);
  
  /* screen coords and boundings are updated by _mirror_world() */
}

/*! \brief Mirror circle using WORLD coordinates.
 *  \par Function Description
 *  This function recalculates the screen coords of the <B>o_current</B> pointed
 *  circle object from its world coords.
 *
 *  The circle coordinates and its bounding are recalculated as well as the
 *  OBJECT specific (line width, filling ...).
 *
 *  \param [in]     w_current      The TOPLEVEL object.
 *  \param [in]     world_centerx  Origin x coordinate in WORLD units.
 *  \param [in]     world_centery  Origin y coordinate in WORLD units.
 *  \param [in,out] object         Circle OBJECT to mirror.
 */
void o_circle_mirror_world(TOPLEVEL *w_current,
			   int world_centerx, int world_centery,
			   OBJECT *object)
{
  /* translate object to origin */
  object->circle->center_x -= world_centerx;
  object->circle->center_y -= world_centery;

  /* mirror the center of the circle */
  object->circle->center_x = -object->circle->center_x;
  object->circle->center_y =  object->circle->center_y;

  /* translate back in position */
  object->circle->center_x += world_centerx;
  object->circle->center_y += world_centery;

  /* recalc boundings and screen coords */
  o_circle_recalc(w_current, object);
  
}

/*! \brief Recalculate circle coordinates in SCREEN units.
 *  \par Function Description
 *  This function recalculates the screen coords of the <B>o_current</B> pointed
 *  circle object from its world coords.
 *
 *  The circle coordinates and its bounding are recalculated as well as the
 *  OBJECT specific (line width, filling ...).
 *
 *  \param [in] w_current      The TOPLEVEL object.
 *  \param [in,out] o_current  Circle OBJECT to be recalculated.
 */
void o_circle_recalc(TOPLEVEL *w_current, OBJECT *o_current)
{
  int screen_x1, screen_y1;
  int left, right, top, bottom;

  if (o_current->circle == NULL) {
    return;
  }

#if DEBUG
  printf("drawing circle\n");
#endif

  
  /* update the screen coords of the center of the circle */
  WORLDtoSCREEN(w_current,
		o_current->circle->center_x, o_current->circle->center_y, 
		&screen_x1, &screen_y1);  
  o_current->circle->screen_x = screen_x1;
  o_current->circle->screen_y = screen_y1;

  /* update the value of the radius in screen unit */
  o_current->circle->screen_radius = SCREENabs(w_current, 
					       o_current->circle->radius);

  /* update the bounding box - screen unit */
  get_circle_bounds(w_current, o_current->circle,
		    &left, &top, &right, &bottom);
  o_current->left   = left;
  o_current->top    = top;
  o_current->right  = right;
  o_current->bottom = bottom;

  /* recalc OBJECT specific parameters */
  o_object_recalc(w_current, o_current);
  
}

/*! \brief Get circle bounding rectangle.
 *  \par Function Description
 *  This function sets the <B>left</B>, <B>top</B>, <B>right</B>
 *  and <B>bottom</B> pointed variables to the boundings of the circle object
 *  described in <B>*circle</B> in screen unit.
 *
 *  The function finds the smallest rectangle that cover this circle.
 *
 *  \param [in]  w_current  The TOPLEVEL object.
 *  \param [in]  circle     Circle OBJECT to read coordinates from.
 *  \param [out] left       Left circle coordinate in SCREEN units.
 *  \param [out] top        Top circle coordinate in SCREEN units.
 *  \param [out] right      Right circle coordinate in SCREEN units.
 *  \param [out] bottom     Bottom circle coordinate in SCREEN units.
 */
void get_circle_bounds(TOPLEVEL *w_current, CIRCLE *circle,
		       int *left, int *top,
		       int *right, int *bottom)
{
  *left   = circle->screen_x - circle->screen_radius;
  *top    = circle->screen_y - circle->screen_radius;
  *right  = circle->screen_x + circle->screen_radius;
  *bottom = circle->screen_y + circle->screen_radius;
  
  /* PB : need to take into account the width of the line */
  
  /* out temp  
   *left = *left - 4;
   *top = *top - 4;
   
   *right = *right + 4;
   *bottom = *bottom + 4;
   */
}

/*! \brief Get circle bounding rectangle in WORLD coordinates.
 *  \par Function Description
 *  This function sets the <B>left</B>, <B>top</B>, <B>right</B> and <B>bottom</B>
 *  parameters to the boundings of the circle object described in <B>*circle</B>
 *  in world units.
 *
 *  \param [in]  w_current  The TOPLEVEL object.
 *  \param [in]  circle     Circle OBJECT to read coordinates from.
 *  \param [out] left       Left circle coordinate in WORLD units.
 *  \param [out] top        Top circle coordinate in WORLD units.
 *  \param [out] right      Right circle coordinate in WORLD units.
 *  \param [out] bottom     Bottom circle coordinate in WORLD units.
 */
void world_get_circle_bounds(TOPLEVEL *w_current, CIRCLE *circle, int *left,
			     int *top, int *right, int *bottom)
{

  *left   = w_current->init_right;
  *top    = w_current->init_bottom;
  *right  = 0;
  *bottom = 0;


  *left   = circle->center_x - circle->radius;
  *top    = circle->center_y - circle->radius;
  *right  = circle->center_x + circle->radius;
  *bottom = circle->center_y + circle->radius;

  /*
   *left = points->x1;
   *top = points->y1;
   *right = points->x1+(temp);
   *bottom = points->y1-(temp); 
   */

  /* 
   *left = min(circle->x1, circle->x1+temp);
   *top = min(circle->y1, circle->y1-temp);
   *right = max(circle->x1, circle->x1+temp);
   *bottom = max(circle->y1, circle->y1-temp);*/

#if DEBUG 
  printf("circle: %d %d %d %d\n", *left, *top, *right, *bottom);
#endif

}

/*! \brief Print circle to Postscript document.
 *  \par Function Description
 *  This function prints the circle described by the <B>o_current</B>
 *  parameter to a Postscript document. It takes into account its line type
 *  and fill type.
 *  The Postscript document is descibed by the file pointer <B>fp</B>.
 *
 *  The validity of the <B>o_current</B> pointer is checked :
 *  a null pointer causes an error message and a return.
 *
 *  The description of the circle is extracted from the <B>o_current</B>
 *  parameter : the coordinates of the center of the circle, its radius,
 *  its line type, its fill type.
 *
 *  The outline and the inside of the circle are successively handled by
 *  two differend sets of functions.
 *  
 *  \param [in] w_current  The TOPLEVEL object.
 *  \param [in] fp         FILE pointer to Postscript document.
 *  \param [in] o_current  Circle OBJECT to write to document.
 *  \param [in] origin_x   Page x coordinate to place circle OBJECT.
 *  \param [in] origin_y   Page y coordinate to place circle OBJECT.
 */
void o_circle_print(TOPLEVEL *w_current, FILE *fp, OBJECT *o_current, 
		    int origin_x, int origin_y)
{
  int x, y, radius;
  int color;
  int circle_width, length, space;
  int fill_width, angle1, pitch1, angle2, pitch2;
  void (*outl_func)() = NULL;
  void (*fill_func)() = NULL;

  if (o_current == NULL) {
    printf("got null in o_circle_print\n");
    return;
  }

  x      = o_current->circle->center_x; 
  y      = o_current->circle->center_y;
  radius = o_current->circle->radius;

  color  = o_current->color;

  /*
   * Depending on the type of the line for this particular circle, the
   * appropriate function is chosen among #o_circle_print_solid(),
   * #o_circle_print_dotted(), #o_circle_print_dashed(),
   * #o_circle_print_center() and #o_circle_print_phantom().
   *
   * The needed parameters for each of these type is extracted from the
   * <B>o_current</B> object. Depending on the type, unused parameters are
   * set to -1.
   *
   * In the eventuality of a length and/or space null, the line is
   * printed solid to avoid and endless loop produced by other functions
   * in such a case.
   */
  circle_width = o_current->line_width;
  if(circle_width <= 2) circle_width=2;
  length       = o_current->line_length;
  space        = o_current->line_space;

  switch(o_current->line_type) {
    case(TYPE_SOLID):
      length = -1; space  = -1;
      outl_func = (void *) o_circle_print_solid;
      break;

    case(TYPE_DOTTED):
      length = -1;
      outl_func = (void *) o_circle_print_dotted;
      break;

    case(TYPE_DASHED):
      outl_func = (void *) o_circle_print_dashed;
      break;

    case(TYPE_CENTER):
      outl_func = (void *) o_circle_print_center;
      break;

    case(TYPE_PHANTOM):
      outl_func = (void *) o_circle_print_phantom;
      break;

    case(TYPE_ERASE):
      /* Unused for now print it solid */
      length = -1; space  = -1;
      outl_func = (void *) o_circle_print_solid;
      break;
  }

  if((length == 0) || (space == 0)) {
    length = -1; space  = -1;
    outl_func = (void *) o_circle_print_solid;
  }

  (*outl_func)(w_current, fp,
               x - origin_x, y - origin_y,
               radius,
               color,
               circle_width, length, space,
               origin_x, origin_y);

  /*
   * If the filling type of the circle is not <B>HOLLOW</B>, the appropriate
   * function is chosen among #o_circle_print_filled(), #o_circle_print_mesh()
   * and #o_circle_print_hatch(). The corresponding parameters are extracted
   * from the <B>o_current</B> object and corrected afterward.
   *
   * The case where <B>pitch1</B> and <B>pitch2</B> are null or negative is
   * avoided as it leads to an endless loop in most of the called functions.
   * In such a case, the circle is printed filled. Unused parameters for
   * each of these functions are set to -1 or any passive value.
   */
  if(o_current->fill_type != FILLING_HOLLOW) {
    fill_width = o_current->fill_width;
    angle1     = o_current->fill_angle1;
    pitch1     = o_current->fill_pitch1;
    angle2     = o_current->fill_angle2;
    pitch2     = o_current->fill_pitch2;
		
    switch(o_current->fill_type) {
      case(FILLING_FILL):
        angle1 = -1; pitch1 = 1;
        angle2 = -1; pitch2 = 1;
        fill_width = -1;
        fill_func = (void *) o_circle_print_filled;
        break;
			
      case(FILLING_MESH):
        fill_func = (void *) o_circle_print_mesh;
        break;
				
      case(FILLING_HATCH):
        angle2 = -1; pitch2 = 1;
        fill_func = (void *) o_circle_print_hatch;
        break;
				
      case(FILLING_VOID):
				/* Unused for now, print it filled */
        angle1 = -1; pitch1 = 1;
        angle2 = -1; pitch2 = 1;
        fill_width = -1;
        fill_func = (void *) o_circle_print_filled;
        break;
        
      case(FILLING_HOLLOW):
        /* nop */
        break;
    }

    if((pitch1 <= 0) || (pitch2 <= 0)) {
      angle1 = -1; pitch1 = 1;
      angle2 = -1; pitch2 = 1;
      fill_func = (void *) o_circle_print_filled;
    }
		
    (*fill_func)(w_current, fp,
                 x, y, radius,
                 color,
                 fill_width,
                 angle1, pitch1, angle2, pitch2,
                 origin_x, origin_y);
  }
}

/*! \brief Print a solid circle to Postscript document.
 *  \par Function Description
 *  This function prints the outline of a circle when a solid line type
 *  is required. The circle is defined by its center in (<B>x</B>, <B>y</B>)
 *  and its radius in <B>radius</B>. It is printed with the color given
 *  in <B>color</B>.
 *  The parameters <B>length</B> and <B>space</B> are ignored.
 *
 *  It uses the function #o_arc_print_solid() to print the outline.
 *  Therefore it acts as an interface between the way a circle is defined
 *  and the way an arc is defined.
 *
 *  All dimensions are in mils.
 *
 *  \param [in] w_current     The TOPLEVEL object.
 *  \param [in] fp            FILE pointer to Postscript document.
 *  \param [in] x             Center x coordinate of circle.
 *  \param [in] y             Center y coordinate of circle.
 *  \param [in] radius        Circle radius.
 *  \param [in] color         Circle color.
 *  \param [in] circle_width  Width of circle.
 *  \param [in] length        (unused).
 *  \param [in] space         (unused).
 *  \param [in] origin_x      Page x coordinate to place circle OBJECT.
 *  \param [in] origin_y      Page y coordinate to place circle OBJECT.
 */
void o_circle_print_solid(TOPLEVEL *w_current, FILE *fp,
			  int x, int y, int radius,
			  int color,
			  int circle_width, int length, int space,
			  int origin_x, int origin_y)
{

  o_arc_print_solid(w_current, fp,
                    x, y, radius,
                    0, FULL_CIRCLE / 64,
                    color,
                    circle_width, -1, -1,
                    origin_x, origin_y);

}


/*! \brief Print a dotted circle to Postscript document.
 *  \par Function Description
 *  This function prints the outline of a circle when a dotted line
 *  type is required. The circle is defined by its center
 *  in (<B>x</B>, <B>y</B>) and its radius in <B>radius</B>. It is printed
 *  with the color given in <B>color</B>.
 *  The parameter <B>length</B> is ignored.
 *
 *  It uses the function #o_arc_print_dotted() to print the outline.
 *  Therefore it acts as an interface between the way a circle is
 *  defined and the way an arc is defined.
 *
 *  All dimensions are in mils.
 *
 *  \param [in] w_current     The TOPLEVEL object.
 *  \param [in] fp            FILE pointer to Postscript document.
 *  \param [in] x             Center x coordinate of circle.
 *  \param [in] y             Center y coordinate of circle.
 *  \param [in] radius        Circle radius.
 *  \param [in] color         Circle color.
 *  \param [in] circle_width  Width of circle.
 *  \param [in] length        (unused).
 *  \param [in] space         Space between dots.
 *  \param [in] origin_x      Page x coordinate to place circle OBJECT.
 *  \param [in] origin_y      Page y coordinate to place circle OBJECT.
 */
void o_circle_print_dotted(TOPLEVEL *w_current, FILE *fp,
			   int x, int y, int radius,
			   int color,
			   int circle_width, int length, int space,
			   int origin_x, int origin_y)
{

  o_arc_print_dotted(w_current, fp,
                     x, y, radius,
                     0, FULL_CIRCLE / 64,
                     color,
                     circle_width, -1, space,
                     origin_x, origin_y);

}

/*! \brief Print a dashed circle to Postscript document.
 *  \par Function Description
 *  This function prints the outline of a circle when a dashed line type
 *  is required. The circle is defined by its center in
 *  (<B>x</B>, <B>y</B>) and its radius in <B>radius</B>. It is printed with the
 *  color given in <B>color</B>.
 *
 *  It uses the function #o_arc_print_dashed() to print the outline.
 *  Therefore it acts as an interface between the way a circle is
 *  defined and the way an arc is defined.
 *
 *  All dimensions are in mils.
 *
 *  \param [in] w_current     The TOPLEVEL object.
 *  \param [in] fp            FILE pointer to Postscript document.
 *  \param [in] x             Center x coordinate of circle.
 *  \param [in] y             Center y coordinate of circle.
 *  \param [in] radius        Circle radius.
 *  \param [in] color         Circle color.
 *  \param [in] circle_width  Width of circle.
 *  \param [in] length        Length of dashed lines.
 *  \param [in] space         Space between dashes.
 *  \param [in] origin_x      Page x coordinate to place circle OBJECT.
 *  \param [in] origin_y      Page y coordinate to place circle OBJECT.
 */
void o_circle_print_dashed(TOPLEVEL *w_current, FILE *fp,
			   int x, int y,
			   int radius,
			   int color,
			   int circle_width, int length, int space,
			   int origin_x, int origin_y)
{

  o_arc_print_dashed(w_current, fp,
                     x, y, radius,
                     0, FULL_CIRCLE / 64,
                     color,
                     circle_width, length, space,
                     origin_x, origin_y);

}

/*! \brief Print a centered line type circle to Postscript document.
 *  \par Function Description
 *  This function prints the outline of a circle when a centered line
 *  type is required. The circle is defined by its center in
 *  (<B>x</B>, <B>y</B>) and its radius in <B>radius</B>. It is printed with the
 *  color given in <B>color</B>.
 *
 *  It uses the function #o_arc_print_center() to print the outline.
 *  Therefore it acts as an interface between the way a circle is
 *  defined and the way an arc is defined.
 *
 *  All dimensions are in mils.
 *
 *  \param [in] w_current     The TOPLEVEL object.
 *  \param [in] fp            FILE pointer to Postscript document.
 *  \param [in] x             Center x coordinate of circle.
 *  \param [in] y             Center y coordinate of circle.
 *  \param [in] radius        Circle radius.
 *  \param [in] color         Circle color.
 *  \param [in] circle_width  Width of circle.
 *  \param [in] length        Length of dashed lines.
 *  \param [in] space         Space between dashes.
 *  \param [in] origin_x      Page x coordinate to place circle OBJECT.
 *  \param [in] origin_y      Page y coordinate to place circle OBJECT.
 */
void o_circle_print_center(TOPLEVEL *w_current, FILE *fp,
			   int x, int y,
			   int radius,
			   int color,
			   int circle_width, int length, int space,
			   int origin_x, int origin_y)
{
	
  o_arc_print_center(w_current, fp,
                     x, y, radius,
                     0, FULL_CIRCLE / 64,
                     color,
                     circle_width, length, space,
                     origin_x, origin_y);

}

/*! \brief Print a phantom line type circle to Postscript document.
 *  \par Function Description
 *  This function prints the outline of a circle when a phantom line type
 *  is required. The circle is defined by its center in
 *  (<B>x</B>, <B>y</B>) and its radius in <B>radius</B>. It is printed with the
 *  color given in <B>color</B>.
 *
 *  It uses the function #o_arc_print_phantom() to print the outline.
 *  Therefore it acts as an interface between the way a circle is defined
 *  and the way an arc is defined.
 *
 *  All dimensions are in mils.
 *
 *  \param [in] w_current     The TOPLEVEL object.
 *  \param [in] fp            FILE pointer to Postscript document.
 *  \param [in] x             Center x coordinate of circle.
 *  \param [in] y             Center y coordinate of circle.
 *  \param [in] radius        Circle radius.
 *  \param [in] color         Circle color.
 *  \param [in] circle_width  Width of circle.
 *  \param [in] length        Length of dashed lines.
 *  \param [in] space         Space between dashes.
 *  \param [in] origin_x      Page x coordinate to place circle OBJECT.
 *  \param [in] origin_y      Page y coordinate to place circle OBJECT.
 */
void o_circle_print_phantom(TOPLEVEL *w_current, FILE *fp,
			    int x, int y,
			    int radius,
			    int color,
			    int circle_width, int length, int space,
			    int origin_x, int origin_y)
{

  o_arc_print_phantom(w_current, fp,
                      x, y, radius,
                      0, FULL_CIRCLE / 64,
                      color,
                      circle_width, length, space,
                      origin_x, origin_y);

}

/*! \brief Print a solid pattern circle to Postscript document.
 *  \par Function Description
 *  The function prints a filled circle with a solid pattern.
 *  No outline is printed. 
 *  The circle is defined by the coordinates of its center in
 *  (<B>x</B>,<B>y</B>) and its radius given by the <B>radius</B> parameter. 
 *  The postscript file is defined by the file pointer <B>fp</B>.
 *  <B>fill_width</B>, <B>angle1</B> and <B>pitch1</B>, <B>angle2</B>
 *  and <B>pitch2</B> parameters are ignored in this functions but
 *  kept for compatibility with other fill functions.
 *
 *  All dimensions are in mils (except <B>angle1</B> and <B>angle2</B> in degree). 
 *
 *  \param [in] w_current   The TOPLEVEL object.
 *  \param [in] fp          FILE pointer to Postscript document.
 *  \param [in] x           Center x coordinate of circle.
 *  \param [in] y           Center y coordinate of circle.
 *  \param [in] radius      Radius of circle.
 *  \param [in] color       Circle color.
 *  \param [in] fill_width  Circle fill width. (unused).
 *  \param [in] angle1      (unused).
 *  \param [in] pitch1      (unused).
 *  \param [in] angle2      (unused).
 *  \param [in] pitch2      (unused).
 *  \param [in] origin_x    Page x coordinate to place circle OBJECT.
 *  \param [in] origin_y    Page y coordinate to place circle OBJECT.
 */
void o_circle_print_filled(TOPLEVEL *w_current, FILE *fp,
			   int x, int y, int radius,
			   int color,
			   int fill_width,
			   int angle1, int pitch1,
			   int angle2, int pitch2,
			   int origin_x, int origin_y)
{
  if (w_current->print_color) {
    f_print_set_color(fp, color);
  }

  fprintf(fp, "%d %d %d dot\n",
	  x-origin_x, y-origin_y,
	  radius);
	
}

/*! \brief Print a mesh pattern circle to Postscript document.
 *  \par Function Description
 *  This function prints a meshed circle. No outline is printed. 
 *  The circle is defined by the coordinates of its center in
 *  (<B>x</B>,<B>y</B>) and its radius by the <B>radius</B> parameter. 
 *  The Postscript document is defined by the file pointer <B>fp</B>. 
 *
 *  The inside mesh is achieved by two successive call to the
 *  #o_circle_print_hatch() function, given <B>angle1</B> and <B>pitch1</B>
 *  the first time and <B>angle2</B> and <B>pitch2</B> the second time.
 *
 *  Negative or null values for <B>pitch1</B> and/or <B>pitch2</B> are
 *  not allowed as it leads to an endless loop in #o_circle_print_hatch().
 *
 *  All dimensions are in mils (except <B>angle1</B> and <B>angle2</B> in degree).
 *
 *  \param [in] w_current   The TOPLEVEL object.
 *  \param [in] fp          FILE pointer to Postscript document.
 *  \param [in] x           Center x coordinate of circle.
 *  \param [in] y           Center y coordinate of circle.
 *  \param [in] radius      Radius of circle.
 *  \param [in] color       Circle color.
 *  \param [in] fill_width  Circle fill width.
 *  \param [in] angle1      1st angle for mesh pattern.
 *  \param [in] pitch1      1st pitch for mesh pattern.
 *  \param [in] angle2      2nd angle for mesh pattern.
 *  \param [in] pitch2      2nd pitch for mesh pattern.
 *  \param [in] origin_x    Page x coordinate to place circle OBJECT.
 *  \param [in] origin_y    Page y coordinate to place circle OBJECT.
 */
void o_circle_print_mesh(TOPLEVEL *w_current, FILE *fp,
			 int x, int y, int radius,
			 int color,
			 int fill_width,
			 int angle1, int pitch1,
			 int angle2, int pitch2,
			 int origin_x, int origin_y)
{
  o_circle_print_hatch(w_current, fp,
                       x, y, radius,
                       color,
                       fill_width,
                       angle1, pitch1,
                       -1, -1,
                       origin_x, origin_y);
  o_circle_print_hatch(w_current, fp,
                       x, y, radius,
                       color,
                       fill_width,
                       angle2, pitch2,
                       -1, -1,
                       origin_x, origin_y);
	
}

/*! \brief Print a hatch pattern circle to Postscript document.
 *  \par Function Description
 *  The function prints a hatched circle. No outline is printed. 
 *  The circle is defined by the coordinates of its center in
 *  (<B>x</B>,<B>y</B>) and its radius by the <B>radius</B> parameter. 
 *  The Postscript document is defined by the file pointer <B>fp</B>. 
 *  <B>angle2</B> and <B>pitch2</B> parameters are ignored in this
 *  functions but kept for compatibility with other fill functions.
 *
 *  The only attribute of line here is its width from the parameter <B>width</B>.
 *
 *  Negative or null values for <B>pitch1</B> is not allowed as it
 *  leads to an endless loop.
 *
 *  All dimensions are in mils (except <B>angle1</B> is in degrees).
 *
 *  \param [in] w_current   The TOPLEVEL object.
 *  \param [in] fp          FILE pointer to Postscript document.
 *  \param [in] x           Center x coordinate of circle.
 *  \param [in] y           Center y coordinate of circle.
 *  \param [in] radius      Radius of circle.
 *  \param [in] color       Circle color.
 *  \param [in] fill_width  Circle fill width.
 *  \param [in] angle1      Angle for hatch pattern.
 *  \param [in] pitch1      Pitch for hatch pattern.
 *  \param [in] angle2      (unused).
 *  \param [in] pitch2      (unused).
 *  \param [in] origin_x    Page x coordinate to place circle OBJECT.
 *  \param [in] origin_y    Page y coordinate to place circle OBJECT.
 */
void o_circle_print_hatch(TOPLEVEL *w_current, FILE *fp,
			  int x, int y, int radius,
			  int color,
			  int fill_width,
			  int angle1, int pitch1,
			  int angle2, int pitch2,
			  int origin_x, int origin_y)
{
  double x0, y0, x1, y1, x2, y2;
  double cos_a_, sin_a_;

  if (w_current->print_color) {
    f_print_set_color(fp, color);
  }

  /* 
   * The values of the cosinus and sinus of the angle
   * <B>angle1</B> are calculated for future usage (repetitive).
   */
  cos_a_ = cos(((double) angle1) * M_PI/180);
  sin_a_ = sin(((double) angle1) * M_PI/180);

  /*
   * When printing a line in a circle there is two intersections.
   * It looks for the coordinates of one of these points when the
   * line is horizontal. The second one can be easily obtained by
   * symmetry in relation to the vertical axis going through the
   * centre of the circle.
   *
   * These two points are therefore rotated of angle <B>angle1</B>
   * using the elements previously computed.
   *
   * The corresponding line can be printed providing that the
   * coordinates are rounded.
   *
   * These operations are repeated for every horizontal line that
   * can fit in the upper half of the circle (using and incrementing
   * the variable #y0).
   */
  y0 = 0;
  while(y0 < (double) radius) {
    x0 = pow((double) radius, 2) - pow(y0, 2);
    x0 = sqrt(x0);

    x1 = (x0*cos_a_ - y0*sin_a_) + x;
    y1 = y + (x0*sin_a_ + y0*cos_a_);
    x2 = ((-x0)*cos_a_ - y0*sin_a_) + x;
    y2 = y + ((-x0)*sin_a_ + y0*cos_a_);

    fprintf(fp, "%d %d %d %d %d line\n",
	    (int) x1, (int) y1, (int) x2, (int) y2, fill_width);

    /*
     * The function uses the symetry in relation to the centre of the
     * circle. It avoid repetitive computation for the second half of
     * the surface of the circle.
     */
    x1 = x + (x0*cos_a_ - (-y0)*sin_a_);
    y1 = y + (x0*sin_a_ + (-y0)*cos_a_);
    x2 = x + ((-x0)*cos_a_ - (-y0)*sin_a_);
    y2 = y + ((-x0)*sin_a_ + (-y0)*cos_a_);
    
    fprintf(fp, "%d %d %d %d %d line\n",
	    (int) x1, (int) y1, (int) x2, (int) y2, fill_width);
    
    y0 = y0 + pitch1;
  }
}

#if 0 /* original way of printing circle, no longer used */
/*! \brief Print Circle to Postscript document using old method.
 *  \par Function Description
 *  This function is the old function to print a circle.
 *  It does not handle line type and filling of a circle.
 *
 *  \param [in] w_current  The TOPLEVEL object.
 *  \param [in] fp         FILE pointer to Postscript document.
 *  \param [in] o_current  Circle object to print.
 *  \param [in] origin_x   Page x coordinate to place circle OBJECT.
 *  \param [in] origin_y   Page x coordinate to place circle OBJECT.
 */
void o_circle_print_old(TOPLEVEL *w_current, FILE *fp, OBJECT *o_current,
			int origin_x, int origin_y)
{
  if (o_current == NULL) {
    printf("got null in o_circle_print\n");
    return;
  }

  o_arc_print_solid(w_current, fp,
                    o_current->circle->center_x, 
		    o_current->circle->center_y,
		    o_current->circle->radius,
                    0, FULL_CIRCLE / 64,
                    o_current->color),
                    o_current->line_width, -1, -1,
                    origin_x, origin_y);

}
#endif

/*! \brief Draw a circle in an image.
 *  \par Function Description
 *  This function draws a circle in an image with the libgdgeda function
 *  #gdImageArc().
 *
 *  \param [in] w_current   The TOPLEVEL object.
 *  \param [in] o_current   Circle OBJECT to draw.
 *  \param [in] origin_x    (unused).
 *  \param [in] origin_y    (unused).
 *  \param [in] color_mode  Draw circle in color if TRUE, B/W otherwise.
 */
void o_circle_image_write(TOPLEVEL *w_current, OBJECT *o_current,
			  int origin_x, int origin_y, int color_mode)
{
  int color;

  if (o_current == NULL) {
    printf("got null in o_circle_image_write\n");
    return;
  }

  if (color_mode == TRUE) {
    color = o_image_geda2gd_color(o_current->color);
  } else {
    color = image_black;
  }

#ifdef HAS_LIBGDGEDA

  gdImageSetThickness(current_im_ptr, SCREENabs(w_current,
                                                o_current->line_width));

  gdImageArc(current_im_ptr, 
             o_current->circle->screen_x, 
             o_current->circle->screen_y,
             SCREENabs(w_current, o_current->circle->radius)*2,
             SCREENabs(w_current, o_current->circle->radius)*2,
             0, 360, 
             color);
#endif
}