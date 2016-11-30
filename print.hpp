#ifndef BOOST_PROTO17_PRINT_HPP_INCLUDED
#define BOOST_PROTO17_PRINT_HPP_INCLUDED

#include "expression_fwd.hpp"

#include <boost/hana/for_each.hpp>
#include <boost/type_index.hpp>
#include <iostream>


namespace boost::proto17 {

    namespace detail {

        inline std::ostream & print_kind (std::ostream & os, expr_kind kind)
        { return os << op_string(kind); }

        template <typename T>
        auto print_value (std::ostream & os, T const & x) -> decltype(os << x)
        { return os << x; }

        inline std::ostream & print_value (std::ostream & os, ...)
        { return os << "<<unprintable-value>>"; }

        template <typename T>
        std::ostream & print_type (std::ostream & os, hana::tuple<T> const &)
        {
            os << typeindex::type_id<T>().pretty_name();
            if (std::is_const_v<T>)
                os << " const";
            if (std::is_volatile_v<T>)
                os << " volatile";
            if (std::is_lvalue_reference_v<T>)
                os << " &";
            if (std::is_rvalue_reference_v<T>)
                os << " &&";
            return os;
        }

        bool is_const_expr_ref (...) { return false; }
        template <typename T, template <expr_kind, class> class expr_template>
        bool is_const_expr_ref (expr_template<expr_kind::expr_ref, hana::tuple<T const *>> const &) { return true; }

        template <typename Expr>
        std::ostream & print_impl (
            std::ostream & os,
            Expr const & expr,
            int indent,
            char const * indent_str,
            bool is_ref = false,
            bool is_const_ref = false)
        {
            if constexpr (Expr::kind == expr_kind::expr_ref) {
                print_impl(
                    os,
                    ::boost::proto17::value(expr),
                    indent,
                    indent_str,
                    true,
                    is_const_expr_ref(expr)
                );
            } else {
                for (int i = 0; i < indent; ++i) {
                    os << indent_str;
                }

                if constexpr (Expr::kind == expr_kind::terminal) {
                    os << "term<";
                    print_type(os, expr.elements);
                    os << ">[=";
                    print_value(os, ::boost::proto17::value(expr));
                    os << "]";
                    if (is_const_ref)
                        os << " const &";
                    else if (is_ref)
                        os << " &";
                    os << "\n";
                } else if constexpr (Expr::kind == expr_kind::placeholder) {
                    os << "placeholder<" << (long long)::boost::proto17::value(expr) << ">";
                    if (is_const_ref)
                        os << " const &";
                    else if (is_ref)
                        os << " &";
                    os << "\n";
                } else {
                    os << "expr<";
                    print_kind(os, Expr::kind);
                    os << ">";
                    if (is_const_ref)
                        os << " const &";
                    else if (is_ref)
                        os << " &";
                    os << "\n";
                    hana::for_each(expr.elements, [&os, indent, indent_str](auto const & element) {
                        print_impl(os, element, indent + 1, indent_str);
                    });
                }
            }

            return os;
        }

    }

    template <typename Expr>
    std::ostream & print (std::ostream & os, Expr const & expr)
    { return detail::print_impl(os, expr, 0, "    "); }

}

#endif
