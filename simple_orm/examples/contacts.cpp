#include <simple_orm/simple_orm.hpp>
#include <iostream>
using namespace simple_orm;
struct Contact {std::int64_t id=0;std::string name;std::optional<std::string> phone;bool active=true;};
int main() {
    try {
        Database db({"contacts.sqlite3",true,5000,JournalMode::WAL});
        db.open();
        auto contacts=makeTable<Contact>("contacts",
            column<&Contact::id>("id").primaryKey().autoIncrement(),
            column<&Contact::name>("name").notNull().unique(),
            column<&Contact::phone>("phone"),
            column<&Contact::active>("active").notNull().defaultValue(true));
        contacts.create(db);
        auto watcher=db.observeTable("contacts",[](const Event& e){
            std::cout<<"Contact event "<<static_cast<int>(e.kind)<<" row "<<e.rowId<<"\n";
        });
        auto nameWatcher=db.observeColumn("contacts","name",[](const Event& e){
            std::cout<<"Changed column "<<e.column<<"\n";
        });
        auto dao=makeDao(db,contacts);
        Contact c; c.name="Person " + std::to_string(dao.count()+1);
        dao.insert(c);
        c.phone="809-555-0188";
        dao.update(c);
        for(const auto& person:dao.query().where(field<&Contact::active>()==true).orderBy(asc(field<&Contact::name>())).all())
            std::cout<<person.id<<" "<<person.name<<"\n";
    } catch(const std::exception& ex) {std::cerr<<ex.what()<<"\n";return 1;}
}
