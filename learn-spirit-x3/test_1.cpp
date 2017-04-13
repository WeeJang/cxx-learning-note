#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>

#include <iostream>
#include <string>
#include <utility>

int main(int argc,char** argv){
	using namespace boost::spirit;
	std::tuple<double,double,int> p;
	std::string str("(10.333 10.21211 -10)");
	auto str_begin = str.begin();
	auto f = [](auto& ctx) {
		std::cout << _attr(ctx) << std::endl;
		_val(ctx) = 0;
		std::cout << _val(ctx) << std::endl;
	};
	x3::phrase_parse(str_begin,str.end(),'(' >> x3::double_[f]>> x3::double_[f] >> x3::int_[f]>> ')',x3::space,p);
	std::cout << std::get<0>(p) << ":" << std::get<1>(p) << ":" << std::get<2>(p) << std::endl;
	return 0;
}
