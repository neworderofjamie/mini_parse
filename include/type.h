#pragma once

// Standard C includes
#include <cstdint>

// Standard C++ includes
#include <limits>

//----------------------------------------------------------------------------
// Macros
//----------------------------------------------------------------------------
#define DECLARE_TYPE(TYPE)                      \
    private:                                    \
        /*GENN_EXPORT*/ static TYPE *s_Instance;    \
    public:                                     \
        static const TYPE *getInstance()        \
        {                                       \
            if(s_Instance == NULL)              \
            {                                   \
                s_Instance = new TYPE;          \
            }                                   \
            return s_Instance;                  \
        }

#define DECLARE_NUMERIC_TYPE(TYPE, UNDERLYING_TYPE, RANK)   \
    class TYPE : public Numeric<UNDERLYING_TYPE, RANK>      \
    {                                                       \
        DECLARE_TYPE(TYPE)                                  \
    }

#define IMPLEMENT_TYPE(TYPE) TYPE *TYPE::s_Instance = NULL


//----------------------------------------------------------------------------
// Type::Base
//----------------------------------------------------------------------------
namespace Type
{
class Base
{
public:
    //------------------------------------------------------------------------
    // Declared virtuals
    //------------------------------------------------------------------------
    virtual size_t getSizeBytes() const = 0;
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

    //------------------------------------------------------------------------
    // NumericBase virtuals
    //------------------------------------------------------------------------
    virtual int getRank() const final { return Rank; }
    virtual double getMin() const final { return std::numeric_limits<T>::min(); }
    virtual double getMax() const final { return std::numeric_limits<T>::max(); }
    virtual double getLowest() const final { return std::numeric_limits<T>::lowest(); }
};

// Declare numeric types
DECLARE_NUMERIC_TYPE(Bool, bool, 0);
DECLARE_NUMERIC_TYPE(Int8, int8_t, 1);
DECLARE_NUMERIC_TYPE(Int16, int16_t, 2);
DECLARE_NUMERIC_TYPE(Int32, int32_t, 3);
DECLARE_NUMERIC_TYPE(Uint8, uint8_t, 1);
DECLARE_NUMERIC_TYPE(Uint16, uint16_t, 2);
DECLARE_NUMERIC_TYPE(Uint32, uint32_t, 3);
//DECLARE_NUMERIC_TYPE(Float, float);
//DECLARE_NUMERIC_TYPE(Double, double);
}   // namespace Type