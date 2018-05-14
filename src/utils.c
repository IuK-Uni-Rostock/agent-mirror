#include "utils.h"

void print_help(const char *name)
{
    printf("Usage: %s [OPTIONS...]\n\n", name);
    puts("OPTIONS:");
    puts("--log FILE\t\tpath to log file (if omitted, logs to stdout)\n");
    puts("--input FILE/NAME/ID/IP\tFILE is path to replay file");
    puts("\t\t\tNAME is serial device name");
    puts("\t\t\tID is usb interface id");
    puts("\t\t\tIP is ip address of ip interface\n");
    puts("--type TYPE\t\t0 - serial device");
    puts("\t\t\t1 - usb device");
    puts("\t\t\t2 - ip device");
    puts("\t\t\t3 - replay file");
    puts("\n\t\t\tNote that input and input type must be passed together.");
    puts("\t\t\tIf none of them is passed to the program, the default serial device will be used.\n");
    puts("--demo\t\t\tenable demonstration mode and write output to FIFO pipe at /tmp/demo_fifo\n");
    puts("--help\t\t\tprint this help list\n");
    puts("--version\t\tprint program version");
}
