#ifndef FLOW_RECORD_H
#define FLOW_RECORD_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// flow expiration in seconds
#define FLOW_EXPIRATION (120)

typedef struct flow_record flow_record;

struct flow_record
{
	bool     is_expired;
	bool     is_group_address;
	uint16_t src;
	uint16_t dest;
	uint16_t apci;
	uint8_t  priority;
	uint8_t  hop_count;
	uint32_t start_time;
	uint16_t duration;
	uint16_t telegram_count;
	uint16_t byte_count;
};

flow_record *flow_create(bool is_group_address, uint16_t src, uint16_t dest, uint16_t apci, uint8_t priority, uint8_t hop_count, uint32_t start_time, uint16_t byte_count);
void flow_free(flow_record *flow);
bool flow_update(flow_record *flow_to_update, flow_record *flow);
void flow_print(flow_record *flow);
bool flow_is_equal(flow_record *flow1, flow_record* flow2);
bool flow_is_expired(flow_record *flow, uint32_t current_time);

#endif
