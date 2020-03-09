// The MIT License

// Copyright (c) 2020 David Ziegler

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// Project is based on:
// https://stackoverflow.com/questions/12598029/how-to-calculate-boolean-expression-in-spirit/12602418#12602418

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/variant/recursive_wrapper.hpp>
#include <boost/lexical_cast.hpp>
#include <set> 


namespace qi    = boost::spirit::qi;
namespace phx   = boost::phoenix;

struct op_or  {};
struct op_and {};
struct op_not {};
struct op_not2 {};

typedef std::string var; 
template <typename tag> struct unop;
template <typename tag> struct binop;

typedef boost::variant<var, 
        boost::recursive_wrapper<unop <op_not> >, 
        boost::recursive_wrapper<binop<op_not2> >, 
        boost::recursive_wrapper<binop<op_and> >,
        boost::recursive_wrapper<binop<op_or> >
        > expr;

template <typename tag> struct binop
{
    explicit binop(const expr& l, const expr& r) : oper1(l), oper2(r) { }
    expr oper1, oper2;
};
template <typename tag> struct unop
{
    explicit unop(const expr& o) : oper1(o) { }
    expr oper1;
};

//logic parser template
template <typename T>
class LogicParser
{
    public:
        std::map<std::string, T> valueMap;
        std::string logic_expression;

        T parse()
        {
            typedef std::string::const_iterator It;
            
            It f(logic_expression.begin()), l(logic_expression.end());
            parser<It> p;
            
            T eval_result;

            try
            {
                expr result;
                bool ok = qi::phrase_parse(f,l,p,qi::space,result);

                if (!ok)
                    std::cerr << "invalid input\n";
                else
                {
                    eval_result = evaluate(result);
                }

            } catch (const qi::expectation_failure<It>& e)
            {
                std::cerr << "expectation_failure at '" << std::string(e.first, e.last) << "'\n";
            }

            if (f!=l) std::cerr << "unparsed: '" << std::string(f,l) << "'\n";

            return eval_result;
        };
        
        LogicParser(std::map<std::string, T>& _valueMap, std::string _logic_expression)
        : valueMap(_valueMap)
        , logic_expression(_logic_expression)
        {};

    protected:
        struct printer : boost::static_visitor<void> 
        {
            printer(std::ostream& os) : _os(os) {}
            std::ostream& _os;

            void operator()(const var& v) const { _os << v; }
            void operator()(const binop<op_and>& b) const { print(" & ", b.oper1, b.oper2); }
            void operator()(const binop<op_or >& b) const { print(" | ", b.oper1, b.oper2); }

            void print(const std::string& op, const expr& l, const expr& r) const
            {
                _os << "(";
                boost::apply_visitor(*this, l);
                _os << op;
                boost::apply_visitor(*this, r);
                _os << ")";
            }

            void operator()(const unop<op_not>& u) const
            {
                _os << "(";
                _os << "!";
                boost::apply_visitor(*this, u.oper1);
                _os << ")";
            } 
        };

        template <typename It, typename Skipper = qi::space_type>
        struct parser : qi::grammar<It, expr(), Skipper>
        {
            parser() : parser::base_type(expr_)
            {
                using namespace qi;

                expr_  = or_.alias();

                or_  = (and_ >> '|'  >> or_ ) [ _val = phx::construct<binop<op_or > >(_1, _2) ] | and_   [ _val = _1 ];
                and_ = (not2_ >> '&' >> and_)  [ _val = phx::construct<binop<op_and> >(_1, _2) ] | not2_   [ _val = _1 ];
                not2_ = (not_ >> '!'  >> not2_ ) [ _val = phx::construct<binop<op_not2> >(_1, _2) ] | not_   [ _val = _1 ];
                not_ = ('!' > simple       )  [ _val = phx::construct<unop <op_not> >(_1)     ] | simple [ _val = _1 ];
                

                simple = (('(' > expr_ > ')') | var_);
                var_ = qi::lexeme[ +(alpha|digit) ];

                BOOST_SPIRIT_DEBUG_NODE(expr_);
                BOOST_SPIRIT_DEBUG_NODE(or_);
                BOOST_SPIRIT_DEBUG_NODE(and_);
                BOOST_SPIRIT_DEBUG_NODE(not_);
                BOOST_SPIRIT_DEBUG_NODE(not2_);
                BOOST_SPIRIT_DEBUG_NODE(simple);
                BOOST_SPIRIT_DEBUG_NODE(var_);
            };

            private:
                qi::rule<It, var() , Skipper> var_;
                qi::rule<It, expr(), Skipper> not_, not2_, and_, or_, simple, expr_; 
        };

        struct eval: boost::static_visitor<T>{
            eval(std::map<std::string, T>& _valueMap) {};

            std::set<std::string> operator()(const var& v) const {};
            std::set<std::string> operator()(const binop<op_and>& b) const {};
            std::set<std::string> operator()(const binop<op_or>& b) const {};
            std::set<std::string> operator()(const unop<op_not>& u) const {};
            std::set<std::string> operator()(const binop<op_not2>& u) const {};

            private:
                template<typename T1>
                std::set<std::string> recurse(T1 const& v) const 
                { 
                    return boost::apply_visitor(*this, v); 
                }
        };
        

        virtual T evaluate(const expr& e)
        { 
            return boost::apply_visitor(eval(valueMap), e); 
        };
};

//logic parser for cpp sets, similiar to python set parsing
template <typename T>
class LogicParser_Set: public virtual LogicParser<std::set<T>>
{
    protected:
        struct eval: boost::static_visitor<std::set<T>> 
        {
            public:
                std::map<std::string, std::set<T>> valueMap;

                eval(const std::map<std::string, std::set<T>> &_valueMap) 
                : valueMap(_valueMap)
                {}

                std::set<T> operator()(const var& v) const 
                { 
                    return valueMap.at(v);
                }

                std::set<T> operator()(const binop<op_and>& b) const
                {
                    std::set<T> intersect;

                    auto v1 = recurse(b.oper1);
                    auto v2 = recurse(b.oper2);
                    std::set_intersection(v1.begin(), v1.end(),
                                        v2.begin(), v2.end(),
                                        std::inserter(intersect,intersect.begin()));
                    return intersect;
                }

                std::set<T> operator()(const binop<op_or>& b) const
                {
                    std::set<T> intersect;
                    auto v1 = recurse(b.oper1);
                    auto v2 = recurse(b.oper2);

                    intersect.insert(v1.begin(), v1.end());
                    intersect.insert(v2.begin(), v2.end());
                    return intersect;
                }

                std::set<T> operator()(const binop<op_not2>& b) const
                {
                    std::set<T> difference;

                    auto v1 = recurse(b.oper1);
                    auto v2 = recurse(b.oper2);
                    std::set_difference(v1.begin(), v1.end(),
                                        v2.begin(), v2.end(),
                                        std::inserter(difference,difference.begin()));
                    return difference;
                }

                std::set<T> operator()(const unop<op_not>& u) const
                {
                    return recurse(u.oper1);
                }
            private:
                template<typename T1>
                std::set<T> recurse(T1 const& v) const 
                { 
                    return boost::apply_visitor(*this, v); 
                }
        };
        
        std::set<T> evaluate(const expr& e) override
        { 
            return boost::apply_visitor(eval(this->valueMap), e); 
        };
    public:
        LogicParser_Set(std::map<std::string, std::set<T>> &_valueMap, std::string _logic_expression)
        : LogicParser<std::set<T>>(_valueMap, _logic_expression)
        {}

};
