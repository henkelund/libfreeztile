/* Tests for `mod.h' interface.
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

#include <check.h>
#include <stdlib.h>
#include <time.h>
#include "malloc.h"
#include "mod.h"
#include "private-mod.h"
#include "class.h"
#include "voice.h"

/* Sample subclass of `mod_c'.  */
typedef struct sample_mod_s
{
  mod_t __parent;
} sample_mod_t;

/* `fz_mod_render' callback.  */
static int_t
sample_mod_renderer (mod_t *mod, const voice_t *voice)
{
  uint_t i;
  size_t nframes = fz_len (mod->stepbuf);
  real_t step = 1. / nframes;

  for (i = 0; i < nframes; ++i)
    fz_val_at (mod->stepbuf, i, real_t) = step * i;

  return nframes;
}

static ptr_t
sample_mod_constructor (ptr_t ptr, va_list *args)
{
  sample_mod_t *self = (sample_mod_t *)
    ((const class_t *) mod_c)->construct (ptr, args);
  self->__parent.render = sample_mod_renderer;
  return self;
}

static ptr_t
sample_mod_destructor (ptr_t ptr)
{
  sample_mod_t *self = (sample_mod_t *)
    ((const class_t *) mod_c)->destruct (ptr);
  return self;
}

static const class_t _sample_mod_c = {
  sizeof (sample_mod_t),
  sample_mod_constructor,
  sample_mod_destructor,
  NULL,
  NULL,
  NULL
};

const class_t *sample_mod_c = &_sample_mod_c;
/* End of `mod_c' sample subclass.  */

/* `mod_c' instance instantiated in `setup'.  */
mod_t *modulator = NULL;

/* Pre-test hook.  */
void
setup ()
{
  ck_assert_int_eq (fz_memusage (0), 0);
  modulator = fz_new (sample_mod_c);
}

/* Post-test hook.  */
void
teardown ()
{
  ck_assert (fz_del (modulator) == 0);
  ck_assert_int_eq (fz_memusage (0), 0);
}

/* Test `fz_mod_state_data' using shorthand macro `fz_mod_state'.  */
START_TEST (test_fz_mod_state_data)
{
  voice_t *voice1 = fz_new (voice_c);
  voice_t *voice2 = fz_new (voice_c);
  int_t *state1, *state2, randint;

  srand (time (0));
  randint = rand ();

  state1 = fz_mod_state (NULL, voice1, int_t);
  fail_unless (state1 == NULL,
               "NULL modulator should yield NULL state.");
  state1 = fz_mod_state (modulator, NULL, int_t);
  fail_unless (state1 == NULL,
               "NULL voice should yield NULL state.");
  state1 = fz_mod_state_data (modulator, voice1, 0);
  fail_unless (state1 == NULL,
               "State should not be allocated given zero size.");

  state1 = fz_mod_state (modulator, voice1, int_t);
  fail_unless (state1 != NULL,
               "non-NULL modulator should not yield NULL state.");
  fail_unless (*state1 == 0,
               "Uninitialized state should be zero, %d found.", *state1);

  state2 = fz_mod_state (modulator, voice2, int_t);
  fail_unless (state2 != state1,
               "States for different voices should not point to the "
               "same data.");

  *state1 = randint;
  state1 = fz_mod_state (modulator, voice1, int_t);
  fail_unless (*state1 == randint,
               "Assigned value was not returned on subsequent call. "
               "%d assigned, %d returned", randint, *state1);

  fz_del (voice2);
  fz_del (voice1);
}
END_TEST

/* Test for `fz_mod_render'.  */
START_TEST (test_fz_mod_render)
{
  const list_t *modbuf;
  size_t nframes = 10;
  int_t err;
  uint_t i;
  real_t step, prev;
  voice_t *voice = fz_new (voice_c);

  fz_mod_prepare (modulator, nframes);
  err = fz_mod_render (modulator, voice);
  fail_unless (err == nframes,
               "Frames rendered should be %u, found %d.",
               nframes, err);

  modbuf = fz_modulate_unorm (modulator, 1);
  fail_unless (err == nframes,
               "Modulated frames should be %u, found %d.",
               nframes, err);

  prev = -1;
  for (i = 0; i < nframes; ++i)
    {
      step = fz_val_at (modbuf, i, real_t);
      fail_unless (step > prev,
                   "Expected %f to be bigger than %f but it was not.",
                   step, prev);
      prev = step;
    }

  fz_mod_prepare (modulator, nframes);
  err = fz_mod_render (NULL, voice);
  fail_unless (err < 0,
               "Expected renderer to return a negative error code "
               "when passed a NULL modulator but %d was returned.",
               err);

  fz_del (voice);
}
END_TEST

/* Initiate a modulator test suite struct.  */
Suite *
mod_suite_create ()
{
  Suite *s = suite_create ("modulator");
  TCase *t = tcase_create ("modulator");
  tcase_add_checked_fixture (t, setup, teardown);
  tcase_add_test (t, test_fz_mod_state_data);
  tcase_add_test (t, test_fz_mod_render);
  suite_add_tcase (s, t);
  return s;
}

/* Run all modulator tests.  */
int
main ()
{
  int fail_count = 0;
  Suite *suite = mod_suite_create ();
  SRunner *runner = srunner_create (suite);
  srunner_run_all (runner, CK_NORMAL);
  fail_count = srunner_ntests_failed (runner);
  srunner_free (runner);
  free (suite);
  return fail_count == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
