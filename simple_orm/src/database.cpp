#include <simple_orm/database.hpp>
#include <sqlite3.h>
#include <algorithm>
#include <cctype>
#include <utility>

namespace simple_orm {
namespace {
void check(int code, sqlite3* db, const std::string& context) {
    if (code != SQLITE_OK) throw Error(context + ": " + (db ? sqlite3_errmsg(db) : sqlite3_errstr(code)), code);
}
Value rawValue(sqlite3_value* v) {
    if (!v || sqlite3_value_type(v) == SQLITE_NULL) return nullptr;
    switch(sqlite3_value_type(v)) {
        case SQLITE_INTEGER: return static_cast<std::int64_t>(sqlite3_value_int64(v));
        case SQLITE_FLOAT: return sqlite3_value_double(v);
        case SQLITE_TEXT: return std::string(reinterpret_cast<const char*>(sqlite3_value_text(v)), static_cast<std::size_t>(sqlite3_value_bytes(v)));
        case SQLITE_BLOB: {
            const auto* p = static_cast<const std::uint8_t*>(sqlite3_value_blob(v));
            int n = sqlite3_value_bytes(v);
            return n ? Blob(p,p+n) : Blob{};
        }
        default: return nullptr;
    }
}
bool blank(const char* str) {
    while (*str) { if (!std::isspace(static_cast<unsigned char>(*str))) return false; ++str; }
    return true;
}
std::string firstWord(const std::string& str) {
    auto start = str.find_first_not_of(" \n\t\r");
    if (start == std::string::npos) return {};
    auto end = str.find_first_of(" \n\t\r", start);
    std::string word = str.substr(start, end == std::string::npos ? end : end-start);
    std::transform(word.begin(),word.end(),word.begin(),[](char c){return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));});
    return word;
}
}

Statement::Statement(Database& db, const std::string& sql) : db_(db) {
    if (!db_.db_) throw Error("Database is closed");
    const char* tail = nullptr;
    check(sqlite3_prepare_v2(db_.db_, sql.c_str(), -1, &stmt_, &tail), db_.db_, "prepare");
    if (!stmt_ || (tail && !blank(tail))) {
        sqlite3_finalize(stmt_); stmt_ = nullptr;
        throw Error("Only one SQL statement is allowed per prepare/execute call");
    }
    pendingStart_ = db_.pending_.size();
}
Statement::~Statement() { sqlite3_finalize(stmt_); }
void Statement::bind(int position, const Value& v) {
    int rc = std::visit([&](const auto& data)->int {
        using T = std::decay_t<decltype(data)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) return sqlite3_bind_null(stmt_, position);
        else if constexpr (std::is_same_v<T,std::int64_t>) return sqlite3_bind_int64(stmt_,position,data);
        else if constexpr (std::is_same_v<T,double>) return sqlite3_bind_double(stmt_,position,data);
        else if constexpr (std::is_same_v<T,std::string>) return sqlite3_bind_text64(stmt_,position,data.data(),data.size(),SQLITE_TRANSIENT,SQLITE_UTF8);
        else return sqlite3_bind_blob64(stmt_, position, data.empty()?static_cast<const void*>(""):static_cast<const void*>(data.data()), data.size(), SQLITE_TRANSIENT);
    },v);
    check(rc,db_.db_,"bind");
}
Value Statement::value(int index) const {
    if (index < 0 || index >= columnCount()) throw Error("Column index out of bounds");
    switch(sqlite3_column_type(stmt_,index)) {
        case SQLITE_INTEGER: return static_cast<std::int64_t>(sqlite3_column_int64(stmt_,index));
        case SQLITE_FLOAT: return sqlite3_column_double(stmt_,index);
        case SQLITE_TEXT: return std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt_,index)),sqlite3_column_bytes(stmt_,index));
        case SQLITE_BLOB: {
            const auto* p = static_cast<const std::uint8_t*>(sqlite3_column_blob(stmt_,index));
            int n=sqlite3_column_bytes(stmt_,index);
            return n ? Blob(p,p+n) : Blob{};
        }
        default: return nullptr;
    }
}
int Statement::columnCount() const { return sqlite3_column_count(stmt_); }
std::string Statement::columnName(int i) const { if(i<0 || i>=columnCount())throw Error("Column index out of bounds"); return sqlite3_column_name(stmt_,i); }
std::int64_t Statement::int64(int index) const { return fromValue<std::int64_t>(value(index)); }
bool Statement::next() {
    const int rc = sqlite3_step(stmt_);
    if (rc==SQLITE_ROW) return true;
    if (rc!=SQLITE_DONE) {
        sqlite3_reset(stmt_);
        db_.pending_.resize(std::min(pendingStart_,db_.pending_.size()));
        throw Error("step: " + std::string(sqlite3_errmsg(db_.db_)),rc);
    }
    sqlite3_reset(stmt_);
    db_.finishStatement();
    return false;
}
int Statement::execute() {
    const int rc=sqlite3_step(stmt_);
    if (rc!=SQLITE_DONE) {
        std::string message=sqlite3_errmsg(db_.db_);
        sqlite3_reset(stmt_);
        db_.pending_.resize(std::min(pendingStart_,db_.pending_.size()));
        throw Error("execute: "+message,rc);
    }
    const int modified = sqlite3_changes(db_.db_);
    sqlite3_reset(stmt_);
    db_.finishStatement();
    return modified;
}

