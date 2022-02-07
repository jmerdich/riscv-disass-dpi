//  SPDX-FileCopyrightText: 2022 Jake Merdich <jake@merdich.com>
//  SPDX-License-Identifier: Unlicense

#include <stdint.h>
#include <string.h>
#include <stdlib.h>


char* rv_disass(unsigned int inst) {
    return strdup("unknown");
}

void rv_free(char* str) {
    free(str);
}
