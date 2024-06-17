#include "Bank.h"
using namespace std;

ReadWriteLock::ReadWriteLock(){
// ReadWriteLock::ReadWriteLock() : count(0) {
    // if((pthread_mutex_init(&this->reader, NULL) != 0)||
    //    (pthread_mutex_init(&this->writer, NULL) != 0))
    // {
    //     perror("pthread_mutex_init failed");
    //     exit(1);
    // }
    // Initialize mutex and condition variables
    if (pthread_mutex_init(&mutex, NULL) != 0){
        perror("pthread_mutex_init failed");
        exit(1);
    };
    if ((pthread_cond_init(&readCondition, NULL) != 0) ||
        (pthread_cond_init(&writeCondition, NULL) != 0)){
        perror("pthread_cond_init failed");
        exit(1);
    }
    readersCount = 0;
    isWriting = false;
}

ReadWriteLock::~ReadWriteLock() {
    // pthread_mutex_destroy(&this->reader);
    // pthread_mutex_destroy(&this->writer);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&readCondition);
    pthread_cond_destroy(&writeCondition);
}

void ReadWriteLock::readerDown() {
    // pthread_mutex_lock(&this->reader);
    // this->count++;
    // if (this->count == 1)
    // {
    //     pthread_mutex_lock(&this->writer);
    // }
    // pthread_mutex_unlock(&this->reader);
    pthread_mutex_lock(&mutex);
    // Wait for any writing to complete
    while (isWriting) {
        pthread_cond_wait(&readCondition, &mutex);
    }
    readersCount++;
    pthread_mutex_unlock(&mutex);
}

void ReadWriteLock::readerUp() {
    // pthread_mutex_lock(&this->reader);
    // this->count--;
    // if (this->count == 0)
    // {
    //     pthread_mutex_unlock(&this->writer);
    // }
    // pthread_mutex_unlock(&this->reader);
    pthread_mutex_lock(&mutex);
    readersCount--;
    // Signal waiting writers if there are no more readers
    if (readersCount == 0) {
        pthread_cond_signal(&writeCondition);
    }
    pthread_mutex_unlock(&mutex);
}

void ReadWriteLock::writerDown() {
    // pthread_mutex_lock(&this->writer);
    pthread_mutex_lock(&mutex);
    // Wait until there are no active readers or writers
    while (readersCount > 0 || isWriting) {
        pthread_cond_wait(&writeCondition, &mutex);
    }
    isWriting = true;
    pthread_mutex_unlock(&mutex);
}

void ReadWriteLock::writerUp() {
    // pthread_mutex_unlock(&this->writer);
    pthread_mutex_lock(&mutex);
    isWriting = false;
    // Signal waiting readers or writers
    pthread_cond_broadcast(&readCondition);
    pthread_cond_signal(&writeCondition);
    pthread_mutex_unlock(&mutex);
}


// handle lock failure in the init or outside.
Account::Account() : valid(false), id(0), password(""), balance(0) {
}


Account::Account(int id, string password, unsigned int balance) : valid(true), id(id),
                                                                password(password),
                                                                balance(balance) {
}

Account::~Account() {
}

int Account::getId() {
    return this->id;
}

string Account::getPassword() {
    return this->password;
}

int Account::unsafeWithdraw(unsigned int amount) {
    if (this->balance < amount) {
        return 1;
    }
    this->balance -= amount;
    return 0;
}

void Account::unsafeDeposit(unsigned int amount) {
    this->balance += amount;
}

bool Account::unsafeGetValid() {
    return this->valid;
}

unsigned int Account::unsafeGetBalance() {
    return this->balance;
}

void Account::accountInit(int id, string password, int balance, int atmId, 
                          LogFile& logFile) {
    this->accountLock.writerDown();
    if (this->valid){
        logFile.writeError(ACCOUNT_ALREADY_EXISTS, atmId);
        sleep(1);
        this->accountLock.writerUp();
        return;
    }
    this->valid = true;
    this->id = id;
    this->password = password;
    this->balance = balance;
    string log = to_string(atmId) + ": New account id is " + to_string(id) 
               + " with password " + this->password + " and initial balance " 
               + to_string(balance);
    logFile.writeLog(log);
    sleep(1);
    this->accountLock.writerUp();
    return;
}

