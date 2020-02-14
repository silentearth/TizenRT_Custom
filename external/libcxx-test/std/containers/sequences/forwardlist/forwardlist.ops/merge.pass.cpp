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

// static int merge(forward_list&& x);

#include <forward_list>
#include <iterator>
#include <cassert>
#include "test_macros.h"
#include "libcxx_tc_common.h"


int tc_libcxx_containers_forwardlist_ops_merge(void)
{
    {
        typedef int T;
        typedef std::forward_list<T> C;
        const T t1[] = {3, 5, 6, 7, 12, 13};
        const T t2[] = {0, 1, 2, 4, 8, 9, 10, 11, 14, 15};
        const T t3[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        C c1(std::begin(t1), std::end(t1));
        C c2(std::begin(t2), std::end(t2));
        c1.merge(c2);
        C c3(std::begin(t3), std::end(t3));
        TC_ASSERT_EXPR(c1 == c3);
    }
    TC_SUCCESS_RESULT();
    return 0;
}
