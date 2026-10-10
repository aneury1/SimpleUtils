#include <bufferUtils.h>
#include <simpleutils/simpleutils.h>
#include <stringUtils.h>
int odr_probe() {
  simpleutils::Buffer b;
  b.setData("abc", 3);
  return static_cast<int>(b.size());
}
