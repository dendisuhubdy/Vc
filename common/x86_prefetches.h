/*  This file is part of the Vc library. {{{
Copyright © 2013-2014 Matthias Kretz <kretz@kde.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the names of contributing organizations nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

}}}*/

#ifndef VC_COMMON_X86_PREFETCHES_H
#define VC_COMMON_X86_PREFETCHES_H

#include <xmmintrin.h>
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Common
{

#if !defined(VC_IMPL_MIC) && !defined(_MM_HINT_ENTA)
#define VC__NO_SUPPORT_FOR_EXCLUSIVE_HINT 1
#define _MM_HINT_ENTA _MM_HINT_NTA
#define _MM_HINT_ET0 _MM_HINT_T0
#define _MM_HINT_ET1 _MM_HINT_T1
#define _MM_HINT_ET2 _MM_HINT_T2
#endif

// TODO: support AMD's prefetchw with correct flags and checks via cpuid

template<typename ExclusiveOrShared = Vc::Shared>
Vc_INTRINSIC void prefetchForOneRead(const void *addr)
{
    if (std::is_same<ExclusiveOrShared, Vc::Shared>::value) {
        _mm_prefetch(static_cast<char *>(const_cast<void *>(addr)), _MM_HINT_NTA);
    } else {
        _mm_prefetch(static_cast<char *>(const_cast<void *>(addr)), _MM_HINT_ENTA);
    }
}
template<typename ExclusiveOrShared = Vc::Shared>
Vc_INTRINSIC void prefetchClose(const void *addr)
{
    if (std::is_same<ExclusiveOrShared, Vc::Shared>::value) {
        _mm_prefetch(static_cast<char *>(const_cast<void *>(addr)), _MM_HINT_T0);
    } else {
        _mm_prefetch(static_cast<char *>(const_cast<void *>(addr)), _MM_HINT_ET0);
    }
}
template<typename ExclusiveOrShared = Vc::Shared>
Vc_INTRINSIC void prefetchMid(const void *addr)
{
    if (std::is_same<ExclusiveOrShared, Vc::Shared>::value) {
        _mm_prefetch(static_cast<char *>(const_cast<void *>(addr)), _MM_HINT_T1);
    } else {
        _mm_prefetch(static_cast<char *>(const_cast<void *>(addr)), _MM_HINT_ET1);
    }
}
template<typename ExclusiveOrShared = Vc::Shared>
Vc_INTRINSIC void prefetchFar(const void *addr)
{
    if (std::is_same<ExclusiveOrShared, Vc::Shared>::value) {
        _mm_prefetch(static_cast<char *>(const_cast<void *>(addr)), _MM_HINT_T2);
    } else {
        _mm_prefetch(static_cast<char *>(const_cast<void *>(addr)), _MM_HINT_ET2);
    }
}

#ifdef VC__NO_SUPPORT_FOR_EXCLUSIVE_HINT
#undef VC__NO_SUPPORT_FOR_EXCLUSIVE_HINT
#undef _MM_HINT_ENTA
#undef _MM_HINT_ET0
#undef _MM_HINT_ET1
#undef _MM_HINT_ET2
#endif

/*handlePrefetch/handleLoadPrefetches/handleStorePrefetches{{{*/
namespace
{
template<size_t L1, size_t L2, bool UseExclusivePrefetch> Vc_INTRINSIC void handlePrefetch(const void *addr_, typename std::enable_if<L1 != 0 && L2 != 0, void *>::type = nullptr)
{
    const char *addr = static_cast<const char *>(addr_);
    prefetchClose<typename std::conditional<UseExclusivePrefetch, Vc::Exclusive, Vc::Shared>::type>(addr + L1);
    prefetchMid  <typename std::conditional<UseExclusivePrefetch, Vc::Exclusive, Vc::Shared>::type>(addr + L2);
}
template<size_t L1, size_t L2, bool UseExclusivePrefetch> Vc_INTRINSIC void handlePrefetch(const void *addr_, typename std::enable_if<L1 == 0 && L2 != 0, void *>::type = nullptr)
{
    const char *addr = static_cast<const char *>(addr_);
    prefetchMid  <typename std::conditional<UseExclusivePrefetch, Vc::Exclusive, Vc::Shared>::type>(addr + L2);
}
template<size_t L1, size_t L2, bool UseExclusivePrefetch> Vc_INTRINSIC void handlePrefetch(const void *addr_, typename std::enable_if<L1 != 0 && L2 == 0, void *>::type = nullptr)
{
    const char *addr = static_cast<const char *>(addr_);
    prefetchClose<typename std::conditional<UseExclusivePrefetch, Vc::Exclusive, Vc::Shared>::type>(addr + L1);
}
template<size_t L1, size_t L2, bool UseExclusivePrefetch> Vc_INTRINSIC void handlePrefetch(const void *, typename std::enable_if<L1 == 0 && L2 == 0, void *>::type = nullptr)
{
}

template<typename Flags> Vc_INTRINSIC void handleLoadPrefetches(const void *    , Flags, typename Flags::EnableIfNotPrefetch = nullptr) {}
template<typename Flags> Vc_INTRINSIC void handleLoadPrefetches(const void *addr, Flags, typename Flags::EnableIfPrefetch    = nullptr)
{
    // load prefetches default to Shared unless Exclusive was explicitely selected
    handlePrefetch<Flags::L1Stride, Flags::L2Stride, Flags::IsExclusivePrefetch>(addr);
}

template<typename Flags> Vc_INTRINSIC void handleStorePrefetches(const void *    , Flags, typename Flags::EnableIfNotPrefetch = nullptr) {}
template<typename Flags> Vc_INTRINSIC void handleStorePrefetches(const void *addr, Flags, typename Flags::EnableIfPrefetch    = nullptr)
{
    // store prefetches default to Exclusive unless Shared was explicitely selected
    handlePrefetch<Flags::L1Stride, Flags::L2Stride, !Flags::IsSharedPrefetch>(addr);
}

} // anonymous namespace
/*}}}*/

}  // namespace Common

using Common::prefetchForOneRead;
using Common::prefetchClose;
using Common::prefetchMid;
using Common::prefetchFar;
}  // namespace Vc

#include "undomacros.h"

#endif // VC_COMMON_X86_PREFETCHES_H

// vim: foldmethod=marker