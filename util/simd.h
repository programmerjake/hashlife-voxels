/*
 * Copyright (C) 2012-2017 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef UTIL_SIMD_H_
#define UTIL_SIMD_H_

#include <cstdint>
#include <type_traits>
#include <array>

namespace programmerjake
{
namespace voxels
{
namespace util
{
struct SIMD256 final
{
    typedef float float32;
    static_assert(sizeof(float32) == sizeof(std::int32_t), "float32 is not 32 bits");
    typedef double float64;
    static_assert(sizeof(float64) == sizeof(std::int64_t), "float64 is not 64 bits");
#ifdef __GNUC__
    typedef std::int8_t i8x32 __attribute__((__vector_size__(sizeof(std::int8_t) * 32)));
    typedef std::uint8_t u8x32 __attribute__((__vector_size__(sizeof(std::uint8_t) * 32)));
    typedef std::int16_t i16x16 __attribute__((__vector_size__(sizeof(std::int16_t) * 16)));
    typedef std::uint16_t u16x16 __attribute__((__vector_size__(sizeof(std::uint16_t) * 16)));
    typedef std::int32_t i32x8 __attribute__((__vector_size__(sizeof(std::int32_t) * 8)));
    typedef std::uint32_t u32x8 __attribute__((__vector_size__(sizeof(std::uint32_t) * 8)));
    typedef std::int64_t i64x4 __attribute__((__vector_size__(sizeof(std::int64_t) * 4)));
    typedef std::uint64_t u64x4 __attribute__((__vector_size__(sizeof(std::uint64_t) * 4)));
    typedef float32 f32x8 __attribute__((__vector_size__(sizeof(float32) * 8)));
    typedef float64 f64x4 __attribute__((__vector_size__(sizeof(float64) * 4)));
#endif
    template <typename T>
    struct GetIntType final
    {
        template <typename T2>
        constexpr static T2 fn(T2 v) noexcept
        {
            return v;
        }
        constexpr static std::uint32_t fn(float32 v) noexcept
        {
            return v;
        }
        constexpr static std::uint64_t fn(float64 v) noexcept
        {
            return v;
        }
        typedef decltype(fn(std::declval<T>())) type;
    };
    template <typename Dest, typename Src>
    Dest bitCast(Src v) noexcept
    {
        static_assert(sizeof(Dest) == sizeof(Src), "invalid bit-cast");
        union
        {
            Src src{};
            Dest dest;
        };
        src = v;
        return dest;
    }
    union
    {
#ifdef __GNUC__
        i8x32 i8Value;
        u8x32 u8Value;
        i16x16 i16Value;
        u16x16 u16Value;
        i32x8 i32Value;
        u32x8 u32Value;
        i64x4 i64Value;
        u64x4 u64Value;
        f32x8 f32Value;
        f64x4 f64Value;
#else
        std::int8_t i8Value[32];
        std::uint8_t u8Value[32];
        std::int16_t i16Value[16];
        std::uint16_t u16Value[16];
        std::int32_t i32Value[8];
        std::uint32_t u32Value[8];
        std::int64_t i64Value[4];
        std::uint64_t u64Value[4];
        float f32Value[8];
        double f64Value[4];
#endif
        std::int8_t i8Array[32];
        std::uint8_t u8Array[32];
        std::int16_t i16Array[8];
        std::uint16_t u16Array[8];
        std::int32_t i32Array[8];
        std::uint32_t u32Array[8];
        std::int64_t i64Array[4];
        std::uint64_t u64Array[4];
        float32 f32Array[8];
        float64 f64Array[4];
    };
#ifdef __GNUC__
    constexpr SIMD256(i8x32 i8Value) noexcept : i8Value(i8Value)
    {
    }
    constexpr SIMD256(u8x32 u8Value) noexcept : u8Value(u8Value)
    {
    }
    constexpr SIMD256(i16x16 i16Value) noexcept : i16Value(i16Value)
    {
    }
    constexpr SIMD256(u16x16 u16Value) noexcept : u16Value(u16Value)
    {
    }
    constexpr SIMD256(i32x8 i32Value) noexcept : i32Value(i32Value)
    {
    }
    constexpr SIMD256(u32x8 u32Value) noexcept : u32Value(u32Value)
    {
    }
    constexpr SIMD256(i64x4 i64Value) noexcept : i64Value(i64Value)
    {
    }
    constexpr SIMD256(u64x4 u64Value) noexcept : u64Value(u64Value)
    {
    }
    constexpr SIMD256(f32x8 f32Value) noexcept : f32Value(f32Value)
    {
    }
    constexpr SIMD256(f64x4 f64Value) noexcept : f64Value(f64Value)
    {
    }
#endif
    constexpr SIMD256() noexcept : u8Value{}
    {
    }
    constexpr SIMD256(std::int8_t v) noexcept : i8Value{
                                                    v, v, v, v, v, v, v, v, //
                                                    v, v, v, v, v, v, v, v, //
                                                    v, v, v, v, v, v, v, v, //
                                                    v, v, v, v, v, v, v, v, //
                                                }
    {
    }
    constexpr SIMD256(std::int8_t v0,
                      std::int8_t v1,
                      std::int8_t v2,
                      std::int8_t v3,
                      std::int8_t v4,
                      std::int8_t v5,
                      std::int8_t v6,
                      std::int8_t v7,
                      std::int8_t v8,
                      std::int8_t v9,
                      std::int8_t v10,
                      std::int8_t v11,
                      std::int8_t v12,
                      std::int8_t v13,
                      std::int8_t v14,
                      std::int8_t v15,
                      std::int8_t v16,
                      std::int8_t v17,
                      std::int8_t v18,
                      std::int8_t v19,
                      std::int8_t v20,
                      std::int8_t v21,
                      std::int8_t v22,
                      std::int8_t v23,
                      std::int8_t v24,
                      std::int8_t v25,
                      std::int8_t v26,
                      std::int8_t v27,
                      std::int8_t v28,
                      std::int8_t v29,
                      std::int8_t v30,
                      std::int8_t v31) noexcept : i8Value{
                                                      v0,  v1,  v2,  v3,  v4,  v5,  v6,  v7, //
                                                      v8,  v9,  v10, v11, v12, v13, v14, v15, //
                                                      v16, v17, v18, v19, v20, v21, v22, v23, //
                                                      v24, v25, v26, v27, v28, v29, v30, v31,
                                                  }
    {
    }
    constexpr SIMD256(std::uint8_t v) noexcept : u8Value{
                                                     v, v, v, v, v, v, v, v, //
                                                     v, v, v, v, v, v, v, v, //
                                                     v, v, v, v, v, v, v, v, //
                                                     v, v, v, v, v, v, v, v, //
                                                 }
    {
    }
    constexpr SIMD256(std::uint8_t v0,
                      std::uint8_t v1,
                      std::uint8_t v2,
                      std::uint8_t v3,
                      std::uint8_t v4,
                      std::uint8_t v5,
                      std::uint8_t v6,
                      std::uint8_t v7,
                      std::uint8_t v8,
                      std::uint8_t v9,
                      std::uint8_t v10,
                      std::uint8_t v11,
                      std::uint8_t v12,
                      std::uint8_t v13,
                      std::uint8_t v14,
                      std::uint8_t v15,
                      std::uint8_t v16,
                      std::uint8_t v17,
                      std::uint8_t v18,
                      std::uint8_t v19,
                      std::uint8_t v20,
                      std::uint8_t v21,
                      std::uint8_t v22,
                      std::uint8_t v23,
                      std::uint8_t v24,
                      std::uint8_t v25,
                      std::uint8_t v26,
                      std::uint8_t v27,
                      std::uint8_t v28,
                      std::uint8_t v29,
                      std::uint8_t v30,
                      std::uint8_t v31) noexcept : u8Value{
                                                       v0,  v1,  v2,  v3,  v4,  v5,  v6,  v7, //
                                                       v8,  v9,  v10, v11, v12, v13, v14, v15, //
                                                       v16, v17, v18, v19, v20, v21, v22, v23, //
                                                       v24, v25, v26, v27, v28, v29, v30, v31,
                                                   }
    {
    }
    constexpr SIMD256(std::int16_t v) noexcept : i16Value{
                                                     v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v,
                                                 }
    {
    }
    constexpr SIMD256(std::int16_t v0,
                      std::int16_t v1,
                      std::int16_t v2,
                      std::int16_t v3,
                      std::int16_t v4,
                      std::int16_t v5,
                      std::int16_t v6,
                      std::int16_t v7,
                      std::int16_t v8,
                      std::int16_t v9,
                      std::int16_t v10,
                      std::int16_t v11,
                      std::int16_t v12,
                      std::int16_t v13,
                      std::int16_t v14,
                      std::int16_t v15) noexcept
        : i16Value{
              v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,
          }
    {
    }
    constexpr SIMD256(std::uint16_t v) noexcept
        : u16Value{
              v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v,
          }
    {
    }
    constexpr SIMD256(std::uint16_t v0,
                      std::uint16_t v1,
                      std::uint16_t v2,
                      std::uint16_t v3,
                      std::uint16_t v4,
                      std::uint16_t v5,
                      std::uint16_t v6,
                      std::uint16_t v7,
                      std::uint16_t v8,
                      std::uint16_t v9,
                      std::uint16_t v10,
                      std::uint16_t v11,
                      std::uint16_t v12,
                      std::uint16_t v13,
                      std::uint16_t v14,
                      std::uint16_t v15) noexcept
        : u16Value{
              v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,
          }
    {
    }
    constexpr SIMD256(std::int32_t v) noexcept : i32Value{
                                                     v, v, v, v, v, v, v, v,
                                                 }
    {
    }
    constexpr SIMD256(std::int32_t v0,
                      std::int32_t v1,
                      std::int32_t v2,
                      std::int32_t v3,
                      std::int32_t v4,
                      std::int32_t v5,
                      std::int32_t v6,
                      std::int32_t v7) noexcept : i32Value{
                                                      v0, v1, v2, v3, v4, v5, v6, v7,
                                                  }
    {
    }
    constexpr SIMD256(std::uint32_t v) noexcept : u32Value{
                                                      v, v, v, v, v, v, v, v,
                                                  }
    {
    }
    constexpr SIMD256(std::uint32_t v0,
                      std::uint32_t v1,
                      std::uint32_t v2,
                      std::uint32_t v3,
                      std::uint32_t v4,
                      std::uint32_t v5,
                      std::uint32_t v6,
                      std::uint32_t v7) noexcept : u32Value{
                                                       v0, v1, v2, v3, v4, v5, v6, v7,
                                                   }
    {
    }
    constexpr SIMD256(std::int64_t v) noexcept : i64Value{
                                                     v, v, v, v,
                                                 }
    {
    }
    constexpr SIMD256(std::int64_t v0, std::int64_t v1, std::int64_t v2, std::int64_t v3) noexcept
        : i64Value{
              v0, v1, v2, v3,
          }
    {
    }
    constexpr SIMD256(std::uint64_t v) noexcept : u64Value{
                                                      v, v, v, v,
                                                  }
    {
    }
    constexpr SIMD256(std::uint64_t v0,
                      std::uint64_t v1,
                      std::uint64_t v2,
                      std::uint64_t v3) noexcept : u64Value{
                                                       v0, v1, v2, v3,
                                                   }
    {
    }
    constexpr SIMD256(float32 v) noexcept : f32Value{
                                                v, v, v, v, v, v, v, v,
                                            }
    {
    }
    constexpr SIMD256(float32 v0,
                      float32 v1,
                      float32 v2,
                      float32 v3,
                      float32 v4,
                      float32 v5,
                      float32 v6,
                      float32 v7) noexcept : f32Value{
                                                 v0, v1, v2, v3, v4, v5, v6, v7,
                                             }
    {
    }
    constexpr SIMD256(float64 v) noexcept : f64Value{
                                                v, v, v, v,
                                            }
    {
    }
    constexpr SIMD256(float64 v0, float64 v1, float64 v2, float64 v3) noexcept : f64Value{
                                                                                     v0, v1, v2, v3,
                                                                                 }
    {
    }
    enum class BinOp
    {
        Add,
        Sub,
        Mul,
        Div,
        Xor,
        Or,
        And,
        CmpEq,
        CmpNE,
        CmpLT,
        CmpGT,
        CmpLE,
        CmpGE,
    };
    template <BinOp Op, typename S = void>
    struct BinOpImpHelper
    {
        template <typename T>
        static constexpr T run(T a, T b) noexcept = delete;
    };
    template <typename S>
    struct BinOpImpHelper<BinOp::Add, S>
    {
        template <typename T>
        static constexpr T run(T a, T b) noexcept
        {
            return a + b;
        }
    };
    template <typename S>
    struct BinOpImpHelper<BinOp::Sub, S>
    {
        template <typename T>
        static constexpr T run(T a, T b) noexcept
        {
            return a - b;
        }
    };
    template <typename S>
    struct BinOpImpHelper<BinOp::Mul, S>
    {
        template <typename T>
        static constexpr T run(T a, T b) noexcept
        {
            return a * b;
        }
    };
    template <typename S>
    struct BinOpImpHelper<BinOp::Div, S>
    {
        template <typename T>
        static constexpr T run(T a, T b) noexcept
        {
            return a / b;
        }
    };
    template <typename S>
    struct BinOpImpHelper<BinOp::Xor, S>
    {
        template <typename T>
        static constexpr T run(T a, T b) noexcept
        {
            typedef typename GetIntType<T>::type IntType;
            return bitCast<T, IntType>(bitCast<IntType>(a) ^ bitCast<IntType>(b));
        }
    };
    template <typename S>
    struct BinOpImpHelper<BinOp::Or, S>
    {
        template <typename T>
        static constexpr T run(T a, T b) noexcept
        {
            typedef typename GetIntType<T>::type IntType;
            return bitCast<T, IntType>(bitCast<IntType>(a) | bitCast<IntType>(b));
        }
    };
    template <typename S>
    struct BinOpImpHelper<BinOp::And, S>
    {
        template <typename T>
        static constexpr T run(T a, T b) noexcept
        {
            typedef typename GetIntType<T>::type IntType;
            return bitCast<T, IntType>(bitCast<IntType>(a) & bitCast<IntType>(b));
        }
    };
    template <typename S>
    struct BinOpImpHelper<BinOp::CmpEq, S>
    {
        template <typename T>
        static constexpr typename GetIntType<T>::type run(T a, T b) noexcept
        {
            return a == b ? -1 : 0;
        }
    };
    template <typename S>
    struct BinOpImpHelper<BinOp::CmpNE, S>
    {
        template <typename T>
        static constexpr typename GetIntType<T>::type run(T a, T b) noexcept
        {
            return a != b ? -1 : 0;
        }
    };
    template <typename S>
    struct BinOpImpHelper<BinOp::CmpLT, S>
    {
        template <typename T>
        static constexpr typename GetIntType<T>::type run(T a, T b) noexcept
        {
            return a < b ? -1 : 0;
        }
    };
    template <typename S>
    struct BinOpImpHelper<BinOp::CmpGT, S>
    {
        template <typename T>
        static constexpr typename GetIntType<T>::type run(T a, T b) noexcept
        {
            return a > b ? -1 : 0;
        }
    };
    template <typename S>
    struct BinOpImpHelper<BinOp::CmpLE, S>
    {
        template <typename T>
        static constexpr typename GetIntType<T>::type run(T a, T b) noexcept
        {
            return a <= b ? -1 : 0;
        }
    };
    template <typename S>
    struct BinOpImpHelper<BinOp::CmpGE, S>
    {
        template <typename T>
        static constexpr typename GetIntType<T>::type run(T a, T b) noexcept
        {
            return a >= b ? -1 : 0;
        }
    };
    template <typename T, BinOp Op, bool specialized = true>
    struct BinOpImp
    {
        static constexpr SIMD256 run(const SIMD256 &a, const SIMD256 &b) noexcept = delete;
    };
    template <BinOp Op, bool specialized>
    struct BinOpImp<std::uint8_t, Op, specialized>
    {
        static constexpr SIMD256 run(const SIMD256 &a, const SIMD256 &b) noexcept
        {
            return SIMD256(BinOpImpHelper<Op>::run(a.u8Value[0], b.u8Value[0]),
                           BinOpImpHelper<Op>::run(a.u8Value[1], b.u8Value[1]),
                           BinOpImpHelper<Op>::run(a.u8Value[2], b.u8Value[2]),
                           BinOpImpHelper<Op>::run(a.u8Value[3], b.u8Value[3]),
                           BinOpImpHelper<Op>::run(a.u8Value[4], b.u8Value[4]),
                           BinOpImpHelper<Op>::run(a.u8Value[5], b.u8Value[5]),
                           BinOpImpHelper<Op>::run(a.u8Value[6], b.u8Value[6]),
                           BinOpImpHelper<Op>::run(a.u8Value[7], b.u8Value[7]),
                           BinOpImpHelper<Op>::run(a.u8Value[8], b.u8Value[8]),
                           BinOpImpHelper<Op>::run(a.u8Value[9], b.u8Value[9]),
                           BinOpImpHelper<Op>::run(a.u8Value[10], b.u8Value[10]),
                           BinOpImpHelper<Op>::run(a.u8Value[11], b.u8Value[11]),
                           BinOpImpHelper<Op>::run(a.u8Value[12], b.u8Value[12]),
                           BinOpImpHelper<Op>::run(a.u8Value[13], b.u8Value[13]),
                           BinOpImpHelper<Op>::run(a.u8Value[14], b.u8Value[14]),
                           BinOpImpHelper<Op>::run(a.u8Value[15], b.u8Value[15]),
                           BinOpImpHelper<Op>::run(a.u8Value[16], b.u8Value[16]),
                           BinOpImpHelper<Op>::run(a.u8Value[17], b.u8Value[17]),
                           BinOpImpHelper<Op>::run(a.u8Value[18], b.u8Value[18]),
                           BinOpImpHelper<Op>::run(a.u8Value[19], b.u8Value[19]),
                           BinOpImpHelper<Op>::run(a.u8Value[20], b.u8Value[20]),
                           BinOpImpHelper<Op>::run(a.u8Value[21], b.u8Value[21]),
                           BinOpImpHelper<Op>::run(a.u8Value[22], b.u8Value[22]),
                           BinOpImpHelper<Op>::run(a.u8Value[23], b.u8Value[23]),
                           BinOpImpHelper<Op>::run(a.u8Value[24], b.u8Value[24]),
                           BinOpImpHelper<Op>::run(a.u8Value[25], b.u8Value[25]),
                           BinOpImpHelper<Op>::run(a.u8Value[26], b.u8Value[26]),
                           BinOpImpHelper<Op>::run(a.u8Value[27], b.u8Value[27]),
                           BinOpImpHelper<Op>::run(a.u8Value[28], b.u8Value[28]),
                           BinOpImpHelper<Op>::run(a.u8Value[29], b.u8Value[29]),
                           BinOpImpHelper<Op>::run(a.u8Value[30], b.u8Value[30]),
                           BinOpImpHelper<Op>::run(a.u8Value[31], b.u8Value[31]));
        }
    };
    template <BinOp Op, bool specialized>
    struct BinOpImp<std::int8_t, Op, specialized>
    {
        static constexpr SIMD256 run(const SIMD256 &a, const SIMD256 &b) noexcept
        {
            return SIMD256(BinOpImpHelper<Op>::run(a.i8Value[0], b.i8Value[0]),
                           BinOpImpHelper<Op>::run(a.i8Value[1], b.i8Value[1]),
                           BinOpImpHelper<Op>::run(a.i8Value[2], b.i8Value[2]),
                           BinOpImpHelper<Op>::run(a.i8Value[3], b.i8Value[3]),
                           BinOpImpHelper<Op>::run(a.i8Value[4], b.i8Value[4]),
                           BinOpImpHelper<Op>::run(a.i8Value[5], b.i8Value[5]),
                           BinOpImpHelper<Op>::run(a.i8Value[6], b.i8Value[6]),
                           BinOpImpHelper<Op>::run(a.i8Value[7], b.i8Value[7]),
                           BinOpImpHelper<Op>::run(a.i8Value[8], b.i8Value[8]),
                           BinOpImpHelper<Op>::run(a.i8Value[9], b.i8Value[9]),
                           BinOpImpHelper<Op>::run(a.i8Value[10], b.i8Value[10]),
                           BinOpImpHelper<Op>::run(a.i8Value[11], b.i8Value[11]),
                           BinOpImpHelper<Op>::run(a.i8Value[12], b.i8Value[12]),
                           BinOpImpHelper<Op>::run(a.i8Value[13], b.i8Value[13]),
                           BinOpImpHelper<Op>::run(a.i8Value[14], b.i8Value[14]),
                           BinOpImpHelper<Op>::run(a.i8Value[15], b.i8Value[15]),
                           BinOpImpHelper<Op>::run(a.i8Value[16], b.i8Value[16]),
                           BinOpImpHelper<Op>::run(a.i8Value[17], b.i8Value[17]),
                           BinOpImpHelper<Op>::run(a.i8Value[18], b.i8Value[18]),
                           BinOpImpHelper<Op>::run(a.i8Value[19], b.i8Value[19]),
                           BinOpImpHelper<Op>::run(a.i8Value[20], b.i8Value[20]),
                           BinOpImpHelper<Op>::run(a.i8Value[21], b.i8Value[21]),
                           BinOpImpHelper<Op>::run(a.i8Value[22], b.i8Value[22]),
                           BinOpImpHelper<Op>::run(a.i8Value[23], b.i8Value[23]),
                           BinOpImpHelper<Op>::run(a.i8Value[24], b.i8Value[24]),
                           BinOpImpHelper<Op>::run(a.i8Value[25], b.i8Value[25]),
                           BinOpImpHelper<Op>::run(a.i8Value[26], b.i8Value[26]),
                           BinOpImpHelper<Op>::run(a.i8Value[27], b.i8Value[27]),
                           BinOpImpHelper<Op>::run(a.i8Value[28], b.i8Value[28]),
                           BinOpImpHelper<Op>::run(a.i8Value[29], b.i8Value[29]),
                           BinOpImpHelper<Op>::run(a.i8Value[30], b.i8Value[30]),
                           BinOpImpHelper<Op>::run(a.i8Value[31], b.i8Value[31]));
        }
    };
    template <BinOp Op, bool specialized>
    struct BinOpImp<std::uint16_t, Op, specialized>
    {
        static constexpr SIMD256 run(const SIMD256 &a, const SIMD256 &b) noexcept
        {
            return SIMD256(BinOpImpHelper<Op>::run(a.u16Value[0], b.u16Value[0]),
                           BinOpImpHelper<Op>::run(a.u16Value[1], b.u16Value[1]),
                           BinOpImpHelper<Op>::run(a.u16Value[2], b.u16Value[2]),
                           BinOpImpHelper<Op>::run(a.u16Value[3], b.u16Value[3]),
                           BinOpImpHelper<Op>::run(a.u16Value[4], b.u16Value[4]),
                           BinOpImpHelper<Op>::run(a.u16Value[5], b.u16Value[5]),
                           BinOpImpHelper<Op>::run(a.u16Value[6], b.u16Value[6]),
                           BinOpImpHelper<Op>::run(a.u16Value[7], b.u16Value[7]),
                           BinOpImpHelper<Op>::run(a.u16Value[8], b.u16Value[8]),
                           BinOpImpHelper<Op>::run(a.u16Value[9], b.u16Value[9]),
                           BinOpImpHelper<Op>::run(a.u16Value[10], b.u16Value[10]),
                           BinOpImpHelper<Op>::run(a.u16Value[11], b.u16Value[11]),
                           BinOpImpHelper<Op>::run(a.u16Value[12], b.u16Value[12]),
                           BinOpImpHelper<Op>::run(a.u16Value[13], b.u16Value[13]),
                           BinOpImpHelper<Op>::run(a.u16Value[14], b.u16Value[14]),
                           BinOpImpHelper<Op>::run(a.u16Value[15], b.u16Value[15]));
        }
    };
    template <BinOp Op, bool specialized>
    struct BinOpImp<std::int16_t, Op, specialized>
    {
        static constexpr SIMD256 run(const SIMD256 &a, const SIMD256 &b) noexcept
        {
            return SIMD256(BinOpImpHelper<Op>::run(a.i16Value[0], b.i16Value[0]),
                           BinOpImpHelper<Op>::run(a.i16Value[1], b.i16Value[1]),
                           BinOpImpHelper<Op>::run(a.i16Value[2], b.i16Value[2]),
                           BinOpImpHelper<Op>::run(a.i16Value[3], b.i16Value[3]),
                           BinOpImpHelper<Op>::run(a.i16Value[4], b.i16Value[4]),
                           BinOpImpHelper<Op>::run(a.i16Value[5], b.i16Value[5]),
                           BinOpImpHelper<Op>::run(a.i16Value[6], b.i16Value[6]),
                           BinOpImpHelper<Op>::run(a.i16Value[7], b.i16Value[7]),
                           BinOpImpHelper<Op>::run(a.i16Value[8], b.i16Value[8]),
                           BinOpImpHelper<Op>::run(a.i16Value[9], b.i16Value[9]),
                           BinOpImpHelper<Op>::run(a.i16Value[10], b.i16Value[10]),
                           BinOpImpHelper<Op>::run(a.i16Value[11], b.i16Value[11]),
                           BinOpImpHelper<Op>::run(a.i16Value[12], b.i16Value[12]),
                           BinOpImpHelper<Op>::run(a.i16Value[13], b.i16Value[13]),
                           BinOpImpHelper<Op>::run(a.i16Value[14], b.i16Value[14]),
                           BinOpImpHelper<Op>::run(a.i16Value[15], b.i16Value[15]));
        }
    };
    template <BinOp Op, bool specialized>
    struct BinOpImp<std::uint32_t, Op, specialized>
    {
        static constexpr SIMD256 run(const SIMD256 &a, const SIMD256 &b) noexcept
        {
            return SIMD256(BinOpImpHelper<Op>::run(a.u32Value[0], b.u32Value[0]),
                           BinOpImpHelper<Op>::run(a.u32Value[1], b.u32Value[1]),
                           BinOpImpHelper<Op>::run(a.u32Value[2], b.u32Value[2]),
                           BinOpImpHelper<Op>::run(a.u32Value[3], b.u32Value[3]),
                           BinOpImpHelper<Op>::run(a.u32Value[4], b.u32Value[4]),
                           BinOpImpHelper<Op>::run(a.u32Value[5], b.u32Value[5]),
                           BinOpImpHelper<Op>::run(a.u32Value[6], b.u32Value[6]),
                           BinOpImpHelper<Op>::run(a.u32Value[7], b.u32Value[7]));
        }
    };
    template <BinOp Op, bool specialized>
    struct BinOpImp<std::int32_t, Op, specialized>
    {
        static constexpr SIMD256 run(const SIMD256 &a, const SIMD256 &b) noexcept
        {
            return SIMD256(BinOpImpHelper<Op>::run(a.i32Value[0], b.i32Value[0]),
                           BinOpImpHelper<Op>::run(a.i32Value[1], b.i32Value[1]),
                           BinOpImpHelper<Op>::run(a.i32Value[2], b.i32Value[2]),
                           BinOpImpHelper<Op>::run(a.i32Value[3], b.i32Value[3]),
                           BinOpImpHelper<Op>::run(a.i32Value[4], b.i32Value[4]),
                           BinOpImpHelper<Op>::run(a.i32Value[5], b.i32Value[5]),
                           BinOpImpHelper<Op>::run(a.i32Value[6], b.i32Value[6]),
                           BinOpImpHelper<Op>::run(a.i32Value[7], b.i32Value[7]));
        }
    };
    template <BinOp Op, bool specialized>
    struct BinOpImp<float32, Op, specialized>
    {
        static constexpr SIMD256 run(const SIMD256 &a, const SIMD256 &b) noexcept
        {
            return SIMD256(BinOpImpHelper<Op>::run(a.f32Value[0], b.f32Value[0]),
                           BinOpImpHelper<Op>::run(a.f32Value[1], b.f32Value[1]),
                           BinOpImpHelper<Op>::run(a.f32Value[2], b.f32Value[2]),
                           BinOpImpHelper<Op>::run(a.f32Value[3], b.f32Value[3]),
                           BinOpImpHelper<Op>::run(a.f32Value[4], b.f32Value[4]),
                           BinOpImpHelper<Op>::run(a.f32Value[5], b.f32Value[5]),
                           BinOpImpHelper<Op>::run(a.f32Value[6], b.f32Value[6]),
                           BinOpImpHelper<Op>::run(a.f32Value[7], b.f32Value[7]));
        }
    };
    template <BinOp Op, bool specialized>
    struct BinOpImp<std::uint64_t, Op, specialized>
    {
        static constexpr SIMD256 run(const SIMD256 &a, const SIMD256 &b) noexcept
        {
            return SIMD256(BinOpImpHelper<Op>::run(a.u64Value[0], b.u64Value[0]),
                           BinOpImpHelper<Op>::run(a.u64Value[1], b.u64Value[1]),
                           BinOpImpHelper<Op>::run(a.u64Value[2], b.u64Value[2]),
                           BinOpImpHelper<Op>::run(a.u64Value[3], b.u64Value[3]));
        }
    };
    template <BinOp Op, bool specialized>
    struct BinOpImp<std::int64_t, Op, specialized>
    {
        static constexpr SIMD256 run(const SIMD256 &a, const SIMD256 &b) noexcept
        {
            return SIMD256(BinOpImpHelper<Op>::run(a.i64Value[0], b.i64Value[0]),
                           BinOpImpHelper<Op>::run(a.i64Value[1], b.i64Value[1]),
                           BinOpImpHelper<Op>::run(a.i64Value[2], b.i64Value[2]),
                           BinOpImpHelper<Op>::run(a.i64Value[3], b.i64Value[3]));
        }
    };
    template <BinOp Op, bool specialized>
    struct BinOpImp<float64, Op, specialized>
    {
        static constexpr SIMD256 run(const SIMD256 &a, const SIMD256 &b) noexcept
        {
            return SIMD256(BinOpImpHelper<Op>::run(a.f64Value[0], b.f64Value[0]),
                           BinOpImpHelper<Op>::run(a.f64Value[1], b.f64Value[1]),
                           BinOpImpHelper<Op>::run(a.f64Value[2], b.f64Value[2]),
                           BinOpImpHelper<Op>::run(a.f64Value[3], b.f64Value[3]));
        }
    };
    template <typename T>
    static constexpr SIMD256 add(const SIMD256 &a, const SIMD256 &b) noexcept
    {
        return BinOpImp<T, BinOp::Add>::run(a, b);
    }
    template <typename T>
    SIMD256 &addAssign(const SIMD256 &b) noexcept
    {
        return *this = BinOpImp<T, BinOp::Add>::run(*this, b);
    }
    template <typename T>
    static constexpr SIMD256 sub(const SIMD256 &a, const SIMD256 &b) noexcept
    {
        return BinOpImp<T, BinOp::Sub>::run(a, b);
    }
    template <typename T>
    SIMD256 &subAssign(const SIMD256 &b) noexcept
    {
        return *this = BinOpImp<T, BinOp::Sub>::run(*this, b);
    }
    template <typename T>
    static constexpr SIMD256 mul(const SIMD256 &a, const SIMD256 &b) noexcept
    {
        return BinOpImp<T, BinOp::Mul>::run(a, b);
    }
    template <typename T>
    SIMD256 &mulAssign(const SIMD256 &b) noexcept
    {
        return *this = BinOpImp<T, BinOp::Mul>::run(*this, b);
    }
    template <typename T>
    static constexpr SIMD256 div(const SIMD256 &a, const SIMD256 &b) noexcept
    {
        return BinOpImp<T, BinOp::Div>::run(a, b);
    }
    template <typename T>
    SIMD256 &divAssign(const SIMD256 &b) noexcept
    {
        return *this = BinOpImp<T, BinOp::Div>::run(*this, b);
    }
    template <typename T>
    static constexpr SIMD256 xor_(const SIMD256 &a, const SIMD256 &b) noexcept
    {
        return BinOpImp<T, BinOp::Xor>::run(a, b);
    }
    template <typename T>
    SIMD256 &xorAssign(const SIMD256 &b) noexcept
    {
        return *this = BinOpImp<T, BinOp::Xor>::run(*this, b);
    }
    template <typename T>
    static constexpr SIMD256 or_(const SIMD256 &a, const SIMD256 &b) noexcept
    {
        return BinOpImp<T, BinOp::Or>::run(a, b);
    }
    template <typename T>
    SIMD256 &orAssign(const SIMD256 &b) noexcept
    {
        return *this = BinOpImp<T, BinOp::Or>::run(*this, b);
    }
    template <typename T>
    static constexpr SIMD256 and_(const SIMD256 &a, const SIMD256 &b) noexcept
    {
        return BinOpImp<T, BinOp::And>::run(a, b);
    }
    template <typename T>
    SIMD256 &andAssign(const SIMD256 &b) noexcept
    {
        return *this = BinOpImp<T, BinOp::And>::run(*this, b);
    }
    template <typename T>
    static constexpr SIMD256 cmpEq(const SIMD256 &a, const SIMD256 &b) noexcept
    {
        return BinOpImp<T, BinOp::CmpEq>::run(a, b);
    }
    template <typename T>
    SIMD256 &cmpEqAssign(const SIMD256 &b) noexcept
    {
        return *this = BinOpImp<T, BinOp::CmpEq>::run(*this, b);
    }
    template <typename T>
    static constexpr SIMD256 cmpNE(const SIMD256 &a, const SIMD256 &b) noexcept
    {
        return BinOpImp<T, BinOp::CmpNE>::run(a, b);
    }
    template <typename T>
    SIMD256 &cmpNEAssign(const SIMD256 &b) noexcept
    {
        return *this = BinOpImp<T, BinOp::CmpNE>::run(*this, b);
    }
    template <typename T>
    static constexpr SIMD256 cmpLT(const SIMD256 &a, const SIMD256 &b) noexcept
    {
        return BinOpImp<T, BinOp::CmpLT>::run(a, b);
    }
    template <typename T>
    SIMD256 &cmpLTAssign(const SIMD256 &b) noexcept
    {
        return *this = BinOpImp<T, BinOp::CmpLT>::run(*this, b);
    }
    template <typename T>
    static constexpr SIMD256 cmpGT(const SIMD256 &a, const SIMD256 &b) noexcept
    {
        return BinOpImp<T, BinOp::CmpGT>::run(a, b);
    }
    template <typename T>
    SIMD256 &cmpGTAssign(const SIMD256 &b) noexcept
    {
        return *this = BinOpImp<T, BinOp::CmpGT>::run(*this, b);
    }
    template <typename T>
    static constexpr SIMD256 cmpLE(const SIMD256 &a, const SIMD256 &b) noexcept
    {
        return BinOpImp<T, BinOp::CmpLE>::run(a, b);
    }
    template <typename T>
    SIMD256 &cmpLEAssign(const SIMD256 &b) noexcept
    {
        return *this = BinOpImp<T, BinOp::CmpLE>::run(*this, b);
    }
    template <typename T>
    static constexpr SIMD256 cmpGE(const SIMD256 &a, const SIMD256 &b) noexcept
    {
        return BinOpImp<T, BinOp::CmpGE>::run(a, b);
    }
    template <typename T>
    SIMD256 &cmpGEAssign(const SIMD256 &b) noexcept
    {
        return *this = BinOpImp<T, BinOp::CmpGE>::run(*this, b);
    }
    template <int i0,
              int i1,
              int i2,
              int i3,
              int i4,
              int i5,
              int i6,
              int i7,
              int i8,
              int i9,
              int i10,
              int i11,
              int i12,
              int i13,
              int i14,
              int i15,
              int i16,
              int i17,
              int i18,
              int i19,
              int i20,
              int i21,
              int i22,
              int i23,
              int i24,
              int i25,
              int i26,
              int i27,
              int i28,
              int i29,
              int i30,
              int i31>
    constexpr SIMD256 swizzle8() const noexcept
    {
        return SIMD256(u8Value[i0],
                       u8Value[i1],
                       u8Value[i2],
                       u8Value[i3],
                       u8Value[i4],
                       u8Value[i5],
                       u8Value[i6],
                       u8Value[i7],
                       u8Value[i8],
                       u8Value[i9],
                       u8Value[i10],
                       u8Value[i11],
                       u8Value[i12],
                       u8Value[i13],
                       u8Value[i14],
                       u8Value[i15],
                       u8Value[i16],
                       u8Value[i17],
                       u8Value[i18],
                       u8Value[i19],
                       u8Value[i20],
                       u8Value[i21],
                       u8Value[i22],
                       u8Value[i23],
                       u8Value[i24],
                       u8Value[i25],
                       u8Value[i26],
                       u8Value[i27],
                       u8Value[i28],
                       u8Value[i29],
                       u8Value[i30],
                       u8Value[i31]);
    }
    template <int i0,
              int i1,
              int i2,
              int i3,
              int i4,
              int i5,
              int i6,
              int i7,
              int i8,
              int i9,
              int i10,
              int i11,
              int i12,
              int i13,
              int i14,
              int i15>
    constexpr SIMD256 swizzle16() const noexcept
    {
        return SIMD256(u16Value[i0],
                       u16Value[i1],
                       u16Value[i2],
                       u16Value[i3],
                       u16Value[i4],
                       u16Value[i5],
                       u16Value[i6],
                       u16Value[i7],
                       u16Value[i8],
                       u16Value[i9],
                       u16Value[i10],
                       u16Value[i11],
                       u16Value[i12],
                       u16Value[i13],
                       u16Value[i14],
                       u16Value[i15]);
    }
    template <int i0, int i1, int i2, int i3, int i4, int i5, int i6, int i7>
    constexpr SIMD256 swizzle32() const noexcept
    {
        return SIMD256(u32Value[i0],
                       u32Value[i1],
                       u32Value[i2],
                       u32Value[i3],
                       u32Value[i4],
                       u32Value[i5],
                       u32Value[i6],
                       u32Value[i7]);
    }
    template <int i0, int i1, int i2, int i3>
    constexpr SIMD256 swizzle64() const noexcept
    {
        return SIMD256(u64Value[i0], u64Value[i1], u64Value[i2], u64Value[i3]);
    }
    template <typename To, typename From, typename S = void>
    struct ConvertHelper final
    {
        static constexpr SIMD256 run(const SIMD256 &v) noexcept = delete;
    };
    template <typename T, typename S = void>
    struct GetValueHelper final
    {
        typedef char valueType;
        static constexpr const valueType &run(const SIMD256 &v) noexcept = delete;
        static constexpr valueType &run(SIMD256 &v) noexcept = delete;
    };
#define programmerjake_voxels_util_SIMD256_GetValueHelper(Type, Var)     \
    template <typename S>                                                \
    struct GetValueHelper<Type, S> final                                 \
    {                                                                    \
        typedef decltype(Var) valueType;                                 \
        static constexpr const valueType &run(const SIMD256 &v) noexcept \
        {                                                                \
            return v.Var;                                                \
        }                                                                \
        static constexpr valueType &run(SIMD256 &v) noexcept             \
        {                                                                \
            return v.Var;                                                \
        }                                                                \
    }
    programmerjake_voxels_util_SIMD256_GetValueHelper(std::uint8_t, u8Value);
    programmerjake_voxels_util_SIMD256_GetValueHelper(std::int8_t, i8Value);
    programmerjake_voxels_util_SIMD256_GetValueHelper(std::uint16_t, u16Value);
    programmerjake_voxels_util_SIMD256_GetValueHelper(std::int16_t, i16Value);
    programmerjake_voxels_util_SIMD256_GetValueHelper(std::uint32_t, u32Value);
    programmerjake_voxels_util_SIMD256_GetValueHelper(std::int32_t, i32Value);
    programmerjake_voxels_util_SIMD256_GetValueHelper(float32, f32Value);
    programmerjake_voxels_util_SIMD256_GetValueHelper(std::uint64_t, u64Value);
    programmerjake_voxels_util_SIMD256_GetValueHelper(std::int64_t, i64Value);
    programmerjake_voxels_util_SIMD256_GetValueHelper(float64, f64Value);
#undef programmerjake_voxels_util_SIMD256_GetValueHelper
    template <typename T>
    constexpr const typename GetValueHelper<T>::valueType &getValue() const noexcept
    {
        return GetValueHelper<T>::run(*this);
    }
    template <typename T>
    typename GetValueHelper<T>::valueType &getValue() noexcept
    {
        return GetValueHelper<T>::run(*this);
    }
    template <typename To,
              typename From,
              typename = typename std::enable_if<sizeof(To) >= sizeof(From)>::type>
    static constexpr auto convert(const SIMD256 &v) noexcept
        -> decltype(ConvertHelper<To, From>::run(v))
    {
        return ConvertHelper<To, From>::run(v);
    }
    template <typename To,
              typename From,
              typename = typename std::enable_if<sizeof(To) * 2 == sizeof(From)>::type>
    static constexpr auto convert(const SIMD256 &v0, const SIMD256 &v1) noexcept
        -> decltype(ConvertHelper<To, From>::run(v0, v1))
    {
        return ConvertHelper<To, From>::run(v0, v1);
    }
    template <typename To,
              typename From,
              typename = typename std::enable_if<sizeof(To) * 4 == sizeof(From)>::type>
    static constexpr auto convert(const SIMD256 &v0,
                                  const SIMD256 &v1,
                                  const SIMD256 &v2,
                                  const SIMD256 &v3) noexcept
        -> decltype(ConvertHelper<To, From>::run(v0, v1, v2, v3))
    {
        return ConvertHelper<To, From>::run(v0, v1, v2, v3);
    }
    template <typename To,
              typename From,
              typename = typename std::enable_if<sizeof(To) * 8 == sizeof(From)>::type>
    static constexpr auto convert(const SIMD256 &v0,
                                  const SIMD256 &v1,
                                  const SIMD256 &v2,
                                  const SIMD256 &v3,
                                  const SIMD256 &v4,
                                  const SIMD256 &v5,
                                  const SIMD256 &v6,
                                  const SIMD256 &v7) noexcept
        -> decltype(ConvertHelper<To, From>::run(v0, v1, v2, v3, v4, v5, v6, v7))
    {
        return ConvertHelper<To, From>::run(v0, v1, v2, v3, v4, v5, v6, v7);
    }
};

#ifdef __GNUC__
#define programmerjake_voxels_util_SIMD256_BINOP4(T, OpCode, Op, v)               \
    template <>                                                                   \
    struct SIMD256::BinOpImp<T, OpCode, true> final                               \
    {                                                                             \
        static constexpr SIMD256 run(const SIMD256 &a, const SIMD256 &b) noexcept \
        {                                                                         \
            return a.v Op b.v;                                                    \
        }                                                                         \
    }

#define programmerjake_voxels_util_SIMD256_BINOP2(OpCode, Op)                          \
    programmerjake_voxels_util_SIMD256_BINOP4(std::int8_t, OpCode, Op, i8Value);       \
    programmerjake_voxels_util_SIMD256_BINOP4(std::uint8_t, OpCode, Op, u8Value);      \
    programmerjake_voxels_util_SIMD256_BINOP4(std::int16_t, OpCode, Op, i16Value);     \
    programmerjake_voxels_util_SIMD256_BINOP4(std::uint16_t, OpCode, Op, u16Value);    \
    programmerjake_voxels_util_SIMD256_BINOP4(std::int32_t, OpCode, Op, i32Value);     \
    programmerjake_voxels_util_SIMD256_BINOP4(std::uint32_t, OpCode, Op, u32Value);    \
    programmerjake_voxels_util_SIMD256_BINOP4(std::int64_t, OpCode, Op, i64Value);     \
    programmerjake_voxels_util_SIMD256_BINOP4(std::uint64_t, OpCode, Op, u64Value);    \
    programmerjake_voxels_util_SIMD256_BINOP4(SIMD256::float32, OpCode, Op, f32Value); \
    programmerjake_voxels_util_SIMD256_BINOP4(SIMD256::float64, OpCode, Op, f64Value)

#define programmerjake_voxels_util_SIMD256_BINOP_BITWISE2(OpCode, Op)               \
    programmerjake_voxels_util_SIMD256_BINOP4(std::int8_t, OpCode, Op, i8Value);    \
    programmerjake_voxels_util_SIMD256_BINOP4(std::uint8_t, OpCode, Op, u8Value);   \
    programmerjake_voxels_util_SIMD256_BINOP4(std::int16_t, OpCode, Op, i16Value);  \
    programmerjake_voxels_util_SIMD256_BINOP4(std::uint16_t, OpCode, Op, u16Value); \
    programmerjake_voxels_util_SIMD256_BINOP4(std::int32_t, OpCode, Op, i32Value);  \
    programmerjake_voxels_util_SIMD256_BINOP4(std::uint32_t, OpCode, Op, u32Value); \
    programmerjake_voxels_util_SIMD256_BINOP4(std::int64_t, OpCode, Op, i64Value);  \
    programmerjake_voxels_util_SIMD256_BINOP4(std::uint64_t, OpCode, Op, u64Value)

programmerjake_voxels_util_SIMD256_BINOP2(SIMD256::BinOp::Add, +);
programmerjake_voxels_util_SIMD256_BINOP2(SIMD256::BinOp::Sub, -);
programmerjake_voxels_util_SIMD256_BINOP2(SIMD256::BinOp::Mul, *);
programmerjake_voxels_util_SIMD256_BINOP2(SIMD256::BinOp::Div, / );
programmerjake_voxels_util_SIMD256_BINOP_BITWISE2(SIMD256::BinOp::And, &);
programmerjake_voxels_util_SIMD256_BINOP_BITWISE2(SIMD256::BinOp::Or, | );
programmerjake_voxels_util_SIMD256_BINOP_BITWISE2(SIMD256::BinOp::Xor, ^);
programmerjake_voxels_util_SIMD256_BINOP2(SIMD256::BinOp::CmpEq, == );
programmerjake_voxels_util_SIMD256_BINOP2(SIMD256::BinOp::CmpNE, != );
programmerjake_voxels_util_SIMD256_BINOP2(SIMD256::BinOp::CmpLT, < );
programmerjake_voxels_util_SIMD256_BINOP2(SIMD256::BinOp::CmpGT, > );
programmerjake_voxels_util_SIMD256_BINOP2(SIMD256::BinOp::CmpLE, <= );
programmerjake_voxels_util_SIMD256_BINOP2(SIMD256::BinOp::CmpGE, >= );

#undef programmerjake_voxels_util_SIMD256_BINOP4
#undef programmerjake_voxels_util_SIMD256_BINOP2
#undef programmerjake_voxels_util_SIMD256_BINOP_BITWISE2

#endif
}
}
}

#include "simd_generated.h"

#endif /* UTIL_SIMD_H_ */
