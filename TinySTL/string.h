#pragma once

#include "memory.h"
#include <initializer_list>

namespace tiny_stl
{

template <typename T>
struct StringConstIterator
{
    using iterator_category = random_access_iterator_tag;
    using value_type        = T;
    using pointer           = const T*;
    using reference         = const T&;
    using difference_type   = ptrdiff_t;
    using Self              = StringConstIterator<T>;

    T* ptr;

    StringConstIterator() = default;
    StringConstIterator(T* p) : ptr(p) {}

    reference operator*() const
    {
        return *ptr;
    }

    pointer operator->() const
    {
        return pointer_traits<pointer>::pointer_to(**this);
    }

    Self& operator++()  // pre
    {
        ++ptr;
        return *this;
    }

    Self operator++(int) // post
    {
        Self tmp = *this;
        ++*this;
        return tmp;
    }

    Self& operator--()
    {
        ++ptr;
        return *this;
    }

    Self operator--(int)
    {
        Self tmp = *this;
        --*this;
        return tmp;
    }

    Self& operator+=(difference_type n)
    {
        ptr += n;
        return *this;
    }

    Self operator+(difference_type n) const
    {
        Self tmp = *this;
        return tmp += n;
    }

    Self& operator-=(difference_type n)
    {
        ptr -= n;
        return *this;
    }

    Self operator-(difference_type n)
    {
        Self tmp = *this;
        return tmp -= n;
    }

    difference_type operator-(const Self& rhs) const
    {
        return this->ptr - rhs.ptr;
    }

    reference operator[](difference_type n) const
    {
        return *(this->ptr + n);
    }

    bool operator==(const Self& rhs) const
    {
        return this->ptr == rhs.ptr;
    }

    bool operator!=(const Self& rhs) const
    {
        return !(*this == rhs);
    }

    bool operator<(const Self& rhs) const
    {
        return this->ptr < rhs.ptr;
    }

    bool operator>(const Self& rhs) const
    {
        return rhs < *this;
    }

    bool operator<=(const Self& rhs) const
    {
        return !(rhs < *this);
    }

    bool operator>=(const Self& rhs) const
    {
        return !(*this < rhs);
    }
}; // StringConstIterator<T>

template <typename T>
struct StringIterator : StringConstIterator<T>
{
    using iterator_category = random_access_iterator_tag;
    using value_type        = T;
    using pointer           = T*;
    using reference         = T&;
    using difference_type   = ptrdiff_t;
    using Base              = StringConstIterator<T>;
    using Self              = StringIterator<T>;

    StringIterator() = default;
    StringIterator(T* p) : Base(p) {}

    reference operator*() const
    {
        return *this->ptr;
    }

    pointer operator->() const
    {
        return pointer_traits<pointer>::pointer_to(**this);
    }

    Self& operator++() // pre
    {
        ++*static_cast<Base*>(this);
        return *this;
    }

    Self operator++(int) // post
    {
        Self tmp = *this;
        ++*this;
        return tmp;
    }

    Self& operator--() // pre
    {
        --*static_cast<Base*>(this);
        return *this;
    }

    Self operator--(int) // post
    {
        Self tmp = *this;
        --*this;
        return tmp;
    }

    Self& operator+=(difference_type n)
    {
        *static_cast<Base*>(this) += n;
        return *this;
    }

    Self& operator-=(difference_type n)
    {
        *static_cast<Base*>(this) -= n;
        return *this;
    }

    Self operator-(difference_type n) const
    {
        Self tmp = *this;
        return tmp -= n;
    }

    difference_type operator-(const Self& rhs) const
    {
        return this->ptr - rhs.ptr;
    }

    reference operator[](difference_type n) const
    {
        return *(this->ptr + n);
    }
}; // StringIterator<T>


template <typename CharT,
    typename Traits = std::char_traits<CharT>, 
    typename Alloc = allocator<CharT>>
class basic_string
{
public:
    static_assert(is_same<typename Traits::char_type, CharT>::value,
        "char type error");

public:
    using traits_type            = Traits;
    using value_type             = CharT;
    using allocator_type         = Alloc;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    using iterator               = StringIterator<value_type>;
    using const_iterator         = StringConstIterator<value_type>;
    using reverse_iterator       = tiny_stl::reverse_iterator<iterator>;
    using const_reverse_iterator = tiny_stl::reverse_iterator<const_iterator>;
    using AllocTraits            = allocator_traits<Alloc>;
public:
    static const size_type npos  = static_cast<size_type>(-1);

private:

