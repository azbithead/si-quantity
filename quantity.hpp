#pragma once
#include <type_traits>
#include <ratio>
#include <limits>
#include "units.hpp"

namespace si
{

// Forward decl
template <typename STORAGE, typename RATIO, typename UNITS > class quantity;

//------------------------------------------------------------------------------
/// @param _Tp a type to be tested
/// @return value true if _Tp is quantity, false otherwise
template <typename _Tp>
struct is_quantity_impl : std::false_type {};

template <typename UNITS, typename STORAGE, typename RATIO>
struct is_quantity_impl<quantity<STORAGE, RATIO, UNITS> > : std::true_type {};

template <typename T>
constexpr bool is_quantity = is_quantity_impl<typename std::remove_cv<T>::type>::value;

//------------------------------------------------------------------------------
/// @param _Xp an integer value
/// @param _Yp an integer value
/// @return value the greatest common divisor (GCD) of _Xp and _Yp
template <intmax_t _Xp, intmax_t _Yp>
struct gcd_impl
{
    static const intmax_t value = gcd_impl<_Yp, _Xp % _Yp>::value;
};

template <intmax_t _Xp>
struct gcd_impl<_Xp, 0>
{
    static const intmax_t value = _Xp;
};

template <>
struct gcd_impl<0, 0>
{
    static const intmax_t value = 1;
};

template <intmax_t _Xp, intmax_t _Yp>
constexpr intmax_t gcd = gcd_impl<_Xp,_Yp>::value;

//------------------------------------------------------------------------------
/// @param _Xp an integer value
/// @param _Yp an integer value
/// @member value the least common multiple (LCM) of _Xp and _Yp
template <intmax_t _Xp, intmax_t _Yp>
struct lcm_impl
{
    static const intmax_t value = _Xp / gcd<_Xp, _Yp> * _Yp;
};

template <intmax_t _Xp, intmax_t _Yp>
constexpr intmax_t lcm = lcm_impl<_Xp,_Yp>::value;

//------------------------------------------------------------------------------
/// @param _R1 a std::ratio
/// @param _R2 a std::ratio
/// @return the std::ratio that is the greatest common divisor (GCD) of _R1 and _R2
template <typename _R1, typename _R2>
using ratio_gcd = std::ratio
<
    gcd
    <
        _R1::num,
        _R2::num
    >,
    lcm
    <
        _R1::den,
        _R2::den
    >
>;

} // end of namespace si

//------------------------------------------------------------------------------
/// Specialization of std::common_type for quantity.
template
<
    typename UNITS,
    typename STORAGE1,
    typename RATIO1,
    typename STORAGE2,
    typename RATIO2
>
struct std::common_type
<
    si::quantity<STORAGE1, RATIO1, UNITS>,
    si::quantity<STORAGE2, RATIO2, UNITS>
>
{
    using type = si::quantity
    <
        typename std::common_type<STORAGE1, STORAGE2>::type,
        si::ratio_gcd<RATIO1, RATIO2>,
        UNITS
    >;
};

