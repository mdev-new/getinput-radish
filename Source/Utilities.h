#pragma once

template<typename T>
inline T _max(T a, T b) {
	return ((a > b) ? a : b);
}

inline constexpr int int_ceil(float number) {
	int inum = (int)(number);
	return ((float)inum != num) ? inum + 1 : inum;
}

char* itoa_(int i);
char* readenv(const char* name);
long getenvnum(const char* name);
long getenvnum_ex(const char* name, int default_val);

void usleep(long long usec);
