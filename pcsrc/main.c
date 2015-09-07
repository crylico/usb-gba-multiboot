/*
    This file is part of the USB-GBA multiboot project.

    The USB-GBA multiboot project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The USB-GBA multiboot project is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the USB-GBA multiboot project.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "xfer.h"
#include "multi.h"

void printHelp() {
    fprintf(stderr, "Usage: gbaxfer /path/to/serialport /path/to/file\n");
}


void processOptions(int argc, char* argv[], char ** romfile, char **serialPort) {

    *serialPort = argv[1];
    *romfile = argv[2];
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printHelp();
        return 0;
    }

    char *filename;
    char *serialPort;
    processOptions(argc, argv, &filename, &serialPort);

    struct timeval timebegin, timeend;
    gettimeofday(&timebegin, NULL);

    gbaHandle *gba = initGbaHandle(serialPort, MODE_NORMAL);

    if (filename) {
        FILE *rom = fopen(filename,"rb");
        if (!gba || !rom) {
            fclose(rom);
            return -1;
        }
        unsigned size;
        struct stat stats;
        fstat(fileno(rom), &stats);
        size = stats.st_size;
        if (size < 0x60 * 2) return -1;
        size = (size+0xf)&(~0xf); //Align to 16bytes

        unsigned char* romdata = malloc(size);
        fread(romdata, 1, size, rom);

        gbaMultibootSend(romdata, size, gba);

        free(romdata);
        fclose(rom);
    }

    freeGbaHandle(gba);

    gettimeofday(&timeend, NULL);
    double executiontime = timeend.tv_sec + (timeend.tv_usec / 1000000.0);
    executiontime -= timebegin.tv_sec + (timebegin.tv_usec / 1000000.0);
    fprintf(stderr,"\nTotal transfer time: %.2f seconds\n", executiontime);
    return 0;

}