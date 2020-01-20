#ifndef INSERTION_ORDERED_MAP_H
#define INSERTION_ORDERED_MAP_H

#include <iostream>
#include <list>
#include <unordered_map>
#include <memory>

#include <vector>
#include <cassert>
using namespace std;
class lookup_error : public std::exception
{
    const char * what () const noexcept override
    {
        return "lookup_error";
    }
};

template <class K, class V, class Hash = std::hash<K>>
class insertion_ordered_map {
private:
    /* creates new map with actual objects (list::iterator)
     * need to have current attr pairs
     */
    shared_ptr<unordered_map<K,typename list<pair<K,V>>::iterator, Hash>> my_make_shared(){
        auto newMap = make_shared<unordered_map<K,typename list<pair<K,V>>::iterator, Hash>>();
        for(auto it = pairs->begin();it != pairs->end();++it){
            newMap->insert({it->first, it});
        }
        return newMap;
    }

    shared_ptr<list<pair<K,V>>> my_make_shared_list(){
        auto l = make_shared<list<pair<K,V>>>();
        for (auto it = pairs->begin();it != pairs->end();++it){
            l->push_back({it->first, it->second});
        }
        return l;
    }
public:

    shared_ptr<list<pair<K,V>>> pairs;
    shared_ptr<unordered_map<K,typename list<pair<K,V>>::iterator, Hash>> map;
    bool isTaken = false;
    ~insertion_ordered_map() noexcept = default;

    insertion_ordered_map() noexcept
    {
        pairs = make_shared<list<pair<K,V>>>();
        map = make_shared<unordered_map<K,typename list<pair<K,V>>::iterator, Hash>>();
    };

    insertion_ordered_map(insertion_ordered_map const &other)
    {
        pairs = other.pairs;
        map = other.map;
        if(other.isTaken){
            auto prevPairs = pairs.get();
            auto prevMap = map.get();
            try {
                pairs = my_make_shared_list();
                map = my_make_shared();
            } catch (exception& e){
                pairs.reset(prevPairs);
                map.reset(prevMap);
                throw e;
            }
        }
    }

    insertion_ordered_map(insertion_ordered_map&& other) noexcept :
            pairs(move(other.pairs)),
            map(move(other.map)),
            isTaken(move(other.isTaken))
    {}

    insertion_ordered_map& operator=(insertion_ordered_map other) noexcept
    {
        if(this != &other){
            pairs = other.pairs;
            map = other.map;
        }
        return *this;
    }

    /* checks if key already exists and then
     *  if exists
     *      erases it from list of pairs
     *      push back this key to the list of pairs,
     *      edge key - key exists as last and
     *      returns false
     *  if not
     *      push back to the list of pairs
     *      insert key, value and list::iterator of the ey to the map
     *      returns true
     */
    bool insert(K const &k, V const &v)
    {
        auto it = map -> find(k);

        if(it != map -> end()) {
            auto last = pairs -> end();
            last--;
            if(it -> second == last) {
                return false;
            }
        }

        auto prevPairs = pairs.get();
        auto prevMap = map.get();
        try {
            if (!map.unique()) {
                pairs = my_make_shared_list();
                map = my_make_shared();
            }
        } catch (exception& e){
            pairs.reset(prevPairs);
            map.reset(prevMap);
            throw e;
        }
        pair<typename unordered_map<K,typename list<pair<K,V>>::iterator, Hash>::iterator,bool> p;
        it = map->find(k);
        V value = v;
        bool newElem = (it == map->end());
        auto afterErase = pairs->begin();
        if(!newElem){
            value = it->second->second;
            afterErase = pairs->erase(it->second);
        }
        pair<K,V> pair = make_pair(k,value);
        pairs->push_back(pair);
        try {
            (*map.get())[k] = --pairs->end();
        } catch (const exception &e) {
            if(!newElem){
                pairs->emplace(afterErase);
            }
            pairs->pop_back();
            throw e;
        }
        isTaken = false;
        return newElem;
    }