int Account::getBalance(int atmId, string inputPass, LogFile& logFile) {
    if (!this->valid)
    {
        logFile.writeError(ACCOUNT_DOES_NOT_EXIST, atmId, this->id);
        sleep(1);
        return -1;
    }
    if (inputPass != this->password){
        logFile.writeError(ACCOUNT_PASSWORD_MISMATCH, atmId, this->id);
        sleep(1);
        return -1;
    }
    
    // critical section
    unsigned int currBalance = this->balance;
    string log = to_string(atmId) + ": Account " + to_string(this->id) 
               + " balance is " + to_string(currBalance);
    logFile.writeLog(log);
    sleep(1);
    return currBalance;
}

bool Account::deposit(unsigned int amount, int atmId, string inputPass, LogFile& logFile) {
    if (!this->valid)
    {
        logFile.writeError(ACCOUNT_DOES_NOT_EXIST, atmId, this->id);
        sleep(1);
        return false;
    }
    if (inputPass != this->password){
        logFile.writeError(ACCOUNT_PASSWORD_MISMATCH, atmId, this->id);
        sleep(1);
        return false;
    }
    if (amount < 0)
    {
        sleep(1);
        return false;
    }
    this->balance += amount;
    string log = to_string(atmId) + ": Account " + to_string(this->id) 
               + " new balance is " + to_string(this->balance) + " after " 
               + to_string(amount) + " $ was deposited";
    logFile.writeLog(log);
    sleep(1);
    return true;
}

bool Account::withdraw(unsigned int amount, int atmId, string inputPass, LogFile& logFile) {
    if (!this->valid)
    {
        logFile.writeError(ACCOUNT_DOES_NOT_EXIST, atmId, this->id);
        sleep(1);
        return -1;
    }
    if (inputPass != this->password){
        logFile.writeError(ACCOUNT_PASSWORD_MISMATCH, atmId, this->id);
        sleep(1);
        return false;
    }
    if (this->balance < amount)
    {
        logFile.writeError(ACCOUNT_NOT_ENOUGH_MONEY, atmId, this->id, amount);
        sleep(1);
        return false;
    }
    this->balance -= amount;
    string log = to_string(atmId) + ": Account " + to_string(this->id)
               + " new balance is " + to_string(this->balance) + " after " 
               + to_string(amount) + " $ was withdrew";
    logFile.writeLog(log);
    sleep(1);
    return true;
}

void Account::closeAccount(int atmId, string inputPass, LogFile& logFile) {
    this->accountLock.writerDown();
    if (!this->valid)
    {
        logFile.writeError(ACCOUNT_DOES_NOT_EXIST, atmId, this->id);
        sleep(1);
        this->accountLock.writerUp();
        return;
    }
    if (inputPass != this->password){
        logFile.writeError(ACCOUNT_PASSWORD_MISMATCH, atmId, this->id);
        sleep(1);
        this->accountLock.writerUp();
        return;
    }
    this->valid = false;
    string log = to_string(atmId) + ": Account " + to_string(this->id) 
                 + " is now closed. Balance was " + to_string(this->balance);
    logFile.writeLog(log);
    sleep(1);
    this->accountLock.writerUp();
    return;
}

void Account::executeCommand(commandLine command, int atmId, LogFile& logFile) {
    if (command.command == DEPOSIT || command.command == WITHDRAW)
    {
        this->accountLock.writerDown();
    }
    else {
        this->accountLock.readerDown();
    }
    switch (command.command)
    {
    case DEPOSIT:
        this->deposit(command.amount, atmId, command.password, logFile);
        break;
    case WITHDRAW:
        this->withdraw(command.amount, atmId, command.password, logFile);
        break;
    case BALANCE:
        this->getBalance(atmId, command.password, logFile);
        break;
    default:
        break;
    }

    if (command.command == DEPOSIT || command.command == WITHDRAW)
    {
        this->accountLock.writerUp();
    }
    else {
        this->accountLock.readerUp();
    }
}

