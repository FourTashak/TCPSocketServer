#pragma once
#pragma warning(disable : 4996)
#include <sstream>
#include <iomanip>
#include <random>
#include <string>
#include <openssl/sha.h>


std::string generate_salt()
{
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, sizeof(alphanum) - 2);
    std::string salt;
    for (int i = 0; i < 16; ++i) {
        salt += alphanum[distribution(generator)];
    }
    return salt;
}

std::string sha256(const std::string str)
{

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

void DecryptReceive(char* Received)
{
#define Buy 0
#define Sell 1


    if (Received[0] == 0)
    {

    }
}