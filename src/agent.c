#define _GNU_SOURCE
#include "../lib/kdriveExpress-17.2.0-raspbian/include/kdrive_express.h"
#include "telegram.h"
#include "export.h"
#include "list.h"
#include "utils.h"
#include "sqlite.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <getopt.h>


// default serial device name
#define DEFAULT_SERIAL_DEVICE ("/dev/serial0")

// group address of collector (for exporting flow records)
#define COLLECTOR_GROUP_ADDRESS (0x5101) // 10/1/1

// group address of agent (for receiving configuration telegrams)
#define AGENT_GROUP_ADDRESS (0x5102) // 10/1/2

// physical address of agent
#define AGENT_PHYSICAL_ADDRESS (0x111F)

// maximum length of extended telegram payload
#define EXTENDED_TELEGRAM_MAX_PAYLOAD_LEN (253)

// kdriveExpress error message length
#define ERROR_MESSAGE_LEN (128)

// uncomment for multi agent environment (one agent per line)
// #define MULTI_AGENT


typedef enum {T_SERIAL, T_USB, T_IP, T_FILE} input_t;


// called when a telegram arrives or gets sent by the device
static void telegram_callback(const uint8_t *telegram, uint32_t telegram_len, void *user_data);

// called when an error occurs
static void error_callback(error_t e, void *user_data);

// called when an event occurs
static void event_callback(int32_t ap, uint32_t e, void *user_data);


static void classify_telegram(const uint8_t *telegram, uint32_t telegram_len, void *user_data, uint32_t timestamp, bool replay);
static void handle_configuration_telegram(const uint8_t *data, uint32_t data_len);
static void export_process(bool replay, uint32_t timestamp);
static void export_single_flow(flow_record *flow, bool replay, uint32_t timestamp);
static void begin_monitoring();
static void begin_replaying_file(char *replay_file);


// starting point of flow_record list
flow_record_node flow_record_list;

// the access port descriptor
int32_t ap = 0;

// timestamp of last flow export
uint32_t last_export = 0;

// export interval in seconds
uint16_t export_interval = 600;

// options for flow export
uint8_t export_options = 0xFF;


// cmdline arguments
bool demo = false;
char *demo_fifo = "/tmp/demo_fifo";
input_t input_type = 0;
char *input;
char *log_file;

