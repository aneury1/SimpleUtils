#include <simple_orm/simple_orm.hpp>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <string>
#include <vector>

using namespace simple_orm;
struct Person {
    std::int64_t id = 0;
    std::string name;
    std::optional<std::string> note;
    bool active = true;
    double score = 0;
    Blob data;
};
struct Parent {std::int64_t id=0;std::string label;};
struct Child {std::int64_t id=0;std::int64_t parentId=0;};
static int passed=0,failed=0;
#define CHECK(x) do {if(!(x))throw std::runtime_error(std::string("check failed: ")+ #x +" at "+__FILE__+":"+std::to_string(__LINE__));}while(false)
#define TEST(name) void test_##name(); static struct Registrar_##name{Registrar_##name(){getTests().emplace_back(#name,test_##name);}} registrar_##name;void test_##name()
using TestList=std::vector<std::pair<std::string,std::function<void()>>>;
static TestList& getTests(){static TestList list;return list;}
static TableSchema<Person> personTable(){return makeTable<Person>("people",
    column<&Person::id>("id").primaryKey().autoIncrement(),
    column<&Person::name>("name").notNull().unique(),
    column<&Person::note>("note"),
    column<&Person::active>("active").notNull().defaultValue(true),
    column<&Person::score>("score").notNull(),
    column<&Person::data>("data").notNull());}
static Person sample(std::string name){Person p;p.name=std::move(name);p.score=3.75;p.data={0,1,128,255};return p;}

