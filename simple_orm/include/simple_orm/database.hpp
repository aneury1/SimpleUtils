#pragma once
#include "events.hpp"
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
struct sqlite3;
struct sqlite3_stmt;
namespace simple_orm {
class Database;
template<typename T> class TableSchema;
template<typename T> class Dao;
class DynamicDao;
class Statement {
public:
    Statement(Database& db, const std::string& sql);
    ~Statement();
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement(Statement&&) = delete;
    Statement& operator=(Statement&&) = delete;
    void bind(int position, const Value& value);
    bool next(); // true => SQLITE_ROW; false => SQLITE_DONE
    int execute(); // non-query, returns sqlite3_changes()
    Value value(int index) const;
    int columnCount() const;
    std::string columnName(int index) const;
    std::int64_t int64(int index) const;
private:
    Database& db_;
    sqlite3_stmt* stmt_ = nullptr;
    std::size_t pendingStart_ = 0;
};

enum class JournalMode { Default, WAL, Delete };
struct DatabaseOptions {
    std::string path = ":memory:";
    bool foreignKeys = true;
    int busyTimeoutMs = 5000;
    JournalMode journalMode = JournalMode::Default;
};
class Transaction {
public:
    explicit Transaction(Database& db);
    ~Transaction() noexcept;
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    Transaction(Transaction&&) = delete;
    Transaction& operator=(Transaction&&) = delete;
    void commit();
    void rollback();
private:
    Database* db_;
};
class Database {
public:
    explicit Database(DatabaseOptions opts = {});
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&&) = delete;
    Database& operator=(Database&&) = delete;
    void open();
    void close();
    bool isOpen() const noexcept { return db_ != nullptr; }
    std::string path() const { return opts_.path; }
    int execute(const std::string& sql);
    std::unique_ptr<Statement> prepare(const std::string& sql);
    std::int64_t lastInsertRowId() const;
    template<typename T> Dao<T> dao(const TableSchema<T>& schema);
    DynamicDao dao(const TableSchema<void>& schema);
    Transaction transaction() { return Transaction(*this); }
    Subscription observeDatabase(EventCallback);
    Subscription observeTable(const std::string& table, EventCallback);
    Subscription observeColumn(const std::string& table, const std::string& column, EventCallback);
    bool hasTable(const std::string& name) const;
    void createTableIfNotExists(const std::string& name, const std::vector<std::string>& columnsSql, bool strict = true);
    void dropTableIfExists(const std::string& name);
    sqlite3* nativeHandle() noexcept { return db_; }
    bool supportsColumnEvents() const noexcept;
    static std::string quoteIdentifier(const std::string& name);
private:
    friend class Statement;
    friend class Transaction;
    static void onUpdate(void*, int, const char*, const char*, long long);
#ifdef SIMPLE_ORM_HAS_PREUPDATE
    static void onPreUpdate(void*, sqlite3*, int, const char*, const char*, long long, long long);
#endif
    void enqueue(Event e);
    void emit(const Event&);
    void finishStatement();
    void discardEvents();
    void notifyEvents();
    void emitState(EventKind);
    void cacheColumns(const std::string& table);
    std::unordered_map<std::string, std::vector<std::string>> columnCache_;
    DatabaseOptions opts_;
    sqlite3* db_ = nullptr;
    std::shared_ptr<detail::EventBus> bus_ = std::make_shared<detail::EventBus>();
    std::vector<Event> pending_;
    bool inTransaction_ = false;
    bool notifying_ = false;
};
}