    void erase(K const &k){
        auto prevPairs = pairs.get();
        auto prevMap = map.get();
        try {
            if (!map.unique()) {
                pairs = my_make_shared_list();
                map = my_make_shared();
            }
        } catch (exception& e){
            pairs.reset(prevPairs);
            map.reset(prevMap);
            throw e;
        }
        auto it = map->find(k);
        if(it != map->end()){
            pairs->erase(it->second);
            map->erase(it);
        } else {
            throw lookup_error();
        }
        isTaken = false;
    }

    void merge(insertion_ordered_map const &other)
    {
        auto prevPairs = pairs.get();
        auto prevMap = map.get();
        try {
            if (!map.unique()) {
                pairs = my_make_shared_list();
                map = my_make_shared();
            }
        } catch (exception& e){
            pairs.reset(prevPairs);
            map.reset(prevMap);
            throw e;
        }
        // now map.unique() is true
        // makes new shared just in case that insert throws
        auto copyPairsShared = my_make_shared_list();
        auto copyMapShared = my_make_shared();
        // these functions makes copies but task allows it there
        auto it = other.pairs->begin();
        try {
            while(it != other.pairs->end()){
                this->insert(it->first, it->second);
                it++;
            }
        } catch (exception &e) {
            map = copyMapShared;
            pairs = copyPairsShared;
            //nothing is changed
            throw e;
        }
        isTaken = false;
    }

    V &at(K const &k){
        auto iter = map->find(k);
        if(iter == map->end()){
            throw lookup_error();
        }
        auto prevPairs = pairs.get();
        auto prevMap = map.get();
        try {
            if (!map.unique()) {
                pairs = my_make_shared_list();
                map = my_make_shared();
            }
        } catch (const exception& e){
            pairs.reset(prevPairs);
            map.reset(prevMap);
            throw e;
        }
        isTaken = true;
        return iter->second->second;
    }

    V const &at(K const &k) const {
        try {
            return map->at(k)->second;
        } catch (const std::exception& e) {
            throw lookup_error();
        }
    }

    template <typename = std::enable_if_t<is_default_constructible<V>::value>>
    V &operator[](K const &k){
        try {
            return this->at(k);// this handles the memory issues
        } catch (const lookup_error& e) {
            this->insert(k, V());
            return this->at(k);
        }
    }

    size_t size() const noexcept
    {
        assert(map->size() == pairs->size());
        return map->size();
    }

    bool empty() const noexcept
    {
        return map->empty();
    }

    void clear()
    {
        auto prevPairs = pairs.get();
        auto prevMap = map.get();
        try {
            if (!map.unique()) {
                pairs = my_make_shared_list();
                map = my_make_shared();
            } else {
                pairs->clear();
                map->clear();
            }
        } catch (exception& e){
            pairs.reset(prevPairs);
            map.reset(prevMap);
            throw e;
        }
        isTaken = false;
    }

    bool contains(K const &k) const noexcept
    {
        return (map->find(k) != map->end());
    }

    class iterator {
    private:
        const insertion_ordered_map<K,V,Hash> *map = nullptr;
        typename list<pair<K,V>>::iterator iter;

    public:
        ~iterator() noexcept = default;
        iterator() noexcept = default;

        iterator(const insertion_ordered_map *m, bool begin) noexcept {
            map = m;
            if(begin) iter = map->pairs->begin();
            else iter = map->pairs->end();
        }

        iterator(const iterator &other) noexcept
        {
            map = other.map;
            iter = other.iter;
        }

        iterator&  operator=(const iterator& other) noexcept {
            map = other.map;
            iter = other.iter;
            return *this;
        }

        const pair<K,V>& operator*()
        {
            if(map == nullptr) {
                throw exception();
            }
            return *iter;
        }

        const pair<K,V>* operator->()
        {
            if(map == nullptr) {
                throw exception();
            }
            return &(*iter);
        }

        iterator operator++()
        {
            if(map == nullptr) {
                throw exception();
            }
            iter++;
            return *this;
        }

        bool operator==(const iterator& b) noexcept
        {
            return (b.map == map && iter == b.iter);
        }

        bool operator!=(const iterator& b) noexcept
        {
            return (b.map != map || iter != b.iter);
        }

    };

    iterator begin() const noexcept
    {
        iterator it(this, true);
        return it;
    }

    iterator end() const noexcept
    {
        iterator it(this, false);
        return it;
    }
};

#endif // INSERTION_ORDERED_MAP_H
