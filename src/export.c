#include "export.h"
#include "../lib/kdriveExpress-17.2.0-raspbian/include/kdrive_express_logger.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

void check_flow_expiration(flow_record *flow, uint32_t current_timestamp)
{
	if (!flow->is_expired)
	{
		flow->is_expired = flow_is_expired(flow, current_timestamp);
	}
}

uint8_t export_calculate_payload_size(uint8_t options)
{
	// attributes, src & dest address are required
	uint8_t size = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t);

	if ((options & EXPORT_APCI) == EXPORT_APCI)
		size += sizeof(uint16_t);

	if ((options & EXPORT_PRIORITY) == EXPORT_PRIORITY)
		size += sizeof(uint8_t);

	if ((options & EXPORT_HOP_COUNT) == EXPORT_HOP_COUNT)
		size += sizeof(uint8_t);

	if ((options & EXPORT_START_TIME) == EXPORT_START_TIME)
		size += sizeof(uint32_t);

	if ((options & EXPORT_DURATION) == EXPORT_DURATION)
		size += sizeof(uint16_t);

	if ((options & EXPORT_TELEGRAM_COUNT) == EXPORT_TELEGRAM_COUNT)
		size += sizeof(uint16_t);

	if ((options & EXPORT_BYTE_COUNT) == EXPORT_BYTE_COUNT)
		size += sizeof(uint16_t);

	return size;
}

flow_export *export_create_long_payload(flow_record *flow, uint8_t options, uint8_t amount, uint8_t *offset, uint32_t current_timestamp)
{
	check_flow_expiration(flow, current_timestamp);

	uint8_t size = EXPORT_HEADER_SIZE + (export_calculate_payload_size(options) * amount);

	uint8_t *payload = calloc(1, size);

	if (payload == NULL)
	{
		kdrive_logger(KDRIVE_LOGGER_FATAL, "Failed to allocate new payload for flow export");
		exit(EXIT_FAILURE);
	}

	/*
	*             Header
	* ---------------------------------
	* | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
	* ---------------------------------
	* | U | U | U | U | V | V | V | V |
	*
	* Bit 0 - 3 : version
	* Bit 4 - 7 : unused
	*/

	uint8_t version = EXPORT_VERSION;

	memset(payload + *offset, version, sizeof(version));
	*offset += sizeof(version);

	memset(payload + *offset, options, sizeof(options));
	*offset += sizeof(options);

	/*
	*           Attributes
	* ---------------------------------
	* | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
	* ---------------------------------
	* | U | U | U | U | V | V | G | E |
	*
	* Bit 0     : is expired
	* Bit 1     : is group address
	* Bit 2 - 7 : unused
	*/

	uint8_t attributes = 0;

	attributes = (uint8_t)flow->is_expired;
	attributes ^= (-(uint8_t)(flow->is_group_address) ^ attributes) & (1 << 1);

	memset(payload + *offset, attributes, sizeof(attributes));
	*offset += sizeof(attributes);

	uint16_t src = htons(flow->src);
	memcpy(payload + *offset, &src, sizeof(src));
	*offset += sizeof(src);

	uint16_t dest = htons(flow->dest);
	memcpy(payload + *offset, &dest, sizeof(dest));
	*offset += sizeof(dest);

	if ((options & EXPORT_APCI) == EXPORT_APCI)
	{
		uint16_t apci = htons(flow->apci);
		memcpy(payload + *offset, &apci, sizeof(apci));
		*offset += sizeof(apci);
	}

	if ((options & EXPORT_PRIORITY) == EXPORT_PRIORITY)
	{
		memset(payload + *offset, flow->priority, sizeof(flow->priority));
		*offset += sizeof(flow->priority);
	}

	if ((options & EXPORT_HOP_COUNT) == EXPORT_HOP_COUNT)
	{
		memset(payload + *offset, flow->hop_count, sizeof(flow->hop_count));
		*offset += sizeof(flow->hop_count);
	}

	if ((options & EXPORT_START_TIME) == EXPORT_START_TIME)
	{
		uint32_t start_time = htonl(flow->start_time);
		memcpy(payload + *offset, &start_time, sizeof(start_time));
		*offset += sizeof(start_time);
	}

	if ((options & EXPORT_DURATION) == EXPORT_DURATION)
	{
		uint16_t duration = htons(flow->duration);
		memcpy(payload + *offset, &duration, sizeof(duration));
		*offset += sizeof(duration);
	}

	if ((options & EXPORT_TELEGRAM_COUNT) == EXPORT_TELEGRAM_COUNT)
	{
		uint16_t telegram_count = htons(flow->telegram_count);
		memcpy(payload + *offset, &telegram_count, sizeof(telegram_count));
		*offset += sizeof(telegram_count);
	}

	if ((options & EXPORT_BYTE_COUNT) == EXPORT_BYTE_COUNT)
	{
		uint16_t byte_count = htons(flow->byte_count);
		memcpy(payload + *offset, &byte_count, sizeof(byte_count));
		*offset += sizeof(byte_count);
	}

	flow_export *f = calloc(1, sizeof(flow_export));
	if (f == NULL)
	{
		kdrive_logger(KDRIVE_LOGGER_FATAL, "Failed to allocate new flow export");
		exit(EXIT_FAILURE);
	}

	f->payload = payload;
	f->size = size;

	return f;
}

