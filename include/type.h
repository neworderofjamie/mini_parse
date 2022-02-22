#pragma once

// Standard C includes
#include <cstdint>

// Standard C++ includes
#include <limits>
#include <set>
#include <string_view>
#include <typeinfo>
#include <type_traits>
#include <vector>

//----------------------------------------------------------------------------
// Macros
//----------------------------------------------------------------------------
#define DECLARE_TYPE(TYPE)                          \
    private:                                        \
        /*GENN_EXPORT*/ static TYPE *s_Instance;    \
    public:                                         \
        static const TYPE *getInstance()            \
        {                                           \
            if(s_Instance == NULL)                  \
            {                                       \
                s_Instance = new TYPE;              \
            }                                       \
            return s_Instance;                      \
        }

#define DECLARE_NUMERIC_TYPE(TYPE, UNDERLYING_TYPE, RANK)                   \
    class TYPE : public Numeric<UNDERLYING_TYPE, RANK>                      \
    {                                                                       \
        DECLARE_TYPE(TYPE)                                                  \
        virtual std::string getTypeName() const{ return #UNDERLYING_TYPE; } \
    };                                                                      \
    template<>                                                              \
    struct TypeTraits<UNDERLYING_TYPE>                                      \
    {                                                                       \
        using NumericType = TYPE;                                           \
    }

#define DECLARE_FOREIGN_FUNCTION_TYPE(TYPE, RETURN_TYPE, ...)       \
    class TYPE : public ForeignFunction<RETURN_TYPE, __VA_ARGS__>   \
    {                                                               \
        DECLARE_TYPE(TYPE)                                          \
    }

#define IMPLEMENT_TYPE(TYPE) TYPE *TYPE::s_Instance = NULL

//----------------------------------------------------------------------------
// Type::TypeTraits
//----------------------------------------------------------------------------
namespace Type
{
//! Empty type trait structure
template<typename T>
struct TypeTraits
{
};

//----------------------------------------------------------------------------
// Type::Base
//----------------------------------------------------------------------------
//! Base class for all supported numericTypes
class Base
{
public:
    //------------------------------------------------------------------------
    // Declared virtuals
    //------------------------------------------------------------------------
    virtual std::string getTypeName() const = 0;
    virtual size_t getSizeBytes() const = 0;
    virtual size_t getTypeHash() const = 0;
};

//----------------------------------------------------------------------------
// Type::NumericBase
//----------------------------------------------------------------------------
class NumericBase : public Base
{
public:
    //------------------------------------------------------------------------
    // Declared virtuals
    //------------------------------------------------------------------------
    virtual int getRank() const = 0;
    virtual double getMin() const = 0;
    virtual double getMax() const = 0;
    virtual double getLowest() const = 0;
    virtual bool isSigned() const = 0;
    virtual bool isIntegral() const = 0;
};


//----------------------------------------------------------------------------
// Type::Numeric
//----------------------------------------------------------------------------
template<typename T, int Rank>
class Numeric : public NumericBase
{
public:
    //------------------------------------------------------------------------
    // Base virtuals
    //------------------------------------------------------------------------
    virtual size_t getSizeBytes() const final { return sizeof(T); }
    virtual size_t getTypeHash() const final { return typeid(T).hash_code(); }

    //------------------------------------------------------------------------
    // NumericBase virtuals
    //------------------------------------------------------------------------
    virtual int getRank() const final { return Rank; }
    virtual double getMin() const final { return std::numeric_limits<T>::min(); }
    virtual double getMax() const final { return std::numeric_limits<T>::max(); }
    virtual double getLowest() const final { return std::numeric_limits<T>::lowest(); }
    virtual bool isSigned() const final { return std::is_signed<T>::value; }
    virtual bool isIntegral() const final { return std::is_integral<T>::value; }
};

//----------------------------------------------------------------------------
// Type::ForeignFunctionBase
//----------------------------------------------------------------------------
class ForeignFunctionBase : public Base
{
public:
    //------------------------------------------------------------------------
    // Base virtuals
    //------------------------------------------------------------------------
    virtual std::string getTypeName() const = 0;
    virtual size_t getSizeBytes() const final { return 0; }
    virtual size_t getTypeHash() const = 0;

    //------------------------------------------------------------------------
    // Declared virtuals
    //------------------------------------------------------------------------
    virtual const NumericBase *getReturnType() const = 0;
    virtual std::vector<const NumericBase*> getArgumentTypes() const = 0;
};

//----------------------------------------------------------------------------
// Type::ForeignFunction
//----------------------------------------------------------------------------
template<typename ReturnType, typename ...ArgTypes>
class ForeignFunction : public ForeignFunctionBase
{
public:
    //------------------------------------------------------------------------
    // Base virtuals
    //------------------------------------------------------------------------
    virtual std::string getTypeName() const final
    {
        std::string typeName = getReturnType()->getTypeName() + "(";
        updateTypeName<ArgTypes...>(typeName);
        typeName += ")";
        return typeName;
    }

    virtual size_t getTypeHash() const final
    {
        // Start with seed of return type hash
        size_t seed = getReturnType()->getTypeHash();
        updateTypeHash<ArgTypes...>(seed);
        return seed;
    }

    //------------------------------------------------------------------------
    // ForeignFunctionBase virtuals
    //------------------------------------------------------------------------
    virtual const NumericBase *getReturnType() const final
    {
        return ReturnType::getInstance();
    }

    virtual std::vector<const NumericBase*> getArgumentTypes() const final
    {
        std::vector<const NumericBase*> args;
        args.reserve(sizeof...(ArgTypes));
        updateArgumentTypes<ArgTypes...>(args);
        return args;
    }

private:
    //------------------------------------------------------------------------
    // Private methods
    //------------------------------------------------------------------------
    template <typename T, typename... Args>
    static void updateTypeHash(size_t &seed)
    {
        // Combine hashes with argument type
        // **NOTE** this is the boost::hash_combine algorithm
        seed ^= T::getInstance()->getTypeHash() + 0x9e3779b9 + (seed << 6) + (seed >> 2);

        // If there are more arguments left in pack, recurse
        if constexpr (sizeof...(Args)) {
            updateTypeHash<Args...>(seed);
        }
    }

    template <typename T, typename... Args>
    static void updateTypeName(std::string &typeName)
    {
        // Add argument typename to string
        typeName += T::getInstance()->getTypeName();

        // If there are more arguments left in pack, add comma and recurse
        if constexpr (sizeof...(Args)) {
            typeName += ", ";
            updateTypeName<Args...>(typeName);
        }
    }

    template <typename T, typename... Args>
    static void updateArgumentTypes(std::vector<const NumericBase*> &args)
    {
        // Add argument typename to string
        args.push_back(T::getInstance());

        // If there are more arguments left in pack, recurse
        if constexpr (sizeof...(Args)) {
            updateArgumentTypes<Args...>(args);
        }
    }

};

//----------------------------------------------------------------------------
// Declare numeric types
//----------------------------------------------------------------------------
DECLARE_NUMERIC_TYPE(Bool, bool, 0);
DECLARE_NUMERIC_TYPE(Int8, int8_t, 10);
DECLARE_NUMERIC_TYPE(Int16, int16_t, 20);
DECLARE_NUMERIC_TYPE(Int32, int32_t, 30);
//DECLARE_NUMERIC_TYPE(Int64, int64_t, 40);
DECLARE_NUMERIC_TYPE(Uint8, uint8_t, 10);
DECLARE_NUMERIC_TYPE(Uint16, uint16_t, 20);
DECLARE_NUMERIC_TYPE(Uint32, uint32_t, 30);
//DECLARE_NUMERIC_TYPE(Uint64, uint64_t, 40);
DECLARE_NUMERIC_TYPE(Float, float, 50);
DECLARE_NUMERIC_TYPE(Double, double, 60);

//----------------------------------------------------------------------------
// Declare standard library foreign function types
//----------------------------------------------------------------------------
DECLARE_FOREIGN_FUNCTION_TYPE(Exp, Double, Double);

//! Look up type based on set of type specifiers
const NumericBase *getNumericType(const std::set<std::string_view> &typeSpecifiers);
const NumericBase *getPromotedType(const NumericBase *type);
const NumericBase *getCommonType(const NumericBase *a, const NumericBase *b);
}   // namespace Type