#include <iostream>
#include <deque>
#include <memory>

template <typename T>
struct MyAllocator {
    typedef T value_type;

    MyAllocator() noexcept {}

    template <class U>
    MyAllocator(const MyAllocator<U>&) noexcept {}

    T* allocate(std::size_t n) {
     std::cout << "Allocating " << n << " elements of type: " << typeid(T).name() << std::endl;
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t n) {
     std::cout << "Deallocating " << n << " elements of type: " << typeid(T).name() << std::endl;
        ::operator delete(p, n * sizeof(T));
    }
};

int main() {
  std::deque<int, MyAllocator<int>> myDeque;
    int noElements = 128 * 8;
    for (int i = 0; i < noElements; i++) {
        myDeque.push_back(i);
    }

    std::cout << "finish inserting"<< std::endl;

    // std::deque<int> dq = {10, 20, 30};

    // auto it = dq.begin();
    // std::cout << "Original first element: " << *it << std::endl;

    // dq.push_front(5); 

    // // 'it' is now INVALIDATED according to the C++ standard.
    // // Accessing it may lead to undefined behavior, even if it "seems" to work.
    // // std::cout << *it << std::endl; // DANGEROUS!

    // // 3. To safely continue, you must re-obtain the iterator
    // it = dq.begin(); 
    // std::cout << "New first element: " << *it << std::endl;
  return 0;
}