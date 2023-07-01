# Code Style

# Arch

## How to use pointer (screenshot from [Leak-Freedom in C++... By Default](https://www.youtube.com/watch?v=JfmTagWcqoE))


### Goal:

- No dangling/invalid deref(use after delete)
- No null ref
- No leaks(delete objects when no longer used, and delete only once)

### Strategy:

![Leak_Freedom_Strategy](Images/Leak_Freedom_Strategy.png)

### Summary:

- owning + share between objects, use make_shared
- owning + not share, scoped, stack-like, use make_unique
- circular reference, use make_weak, (but this should be avoided when design)
- no ownership, use raw ptr

PS: we use a simple alias of raw pointer to diff nonnull and nullable:
```c++
template <typename T>
using nullable = T*;

float* pSomething1; // NOT allowed to be nullptr
nullable<float> pSomething2; // Allowed to be nullptr, in other word, we need
                             // to write some codes like `if (pSomething2 == nullptr) {...}`


                             
```

## Naming Convention

### File Extensions

The project uses the following file extensions:
- .cpp, c++ source file
- .hpp, c++ public header file
- .hxx, c++ private header file, only be include by the file in the same module or layer
- .inl, code snippet, only be included by some specified .cpp
  
NOTE:
- All other extensions(.h, .c, ...) come from third-party libraries,
- All included header from third-party libraries, will be surround by `<>`,
- All included header of the project, will be surround by `""`,


### Comments
There are 4 types of annotations here, for example:
```c++
[[deprecated("this function is not supported yet")]]               // <--- Type 1, explanation of deprecated functions
void foo()
{
    // TODO: remove this function in next version
}

#if AXE_(false, "the following code haven't be done yet")          // <--- Type 2, explanation of active/inactive code blocks
    // some inactive code
#endif

// It's a common add function                                      // <--- Type 3, explanation of anything
void add(int a, int b)
{
    return a + b;
}

// @DISCUSSION@  tech-A v.s. tech-B                                // <--- Type 4, record some tech discussion for learning purpose
// @CONVENTION@ we should define macro using all-upper-snake style // <--- Type 4, record some coding convection for make readers happier
                                                                   // you can search @CONVENTION@ in root directory globally to find all convection
```

### C++ Code

```c++
#define AXE_TEST 1                     // MACRO, all upper-cases and with prefix AXE_
constexpr auto APP_NAME = "Hello";     // constant evaluated at compile-time, all upper-cases

class FruitVendor                      // Type, upper-case camel
{
private:
    int _mBestIndex;                   // `_` refers to private, `m` refers to member
    int _m_bestIndex                   // is also OK

    const int BEST_INDEX = 1;          // for constant, all upper-cases is enough

    static  int _s_currentIndex = 1;   // `s` refers to static
    static  int _sCurrentIndex = 1;    // is also OK
    static  int _msCurrentIndex = 1;   // is also OK

    int* _mpBestIndex;                 // `p` refers to pointer
    int* _mp_bestIndex;                // is also OK

public:
    int mIndex;
    int m_index;

private:
        int _getValue();               // member function uses lower-case camel
        static int _getDate();
    public:
        int getValue();
        static int getDate();
private:

    char _mChar;
    char _m_char;

    struct Pair { int x, y; };
    Pair _mPair;
    Pair m_pair;

    Parent* _mpParent;                // raw pointer refer to a reference and no ownership, and it should never be nullptr, 
                                      // which implies we should write AXE_ASSERT(_mpParent != nullptr)
    Parent* _mp_parent;               // is also OK

    nullable<Parent> _mpParent        // a alias of raw pointer and no ownership, it's allowed to be nullptr, 
                                      // which implies we should write some code like if(_mpParent == nullptr)
    nullable<const Parent> _mp_Parent // is also OK

    
    std::unique_ptr<Image> _mpImage;  // resource handle(has ownership)
    std::unique_ptr<Image> _mp_image; // is also OK
    
    typedef Allocator_T* Allocator;
    Allocator _mpAllocatorHandle;     // raw pointer from external library
                                      // refer to resource handle(has ownership)
    Allocator _mp_allocatorHandle;    // is also OK

    std::vector<u32> _mIndices;       // is OK
    std::vector<u32> _m_indices;      // is also OK
};

static int gs_count;        // `g` refer to global
static int gsCount;         // is also OK

const int MAX_NUM = 10;     // for constant, all upper-cases is enough

int gArray[MAX_NUM];        // `g` refer to global
int g_array[MAX_NUM];       // is also OK

void foo_func() {}          // common function uses lower-snake naming
static void foo_func() {}   // OK
static void _foo_func() {}  // OK

// struct should be simple as soon as possible, without complex encapsulation or functionality and resource ownership
struct Apple {
    int vendorId;           // not mVendorId since struct just a aggregation of variables
    const char* pName;
};

enum class FruitFlag {          // used for multi-element, maybe one or more flags
    APPLE  = 0,
    ORANGE = 1 << 0,
    PEAR   = 1 << 1,
    CHERRY = 1 << 2,
};
using FruitFlagOneBit = FruitFlag; // alias without any side-effects,
                                   // to indicate that the current variable will be used as single bit

FruitFlag fruits = FruitFlag::APPLE | FruitFlag::ORANGE;      // OK, fruits will be used as flags
FruitFlagOneBit fruit = FruitFlag::ORANGE                     // OK, indicate fruit will be used as single bit
```
