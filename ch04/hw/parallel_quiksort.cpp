#include <algorithm>
#include <future>
#include <iostream>
#include <list>

template<typename T>
std::list<T> ParallelQuikSort(std::list<T> input) {
  // Baseline case: empty linked list
  if (input.empty())
    return input;

  std::list<T> result;
  // Cut the first element of the input list to result as the pivot value (Pivot)
  result.splice(result.begin(), input, input.begin());
  const T& pivot = *result.begin();

  // Use std::partition to divide the elements in input according to pivot  
  // Hint: std::partition takes iterators and a lambda predicate, returning the iterator pointing to the partition point
  auto divide_point = std::partition(input.begin(), input.end(), 
                                     [&](const T& t) { return t < pivot; });

  // Move elements smaller than pivot to the lower_part.
  std::list<T> lower_part;
  lower_part.splice(lower_part.end(), input, input.begin(), divide_point);

  // Asynchronously sort the lower_part
  std::future<std::list<T>> new_lower = std::async(&ParallelQuikSort<T>, std::move(lower_part));  // Q1: Why is <T> used here?
                                                                                                  // A1: Template functions fail to perform automatic type deduction when passed as parameters;  
                                                                                                  // ParallelQuikSort(input) allows the compiler to deduce the type T from the input parameter type (std::list<T>), 
                                                                                                  // enabling type inference.
  std::list<T> new_higher = ParallelQuikSort(input); // Q2: Why can input be passed directly here? What form does input take at this point?
  // A2:
  /*
  input (Size: 9)
  head 
   │
   ▼
 ┌───┐    ┌───┐    ┌───┐    ┌───┐    ┌───┐    ┌───┐    ┌───┐
 │ 3 │◄══►│ 1 │◄══►│ 4 │◄══►│ 2 │◄══►│ 7 │◄══►│ 8 │◄══►│...│
 └───┘    └───┘    └───┘    └───┘    └───┘    └───┘    └───┘
                                       ▲
                                       │
                                 divide_point    

  After executing splice: the pointer is disconnected and then reconnected.

  lower_part (Received the broken first half)
  head
   │
   ▼
 ┌───┐    ┌───┐    ┌───┐    ┌───┐
 │ 3 │◄══►│ 1 │◄══►│ 4 │◄══►│ 2 │
 └───┘    └───┘    └───┘    └───┘

-----------------------------------------------------------

  input (Only the second half remains, Size automatically shrinks to 5, with no placeholders)
  head 
   │
   ▼
 ┌───┐    ┌───┐    ┌───┐
 │ 7 │◄══►│ 8 │◄══►│...│
 └───┘    └───┘    └───┘

  */
                                                  
  result.splice(result.end(), new_higher);

  result.splice(result.begin(), new_lower.get());

  return result;
}

int main (int argc, char *argv[]) {
  std::list<int> data = {5, 3, 1, 7, 4, 2, 8, 10, 6, 9};

  std::cout << "Original data: ";
  for (int x : data) std::cout << x << " ";
  std::cout << "\n";

  auto sorted_data = ParallelQuikSort(std::move(data));

  std::cout << "Sorted data:   ";
  for (int x : sorted_data) std::cout << x << " ";
  std::cout << "\n";
  
  return 0;
}