void cmdlineargs(int argc, char *argv[])
{
	int c;
	bool input_set = false, input_type_set = false;

	while (1)
	{
		static struct option long_options[] =
		{
			{"log", required_argument, 0, 'l'},
			{"input", required_argument, 0, 'i'},
			{"type", required_argument, 0, 't'},
			{"demo", no_argument, 0, 'd'},
			{"version", no_argument, 0, 'v'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};

		int option_index = 0;

		c = getopt_long(argc, argv, "i:", long_options, &option_index);

		if (c == -1)
		{
			if (input_set != input_type_set)
			{
				fprintf(stderr, "Both input and input type must be passed together\n");
				exit(EXIT_FAILURE);
			}

			break;
		}

		switch (c)
		{
			case 'h':
				print_help(argv[0]);
				exit(EXIT_SUCCESS);
				break;
			case 'i':
				input = optarg;
				input_set = true;
				break;
			case 'l':
				log_file = optarg;
				break;
			case 't':
				c = atoi(optarg);
				switch (c)
				{
					case 0:
					case 1:
					case 2:
					case 3:
						input_type = c;
						input_type_set = true;
						break;
					default:
						fprintf(stderr, "Wrong input type '%d', allowed values are:\n0\tserial device\n1\tusb device\n2\tip device\n3\treplay file\n", c);
						exit(EXIT_FAILURE);
				}
				break;
			case 'd':
				demo = true;
				break;
			case 'v':
				printf("KNX Agent\nExport Protocol Version %u\n", EXPORT_VERSION);
				exit(EXIT_SUCCESS);
			default:
				print_help(argv[0]);
				exit(EXIT_FAILURE);
		}
	}
}

void init()
{
	// create fifo in demonstration mode
	if (demo)
	{
		if (!mkfifo(demo_fifo, 0666))
		{
			kdrive_logger(KDRIVE_LOGGER_FATAL, "Unable to create fifo in demonstration mode");
			exit(EXIT_FAILURE);
		}
	}

	// configure logging level and output
	kdrive_logger_set_level(KDRIVE_LOGGER_INFORMATION);
	if (log_file != NULL || demo)
	{
		if (log_file == NULL)
			log_file = demo_fifo;

		kdrive_logger_file_ex(log_file);
	}
	else
		kdrive_logger_console();

	// register error callback
	kdrive_register_error_callback(&error_callback, NULL);

	// create access port descriptor
	ap = kdrive_ap_create();

	if (ap == KDRIVE_INVALID_DESCRIPTOR)
	{
		kdrive_logger(KDRIVE_LOGGER_FATAL, "Unable to create access port");
		exit(EXIT_FAILURE);
	}

	// initialize last export timestamp
	last_export = time(NULL);

	// register event callback for access port events
	kdrive_set_event_callback(ap, &event_callback, NULL);

	if (input == NULL)
		input = DEFAULT_SERIAL_DEVICE;
}

int main(int argc, char *argv[])
{
	cmdlineargs(argc, argv);
	init();

	kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Starting KNX Agent - Export Protocol Version %u", EXPORT_VERSION);
	if (demo)
		kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Running in demonstration mode");

	switch (input_type)
	{
		case T_SERIAL:
			if (kdrive_ap_open_serial_ft12(ap, input) != KDRIVE_ERROR_NONE)
			{
				kdrive_logger_ex(KDRIVE_LOGGER_FATAL, "Unable to open serial access port '%s'", input);
				exit(EXIT_FAILURE);
			}
			begin_monitoring();
			break;
		case T_USB:
			if (kdrive_ap_open_usb(ap, atoi(input)) == KDRIVE_ERROR_NONE)
			{
				kdrive_logger_ex(KDRIVE_LOGGER_FATAL, "Unable to open usb access port '%d'", atoi(input));
				exit(EXIT_FAILURE);
			}
			begin_monitoring();
			break;
		case T_IP:
			if (kdrive_ap_open_ip(ap, input) == KDRIVE_ERROR_NONE)
			{
				kdrive_logger_ex(KDRIVE_LOGGER_FATAL, "Unable to open ip access port '%d'", input);
				exit(EXIT_FAILURE);
			}
			begin_monitoring();
			break;
		case T_FILE:
			begin_replaying_file(input);
			break;
	}

	// release the access port
	kdrive_ap_release(ap);

	// remove fifo in demonstration mode
	if (demo)
		unlink(demo_fifo);

	// dispose flow record list
	list_dispose(&flow_record_list);

	return EXIT_SUCCESS;
}

void begin_monitoring()
{
	// this is a fix for bus monitor mode to receive all telegrams
	kdrive_ap_set_filter_dest_addr(ap, false);

	// set physical address of local device
	kdrive_ap_set_ind_addr(ap, AGENT_PHYSICAL_ADDRESS);

	// connect the packet trace logging mechanism to see the Rx and Tx packets
	kdrive_ap_packet_trace_disconnect(ap);

	// test management telegram
	/*uint8_t telegram[] = {0x11, 0x00, 0xBC, 0x50, 0x00, 0x00, 0x09, 0x01, 0x01, 0x00, 0x81};
	if (kdrive_ap_send(ap, telegram, sizeof(telegram) / sizeof(telegram[0])) == KDRIVE_ERROR_NONE)
		kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Success!");*/

	// go into bus monitor mode
	uint32_t callback_key = 0;
	kdrive_ap_register_telegram_callback(ap, &telegram_callback, NULL, &callback_key);

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Entering BusMonitor Mode");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Press [Enter] to exit the application ...");
	getchar();

	// close access port
	kdrive_ap_close(ap);
}

void begin_replaying_file(char *replay_file)
{
	FILE *fp;
	char buffer[512];

	fp = fopen(replay_file, "r");

	if (fp == NULL)
	{
		kdrive_logger_ex(KDRIVE_LOGGER_FATAL, "Unable to open replay file '%s'", replay_file);
		exit(EXIT_FAILURE);
	}

	// open sqlite3 database to save exported flow records
	if (!db_open("export.sqlite3"))
	{
		exit(EXIT_FAILURE);
	}

	bool last_export_set = false;

	while (fgets(buffer, 512, fp))
	{
		char *str, *tofree, *token;
		str = tofree = strdup(buffer);

		uint8_t ccount = 0;
		char *column[7];
		while ((token = strsep((char**)&str, "\t")))
		{
			column[ccount] = token;
			ccount++;
		}

		// to convert the time and date to unix timestamp combine column 1 and 2 by replacing the null terminator with a space
		*((uint8_t*)(column[2] - 1)) = ' ';

		struct tm tm;
		memset(&tm, 0, sizeof(struct tm));
		strptime(column[1], "%H:%M:%S %Y-%m-%d", &tm);
		uint32_t timestamp = mktime(&tm);

		uint32_t telegram_len = strlen(column[6]) / 2;
		uint8_t *telegram = calloc(telegram_len, 1);

		for (uint32_t count = 0; count < telegram_len; count++)
		{
			sscanf(column[6], "%2hhx", telegram + count);
			column[6] += 2;
		}

		// set last export time to first timestamp
		if (!last_export_set)
		{
			last_export = timestamp;
			last_export_set = true;
		}

		classify_telegram(telegram, telegram_len, NULL, timestamp, true);
		//sleep(1);

		free(telegram);
		free(tofree);
	}

	db_close();
	fclose(fp);
}

