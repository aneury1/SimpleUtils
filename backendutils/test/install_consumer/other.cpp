#include <simpleutils/simpleutils.h>
int other() { return static_cast<int>(simpleutils::split("a,b", ",").size()); }