namespace si
{

template
<
    typename FromQ,
    typename ToQ,
    typename RATIO = typename std::ratio_divide
    <
        typename FromQ::ratio_t,
        typename ToQ::ratio_t
    >::type,
    bool = RATIO::num == 1,
    bool = RATIO::den == 1>
struct quantity_cast_impl;

template
<
    typename FromQ,
    typename ToQ,
    typename RATIO
>
struct quantity_cast_impl<FromQ, ToQ, RATIO, true, true>
{
    constexpr
    ToQ operator()(const FromQ& __fq) const
    {
        return ToQ
        (
            static_cast<typename ToQ::storage_t>(__fq.count())
        );
    }
};

template
<
    typename FromQ,
    typename ToQ,
    typename RATIO
>
struct quantity_cast_impl<FromQ, ToQ, RATIO, true, false>
{
    constexpr
    ToQ operator()(const FromQ& __fq) const
    {
        using _Cs = typename std::common_type
        <
            typename ToQ::storage_t,
            typename FromQ::storage_t,
            intmax_t
        >::type;
        return ToQ
        (
            static_cast<typename ToQ::storage_t>
            (
                static_cast<_Cs>(__fq.count()) /
                static_cast<_Cs>(RATIO::den)
            )
        );
    }
};

template
<
    typename FromQ,
    typename ToQ,
    typename RATIO
>
struct quantity_cast_impl<FromQ, ToQ, RATIO, false, true>
{
    constexpr
    ToQ operator()(const FromQ& __fq) const
    {
        using _Cs = typename std::common_type
        <
            typename ToQ::storage_t,
            typename FromQ::storage_t,
            intmax_t
        >::type;
        return ToQ
        (
            static_cast<typename ToQ::storage_t>
            (
                static_cast<_Cs>(__fq.count()) *
                static_cast<_Cs>(RATIO::num)
            )
        );
    }
};

template
<
    typename FromQ,
    typename ToQ,
    typename RATIO
>
struct quantity_cast_impl<FromQ, ToQ, RATIO, false, false>
{
    constexpr
    ToQ operator()(const FromQ& __fq) const
    {
        using _Cs = typename std::common_type
        <
            typename ToQ::storage_t,
            typename FromQ::storage_t,
            intmax_t
        >::type;
        return ToQ
        (
            static_cast<typename ToQ::storage_t>
            (
                static_cast<_Cs>(__fq.count()) *
                static_cast<_Cs>(RATIO::num) /
                static_cast<_Cs>(RATIO::den)
            )
        );
    }
};


//------------------------------------------------------------------------------
/// Cast a quantity to another quantity
/// Both quantities must have the same unit_t type.
template <typename ToQ, typename UNITS, typename STORAGE, typename RATIO>
inline
constexpr
typename std::enable_if
<
    is_quantity<ToQ> && std::is_same<typename ToQ::units_t,UNITS>::value,
    ToQ
>::type
quantity_cast(const quantity<STORAGE, RATIO, UNITS>& __fq)
{
    return quantity_cast_impl
    <
        quantity<STORAGE, RATIO, UNITS>,
        ToQ
    >()(__fq);
}

// some special quantity values
template <typename STORAGE>
struct quantity_values
{
public:
    static constexpr STORAGE zero() {return STORAGE(0);}
    static constexpr STORAGE max()  {return std::numeric_limits<STORAGE>::max();}
    static constexpr STORAGE min()  {return std::numeric_limits<STORAGE>::lowest();}
};

// is_ratio
template <typename _Tp>
struct is_ratio_impl : std::false_type{};

template <std::intmax_t _Num, std::intmax_t _Den>
struct is_ratio_impl<std::ratio<_Num, _Den>> : std::true_type{};

template <typename _Tp>
constexpr bool is_ratio = is_ratio_impl<_Tp>::value;

// quantity
template <typename STORAGE, typename RATIO, typename UNITS>
class quantity
{
    static_assert(std::is_arithmetic<STORAGE>::value, "First template parameter of si::quantity must be an arithmetic type");
    static_assert(is_ratio<RATIO>, "Second template parameter of si::quantity must be of type std::ratio");
    static_assert(RATIO::num > 0, "Second template parameter of si::quantity must have positive numerator");
    static_assert(is_units<UNITS>, "Third template parameter of si::quantity units must be of type si::units" );

    template <typename _R1, typename _R2>
    struct __no_overflow
    {
    private:
        static const intmax_t __gcd_n1_n2 = gcd<_R1::num, _R2::num>;
        static const intmax_t __gcd_d1_d2 = gcd<_R1::den, _R2::den>;
        static const intmax_t __n1 = _R1::num / __gcd_n1_n2;
        static const intmax_t __d1 = _R1::den / __gcd_d1_d2;
        static const intmax_t __n2 = _R2::num / __gcd_n1_n2;
        static const intmax_t __d2 = _R2::den / __gcd_d1_d2;
        static const intmax_t max = -((intmax_t(1) << (sizeof(intmax_t) * CHAR_BIT - 1)) + 1);

