#include <cstdlib>
#include <new>

void* operator new(size_t size) noexcept { return malloc(size); }

void operator delete(void* p) noexcept { free(p); }

void operator delete(void* p, size_t) noexcept { operator delete(p); }

void* operator new[](size_t size) noexcept
{
    return operator new(size);  // Same as regular new
}

void operator delete[](void* p) noexcept
{
    operator delete(p);  // Same as regular delete
}

void operator delete[](void* p, size_t) noexcept
{
    operator delete(p);  // Same as regular delete
}

void* operator new(size_t size, std::nothrow_t) noexcept
{
    return operator new(size);  // Same as regular new
}

void operator delete(void* p, std::nothrow_t) noexcept
{
    operator delete(p);  // Same as regular delete
}

void* operator new[](size_t size, std::nothrow_t) noexcept
{
    return operator new(size);  // Same as regular new
}

void operator delete[](void* p, std::nothrow_t) noexcept
{
    operator delete(p);  // Same as regular delete
}