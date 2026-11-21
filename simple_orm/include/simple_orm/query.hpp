#pragma once
#include "schema.hpp"
#include <functional>
#include <sstream>
#include <utility>

namespace simple_orm {
inline std::string resolveField(const std::vector<ColumnDef>& columns,const void* token) {
    for(const auto& c:columns)if(c.memberId==token)return Database::quoteIdentifier(c.name);
    throw Error("Field does not belong to this table schema");
}
struct Condition {
    std::function<std::string(const std::vector<ColumnDef>&)> sql;
    std::vector<Value> binds;
};
inline Condition operator&&(Condition l,Condition r) {
    Condition out;
    out.sql=[a=l.sql,b=r.sql](const auto& columns){return "("+a(columns)+" AND "+b(columns)+")";};
    out.binds=std::move(l.binds);
    out.binds.insert(out.binds.end(),r.binds.begin(),r.binds.end());
    return out;
}
inline Condition operator||(Condition l,Condition r) {
    Condition out;
    out.sql=[a=l.sql,b=r.sql](const auto& columns){return "("+a(columns)+" OR "+b(columns)+")";};
    out.binds=std::move(l.binds);
    out.binds.insert(out.binds.end(),r.binds.begin(),r.binds.end());
    return out;
}
template<auto Member> class Field {
public:
    using Type=typename MemberTraits<decltype(Member)>::Member;
    Field()=default;
    template<typename U> Condition eq(const U& v) const {return cmp("=",v);}
    template<typename U> Condition ne(const U& v) const {return cmp("!=",v);}
    template<typename U> Condition lt(const U& v) const {return cmp("<",v);}
    template<typename U> Condition le(const U& v) const {return cmp("<=",v);}
    template<typename U> Condition gt(const U& v) const {return cmp(">",v);}
    template<typename U> Condition ge(const U& v) const {return cmp(">=",v);}
    template<typename U> Condition operator==(const U& v) const {return eq(v);}
    template<typename U> Condition operator!=(const U& v) const {return ne(v);}
    template<typename U> Condition operator<(const U& v) const {return lt(v);}
    template<typename U> Condition operator<=(const U& v) const {return le(v);}
    template<typename U> Condition operator>(const U& v) const {return gt(v);}
    template<typename U> Condition operator>=(const U& v) const {return ge(v);}
    Condition isNull() const {return {[](const auto& c){return resolveField(c,&MemberId<Member>::token)+" IS NULL";},{}};}
    Condition isNotNull() const {return {[](const auto& c){return resolveField(c,&MemberId<Member>::token)+" IS NOT NULL";},{}};}
    template<typename V=Type> Condition startsWith(std::string s) const {return like(std::move(s),1);}
    template<typename V=Type> Condition contains(std::string s) const {return like(std::move(s),2);}
private:
    static std::string escapeLike(const std::string& text){
        std::string out;
        for(char c:text){if(c=='%'||c=='_'||c=='!')out+='!';out+=c;}
        return out;
    }
    Condition like(std::string v,int kind) const {
        using Raw=typename optional_traits<Type>::type;
        static_assert(std::is_same_v<Raw,std::string>,"LIKE requires std::string member");
        v=escapeLike(v);
        if(kind==1)v+='%';else v='%'+v+'%';
        return {[](const auto& c){return resolveField(c,&MemberId<Member>::token)+" LIKE ? ESCAPE '!'";},{toValue(v)}};
    }
    template<typename U> Condition cmp(std::string op,const U& value) const {
        using Raw=typename optional_traits<Type>::type;
        using V=typename optional_traits<remove_cvref_t<U>>::type;
        static_assert(std::is_same_v<Raw,V> || (std::is_arithmetic_v<Raw> && std::is_arithmetic_v<V>) || (std::is_same_v<Raw,std::string> && std::is_convertible_v<const U&,std::string_view>) || std::is_same_v<remove_cvref_t<U>,std::nullptr_t>,"Mismatched field comparison types");
        auto v=toValue(value);
        if(std::holds_alternative<std::nullptr_t>(v)) {
            if(op=="=") return isNull();
            if(op=="!=") return isNotNull();
            throw Error("NULL only supports = or != comparisons");
        }
        return {[op](const auto& cols){return resolveField(cols,&MemberId<Member>::token)+" "+op+" ?";},{std::move(v)}};
    }
};
template<auto M> Field<M> field(){return {};}
struct Sort {
    std::function<std::string(const std::vector<ColumnDef>&)> sql;
};
template<auto M> Sort asc(Field<M>){return {[](const auto& c){return resolveField(c,&MemberId<M>::token)+" ASC";}};}
template<auto M> Sort desc(Field<M>){return {[](const auto& c){return resolveField(c,&MemberId<M>::token)+" DESC";}};}

template<typename T> class Query {
public:
    Query(Database& db,TableSchema<T> table):db_(&db),table_(std::move(table)){}
    Query& where(Condition expression){where_=std::move(expression);return *this;}
    Query& orderBy(Sort s){sort_=std::move(s);return *this;}
    Query& limit(std::int64_t n){if(n<0)throw Error("Negative limit");limit_=n;return *this;}
    Query& offset(std::int64_t n){if(n<0)throw Error("Negative offset");offset_=n;return *this;}
    std::vector<T> all() const {
        auto stmt=prepareSelect("*");
        std::vector<T> items;
        while(stmt->next()) {
            T object{};
            for(std::size_t i=0;i<table_.mappings().size();++i)table_.mappings()[i].set(object,stmt->value(static_cast<int>(i)));
            items.push_back(std::move(object));
        }
        return items;
    }
    std::optional<T> first() const {
        auto copy=*this;
        copy.limit(1);
        auto items=copy.all();
        if(items.empty())return std::nullopt;
        return std::move(items.front());
    }
    std::int64_t count() const {
        std::string sql="SELECT COUNT(*) FROM "+Database::quoteIdentifier(table_.name());
        if(where_)sql+=" WHERE "+where_->sql(table_.columns());
        auto stmt=db_->prepare(sql);
        bindWhere(*stmt);
        if(!stmt->next())throw Error("COUNT query returned no row");
        return stmt->int64(0);
    }
    bool exists() const {return count()>0;}
    Subscription observe(std::function<void(const std::vector<T>&)> callback,bool emitInitial=true) const {
        if(!callback)throw Error("Null Live Query callback");
        auto current=*this;
        if(emitInitial)callback(current.all());
        return db_->observeTable(table_.name(),[current,callback=std::move(callback)](const Event& event) mutable {
            if(event.kind==EventKind::Insert || event.kind==EventKind::Update || event.kind==EventKind::Delete || event.kind==EventKind::TableCreated) {
                callback(current.all());
            }
        });
    }
private:
    std::unique_ptr<Statement> prepareSelect(const std::string&) const {
        std::string sql="SELECT ";
        for(std::size_t i=0;i<table_.mappings().size();++i) {
            if(i)sql+=",";
            sql+=Database::quoteIdentifier(table_.mappings()[i].def.name);
        }
        sql+=" FROM "+Database::quoteIdentifier(table_.name());
        if(where_)sql+=" WHERE "+where_->sql(table_.columns());
        if(sort_)sql+=" ORDER BY "+sort_->sql(table_.columns());
        if(limit_)sql+=" LIMIT "+std::to_string(*limit_);
        else if(offset_)sql+=" LIMIT -1";
        if(offset_)sql+=" OFFSET "+std::to_string(*offset_);
        auto stmt=db_->prepare(sql);
        bindWhere(*stmt);
        return stmt;
    }
    void bindWhere(Statement& statement) const {
        if(where_)for(std::size_t i=0;i<where_->binds.size();++i)statement.bind(static_cast<int>(i+1),where_->binds[i]);
    }
    Database* db_;
    TableSchema<T> table_;
    std::optional<Condition> where_;
    std::optional<Sort> sort_;
    std::optional<std::int64_t> limit_,offset_;
};
}
