/*
 *  Phusion Passenger - https://www.phusionpassenger.com/
 *  Copyright (c) 2014 Phusion
 *
 *  "Phusion Passenger" is a trademark of Hongli Lai & Ninh Bui.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <Constants.h>

using namespace std;

int watchdogMain(int argc, char *argv[]);
int serverMain(int argc, char *argv[]);
int loggingAgentMain(int argc, char *argv[]);
int systemMetricsMain(int argc, char *argv[]);
int tempDirToucherMain(int argc, char *argv[]);
int spawnPreparerMain(int argc, char *argv[]);

static bool
isHelp(const char *arg) {
	return strcmp(arg, "help") == 0
		|| strcmp(arg, "--help") == 0
		|| strcmp(arg, "-h") == 0;
}

static void
usage(int argc, char *argv[]) {
	printf("Usage: " AGENT_EXE " <SUBCOMMAND> [options...]\n");
	printf(PROGRAM_NAME " version " PASSENGER_VERSION ".\n");
	printf("Type '%s <SUBCOMMAND> --help' for help on a specific subcommand.\n",
		argv[0]);
	printf("\n");
	printf("Daemon subcommands:\n");
	printf("  server\n");
	printf("  watchdog\n");
	printf("  logger\n");
	printf("\n");
	printf("Utility subcommands:\n");
	printf("  system-metrics\n");
}

static bool
dispatchHelp(int argc, char *argv[]) {
	if (argc == 1) {
		usage(argc, argv);
		exit(1);
	} else if (argc == 2 && isHelp(argv[1])) {
		usage(argc, argv);
		exit(0);
	} else if (isHelp(argv[1])) {
		fprintf(stderr, "Please type '%s %s --help' for help on this specific subcommand.\n",
			argv[0], argv[2]);
		exit(1);
	}
	return false;
}

static void
dispatchSubcommand(int argc, char *argv[]) {
	if (strcmp(argv[1], "watchdog") == 0) {
		exit(watchdogMain(argc, argv));
	} else if (strcmp(argv[1], "server") == 0) {
		exit(serverMain(argc, argv));
	} else if (strcmp(argv[1], "logger") == 0) {
		exit(loggingAgentMain(argc, argv));
	} else if (strcmp(argv[1], "system-metrics") == 0) {
		exit(systemMetricsMain(argc, argv));
	} else if (strcmp(argv[1], "temp-dir-toucher") == 0) {
		exit(tempDirToucherMain(argc, argv));
	} else if (strcmp(argv[1], "spawn-preparer") == 0) {
		exit(spawnPreparerMain(argc, argv));
	} else if (strcmp(argv[1], "test-binary") == 0) {
		printf("PASS\n");
		exit(0);
	} else {
		usage(argc, argv);
		exit(1);
	}
}

int
main(int argc, char *argv[]) {
	if (!dispatchHelp(argc, argv)) {
		dispatchSubcommand(argc, argv);
	}
	return 1; // Never reached
}
