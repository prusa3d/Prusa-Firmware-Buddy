#pragma once

struct stat;

int stat_retry(const char *path, struct stat *st);