    class StringValue
    {
    public:
        static_assert(sizeof(value_type) <= 16, "size of value_type is too large");
        static constexpr const size_type kBufferSize = 16 / sizeof(value_type);
        static constexpr const size_type kBufferMask = sizeof(value_type) <= 1 ? 15 :
            sizeof(value_type) <= 2 ? 7 :
            sizeof(value_type) <= 4 ? 3 :
            sizeof(value_type) <= 8 ? 1 : 0;
    public:
        size_type size;
        size_type capacity;
        
        // short string optimization
        union Data
        {
            Data() : buf() { }
            ~Data() noexcept { }

            value_type buf[kBufferSize];
            pointer ptr;
            char placeholder[kBufferSize]; // unused
        }data;

        StringValue() : size(0), capacity(0), data() { }

        const value_type* getPtr() const 
        {
            const value_type* ptr = data.ptr;
            if (isShortString()) 
            {
                ptr = data.buf;
            }
            return ptr;
        }

        value_type* getPtr()
        {
            return const_cast<value_type*>(
                static_cast<const StringValue*>(this)->getPtr());
        }

        void checkIndex(const size_type idx) const 
        {
            if (idx > size)
            {
                xRange();
            }
        }

        bool isShortString() const
        {
            return capacity < kBufferSize;
        }

        [[noreturn]] static void xRange()
        {
            throw "invalid tiny_stl::basic_string<CharT> index";
        }
    };

private:
    extra::compress_pair<Alloc, StringValue> allocVal;

public:
    explicit basic_string(const Alloc& a)
        : allocVal(a)
    {
        initEmpty();
    }

    basic_string() noexcept(noexcept(Alloc()))
        : basic_string(Alloc()) 
    { 
    }

    basic_string(size_type count, value_type ch, const Alloc& a = Alloc())
        : allocVal(a)
    {
        init(count, ch);
    }

    basic_string(const basic_string& rhs, size_type pos,
        const Alloc& a = Alloc())
        : basic_string(rhs, pos, npos, a)
    {
    }

    basic_string(const basic_string& rhs, size_type pos,
        size_type count, const Alloc& a = Alloc())
        : allocVal(a)
    {
        initEmpty(); // for setting capacity
        init(rhs, pos, count);
    }

    basic_string(const value_type* str, size_type count, const Alloc& a)
        : allocVal(a)
    {
        initEmpty(); // for setting capacity
        init(str, count);
    }

    basic_string(const value_type* str, const Alloc& a = Alloc())
        : allocVal(a)
    {
        initEmpty(); // for setting capacity
        init(str);
    }

    template<typename InIter,
        typename = enable_if_t<is_iterator<Initer>::value>>
    basic_string(InIter first, InIter last, const Alloc& a = Alloc())
        : allocVal(a)
    {
        initEmpty(); // for setting capacity
        constructRange(first, last,
            typename iterator_traits<InIter>::iterator_category{});
    }

    ~basic_string()
    {
        tidy();
    }


public:
    allocator_type get_allocator() const noexcept
    {
        return static_cast<allocator_type>(getAlloc());
    }

private:
    void initEmpty() noexcept
    {
        getVal().size = 0;
        getVal().capacity = StringValue::kBufferSize - 1;
        Traits::assign(getVal().data.buf[0], value_type());
    }

    basic_string& init(size_type count, value_type ch)
    {
        if (count <= getVal().capacity)
        {
            value_type* const ptr = getVal().getPtr();
            getVal().size = count;
            Traits::assign(ptr, count, ch);
            Traits::assign(ptr[count], value_type());

            return *this;
        }

        // allocate and assign
        return reallocAndAssign(count,
            [](value_type* const dst, size_type count, const value_type* const src)
            {
                Traits::assign(dst, count, ch);
                Traits::assign(dst[count], value_type());
            },
            ch);
    }

    basic_string& init(const basic_string& rhs, size_type pos, size_type count = npos)
    {
        rhs.getVal().checkIndex(pos);
        count = tiny_stl::min(count, rhs.getVal().size - pos);
        return init(rhs.getVal().getPtr(), count);
    }

    basic_string& init(const value_type* str, size_type count)
    {
        if (count <= getVal().capacity())
        {
            value_type* const ptr = getVal().getPtr();
            getVal().size = count;
            Traits::move(ptr, str, count);
            Traits::assign(ptr[count], value_type());
            return *this;
        }

        // allocate and assign
        return reallocAndAssign(count,
            [](value_type* const dst, size_type count, const value_type* src)
            {
                Traits::move(dst, src, count);
                Traits::move(dst[count], value_type());
            },
            str);
    }