void export_append_long_payload(flow_record *flow, flow_export *payload, uint8_t options, uint8_t *offset, uint32_t current_timestamp)
{
	check_flow_expiration(flow, current_timestamp);

	/*
	*           Attributes
	* ---------------------------------
	* | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
	* ---------------------------------
	* | U | U | U | U | V | V | G | E |
	*
	* Bit 0     : is expired
	* Bit 1     : is group address
	* Bit 2 - 7 : unused
	*/

	uint8_t attributes = 0;

	attributes = (uint8_t)flow->is_expired;
	attributes ^= (-(uint8_t)(flow->is_group_address) ^ attributes) & (1 << 1);

	memset(payload->payload + *offset, attributes, sizeof(attributes));
	*offset += sizeof(attributes);

	uint16_t src = htons(flow->src);
	memcpy(payload->payload + *offset, &src, sizeof(src));
	*offset += sizeof(src);

	uint16_t dest = htons(flow->dest);
	memcpy(payload->payload + *offset, &dest, sizeof(dest));
	*offset += sizeof(dest);

	if ((options & EXPORT_APCI) == EXPORT_APCI)
	{
		uint16_t apci = htons(flow->apci);
		memcpy(payload->payload + *offset, &apci, sizeof(apci));
		*offset += sizeof(apci);
	}

	if ((options & EXPORT_PRIORITY) == EXPORT_PRIORITY)
	{
		memset(payload->payload + *offset, flow->priority, sizeof(flow->priority));
		*offset += sizeof(flow->priority);
	}

	if ((options & EXPORT_HOP_COUNT) == EXPORT_HOP_COUNT)
	{
		memset(payload->payload + *offset, flow->hop_count, sizeof(flow->hop_count));
		*offset += sizeof(flow->hop_count);
	}

	if ((options & EXPORT_START_TIME) == EXPORT_START_TIME)
	{
		uint32_t start_time = htonl(flow->start_time);
		memcpy(payload->payload + *offset, &start_time, sizeof(start_time));
		*offset += sizeof(start_time);
	}

	if ((options & EXPORT_DURATION) == EXPORT_DURATION)
	{
		uint16_t duration = htons(flow->duration);
		memcpy(payload->payload + *offset, &duration, sizeof(duration));
		*offset += sizeof(duration);
	}

	if ((options & EXPORT_TELEGRAM_COUNT) == EXPORT_TELEGRAM_COUNT)
	{
		uint16_t telegram_count = htons(flow->telegram_count);
		memcpy(payload->payload + *offset, &telegram_count, sizeof(telegram_count));
		*offset += sizeof(telegram_count);
	}

	if ((options & EXPORT_BYTE_COUNT) == EXPORT_BYTE_COUNT)
	{
		uint16_t byte_count = htons(flow->byte_count);
		memcpy(payload->payload + *offset, &byte_count, sizeof(byte_count));
		*offset += sizeof(byte_count);
	}
}

