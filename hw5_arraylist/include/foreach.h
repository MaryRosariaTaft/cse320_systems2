#ifndef FOREACH_H
#define FOREACH_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "const.h"

#define foreach(type, value, items) for ( type *value = foreach_init(items)  \
                                        ; value ;                            \
                                        value = foreach_next(items, value))

#define foreach_break if( foreach_break_f() ) break

void *foreach_init(arraylist_t *items);

void *foreach_next(arraylist_t *items, void *data);

size_t foreach_index();

bool foreach_break_f();

int apply(arraylist_t *items, int (*application)(void *));

#endif
