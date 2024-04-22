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
#define PROBABILITY_FACTOR 0.25
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

template<typename Key,typename Value>
SkipList<Key,Value>::SkipList()
        :currentLevel(0),distribution(0, 1){
    Key key;
    Value value;
    this->head=std::make_shared<SkipListNode<Key,Value>>(key,value); //初始化头节点,层数为最大层数
}

template< typename Key , typename Value >
SkipList< Key , Value >::~SkipList(){
    if( this->readFile ){
        readFile.close() ;
    }
    if( this->writeFile ){
        writeFile.close() ;
    }
}

//随机生成新节点的层数
template< typename Key , typename Value >
int SkipList< Key , Value >::randomLevel() {
    int level = 1 ;
    while( distribution( generator ) < PROBABILITY_FACTOR
    && level < MAX_SKIP_LIST_LEVEL ){
        level ++ ;
    }
    return level ;
}
// 解析字符串
template< typename  Key , typename  Value >
bool SkipList< Key , Value >::parseString(const std::string &line, std::string &key, std::string &value) {
    if( !isVaildString( line ) ){
        return false ;
    }
    auto pos = line.find( DELIMITER ) ;
    key = line.substr( 0 , pos ) ;
    value = line.substr( pos + 1 ) ;
    return true ;
}
// 字符串是否合法
template< typename Key , typename Value >
bool SkipList< Key , Value >::isVaildString(const std::string &line) {
    if( line.empty() ){
        return false ;
    }
    // line.find(":") ;
    if( line.find( DELIMITER ) == std::string::npos ){
        return false ;
    }
    return true ;
}

template<typename Key,typename Value>
int SkipList<Key,Value>::size(){
    mutex.lock();
    int ret=this->elementNumber;
    mutex.unlock();
    return ret;
}

template< typename Key , typename Value >
bool SkipList< Key , Value >::addItem(const Key &key, const Value &value) {
    mutex.lock() ;
    auto currentNode = this->head ;
    std::vector< std::shared_ptr< SkipListNode< Key , Value > > > update( MAX_SKIP_LIST_LEVEL , head ) ;
    for( int i = currentLevel - 1 ; i >= 0 ; i -- ){
        while( currentNode->forward[ i ] && currentNode->forward[ i ]->key < key ){
            currentNode = currentNode->forward[ i ] ;
        }
        update[ i ] = currentNode ;
    }
    int newLevel = this->randomLevel() ;
    currentLevel = std::max( newLevel , currentLevel ) ;
    std::shared_ptr< SkipListNode< Key , Value > > newNode = std::make_shared< SkipListNode< Key , Value > >( key , value , newLevel ) ;
    for( int i = 0 ; i < newLevel ; i++ ){
        newNode->forward[ i ] = update[ i ]->forward[ i ] ;
        update[ i ]->forward[ i ] = newNode ;
    }
    elementNumber ++ ;
    mutex.unlock() ;
    return true ;
}

template<typename Key,typename Value>
bool SkipList<Key,Value>::deleteItem(const Key& key){
    mutex.lock();
    std::shared_ptr<SkipListNode<Key,Value>> currentNode=this->head;
    std::vector<std::shared_ptr<SkipListNode<Key,Value>>>update(MAX_SKIP_LIST_LEVEL,head);
    for(int i=currentLevel-1;i>=0;i--){
        while(currentNode->forward[i]&&currentNode->forward[i]->key<key){
            currentNode=currentNode->forward[i];
        }
        update[i]=currentNode;
    }
    currentNode=currentNode->forward[0];
    if(!currentNode||currentNode->key!=key){
        mutex.unlock();
        return false;
    }
    for(int i=0;i<currentLevel;i++){
        if(update[i]->forward[i]!=currentNode){
            break;
        }
        update[i]->forward[i]=currentNode->forward[i];
    }
    currentNode.reset();
    while(currentLevel>1&&head->forward[currentLevel-1]==nullptr){
        currentLevel--;
    }
    elementNumber--;
    mutex.unlock();
    return true;
}

template< typename Key , typename Value >
std::shared_ptr< SkipListNode< Key , Value > > SkipList< Key , Value >::searchItem(const Key &key) {
    mutex.lock() ;
    std::shared_ptr< SkipListNode< Key , Value > > currentNode = this->head ;
    if( !currentNode ){
        mutex.unlock() ;
        return nullptr ;
    }
    for( int i = currentLevel - 1 ; i >= 0 ; i -- ){
        while( currentNode->forward[ i ] != nullptr && currentNode->forward[ i ]->key < key ){
            currentNode = currentNode->forward[ i ] ;
        }
    }
    currentNode = currentNode->forward[ 0 ] ;
    if( currentNode && currentNode->key == key ){
        mutex.unlock() ;
        return currentNode ;
    }
    mutex.unlock() ;
    return nullptr ;
}

template< typename Key , typename Value >
bool SkipList< Key , Value >::modifyItem(const Key &key, const Value &value) {
    std::shared_ptr< SkipListNode< Key , Value > > targetNode = this->searchItem( key ) ;
    mutex.lock() ;
    if( targetNode == nullptr ){
        mutex.unlock() ;
        return false ;
    }
    targetNode->value = value ;
    mutex.unlock() ;
    return true ;
}

template< typename Key , typename Value >
void SkipList< Key , Value >::printList() {
    mutex.lock() ;
    for( int i = currentLevel ; i >= 0 ; i-- ){
        auto node = this->head->forward[ i ] ;
        std::cout << "Level = " << i + 1 << " : " ;
        while( node != nullptr ){
            std::cout << node->key << " : " << node->value << "; " ;
            node = node->forward[ i ] ;
        }
        std::cout << '\n' ;
    }
    mutex.unlock() ;
}

template< typename  Key , typename Value >
void SkipList< Key , Value >::dumpFile(std::string save_path) {
    mutex.lock() ;
    writeFile.open( save_path ) ;
    auto node = this->head->forward[ 0 ] ;
    while( node != nullptr ){
        writeFile<<node->key<<DELIMITER<< node->value.dump() << '\n' ;\
        node = node->forward[ 0 ] ;
    }
    writeFile.flush() ;
    writeFile.close() ;
    mutex.unlock() ;
}
template< typename  Key , typename Value >
void SkipList< Key , Value >::loadFile(std::string load_path) {
    readFile.open( load_path ) ;
    if( !readFile.is_open() ){
        mutex.unlock() ; return ;
    }
    std::string line , key , value , err ;
    while( std::getline( readFile , line ) ){
        if(parseString( line , key , value ) ){
            addItem( key , RedisValue::parse( value , err ) ) ;
        }
    }
    readFile.close() ;
}

#endif
