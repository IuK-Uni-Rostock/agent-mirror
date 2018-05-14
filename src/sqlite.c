#include "sqlite.h"
#include "../lib/kdriveExpress-17.2.0-raspbian/include/kdrive_express_logger.h"
#include <unistd.h>
#include <string.h>

static sqlite3 *db = NULL;

static void db_error(uint8_t level)
{
	kdrive_logger_ex(level, "Database error %d: %s", sqlite3_errcode(db), sqlite3_errmsg(db));
}

static inline void log_error() { db_error(KDRIVE_LOGGER_ERROR); }
static inline void log_fatal() { db_error(KDRIVE_LOGGER_FATAL); }

bool db_open(const char *db_filename)
{
	// try to open database
	if (sqlite3_open(db_filename, &db) != SQLITE_OK)
	{
		log_fatal();
		sqlite3_close(db);
		return false;
	}

	// initialize database with default table if not exists already
	if (sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS flow_export(id INTEGER PRIMARY KEY, timestamp_export INTEGER, is_expired INTEGER, is_group_address INTEGER, src INTEGER, dest INTEGER, apci INTEGER, priority INTEGER, hop_count INTEGER, start_time INTEGER, duration INTEGER, telegram_count INTEGER, byte_count INTEGER);", 0, 0, NULL) != SQLITE_OK)
	{
		log_fatal();
		sqlite3_close(db);
		// delete not initialized database
		unlink(db_filename);
		return false;
	}

	return true;
}

void db_insert(uint32_t timestamp_export, bool is_expired, bool is_group_address, uint16_t src, uint16_t dest, uint16_t apci, uint8_t priority, uint8_t hop_count, uint32_t start_time, uint16_t duration, uint16_t telegram_count, uint16_t byte_count)
{
	sqlite3_stmt *stmt = NULL;

	if (sqlite3_prepare_v2(db, "INSERT INTO flow_export(timestamp_export, is_expired, is_group_address, src, dest, apci, priority, hop_count, start_time, duration, telegram_count, byte_count) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12);", -1, &stmt, NULL) != SQLITE_OK)
	{
		log_error();
		goto cleanup;
	}

	if (sqlite3_bind_int(stmt, 1, timestamp_export) != SQLITE_OK)
	{
		log_error();
		goto cleanup;
	}

	if (sqlite3_bind_int(stmt, 2, is_expired) != SQLITE_OK)
	{
		log_error();
		goto cleanup;
	}

	if (sqlite3_bind_int(stmt, 3, is_group_address) != SQLITE_OK)
	{
		log_error();
		goto cleanup;
	}

	if (sqlite3_bind_int(stmt, 4, src) != SQLITE_OK)
	{
		log_error();
		goto cleanup;
	}
	
	if (sqlite3_bind_int(stmt, 5, dest) != SQLITE_OK)
	{
		log_error();
		goto cleanup;
	}

	if (sqlite3_bind_int(stmt, 6, apci) != SQLITE_OK)
	{
		log_error();
		goto cleanup;
	}

	if (sqlite3_bind_int(stmt, 7, priority) != SQLITE_OK)
	{
		log_error();
		goto cleanup;
	}

	if (sqlite3_bind_int(stmt, 8, hop_count) != SQLITE_OK)
	{
		log_error();
		goto cleanup;
	}

	if (sqlite3_bind_int(stmt, 9, start_time) != SQLITE_OK)
	{
		log_error();
		goto cleanup;
	}

	if (sqlite3_bind_int(stmt, 10, duration) != SQLITE_OK)
	{
		log_error();
		goto cleanup;
	}

	if (sqlite3_bind_int(stmt, 11, telegram_count) != SQLITE_OK)
	{
		log_error();
		goto cleanup;
	}

	if (sqlite3_bind_int(stmt, 12, byte_count) != SQLITE_OK)
	{
		log_error();
		goto cleanup;
	}

	if (sqlite3_step(stmt) != SQLITE_DONE)
	{
		log_error();
	}

cleanup:
	sqlite3_finalize(stmt);
}

void db_close()
{
	if (sqlite3_close(db) != SQLITE_OK)
	{
		log_fatal();
	}
}
