#include <iostream>
#include <fstream>
#include "pfm.h"
#include "util.h"

PagedFileManager* PagedFileManager::_pf_manager = 0;

PagedFileManager* PagedFileManager::instance()
{
    if(!_pf_manager)
        _pf_manager = new PagedFileManager();

    return _pf_manager;
}


PagedFileManager::PagedFileManager()
{
}


PagedFileManager::~PagedFileManager()
{
}


RC PagedFileManager::createFile(const string &fileName)
{
    // CreateFile only if file with fileName does not exist
    if (!FileExistsConst(fileName)) {
        FILE* file = fopen(fileName.c_str(), "wb" );

        if (file) {
            fclose(file);
            return 0;
        }
    }

    return -1 ;

}


RC PagedFileManager::destroyFile(const string &fileName)
{
    // Destroy file only if file with fileName exists
    if (FileExistsConst(fileName)) {
        FILE *file = fopen(fileName.c_str(), "wb");

        if (file) {
            fclose(file);

            if (std::remove(fileName.c_str()) == 0) {
                return 0;
            }
        }
    }

    return -1;
}


RC PagedFileManager::openFile(const string &fileName, FileHandle &fileHandle)
{

    // return error if fileHandle is already assigned to a file
    if (fileHandle.file) {
        std::cout << "fileHandle check: " << fileHandle.file << fileHandle.fileName;
        return -1;
    }

    if (FileExistsConst(fileName)) {
        FILE* file = fopen(fileName.c_str(), "rb+");
        if (!file) {
            // File could not be opened
            return -1;
        }

        fileHandle.file = file;     // assigning the filehandle to this open file
        fileHandle.fileName = fileName;  // assigning the fileName to the fileHandle
        //fileHandle.initializeCounters(); // intialize the counters
        return 0;

    }
    return -1;
}


RC PagedFileManager::closeFile(FileHandle &fileHandle)
{
    // close file only if fileHandle is assigned
    if (fileHandle.file) {
        //fileHandle.updateCounters();
        fclose(fileHandle.file);
        return 0;
    }

    return -1;
}


FileHandle::FileHandle()
{
    file = NULL;
    fileName = "";
    readPageCounter = 0;
    writePageCounter = 0;
    appendPageCounter = 0;
}


FileHandle::~FileHandle()
{
}

RC FileHandle::initializeCounters()
{
    int *data = new int[3];
    cout << "initializeCounters" << endl;

    // Read the counters from the file
    if (readPage(-1, reinterpret_cast<void*>(data)) != 0) {
        std::cerr << "Error: Failed to read counters from page 0." << std::endl;
        delete[] data; // Free allocated memory
        return -1; // Return error code
    }

    // Extract counters
    readPageCounter = data[0];
    writePageCounter = data[1];
    appendPageCounter = data[2];

    std::cout << "Counters initialized." << std::endl;

    // Free allocated memory
    delete[] data;

    return 0;
}

RC FileHandle::updateCounters()
{
    int *data = new int[3];
    cout << "updateCounters" << endl;

    // Store counters in the data array
    data[0] = readPageCounter;
    data[1] = writePageCounter;
    data[2] = appendPageCounter;

    // Write the counters to page 0 of the file
    if (writePage(-1, reinterpret_cast<void*>(data)) != 0) {
        std::cerr << "Error: Failed to write counters to page 0." << std::endl;
        delete[] data; // Free allocated memory
        return -1; // Return error code
    }

    std::cout << "Counters updated." << std::endl;

    // Free allocated memory
    delete[] data;
    return 0;
}

RC FileHandle::readPage(PageNum pageNum, void *data)
{


    // Check if the specified page number exists
    if((pageNum + 1) > getNumberOfPages()){
        return -1;
    }

    // Set file pointer offset to the beginning of the specified page

    int fsk = fseek(file, (pageNum + 1) * PAGE_SIZE, SEEK_SET);
    if (fsk != 0){
        return -1;
    }


    // Read a page and check if the read operation was successful
    int fr = fread(data, PAGE_SIZE, 1, file);

    if (fr == 0 ){
        return -1;
    }

    // Increment the read  counter
    ++readPageCounter;

    return 0;
}


RC FileHandle::writePage(PageNum pageNum, const void *data)
{

    // Check if the specified page number exists
    if((pageNum + 1) > getNumberOfPages()){
        return -1;
    }


    // Set file pointer offset to the beginning of the specified page
    int fsk = fseek(file, (pageNum + 1) * PAGE_SIZE, SEEK_SET);
    if (fsk != 0){
        perror("fseek error");
        return -1;
    }


    // Read a page and check if the read operation was successful
    int fw = fwrite(data, PAGE_SIZE, 1, file);

    if (fw != 1 ){

        perror("fwrite error");
        return -1;
    }

    // Increment the read  counter
    ++writePageCounter;

    return 0;
}


RC FileHandle::appendPage(const void *data)
{
    int fs;

    if (ftell(file) == 0) {
        // create the hidden file
        fs = fseek(file, PAGE_SIZE, SEEK_END);
    }
    else {
        fs = fseek(file, 0, SEEK_END);
    }

    // Set file pointer to the end of fil
    if(fs) {
        return -1;
    }
    
    if (fwrite(data, PAGE_SIZE, 1, file) != 1) {
        return -1;
    }

    // Increment the append  counter
    ++appendPageCounter;


    return 0;
}


unsigned FileHandle::getNumberOfPages()
{
    int fsk, size, noOfPages;
    fsk = fseek(file, 0, SEEK_END);

    if (fsk != 0) {
        return -1;
    }
    size = ftell(file);

    if (!size) {
        return 0;
    }

    noOfPages = size / PAGE_SIZE;
    return noOfPages - 1;

}


RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount)
{

    readPageCount = readPageCounter;
    writePageCount = writePageCounter;
    appendPageCount = appendPageCounter;

    return 0;
}
