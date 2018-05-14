#ifndef EXPORT_H
#define EXPORT_H

#include "flow.h"

// increase in case of new version, maximum: 15 (4 bit)
#define EXPORT_VERSION (1)

// export options as unique bits in 1 byte bitmask
#define EXPORT_APCI			(0x01)
#define EXPORT_PRIORITY			(0x02)
#define EXPORT_HOP_COUNT		(0x04)
#define EXPORT_START_TIME		(0x08)
#define EXPORT_DURATION			(0x10)
#define EXPORT_TELEGRAM_COUNT		(0x20)
#define EXPORT_BYTE_COUNT		(0x40)
//#define EXPORT_xxx			(0x80)

#define EXPORT_HEADER_SIZE (2)

typedef struct flow_export flow_export;

struct flow_export
{
	uint8_t *payload;
	uint8_t size;
};

uint8_t export_calculate_payload_size(uint8_t options);
flow_export *export_create_payload(flow_record *flow, uint8_t options, uint32_t current_timestamp);
flow_export *export_create_long_payload(flow_record *flow, uint8_t options, uint8_t amount, uint8_t *offset, uint32_t current_timestamp);
void export_append_long_payload(flow_record *flow, flow_export *payload, uint8_t options, uint8_t *offset, uint32_t current_timestamp);
void export_free(flow_export *flow);

#endif
