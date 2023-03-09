# Code Style

# Arch

## How to use pointer (screenshot from [Leak-Freedom in C++... By Default](https://www.youtube.com/watch?v=JfmTagWcqoE))


### Goal:

- No dangling/invalid deref(use after delete)
- No null ref
- No leaks(delete objects when no longer used, and delete only once)

### Strategy:

![Leak_Freedom_Strategy](Images\Leak_Freedom_Strategy.png)

### Summary:

- owning + share between objects, use make_shared
- owning + not share, scoped, stack-like, use make_unique
- circular reference, use make_weak, (but this should be avoided when design)
- no ownership, use raw ptr

### For more clear, Axe will

- use `unique_ptr` if it has ownership, otherwise use raw ptr, to avoid memory leaks (Goal 3)
- use assertion to check unacceptable nullptr in Debug mode and if-else for acceptable nullptr, to avoid null ref (Goal 2)
- use `owner_ptr`&`observer_ptr` (just a warper of `shared_ptr`&`weak_ptr`, which is not allowed to be used), to avoid dangling pinter (Goal 1)

## Naming Convention

```c++
#define AXE_TEST 1                  // MACRO, all upper-cases and with prefix AXE_
constexpr auto APP_NAME = "Hello"; // constant evaluated at compile-time, all upper-cases

class FruitVendor                         // Type, upper-case camel
{
private:
    int _mBestIndex;                // `_` refers to private
    const int BEST_INDEX = 1; 
    static int _msBestIndex;        // `m` refers to member
    static const  int BEST_INDEX = 1; // `s` refers to static
    int* _mpBestIndex;              // `p` refers to pointer

public:
    int mIndex;

private:
        int _getValue();        // (static) member function uses lower-case camel
        static int _getDate();
    public:
        int getValue();
        static int getDate();
private:
    // constant
    const int MAX_NUM = 16;

    // scalar
    char _mChar;
    struct Pair { int x, y; };
    Pair _mPair;

    // ref
    Parent* _mpParent;

    // resource handle(has ownership)
    std::unique_ptr<Image> _mpImage;
    typedef Allocator_T* Allocator;
    Allocator _mpAllocatorHandle;

    // resource will be release automatically
    std::vector<u32> _mIndices;
};

static int gsCount;             // `g` refer to global
const int MAX_NUM = 10;
int gArray[MAX_NUM];

void eval_sum(int* pData) {     // common function uses lower-snake naming
    int x;
    static int sY;
    const float PI = 3.14f;
    int* pPos = nullptr;  
}

```

