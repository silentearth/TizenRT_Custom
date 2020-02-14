/****************************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <map>

// map& operator=(map&& c)
//     noexcept(
//          allocator_type::propagate_on_container_move_assignment::value &&
//          is_nothrow_move_assignable<allocator_type>::value &&
//          is_nothrow_move_assignable<key_compare>::value);

// This tests a conforming extension

// UNSUPPORTED: c++98, c++03

#include <map>
#include <cassert>
#include "libcxx_tc_common.h"

#include "test_macros.h"
#include "MoveOnly.h"
#include "test_allocator.h"

template <class T>
struct some_comp
{
    typedef T value_type;
    some_comp& operator=(const some_comp&);
    bool operator()(const T&, const T&) const { return false; }
};

int tc_libcxx_containers_map_cons_move_assign_noexcept(void)
{
    typedef std::pair<const MoveOnly, MoveOnly> V;
    {
        typedef std::map<MoveOnly, MoveOnly> C;
        static_assert(std::is_nothrow_move_assignable<C>::value, "");
    }
    {
        typedef std::map<MoveOnly, MoveOnly, std::less<MoveOnly>, test_allocator<V>> C;
        static_assert(!std::is_nothrow_move_assignable<C>::value, "");
    }
#if defined(_LIBCPP_VERSION)
    {
        typedef std::map<MoveOnly, MoveOnly, std::less<MoveOnly>, other_allocator<V>> C;
        static_assert(std::is_nothrow_move_assignable<C>::value, "");
    }
#endif // _LIBCPP_VERSION
    {
        typedef std::map<MoveOnly, MoveOnly, some_comp<MoveOnly>> C;
        static_assert(!std::is_nothrow_move_assignable<C>::value, "");
    }
    TC_SUCCESS_RESULT();
    return 0;
}