        template <intmax_t _Xp, intmax_t _Yp, bool __overflow>
        struct __mul    // __overflow == false
        {
            static const intmax_t value = _Xp * _Yp;
        };

        template <intmax_t _Xp, intmax_t _Yp>
        struct __mul<_Xp, _Yp, true>
        {
            static const intmax_t value = 1;
        };

    public:
        static const bool value = (__n1 <= max / __d2) && (__n2 <= max / __d1);
        using type = std::ratio<__mul<__n1, __d2, !value>::value,
                      __mul<__n2, __d1, !value>::value>;
    };

public:
    using units_t = UNITS;
    using storage_t = STORAGE;
    using ratio_t = RATIO;
private:
    storage_t mStorage;
public:

    constexpr
    quantity() = default;

    template <typename STORAGE2>
    constexpr
    explicit
    quantity
    (
        const STORAGE2& __s,
        typename std::enable_if
        <
            std::is_convertible<STORAGE2, storage_t>::value &&
            (
                std::is_floating_point<storage_t>::value ||
                !std::is_floating_point<STORAGE2>::value
            )
        >::type* = 0
    )
    :
    mStorage(__s) {}

    // conversions
    template <typename STORAGE2, typename RATIO2>
    constexpr
    quantity
    (
        const quantity<STORAGE2, RATIO2, UNITS>& __q,
        typename std::enable_if
        <
            __no_overflow<RATIO2, ratio_t>::value &&
            (
                std::is_floating_point<storage_t>::value ||
                (
                    __no_overflow<RATIO2, ratio_t>::type::den == 1 &&
                    !std::is_floating_point<STORAGE2>::value
                )
            )
        >::type* = 0
    )
    :
    mStorage(quantity_cast<quantity>(__q).count()) {}

    // observer

    constexpr storage_t count() const {return mStorage;}
    constexpr ratio_t ratio() const { return ratio_t{}; }
    constexpr units_t units() const { return units_t{}; }

    // arithmetic

    constexpr quantity  operator+() const {return *this;}
    constexpr quantity  operator-() const {return quantity(-mStorage);}
    quantity& operator++()      {++mStorage; return *this;}
    quantity  operator++(int)   {return quantity(mStorage++);}
    quantity& operator--()      {--mStorage; return *this;}
    quantity  operator--(int)   {return quantity(mStorage--);}

    quantity& operator+=(const quantity& __q) {mStorage += __q.count(); return *this;}
    quantity& operator-=(const quantity& __q) {mStorage -= __q.count(); return *this;}

    quantity& operator*=(const storage_t& rhs) {mStorage *= rhs; return *this;}
    quantity& operator/=(const storage_t& rhs) {mStorage /= rhs; return *this;}
    quantity& operator%=(const storage_t& rhs) {mStorage %= rhs; return *this;}
    quantity& operator%=(const quantity& rhs) {mStorage %= rhs.count(); return *this;}

    // special values

