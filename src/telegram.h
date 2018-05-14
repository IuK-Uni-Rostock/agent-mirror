#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <stdint.h>

//----------------------------------
// universal telegram functions
//----------------------------------
int8_t telegram_get_hop_count(const uint8_t *telegram, uint32_t telegram_len, uint8_t *hop_count);
int8_t telegram_get_priority(const uint8_t *telegram, uint32_t telegram_len, uint8_t *priority);
int8_t telegram_get_data(const uint8_t *telegram, uint32_t telegram_len, uint8_t *data, uint32_t *data_len);

//----------------------------------
// configuration telegram functions
//----------------------------------
uint8_t telegram_data_get_version(const uint8_t *data, uint32_t data_len);
uint8_t telegram_data_get_options(const uint8_t *data, uint32_t data_len);
uint16_t telegram_data_get_interval(const uint8_t *data, uint32_t data_len);

#endif
