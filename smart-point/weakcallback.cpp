#include "base/Mutex.h"

#include <map>
#include <vector>
#include <iostream>
#include <algorithm>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/bind.hpp>
#include <assert.h>
#include <stdio.h>
using std::string;

class Stock : boost::noncopyable
{
public:
    Stock(const string& name) : name_(name)
    {
        printf("Stock[%p] %s\n", this, name_.c_str());
    }

    ~Stock()
    {
        printf("~Stock[%p] %s\n", this, name_.c_str());
    }

    const string& key() const {
        return name_;
    }
private:
    string name_;
};

//////////////////////////////////////////////////////
namespace version1
{
class StockFactory : boost::noncopyable
{
public:
    boost::shared_ptr<Stock> get(const string& str)
    {
        MutexLockGuard lock(mutex_);
        Iterator it = stocks_.find(str);
        if (it == stocks_.end())
        {
            stocks_[str] = boost::make_shared<Stock>(str);
            return stocks_[str];
        }
        return it->second;
    }
private:
    mutable MutexLock mutex_;
    std::map<std::string, boost::shared_ptr<Stock> > stocks_;
    typedef std::map<string, boost::shared_ptr<Stock> >::iterator Iterator;
};
}
//////////////////////////////////////////////////////
namespace version2
{
class StockFactory : public boost::enable_shared_from_this<StockFactory>,
                            boost::noncopyable
{
public:
    boost::shared_ptr<Stock> get(const string& key)
    {
        boost::shared_ptr<Stock> pStock;
        MutexLockGuard lock(mutex_);
        boost::weak_ptr<Stock>& wkStock = stocks_[key];
        pStock = wkStock.lock();
        if (!pStock)
        {
            pStock.reset(new Stock(key));
            wkStock = pStock;
        }
        return pStock;
    }
private:
    MutexLock mutex_;
    std::map<string, boost::weak_ptr<Stock> > stocks_;
    typedef std::map<string, boost::weak_ptr<Stock> >::iterator Iterator;
};
}
//////////////////////////////////////////////////////
namespace version3
{
class StockFactory
{
public:
    boost::shared_ptr<Stock> get(const string& key)
    {
        boost::shared_ptr<Stock> pStock;
        MutexLockGuard lock(mutex_);
        boost::weak_ptr<Stock>& wkStock = stocks_[key];
        pStock = wkStock.lock();
        if (!pStock)
        {
            pStock.reset(new Stock(key),
                        boost::bind(&StockFactory::deleteStock, this, _1));
            wkStock = pStock;
        }
        return pStock;
    }
private:
    void deleteStock(Stock* stock)
    {
        printf("delete stock[%p]", stock);
        if (stock)
        {
            MutexLockGuard lock(mutex_);
            stocks_.erase(stock->key());
        }
        delete stock;
    }
    MutexLock mutex_;
    std::map<string, boost::weak_ptr<Stock> > stocks_;
};
}
//////////////////////////////////////////////////////
namespace version4
{
class StockFactory : public boost::enable_shared_from_this<StockFactory>,
                            boost::noncopyable
{
public:
    boost::shared_ptr<Stock> get(const string& key)
    {
        boost::shared_ptr<Stock> pStock;
        MutexLockGuard lock(mutex_);
        boost::weak_ptr<Stock>& wkStock = stocks_[key];
        pStock = wkStock.lock();
        if (!pStock) {
            pStock.reset(new Stock(key),
                            boost::bind(&StockFactory::deletestock,
                            shared_from_this(), _1));
            wkStock = pStock;
        }
        return pStock;
    }
    ~StockFactory()
    {
        printf("~StockFactory\n");
    }
private:
    void deletestock(Stock* stock)
    {
        printf("deletestock[%p]\n", stock);
        if (stock) {
            MutexLockGuard lock(mutex_);
            stocks_.erase(stock->key());
        }
        delete stock;
    }

    mutable MutexLock mutex_;
    std::map<string, boost::weak_ptr<Stock> > stocks_;
    typedef std::map<string, boost::weak_ptr<Stock> >::iterator Iterator;
};
}
//////////////////////////////////////////////////////
class StockFactory : public boost::enable_shared_from_this<StockFactory>,
                            boost::noncopyable
{
public:
    boost::shared_ptr<Stock> get(const string& key)
    {
        boost::shared_ptr<Stock> pStock;
        MutexLockGuard lock(mutex_);
        boost::weak_ptr<Stock>& wkStock = stocks_[key];
        pStock = wkStock.lock();
        if (!pStock) {
            pStock.reset(new Stock(key),
                            boost::bind(&StockFactory::weakDeleteCallback,
                            boost::weak_ptr<StockFactory>(shared_from_this()), _1));
            wkStock = pStock;
        }
        return pStock;
    }
private:
    static void weakDeleteCallback(const boost::weak_ptr<StockFactory>& wkFactory,
                                   Stock* stock)
    {
        printf("weakDeleteStock[%p]\n", stock);
        boost::shared_ptr<StockFactory> factory(wkFactory.lock());
        if (factory) {
            factory->removeStock(stock);
        }
        else
        {
            printf("factory died.\n");
        }
        delete stock;
    }

    void removeStock(Stock* stock)
    {
        if (stock)
        {
            MutexLockGuard lock(mutex_);
            Iterator it = stocks_.find(stock->key());
            if (it->second.expired())
            {
                stocks_.erase(stock->key());
            }
        }
    }
    mutable MutexLock mutex_;
    std::map<string, boost::weak_ptr<Stock> > stocks_;
    typedef std::map<string, boost::weak_ptr<Stock> >::iterator Iterator;
};


void shortLifeFactory()
{
    using namespace boost;
    shared_ptr<Stock> stock;
    {
        printf("1\n");
        shared_ptr<version4::StockFactory> factory(new version4::StockFactory);
        stock = factory->get("NYSE:IBM");
        shared_ptr<Stock> stock2 = factory->get("NYSE:IBM");
        assert(stock == stock2);
        printf("2\n");
    }
}
int main() {
    shortLifeFactory();
}
