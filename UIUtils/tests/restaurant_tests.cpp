#include <gtest/gtest.h>
#include "RestaurantModel.h"
using namespace restaurant;
TEST(Orders, RepeatedProductMergesAndQuantitiesUpdateTotals) {
  OrderBook book;
  EXPECT_EQ(book.Add(0), 0);
  EXPECT_EQ(book.Add(0), 0);
  ASSERT_EQ(book.Current().lines.size(), 1u);
  EXPECT_EQ(book.Total(), 1700);
  book.SetQuantity(0, 3);
  EXPECT_EQ(book.Total(), 2550);
  book.Add(7);
  EXPECT_EQ(book.Total(), 2700);
  book.Remove(0);
  EXPECT_EQ(book.Total(), 150);
}
TEST(Orders, TablesAndTakeawayKeepIndependentOrdersAndNotes) {
  OrderBook book;
  book.Add(0);
  book.Current().notes = "No onion";
  book.SelectTable(1);
  EXPECT_EQ(book.Total(), 0);
  book.Add(7);
  book.SelectTable(6);
  EXPECT_EQ(book.TableName(), "Takeaway");
  book.Add(9);
  book.Clear();
  EXPECT_EQ(book.Total(), 0);
  book.SelectTable(0);
  EXPECT_EQ(book.Total(), 850);
  EXPECT_EQ(book.Current().notes, "No onion");
  book.SelectTable(1);
  EXPECT_EQ(book.Total(), 150);
}
TEST(Orders, InvalidChangesLeaveTheOrderIntact) {
  OrderBook book;
  book.Add(0);
  EXPECT_THROW(book.Add(products.size()), std::out_of_range);
  EXPECT_THROW(book.SelectTable(7), std::out_of_range);
  EXPECT_THROW(book.SetQuantity(0, 100), std::out_of_range);
  EXPECT_THROW(book.Remove(-1), std::out_of_range);
  EXPECT_EQ(book.Total(), 850);
  book.SetQuantity(0, 99);
  EXPECT_EQ(book.Add(0), uiutils::NoSelection);
  EXPECT_EQ(book.Total(), 84150);
}
TEST(Orders, CurrencyFormattingUsesExactCents) {
  EXPECT_EQ(Money(0), "EUR 0.00");
  EXPECT_EQ(Money(5), "EUR 0.05");
  EXPECT_EQ(Money(1050), "EUR 10.50");
  EXPECT_THROW(Money(-1), std::invalid_argument);
}
