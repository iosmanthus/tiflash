// Copyright 2022 PingCAP, Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <common/StringRef.h>
#include <common/defines.h>
#include <common/mem_utils_opt.h>

#include <cstring>
#include <memory>

namespace DB
{

template <typename T>
ALWAYS_INLINE inline int signum(T val)
{
    return (0 < val) - (val < 0);
}

// Check equality is much faster than other comparison.
// - check size first
// - return 0 if equal else 1
FLATTEN_INLINE_PURE static inline int RawStrEqualCompare(const std::string_view & lhs, const std::string_view & rhs)
{
    return mem_utils::IsStrViewEqual(lhs, rhs) ? 0 : 1;
}

// Compare str view by memcmp
FLATTEN_INLINE_PURE inline int RawStrCompare(const std::string_view & v1, const std::string_view & v2)
{
    return v1.compare(v2);
}

constexpr char SPACE = ' ';

FLATTEN_INLINE_PURE inline std::string_view RightTrimRaw(const std::string_view & v)
{
    size_t end = v.find_last_not_of(SPACE);
    return end == std::string_view::npos ? std::string_view{} : std::string_view(v.data(), end + 1);
}

// Remove tail space
FLATTEN_INLINE_PURE inline std::string_view RightTrim(const std::string_view & v)
{
    if (likely(v.empty() || v.back() != SPACE))
        return v;
    return RightTrimRaw(v);
}

FLATTEN_INLINE_PURE inline std::string_view RightTrimNoEmpty(const std::string_view & v)
{
    if (likely(v.back() != SPACE))
        return v;
    return RightTrimRaw(v);
}

FLATTEN_INLINE_PURE inline int RtrimStrCompare(const std::string_view & va, const std::string_view & vb)
{
    return RawStrCompare(RightTrim(va), RightTrim(vb));
}

template <bool padding>
FLATTEN_INLINE_PURE inline int BinCollatorCompare(const char * s1, size_t length1, const char * s2, size_t length2)
{
    if constexpr (padding)
        return DB::RtrimStrCompare({s1, length1}, {s2, length2});
    else
        return DB::RawStrCompare({s1, length1}, {s2, length2});
}

template <bool padding>
FLATTEN_INLINE_PURE inline StringRef BinCollatorSortKey(const char * s, size_t length)
{
    if constexpr (padding)
    {
        return StringRef(RightTrim({s, length}));
    }
    else
    {
        return StringRef(s, length);
    }
}

// Loop columns and invoke callback for each pair.
// Remove last zero byte.
template <typename Chars, typename Offsets, typename F>
FLATTEN_INLINE static inline void LoopTwoColumns(
    const Chars & a_data,
    const Offsets & a_offsets,
    const Chars & b_data,
    const Offsets & b_offsets,
    size_t size,
    F && func)
{
    uint64_t a_prev_offset = 0;
    uint64_t b_prev_offset = 0;

    for (size_t i = 0; i < size; ++i)
    {
        auto a_size = a_offsets[i] - a_prev_offset;
        auto b_size = b_offsets[i] - b_prev_offset;

        // Remove last zero byte.
        func({reinterpret_cast<const char *>(&a_data[a_prev_offset]), a_size - 1},
             {reinterpret_cast<const char *>(&b_data[b_prev_offset]), b_size - 1},
             i);

        a_prev_offset = a_offsets[i];
        b_prev_offset = b_offsets[i];
    }
}

// Loop one column and invoke callback for each pair.
// Remove last zero byte.
template <typename Chars, typename Offsets, typename F>
FLATTEN_INLINE static inline void LoopOneColumn(
    const Chars & a_data,
    const Offsets & a_offsets,
    size_t size,
    F && func)
{
    uint64_t a_prev_offset = 0;

    for (size_t i = 0; i < size; ++i)
    {
        auto a_size = a_offsets[i] - a_prev_offset;

        // Remove last zero byte.
        func({reinterpret_cast<const char *>(&a_data[a_prev_offset]), a_size - 1}, i);
        a_prev_offset = a_offsets[i];
    }
}

} // namespace DB
