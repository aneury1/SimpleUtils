#pragma once
#include "query.hpp"
#include <algorithm>
#include <unordered_set>

namespace simple_orm {
template<typename T> class Dao {
public:
    Dao(Database& database,TableSchema<T> schema):db_(&database),table_(std::move(schema)){}
    std::int64_t insert(T& item) {
        std::string sql="INSERT INTO "+Database::quoteIdentifier(table_.name())+" (";
        std::vector<const ColumnMapping<T>*> cols;
        for(auto& c:table_.mappings()) {
            Value v=c.get(item);
            if(c.def.autoIncrement && (v==toValue(std::int64_t{0}) || v==Value{nullptr}))continue;
            cols.push_back(&c);
        }
        if(cols.empty())sql="INSERT INTO "+Database::quoteIdentifier(table_.name())+" DEFAULT VALUES";
        else {
            for(std::size_t i=0;i<cols.size();++i){if(i)sql+=",";sql+=Database::quoteIdentifier(cols[i]->def.name);}
            sql+=") VALUES (";
            for(std::size_t i=0;i<cols.size();++i){if(i)sql+=",";sql+="?";}
            sql+=")";
        }
        auto stmt=db_->prepare(sql);
        for(std::size_t i=0;i<cols.size();++i)stmt->bind(static_cast<int>(i+1),cols[i]->get(item));
        stmt->execute();
        std::int64_t id=db_->lastInsertRowId();
        for(const auto& c:table_.mappings())if(c.def.primaryKey && c.def.autoIncrement)c.set(item,toValue(id));
        return id;
    }
    std::optional<T> findById(const Value& key) const {
        const auto& pk=table_.primaryKey();
        auto result=query().where(Condition{[col=pk.def.name](const auto&){return Database::quoteIdentifier(col)+" = ?";},{key}}).first();
        return result;
    }
    template<typename Key> std::optional<T> findById(const Key& key) const {return findById(toValue(key));}
    std::vector<T> findAll() const {return query().all();}
    bool update(const T& item) {
        const auto& pk=table_.primaryKey();
        std::string sql="UPDATE "+Database::quoteIdentifier(table_.name())+" SET ";
        int count=0;
        for(const auto& m:table_.mappings())if(!m.def.primaryKey){if(count++)sql+=",";sql+=Database::quoteIdentifier(m.def.name)+"=?";}
        if(!count)throw Error("No mutable columns");
        sql+=" WHERE "+Database::quoteIdentifier(pk.def.name)+"=?";
        auto stmt=db_->prepare(sql);
        int pos=1;
        for(const auto& m:table_.mappings())if(!m.def.primaryKey)stmt->bind(pos++,m.get(item));
        stmt->bind(pos,pk.get(item));
        return stmt->execute()>0;
    }
    bool remove(const T& item){return removeById(table_.primaryKey().get(item));}
    bool removeById(const Value& id) {
        auto stmt=db_->prepare("DELETE FROM "+Database::quoteIdentifier(table_.name())+" WHERE "+Database::quoteIdentifier(table_.primaryKey().def.name)+"=?");
        stmt->bind(1,id);
        return stmt->execute()>0;
    }
    template<typename Key> bool removeById(const Key& id) {return removeById(toValue(id));}
    int deleteAll() {auto stmt=db_->prepare("DELETE FROM "+Database::quoteIdentifier(table_.name()));return stmt->execute();}
    std::int64_t count() const {return query().count();}
    bool existsById(const Value& id) const {return findById(id).has_value();}
    template<typename Key> bool existsById(const Key& key) const {return existsById(toValue(key));}
    Query<T> query() const {return Query<T>(*db_,table_);}
private:
    Database* db_;
    TableSchema<T> table_;
};

class DynamicDao {
public:
    DynamicDao(Database& database,DynamicTable table):db_(&database),table_(std::move(table)){}
    std::int64_t insert(Row row) {
        // Strictly permit only column names present in the declared schema.
        for(const auto& [name,value]:row) {
            (void)value;
            if(!known(name))throw Error("Unknown column: "+name);
        }
        std::string sql="INSERT INTO "+Database::quoteIdentifier(table_.name());
        std::vector<std::pair<std::string,Value>> values;
        // Use schema order, not unordered_map iteration order.
        for(const auto& c:table_.columns()){
            auto it=row.find(c.name);
            if(it!=row.end()){
                if(c.autoIncrement && (it->second==toValue(std::int64_t{0}) || it->second==Value{nullptr}))continue;
                validateValue(c,it->second);
                values.emplace_back(it->first,it->second);
            }
        }
        if(values.empty())sql+=" DEFAULT VALUES";
        else {
            sql+=" (";
            for(std::size_t i=0;i<values.size();++i){if(i)sql+=",";sql+=Database::quoteIdentifier(values[i].first);}
            sql+=") VALUES (";
            for(std::size_t i=0;i<values.size();++i){if(i)sql+=",";sql+="?";}
            sql+=")";
        }
        auto stmt=db_->prepare(sql);
        for(std::size_t i=0;i<values.size();++i)stmt->bind(static_cast<int>(i+1),values[i].second);
        stmt->execute();
        return db_->lastInsertRowId();
    }
    std::optional<Row> findById(const Value& id) const {
        auto stmt=db_->prepare("SELECT * FROM "+Database::quoteIdentifier(table_.name())+" WHERE "+Database::quoteIdentifier(table_.primaryKey().name)+"=? LIMIT 1");
        stmt->bind(1,id);
        if(!stmt->next())return std::nullopt;
        Row row;for(int i=0;i<stmt->columnCount();++i)row.emplace(stmt->columnName(i),stmt->value(i));
        return row;
    }
    template<typename Key> std::optional<Row> findById(const Key& id) const{return findById(toValue(id));}
    std::vector<Row> findAll() const {
        auto stmt=db_->prepare("SELECT * FROM "+Database::quoteIdentifier(table_.name()));
        std::vector<Row> rows;
        while(stmt->next()){
            Row row;for(int i=0;i<stmt->columnCount();++i)row.emplace(stmt->columnName(i),stmt->value(i));
            rows.push_back(std::move(row));
        }
        return rows;
    }
    bool update(const Value& id,const Row& changes) {
        if(changes.empty())return false;
        std::string sql="UPDATE "+Database::quoteIdentifier(table_.name())+" SET ";
        std::vector<std::pair<std::string,Value>> vals;
        for(const auto& c:table_.columns()){
            auto it=changes.find(c.name);
            if(it==changes.end())continue;
            if(c.primaryKey)throw Error("Primary key cannot be updated");
            validateValue(c,it->second);
            vals.emplace_back(it->first,it->second);
        }
        if(vals.size()!=changes.size())throw Error("Unknown column in UPDATE");
        for(std::size_t i=0;i<vals.size();++i){if(i)sql+=",";sql+=Database::quoteIdentifier(vals[i].first)+"=?";}
        sql+=" WHERE "+Database::quoteIdentifier(table_.primaryKey().name)+"=?";
        auto stmt=db_->prepare(sql);
        for(std::size_t i=0;i<vals.size();++i)stmt->bind(static_cast<int>(i+1),vals[i].second);
        stmt->bind(static_cast<int>(vals.size()+1),id);
        return stmt->execute()>0;
    }
    template<typename Key> bool update(const Key& key,const Row& changes){return update(toValue(key),changes);}
    bool removeById(const Value& id){auto stmt=db_->prepare("DELETE FROM "+Database::quoteIdentifier(table_.name())+" WHERE "+Database::quoteIdentifier(table_.primaryKey().name)+"=?");stmt->bind(1,id);return stmt->execute()>0;}
    template<typename Key> bool removeById(const Key& key){return removeById(toValue(key));}
    int deleteAll(){auto stmt=db_->prepare("DELETE FROM "+Database::quoteIdentifier(table_.name()));return stmt->execute();}
    std::int64_t count() const {auto stmt=db_->prepare("SELECT COUNT(*) FROM "+Database::quoteIdentifier(table_.name()));if(!stmt->next())throw Error("COUNT failed");return stmt->int64(0);}
private:
    bool known(const std::string& col) const {for(auto& c:table_.columns())if(c.name==col)return true;return false;}
    Database* db_;
    DynamicTable table_;
};

template<typename T> Dao<T> makeDao(Database& db,const TableSchema<T>& table){return Dao<T>(db,table);}
inline DynamicDao makeDao(Database& db,const DynamicTable& table){return DynamicDao(db,table);}
template<typename T> Dao<T> Database::dao(const TableSchema<T>& schema) {return makeDao(*this,schema);}
inline DynamicDao Database::dao(const DynamicTable& schema) {return makeDao(*this,schema);}
}