// export one flow record per telegram (not aggregated)
flow_export *export_create_payload(flow_record *flow, uint8_t options, uint32_t current_timestamp)
{
	check_flow_expiration(flow, current_timestamp);

	uint8_t size = EXPORT_HEADER_SIZE + export_calculate_payload_size(options);

	uint8_t *payload = calloc(1, size);

	if (payload == NULL)
	{
		kdrive_logger(KDRIVE_LOGGER_FATAL, "Failed to allocate new payload for flow export");
		exit(EXIT_FAILURE);
	}

	/*
	*             Header
	* ---------------------------------
	* | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
	* ---------------------------------
	* | U | U | U | U | V | V | V | V |
	*
	* Bit 0 - 3 : version
	* Bit 4 - 7 : unused
	*/

	uint8_t version = EXPORT_VERSION;

	uint8_t offset = 0;
	memset(payload + offset, version, sizeof(version));
	offset += sizeof(version);

	memset(payload + offset, options, sizeof(options));
	offset += sizeof(options);

	/*
	*           Attributes
	* ---------------------------------
	* | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
	* ---------------------------------
	* | U | U | U | U | V | V | G | E |
	*
	* Bit 0     : is expired
	* Bit 1     : is group address
	* Bit 2 - 7 : unused
	*/

	uint8_t attributes = 0;

	attributes = (uint8_t)flow->is_expired;
	attributes ^= (-(uint8_t)(flow->is_group_address) ^ attributes) & (1 << 1);

	memset(payload + offset, attributes, sizeof(attributes));
	offset += sizeof(attributes);

	uint16_t src = htons(flow->src);
	memcpy(payload + offset, &src, sizeof(src));
	offset += sizeof(src);

	uint16_t dest = htons(flow->dest);
	memcpy(payload + offset, &dest, sizeof(dest));
	offset += sizeof(dest);

	if ((options & EXPORT_APCI) == EXPORT_APCI)
	{
		uint16_t apci = htons(flow->apci);
		memcpy(payload + offset, &apci, sizeof(apci));
		offset += sizeof(apci);
	}

	if ((options & EXPORT_PRIORITY) == EXPORT_PRIORITY)
	{
		memset(payload + offset, flow->priority, sizeof(flow->priority));
		offset += sizeof(flow->priority);
	}

	if ((options & EXPORT_HOP_COUNT) == EXPORT_HOP_COUNT)
	{
		memset(payload + offset, flow->hop_count, sizeof(flow->hop_count));
		offset += sizeof(flow->hop_count);
	}

	if ((options & EXPORT_START_TIME) == EXPORT_START_TIME)
	{
		uint32_t start_time = htonl(flow->start_time);
		memcpy(payload + offset, &start_time, sizeof(start_time));
		offset += sizeof(start_time);
	}

	if ((options & EXPORT_DURATION) == EXPORT_DURATION)
	{
		uint16_t duration = htons(flow->duration);
		memcpy(payload + offset, &duration, sizeof(duration));
		offset += sizeof(duration);
	}

	if ((options & EXPORT_TELEGRAM_COUNT) == EXPORT_TELEGRAM_COUNT)
	{
		uint16_t telegram_count = htons(flow->telegram_count);
		memcpy(payload + offset, &telegram_count, sizeof(telegram_count));
		offset += sizeof(telegram_count);
	}

	if ((options & EXPORT_BYTE_COUNT) == EXPORT_BYTE_COUNT)
	{
		uint16_t byte_count = htons(flow->byte_count);
		memcpy(payload + offset, &byte_count, sizeof(byte_count));
		offset += sizeof(byte_count);
	}

	if (offset != size)
	{
		kdrive_logger(KDRIVE_LOGGER_FATAL, "Failed to write export payload");
		exit(EXIT_FAILURE);
	}

	flow_export *f = calloc(1, sizeof(flow_export));
	if (f == NULL)
	{
		kdrive_logger(KDRIVE_LOGGER_FATAL, "Failed to allocate new flow export");
		exit(EXIT_FAILURE);
	}

	f->payload = payload;
	f->size = size;

	return f;
}

void export_free(flow_export *flow)
{
	free(flow->payload);
	free(flow);
}
