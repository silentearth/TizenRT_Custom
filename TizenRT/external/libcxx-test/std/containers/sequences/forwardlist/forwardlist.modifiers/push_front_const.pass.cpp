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

// <forward_list>

// static int push_front(const value_type& v);

#include <forward_list>
#include <cassert>
#include "test_macros.h"
#include "libcxx_tc_common.h"


int tc_libcxx_containers_forwardlist_modifiers_push_front_const(void)
{
    {
        typedef int T;
        typedef std::forward_list<T> C;
        C c;
        c.push_front(1);
        TC_ASSERT_EXPR(c.front() == 1);
        TC_ASSERT_EXPR(distance(c.begin(), c.end()) == 1);
        c.push_front(3);
        TC_ASSERT_EXPR(c.front() == 3);
        TC_ASSERT_EXPR(*next(c.begin()) == 1);
        TC_ASSERT_EXPR(distance(c.begin(), c.end()) == 2);
    }
    TC_SUCCESS_RESULT();
    return 0;
}
