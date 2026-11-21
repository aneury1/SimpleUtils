#pragma once
#include "database.hpp"
#include <functional>
#include <sstream>
#include <iomanip>
#include <type_traits>
#include <utility>

namespace simple_orm {
struct ColumnDef {
    std::string name;
    DataType type = DataType::Text;
    bool nullable = false;
    bool primaryKey = false;
    bool autoIncrement = false;
    bool unique = false;
    std::optional<Value> defaultValue;
    std::string referenceTable;
    std::string referenceColumn;
    bool cascadeOnDelete = false;
    const void* memberId = nullptr;
};

inline std::string sqlType(DataType t) {
    switch(t) {case DataType::Integer:return "INTEGER";case DataType::Real:return "REAL";case DataType::Text:return "TEXT";case DataType::Blob:return "BLOB";case DataType::Any:return "ANY";}
    throw Error("Unknown SQLite type");
}
inline std::string sqlLiteral(const Value& v) {
    if(std::holds_alternative<std::nullptr_t>(v))return "NULL";
    if(auto p=std::get_if<std::int64_t>(&v))return std::to_string(*p);
    if(auto p=std::get_if<double>(&v)) {std::ostringstream s; s<<std::setprecision(17)<<*p; return s.str();}
    if(auto p=std::get_if<std::string>(&v)) {
        std::string x="'"; for(char c:*p) {if(c=='\'')x+="''";else x+=c;} return x+"'";
    }
    const auto& bytes=std::get<Blob>(v);
    static const char* hex="0123456789ABCDEF";
    std::string x="X'"; for(std::uint8_t b:bytes) {x+=hex[b>>4];x+=hex[b&15];} return x+"'";
}
inline std::string definitionSql(const ColumnDef& c) {
    auto q = Database::quoteIdentifier;
    if(c.autoIncrement && (!c.primaryKey || c.type!=DataType::Integer)) throw Error("AUTOINCREMENT requires INTEGER PRIMARY KEY");
    std::string s=q(c.name)+" "+sqlType(c.type);
    if(c.primaryKey)s+=" PRIMARY KEY";
    if(c.autoIncrement)s+=" AUTOINCREMENT";
    if(!c.nullable && !c.primaryKey)s+=" NOT NULL";
    if(c.unique)s+=" UNIQUE";
    if(c.defaultValue)s+=" DEFAULT "+sqlLiteral(*c.defaultValue);
    if(!c.referenceTable.empty()) {
        s+=" REFERENCES "+q(c.referenceTable);
        if(!c.referenceColumn.empty())s+="("+q(c.referenceColumn)+")";
        if(c.cascadeOnDelete)s+=" ON DELETE CASCADE";
    }
    return s;
}
inline void validateValue(const ColumnDef& c, const Value& value) {
    if(std::holds_alternative<std::nullptr_t>(value)) {
        if(!c.nullable) throw Error("NULL is not allowed for column: "+c.name);
        return;
    }
    bool valid=false;
    switch(c.type) {
        case DataType::Integer:valid=std::holds_alternative<std::int64_t>(value);break;
        case DataType::Real:valid=std::holds_alternative<double>(value);break;
        case DataType::Text:valid=std::holds_alternative<std::string>(value);break;
        case DataType::Blob:valid=std::holds_alternative<Blob>(value);break;
        case DataType::Any:valid=true;break;
    }
    if(!valid)throw Error("Incorrect typed value for column: "+c.name);
}

inline void validateColumns(const std::vector<ColumnDef>& cols) {
    if(cols.empty())throw Error("Table must contain columns");
    int pk=0;
    for(std::size_t i=0;i<cols.size();++i) {
        Database::quoteIdentifier(cols[i].name);
        if(cols[i].primaryKey)++pk;
        for(std::size_t j=0;j<i;++j) if(cols[i].name==cols[j].name)throw Error("Duplicate column: "+cols[i].name);
    }
    if(pk>1)throw Error("Composite primary keys are not supported in v2.0");
}
inline std::vector<std::string> columnSql(const std::vector<ColumnDef>& cols) {
    validateColumns(cols);
    std::vector<std::string> result;
    for(const auto& c:cols)result.push_back(definitionSql(c));
    return result;
}

template<typename T> struct MemberTraits;
template<typename Class, typename MemberType>
struct MemberTraits<MemberType Class::*> {using Entity=Class;using Member=MemberType;};
template<auto Pointer> struct MemberId {static inline int token=0;};

template<typename Entity> struct ColumnMapping {
    ColumnDef def;
    std::function<Value(const Entity&)> get;
    std::function<void(Entity&,const Value&)> set;
};

template<auto Member> class TypedColumnBuilder {
public:
    using Traits=MemberTraits<decltype(Member)>;
    using Entity=typename Traits::Entity;
    using Type=typename Traits::Member;
    explicit TypedColumnBuilder(std::string name) {
        mapping_.def.name=std::move(name);
        mapping_.def.type=dataType<Type>();
        mapping_.def.nullable=optional_traits<Type>::is_optional;
        mapping_.def.memberId=&MemberId<Member>::token;
        mapping_.get=[](const Entity& object){ return toValue(object.*Member); };
        mapping_.set=[](Entity& object,const Value& v){ object.*Member=fromValue<Type>(v); };
    }
    TypedColumnBuilder& notNull() {mapping_.def.nullable=false;return *this;}
    TypedColumnBuilder& nullable() {static_assert(optional_traits<Type>::is_optional,"Use std::optional<T> for nullable model fields");mapping_.def.nullable=true;return *this;}
    TypedColumnBuilder& primaryKey() {mapping_.def.primaryKey=true;mapping_.def.nullable=false;return *this;}
    TypedColumnBuilder& autoIncrement() {mapping_.def.autoIncrement=true;return *this;}
    TypedColumnBuilder& unique() {mapping_.def.unique=true;return *this;}
    template<typename V> TypedColumnBuilder& defaultValue(V v) {mapping_.def.defaultValue=toValue(v);return *this;}
    TypedColumnBuilder& references(std::string table,std::string column="") {mapping_.def.referenceTable=std::move(table);mapping_.def.referenceColumn=std::move(column);return *this;}
    TypedColumnBuilder& onDeleteCascade() {mapping_.def.cascadeOnDelete=true;return *this;}
    ColumnMapping<Entity> build() const {return mapping_;}
private:
    ColumnMapping<Entity> mapping_;
};
template<auto Member> TypedColumnBuilder<Member> column(const std::string& name) {return TypedColumnBuilder<Member>(name);}

template<typename Type> class DynamicColumnBuilder {
public:
    explicit DynamicColumnBuilder(std::string name) {def_.name=std::move(name);def_.type=dataType<Type>();def_.nullable=optional_traits<Type>::is_optional;}
    DynamicColumnBuilder& notNull(){def_.nullable=false;return *this;}
    DynamicColumnBuilder& nullable(){def_.nullable=true;return *this;}
    DynamicColumnBuilder& primaryKey(){def_.primaryKey=true;def_.nullable=false;return *this;}
    DynamicColumnBuilder& autoIncrement(){def_.autoIncrement=true;return *this;}
    DynamicColumnBuilder& unique(){def_.unique=true;return *this;}
    template<typename V> DynamicColumnBuilder& defaultValue(V v){def_.defaultValue=toValue(v);return *this;}
    DynamicColumnBuilder& references(std::string table,std::string column=""){def_.referenceTable=std::move(table);def_.referenceColumn=std::move(column);return *this;}
    DynamicColumnBuilder& onDeleteCascade(){def_.cascadeOnDelete=true;return *this;}
    ColumnDef build() const {return def_;}
private:
    ColumnDef def_;
};
template<typename T> DynamicColumnBuilder<T> column(const std::string& name){return DynamicColumnBuilder<T>(name);}

template<typename T=void> class TableSchema;
template<typename T> class TableSchema {
public:
    explicit TableSchema(std::string name):name_(std::move(name)) {Database::quoteIdentifier(name_);}
    template<auto Member> void add(const TypedColumnBuilder<Member>& c) {
        static_assert(std::is_same_v<typename TypedColumnBuilder<Member>::Entity,T>,"Column member belongs to a different model");
        columns_.push_back(c.build());
    }
    std::string name() const{return name_;}
    std::vector<ColumnDef> columns() const {std::vector<ColumnDef> out;for(auto& c:columns_)out.push_back(c.def);return out;}
    const std::vector<ColumnMapping<T>>& mappings() const noexcept {return columns_;}
    const ColumnMapping<T>& primaryKey() const {
        for(const auto& m:columns_) if(m.def.primaryKey)return m;
        throw Error("DAO requires a primary key");
    }
    void create(Database& db,bool strict=true) const {db.createTableIfNotExists(name_,columnSql(columns()),strict);}
private:
    std::string name_;
    std::vector<ColumnMapping<T>> columns_;
};
template<> class TableSchema<void> {
public:
    explicit TableSchema(std::string name):name_(std::move(name)){Database::quoteIdentifier(name_);}
    template<typename T> void add(const DynamicColumnBuilder<T>& c){columns_.push_back(c.build());}
    void add(ColumnDef c){columns_.push_back(std::move(c));}
    std::string name() const{return name_;}
    const std::vector<ColumnDef>& columns() const noexcept{return columns_;}
    const ColumnDef& primaryKey() const {for(auto& c:columns_)if(c.primaryKey)return c;throw Error("DynamicDao requires primary key");}
    void create(Database& db,bool strict=true) const {db.createTableIfNotExists(name_,columnSql(columns_),strict);}
private:
    std::string name_;
    std::vector<ColumnDef> columns_;
};
using DynamicTable = TableSchema<void>;
template<typename T,typename... Builders> TableSchema<T> makeTable(std::string name,const Builders&... builders) {
    TableSchema<T> t(std::move(name));
    (t.add(builders), ...);
    return t;
}

// Non-destructive schema grouping. Version is metadata; no automatic migrations.
class Schema {
public:
    Schema(std::string name, int version):name_(std::move(name)),version_(version) {
        if(name_.empty() || version_<1)throw Error("Schema requires name and positive version");
    }
    template<typename T> Schema& add(TableSchema<T> table) {
        creators_.push_back([table=std::move(table)](Database& db){table.create(db);});
        return *this;
    }
    void createMissing(Database& db) const {
        auto tx=db.transaction();
        for(const auto& create:creators_)create(db);
        tx.commit();
    }
    const std::string& name() const noexcept {return name_;}
    int version() const noexcept {return version_;}
private:
    std::string name_;
    int version_;
    std::vector<std::function<void(Database&)>> creators_;
};
}
