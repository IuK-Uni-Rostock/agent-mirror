#include "flow.h"
#include "../lib/kdriveExpress-17.2.0-raspbian/include/kdrive_express_logger.h"
#include <stdlib.h> 

flow_record *flow_create(bool is_group_address, uint16_t src, uint16_t dest, uint16_t apci, uint8_t priority, uint8_t hop_count, uint32_t start_time, uint16_t byte_count)
{
	// zero initialize flow_record
	flow_record *f = calloc(1, sizeof(flow_record));
	if (f == NULL)
	{
		kdrive_logger(KDRIVE_LOGGER_FATAL, "Failed to allocate new flow record");
		exit(EXIT_FAILURE);
	}

	f->is_group_address = is_group_address;
	f->is_expired = false;
	f->src = src;
	f->dest = dest;
	f->apci = apci;
	f->priority = priority;
	f->hop_count = hop_count;
	f->start_time = start_time;
	f->byte_count = byte_count;
	f->telegram_count = 1;
	f->duration = 0;

	return f;
}

bool flow_update(flow_record *flow_to_update, flow_record *flow)
{
	if (flow_is_expired(flow_to_update, flow->start_time))
	{
		flow_to_update->is_expired = true;
		return false;
	}

	flow_to_update->byte_count += flow->byte_count;
	flow_to_update->telegram_count += flow->telegram_count;
	flow_to_update->duration = flow->start_time - flow_to_update->start_time;
	return true;
}

void flow_free(flow_record *flow)
{
	free(flow);
}

void flow_print(flow_record *flow)
{
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "--------------------------------------------");
	kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Source Address: 0x%04x", flow->src);
	kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Destination Address: 0x%04x", flow->dest);
	kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Start Time: %u", flow->start_time);
	kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Duration: %u", flow->duration);
	kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "APCI: 0x%04x", flow->apci);
	kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Telegram Count: %u", flow->telegram_count);
	kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Byte Count: %u", flow->byte_count);
	kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Priority: %u", flow->priority);
	kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Hop Count: %u", flow->hop_count);
	kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Is Group Address: %s", (flow->is_group_address) ? "true" : "false");
	kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Is Expired: %s", (flow->is_expired) ? "true" : "false");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "--------------------------------------------");
}

bool flow_is_equal(flow_record *flow1, flow_record* flow2)
{
	return (flow1->src == flow2->src &&
		flow1->dest == flow2->dest &&
		flow1->hop_count == flow2->hop_count &&
		flow1->apci == flow2->apci &&
		flow1->priority == flow2->priority);
}

bool flow_is_expired(flow_record *flow, uint32_t current_time)
{
	return current_time - (flow->start_time + flow->duration) > FLOW_EXPIRATION;
}