int Account::bankCommission(int percent, LogFile& logFile) {
    this->accountLock.writerDown();
    if (!this->valid)
    {   
        this->accountLock.writerUp();
        return 0;
    }
    unsigned int amount = (this->balance * percent) / 100;
    this->balance -= amount;
    string log = "Bank: commissions of " + to_string(percent) + " % were charged, the bank gained " 
               + to_string(amount) + " $ from account " + to_string(this->id);
    logFile.writeLog(log);
    this->accountLock.writerUp();
    return amount;
}

void Account::printAccount() {
    this->accountLock.readerDown();
    if (!this->valid)
    {
        this->accountLock.readerUp();
        return;
    }
    cout << "Account " << this->id << ": Balance - " << this->balance 
         << " $, Account Password - " << this->password << endl;
    this->accountLock.readerUp();
    return;
}


Bank::Bank(map<int, Account>& accounts, LogFile& logFile, bool atmsWorking,
           ReadWriteLock* globalAccountsLock) : accounts(accounts),
                                                logFile(logFile),
                                                bankBalance(0),
                                                atmsWorking(atmsWorking),
                                                globalAccountsLock(globalAccountsLock) {
    if (pthread_mutex_init(&this->bankLock, NULL) != 0)
    {
        perror("pthread_mutex_init failed");
        exit(1);
    }
}

Bank::~Bank() {
    pthread_mutex_destroy(&this->bankLock);
}



bool Bank::getAtmsWorking() {
    this->atmFlagLock.readerDown();
    bool working = this->atmsWorking;
    this->atmFlagLock.readerUp();
    return working;
}

void Bank::setAtmsWorking(bool working) {
    this->atmFlagLock.writerDown();
    this->atmsWorking = working;
    this->atmFlagLock.writerUp();
}

void Bank::commissionAccounts() {
    pthread_mutex_lock(&this->bankLock);
    this->globalAccountsLock->readerDown();
    for (auto &account : accounts)
    {
        // generate random number between 1 and 5
        int random = rand() % 5 + 1;
        this->bankBalance += account.second.bankCommission(random, this->logFile);
    }
    this->globalAccountsLock->readerUp();
    pthread_mutex_unlock(&this->bankLock);
}

void Bank::printAllAccounts() {
    pthread_mutex_lock(&this->bankLock);
    this->globalAccountsLock->writerDown();
    printf("\033[2J"); // clear console
    printf("\033[1;1H"); // move cursor to top left
    printf("Current Bank Status\n");
    for (auto &account : accounts)
    {
        account.second.printAccount();
    }
    cout << "The bank has " << this->bankBalance << " $" << endl;
    this->globalAccountsLock->writerUp();
    pthread_mutex_unlock(&this->bankLock);
}

ATM::ATM(map<int, Account>& accounts, ReadWriteLock* globalAccountsLock) : accounts(accounts),
                                                                      globalAccountsLock(globalAccountsLock)
{
    ATM::atmsCreated++;
    this->atmId = ATM::atmsCreated;
}

ATM::~ATM() {
}

void ATM::transferLock(Account* src_acc, Account* dst_acc){
    src_acc->accountLock.writerDown();
    dst_acc->accountLock.writerDown();
}

void ATM::transferUnlock(Account* src_acc, Account* dst_acc){
    dst_acc->accountLock.writerUp();
    src_acc->accountLock.writerUp();
}

