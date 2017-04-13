#include<boost/variant.hpp>
#include<string>
#include<map>

namespace test{

struct expr;
struct expr_value : boost::variant<std::string,\
		expr*>{

};

struct expr{
	std::map<std::string,expr_value> expr_map;
};


}



int main(){
	test::expr ex;
	return 0;	
}
