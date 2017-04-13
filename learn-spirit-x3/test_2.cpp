#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted.hpp>

//#include <boost/fusion/adapted/std_tuple.hpp>
#include <utility>
#include <iostream>

namespace my_parser{	
	using namespace boost::spirit::x3;
	const rule<class test,std::vector<double>> test = "test";
	auto f = [](auto& ctx){ _val(ctx).push_back(_attr(ctx)); };

	const auto test_def = double_[f] % "," ;
	BOOST_SPIRIT_DEFINE(test);	
}//namespace parser


template<typename Iterator>
bool parser_numbers(Iterator& first,Iterator end,std::vector<double>& v){
	using namespace boost::spirit::x3;

	//phrase_parse(first,end,\
		( \
			double_ % "," \
		),\
		space, \
		v);
	parse(first,end,my_parser::test,v);
	
	if(first != end){
		return false;
	}
	return true;	
}



int main(int argc,char** argv){
	std::string str("3.14,15.5,132.1");
	auto str_begin = str.begin();
	std::vector<double> v;
	//std::cout << parser_numbers(str_begin,str.end(),v) << std::endl;
	std::cout << parser_numbers(str_begin,str.end(),v) << std::endl;
	for(auto& elem : v){
		std::cout << elem << std::endl;
	}
	return 0;
}
