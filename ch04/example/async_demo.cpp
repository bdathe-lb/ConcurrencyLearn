#include <cmath>
#include <exception>
#include <future>
#include <iostream>
#include <stdexcept>

// A potentially failing background-intensive computation function
double CalculateSquareRoot(double x) {
  if (x < 0) {
    throw std::out_of_range("Begative value error in worker thread!");
  }
  return std::sqrt(x);
}

int main (int argc, char *argv[]) {
  std::future<double> fut = std::async(CalculateSquareRoot, -4.0);

  std::cout << "Main thread is doing other important work..." << std::endl;

  try {
    double result = fut.get();
    std::cout << "Result: " << result << std::endl;
  } catch (const std::exception& e) {
    std::cout << "Caught exception from async task: "  << e.what() << std::endl;
  }

  return 0;
}