void ATM::transfer(int src_id, string src_password, int target_id, unsigned int amount, 
                   LogFile& logFile)
{
    if (this->accounts.find(src_id) == this->accounts.end()){
        // account with the same id does not exist
        logFile.writeError(ACCOUNT_DOES_NOT_EXIST, this->atmId, src_id);
        sleep(1);
        return;
    }
    if (this->accounts.find(target_id) == this->accounts.end()){
        // account with the same id does not exist
        logFile.writeError(ACCOUNT_DOES_NOT_EXIST, this->atmId, target_id);
        sleep(1);
        return;
    }
    
    // mutex stuff
    this->accounts[src_id].accountLock.writerDown();
    // check if accounts are open
    if (!this->accounts[src_id].unsafeGetValid()){
        logFile.writeError(ACCOUNT_DOES_NOT_EXIST, this->atmId, src_id);
        sleep(1);
        this->accounts[src_id].accountLock.writerUp();
        return;
    }
    this->accounts[target_id].accountLock.writerDown();
    if (!this->accounts[target_id].unsafeGetValid()){
        logFile.writeError(ACCOUNT_DOES_NOT_EXIST, this->atmId, target_id);
        this->transferUnlock(&this->accounts[src_id], &this->accounts[target_id]);
        sleep(1);
        return;
    }
    // check if passwords are correct
    if (this->accounts[src_id].getPassword() != src_password){
        logFile.writeError(ACCOUNT_PASSWORD_MISMATCH, this->atmId, src_id);
        sleep(1);
        this->transferUnlock(&this->accounts[src_id], &this->accounts[target_id]);
        return;
    }
    // check if src_id has enough money
    if(this->accounts[src_id].unsafeWithdraw(amount)){
        logFile.writeError(ACCOUNT_NOT_ENOUGH_MONEY, this->atmId, src_id, amount);
        sleep(1);
        this->transferUnlock(&this->accounts[src_id], &this->accounts[target_id]);
        return;
    }
    
    // transfer money
    this->accounts[target_id].unsafeDeposit(amount);

    // log the transfer
    string log = to_string(this->atmId) + ": Transfer " + to_string(amount) 
               + " from account " + to_string(src_id) + " to account " 
               + to_string(target_id) + " new account balance is "
               + to_string(this->accounts[src_id].unsafeGetBalance()) 
               + " new target account balance is "
               + to_string(this->accounts[target_id].unsafeGetBalance());
    logFile.writeLog(log);
    sleep(1);
    this->transferUnlock(&this->accounts[src_id], &this->accounts[target_id]);
    return;
    // mutex unlock
}


void ATM::run(ifstream& file, LogFile& logFile){

    string line;
    while (getline(file, line))
    {
        // get rid of \n and \r chars
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')){
            line.pop_back();
        }
        usleep(100000); // 0.1 seconds (100000 microseconds)
        // parse the command
        commandLine command = commandParser(line);
        if (command.command == END)
        {
            return;
        }
        this->executeCommand(command, logFile);
    }
    return;
}

void ATM::executeCommand(commandLine command, LogFile& logFile) {
    if (command.command == OPEN_ACCOUNT){
        this->globalAccountsLock->writerDown();
        int id = command.id;
        if (this->accounts.find(id) == this->accounts.end()){ //check if spot empty
            this->accounts[id] = Account();
        }
        this->accounts[id].accountInit(id, command.password, command.amount,
                                       this->atmId, logFile);
        this->globalAccountsLock->writerUp();

    }
    else if(command.command == CLOSE_ACCOUNT){
        this->globalAccountsLock->writerDown();
        int id = command.id;
        if (this->accounts.find(id) == this->accounts.end()){
            // account with the same id does not exist
            logFile.writeError(ACCOUNT_DOES_NOT_EXIST, this->atmId, id);
            this->globalAccountsLock->writerUp();
            sleep(1);
            return;
        }
        this->accounts[id].closeAccount(this->atmId, command.password, logFile);
        this->globalAccountsLock->writerUp();
    }
    else if (command.command == TRANSFER){
        this->globalAccountsLock->readerDown();
        this->transfer(command.id, command.password, command.targetId, 
                       command.amount, logFile);
        this->globalAccountsLock->readerUp();
    }
    else {
        this->globalAccountsLock->readerDown();
        int id = command.id;
        if (this->accounts.find(id) == this->accounts.end()){
            // account with the same id does not exist
            logFile.writeError(ACCOUNT_DOES_NOT_EXIST, this->atmId, id);
            sleep(1);
            this->globalAccountsLock->readerUp();
            return;
        }
        this->accounts[id].executeCommand(command, this->atmId, logFile);
        this->globalAccountsLock->readerUp();
    }
}

