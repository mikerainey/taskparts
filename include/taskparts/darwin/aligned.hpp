#pragma once

#include <type_traits>
#include <memory>
#include <assert.h>
#include <cstdlib>

namespace taskparts {
  
template <size_t cache_align_szb=TASKPARTS_CACHE_LINE_SZB>
auto aligned_alloc(size_t sizeb) -> void* {
  //  return std::aligned_alloc(cache_align_szb, cache_align_szb * sizeb);
  // later: implement properly using posix_memalign()
  return malloc(sizeb);
}

} // end namespace
