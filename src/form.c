/* Form node implementation.
   Copyright (C) 2013-2014 Henrik Hedelund.

   This file is part of libfreeztile.

   libfreeztile is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   libfreeztile is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License
   along with libfreeztile; see the file COPYING.  If not see
   <http://www.gnu.org/licenses/>.  */

#include <math.h>
#include <errno.h>
#include "form.h"
#include "node.h"
#include "private-node.h"
#include "list.h"

#define SHAPE_SIZE 4096
#define TWO_PI 6.28318530718

/* Form class struct.  */
typedef struct form_s
{
  node_t __parent;
  list_t *shape;
  real_t shifting;
} form_t;

/* Struct to keep track of individual voice states.  */
struct state_s
{
  real_t pos;
};

/* Form node renderer.  */
static int_t
form_render (node_t *node, const request_t *request)
{
  form_t *form = (form_t *) node;
  uint_t i = 0;
  size_t nframes = fz_len (node->framebuf);
  real_t *frames = fz_list_data (node->framebuf);
  real_t *formdata = fz_list_data (form->shape);
  size_t period = fz_len (form->shape);
  real_t pos;
  real_t shift;
  real_t freq;
  real_t fwidth;
  struct state_s *state;
  const real_t *amoddata;

  /* Frequency modulation vars.  */
  real_t fupper, flower;
  real_t fmoddepth;
  real_t *fmodarg;
  const real_t *fmoddata;

  if (period == 0 || request->voice == NULL || request->srate <= 0)
    return 0;

  freq = fz_voice_frequency (request->voice);
  state = fz_node_state (node, request->voice, struct state_s);
  if (freq <= 0 || state == NULL)
    return 0;

  /* Modulate amplitude.  */
  amoddata = fz_node_modulate_unorm (node, FORM_SLOT_AMP, 1);

  /* Modulate frequency.  */
  fmodarg = fz_node_modargs (node, FORM_SLOT_FREQ);
  fmoddepth = fmodarg ? *fmodarg : 1;
  fupper = (freq * pow (TWELFTH_ROOT_OF_TWO, fmoddepth)) - freq;
  flower = (freq * pow (TWELFTH_ROOT_OF_TWO, -fmoddepth)) - freq;
  fmoddata = fz_node_modulate (node, FORM_SLOT_FREQ, 1,
                               1. / (request->srate / flower),
                               1. / (request->srate / fupper));

  fwidth = 1. / (request->srate / freq);
  for (; i < nframes; ++i)
    {
      pos = state->pos;

      if (form->shifting != .5) /* Shifting is not centered.  */
        {
          if (period == 2)
            /* Special case for SQUARE since peak shifting
               has no effect when there are no slopes.  */
            pos = pos < form->shifting ? 0 : 0.5;
          else
            {
              /* Shifting assumes that the shape alignment puts its
                 peak and trough at .25 and .75, respectively.  */
              shift = form->shifting / 2;
              if (pos < shift)
                pos = pos / shift * .25;
              else if (pos < .5)
                pos = (pos - shift) / (.5 - shift) * .25 + .25;
              else if (pos < 1 - shift)
                pos = (pos - .5) / (.5 - shift) * .25 + .5;
              else
                pos = ((pos - (1 - shift)) / shift) * .25 + .75;
            }
        }

      frames[i] += formdata[(uint_t) (pos * period) % period]
        * (amoddata ? amoddata[i] : 1);

      state->pos += fmoddata ? fmoddata[i] + fwidth : fwidth;
      while (state->pos >= 1)
        state->pos -= 1;
    }

  return nframes;
}

/* Form constructor.  */
static ptr_t
form_constructor (ptr_t ptr, va_list *args)
{
  form_t *self = (form_t *)
    ((const class_t *) node_c)->construct (ptr, args);
  int_t shape = va_arg (*args, int_t);

  self->__parent.render = form_render;
  self->__parent.flags |= NODE_PRODUCER;
  self->shape = fz_new_simple_vector (real_t);
  self->shifting = 0.5;
  fz_form_set_shape (self, shape);

  return self;
}

/* Form destructor.  */
static ptr_t
form_destructor (ptr_t ptr)
{
  form_t *self = (form_t *)
    ((const class_t *) node_c)->destruct (ptr);
  fz_del (self->shape);
  return self;
}

/* Set shape to be used for `fz_node_render'.  */
int_t
fz_form_set_shape (form_t *form, int_t shape)
{
  uint_t i;
  size_t shape_size;
  uint_t offset; /* Used to align triangle with sine and square.  */

  if (form == NULL)
    return -EINVAL;

  shape_size = (shape == SHAPE_SQUARE) ? 2 : SHAPE_SIZE;
  fz_clear (form->shape, shape_size);

  switch (shape)
    {
    case SHAPE_SINE:
      for (i = 0; i < shape_size; ++i)
        fz_val_at (form->shape, i, real_t)
          = sin (TWO_PI * ((real_t) i) / shape_size);
      break;
    case SHAPE_TRIANGLE:
      for (i = 0; i < shape_size; ++i)
        {
          offset = ((i - (shape_size / 4)) + shape_size) % shape_size;
          fz_val_at (form->shape, i, real_t)
            = (fabs ((((real_t) offset * 4) / shape_size) - 2) - 1);
        }
      break;
    case SHAPE_SQUARE:
      for (i = 0; i < shape_size; ++i)
        fz_val_at (form->shape, i, real_t)
          = i < (shape_size / 2) ? 1 : -1;
          break;
    default:
      return -EINVAL;
    }

  return shape;
}

/* `form_c' class descriptor.  */
static const class_t _form_c = {
  sizeof (form_t),
  form_constructor,
  form_destructor,
  NULL,
  NULL,
  NULL
};

const class_t *form_c = &_form_c;