commandLine commandParser(string command) {
    string commandType = command.substr(0, command.find(" "));
    commandLine commandLine;
    vector<string> commandParts;
    string delimiter = " ";
    size_t start = 0, end = 0;
    while (end != string::npos)
    {
        end = command.find(delimiter, start);
        commandParts.push_back(command.substr(start, end - start));
        start = end + delimiter.length();
    }
    if (start != string::npos)
    {
        commandParts.push_back(command.substr(start));
    }
    
    switch (commandType.c_str()[0])
    {
    case 'O':
        commandLine.command = OPEN_ACCOUNT;
        commandLine.id = stoi(commandParts[1]);
        commandLine.password = commandParts[2];
        commandLine.amount = stoi(commandParts[3]);
        commandLine.targetId = 0;
        break;
    
    case 'D':
        commandLine.command = DEPOSIT;
        commandLine.id = stoi(commandParts[1]);
        commandLine.password = commandParts[2];
        commandLine.amount = stoi(commandParts[3]);
        commandLine.targetId = 0;
        break;

    case 'W':
        commandLine.command = WITHDRAW;
        commandLine.id = stoi(commandParts[1]);
        commandLine.password = commandParts[2];
        commandLine.amount = stoi(commandParts[3]);
        commandLine.targetId = 0;
        break;

    case 'B':
        commandLine.command = BALANCE;
        commandLine.id = stoi(commandParts[1]);
        commandLine.password = commandParts[2];
        commandLine.amount = 0;
        commandLine.targetId = 0;
        break;

    case 'Q':
        commandLine.command = CLOSE_ACCOUNT;
        commandLine.id = stoi(commandParts[1]);
        commandLine.password = commandParts[2];
        commandLine.amount = 0;
        commandLine.targetId = 0;
        break;
    
    case 'T':
        commandLine.command = TRANSFER;
        commandLine.id = stoi(commandParts[1]);
        commandLine.password = commandParts[2];
        commandLine.targetId = stoi(commandParts[3]);
        commandLine.amount = stoi(commandParts[4]);
        break;
    
    default:
        // end of file - assumed right structure of commands
        commandLine.command = commandNum::END;
        break;
    }
    
    return commandLine;
}

LogFile::LogFile(ofstream& logFile) : logFile(logFile) {
    if (pthread_mutex_init(&this->logLock, NULL) != 0)
    {
        perror("pthread_mutex_init failed");
        exit(1);
    }
}

LogFile::~LogFile() {
    pthread_mutex_destroy(&this->logLock);
}

void LogFile::closeLog() {
    this->logFile.close();
}

void LogFile::writeLog(string log) {
    pthread_mutex_lock(&this->logLock);
    this->logFile << log << endl;
    pthread_mutex_unlock(&this->logLock);
}

void LogFile::writeError(AccountError error, int atmId, int accountId, unsigned int amount) {
    pthread_mutex_lock(&this->logLock);
    string log = "Error " + to_string(atmId) + ": Your transaction failed - ";
    switch (error)
    {
    case ACCOUNT_ALREADY_EXISTS:
        log += "account with the same id exists";
        break;
    case ACCOUNT_DOES_NOT_EXIST:
        log += "account id " + to_string(accountId) + " does not exist";
        break;
    case ACCOUNT_NOT_ENOUGH_MONEY:
        log += "account id " + to_string(accountId) + " balance is lower than " + to_string(amount);
        break;
    case ACCOUNT_PASSWORD_MISMATCH:
        log += "password for account id " + to_string(accountId) + " is incorrect";
        break;
    default:
        break;
    }
    this->logFile << log << endl;
    pthread_mutex_unlock(&this->logLock);
}

ThreadData::ThreadData() : atm(nullptr), file(nullptr),
                           logFile(nullptr), atmId(0) {}

ThreadData::~ThreadData() {
}

void ThreadData::setThreadData(ATM* atm, ifstream* file, LogFile* logFile, int atmId){
    this->atm = atm;
    this->file = file;
    this->logFile = logFile;
    this->atmId = atmId;
}


void *atmThreadFunc(void* data){
    ThreadData* threadData = static_cast<ThreadData*>(data);
    ATM* atm = threadData->atm;
    ifstream* file = threadData->file;
    LogFile* logFile = threadData->logFile;

    
    if (atm && file && logFile) {
        atm->run(*file, *logFile);
    } else {
        std::cerr << "ThreadData contains a null pointer. Thread cannot proceed." << std::endl;
    }
    return nullptr;
}

