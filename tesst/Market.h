#pragma once
#include <vector>
#include <unordered_map>
#include "BasicFuns.h"

struct Stonks
{
    std::string Name;
    float price;

    Stonks()
    {

    }

    Stonks(std::string name, float Price)
    {
        Name = name;
        price = Price;
    }
};

struct Shares
{
    std::string Share_Name;
    int Share_Quantity;

    Shares()
    {

    }

    Shares(std::string Name, int Quantity)
    {
        Share_Name = Name;
        Share_Quantity = Quantity;
    }
};

struct Customer
{
    int id;
    std::string Name;
    std::string password;
    float Balance;
    bool logged_in = false;

    std::vector<Shares> Customershares;
};

std::vector<Stonks> Market; //Prices of stocks

namespace std
{
    template<>struct hash<Stonks>
    {
        size_t operator()(const Stonks& key)
        {
            return hash<std::string>()(key.Name);
        }
    };

    template<>struct hash<Customer>
    {
        size_t operator()(const Customer& key)
        {
            return hash<std::string>()(key.Name);
        }
    };
}

auto StockMapConstruct()
{
    std::unordered_map<std::string, Stonks> Stocks;
    return Stocks;
}

auto Stock_Map = StockMapConstruct();

auto CustomerMapConstruct()
{
    std::unordered_map<std::string, Customer> Customers;
    return Customers;
}

auto Cus_Map = CustomerMapConstruct();

const Customer Authenticate(std::string Username, std::string Password) //Authenticating if the username and password is correct
{
    const auto& account = Cus_Map;
    Customer Null;
    Null.id = -1;
    if (account.find(Username) != account.end())
    {
        const Customer& Acc = account.at(Username);
        if (Acc.password == sha256(Password))
        {
            return Acc;
        }
        return Null;
    }
    else
    {
        return Null;
    }
}

bool BuyStock(int Qant, std::string StockName, Customer& Auth)
{
    if ((Stock_Map[StockName].price * Qant) <= Auth.Balance)
    {
        for (int i = 0; i < Market.size(); i++)
        {
            if (Market[i].Name == StockName)
            {
                for (int j = 0; j < Auth.Customershares.size(); j++)
                {
                    if (Auth.Customershares[j].Share_Name == Market[i].Name)
                    {
                        Auth.Customershares[j].Share_Quantity += Qant;
                        Auth.Balance -= (Market[i].price * Qant);
                        return true;
                    }
                }
                Auth.Customershares.push_back(Shares(StockName, Qant));
                Auth.Balance -= (Market[i].price * Qant);
                return true;
            }
        }
    }
    else
        return false;
}

bool SellStock(int quant, std::string StockName, Customer& Auth)
{
    for (int i = 0; i < Market.size(); i++)
    {
        if (Market[i].Name == StockName)
        {
            for (int j = 0; j < Auth.Customershares.size(); j++)
            {
                if (Auth.Customershares[j].Share_Name == Market[i].Name)
                {
                    if (Auth.Customershares[j].Share_Quantity >= quant)
                    {
                        Auth.Customershares[j].Share_Quantity -= quant;
                        Auth.Balance += (quant * Market[i].price);
                        if (Auth.Customershares[j].Share_Quantity == 0)
                        {
                            Auth.Customershares.erase(Auth.Customershares.begin() + j);
                            return true;
                        }
                        return true;
                    }
                    return false;
                }
            }
        }
    }
}

float getprice(std::string Name, std::string Quantity)
{
    if (Quantity != "")
    {
        int quant = stoi(Quantity);
        std::string realname = Name;
        for (int i = 0; i < Market.size(); i++)
        {
            if (Market[i].Name == realname)
            {
                return (Market[i].price * quant);
            }
        }
    }
}

//make a function which will send the stock list to the client
//System::Object^ stocklist()
//{
//    array<System::Object^>^ StockArray;
//    System::Array::Resize(StockArray, Market.size());
//    for (int i = 0; i < Market.size(); i++)
//    {
//        StockArray[i] = StdStringToCString(Market[i].Name);
//    }
//    return StockArray;
//}

bool DoesCustomerHaveShare(std::string Share, std::string Quantity, Customer& Auth)
{
    if (Quantity != "")
    {
        std::string Realshare = Share;
        int realQuant = stoi(Quantity);
        for (int i = 0; i < Auth.Customershares.size(); i++)
        {
            if (Auth.Customershares[i].Share_Name == Realshare && Auth.Customershares[i].Share_Quantity >= realQuant && realQuant > 0)
                return true;
        }
    }
    return false;
}


