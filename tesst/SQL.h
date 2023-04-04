#pragma once
#include <sql.h>
#include <sqlext.h>
#include <Windows.h>
#include <iostream>
#include <locale>

#pragma comment(lib,"odbc32.lib")

int ServerStartup()
{
    SQLHANDLE henv, hdbc, hstmt;
    SQLRETURN retcode;

    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
    {
        std::cout << "Error allocating environment handle" << std::endl;
        return 0;
    }

    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
    {
        std::cout << "Error setting environment attribute" << std::endl;
        return 0;
    }

    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
    {
        std::cout << "Error allocating connection handle" << std::endl;
        return 0;
    }

    SQLWCHAR ConStr[] = L"DRIVER={SQL Server};SERVER=localhost\\MSSQLSERVER02;DATABASE=master;Trusted_Connection=True;";
    retcode = SQLDriverConnect(hdbc, NULL, ConStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
        std::cout << "Error connecting to database" << std::endl;
        return 0;
    }

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt))
    {
        return 0;
    }

    //Get number of rows from the table named Shares
    if (SQL_SUCCESS != SQLExecDirect(hstmt, (SQLWCHAR*)L"Select count(*) from Shares",SQL_NTS))
    {
        return 0;
    }
    else
    {
        char name[64];
        SQLFetch(hstmt);
        SQLGetData(hstmt, 1, SQL_C_CHAR, name, sizeof(name), NULL);
        Market.resize(std::stoi(name));
        SQLFreeStmt(hstmt,SQL_CLOSE);
    }

    //Get Name and price data from shares
    if (SQL_SUCCESS != SQLExecDirect(hstmt, (SQLWCHAR*)L"Select Name,Price from shares", SQL_NTS))
    {
        return 0;
    }
    else
    {
        char val[64];
        SQLLEN readlen;

        for (int i = 0;SQLFetch(hstmt) == SQL_SUCCESS;i++)
        {
            SQLGetData(hstmt, 1, SQL_C_CHAR, val, sizeof(val), &readlen);
            Market[i].Name = val;
            SQLGetData(hstmt, 2, SQL_C_CHAR, val, sizeof(val), &readlen);
            Market[i].price = std::stof(val);
        }
        SQLFreeStmt(hstmt, SQL_CLOSE);
    }

    //Get number of columns in the table Customer
    int Columncount;
    if (SQL_SUCCESS != SQLExecDirect(hstmt, (SQLWCHAR*)L"Select Count(*) from INFORMATION_SCHEMA.Columns where TABLE_NAME = 'Customer'",SQL_NTS))
    {
        return 0;
    }
    else
    {
        char val[64];
        SQLFetch(hstmt);
        SQLGetData(hstmt, 1, SQL_C_CHAR, val, sizeof(val), NULL);
        Columncount = std::stoi(val);
        SQLFreeStmt(hstmt, SQL_CLOSE);
    }

    //Get all data from row from table Customer and put them into Customer Map
    std::string Buf = "Select CustomerID, Name, Password, Balance, ";
    for (int i = 0; i < Market.size(); i++)
    {
        if (i == (Market.size() - 1))
            Buf += Market[i].Name + " ";
        else
        {
            Buf += Market[i].Name + ", ";
        }
    }
    Buf += " from Customer";

    std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>> converter;
    std::wstring widestr = converter.from_bytes(Buf);
    const SQLWCHAR* statement = reinterpret_cast<const SQLWCHAR*>(widestr.c_str());

    if (SQL_SUCCESS != SQLExecDirect(hstmt, (SQLWCHAR*)L"Select count(*) from Customer", SQL_NTS))
    {
        return 0;
    }
    else
    {
        //Get number of rows inside Customer table
        char val[66];
        SQLFetch(hstmt);
        SQLGetData(hstmt, 1, SQL_C_CHAR, val, sizeof(val), NULL);
        int rows = std::stoi(val);
        SQLFreeStmt(hstmt,SQL_CLOSE);

        if (SQL_SUCCESS != SQLExecDirect(hstmt, (SQLWCHAR*)statement, SQL_NTS))
        {
            return 0;
        }
        else
        {
            while (SQLFetch(hstmt) == SQL_SUCCESS)
            {
                std::vector<Stonks> TempStonk;
                SQLGetData(hstmt, 1, SQL_C_CHAR, val, sizeof(val), NULL);
                int id_ = std::stoi(val);
                SQLGetData(hstmt, 2, SQL_C_CHAR, val, sizeof(val), NULL);
                std::string Name = val;
                Cus_Map[Name].id = id_;
                Cus_Map[Name].Name = val;
                SQLGetData(hstmt, 3, SQL_C_CHAR, val, sizeof(val), NULL);
                Cus_Map[Name].password = val;
                SQLGetData(hstmt, 4, SQL_C_CHAR, val, sizeof(val), NULL);
                Cus_Map[Name].Balance = std::stof(val);
                Cus_Map[Name].Customershares.resize(Market.size());
                for (int j = 5, a = 0; j <= (4+Market.size()); j++, a++)
                {
                    SQLGetData(hstmt, j, SQL_C_CHAR, val, sizeof(val), NULL);
                    Cus_Map[Name].Customershares[a].Share_Name = Market[a].Name;
                    Cus_Map[Name].Customershares[a].Share_Quantity = std::stoi(val);
                }
            }
        }
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    SQLDisconnect(hdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    SQLFreeHandle(SQL_HANDLE_ENV, henv);
}

int ServerShutdown()
{
    SQLHANDLE henv, hdbc, hstmt;
    SQLRETURN retcode;

    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
        std::cout << "Error allocating environment handle" << std::endl;
        return 0;
    }

    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
        std::cout << "Error setting environment attribute" << std::endl;
        return 0;
    }

    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
        std::cout << "Error allocating connection handle" << std::endl;
        return 0;
    }

    SQLWCHAR ConStr[] = L"DRIVER={SQL Server};SERVER=localhost\\MSSQLSERVER02;DATABASE=master;Trusted_Connection=True;";
    retcode = SQLDriverConnect(hdbc, NULL, ConStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
        std::cout << "Error connecting to database" << std::endl;
        return 0;
    }

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt))
    {
        return 0;
    }

    //Get number of columns from the table customer
    int Columncount;
    if (SQL_SUCCESS != SQLExecDirect(hstmt, (SQLWCHAR*)L"Select Count(*) from INFORMATION_SCHEMA.Columns where TABLE_NAME = 'Customer'", SQL_NTS))
    {
        return 0;
    }
    else
    {
        char val[64];
        SQLFetch(hstmt);
        SQLGetData(hstmt, 1, SQL_C_CHAR, val, sizeof(val), NULL);
        Columncount = std::stoi(val);
        SQLFreeStmt(hstmt, SQL_CLOSE);
    }

    std::vector<std::string> Columnlist(Columncount);
    SQLWCHAR table_name[] = L"Customer";
    SQLCHAR column_name[256];

    //Get name of columns from table Customer
    SQLColumns(hstmt, NULL, 0, NULL, 0, table_name, SQL_NTS, NULL, 0);
    for (int i = 0;SQLFetch(hstmt) == SQL_SUCCESS;i++)
    {
        SQLGetData(hstmt, 4, SQL_C_CHAR, column_name, sizeof(column_name), NULL);
        Columnlist[i] = (char*)column_name;
    }
    SQLFreeStmt(hstmt, SQL_CLOSE);

    //Update the Database with the new customer information
    for (auto it = Cus_Map.begin(); it != Cus_Map.end(); ++it)
    {
        std::string Statement = "Update Customer Set ";
        Statement += (Columnlist[3] + " =" + std::to_string(it->second.Balance) + ", ");
        for (int i = 4; i < Columnlist.size(); i++)
        {
			for (auto tit = it->second.Customershares.begin(); tit != it->second.Customershares.end(); ++tit)
			{
                if (i == (Columnlist.size() - 1))
                {
                    Statement += (Columnlist[i] + " =" + std::to_string(tit->Share_Quantity));
                }
                else
                {
                    Statement += (Columnlist[i] + " =" + std::to_string(tit->Share_Quantity) + ", ");
                    i++;
                }
			}
        }
        Statement += " where CustomerID = " + std::to_string(it->second.id);

        std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>> converter;
        std::wstring widestr = converter.from_bytes(Statement);
        const SQLWCHAR* statement = reinterpret_cast<const SQLWCHAR*>(widestr.c_str());

        if (SQL_SUCCESS != SQLExecDirect(hstmt, (SQLWCHAR*)statement ,SQL_NTS))
        {
            return 0;
        }
        else
        {
            SQLFreeStmt(hstmt, SQL_CLOSE);
        }
    }
    
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    SQLDisconnect(hdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    SQLFreeHandle(SQL_HANDLE_ENV, henv);
}