void telegram_callback(const uint8_t *telegram, uint32_t telegram_len, void *user_data)
{
	classify_telegram(telegram, telegram_len, NULL, time(NULL), false);
}

void classify_telegram(const uint8_t *telegram, uint32_t telegram_len, void *user_data, uint32_t timestamp, bool replay)
{
	static uint8_t data[EXTENDED_TELEGRAM_MAX_PAYLOAD_LEN];
	uint32_t data_len = EXTENDED_TELEGRAM_MAX_PAYLOAD_LEN;
	uint16_t src = 0;
	uint16_t dest = 0;
	uint16_t apci = 0;
	uint8_t priority = 0;
	uint8_t hop_count = 0;
	uint8_t message_code = 0;

	kdrive_ap_get_message_code(telegram, telegram_len, &message_code);

	if (message_code == KDRIVE_CEMI_L_DATA_IND)
	{
		kdrive_ap_get_dest(telegram, telegram_len, &dest);
		kdrive_ap_get_src(telegram, telegram_len, &src);
		kdrive_ap_get_apci(telegram, telegram_len, &apci);
		telegram_get_priority(telegram, telegram_len, &priority);
		telegram_get_hop_count(telegram, telegram_len, &hop_count);
		telegram_get_data(telegram, telegram_len, data, &data_len);

		#ifdef MULTI_AGENT
		if (hop_count < 6) return;
		#endif

		// handle group write / read / response
		if (kdrive_ap_is_group(telegram, telegram_len))
		{
			flow_record *flow = flow_create(true, src, dest, apci, priority, hop_count, timestamp, data_len);

			flow_print(flow);
			list_insert(&flow_record_list, flow);

			// check if collector is sending configuration telegrams to my agent group address
			if (dest == AGENT_GROUP_ADDRESS)
			{
				handle_configuration_telegram(data, data_len);
			}
		}
		else // handle management telegram
		{
			flow_record *flow = flow_create(false, src, dest, apci, priority, hop_count, timestamp, data_len);

			flow_print(flow);

			export_single_flow(flow, replay, timestamp);

			flow_free(flow);
		}
	}
	else if (message_code == KDRIVE_CEMI_L_DATA_CON)
	{
		// a copy of our sent telegram (con = confirmation)
	}
	else if (message_code == KDRIVE_CEMI_L_DATA_REQ)
	{
		// telegram request (polling??)
	}

	// check export interval
	if (timestamp - last_export > export_interval)
	{
		kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Starting flow export");

		export_process(replay, timestamp);
		//list_dispose_expired(&flow_record_list);
		list_dispose(&flow_record_list);
		last_export = (replay) ? timestamp : time(NULL);

		kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Finished flow export");
	}
}

void handle_configuration_telegram(const uint8_t *data, uint32_t data_len)
{
	if (telegram_data_get_version(data, data_len) != EXPORT_VERSION)
	{
		kdrive_logger(KDRIVE_LOGGER_WARNING, "Collector is using an incompatible version");
	}
	else
	{
		// type of configuration telegram is currently determined by data length
		if (data_len == 2)
		{
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Received export options telegram from collector");
			export_options = telegram_data_get_options(data, data_len);
		}
		else if (data_len == 3)
		{
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Received export interval telegram from collector");
			export_interval = telegram_data_get_interval(data, data_len);
		}
		else
		{
			kdrive_logger(KDRIVE_LOGGER_WARNING, "Received unknown configuration telegram from collector");
			kdrive_logger_dump(KDRIVE_LOGGER_DEBUG, "Telegram Data:", data, data_len);
		}
	}
}

