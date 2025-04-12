#include <string.h>
#include <vector>
#include <string>
#include <stdint.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

struct Buffer{
   std::vector<uint8_t> data;
      
   void setData(uint8_t* buffer, uint32_t size);
   void setData(char* buffer, uint32_t size);
   void setData(std::vector<uint8_t> buffer);
   void Mid(std::vector<uint8_t>& res, int start, int end);
   void Mid(std::string& res, int start, int end);
   
   void insertInto(std::vector<uint8_t> buffer, int where);
   void insertInto(Buffer buffer, int where);
   void insertInto(std::string buffer, int where);
   void insertInto(char* buffer, int where);
   
   void append(uint8_t data);
   
   uint8_t operator[](size_t s){
      return data[s];
   }

   void removeFrom(int where, int count); 
  
   typedef void (*WorkerFunction)(Buffer *, unsigned char, int pos);
   
   void iterateOver(WorkerFunction worker);

   const uint32_t size()const{return data.size();}

};


struct ByteUtils{

static std::string toHexString(Buffer buffer){
    std::ostringstream oss;
    for (size_t i = 0; i < buffer.size(); ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]);
    }
    return oss.str();
}
static Buffer toBuffer(uint16_t b);

};

Buffer ByteUtils::toBuffer(uint16_t b){
    Buffer ret;
    ret.append(b>>8);
    ret.append(b&0x00FF);
    return ret;
}

void Buffer::append(uint8_t data)
{
    this->data.emplace_back(data);
}



void Buffer::setData(uint8_t* buffer, uint32_t size){
    data.clear();
    data.reserve(size+16);
    for(int32_t i =0;i<size;i++){
        data.emplace_back(buffer[i]);
    }
}

void Buffer::setData(char* buffer, uint32_t size){
    data.clear();
    data.reserve(size+16);
    for(int32_t i =0;i<size;i++){
        data.emplace_back((uint8_t)buffer[i]);
    }
}

void Buffer::setData(std::vector<uint8_t> buffer){
    setData(buffer.data(), buffer.size());
}

void Buffer::Mid(std::vector<uint8_t>& res, int start, int end){
    if((end>start)&&(end-start < data.size())){
       int cnt = end-start;
       for(int i=0;i<cnt;i++){
          res.emplace_back(data[i+start]);
       }
    }
}

void Buffer::Mid(std::string& res, int start, int end){
    if((end>start)){
       int cnt = end-start;
       for(int i=0;i<cnt;i++){
          res+=data[i+start];
       }
       res[cnt]={0x00};
    }
}

void Buffer::insertInto(std::vector<uint8_t> buffer, int where)
{
    if (where < 0 || where > static_cast<int>(data.size())) {
        std::cerr << "Invalid insert position\n";
        return;
    }

    data.insert(data.begin() + where, buffer.begin(), buffer.end());
}

void Buffer::insertInto(Buffer buffer, int where)
{
    this->insertInto(buffer.data, where);    
}

void Buffer::insertInto(std::string buffer, int where)
{
    Buffer data;
    data.setData(buffer.data(), buffer.size());
    this->insertInto(data,where);
}
   
void Buffer::insertInto(char* buffer, int where)
{
    Buffer data;
    data.setData(buffer,strlen(buffer));
    this->insertInto(data,where);
}
void Buffer::removeFrom(int where, int count) 
{
    if (where < 0 || where >= static_cast<int>(data.size())) {
        std::cerr << "Invalid start position\n";
        return;
    }

    // Clamp the end to the size of the vector
    int end = std::min(where + count, static_cast<int>(data.size()));

    data.erase(data.begin() + where, data.begin() + end);
}

void Buffer::iterateOver(WorkerFunction worker)
{
   for(int i=0;i<data.size();i++){
     uint8_t p = data[i];
     worker(this, p, i);
   }
}

 