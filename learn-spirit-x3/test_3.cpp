#include <boost/spirit/home/x3.hpp>
//#include <boost/fusion/adapted.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>

#include <string>
#include <iostream>

namespace ast{


struct employee{
public:
	void print(){
		std::cout << age << "," \
			<< name << "," \
			<< forename << "," \
			<< salary << std::endl;
	}
public:
	int age;
	std::string name; 
	std::string forename;
	double salary;
};
}

BOOST_FUSION_ADAPT_STRUCT(
	ast::employee,
	(int,age)
	(std::string,name)
	(std::string,forename)
	(double,salary)
)

namespace test{
/**
employee { age,"name","forename",salary }
*/

namespace parser{
	namespace x3 = boost::spirit::x3;
	using x3::ascii::char_;
	using x3::int_;
	using x3::double_;
	using x3::lit;
	using x3::lexeme;
	
	const x3::rule<class employee,ast::employee> employee = "employee";
	
	auto f = [](auto& ctx) { std::cout << "quota :" << _attr(ctx) << std::endl;};
	const auto quota = lexeme['"' >> +(char_ - '"') >> '"'];
	
	const auto employee_def = \
		lit("employee") >> \
		'{' >> \
		int_ >> lit(",") >> \ /*note : int_[f] >> lit(",") >> */
		quota >> lit(",") >> \
		quota >> lit(",") >> \
		double_ >> \
		'}';

	BOOST_SPIRIT_DEFINE(employee);
};
}

int main(int argc,char** argv){
	namespace x3 =  boost::spirit::x3;
	ast::employee em;
	std::string str {"employee{11,\"jangwee\",\"jiangwei\",10.111}" };

	auto str_begin = str.begin();	
	bool r = false;
	std::cout <<  (r = x3::phrase_parse(str_begin,str.end(),test::parser::employee,x3::space,em)) << std::endl;

	em.print();
	
	return 0;
}




