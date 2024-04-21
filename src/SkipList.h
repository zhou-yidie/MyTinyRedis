#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <cstring>
#include <mutex>
#include <fstream>
#include <random>
#include "global.h"
#include "RedisValue/RedisValue.h"
#define MAX_SKIP_LIST_LEVEL 32
#define PROBABILITY_FACTORY 0.25
#define DELIMITER ":"
#define SAVE_PATH "data_file"

// 跳表节点
template <typename Key , typename Value >
class SkipListNode{
public:
    Key key ;
    Value value ;
    std::vector< std::shared_ptr< SkipListNode< Key , Value > > > forward ;
    SkipListNode( Key key , Value value , int maxLevel = MAX_SKIP_LIST_LEVEL ):
    key( key ) , value( value ) , forward( maxLevel , nullptr ){}
};


// 跳表结构和基本接口
template< typename Key , typename Value >
class SkipList{
private:
    int currentLevel ;
    std::shared_ptr< SkipListNode< Key , Value > > head ;
    int elementNumber = 0 ;
    std::mutex mutex ;
    std::ofstream writeFile ;
    std::ifstream readFile ;
    std::mt19937 generator{ std::random_device{}() } ;
    std::uniform_real_distribution< double > distribution ;

    int randomLevel() ;
    bool parseString( const std::string& line , std::string& key , std::string& value ) ;
    bool isVaildString( const std::string& line ) ;

public:
    SkipList() ;
    ~SkipList() ;
    bool addItem( const Key& key , const Value& value ) ;
    bool modifyItem( const Key& key , const Value& value ) ;
    bool deleteItem( const Key& key ) ;
    std::shared_ptr< SkipListNode< Key , Value > > searchItem( const Key& key ) ;
    int getCurrentLevel(){ return currentLevel ; }
    std::shared_ptr< SkipListNode< Key , Value > > getHead(){ return head ; }
    int size() ;
    void printList() ;
    void dumpFile( std::string save_path ) ;
    void loadFile( std::string load_path ) ;



};



#endif