void export_process(bool replay, uint32_t timestamp)
{
	uint16_t flows_count = list_count(&flow_record_list);
	uint16_t exported_flows = 0;
	uint8_t payload_size = export_calculate_payload_size(export_options);
	uint8_t flows_per_telegram = (EXTENDED_TELEGRAM_MAX_PAYLOAD_LEN - EXPORT_HEADER_SIZE) / payload_size;

	flow_record_node *cursor = &flow_record_list;
	while (flows_count > exported_flows)
	{
		uint8_t offset = 0, count = 1;

		uint8_t flows_to_export = MIN(flows_count - exported_flows, flows_per_telegram);
		flow_print(cursor->flow);
		flow_export *e = export_create_long_payload(cursor->flow, export_options, flows_to_export, &offset, (replay) ? timestamp : time(NULL));
		if (replay)
		{
			db_insert(timestamp, cursor->flow->is_expired, cursor->flow->is_group_address, cursor->flow->src, cursor->flow->dest, cursor->flow->apci,
				cursor->flow->priority, cursor->flow->hop_count, cursor->flow->start_time, cursor->flow->duration, cursor->flow->telegram_count,
				cursor->flow->byte_count);
		}

		cursor = cursor->next;
		while (cursor != NULL && count < flows_to_export)
		{
			flow_print(cursor->flow);
			export_append_long_payload(cursor->flow, e, export_options, &offset, (replay) ? timestamp : time(NULL));
			if (replay)
			{
				db_insert(timestamp, cursor->flow->is_expired, cursor->flow->is_group_address, cursor->flow->src, cursor->flow->dest, cursor->flow->apci,
					cursor->flow->priority, cursor->flow->hop_count, cursor->flow->start_time, cursor->flow->duration, cursor->flow->telegram_count,
					cursor->flow->byte_count);
			}
			cursor = cursor->next;
			count++;
		}

		kdrive_logger_dump(KDRIVE_LOGGER_DEBUG, "Flow record export:", e->payload, e->size);

		// do not export flows to KNX bus when replaying
		if (!replay)
		{
			if (kdrive_ap_group_write(ap, COLLECTOR_GROUP_ADDRESS, e->payload, KDRIVE_BITS(e->size)) == KDRIVE_ERROR_NONE)
			{
				kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Successfully exported flow record(s) to collector");
			}
			else
			{
				kdrive_logger(KDRIVE_LOGGER_ERROR, "Could not export flow record(s) to collector");
			}
		}

		exported_flows += count;
		export_free(e);
	}
}

void export_single_flow(flow_record *flow, bool replay, uint32_t timestamp)
{
	flow_export *e = export_create_payload(flow, export_options, (replay) ? timestamp : time(NULL));

	// do not export flow to KNX bus when replaying
	if (!replay)
	{
		if (kdrive_ap_group_write(ap, COLLECTOR_GROUP_ADDRESS, e->payload, KDRIVE_BITS(e->size)) == KDRIVE_ERROR_NONE)
		{
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Successfully exported flow record to collector");
		}
		else
		{
			kdrive_logger(KDRIVE_LOGGER_ERROR, "Could not export flow record to collector");
		}
	}

	export_free(e);
}

void error_callback(error_t e, void *user_data)
{
	if (e != KDRIVE_TIMEOUT_ERROR)
	{
		static char error_message[ERROR_MESSAGE_LEN];
		kdrive_get_error_message(e, error_message, ERROR_MESSAGE_LEN);
		kdrive_logger_ex(KDRIVE_LOGGER_ERROR, "kdrive error: %s", error_message);
	}
}

void event_callback(int32_t ap, uint32_t e, void *user_data)
{
	switch (e)
	{
		case KDRIVE_EVENT_ERROR:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Access Port Error");
			break;

		case KDRIVE_EVENT_OPENING:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Access Port Opening");
			break;

		case KDRIVE_EVENT_OPENED:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Access Port Opened");
			break;

		case KDRIVE_EVENT_CLOSED:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Access Port Closed");
			break;

		case KDRIVE_EVENT_CLOSING:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Access Port Closing");
			break;

		case KDRIVE_EVENT_TERMINATED:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Access Port Terminated");
			break;

		case KDRIVE_EVENT_KNX_BUS_CONNECTED:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "KNX Bus Connected");
			break;

		case KDRIVE_EVENT_KNX_BUS_DISCONNECTED:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "KNX Bus Disconnected");
			break;

		case KDRIVE_EVENT_LOCAL_DEVICE_RESET:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Local Device Reset");
			break;

		case KDRIVE_EVENT_TELEGRAM_INDICATION:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Telegram Indication");
			break;

		case KDRIVE_EVENT_TELEGRAM_CONFIRM:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Telegram Confirm");
			break;

		case KDRIVE_EVENT_TELEGRAM_CONFIRM_TIMEOUT:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Telegram Confirm Timeout");
			break;

		case KDRIVE_EVENT_INTERNAL_01:
			break;

		default:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Unknown kdrive event");
			break;
	}
}