    static constexpr quantity zero() {return quantity(quantity_values<storage_t>::zero());}
    static constexpr quantity min()  {return quantity(quantity_values<storage_t>::min());}
    static constexpr quantity max()  {return quantity(quantity_values<storage_t>::max());}
};

// quantity ==

template <typename _LhsQ, typename _RhsQ>
struct quantity_eq_impl
{
    constexpr
    bool operator()(const _LhsQ& __lhs, const _RhsQ& __rhs) const
        {
            using _Cq = typename std::common_type<_LhsQ, _RhsQ>::type;
            return _Cq(__lhs).count() == _Cq(__rhs).count();
        }
};

template <typename _LhsQ>
struct quantity_eq_impl<_LhsQ, _LhsQ>
{
    constexpr
    bool operator()(const _LhsQ& __lhs, const _LhsQ& __rhs) const
        {return __lhs.count() == __rhs.count();}
};

template <typename UNITS, typename STORAGE1, typename RATIO1, typename STORAGE2, typename RATIO2>
inline
constexpr
bool
operator==
(
    const quantity<STORAGE1, RATIO1, UNITS>& __lhs,
    const quantity<STORAGE2, RATIO2, UNITS>& __rhs
)
{
    return quantity_eq_impl
    <
        quantity<STORAGE1, RATIO1, UNITS>,
        quantity<STORAGE2, RATIO2, UNITS>
    >()(__lhs, __rhs);
}

// quantity !=

template <typename UNITS, typename STORAGE1, typename RATIO1, typename STORAGE2, typename RATIO2>
inline
constexpr
bool
operator!=
(
    const quantity<STORAGE1, RATIO1, UNITS>& __lhs,
    const quantity<STORAGE2, RATIO2, UNITS>& __rhs
)
{
    return !(__lhs == __rhs);
}

// quantity <

template <typename _LhsQ, typename _RhsQ>
struct quantity_lt_impl
{
    constexpr
    bool operator()(const _LhsQ& __lhs, const _RhsQ& __rhs) const
    {
        using _Cq = typename std::common_type<_LhsQ, _RhsQ>::type;
        return _Cq(__lhs).count() < _Cq(__rhs).count();
    }
};

template <typename _LhsQ>
struct quantity_lt_impl<_LhsQ, _LhsQ>
{
    constexpr
    bool operator()
    (
        const _LhsQ& __lhs,
        const _LhsQ& __rhs
    ) const
    {
        return __lhs.count() < __rhs.count();
    }
};

template <typename UNITS, typename STORAGE1, typename RATIO1, typename STORAGE2, typename RATIO2>
inline
constexpr
bool
operator<
(
    const quantity<STORAGE1, RATIO1, UNITS>& __lhs,
    const quantity<STORAGE2, RATIO2, UNITS>& __rhs)
{
    return quantity_lt_impl
    <
        quantity<STORAGE1, RATIO1, UNITS>,
        quantity<STORAGE2, RATIO2, UNITS>
    >()(__lhs, __rhs);
}

// quantity >

template <typename UNITS, typename STORAGE1, typename RATIO1, typename STORAGE2, typename RATIO2>
inline
constexpr
bool
operator>
(
    const quantity<STORAGE1, RATIO1, UNITS>& __lhs,
    const quantity<STORAGE2, RATIO2, UNITS>& __rhs
)
{
    return __rhs < __lhs;
}

// quantity <=

template <typename UNITS, typename STORAGE1, typename RATIO1, typename STORAGE2, typename RATIO2>
inline
constexpr
bool
operator<=
(
    const quantity<STORAGE1, RATIO1, UNITS>& __lhs,
    const quantity<STORAGE2, RATIO2, UNITS>& __rhs
)
{
    return !(__rhs < __lhs);
}

// quantity >=

template <typename UNITS, typename STORAGE1, typename RATIO1, typename STORAGE2, typename RATIO2>
inline
constexpr
bool
operator>=
(
    const quantity<STORAGE1, RATIO1, UNITS>& __lhs,
    const quantity<STORAGE2, RATIO2, UNITS>& __rhs
)
{
    return !(__lhs < __rhs);
}

// quantity +

template <typename UNITS, typename STORAGE1, typename RATIO1, typename STORAGE2, typename RATIO2>
inline
constexpr
typename std::common_type
<
    quantity<STORAGE1, RATIO1, UNITS>,
    quantity<STORAGE2, RATIO2, UNITS>
>::type
operator+
(
    const quantity<STORAGE1, RATIO1, UNITS>& __lhs,
    const quantity<STORAGE2, RATIO2, UNITS>& __rhs
)
{
    using _Cq = typename std::common_type
    <
        quantity<STORAGE1, RATIO1, UNITS>,
        quantity<STORAGE2, RATIO2, UNITS>
    >::type;
    return _Cq(_Cq(__lhs).count() + _Cq(__rhs).count());
}

// quantity -

template <typename UNITS, typename STORAGE1, typename RATIO1, typename STORAGE2, typename RATIO2>
inline
constexpr
typename std::common_type
<
    quantity<STORAGE1, RATIO1, UNITS>,
    quantity<STORAGE2, RATIO2, UNITS>
>::type
operator-
(
    const quantity<STORAGE1, RATIO1, UNITS>& __lhs,
    const quantity<STORAGE2, RATIO2, UNITS>& __rhs
)
{
    using _Cq = typename std::common_type
    <
        quantity<STORAGE1, RATIO1, UNITS>,
        quantity<STORAGE2, RATIO2, UNITS>
    >::type;
    return _Cq(_Cq(__lhs).count() - _Cq(__rhs).count());
}

// quantity * scalar
template <typename STORAGE1, typename RATIO, typename UNITS, typename STORAGE2>
inline
constexpr
typename std::enable_if
<
    !is_quantity<STORAGE2> &&
    std::is_convertible
    <
        STORAGE2,
        typename std::common_type<STORAGE1, STORAGE2>::type
    >::value,
    quantity
    <
        typename std::common_type<STORAGE1, STORAGE2>::type,
        RATIO,
        UNITS
    >
>::type
operator*
(
    const quantity<STORAGE1, RATIO, UNITS>& __q,
    const STORAGE2& __s
)
{
    using _Cs = typename std::common_type<STORAGE1, STORAGE2>::type;
    using _Cq = quantity<_Cs, RATIO, UNITS>;
    return _Cq(_Cq(__q).count() * static_cast<_Cs>(__s));
}

// scalar * quantity
template <typename STORAGE1, typename RATIO, typename UNITS, typename STORAGE2>
inline
constexpr
typename std::enable_if
<
    !is_quantity<STORAGE2> &&
    std::is_convertible
    <
        STORAGE2,
        typename std::common_type<STORAGE1, STORAGE2>::type
    >::value,
    quantity
    <
        typename std::common_type<STORAGE1, STORAGE2>::type,
        RATIO,
        UNITS
    >
>::type
operator*
(
    const STORAGE2& __s,
    const quantity<STORAGE1, RATIO, UNITS>& __q
)
{
    return __q * __s;
}

// quantity * quantity
#if 0
template
<
    typename UNITS1,
    typename STORAGE1,
    typename RATIO1,
    typename UNITS2,
    typename STORAGE2,
    typename RATIO2
>
inline
constexpr
quantity
<
    typename std::common_type<STORAGE1, STORAGE2>::type,
    std::ratio_multiply<RATIO1, RATIO2>,
    multiply_units<UNITS1, UNITS2>
>
operator*
(
    const quantity<STORAGE1, RATIO1, UNITS1>& __lhs,
    const quantity<STORAGE2, RATIO2, UNITS2>& __rhs
)
{
    using _Cs = typename std::common_type<STORAGE1, STORAGE2>::type;
    using _Result = quantity
    <
        multiply_units<UNITS1, UNITS2>,
        _Cs,
        std::ratio_multiply<RATIO1, RATIO2>
    >;

    return _Result
    (
        static_cast<_Cs>( __lhs.count() )
        *
        static_cast<_Cs>( __rhs.count() )
    );
}
#endif

// quantity /

template
<
    typename _Q,
    typename STORAGE,
    bool = is_quantity<STORAGE>
>
struct __quantity_divide_result
{
};

template
<
    typename _Q,
    typename STORAGE,
    bool = std::is_convertible
    <
        STORAGE,
        typename std::common_type<typename _Q::storage_t, STORAGE>::type
    >::value
>
struct __quantity_divide_imp
{
};

template
<
    typename STORAGE1,
    typename RATIO,
    typename UNITS,
    typename STORAGE2
>
struct __quantity_divide_imp
<
    quantity<STORAGE1, RATIO, UNITS>,
    STORAGE2,
    true
>
{
    using type = quantity
    <
        typename std::common_type<STORAGE1, STORAGE2>::type,
        RATIO,
        UNITS
    >;
};

template
<
    typename STORAGE1,
    typename RATIO,
    typename UNITS,
    typename STORAGE2
>
struct __quantity_divide_result
<
    quantity<STORAGE1, RATIO, UNITS>,
    STORAGE2,
    false
>
: __quantity_divide_imp<quantity<STORAGE1, RATIO, UNITS>, STORAGE2>
{
};

// divide quantity by scalar
template
<
    typename STORAGE1,
    typename RATIO,
    typename UNITS,
    typename STORAGE2
>
inline
constexpr
typename __quantity_divide_result
<
    quantity<STORAGE1, RATIO, UNITS>,
    STORAGE2
>::type
operator/
(
    const quantity<STORAGE1, RATIO, UNITS>& __q,
    const STORAGE2& __s
)
{
    using _Cs = typename std::common_type<STORAGE1, STORAGE2>::type;
    using _Cq = quantity<_Cs, RATIO, UNITS>;
    return _Cq(_Cq(__q).count() / static_cast<_Cs>(__s));
}

// divide quantity by quantity, same units
template
<
    typename UNITS,
    typename STORAGE1,
    typename RATIO1,
    typename STORAGE2,
    typename RATIO2
>
inline
constexpr
typename std::common_type<STORAGE1, STORAGE2>::type
operator/
(
    const quantity<STORAGE1, RATIO1, UNITS>& __lhs,
    const quantity<STORAGE2, RATIO2, UNITS>& __rhs
)
{
    using _Cq = typename std::common_type
    <
        quantity<STORAGE1, RATIO1, UNITS>,
        quantity<STORAGE2, RATIO2, UNITS>
    >::type;
    return _Cq(__lhs).count() / _Cq(__rhs).count();
}

template
<
    typename STORAGE1,
    typename RATIO1,
    typename UNITS1,
    typename STORAGE2,
    typename RATIO2,
    typename UNITS2
>
using __diff_units_result = quantity
<
    typename std::common_type<STORAGE1, STORAGE2>::type,
    std::ratio_divide<RATIO1, RATIO2>,
    divide_units<UNITS1, UNITS2>
>;

// divide quantity by quantity, different units
template
<
    typename UNITS1,
    typename STORAGE1,
    typename RATIO1,
    typename UNITS2,
    typename STORAGE2,
    typename RATIO2
>
inline
constexpr
typename std::enable_if
<
    !std::is_same<UNITS1,UNITS2>::value,
    __diff_units_result<STORAGE1, RATIO1, UNITS1, STORAGE2, RATIO2, UNITS2>
>::type
operator/
(
    const quantity<STORAGE1, RATIO1, UNITS1>& __lhs,
    const quantity<STORAGE2, RATIO2, UNITS2>& __rhs
)
{
    using _Cq = __diff_units_result<STORAGE1, RATIO1, UNITS1, STORAGE2, RATIO2, UNITS2>;
    return _Cq( __lhs.count() / __rhs.count() );
}

// quantity %

template
<
    typename UNITS,
    typename STORAGE1,
    typename RATIO,
    typename STORAGE2
>
inline
constexpr
typename __quantity_divide_result<quantity<STORAGE1, RATIO, UNITS>, STORAGE2>::type
operator%
(
    const quantity<STORAGE1, RATIO, UNITS>& __q,
    const STORAGE2& __s
)
{
    using _Cs = typename std::common_type<STORAGE1, STORAGE2>::type;
    using _Cq = quantity<_Cs, RATIO, UNITS>;
    return _Cq(_Cq(__q).count() % static_cast<_Cs>(__s));
}

template
<
    typename UNITS,
    typename STORAGE1,
    typename RATIO1,
    typename STORAGE2,
    typename RATIO2
>
inline
constexpr
typename std::common_type<quantity<STORAGE1, RATIO1, UNITS>, quantity<STORAGE2, RATIO2, UNITS> >::type
operator%
(
    const quantity<STORAGE1, RATIO1, UNITS>& __lhs,
    const quantity<STORAGE2, RATIO2, UNITS>& __rhs
)
{
    using _Cs = typename std::common_type<STORAGE1, STORAGE2>::type;
    using _Cq = typename std::common_type<quantity<STORAGE1, RATIO1, UNITS>, quantity<STORAGE2, RATIO2, UNITS> >::type;
    return _Cq(static_cast<_Cs>(_Cq(__lhs).count()) % static_cast<_Cs>(_Cq(__rhs).count()));
}

// multiply to derived type
template
<
    typename UNITS,
    typename STORAGE1,
    typename STORAGE2,
    typename RATIO1,
    typename RATIO2
>
using multiply_result = quantity
<
    typename std::common_type<STORAGE1, STORAGE2>::type,
    typename std::ratio_multiply<RATIO1, RATIO2>,
    UNITS
>;

template
<
    typename UNITS,
    typename UNITS1,
    typename STORAGE1,
    typename RATIO1,
    typename UNITS2,
    typename STORAGE2,
    typename RATIO2
>
inline
constexpr
multiply_result<UNITS, STORAGE1, STORAGE2, RATIO1, RATIO2>
multiply
(
    const quantity<STORAGE1, RATIO1, UNITS1>& lhs,
    const quantity<STORAGE2, RATIO2, UNITS2>& rhs
)
{
    using RT = multiply_result<UNITS, STORAGE1, STORAGE2, RATIO1, RATIO2>;

    using CS = typename RT::storage_t;

    return RT{ CS(CS( lhs.count() ) * CS( rhs.count() )) };
}

// divide to derived type
template
<
    typename UNITS,
    typename STORAGE1,
    typename STORAGE2,
    typename RATIO1,
    typename RATIO2
>
using divide_result = quantity
<
    typename std::common_type<STORAGE1, STORAGE2>::type,
    typename std::ratio_divide<RATIO1, RATIO2>,
    UNITS
>;

template
<
    typename UNITS,
    typename UNITS1,
    typename STORAGE1,
    typename RATIO1,
    typename UNITS2,
    typename STORAGE2,
    typename RATIO2
>
inline
constexpr
divide_result<UNITS, STORAGE1, STORAGE2, RATIO1, RATIO2>
divide
(
    const quantity<STORAGE1, RATIO1, UNITS1>& lhs,
    const quantity<STORAGE2, RATIO2, UNITS2>& rhs
)
{
    using RT = divide_result<UNITS, STORAGE1, STORAGE2, RATIO1, RATIO2>;

    using CS = typename RT::storage_t;

    return RT{ CS(CS( lhs.count() ) / CS( rhs.count() )) };
}

// reciprocal to derived type
template
<
    typename UNITS,
    typename STORAGE,
    typename RATIO
>
using reciprocal_result = quantity
<
    STORAGE,
    std::ratio_divide<std::ratio<1>, RATIO>,
    UNITS
>;

template
<
    typename UNITS,
    typename UNITS1,
    typename STORAGE,
    typename RATIO
>
inline
constexpr
reciprocal_result<UNITS, STORAGE, RATIO>
reciprocal
(
    const quantity<STORAGE, RATIO, UNITS1>& input
)
{
    using RT = reciprocal_result<UNITS, STORAGE, RATIO>;

    using CS = typename RT::storage_t;

    return RT{ CS(1 / CS( input.count() )) };
}

} // end of namespace si
