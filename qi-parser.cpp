
#include <boost/config/warning_disable.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/qi.hpp>

#include <iostream>
#include <string>
#include <vector>

namespace client {
namespace qi = boost::spirit::qi;
namespace phoenix = boost::phoenix;
namespace ascii = boost::spirit::ascii;

struct ToppingItem : std::pair<unsigned int, double> {};

struct Pizza {
  unsigned int id_;
  std::string name_;
  unsigned int n_toppings;
  std::vector<ToppingItem> toppings;
};

std::ostream &operator<<(std::ostream &os, const ToppingItem &topping) {
  os << '{' << topping.first << ", " << topping.second << "}";
  return os;
}

std::ostream &operator<<(std::ostream &os, const Pizza &pizza) {
  os << pizza.id_ << ", " << pizza.name_ << ", ";
  os << pizza.n_toppings << " {";
  for (const auto &topping : pizza.toppings) {
    os << topping << ", ";
  }
  os << "}";
  return os;
}

} // namespace client

// BOOST_FUSION_ADAPT_STRUCT has to be in the global scope

BOOST_FUSION_ADAPT_STRUCT(client::ToppingItem,
                          (unsigned int, first) (double, second))

BOOST_FUSION_ADAPT_STRUCT(client::Pizza,
                          (unsigned int, id_)
                          (std::string, name_)
                          (unsigned int, n_toppings)
                          (std::vector<client::ToppingItem>, toppings))

namespace client {

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

//! Skipper struct
//! Skips either whitespace or
//! a hash following by 0 or chars up to the end of line
template <typename Iterator>
struct skipper : qi::grammar<Iterator>
{
    skipper() : skipper::base_type(start)
    {
        qi::char_type char_;
        qi::eol_type eol;
        ascii::space_type space;

        start =
            space
            | '#' >> *(char_ - eol) >> eol
            ;
    }

    qi::rule<Iterator> start;
};

template <typename Iterator>
struct pizza_parser : qi::grammar<Iterator, Pizza(), skipper<Iterator> > {
  pizza_parser() : pizza_parser::base_type(start) {
    using ascii::char_;
    using qi::double_;
    using qi::lexeme;
    using qi::lit;
    using qi::no_case;
    using qi::uint_;
    using qi::fail;
    using qi::on_error;
    using namespace qi::labels;
    using phoenix::construct;
    using phoenix::val;
    using qi::eps;
    namespace phx = boost::phoenix;
    using qi::repeat;

    using boost::spirit::qi::uint_parser;
    using boost::spirit::qi::_1;

    quoted_string %= eps > lexeme['"' >> +(char_ - '"') >> '"'];

    topping_item %= eps >> uint_ >> double_;

    unsigned int n;

    start %= eps
        > uint_
        > quoted_string
        > uint_[phx::ref(n) = _1]
        > repeat(phx::ref(n))[topping_item];

    quoted_string.name("quoted_string");
    topping_item.name("topping_item");
    start.name("start");

    on_error<fail>
    (
        start
      , std::cout
            << val("Error! Expecting ")
            << _4                               // what failed?
            << val(" here: \"")
            << construct<std::string>(_3, _2)   // iterators to error-pos, end
            << val("\"")
            << std::endl
    );
  }

  qi::rule<Iterator, std::string(), skipper<Iterator> > quoted_string;
  qi::rule<Iterator, ToppingItem(), skipper<Iterator> > topping_item;
  qi::rule<Iterator, Pizza(), skipper<Iterator> > start;
};

} // namespace client

int main() {

  using boost::spirit::ascii::space;
  typedef std::string::const_iterator iterator_type;
  typedef client::pizza_parser<iterator_type> pizza_parser;
  client::skipper<iterator_type> skipper;

  pizza_parser g;  // Our grammar
  std::vector<std::string> input{
    "100           # pizza id\n"
    "\"hawaiian\"  # pizza name\n"
    "2             # number of toppings\n"
    "0 0.0         # topping index and quantity\n"
    "1 1.1         # topping index and quantity\n",
    "101 \"bbq chicken\" 3 0 0.0 1 1.1 2 2.2",
    "# This input has\n"
    "# several lines\n"
    "# of comments preceeding\n"
    "# the data\n"
    "101 \"farmhouse\" 3 0 0.0 1 1.1 2 2.2",
    "102 \"three cheese\" 4 0 0.0 1 1.1 2 2.2 3 3.3",
    "103 \"pepperoni\" 5 0 0.0 1 1.1 2 2.2 3 3.3 4 4.4  # Trailing comments not consumed by parser",
    "104 \"pizza#with#hashes#on#top\" 1 0 0.0",
  };
  for (const auto &str : input) {

    client::Pizza piz;
    std::string::const_iterator iter = str.cbegin();
    std::string::const_iterator end = str.cend();
    bool r = phrase_parse(iter, end, g, skipper, piz);

    std::cout << "================================================================" << std::endl;
    std::cout << str << std::endl;

    if (r) {
      std::cout << "Parsing succeeded" << std::endl;
      std::cout << "got: " << piz << std::endl;
      if (iter != end) {
        std::cout << "Some input remaining" << std::endl;
      }
    } else {
      std::cout << "Parsing failed\n";
    }
  }
  return 0;
}
