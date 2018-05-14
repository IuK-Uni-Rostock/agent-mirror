#include "telegram.h"
#include "../lib/kdriveExpress-17.2.0-raspbian/include/kdrive_express_logger.h"
#include <string.h>
#include <arpa/inet.h>

int8_t telegram_get_hop_count(const uint8_t *telegram, uint32_t telegram_len, uint8_t *hop_count)
{
	if (telegram_len < 4)
	{
		kdrive_logger(KDRIVE_LOGGER_ERROR, "Could not get telegram hop count");
		return 1;
	}

	*hop_count = telegram[3] >> 4 & 0x07;
	return 0;
}

int8_t telegram_get_priority(const uint8_t *telegram, uint32_t telegram_len, uint8_t *priority)
{
	if (telegram_len < 3)
	{
		kdrive_logger(KDRIVE_LOGGER_ERROR, "Could not get telegram priority");
		return 1;
	}

	*priority = telegram[2] >> 2 & 0x03;
	return 0;
}

int8_t telegram_get_data(const uint8_t *telegram, uint32_t telegram_len, uint8_t *data, uint32_t *data_len)
{
	if (telegram_len < 9 || telegram[8] > *data_len)
	{
		kdrive_logger(KDRIVE_LOGGER_ERROR, "Could not get telegram data");
		return 1;
	}

	*data_len = telegram[8];
	if (*data_len > 0)
		memcpy(data, telegram, *data_len);
	return 0;
}

uint8_t telegram_data_get_version(const uint8_t *data, uint32_t data_len)
{
	if (data_len < 1)
	{
		kdrive_logger(KDRIVE_LOGGER_ERROR, "Could not get telegram data version");
		return 0;
	}

	return data[0] & 0x0E;
}

uint8_t telegram_data_get_options(const uint8_t *data, uint32_t data_len)
{
	return data[1];
}

uint16_t telegram_data_get_interval(const uint8_t *data, uint32_t data_len)
{
	// data + 1 = position of interval
	return ntohs(data + 1);
}
