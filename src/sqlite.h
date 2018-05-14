#ifndef SQLITE_H
#define SQLITE_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sqlite3.h>

bool db_open(const char *db_filename);
void db_insert(uint32_t timestamp_export, bool is_expired, bool is_group_address, uint16_t src, uint16_t dest, uint16_t apci, uint8_t priority, uint8_t hop_count, uint32_t start_time, uint16_t duration, uint16_t telegram_count, uint16_t byte_count);
void db_close();

#endif
