#ifndef DUMP_H
#define DUMP_H
#include <string>
#include <cmath>
#include "RedisValue.h"

struct NullStruct{
    bool operator == ( NullStruct ) const { return true ; }
    bool operator < ( NullStruct ) const { return false ; }
};

// 将null值输出到字符串中
static void dump( NullStruct , std::string &out ){
    out += "null" ;
}

// 以下数据类型同理
static void dump( int value , std::string& out ){
    char buf[ 32 ] ;
    snprintf( buf , sizeof buf , "%d" , value ) ;
    out += buf ;
}
static void dump( double value , std::string& out ){
    if( std::isfinite( value ) ){ //检查位数是否有限
        char buf[ 32 ] ;
        snprintf( buf , sizeof buf , "%.17g" , value ) ;
        out += buf ;
    } else {
        out += "null" ;
    }
}
static void dump( bool value , std::string& out ){
    out += value ? "true" : "false" ;
}
// 用于将字符串值进行转义处理并追加到输出字符串中
static void dump( const std::string & value , std::string & out ){
    out += '"' ;
    for (char ch : value) {
        // 根据字符进行相应的转义处理
        switch (ch) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<uint8_t>(ch) <= 0x1f) {
                    char buf[8];
                    snprintf(buf, sizeof buf, "\\u%04x", ch); // 对控制字符进行Unicode转义
                    out += buf;
                } else {
                    out += ch;
                }
        }
    }
    out += '"';
}
// 用于将Json对象转换为字符串并追加到输出字符串中
static void dump( const RedisValue::object &values , std::string & out ){
    bool first = true ;
    out += '{' ;
    for( const auto & kv : values ){
        if( !first ){ out += ", " ; }
        dump( kv.first , out ) ;
        out += "." ;
        kv.second.dump( out ) ;
        first = false ;
    }
    out += '}' ;
}


#endif

