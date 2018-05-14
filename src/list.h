#ifndef LIST_H
#define LIST_H

#include "flow.h"

typedef struct flow_record_node flow_record_node;

struct flow_record_node
{
	flow_record *flow;
	flow_record_node *next;
};

typedef void(*flow_record_callback)(flow_record *flow);

void list_insert(flow_record_node *head, flow_record *flow);
void list_traverse(flow_record_node *head, flow_record_callback f);
void list_dispose(flow_record_node *head);
void list_dispose_expired(flow_record_node *head);
uint16_t list_count(flow_record_node *head);
flow_record_node *list_create(flow_record_node *next, flow_record *flow);
flow_record_node *list_search(flow_record_node *head, flow_record *flow);

#endif
