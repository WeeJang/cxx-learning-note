# Boost Spirit X3 学习记录

[TOC]

## 摘要

最近着实发现需要一个靠谱的PEG（Parser Expression Grammer）工具。简单调研了一下。有比较远古的Bison/Flex工具链（苏毅之前吐槽过，Pure C的， Global Variable满天飞）。找来找去，发现了Boost Spirit X3，着实惊艳了一脸。简单来讲，Boost Spirit X3使用了Template Metagramming，将Grammar在Compile time时构建，大量的使用了TM中的黑科技：

包括不限于：

1. CRTP([Curiously Recurring Template Pattern](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern));
2. ET([Expression Template](https://en.wikipedia.org/wiki/Expression_templates));
3. ADL([Argument-dependent name lookup]([https://en.wikipedia.org/wiki/Argument-dependent_name_lookup));

因此，理解Spirit 需要对以上TMP有所了解。虽然可能没发一时半会把源码琢磨的很透，但起码会有比较好的感觉。

学习Spirit X3，我主要参考了参考文献中的<<Inside Spirit X3>>,<<Using Spirit X3 to write Parser>>，还有官方文档。尤其是<<Inside Spirit X3>>,先是设计了一个Toy version的Spirit，这个Spirit真的是麻雀虽小，但五脏俱全。基本把Spirit X3的Kernel Concept : Parser,Context,Rule/RuleDefinition描述的非常清晰，不可不看。

这里的回顾，仅是对学习的一个记录，后期肯能会再修改。

## 主要要点：

### 1.Parser

​      Parser 是构建Grammar的基石。对Parser的各种组合构成了完整的Grammar。

​      Paser的实现利用了CRTP，以达到static polymorphism的效果。

```
template<typename Derived>
class parser<Derived>{
  ... ...
  const Derived& derived() const{
    return *static_cast<const Derived*>(this);
  }
  ... ...
  bool parse(Iterator& first,Iterator last,const Context& ctx) const;
};
```

### 2.Context

​     Context 在不同上下文（调用栈）中起到共享数据的左右，抽象上看是一个链表结构，而且是ploymorphism的。同时秉承原则：Caller setup context, Callee retrieve the context as needed。

```
template<typename ID,typename T,typename NextConext>
struct context{
  context(const T& val,const NextContext& next_ctx):val_(val),next_ctx_(next_ctx){}
  const T& get(mpl::identity<ID>) const { return val_;}
  decltype(std::declval<NextContext>().get(Identity()))
  get(Identity id) const { return next_ct_.get(id); }
 ... ...
 private:
  const T& val_;
  const NextContext& next_ctx_;
};
```

### 3.Rule    

rule 与 rule_definition 的构建关系。这个在参考<<Inside Spirit X3>>的toy spirit 实现中有。

rule 与 rule_definition 设计，巧妙的解决了Grammar中的递归依赖。参考slide 中的代码：

```c++
template<typename ID,typename RHS>
class rule_definition : public parser<rule_definition<ID,RHS>>{
  rule_definition(RHS rhs) : rhs_(RHS) {}
  
  template<typename Iterator,typename Context>
  bool parse(Iterator& first,Iterator last,const Context& ctx){
    context<ID,RHS,Context> this_ctx(rhs,ctx);
    return rhs.parse(first,last,this_ctx);//可以看到，这里的RHS是一个parser
  }
  RHS rhs;
};

template<typename ID>
struct rule : public parser<rule<ID>>{
public:
  rule(const char* rule_name) : rule_name_(rule_name){....}
  ... ...
  template<typename ID,typename Derived>
  rule_definition<ID,Derived> operator=(const parser<Derived>& definition) const{
    return rule_definition<ID,Derived>(definition.derived());
  }
  
  bool parse(Iterator& first,Iterator last,const Context& ctx) const{
    ctx.get(mpl::identity<ID>()).parser(first,last,ctx);
  }
  
private:
  const char* rule_name_;
};//class rule


```

面对递归结构是：

```
const rule<class parser_1> parser_1 = "parser_1"; //declare
const auto& parser_2 = int_ >> parser_1;
const auto& start = parser_1 = parser_2 | char_; //operator= ,bind rule with rule_definition
```

### 4.构建AST的技巧

  AST是Grammar parse  之后的结果存储，因此要能够处理递归结构。因此很容易想到利用Variant（注：C++17之后才引入标准库，使用Boost中的实现）。以文档中提到的[例子](http://ciere.com/example/x3/rexpr/rexpr_min/rexpr.cpp)

要处理的min-json格式：

```
{
	"color" = "blue"
	"size" = "29 cm."
	"position" = {
		"x" = "123"
		"y" = "456"
	}
}
```

很容想到，AST可以用map结构存储， key为string，value 为一个string或者是一个子map结构。因此有

```c++
struct expr{
  std::map<string,std::variant<std::string,expr>> expr_map_;
};
```

或者是按照文档中，拆分一下:

```c++
struct expr;
struct expr_value : boost::variant<std::string,expr> {...};
struct expr { 
  std::map<std::string,expr_value> expr_map_;
};
```

这样肯定不能编译通过。因为在 编译expr_value的时候，expr 是一个incomplete-type(仅有声明，定义在后面)。编译器无法知道这个expr的对象模型，也就不知道内存空间。所以，简单的做法，就是将expr_value 定义为：

```c++
struct expr_value : boost::variant<std::string,expr*>{...};//或者引用expr&
```

使用指针（或者引用）就可以了。

Spirit X3为了统一 variant中可能元素的类型（即都使用非指针／非引用类型），以便于提供统一的访问方式，提供了一个forward_ast的wrapper:

```c++
template<typename T>
class forward_ast{
public:
  typedef T type;
  ... ...
  forward_ast(const T& operand):p_(new T(operand)){}
 private:
  T* p_;
};
```

因此struct expr_value 可改写为：

```c++
struct expr_value : boost::variant<std::string,\
  					x3::forward_ast<expr>>{...};
```

 



### 5.Other

1. rule 与 parser 的逻辑关系。多个primitive parser / rule  -> rule。rule 也是parser 的derived class. 
2. build-in function的中_val 和 _attr的理解：

> `_val`: A reference to the attribute of the innermost rule that directly or indirectly invokes the parser `p`
>
> `_attr` : A reference to the attribute of the parser `p`

上代码吧：

```c++
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted.hpp> //必须要!将std数据结构adapted到fusion

#include <utility>
#include <iostream>

namespace my_parser{ 
 using namespace boost::spirit::x3;
 const rule<class test,std::vector<double>> test = "test";
 auto f = { _val(ctx).push_back(_attr(ctx)); };
 const auto test_def = double_[f] % "," ;
 BOOST_SPIRIT_DEFINE(test); 
}//namespace parser

template<typename Iterator>
bool parser_numbers(Iterator& first,Iterator end,std::vector<double>& v){
 using namespace boost::spirit::x3;
 return parse(first,end,my_parser::test,v);
}
```

上图中，每次double_匹配成功，就会将当前parser(double\_)中当前匹配值\_attr(ctx) 就添加到 该rule 的已匹配数据存储区（\_val(ctx)，即那个vector<double>)中。

3. 如果在parser使用operator[callable]，即使用自定义的[Parser Semantic Actions](http://ciere.com/cppnow15/x3_docs/spirit/quick_reference/semantic_actions.html)，相当于进行overload，屏蔽默认的行为（将parser 结果填入对应的attribute).如下面代码，如果把__note__注释中的代码替换该行，那么employee类型实例em无法得到parser值。

   ```
   #include <boost/spirit/home/x3.hpp>
   #include <boost/fusion/include/adapt_struct.hpp>
   #include <string>
   #include <iostream>

   namespace ast{
   struct employee{
   public:
   	void print(){
   		std::cout << age << "," << name << "," << forename << "," << salary << std::endl;
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
   	//auto f = [](auto& ctx) {std::cout << "gotta :" << _attr(ctx) << std::endl;};
   	const auto quota = lexeme['"' >> +(char_ - '"') >> '"'];
   	
   	const auto employee_def = \
   		lit("employee") >> \
   		'{' >> \
   		int_ >> lit(",") >> \  /*note : int_[f] >> lit(",") >>*/
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
   ```


## Reference

1. \<\<Inside Spirit X3\>\>
2. \<\<Using Spirit X3 to write Parser\>\>
3. [http://boost-spirit.com/home/2015/05/16/spirit-3-0-0/](http://boost-spirit.com/home/2015/05/16/spirit-3-0-0/)
4. [http://ciere.com/cppnow15/x3_docs/](http://ciere.com/cppnow15/x3_docs/)