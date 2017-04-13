#include <boost/fusion/adapted.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/variant/apply_visitor.hpp>

#include <iostream>
#include <map>
#include <string>

namespace x3 = boost::spirit::x3;

/**
    AST
*/
namespace test {
namespace ast {

struct expr;

struct expr_value : public x3::variant<std::string, x3::forward_ast<expr>> {
  using base_type::base_type;
  using base_type::operator=;
};

typedef std::map<std::string, expr_value> map_type;
typedef std::pair<std::string, expr_value> entry_type;

struct expr {
public:
  map_type entries;
};

} // namespace ast
} // namespace test

/**
boost fusion adapt
*/
BOOST_FUSION_ADAPT_STRUCT(test::ast::expr, (test::ast::map_type, entries))

/**
*
        AST Visitor
*/
namespace test {
class ASTPrintor {

public:
  ASTPrintor(int tab_num) : tab_num_(tab_num) {}

  void operator()(const std::string &str_val) const{
    std::cout << str_val << std::endl;
  }

  void operator()(const test::ast::expr &expr_val) const {
    print_tab(0);
    std::cout << "{" << std::endl;

    for (const auto& entry : expr_val.entries) {
      print_tab(0);
      std::cout << entry.first << " : ";
      boost::apply_visitor(test::ASTPrintor(tab_num_ + 1), entry.second);
    }

    print_tab(0);
    std::cout << "}" << std::endl;
  }

  void print_tab(int append_num) const {
    for (int i = 0; i < tab_num_ + append_num; i++) {
      std::cout << "\t";
    }
  }

private:
  int tab_num_;
};

} // namespace test

/**
*
        Grammar

{
    "color" = "blue"
    "size" = "29 cm."
    "position" = {
        "x" = "123"
        "y" = "456"
    }
}
*/

namespace test {
namespace parser {

using x3::ascii::char_;
using x3::lexeme;

const x3::rule<class expr_value, test::ast::expr_value> expr_value = "expr_value";
const x3::rule<class entry,test::ast::entry_type> entry = "entry";
const x3::rule<class expr, test::ast::expr> expr = "expr";

const auto quato = lexeme['"' >> +(char_ - '"') >> '"'];
auto expr_value_def = (quato | expr);
auto entry_def = quato >> '=' >> expr_value;
auto expr_def = '{' >> *(entry) >> '}';

BOOST_SPIRIT_DEFINE(expr_value,entry,expr)
} // namespace parser
} // namespace test

int main(int argc, char **argv) {
  test::ast::expr expression;

  std::string test_json = "{ \
    \"color\" = \"blue\" \
    \"size\" = \"29 cm.\" \
    \"position\" = {  \
        \"x\" = \"123\" \
        \"y\" = \"454\" \
    	} \
   }";

  test::ast::expr ast_tree;
  auto str_first = test_json.begin();
  std::cout << "parser result : "
            << x3::phrase_parse(str_first, test_json.end(), test::parser::expr,
                                x3::space, ast_tree) << std::endl;
  test::ASTPrintor printer(0);
  printer(ast_tree);
  return 0;
}

