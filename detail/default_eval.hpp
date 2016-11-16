#ifndef BOOST_PROTO17_DETAIL_DEFAULT_EVAL_HPP_INCLUDED
#define BOOST_PROTO17_DETAIL_DEFAULT_EVAL_HPP_INCLUDED

#include "../expression_fwd.hpp"
#include "../operators.hpp"

#include <boost/hana/transform.hpp>

#include <cassert>


namespace boost::proto17 {

    namespace detail {

        struct nonexistent_transform {};
        inline nonexistent_transform transform_expression (...) { return {}; }

        template <typename T>
        struct kind_of;

        template <expr_kind Kind, typename ...T>
        struct kind_of<expression<Kind, T...>>
        { static expr_kind const value = Kind; };

        template <typename I, typename T>
        decltype(auto) eval_placeholder (I, T && arg)
        {
            static_assert(I::value == 0);
            return static_cast<T &&>(arg);
        }

        template <typename I, typename T, typename ...Ts>
        auto eval_placeholder (I, T && arg, Ts &&... args)
        {
            if constexpr (I::value == 0) {
                return arg;
            } else {
                return eval_placeholder(hana::llong<I::value - 1>{}, static_cast<Ts &&>(args)...);
            }
        }

        template <typename Expr, typename ...T>
        decltype(auto) default_eval_expr (Expr const & expr, T &&... args)
        {
            constexpr expr_kind kind = kind_of<Expr>::value;

            using namespace hana::literals;

            if constexpr (
                !std::is_same_v<
                    decltype(transform_expression(expr, static_cast<T &&>(args)...)),
                    nonexistent_transform
                >
            ) {
                return transform_expression(expr, static_cast<T &&>(args)...);
            } else if constexpr (kind == expr_kind::terminal) {
                static_assert(decltype(hana::size(expr.elements))::value == 1UL);
                return expr.elements[0_c];
            } else if constexpr (kind == expr_kind::placeholder) {
                static_assert(decltype(hana::size(expr.elements))::value == 1UL);
                return eval_placeholder(expr.elements[0_c], static_cast<T &&>(args)...);
            }

#define BOOST_PROTO17_UNARY_OPERATOR_CASE(op_name)                      \
            else if constexpr (kind == expr_kind:: op_name) {           \
                return                                                  \
                    eval_ ## op_name(                                   \
                        default_eval_expr(expr.elements[0_c], static_cast<T &&>(args)...) \
                    );                                                  \
            }

            BOOST_PROTO17_UNARY_OPERATOR_CASE(unary_plus) // +
            BOOST_PROTO17_UNARY_OPERATOR_CASE(negate) // -
            BOOST_PROTO17_UNARY_OPERATOR_CASE(dereference) // *
            BOOST_PROTO17_UNARY_OPERATOR_CASE(complement) // ~
            BOOST_PROTO17_UNARY_OPERATOR_CASE(address_of) // &
            BOOST_PROTO17_UNARY_OPERATOR_CASE(logical_not) // !
            BOOST_PROTO17_UNARY_OPERATOR_CASE(pre_inc) // ++
            BOOST_PROTO17_UNARY_OPERATOR_CASE(pre_dec) // --
            BOOST_PROTO17_UNARY_OPERATOR_CASE(post_inc) // ++(int)
            BOOST_PROTO17_UNARY_OPERATOR_CASE(post_dec) // --(int)

#undef BOOST_PROTO17_UNARY_OPERATOR_CASE

#define BOOST_PROTO17_BINARY_OPERATOR_CASE(op_name)                     \
            else if constexpr (kind == expr_kind:: op_name) {           \
                return                                                  \
                    eval_ ## op_name(                                   \
                        default_eval_expr(expr.elements[0_c], static_cast<T &&>(args)...), \
                        default_eval_expr(expr.elements[1_c], static_cast<T &&>(args)...) \
                    );                                                  \
            }

