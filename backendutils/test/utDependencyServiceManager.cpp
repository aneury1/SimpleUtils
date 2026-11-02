#include <gtest/gtest.h>

#include "simpleutils/DependencyServiceManager.hpp"

// Polymorphic base class used for dynamic_cast testing
struct Base {
  virtual ~Base() = default;
};

// Derived class that inherits from Base
struct Derived : public Base {};

// Helper to clear the static services vector for a given type.
// This is needed because the tests rely on the static member persisting
// across test cases.
template <typename T>
void clearServices() {
  // The static member is a template static member of DependencyServiceManager
  DependencyServiceManager::services<T>.clear();
}

// ---------------------------------------------------------------------
// Test cases for DependencyServiceManager
// ---------------------------------------------------------------------

TEST(DependencyServiceManagerTest, RegisterAndGetService_returnsRegisteredPointer) {
  Derived* d = new Derived();
  // Register a Derived service using the actual type (not pointer type)
  DependencyServiceManager::registerService<Derived>(d);

  // Retrieve it; dynamic_cast should succeed and return the same pointer
  Derived* retrieved = DependencyServiceManager::get<Derived>();
  EXPECT_NE(retrieved, nullptr);
  EXPECT_EQ(retrieved, d);

  // Cleanup
  delete d;
  // Clear for next tests
  DependencyServiceManager::services<Derived>.clear();
}

TEST(DependencyServiceManagerTest, GetReturnsNullptr_whenNoServiceRegistered) {
  // Ensure the vector for Derived is empty before the test
  DependencyServiceManager::services<Derived>.clear();

  // Requesting a service of type Derived should return nullptr
  Derived* retrieved = DependencyServiceManager::get<Derived>();
  EXPECT_EQ(retrieved, nullptr);
}

TEST(DependencyServiceManagerTest,
     GetReturnsFirstMatchingDerived_whenMultipleBaseServicesRegistered) {
  // Register a Derived object as a Base*
  Derived* d = new Derived();
  Base* b1 = d;  // up-cast, static type Base*, dynamic type Derived*

  // Register a plain Base object
  Base* b2 = new Base();

  // Both are registered as Base* services (template param is Base, not Base*)
  DependencyServiceManager::registerService<Base>(b1);
  DependencyServiceManager::registerService<Base>(b2);

  // get<Base> performs dynamic_cast<Base*> on each element.
  // The first element (b1) will succeed because it really is a Derived.
  Base* found = DependencyServiceManager::get<Base>();
  EXPECT_EQ(found, b1);  // first matching service should be returned

  // Cleanup
  delete b1;  // actually deletes the Derived object
  delete b2;
  DependencyServiceManager::services<Base>.clear();
}

TEST(DependencyServiceManagerTest, GetReturnsNullptrForWrongType) {
  Derived* d = new Derived();
  DependencyServiceManager::registerService<Base>(d);  // register Derived* as Base*

  // Register a plain Base object as well
  Base* plainBase = new Base();
  DependencyServiceManager::registerService<Base>(plainBase);

  // Now request Derived*; dynamic_cast<Derived*>(Base*) would fail,
  // but services<Derived> is a separate vector that is empty
  Derived* found = DependencyServiceManager::get<Derived>();
  EXPECT_EQ(found, nullptr);

  // Cleanup
  delete plainBase;
  delete d;
  DependencyServiceManager::services<Base>.clear();
  DependencyServiceManager::services<Derived>.clear();
}

// ---------------------------------------------------------------------
// Main function to run all tests
// ---------------------------------------------------------------------
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}