void *bankThreadFuncCommission(void* data){
    Bank* bank = static_cast<Bank*>(data);
    while (bank->getAtmsWorking())
    {
        // commission accounts every 3 seconds
        usleep(3000000); // 3 seconds
        if(bank->getAtmsWorking()){
            bank->commissionAccounts();
        }
        
    }
    return nullptr;
}

void *bankThreadFuncPrint(void* data){
    Bank* bank = static_cast<Bank*>(data);
    while (bank->getAtmsWorking())
    {
        // print all accounts every 0.5 seconds
        usleep(500000); // 0.5 seconds
        if(bank->getAtmsWorking()){
            bank->printAllAccounts();
        }
    }
    return nullptr;
}

void closeFiles(vector<ifstream>& commandFiles, LogFile& logFile){
    for (auto &commandFile : commandFiles)
    {
        commandFile.close();
    }
    logFile.closeLog();
}

int ATM::atmsCreated = 0;

int main(int argc, char *argv[]) {
    int numATMs = argc - 1;
    if (numATMs < 1)
    {
        std::cerr << "Bank error: illegal arguments" << std::endl;
        return 1;
    }
    // initialize log.txt file
    ofstream logOut;
    logOut.open("log.txt", ios::out | ios::trunc);
    if (!logOut.is_open())
    {
        std::cerr << "Bank error: failed to open log file" << std::endl;
        return 1;
    }
    // open command files from arguments
    vector<ifstream> commandFiles(numATMs);
    for (int i = 1; i < argc; i++)
    {
        commandFiles[i - 1].open(argv[i]);
        if (!commandFiles[i - 1].is_open())
        {
            std::cerr << "Bank error: illegal arguments" << std::endl;
            return 1;
        }
        // commandFiles[i - 1] = commandFile;
    }
    
    // init global
    map<int, Account> accounts;
    ReadWriteLock globalAccountsLock;

    LogFile logFile = LogFile(logOut);

    // create bank and ATMs
    Bank bank = Bank(accounts, logFile,  true, &globalAccountsLock);

    vector<ATM> atms;
    for (int i = 0; i < numATMs; i++)
    {
        atms.push_back(ATM(accounts, &globalAccountsLock));
    }

    

    // create threads
    pthread_t bankCommissionThread;
    pthread_t bankPrintThread;
    vector<pthread_t> atmThreads(numATMs);
    vector<ThreadData> atmData(numATMs);
    
    // run bank and ATMs
    if (pthread_create(&bankCommissionThread, NULL, bankThreadFuncCommission, &bank) != 0){
        perror("Bank error: pthread_create failed");
        closeFiles(commandFiles, logFile);
        return 1;
    }
    if(pthread_create(&bankPrintThread, NULL, bankThreadFuncPrint, &bank)){
        perror("Bank error: pthread_create failed");
        closeFiles(commandFiles, logFile);
        return 1;
    }

    for (int i = 0; i < numATMs; i++)
    {
        atmData[i].setThreadData(&atms[i], &commandFiles[i], &logFile, i);
        if (pthread_create(&(atmThreads[i]), NULL, atmThreadFunc, &(atmData[i])) != 0){
            perror("Bank error: pthread_create failed");
            closeFiles(commandFiles, logFile);
            return 1;
        }
    }
    
    // join threads - wait for threads to finish
    for (int i = 0; i < numATMs; i++)
    {
        if (pthread_join(atmThreads[i], NULL) != 0){
            perror("Bank error: pthread_join failed");
            closeFiles(commandFiles, logFile);
            return 1;
        }
    }

    bank.setAtmsWorking(false);

    // join bank threads
    if (pthread_join(bankCommissionThread, NULL) != 0){
        perror("Bank error: pthread_join failed");
        closeFiles(commandFiles, logFile);
        return 1;
    }
    
    if(pthread_join(bankPrintThread, NULL) != 0){
        perror("Bank error: pthread_join failed");
        closeFiles(commandFiles, logFile);
        return 1;
    }

    // close files
    closeFiles(commandFiles, logFile);

    printf("\033[2J"); // clear console
    printf("\033[1;1H"); // move cursor to top left
    
    return 0;
}