TEST(static_create_and_crud){
    Database db;db.open();auto table=personTable();table.create(db);CHECK(db.hasTable("people"));
    auto dao=db.dao(table);
    auto p=sample("Ana");dao.insert(p);CHECK(p.id==1);CHECK(dao.count()==1);
    auto q=dao.findById(p.id);CHECK(q);CHECK(q->name=="Ana");CHECK(!q->note);CHECK(q->data==p.data);CHECK(q->score==p.score);
    p.name="Ana M";p.note="nota";p.active=false;CHECK(dao.update(p));
    q=dao.findById(1);CHECK(q && q->name=="Ana M" && q->note=="nota" && !q->active);
    CHECK(dao.existsById(1));CHECK(dao.removeById(1));CHECK(!dao.findById(1));CHECK(dao.count()==0);CHECK(!dao.removeById(1));
}
TEST(query_builder_and_null){
    Database db;db.open();auto table=personTable();table.create(db);auto dao=makeDao(db,table);
    auto a=sample("Alice");a.score=8;dao.insert(a);
    auto b=sample("Bob");b.note="yes";b.score=5;dao.insert(b);
    auto c=sample("Alicia");c.score=10;dao.insert(c);
    CHECK(dao.query().where(field<&Person::name>().startsWith("Ali")).count()==2);
    CHECK(dao.query().where((field<&Person::score>()>7.0) && field<&Person::note>().isNull()).count()==2);
    CHECK(dao.query().where(field<&Person::name>().contains("lic")).count()==2);
    CHECK(dao.query().where(field<&Person::note>().isNotNull()).count()==1);
    auto rows=dao.query().orderBy(desc(field<&Person::score>())).limit(1).all();
    CHECK(rows.size()==1 && rows[0].name=="Alicia");
    CHECK(dao.query().orderBy(asc(field<&Person::name>())).offset(1).limit(1).all()[0].name=="Alicia");
    CHECK(dao.query().where(field<&Person::name>()=="Bob").first()->name=="Bob");
    CHECK(dao.query().where(field<&Person::score>()<4.0).first()==std::nullopt);
}
TEST(sql_injection_is_data){
    Database db;db.open();auto table=personTable();table.create(db);auto dao=makeDao(db,table);
    auto p=sample("Robert'); DROP TABLE people;--");dao.insert(p);
    auto quote=sample("O'Connor");dao.insert(quote);
    CHECK(dao.count()==2);CHECK(db.hasTable("people"));
    CHECK(dao.query().where(field<&Person::name>()==p.name).count()==1);
    CHECK(dao.query().where(field<&Person::name>()==quote.name).count()==1);
    CHECK(dao.query().where(field<&Person::name>().contains("%_")).count()==0);
}
TEST(dynamic_crud){
    Database db;db.open();DynamicTable schema("config");
    schema.add(column<std::int64_t>("id").primaryKey().autoIncrement());
    schema.add(column<std::string>("key").notNull().unique());
    schema.add(column<std::optional<std::string>>("value"));
    schema.create(db);
    auto dao=makeDao(db,schema);
    auto id=dao.insert({{"key",toValue(std::string("host"))},{"value",toValue(std::string("localhost"))}});
    CHECK(id==1);CHECK(dao.count()==1);
    auto row=dao.findById(id);CHECK(row && fromValue<std::string>(row->at("key"))=="host");
    CHECK(dao.update(id,{{"value",toValue(std::string("example.com"))}}));
    CHECK(fromValue<std::string>(dao.findById(id)->at("value"))=="example.com");
    CHECK(dao.findAll().size()==1);CHECK(dao.removeById(id));CHECK(!dao.findById(id));
}
TEST(strict_columns_and_validation){
    Database db;db.open();DynamicTable t("typed");t.add(column<std::int64_t>("id").primaryKey());t.add(column<std::string>("name").notNull());t.create(db);
    auto dao=makeDao(db,t);
    bool bad=false;try{dao.insert({{"id",toValue(1)},{"name",toValue(123)}});}catch(const Error&){bad=true;}
    CHECK(bad);CHECK(dao.count()==0);
    bad=false;try{dao.insert({{"nonexistent",toValue(1)}});}catch(const Error&){bad=true;}CHECK(bad);
}
TEST(row_and_column_events){
    Database db;db.open();auto table=personTable();table.create(db);auto dao=makeDao(db,table);
    if(!db.supportsColumnEvents()) {
        int rows=0;
        auto sub=db.observeTable("people",[&](const Event& e){if(e.kind==EventKind::Insert)++rows;});
        auto p=sample("fallback");dao.insert(p);
        CHECK(rows==1);
        return;
    }
    std::vector<Event> all, tableEvents, nameEvents;
    auto subDb=db.observeDatabase([&](const Event&e){all.push_back(e);});
    auto subTab=db.observeTable("people",[&](const Event&e){tableEvents.push_back(e);});
    auto subName=db.observeColumn("people","name",[&](const Event&e){nameEvents.push_back(e);});
    auto p=sample("First");dao.insert(p);
    CHECK(!all.empty());CHECK(!tableEvents.empty());CHECK(nameEvents.size()==1);
    CHECK(nameEvents[0].kind==EventKind::Insert);CHECK(!nameEvents[0].oldValue);CHECK(fromValue<std::string>(*nameEvents[0].newValue)=="First");
    p.name="Second";dao.update(p);
    CHECK(nameEvents.size()==2);
    CHECK(nameEvents[1].kind==EventKind::Update);
    CHECK(fromValue<std::string>(*nameEvents[1].oldValue)=="First");
    CHECK(fromValue<std::string>(*nameEvents[1].newValue)=="Second");
    dao.removeById(p.id);
    CHECK(nameEvents.size()==3);CHECK(nameEvents.back().kind==EventKind::Delete);
    subName.unsubscribe();
    auto q=sample("Third");dao.insert(q);CHECK(nameEvents.size()==3);
}
TEST(rollback_and_commit_events){
    Database db;db.open();auto t=personTable();t.create(db);auto dao=makeDao(db,t);
    int inserted=0,commits=0,rollbacks=0;
    auto observer=db.observeTable("people",[&](const Event&e){if(e.kind==EventKind::Insert)++inserted;});
    auto observerDb=db.observeDatabase([&](const Event&e){if(e.kind==EventKind::Committed)++commits;if(e.kind==EventKind::RolledBack)++rollbacks;});
    {auto tx=db.transaction();auto p=sample("rolled-back");dao.insert(p);CHECK(inserted==0);}
    CHECK(inserted==0);CHECK(rollbacks==1);CHECK(dao.count()==0);
    {auto tx=db.transaction();auto p=sample("committed");dao.insert(p);CHECK(inserted==0);tx.commit();}
    CHECK(dao.count()==1);CHECK(inserted==1);CHECK(commits==1);
}
TEST(prepared_statements_and_raw_dml_events){
    Database db;db.open();db.execute("CREATE TABLE \"logs\"(\"id\" INTEGER PRIMARY KEY,\"message\" TEXT) STRICT");
    Subscription colSub;
    if(db.supportsColumnEvents()) colSub=db.observeColumn("logs","message",[](const Event&){});
    int changes=0;
    auto obs=db.observeTable("logs",[&](const Event&e){if(e.kind==EventKind::Insert)++changes;});
    auto stmt=db.prepare("INSERT INTO logs(message) VALUES(?)");
    stmt->bind(1,toValue("value ' 123"));CHECK(stmt->execute()==1);CHECK(changes==1);
    auto select=db.prepare("SELECT message FROM logs WHERE id=?");select->bind(1,toValue(1));CHECK(select->next());CHECK(fromValue<std::string>(select->value(0))=="value ' 123");
    CHECK(!select->next());
}
TEST(quoted_identifiers_and_bad_identifier){
    Database db;db.open();DynamicTable tab("table \"quoted\"");
    tab.add(column<std::int64_t>("id").primaryKey());tab.add(column<std::string>("select").notNull());tab.create(db);
    auto dao=makeDao(db,tab);dao.insert({{"id",toValue(8)},{"select",toValue("yes")}});
    CHECK(dao.count()==1);CHECK(fromValue<std::string>(dao.findById(8)->at("select"))=="yes");
    bool bad=false;try{Database::quoteIdentifier(std::string("a\0b",3));}catch(const Error&){bad=true;}CHECK(bad);
}
TEST(foreign_key_cascade){
    Database db;db.open();
    auto p=makeTable<Parent>("parents",column<&Parent::id>("id").primaryKey().autoIncrement(),column<&Parent::label>("label").notNull());
    auto c=makeTable<Child>("children",column<&Child::id>("id").primaryKey().autoIncrement(),column<&Child::parentId>("parent_id").notNull().references("parents","id").onDeleteCascade());
    p.create(db);c.create(db);
    auto parents=makeDao(db,p);auto children=makeDao(db,c);
    Parent parent;parent.label="parent";parents.insert(parent);
    Child child;child.parentId=parent.id;children.insert(child);
    CHECK(children.count()==1);parents.removeById(parent.id);CHECK(children.count()==0);
}
TEST(schema_events_and_unsubscribe){
    Database db;db.open();int created=0,dropped=0;
    auto listener=db.observeDatabase([&](const Event& e){if(e.kind==EventKind::TableCreated)++created;if(e.kind==EventKind::TableDropped)++dropped;});
    auto t=personTable();t.create(db);t.create(db);CHECK(created==1);
    db.dropTableIfExists("people");db.dropTableIfExists("people");CHECK(dropped==1);
    listener.unsubscribe();t.create(db);CHECK(created==1);
}
TEST(transaction_constraint_failure){
    Database db;db.open();auto t=personTable();t.create(db);auto dao=makeDao(db,t);
    auto observer=db.observeTable("people",[](const Event&){});
    auto tx=db.transaction();
    auto p=sample("same");dao.insert(p);
    auto q=sample("same");bool conflict=false;
    try {dao.insert(q);}catch(const Error&){conflict=true;}
    CHECK(conflict);
    tx.commit();
    CHECK(dao.count()==1);
}
TEST(nullable_and_delete_all){
    Database db;db.open();DynamicTable t("kv");t.add(column<std::int64_t>("id").primaryKey().autoIncrement());t.add(column<std::optional<std::string>>("value"));t.create(db);
    auto dao=makeDao(db,t);dao.insert({{"value",nullptr}});dao.insert({{"value",toValue(std::string("ok"))}});
    CHECK(dao.count()==2);CHECK(std::holds_alternative<std::nullptr_t>(dao.findById(1)->at("value")));
    CHECK(dao.deleteAll()==2);CHECK(dao.count()==0);
}
TEST(live_query_reacts_to_commits){
    Database db;db.open();auto table=personTable();table.create(db);auto dao=db.dao(table);
    std::vector<std::size_t> counts;
    auto observe=dao.query().where(field<&Person::active>()==true).observe(
        [&](const std::vector<Person>& rows){counts.push_back(rows.size());});
    CHECK(counts.size()==1 && counts.back()==0);
    auto a=sample("active");dao.insert(a);CHECK(counts.back()==1);
    {
        auto tx=db.transaction();auto b=sample("reverted");dao.insert(b);
        CHECK(counts.back()==1);
        tx.rollback();
    }
    CHECK(counts.back()==1);
    a.active=false;dao.update(a);CHECK(counts.back()==0);
    observe.unsubscribe();
}
TEST(empty_blob_roundtrip){
    Database db;db.open();auto table=personTable();table.create(db);auto dao=db.dao(table);
    auto p=sample("empty");p.data={};dao.insert(p);
    auto found=dao.findById(p.id);CHECK(found.has_value());CHECK(found->data.empty());
}
TEST(observer_can_write){
    Database db;db.open();auto table=personTable();table.create(db);auto dao=db.dao(table);
    int calls=0;
    auto sub=db.observeTable("people",[&](const Event& e){
        if(e.kind==EventKind::Insert && ++calls==1){auto p=sample("second");dao.insert(p);}
    });
    auto p=sample("first");dao.insert(p);
    CHECK(dao.count()==2);CHECK(calls==2);
}
TEST(schema_group_create_missing){
    Database db;db.open();
    DynamicTable config("config");config.add(column<std::int64_t>("id").primaryKey());
    Schema schema("app",1);
    schema.add(personTable()).add(config);
    CHECK(schema.name()=="app" && schema.version()==1);
    schema.createMissing(db);
    CHECK(db.hasTable("people") && db.hasTable("config"));
    schema.createMissing(db);
    auto dao=db.dao(config);dao.insert({{"id",toValue(42)}});
    CHECK(dao.count()==1);
}
TEST(bad_schema_and_multi_sql){
    Database db;db.open();bool caught=false;
    try {db.execute("CREATE TABLE a(x INTEGER); CREATE TABLE b(x INTEGER)");}catch(const Error&){caught=true;}CHECK(caught);
    DynamicTable bad("bad");bad.add(column<std::string>("same"));bad.add(column<std::string>("same"));caught=false;
    try {bad.create(db);}catch(const Error&){caught=true;}CHECK(caught);
}
int main() {
    for(auto& [name,run]:getTests()) {
        try{run();std::cout<<"[PASS] "<<name<<"\n";++passed;}
        catch(const std::exception& e){std::cerr<<"[FAIL] "<<name<<": "<<e.what()<<"\n";++failed;}
    }
    std::cout<<passed<<" passed, "<<failed<<" failed\n";
    return failed==0?0:1;
}
