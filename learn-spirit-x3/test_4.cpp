#include <boost/variant.hpp>
#include <map>
#include <string>

namespace test {

struct expr;
struct expr_value : boost::variant<std::string, expr *> {};

struct expr {
  std::map<std::string, expr_value> expr_map;
};

} // namespace test

int main() {
  test::expr ex;
  return 0;
}
