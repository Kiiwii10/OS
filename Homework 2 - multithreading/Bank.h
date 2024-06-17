#pragma once
#ifndef BANK_H
#define BANK_H
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <random>
#include <pthread.h>
#include <unistd.h>
#include <exception>

using namespace std;

enum AccountError
{
    ACCOUNT_ALREADY_EXISTS = 1,
    ACCOUNT_PASSWORD_MISMATCH = 2,
    ACCOUNT_NOT_ENOUGH_MONEY = 3,
    ACCOUNT_DOES_NOT_EXIST = 4
};

enum commandNum
{
    OPEN_ACCOUNT = 1,
    DEPOSIT = 2,
    WITHDRAW = 3,
    BALANCE = 4,
    CLOSE_ACCOUNT = 5,
    TRANSFER = 6,
    END = 7
};

typedef struct {
    commandNum command;
    int id;
    string password;
    unsigned int amount;
    int targetId;
} commandLine;

class ReadWriteLock{
private:
    // int count;
    // pthread_mutex_t reader;
    // pthread_mutex_t writer;
    pthread_mutex_t mutex;          // Mutex for protecting shared variables
    pthread_cond_t readCondition;   // Condition variable for readers
    pthread_cond_t writeCondition;  // Condition variable for writers
    int readersCount;               // Number of active readers
    bool isWriting;                 // Whether a writer is currently writing

public:
    ReadWriteLock();
    ~ReadWriteLock();
    void readerDown();
    void readerUp();
    void writerDown();
    void writerUp();
};

using namespace std;

class LogFile
{
private:
    ofstream& logFile;
    pthread_mutex_t logLock;

public:
    LogFile(ofstream& logFile);
    ~LogFile();
    void closeLog();
    void writeLog(string log);
    void writeError(AccountError error, int atmId, int accountId=-1, unsigned int amount=0);
};


class Account
{
private:
    bool valid;
    int id;
    string password;
    unsigned int balance;
    
public:
    ReadWriteLock accountLock;
    
    Account();
    Account(int id, string password, unsigned int balance);
    ~Account();
    int getId();
    string getPassword();
    int unsafeWithdraw(unsigned int amount);
    void unsafeDeposit(unsigned int amount);
    bool unsafeGetValid();
    unsigned int unsafeGetBalance();
    void accountInit(int id, string password, int balance, int atmId, LogFile& logFile);
    int getBalance(int atmId, string inputPass, LogFile& logFile);
    bool deposit(unsigned int amount, int atmId, string inputPass, LogFile& logFile);
    bool withdraw(unsigned int amount, int atmId, string inputPass, LogFile& logFile);
    void closeAccount(int atmId, string inputPass, LogFile& logFile);

    void executeCommand(commandLine command, int atmId, LogFile& logFile);


    int bankCommission(int percent, LogFile& logFile);
    void printAccount();

    
};

class Bank
{
private:
    map<int, Account>& accounts;
    LogFile& logFile;
    unsigned int bankBalance;
    bool atmsWorking;
    ReadWriteLock* globalAccountsLock;
    ReadWriteLock atmFlagLock;
    pthread_mutex_t bankLock;

public:
    Bank(map<int, Account>& accounts, LogFile& logFile,  bool atmsWorking, 
         ReadWriteLock* globalAccountLock);
    ~Bank();
    bool getAtmsWorking();
    void setAtmsWorking(bool working);
    void commissionAccounts();
    void printAllAccounts();

};

class ATM
{
private:
    map<int, Account>& accounts;
    ReadWriteLock* globalAccountsLock;
    int atmId;

public:
    static int atmsCreated;
    ATM(map<int, Account>& accounts, ReadWriteLock* globalAccountsLock);
    ~ATM();
    void transfer(int src_id, string src_password, int target_id, unsigned int amount, LogFile& logFile);
        
    void run(ifstream& file, LogFile& logFile);
    void executeCommand(commandLine command, LogFile& logFile);

    void transferLock(Account* src_acc, Account* dst_acc);
    void transferUnlock(Account* src_acc, Account* dst_acc);
};

class ThreadData 
{
public:
    ATM* atm;
    ifstream* file;
    LogFile* logFile; 
    int atmId;

    ThreadData();
    ~ThreadData();
    void setThreadData(ATM* atm, ifstream* file, LogFile* logFile, int atmId);
};


/**
 * @brief gets the command from the user and returns the command number
 * 1 - open account
 * 2 - deposit
 * 3 - withdraw
 * 4 - balance
 * 5 - close account
 * 6 - transfer
 * 
 * @param command 
 * @return int 
 */
commandLine commandParser(string command);

void *atmThreadFunc(void* data);

void *bankThreadFuncCommission(void* data);

void *bankThreadFuncPrint(void* data);

void closeFiles(vector<ifstream>& commandFiles, LogFile& logFile);


#endif