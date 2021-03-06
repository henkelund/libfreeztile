/* Header file for list types functions interface.
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

#ifndef FZ_LIST_H
#define FZ_LIST_H 1

#include "class.h"

__BEGIN_DECLS

typedef struct list_s list_t;

#define LISTOPT_NONE 0
#define LISTOPT_PTRS (1 << 0)
#define LISTOPT_KEEP ((1 << 1) | LISTOPT_PTRS)
#define LISTOPT_PASS ((1 << 2) | LISTOPT_PTRS)

#define fz_new_owning_vector(type) \
  fz_new_vector (type, LISTOPT_PASS)

#define fz_new_retaining_vector(type) \
  fz_new_vector (type, LISTOPT_KEEP)

#define fz_new_pointer_vector(type) \
  fz_new_vector (type, LISTOPT_PTRS)

#define fz_new_simple_vector(type) \
  fz_new_vector (type, LISTOPT_NONE)

#define fz_new_vector(type, flags) \
  fz_new (vector_c, sizeof (type), #type, flags)

#define fz_ref_at(list, index, type) \
  ((type *) fz_at (list, index))

#define fz_val_at(list, index, type) \
  (*((type *) fz_at (list, index)))

#define fz_insert_one(list, index, item) \
  fz_insert (list, index, 1, item)

#define fz_push(list, num, item) \
  fz_insert (list, fz_len (list), num, item)

#define fz_push_one(list, item) \
  fz_insert (list, fz_len (list), 1, item)

#define fz_erase_one(list, index) \
  fz_erase (list, index, 1)

#define fz_list_data(list) \
  (fz_instance_of ((const ptr_t) list, vector_c) == TRUE \
   ? fz_at (list, 0) : NULL)

extern ptr_t fz_at (const list_t *, uint_t);
extern int_t fz_insert (list_t *, uint_t, uint_t, ptr_t);
extern int_t fz_erase (list_t *, uint_t, uint_t);
extern int_t fz_clear (list_t *, size_t);
extern int_t fz_index_of (const list_t *, const ptr_t, cmp_f);
extern int_t fz_sort (list_t *, cmp_f);

extern int_t fz_cmp_ptr (const ptr_t, const ptr_t);
extern int_t fz_cmp_int (const ptr_t, const ptr_t);
extern int_t fz_cmp_real (const ptr_t, const ptr_t);

extern const class_t *vector_c;

__END_DECLS

#endif /* ! FZ_LIST_H */

