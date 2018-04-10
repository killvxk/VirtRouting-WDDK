
/*
 * DESCRIPTION:
 * This program does nothing except divert packets and re-inject them.  This is
 * useful for performance testing.
 *
 * usage: netdump.exe windivert-filter num-threads
 */

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "windivert.h"

#define MAXBUF  0xFFFF

static DWORD passthru(LPVOID arg);

/*
 * Entry.
 */
int __cdecl main(int argc, char **argv)
{
    int num_threads, priority = 0, i;
    HANDLE handle, thread;

    if (argc != 3 && argc != 4)
    {
        fprintf(stderr, "usage: %s filter num-threads [priority]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    num_threads = atoi(argv[2]);
    if (num_threads < 1 || num_threads > 64)
    {
        fprintf(stderr, "error: invalid number of threads\n");
        exit(EXIT_FAILURE);
    }
    if (argc == 4)
    {
        priority = atoi(argv[3]);
    }

    // Divert traffic matching the filter:
    handle = WinDivertOpen(argv[1], WINDIVERT_LAYER_NETWORK, (INT16)priority,
        0);
    if (handle == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_INVALID_PARAMETER)
        {
            fprintf(stderr, "error: filter syntax error\n");
            exit(EXIT_FAILURE);
        }
        fprintf(stderr, "error: failed to open the WinDivert device (%d)\n",
            GetLastError());
        exit(EXIT_FAILURE);
    }

    // Start the threads
    for (i = 1; i < num_threads; i++)
    {
        thread = CreateThread(NULL, 1, (LPTHREAD_START_ROUTINE)passthru,
            (LPVOID)handle, 0, NULL);
        if (thread == NULL)
        {
            fprintf(stderr, "error: failed to start passthru thread (%u)\n",
                GetLastError());
            exit(EXIT_FAILURE);
        }
    }

    // Main thread:
    passthru((LPVOID)handle);

    return 0;
}

// Passthru thread.
static DWORD passthru(LPVOID arg)
{
    unsigned char packet[MAXBUF];
    UINT packet_len;
    WINDIVERT_ADDRESS addr;
    HANDLE handle = (HANDLE)arg;

    // Main loop:
    while (TRUE)
    {
        // Read a matching packet.
        if (!WinDivertRecv(handle, packet, sizeof(packet), &addr, &packet_len))
        {
            fprintf(stderr, "warning: failed to read packet (%d)\n",
                GetLastError());
            continue;
        }

        // Re-inject the matching packet.
        if (!WinDivertSend(handle, packet, packet_len, &addr, NULL))
        {
            fprintf(stderr, "warning: failed to reinject packet (%d)\n",
                GetLastError());
        }
    }
}

