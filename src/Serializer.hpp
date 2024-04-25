#ifndef SERIALIZER_HPP
#define SERIALIZER_HPP

#include <vector>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
using namespace std;


/*
    采用vector<char> 因为是一个动态数组，可以存储任意数量的 char 元素，、
    并且可以动态地增加或减少元素。这使得 std::vector<char> 非常适合用作字节流的缓冲区

    然而，std::vector<char> 并没有提供一些字节流操作所需要的功能，
    例如移动当前位置、查找特定的字节、检查是否已经到达末尾等。
    因此，StreamBuffer 类继承自 std::vector<char>，并添加了这些功能。
*/

class StreamBuffer : public vector<char>
{
public:
    StreamBuffer():m_curpos(0){}

    StreamBuffer(const char* in, size_t len)
    {
        m_curpos = 0;
        insert(begin(), in, in+len);
    }

    ~StreamBuffer(){};

    void reset(){ m_curpos = 0; }

    const char* data(){ return &(*this)[0]; } //获取缓冲区的数据

    const char* current(){ 
        return&(*this)[m_curpos]; 
    } // 获取当前位置的数据

    void offset(int offset){ m_curpos += offset; } //移动当前位置

    bool is_eof(){ return m_curpos >= size(); } //检查是否已经到达末尾

    void input(const char* in, size_t len) //输入字符数组
    {
        insert(end(), in, in+len);
    }

    int findc(char c) //在缓冲区中查找特定的字节
    {
       iterator itr = find(begin()+m_curpos, end(), c);		
        if (itr != end())
        {
            return itr - (begin()+m_curpos);
        }
        return -1;
    }

private:
    unsigned int m_curpos; //当前字节流的位置

};

/*
   序列号和反序列号的类
   用于将数据序列化为字节流，或者将字节流反序列化为数据

    存储数据：写入数据长度和数据本身
    读取数据：读取数据长度，然后读取数据本身
*/

class Serializer
{
    
public:
    enum ByteOrder
    {
        BigEndian = 0,
        LittleEndian = 1
    };

    Serializer(){ m_byteorder = LittleEndian; }

    Serializer(StreamBuffer dev, int byteorder = LittleEndian)
    {
        m_byteorder = byteorder;
        m_iodevice = dev;
    }

    void reset(){ 
        m_iodevice.reset();
    }

    int size(){
		return m_iodevice.size();
	}


    void skip_raw_date(int k){
        m_iodevice.offset(k);
    }

    const char* data(){
        return m_iodevice.data();
    }
    void byte_orser(char* in, int len){
		if (m_byteorder == BigEndian){
			reverse(in, in+len); //大端的化直接反转 
		}
	}
   
    void write_raw_data(char* in, int len){\
		m_iodevice.input(in, len);
		m_iodevice.offset(len);
	}

    const char* current(){
		return m_iodevice.current();
	}

    /**
	 * @brief 清空序列化器中的数据。
	 */
	void clear(){
		m_iodevice.clear();
		reset();
	}
        
    template<typename T>
	void output_type(T& t);

    template<typename T>
	void input_type(T t);
   
    template<typename T>
    Serializer &operator >>(T& i){
        output_type(i); 
        return *this;
    }

    
    template<typename T>
	Serializer &operator << (T i){
		input_type(i);
		return *this;
	}    

private:
    int m_byteorder; //字节序
    StreamBuffer m_iodevice; //字节流缓冲区
    

};  

template<typename T>
inline void Serializer::output_type(T& t)
{
	int len = sizeof(T);
	char* d = new char[len];
	if (!m_iodevice.is_eof()){
		memcpy(d, m_iodevice.current(), len);
		m_iodevice.offset(len);
		byte_orser(d, len);
		t = *reinterpret_cast<T*>(&d[0]);
	}
	delete [] d;
}
template<>
inline void Serializer::output_type(std::string& in){
    int marklen = sizeof(uint16_t); //读取长度

    char* d = new char[marklen];
    memcpy(d, m_iodevice.current(), marklen); 
    int len = *reinterpret_cast<uint16_t*>(&d[0]); 
    byte_orser(d, marklen);
    m_iodevice.offset(marklen); 
    delete d;
    if(len == 0) return;

    in.insert(in.begin(), m_iodevice.current(), m_iodevice.current() + len); //输入到in中
	m_iodevice.offset(len);
}


template<typename T>
inline void Serializer::input_type(T t)
{
    
	int len = sizeof(T); 
	char* d = new char[len];   
	const char* p = reinterpret_cast<const char*>(&t);
	memcpy(d, p, len); 
	byte_orser(d, len); 
	m_iodevice.input(d, len); 
	delete [] d;
}


template<>
inline void Serializer::input_type(std::string in)
{
    
	uint16_t len = in.size(); 
	char* p = reinterpret_cast<char*>(&len);
	byte_orser(p, sizeof(uint16_t));
	m_iodevice.input(p, sizeof(uint16_t)); 
	if (len == 0) return;

    
	char* d = new char[len];
	memcpy(d, in.c_str(), len); 
	m_iodevice.input(d, len);
	delete [] d;
}


template<>
inline void Serializer::input_type(const char* in)
{
	input_type<std::string>(std::string(in)); 
}

#endif