Database::Database(DatabaseOptions opts) : opts_(std::move(opts)) {}
Database::~Database() {
    if (db_) {
        sqlite3_update_hook(db_,nullptr,nullptr);
#ifdef SIMPLE_ORM_HAS_PREUPDATE
        sqlite3_preupdate_hook(db_,nullptr,nullptr);
#endif
        sqlite3_close_v2(db_);
        db_ = nullptr;
    }
}
void Database::open() {
    if(db_) throw Error("Database is already open");
    int rc=sqlite3_open_v2(opts_.path.c_str(),&db_, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX,nullptr);
    if(rc != SQLITE_OK) { std::string msg=db_?sqlite3_errmsg(db_):sqlite3_errstr(rc); sqlite3_close(db_); db_=nullptr; throw Error("open: "+msg,rc); }
    try {
        if (sqlite3_libversion_number() < 3037000) throw Error("SQLite >= 3.37.0 required for STRICT support");
        check(sqlite3_busy_timeout(db_,opts_.busyTimeoutMs),db_,"busy timeout");
        execute(std::string("PRAGMA foreign_keys = ") + (opts_.foreignKeys?"ON":"OFF"));
        if(opts_.journalMode != JournalMode::Default) {
            Statement pragma(*this,opts_.journalMode==JournalMode::WAL ? "PRAGMA journal_mode=WAL" : "PRAGMA journal_mode=DELETE");
            while(pragma.next()) {}
        }
#ifdef SIMPLE_ORM_HAS_PREUPDATE
        if (!sqlite3_compileoption_used("ENABLE_PREUPDATE_HOOK")) throw Error("SQLite runtime lacks ENABLE_PREUPDATE_HOOK");
        sqlite3_preupdate_hook(db_,&Database::onPreUpdate,this);
#else
        sqlite3_update_hook(db_,&Database::onUpdate,this);
#endif
        emitState(EventKind::Opened);
    } catch (...) { sqlite3_close(db_); db_=nullptr; throw; }
}
void Database::close() {
    if (!db_) return;
    if(inTransaction_) throw Error("Cannot close while transaction is active");
    emitState(EventKind::Closed);
    sqlite3_update_hook(db_,nullptr,nullptr);
#ifdef SIMPLE_ORM_HAS_PREUPDATE
    sqlite3_preupdate_hook(db_,nullptr,nullptr);
#endif
    int rc = sqlite3_close(db_);
    if (rc!=SQLITE_OK) {
#ifdef SIMPLE_ORM_HAS_PREUPDATE
        sqlite3_preupdate_hook(db_,&Database::onPreUpdate,this);
#else
        sqlite3_update_hook(db_,&Database::onUpdate,this);
#endif
        throw Error("close: pending statements exist",rc);
    }
    db_=nullptr;
    columnCache_.clear();
    pending_.clear();
}
int Database::execute(const std::string& sql) {
    const std::string action = firstWord(sql);
    if(action=="ROLLBACK") discardEvents();
    Statement stmt(*this,sql);
    return stmt.execute();
}
std::unique_ptr<Statement> Database::prepare(const std::string& sql) { return std::make_unique<Statement>(*this,sql); }
std::int64_t Database::lastInsertRowId() const { if(!db_)throw Error("Database is closed"); return sqlite3_last_insert_rowid(db_); }
std::string Database::quoteIdentifier(const std::string& name) {
    if (name.empty() || name.find('\0')!=std::string::npos) throw Error("Invalid SQLite identifier");
    std::string out="\"";
    for (char c:name) { if(c=='\"')out+="\"\""; else out+=c; }
    out+='\"';
    return out;
}
Subscription Database::observeDatabase(EventCallback cb) { return observeTable("",std::move(cb)); }
Subscription Database::observeTable(const std::string& table, EventCallback cb) {
    if (!cb) throw Error("Null observer callback");
    std::lock_guard<std::mutex> lock(bus_->mutex);
    auto id=bus_->nextId++;
    bus_->listeners.push_back({id,table,"",std::move(cb)});
    return Subscription(bus_,id);
}
Subscription Database::observeColumn(const std::string& table,const std::string& column,EventCallback cb) {
    if(table.empty() || column.empty() || !cb) throw Error("Invalid column observer");
    if(!supportsColumnEvents()) throw Error("Column events require SQLite ENABLE_PREUPDATE_HOOK");
    cacheColumns(table);
    auto it=columnCache_.find(table);
    if(it==columnCache_.end() || std::find(it->second.begin(),it->second.end(),column)==it->second.end()) throw Error("Unknown column for observer: "+column);
    std::lock_guard<std::mutex> lock(bus_->mutex);
    auto id=bus_->nextId++;
    bus_->listeners.push_back({id,table,column,std::move(cb)});
    return Subscription(bus_,id);
}
bool Database::supportsColumnEvents() const noexcept {
#ifdef SIMPLE_ORM_HAS_PREUPDATE
    return db_ && sqlite3_compileoption_used("ENABLE_PREUPDATE_HOOK");
#else
    return false;
#endif
}
void Database::emit(const Event& e) {
    std::vector<EventCallback> callbacks;
    {
        std::lock_guard<std::mutex> lock(bus_->mutex);
        for(const auto& item:bus_->listeners) {
            if(!item.table.empty() && item.table!=e.table)continue;
            if(!item.column.empty() && item.column!=e.column)continue;
            if(item.column.empty() && !e.column.empty())continue; // database/table get only row/schema events
            callbacks.push_back(item.callback);
        }
    }
    // Subscribers throwing must not turn committed SQL into an apparent failure.
    for(auto& cb:callbacks) { try {cb(e);} catch(...) {} }
}
void Database::enqueue(Event e) { e.database=opts_.path; pending_.push_back(std::move(e)); }
void Database::notifyEvents() {
    if(notifying_ || pending_.empty())return;
    notifying_=true;
    // Queue changes generated by listeners and process them after the current batch.
    // The cap protects against accidentally recursive event-driven writes.
    for(int batch=0; batch<100 && !pending_.empty(); ++batch) {
        auto messages=std::move(pending_);
        pending_.clear();
        for(const auto& e:messages)emit(e);
    }
    notifying_=false;
}
void Database::discardEvents() {pending_.clear();}
void Database::finishStatement() {
    if (db_ && sqlite3_get_autocommit(db_)) notifyEvents();
}
void Database::emitState(EventKind kind) { Event e; e.kind=kind; e.database=opts_.path; emit(e); }
void Database::cacheColumns(const std::string& table) {
    if(!db_) throw Error("Database closed");
    auto statement=prepare("PRAGMA table_info("+quoteIdentifier(table)+")");
    std::vector<std::string> columns;
    while(statement->next()) columns.push_back(fromValue<std::string>(statement->value(1)));
    columnCache_[table]=std::move(columns);
}
bool Database::hasTable(const std::string& name) const {
    if(!db_)throw Error("Database closed");
    auto stmt=const_cast<Database*>(this)->prepare("SELECT 1 FROM sqlite_master WHERE type='table' AND name=? LIMIT 1");
    stmt->bind(1,toValue(name));
    return stmt->next();
}
void Database::createTableIfNotExists(const std::string& name,const std::vector<std::string>& columnsSql,bool strict) {
    const bool exists=hasTable(name);
    if(columnsSql.empty())throw Error("A table must have columns");
    std::string sql="CREATE TABLE IF NOT EXISTS "+quoteIdentifier(name)+" (";
    for(std::size_t i=0;i<columnsSql.size();++i) { if(i)sql+=","; sql+=columnsSql[i]; }
    sql+=")";
    if(strict)sql+=" STRICT";
    execute(sql);
    cacheColumns(name);
    if(!exists) { Event e; e.kind=EventKind::TableCreated; e.table=name; enqueue(e); finishStatement(); }
}
void Database::dropTableIfExists(const std::string& name) {
    bool exists=hasTable(name);
    execute("DROP TABLE IF EXISTS "+quoteIdentifier(name));
    columnCache_.erase(name);
    if(exists) {Event e; e.kind=EventKind::TableDropped; e.table=name; enqueue(e); finishStatement();}
}
void Database::onUpdate(void* ud,int action,const char*,const char* table,long long rowid) {
    auto* self=static_cast<Database*>(ud);
    Event e; e.kind=action==SQLITE_INSERT?EventKind::Insert:action==SQLITE_DELETE?EventKind::Delete:EventKind::Update;
    e.table=table; e.rowId=rowid; self->enqueue(std::move(e));
}
#ifdef SIMPLE_ORM_HAS_PREUPDATE
void Database::onPreUpdate(void* ud,sqlite3* handle,int action,const char*,const char* table,long long oldRow,long long newRow) {
    auto* self=static_cast<Database*>(ud);
    try {
        Event e; e.kind=action==SQLITE_INSERT?EventKind::Insert:action==SQLITE_DELETE?EventKind::Delete:EventKind::Update;
        e.table=table; e.rowId=action==SQLITE_DELETE?oldRow:newRow;
        self->enqueue(e);
        auto it=self->columnCache_.find(table);
        if(it==self->columnCache_.end()) return;
        int count=sqlite3_preupdate_count(handle);
        for(int i=0;i<count && i<static_cast<int>(it->second.size());++i) {
            sqlite3_value *oldPtr=nullptr,*newPtr=nullptr;
            if(action!=SQLITE_INSERT && sqlite3_preupdate_old(handle,i,&oldPtr)!=SQLITE_OK)continue;
            if(action!=SQLITE_DELETE && sqlite3_preupdate_new(handle,i,&newPtr)!=SQLITE_OK)continue;
            Value oldValue=rawValue(oldPtr),newValue=rawValue(newPtr);
            if (oldValue==newValue)continue;
            Event c=e; c.column=it->second[i];
            if(action!=SQLITE_INSERT)c.oldValue=std::move(oldValue);
            if(action!=SQLITE_DELETE)c.newValue=std::move(newValue);
            self->enqueue(std::move(c));
        }
    } catch (...) { /* SQLite callbacks must not throw */ }
}
#endif
Transaction::Transaction(Database& db) : db_(&db) {
    if(db_->inTransaction_)throw Error("Nested transactions not supported");
    db_->execute("BEGIN IMMEDIATE"); db_->inTransaction_=true;
}
Transaction::~Transaction() noexcept { if(db_) {try{rollback();}catch(...){} } }
void Transaction::commit() {
    if(!db_)throw Error("Transaction has ended");
    db_->execute("COMMIT");
    db_->inTransaction_=false;
    db_->emitState(EventKind::Committed);
    db_=nullptr;
}
void Transaction::rollback() {
    if(!db_)return;
    db_->discardEvents();
    db_->execute("ROLLBACK");
    db_->inTransaction_=false;
    db_->emitState(EventKind::RolledBack);
    db_=nullptr;
}
}
