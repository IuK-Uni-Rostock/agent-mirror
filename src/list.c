#include "list.h"
#include "../lib/kdriveExpress-17.2.0-raspbian/include/kdrive_express_logger.h"
#include <stdlib.h>

void list_insert(flow_record_node *head, flow_record *flow)
{
	// first insert
	if (head->flow == NULL)
	{
		head->flow = flow;
		return;
	}

	// search for existing flow record
	flow_record_node *cursor = list_search(head, flow);
	
	// flow record found
	if (cursor != NULL)
	{
		if (flow_update(cursor->flow, flow))
		{
			flow_free(flow);
			return;
		}
	}

	// go to the last node
	cursor = head;
	while (cursor->next != NULL)
		cursor = cursor->next;

	// create a new node and insert at the end of the list
	flow_record_node *new_node = list_create(NULL, flow);
	if (new_node != NULL)
		cursor->next = new_node;
}

flow_record_node *list_create(flow_record_node *next, flow_record *flow)
{
	flow_record_node *new_node = calloc(1, sizeof(flow_record_node));

	if (new_node == NULL)
	{
		kdrive_logger(KDRIVE_LOGGER_FATAL, "Failed to allocate new flow record");
		exit(EXIT_FAILURE);
	}

	new_node->flow = flow;
	new_node->next = next;

	return new_node;
}

flow_record_node *list_search(flow_record_node *head, flow_record *flow)
{
	flow_record_node *cursor = head;
	while (cursor != NULL)
	{
		// do not modify expired flows
		if (!cursor->flow->is_expired && flow_is_equal(cursor->flow, flow))
			return cursor;
		cursor = cursor->next;
	}
	return NULL;
}

void list_traverse(flow_record_node *head, flow_record_callback f)
{
	flow_record_node *cursor = head;
	while (cursor != NULL)
	{
		f(cursor->flow);
		cursor = cursor->next;
	}
}

uint16_t list_count(flow_record_node *head)
{
	flow_record_node *cursor = head;
	uint16_t c = 0;

	while (cursor != NULL)
	{
		c++;
		cursor = cursor->next;
	}

	return c;
}

void list_dispose(flow_record_node *head)
{
	flow_record_node *cursor, *tmp;

	if (head->flow != NULL)
	{
		cursor = head->next;

		while (cursor != NULL)
		{
			tmp = cursor->next;
			flow_free(cursor->flow);
			free(cursor);
			cursor = tmp;
		}

		// clean up head
		flow_free(head->flow);
		head->flow = NULL;
		head->next = NULL;
	}
}

void list_dispose_expired(flow_record_node *head)
{
	flow_record_node *cursor, *prev;

	if (head->flow == NULL) return;
	
	// start at second node in list
	cursor = head->next;

	// save head
	prev = head;

	while (cursor != NULL)
	{
		// find expired flow record
		while (cursor != NULL && !cursor->flow->is_expired)
		{
			prev = cursor;
			cursor = cursor->next;
		}

		if (cursor == NULL) continue;

		// free expired flow record and remove node
		prev->next = cursor->next;
		flow_free(cursor->flow);
		free(cursor);
		cursor = prev->next;
	}

	// head node can not be freed since it's statically allocated, so we update head with last node in list and delete the last node afterwards
	if (head->flow->is_expired)
	{
		cursor = head;

		// check if list contains only one item
		if (cursor->next != NULL)
		{
			// find last node, save last node - 1
			while (cursor->next != NULL)
			{
				prev = cursor;
				cursor = cursor->next;
			}

			// update head node and free last node
			prev->next = NULL;
			flow_free(head->flow);
			head->flow = cursor->flow;
			free(cursor);
		}
		else
		{
			// head is the only node
			flow_free(head->flow);
			head->flow = NULL;
			head->next = NULL;
		}
	}
}
