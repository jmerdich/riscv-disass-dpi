//  SPDX-FileCopyrightText: 2022 Jake Merdich <jake@merdich.com>
//  SPDX-License-Identifier: Unlicense

#ifndef RV_DISASS_DPI
#define RV_DISASS_DPI

#ifndef DPI_DLLISPEC
#ifdef _WIN32
#define DPI_DLLISPEC __declspec(dllimport)
#else
#define DPI_DLLISPEC
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

DPI_DLLISPEC char* rv_disass(unsigned int inst);
DPI_DLLISPEC void rv_free(char* str);
DPI_DLLISPEC void rv_set_option(const char* str, bool enabled);
DPI_DLLISPEC void rv_reset_options();

#ifdef __cplusplus
} // extern "C"
#endif

#endif