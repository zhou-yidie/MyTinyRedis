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
    /**
	 * @brief 将指定数据写入序列化器。
	 * @param in 输入的字符数组。
	 * @param len 输入字符数组的长度。
	 */
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
        /**
	 * @brief 输出指定类型的数据。
	 * @tparam T 要输出的数据类型。
	 * @param t 要输出的数据。
	 */
    template<typename T>
	void output_type(T& t);

    /**
	 * @brief 输入指定类型的数据。
	 * @tparam T 要输入的数据类型。
	 * @param t 要输入的数据。
	 */
    template<typename T>
	void input_type(T t);
    /**
	 * @brief 重载运算符>>，用于从序列化器中读取数据。
	 * @tparam T 要读取的数据类型。
	 * @param i 用于存储读取结果的变量。
	 * @return 当前序列化器对象的引用。
	 */
    template<typename T>
    Serializer &operator >>(T& i){
        output_type(i); 
        return *this;
    }

    /**
	 * @brief 重载运算符<<，用于向序列化器中写入数据。
	 * @tparam T 要写入的数据类型。
	 * @param i 要写入的数据。
	 * @return 当前序列化器对象的引用。
	 */
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
    memcpy(d, m_iodevice.current(), marklen); //将字节流的数据的两个字节拷贝到d中
    int len = *reinterpret_cast<uint16_t*>(&d[0]);  //取出长度
    byte_orser(d, marklen);
    m_iodevice.offset(marklen); //将字节流的位置向后移动两个字节
    delete d;
    if(len == 0) return;

    in.insert(in.begin(), m_iodevice.current(), m_iodevice.current() + len); //输入到in中
	m_iodevice.offset(len);
}


template<typename T>
inline void Serializer::input_type(T t)
{
    //求出放入缓存区数据的长度
	int len = sizeof(T); 
	char* d = new char[len];   
	const char* p = reinterpret_cast<const char*>(&t);
	memcpy(d, p, len); 
	byte_orser(d, len); 
	m_iodevice.input(d, len); //将d中的数据输入到字节流中
	delete [] d;
}

/**
 * @brief 输入字符串到序列化器中。
 * @param in 要输入的字符串。
 */
template<>
inline void Serializer::input_type(std::string in)
{
    //先将字符串的长度输入到字节流中
	uint16_t len = in.size(); 
	char* p = reinterpret_cast<char*>(&len);
	byte_orser(p, sizeof(uint16_t));
	m_iodevice.input(p, sizeof(uint16_t)); 
	if (len == 0) return;

    //再将字符串的数据输入到字节流中
	char* d = new char[len];
	memcpy(d, in.c_str(), len); //将字符串的数据拷贝到d中
	m_iodevice.input(d, len);
	delete [] d;
}

/**
 * @brief Inputs a null-terminated string into the serializer.
 * @param in The null-terminated string to input.
 */
template<>
inline void Serializer::input_type(const char* in)
{
	input_type<std::string>(std::string(in)); //调用input_type<std::string>函数
}

#endif

/*
Q1:Serializer类可以直接继承StreamBuffer 类吗，如果可以，继承和不继承各有什么优劣?

A1:Serializer类是否可以直接继承StreamBuffer类，取决于你的具体使用场景和设计目标。在考虑是否要使用继承，你需要权衡各种因素，包括代码复杂性、易用性、性能和灵活性等。
接下来我将列出继承和不继承的一些可能的优点和缺点。
如果Serializer继承自StreamBuffer：
优点：
使用继承可以简化Serializer类的接口，因为Serializer可以直接让用户访问StreamBuffer的方法。
如果StreamBuffer提供了一些可以在Serializer中直接使用或覆写的方法，那么继承可能帮助减少代码量。
缺点：
继承可能会破坏缺乏逻辑关系的类的封装性。在这个例子中，如果StreamBuffer有一些与序列化不直接相关的功能，那么Serializer也会有这些功能，这可能会对使用Serializer的人造成混淆。
如果StreamBuffer的部分实现细节改变，Serializer可能需要相应地更改，这增加了代码维护的复杂性。
如果Serializer不继承自StreamBuffer，而是在内部维护一个StreamBuffer的实例：
优点：
这种方法可以更好地保留类的封装性，使得Serializer的用户只能访问到与序列化直接相关的方法。
它也提供了更好的灵活性，如果未来需要更改缓冲策略，你可以更轻易地替换StreamBuffer。
缺点：
不使用继承可能需要编写更多的代码，因为你可能需要编写一些方法来包装StreamBuffer的功能。
如果Serializer和StreamBuffer有很多相互操作，那么在Serializer中维护一个StreamBuffer实例可能会使得代码复杂性增加。
总的来说，无论是选择继承还是组合，你都需要根据具体的使用场景和设计目标来权衡。另外，一般建议优先使用组合而不是继承，因为组合提供了更好的封装性和灵活性。

P1：此段代码量不大，而且为了更好的封装性和灵活性，理应不直接继承，否则在代码量增大后，可能会出现一些Serializer类对象使用StreamBuffer 类的函数而发生错误

Q2:为什么对于output_type模板函数需要单独把传入参数为string类型时单独讨论，其中有什么讲究吗

A2:在C++模板编程中，模板特化允许我们对模板进行局部或完全特化，当模板参数满足一定条件时改变模板的行为。这个机制常用于处理一些特殊类型的特殊情况。
在你给出的例子中，为std::string类型参数特殊处理的原因在于std::string的内存布局与其他内置类型和标准容器是不同的。大多数情况下，像int、double这样的内置类型，或者像std::array<T, N>这样的标准容器，他们的内存布局是连续的，就像一个数组。这意味着我们可以将数据的内存块直接当作字节流进行读写。
然而并非所有类型都具有这种特性，例如std::string。在标准的C++实现中，std::string的内部通常由一个字符指针和一些描述字符串状态的元数据（如大小和容量）组成，字符指针指向一个动态分配的字符数组，用于存储字符串的内容。由于这种设计，我们不能直接读取或写入std::string的内存块，因为这样做不能正确地得到字符串的内容。
因此，对于std::string类型，我们需要特别处理。常见的处理方式是先读取或写入字符串的长度，然后再读取或写入字符串的内容。你给出的代码段就是这么做的。

Q3:解释 reinterpret_cast 的用法

A3:reinterpret_cast 是C++语言中的一种类型转换运算符。它可以在任何指针（或引用）类型之间进行转换，即便它们不是相关的。也就是说，reinterpret_cast可以在任意类型的指针和引用类型之间进行转换，包括将指针的值直接转换为一个整数。此外，它也可以在函数指针类型之间进行转换。
具体使用方法：
cpp
reinterpret_cast <new_type> (expression)
在这里，new_type 是你想要转换到的新的数据类型，expression 是你想要转换的数据或者表达式。
举个例子：
cpp
int* ptr = new int(123);
char* ch = reinterpret_cast<char*>(ptr);
在这个例子中，ptr是一个指向整数的指针，我们使用reinterpret_cast将其转换为一个指向字符的指针。
需要注意的是，reinterpret_cast是非常危险的，因为它简单地把源对象的位模式重新解释为目标类型，而并不会进行任何的类型检查或者格式转换。这个运算符只有在你完全确定你知道自己在做什么，并且这确实是你唯一的选择的时候才使用。

*/
