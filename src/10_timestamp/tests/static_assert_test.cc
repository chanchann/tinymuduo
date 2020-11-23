#include <boost/static_assert.hpp>

class Timestamp {
private:
	int64_t microSecondsSinceEpoch_;
};

static_assert(sizeof(Timestamp) == sizeof(int64_t),
              "Timestamp should be same size as int64_t");
// static_assert(sizeof(Timestamp) == sizeof(short),
//               "Timestamp should be same size as int64_t");

int main(void) {
	return 0;
}