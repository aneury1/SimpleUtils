#include <iostream>


template<typename D>
struct link_node{
   D value;
   link_node *prev;
   link_node *next;

   link_node(D v){
    prev = nullptr;
    next = nullptr;
    value = v;
   }

};



template<typename T>
class link_list{
   typedef link_node<T> node;
   node* head;

public:
   link_list(){
    head = nullptr;
   }


   link_list<T>& insert(T v){
    
    if(!head){
      head = new link_node<T>(v);
      head->value = v;
      return (*this);
    }
    node* cpy = head;
    while(cpy->next)
       cpy = cpy->next;

    node *new_v = new node(v);
    new_v->next =head;
    head = new_v; 
    
    return (*this);
   } 
 
   void print(){
       node *cpy = head;
       while(cpy){
         std::cout << cpy->value <<"\n";
         cpy = cpy->next;
       }
   }


};

int main(){
    
    link_list<int> li;
    for(int i =0;i<10;i++)
       li.insert(i);
    li.print();
    return 0;
}


