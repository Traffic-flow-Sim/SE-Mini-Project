#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

#include <stddef.h>

#include "simulation.h"

#define CONFIG_CSV_MAX_LINE 512

/* Loads and validates a fixed-schema CSV into config atomically. */
int config_csv_load(const char *path, SimConfig *config,
                    char *error, size_t error_size);

/* Saves one validated configuration row, restricting POSIX permissions. */
int config_csv_save(const char *path, const SimConfig *config,
                    char *error, size_t error_size);

#endif