    basic_string& init(const value_type* str)
    {
        return init(str, Traits::length(str));
    }

    template <typename Iter>
    void constructRange(Iter first, Iter last, input_iterator_tag)
    {
        TidyRAII<basic_string> guard{ this };

        for (; first != last; ++first)
        {
            push_back(*first);
        }

        guard.obj = nullptr;
    }

    template <typename Iter>
    void constructRange(Iter first, Iter last, forward_iterator_tag)
    {
        const size_type count = static_cast<size_type>(
            tiny_stl::distance(first, last));
        reserve(count);
        constructRange(first, last, input_iterator_tag);
    }

    void constructRange(const value_type* const first, const value_type* const last,
        random_access_iterator_tag)
    {
        if (first == last)
        {
            return;
        }

        init(first, static_cast<size_type>(last - first));
    }

    void constructRange(value_type* const first, value_type* const last,
        random_access_iterator_tag)
    {
        if (first == last)
        {
            return;
        }

        init(first, static_cast<size_type>(last - first));
    }

    template <typename F, typename... Args>
    basic_string& reallocAndAssign(size_type newSize, F func, Args... args)
    {
        checkLength(newSize);
        Alloc& alloc = getAlloc();
        const size_type oldCapacity = getVal().capacity;
        const size_type newCapacity = capacityGrowth(newSize);

        pointer newPtr = alloc.allocate(newCapacity + 1); // for null character
        getVal().size = newSize;
        getVal().capacity = newCapacity;
        func(newPtr, newSize, args...);

        if (oldCapacity >= StringValue::kBufferSize)
        {
            alloc.deallocate(getVal().data.ptr, oldCapacity + 1);
        }
        getVal().data.ptr = newPtr;

        return *this;
    }


    Alloc& getAlloc() noexcept
    {
        return allocVal.get_first();
    }

    const Alloc& getAlloc() const noexcept
    {
        return allocVal.get_first();
    }

    StringValue& getVal() noexcept
    {
        return allocVal.get_second();
    }

    const StringValue& getVal() const noexcept
    {
        return allocVal.get_second();
    }

    void tidy() noexcept
    {
        if (!getVal().isShortString())
        {
            Alloc& alloc = getAlloc();
            const pointer ptr = getVal().getPtr();
            alloc.deallocate(ptr, getVal().capacity + 1);
        }
        initEmpty();
    }

public:
    bool empty() const noexcept
    {
        return size() == 0;
    }

    size_type size() const noexcept
    {
        return allocVal.get_second().size;
    }

    size_type length() const noexcept
    {
        return size();
    }

    size_type max_size() const noexcept
    {
        return tiny_stl::min(static_cast<size_type>(-1) / sizeof(value_type) - 1,
            std::numeric_limits<difference_type>::max());
    }

    void reserve(size_type newCapacity = 0)
    {
        if (newCapacity < getVal().size)
        {
            shrink_to_fit();
            return;
        }

        if (newCapacity <= getVal().capacity)
        {
            return; // do nothing
        }

        // reallocate memory if newCapacity > oldCapacity
        const size_type oldSize = getVal().size;
        auto& value = getVal();
        
    }

    size_type capacity() const noexcept
    {
        return allocVal.get_second().capacity;
    }

    void shrink_to_fit()
    {
        // do nothing
    }

private:
    void checkLength(size_type newSize)
    {
        if (newSize >= max_size())
        {
            xLength();
        }

    }

    size_type capacityGrowth(size_type newSize)
    {
        const size_type oldSize = allocVal.get_second().size;
        const size_type masked = newSize | StringValue::kBufferMask;
        const size_type maxSize = max_size();
        if (masked > maxSize)
        {
            return maxSize;
        }

        if (oldSize > maxSize - oldSize / 2) // for avoiding overflow
        {
            return maxSize;
        }

        return tiny_stl::max(masked, oldSize + oldSize / 2);
    }

private:
    [[noreturn]] static void xLength()
    {
        throw "tiny_stl::basic_string<CharT> too long";
    }

    [[noreturn]] static void xRange()
    {
        throw "invalid tiny_stl::basic_string<CharT> index";
    }
};

using string     = basic_string<char>;
using wstring    = basic_string<wchar_t>;
using u16string  = basic_string<char16_t>;
using u32string  = basic_string<char32_t>;

} // namespace tiny_stl