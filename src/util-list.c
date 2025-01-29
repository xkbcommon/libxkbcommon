/*
 * Copyright © 2008-2011 Kristian Høgsberg
 * Copyright © 2011 Intel Corporation
 * Copyright © 2013-2015 Red Hat, Inc.
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <stddef.h>
#include <stdbool.h>

#include "util-list.h"

void
list_init(struct list *list)
{
	list->prev = list;
	list->next = list;
}

void
list_insert(struct list *list, struct list *elm)
{
	assert((list->next != NULL && list->prev != NULL) ||
	       !"list->next|prev is NULL, possibly missing list_init()");
	assert(((elm->next == NULL && elm->prev == NULL) || list_empty(elm)) ||
	       !"elm->next|prev is not NULL, list node used twice?");

	elm->prev = list;
	elm->next = list->next;
	list->next = elm;
	elm->next->prev = elm;
}

void
list_append(struct list *list, struct list *elm)
{
	assert((list->next != NULL && list->prev != NULL) ||
	       !"list->next|prev is NULL, possibly missing list_init()");
	assert(((elm->next == NULL && elm->prev == NULL) || list_empty(elm)) ||
	       !"elm->next|prev is not NULL, list node used twice?");

	elm->next = list;
	elm->prev = list->prev;
	list->prev = elm;
	elm->prev->next = elm;
}

void
list_remove(struct list *elm)
{
	assert((elm->next != NULL && elm->prev != NULL) ||
	       !"list->next|prev is NULL, possibly missing list_init()");

	elm->prev->next = elm->next;
	elm->next->prev = elm->prev;
	elm->next = NULL;
	elm->prev = NULL;
}

bool
list_empty(const struct list *list)
{
	assert((list->next != NULL && list->prev != NULL) ||
	       !"list->next|prev is NULL, possibly missing list_init()");

	return list->next == list;
}

bool
list_is_last(const struct list *list, const struct list *elm)
{
	return elm->next == list;
}
