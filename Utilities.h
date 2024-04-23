#pragma once

char* itoa_(int i);
char* readenv(const char* name);
long getenvnum(const char* name);
long getenvnum_ex(const char* name, int default_val);

void usleep(long long usec);