            BOOST_PROTO17_BINARY_OPERATOR_CASE(shift_left) // <<
            BOOST_PROTO17_BINARY_OPERATOR_CASE(shift_right) // >>
            BOOST_PROTO17_BINARY_OPERATOR_CASE(multiplies) // *
            BOOST_PROTO17_BINARY_OPERATOR_CASE(divides) // /
            BOOST_PROTO17_BINARY_OPERATOR_CASE(modulus) // %
            BOOST_PROTO17_BINARY_OPERATOR_CASE(plus) // +
            BOOST_PROTO17_BINARY_OPERATOR_CASE(minus) // -
            BOOST_PROTO17_BINARY_OPERATOR_CASE(less) // <
            BOOST_PROTO17_BINARY_OPERATOR_CASE(greater) // >
            BOOST_PROTO17_BINARY_OPERATOR_CASE(less_equal) // <=
            BOOST_PROTO17_BINARY_OPERATOR_CASE(greater_equal) // >=
            BOOST_PROTO17_BINARY_OPERATOR_CASE(equal_to) // ==
            BOOST_PROTO17_BINARY_OPERATOR_CASE(not_equal_to) // !=
            BOOST_PROTO17_BINARY_OPERATOR_CASE(logical_or) // ||
            BOOST_PROTO17_BINARY_OPERATOR_CASE(logical_and) // &&
            BOOST_PROTO17_BINARY_OPERATOR_CASE(bitwise_and) // &
            BOOST_PROTO17_BINARY_OPERATOR_CASE(bitwise_or) // |
            BOOST_PROTO17_BINARY_OPERATOR_CASE(bitwise_xor) // ^

            else if constexpr (kind == expr_kind::comma) {
                return
                    eval_comma(
                        default_eval_expr(expr.elements[0_c], static_cast<T &&>(args)...),
                        default_eval_expr(expr.elements[1_c], static_cast<T &&>(args)...)
                    );
            }

            BOOST_PROTO17_BINARY_OPERATOR_CASE(mem_ptr) // ->*
            BOOST_PROTO17_BINARY_OPERATOR_CASE(assign) // =
            BOOST_PROTO17_BINARY_OPERATOR_CASE(shift_left_assign) // <<=
            BOOST_PROTO17_BINARY_OPERATOR_CASE(shift_right_assign) // >>=
            BOOST_PROTO17_BINARY_OPERATOR_CASE(multiplies_assign) // *=
            BOOST_PROTO17_BINARY_OPERATOR_CASE(divides_assign) // /=
            BOOST_PROTO17_BINARY_OPERATOR_CASE(modulus_assign) // %=
            BOOST_PROTO17_BINARY_OPERATOR_CASE(plus_assign) // +=
            BOOST_PROTO17_BINARY_OPERATOR_CASE(minus_assign) // -=
            BOOST_PROTO17_BINARY_OPERATOR_CASE(bitwise_and_assign) // &=
            BOOST_PROTO17_BINARY_OPERATOR_CASE(bitwise_or_assign) // |=
            BOOST_PROTO17_BINARY_OPERATOR_CASE(bitwise_xor_assign) // ^=
            BOOST_PROTO17_BINARY_OPERATOR_CASE(bitwise_xor_assign) // []

#undef BOOST_PROTO17_BINARY_OPERATOR_CASE

            else if constexpr (kind == expr_kind::call) {
                auto expand_args = [&](auto && element) {
                    return default_eval_expr(
                        static_cast<decltype(element) &&>(element),
                        static_cast<T &&>(args)...
                    );
                };

                return hana::unpack(
                    expr.elements,
                    [&] (auto && ... elements) {
                        return eval_call(
                            expand_args(static_cast<decltype(elements) &&>(elements))...
                        );
                    });
            } else {
                assert(false && "Unhandled expr_kind in default_evaluate!");
                return;
            }
        }

    }

}

#endif
