#pragma once
#include <vector>
#include <string>
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

bool Authenticate(std::string Username, std::string Password)
{
    const auto& account = Cus_Map;
    if (account.find(Username) != account.end())
    {
        const Customer& Acc = account.at(Username);
        if (Acc.password == sha256(Password))
        {
            return true;
        }
        return false;
    }
    else
    {
        return false;
    }
}

float BuyStock(System::String^ Quantity, System::String^ StockName, Customer& Auth)
{
    for (int i = 0; i < Market.size(); i++)
    {
        if (Market[i].Name == Cstring_to_String(StockName))
        {
            for (int j = 0; j < Auth.Customershares.size(); j++)
            {
                if (Auth.Customershares[j].Share_Name == Market[i].Name)
                {
                    int Qant = int::Parse(Quantity);
                    Auth.Customershares[j].Share_Quantity += Qant;
                    return Auth.Balance -= (Market[i].price * Qant);
                }
            }
            int Qant = int::Parse(Quantity);
            Auth.Customershares.push_back(Shares(Cstring_to_String(StockName), Qant));
            return Auth.Balance -= (Market[i].price * Qant);
        }
    }
}


float SellStock(System::String^ Quantity, System::String^ StockName, Customer& Auth)
{
    for (int i = 0; i < Market.size(); i++)
    {
        if (Market[i].Name == Cstring_to_String(StockName))
        {
            for (int j = 0; j < Auth.Customershares.size(); j++)
            {
                if (Auth.Customershares[j].Share_Name == Market[i].Name)
                {
                    int quant = int::Parse(Quantity);
                    Auth.Customershares[j].Share_Quantity -= quant;
                    if (Auth.Customershares[j].Share_Quantity == 0)
                    {
                        Auth.Customershares.erase(Auth.Customershares.begin() + j);
                    }
                    return Auth.Balance += (Market[i].price * quant);
                }
            }
        }
    }
}

float getprice(System::String^ Name, System::String^ Quantity)
{
    if (Quantity != "")
    {
        int quant = int::Parse(Quantity);
        std::string realname = Cstring_to_String(Name);
        for (int i = 0; i < Market.size(); i++)
        {
            if (Market[i].Name == realname)
            {
                return (Market[i].price * quant);
            }
        }
    }
}

System::Object^ stocklist()
{
    array<System::Object^>^ StockArray;
    System::Array::Resize(StockArray, Market.size());
    for (int i = 0; i < Market.size(); i++)
    {
        StockArray[i] = StdStringToCString(Market[i].Name);
    }
    return StockArray;
}

bool DoesCustomerHaveShare(System::Object^ Share, System::String^ Quantity, Customer& Auth)
{
    if (Quantity != "")
    {
        std::string Realshare = Cstring_to_String(Share->ToString());
        int realQuant = int::Parse(Quantity);
        for (int i = 0; i < Auth.Customershares.size(); i++)
        {
            if (Auth.Customershares[i].Share_Name == Realshare && Auth.Customershares[i].Share_Quantity >= realQuant && realQuant > 0)
                return true;
        }
    }
    return false;
}


