#ifndef REDISVALUE_H 
#define REDISVALUE_H

#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <memory>
#include <limits>
#include <initializer_list>

class RedisValueType ; // 声明RedisValueType类 、

class RedisValue{
private:
    std::shared_ptr< RedisValueType > redisValue ; // 指向实际存储的RedisValueType类型的智能指针 、
public:
    // Redis 中支持的数据类型
    enum Type{
        NUL , NUMBER , BOOL , STRING , ARRAY , OBJECT
    };
    // 用typedef重命名 数组 和 对象 类型
    typedef std::vector< RedisValue > array ;
    typedef std::map< std::string , RedisValue > object ;

    RedisValue() noexcept ;
    RedisValue( std::nullptr_t ) noexcept ;
    RedisValue( const char* value ) ;
    RedisValue( const array&value) ;
    RedisValue( array&& values) ;
    RedisValue( const object& values) ;
    RedisValue( object && values) ;
    RedisValue( const std::string& value) ;
    RedisValue( std::string&& value ) ;

    // 从具有 toJson 成员函数的类实例构造 RedisValue
    template<class T , class = decltype( &T::toJson ) >
    RedisValue( const T& t ) : RedisValue( t.toJson() ){}

    // 从支持 begin/end 迭代器的容器构造 RedisValue 对象
    template<class M, typename std::enable_if<
            std::is_constructible< std::string , decltype( std::declval< M >().begin()->first )>::value
            && std::is_constructible< RedisValue , decltype( std::declval< M >().begin()->second ) >::value ,
            int >::type = 0 >
    RedisValue( const M & m ) : RedisValue( object( m.begin() , m.end() ) ) {}

    template<class V, typename std::enable_if<
            std::is_constructible< RedisValue , decltype( *std::declval< V >().begin() ) >::value ,
            int >::type = 0>
    RedisValue( const V & v ) : RedisValue( array( v.begin() , v.end() ) ) {}
    // 禁止 void* 构造
    RedisValue( void* ) = delete ;

    // 判断函数类型
    Type type() const;
    bool isNull() const{ return type() == NUL ; }
    bool isNumber() const { return type() == NUMBER ; }
    bool isBoolean() const { return type() == BOOL ; }
    bool isString() const { return type() == STRING ; }
    bool isArray() const { return type() == ARRAY ; }
    bool isObject() const { return type() == OBJECT ; }

    // 获取值的函数
    std::string& stringValue() ;
    array& arrayItems() ;
    object& objectItems() ;

    // 重载 [] 操作符，用于访问数组元素和对象成员
    RedisValue & operator[] (size_t i) ;
    RedisValue & operator[] (const std::string &key) ;

    // 序列化函数
    void dump( std::string &out ) const ;
    std::string dump() const{
        std::string out ;
        dump( out ) ;
        return out ;
    }

    // 解析 JSON 文本的静态函数
    static RedisValue parse( const std::string&in, std::string& err );
    static RedisValue parse( const char* in, std::string& err );

    // 解析多个 JSON 值的静态函数
    static std::vector< RedisValue > parseMulti(
            const std::string&in,
            std::string::size_type & parserStopPos,
            std::string& err
    );
    static std::vector< RedisValue > parseMulti(
            const std::string & in,
            std::string & err
    );

    // 重载比较运算符
    bool operator == (const RedisValue &rhs) const;
    bool operator < (const RedisValue &rhs) const;
    bool operator != (const RedisValue &rhs) const {return !(*this==rhs);}
    bool operator <= (const RedisValue &rhs) const {return !(rhs<*this);}
    bool operator > (const RedisValue &rhs) const { return (rhs<*this);}
    bool operator >= (const RedisValue &rhs) const {return !(*this<rhs);}

    // 检查 RedisValue 对象是否符合指定的形状
    typedef std::initializer_list< std::pair< std::string , Type > > shape ;
    bool hasShape( const shape &types , std::string &err ) ;
};


#endif
