#include <iostream>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include "rbfm.h"

PagedFileManager* pfm = PagedFileManager::instance();
RecordBasedFileManager* RecordBasedFileManager::_rbf_manager = 0;
Page* Page::_page_manager = 0;

Page* Page::instance()
{
    if(!_page_manager)
        _page_manager = new Page();

    return _page_manager;
}


RecordBasedFileManager* RecordBasedFileManager::instance()
{
    if(!_rbf_manager)
        _rbf_manager = new RecordBasedFileManager();

    return _rbf_manager;
}

RecordBasedFileManager::RecordBasedFileManager()
{
}

RecordBasedFileManager::~RecordBasedFileManager()
{
}

RC RecordBasedFileManager::createFile(const string &fileName)
{
    // check if we can create pfm inside constructor as it is required for all create, open, close, destroy
    return pfm->createFile(fileName.c_str());
}

RC RecordBasedFileManager::destroyFile(const string &fileName) {
    return pfm->destroyFile(fileName.c_str());
}

RC RecordBasedFileManager::openFile(const string &fileName, FileHandle &fileHandle) {
    return pfm->openFile(fileName.c_str(), fileHandle);
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
    return pfm->closeFile(fileHandle);
}

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {

    void *recordToInsert = (char*)malloc(PAGE_SIZE);
    short recordLength = transformRecord(recordDescriptor, (void*)data, recordToInsert);

    unsigned pageNo = findPage(fileHandle,recordLength);

    rid.pageNum = pageNo;
    cout<<"pageNum::"<<pageNo<<endl;

    void *page_buffer = calloc(PAGE_SIZE,sizeof(char));

    if(fileHandle.readPage(pageNo, page_buffer) != 0){
        return -1;
    }

    Page page((char *)page_buffer);

    // return error if writing record fails
    if(page.writeRecord((char *)recordToInsert, recordLength,rid.slotNum)!=0){
        return -1;
    };

    // return error if writing a page fails
    if(fileHandle.writePage(pageNo, page.getPageData())!=0){
        return -1;
    };

    free(recordToInsert);
    free(page_buffer);

    return 0;
}

unsigned RecordBasedFileManager::findPage(FileHandle &fileHandle, short length) {
    unsigned page_count = fileHandle.getNumberOfPages();
    void *page_buffer = calloc(PAGE_SIZE,sizeof(char));

    // linearly search for page with sufficient space
    for(int i=0; i<page_count; i++){
        fileHandle.readPage(i,page_buffer);

        Page page((char *)page_buffer);
        if(page.getFreeSpace() >= length + SLOT_OFFSET_LEN + SLOT_LENGTH_LEN){
            free(page_buffer);
            cout<<"Findpage"<<i<<endl;
            return i;
        }

    }

    free(page_buffer);

    char* new_page_buffer = (char*)calloc(PAGE_SIZE,sizeof(char));
    short freeSpace = PAGE_SIZE-FREE_OFFSET_LEN-FREE_SPACE_LEN-SLOT_CNT_LEN;
    memcpy(new_page_buffer + PAGE_SIZE - FREE_OFFSET_LEN - FREE_SPACE_LEN, &freeSpace, FREE_SPACE_LEN);


    if(fileHandle.appendPage(new_page_buffer) !=0) {
        cout<< "Failed appending page"<< endl;
    }

    free(new_page_buffer);
    return page_count;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {

    // using RID get the record from the page and the slot
    char *page_buffer = (char *) calloc(PAGE_SIZE, sizeof(char));
    fileHandle.readPage(rid.pageNum, page_buffer);
    Page page(page_buffer);
    RC rc = page.readRecord(rid.slotNum, data);

    if (rc == -1 ) {
        return -1;
    }

    if (rc == 2) {
        // tombstone
        short pageNo = 0;
        short slotNum = 0;
        page.getSlotInfo(rid.slotNum, pageNo, slotNum);
        RID rid1;
        rid1.pageNum = pageNo * -1;
        rid1.slotNum = slotNum * -1;
        if (readRecord(fileHandle, recordDescriptor, rid1, data) !=0) {
            return -1;
        }
        return 0;
    }
    // data record representation - FieldCount - NullFieldsINdicator - N Non null Field pointers - data fields
    short offset = 0;
    int fieldCount = 0;

    // get field count from first 2 bytes of the record
    memcpy(&fieldCount, ((char*)data+offset), SIZE_OF_SHORT);
    offset += SIZE_OF_SHORT;

    // extract the nullfieldindicator values
    short nullFieldsIndicatorSize = ceil((double) fieldCount / CHAR_BIT);
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorSize);
    memcpy(nullsIndicator, ((char*)data+offset), nullFieldsIndicatorSize);
    offset += nullFieldsIndicatorSize;

    // count the non null field count
    bool nullBit;
    short nonNullFieldCount = 0;
    for(int i=0; i<fieldCount; i++) {
        nullBit = nullsIndicator[0] & (1 << (7 - i));
        if (!nullBit) {
            nonNullFieldCount += 1;
        }
    }

    // extract the field offsets for all the non-null fields
    unsigned char *fieldOffsets = (unsigned char *) malloc(nonNullFieldCount * SIZE_OF_SHORT);
    memcpy(fieldOffsets, ((char*)data+offset), nonNullFieldCount * SIZE_OF_SHORT);
    offset += nonNullFieldCount * SIZE_OF_SHORT;

    short nonNullCounts = 0;
    int charLength;
    int prevOffset = 0;
    short fieldLengthOffset = 0;
    int recordLength = nullFieldsIndicatorSize; // final record length that is returned

    void *bufferData = (char *) malloc(PAGE_SIZE);
    memcpy((char*)bufferData, nullsIndicator, nullFieldsIndicatorSize);

    for(int i=0; i<fieldCount; i++) {
        nullBit = nullsIndicator[0] & (1 << (7 - i));

        prevOffset = fieldLengthOffset;

        if (!nullBit) {
            fieldLengthOffset = *((int*) ((char *) fieldOffsets + (nonNullCounts * SIZE_OF_SHORT)));
            nonNullCounts += 1;

            // get the fieldLength value from the fieldOffsets for eac nonNULL field
            Attribute a = recordDescriptor[i];
            if(a.type == TypeVarChar)
            {

                // 4 bytes for storing the length of the varchar attribute
                charLength = fieldLengthOffset - prevOffset;
                memcpy((char*)bufferData + recordLength, &charLength, sizeof(charLength));
                recordLength += sizeof(charLength);

            }
            // copy each non null field value to bufferData
            memcpy((char*)bufferData + recordLength, ((char*)data + offset + prevOffset ), fieldLengthOffset - prevOffset);
            recordLength += (fieldLengthOffset - prevOffset);

        }
    }


    // copy entire bufferData to data
    memcpy(data, bufferData, recordLength);

    // free the memory
    free(bufferData);
    free(fieldOffsets);
    free(nullsIndicator);
    return 0;
}

RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {

    int fieldCount = recordDescriptor.size();

    // given that value n can be calculated by using the formula: ceil(number of fields in a record/8)
    int nullFieldsIndicatorSize = ceil((double) fieldCount / CHAR_BIT);
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorSize);

    // given that the first part in *data contains n bytes for passing the null information
    memcpy(nullsIndicator, data, nullFieldsIndicatorSize);

    bool nullBit;
    // initial record length set to null field size and then total length calculated using for loop
    int recordLength = nullFieldsIndicatorSize;
    for(int i=0; i<fieldCount; i++)
    {
        nullBit = nullsIndicator[0] & (1 << (7-i));
        Attribute a = recordDescriptor[i];
        std::cout << a.name << ": ";
        if(!nullBit)
        {
            if(a.type == TypeInt){
                std::cout << *(int*)((char *)data+recordLength) << endl;
                recordLength += 4;
            }
            else if(a.type == TypeReal){
                std::cout << *(float*)((char *)data+recordLength) << endl;
                recordLength += 4;
            }
            else if(a.type == TypeVarChar)
            {

                // read the length value of varchar field by de referencing
                int attributeLength = *(int *)((char *)data+recordLength);
                // 4 bytes for storing the length of the varchar attribute
                recordLength += 4;
                for(int j=0; j<attributeLength; j++)
                {
                    std::cout <<  *((char *)data+recordLength+j);
                }
                std::cout << "" << endl;
                recordLength = recordLength + attributeLength;

            }
        }
        else
        {
            std::cout << "NULL" << endl;
        }

    }
    free(nullsIndicator);

}

RC RecordBasedFileManager::deleteRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid){
    // Given a record descriptor, delete the record identified by the given rid. \
    // Also, each time when a record is deleted, you will need to compact that page.
    // That is, keep the free space in the middle of the page -- the slot table will be at one end,
    // the record data area will be at the other end, and the free space should be in the middle.

    // using RID get the record from the page and the slot
    void *page_buffer = (char *) malloc(PAGE_SIZE);
    short rlength;
    short roffset;

    fileHandle.readPage(rid.pageNum, page_buffer);
    Page page((char*) page_buffer);

    page.getSlotInfo(rid.slotNum, roffset, rlength);
    // return -1 for deleted tuple
    if ((rlength == 0) && (roffset == -1)) {
        return -1;
    }
    // recursively delete tomstone records
    if ((rlength < 0) && (roffset < 0)) {
       // detected tombstone
        RID rid1;
        rid1.slotNum = rlength * -1;
        rid1.pageNum = roffset * -1;
        deleteRecord(fileHandle, recordDescriptor, rid1);
        rlength = 0;
        roffset = -1;
        page.setSlotInfo(rid.slotNum, roffset, rlength);
        return 0;
    }

    page.deleteRecord(rid.slotNum);

    // return error if writing a page fails
    if(fileHandle.writePage(rid.pageNum, page.getPageData())!=0){
        return -1;
    };

    free(page_buffer);
    return 0;

}

RC RecordBasedFileManager::updateRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, const RID &rid){
    // find the page and the record
    // delete the record
    // insert the record
    // use same slot
    // update the freespace and offset
    short recordOffset = 0;
    short recordLength = 0;
    short slotNum = rid.slotNum;
    void *page_buffer = (char *) malloc(PAGE_SIZE);
    void *recordToInsert = (char *) malloc(PAGE_SIZE);

    // read current page
    fileHandle.readPage(rid.pageNum, page_buffer);
    Page page((char*) page_buffer);

    page.getSlotInfo(slotNum, recordOffset, recordLength);
    short newLength = transformRecord( recordDescriptor, (void*)data, recordToInsert);

    // if only int values have changed
    if (recordLength == newLength) {
        cout << "newlength is same" << endl;
        // insert at the same location and no shifting needed
        memcpy((char *) page_buffer + recordOffset, (char *) recordToInsert, recordLength);
        // write current page to disk
        if (fileHandle.writePage(rid.pageNum, page.getPageData()) != 0) {
            return -1;
        };
        return 0;
    }
    else {

        // iF new record is greater or smaller than current length
        // delete and move records

        short newOffset = recordOffset;
        short length = recordLength;
        short totalSlots = page.getNoOfSLots();

        // iterate over each slot to move the record and change their offsets
        for (int i = slotNum + 1; i <= totalSlots; i++) {

            page.getSlotInfo(i, recordOffset, recordLength);

            // move only the records after the current record
            if (recordOffset >= newOffset) {
                // move the records from current position to new position
                page.moveData(newOffset, recordOffset, recordLength);

                page.setSlotInfo(i, newOffset, recordLength);
                newOffset += recordLength;
            }

        }

        // check if current page has sufficient space to reinsert the data: if yes insert
        if (page.getFreeSpace() > newLength) {

            //insert in same page & update slot offset & length
            memcpy((char*) page_buffer + newOffset, (char*)recordToInsert, newLength);
            page.setSlotInfo(slotNum, newOffset, newLength);

            // set freespace & freeoffset
            page.setFreeSpaceOffset(newOffset + newLength);
            page.setFreeSpace(page.getFreeSpace() + length - newLength);
        }
        else {
            void *newPage = (char *)malloc(PAGE_SIZE);

            // find a page
            unsigned pageNo = findPage(fileHandle,newLength);

            if(fileHandle.readPage(pageNo, newPage) != 0){
                return -1;
            }

            unsigned newSlotnum = 0;
            Page page1 = Page((char*)newPage);

            // return error if writing record fails
            if(page1.writeRecord((char *)recordToInsert,newLength, newSlotnum)!=0){
                return -1;
            }

            // return error if writing a page fails
            if(fileHandle.writePage(pageNo, page1.getPageData())!=0){
                return -1;
            }
            free(newPage);

            // set tombstone for original page
            short spageNo = (short) pageNo * -1;
            short snewSlotnum = (short) newSlotnum * -1;

            page.setSlotInfo(rid.slotNum, spageNo, snewSlotnum); // may not work

        }
        if(fileHandle.writePage(rid.pageNum, page.getPageData())!=0) {
            return -1;
        }
    }

    free(recordToInsert);
    free(page_buffer);
    return 0;

}

RC RecordBasedFileManager::readAttribute(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, const string &attributeName, void *data)
{

    char *page_buffer = (char *) calloc(PAGE_SIZE, sizeof(char));
    fileHandle.readPage(rid.pageNum, page_buffer);
    Page page(page_buffer);
    RC rc = page.readRecord(rid.slotNum, data);

    if (rc == -1 ) {
        return -1;
    }
    // data record representation - FieldCount - NullFieldsINdicator - N Non null Field pointers - data fields
    short offset = 0;
    int fieldCount = 0;
    char nullIndicator = 0;
    // get field count from first 2 bytes of the record
    memcpy(&fieldCount, ((char*)data+offset), SIZE_OF_SHORT);
    offset += SIZE_OF_SHORT;
    // extract the nullfieldindicator values
    short nullFieldsIndicatorSize = ceil((double) fieldCount / CHAR_BIT);
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorSize);
    memcpy(nullsIndicator, ((char*)data+offset), nullFieldsIndicatorSize);

    offset += nullFieldsIndicatorSize;

    // count the non null field count
    bool nullBit;
    short nonNullFieldCount = 0;
    for(int i=0; i<fieldCount; i++) {
        nullBit = nullsIndicator[0] & (1 << (7 - i));
        if (!nullBit) {
            nonNullFieldCount += 1;
        }
    }

    short fulloffset = offset + nonNullFieldCount * SIZE_OF_SHORT;

    // get position of attribute
    Attribute a;
    short nonNullCounts = 0;
    short prevOffset;
    short curOffset;
    void *result = (char*) malloc(PAGE_SIZE);
    for (int i = 0; i < fieldCount; i++) {
        a = recordDescriptor[i];
        nullBit = nullsIndicator[0] & (1 << (7 - i));
        // check for the attribute name requested
        if (a.name == attributeName) {
            if (!nullBit) {
                if (nonNullCounts == 0) {
                    curOffset = 0;
                } else {
                    prevOffset = (nonNullCounts - 1) * SIZE_OF_SHORT;
                    memcpy(&curOffset, (char*)data+offset+prevOffset, SIZE_OF_SHORT);
                }

                memcpy((char *)result, ((char*)data+fulloffset+curOffset),a.length);
                nullIndicator |= (1 << 7);;
                memcpy((char *)data, &nullIndicator, 1);
                memcpy((char *)data+1, (char*)result, a.length);
                break;
                } else {
                    memcpy((char *)data, &nullIndicator, 1);
                    data = NULL;
                    break;
            }
        }
        if (!nullBit){
            nonNullCounts += 1;
        }
    }

    free(result);
    return 0;
}

RC RecordBasedFileManager::scan(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const string &conditionAttribute, const CompOp compOp, const void *value, const vector<string> &attributeNames,
        RBFM_ScanIterator &rbfm_ScanIterator) {
    // return -1;
    rbfm_ScanIterator.fileHandle = fileHandle;
    rbfm_ScanIterator.recordDescriptor = recordDescriptor;
    rbfm_ScanIterator.conditionAttribute = conditionAttribute;
    rbfm_ScanIterator.compOp = compOp;
    rbfm_ScanIterator.value = (void*)value;
    rbfm_ScanIterator.attributeNames = attributeNames;
    rbfm_ScanIterator.recordID.pageNum = 0;
    rbfm_ScanIterator.recordID.slotNum = 1;
    return 0;
}

RBFM_ScanIterator::RBFM_ScanIterator() {
    rbfm = RecordBasedFileManager::instance();

}
void printVoidString(void* ptr, int length) {
    char* charPtr = (char*)ptr; // Cast void* to char*

    for (int i = 0; i < length; i++) {
        printf("%c", charPtr[i]);
    }
    printf("\n");
}

RC RBFM_ScanIterator::getNextRecord(RID &rid, void *data) {
    int numOfPages = fileHandle.getNumberOfPages();
    void *pageDataBuffer = calloc(PAGE_SIZE, sizeof(char));
    short pNum;
    short sNum;
    // predicateHolds = 0 => attribute not found yet
    // 1 => holds true
    // -1 => does not hold true
    short predicateHolds = 0;
    short numOfSlots = -1;
    //cout << "No of Pages in file: " << numOfPages << endl;
    //cout<<"Current recordID.pageNum" << recordID.pageNum<<endl;
    //cout<<"Current recordID.slotNum" << recordID.slotNum<<endl;

    for (pNum = recordID.pageNum; pNum < numOfPages; pNum++) {
        if (fileHandle.readPage(pNum, pageDataBuffer) != 0) {
            return -1;
        }
        Page page((char *)pageDataBuffer);
        numOfSlots = page.getNoOfSLots();
        for (sNum = recordID.slotNum; sNum <= numOfSlots; sNum++) {
            void *recordDataBuffer = (char *) malloc(PAGE_SIZE);
            if (page.readRecord(sNum, recordDataBuffer) != 0) {
                continue;
            }
            /* *******************Possibly convert the logic below for new data formation into a function************************** */
            short offset = 0;
            int fieldCount = 0;
            // get field count from first 2 bytes of the record
            memcpy(&fieldCount, ((char *) recordDataBuffer + offset), SIZE_OF_SHORT);
            offset += SIZE_OF_SHORT;
            short nullFieldsIndicatorSize = ceil((double) fieldCount / CHAR_BIT);
            unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorSize);
            memcpy(nullsIndicator, ((char *) recordDataBuffer + offset), nullFieldsIndicatorSize);
            offset += nullFieldsIndicatorSize;

            // count the non null field count
            bool nullBit;
            short nonNullFieldCount = 0;
            for (int i = 0; i < fieldCount; i++) {
                Attribute a = recordDescriptor[i];
                nullBit = nullsIndicator[0] & (1 << (7 - i));
                if (!nullBit) {
                    nonNullFieldCount += 1;
                }
                else if(a.name == conditionAttribute){
                    predicateHolds = -1;
                }
            }

            // extract the field offsets for all the non-null fields
            unsigned char *fieldOffsets = (unsigned char *) malloc(nonNullFieldCount * SIZE_OF_SHORT);
            memcpy(fieldOffsets, ((char *) recordDataBuffer + offset), nonNullFieldCount * SIZE_OF_SHORT);
            offset += nonNullFieldCount * SIZE_OF_SHORT;
            short nonNullCounts = 0;
            int charLength;
            int prevOffset = 0;
            short fieldLengthOffset = 0;
            int recordLength = nullFieldsIndicatorSize; // final record length that is returned
            void *newBufferData = (char *) malloc(PAGE_SIZE);
            short newDataNullFieldsIndicatorSize = ceil((double) attributeNames.size() / CHAR_BIT);
            unsigned char *newDataNullFieldsIndicator = (unsigned char *) malloc(newDataNullFieldsIndicatorSize);
            memset(nullsIndicator, 0, newDataNullFieldsIndicatorSize);


            int fc = 0;
            int newDataNonNullFeildCount = 0; //nullIndicator index
            int newBufferRecordLength = 0;
            int newBufferAttributeCount = 0;
            predicateHolds = 0;

            for (const std::string& str : attributeNames) {
                std::cout << str << " ";
            }
            std::cout << std::endl;


            while (fc < fieldCount && predicateHolds != -1) {
                nullBit = nullsIndicator[0] & (1 << (7 - fc));
                prevOffset = fieldLengthOffset;
                Attribute a = recordDescriptor[fc];
                auto it = std::find(attributeNames.begin(), attributeNames.end(), a.name);

                if (!nullBit) {
                    fieldLengthOffset = *((int *) ((char *) fieldOffsets + (nonNullCounts * SIZE_OF_SHORT)));
                    nonNullCounts += 1;

                    // get the fieldLength value from the fieldOffsets for each nonNULL field
                    if (a.type == TypeVarChar) {

                        // 4 bytes for storing the length of the varchar attribute
                        charLength = fieldLengthOffset - prevOffset;
                        //memcpy((char *) newBufferData + recordLength, &charLength, sizeof(charLength));
                        //recordLength += sizeof(charLength);
                    }

                    if (conditionAttribute == "" | (value == NULL | compOp == NO_OP)) {
                        predicateHolds = 1;
                    } else {
                        if (a.name == conditionAttribute && predicateHolds != 1) {
                            void *attributeData = (char *) malloc(fieldLengthOffset - prevOffset);
                            memcpy((char *) attributeData, ((char *) recordDataBuffer + offset + prevOffset),
                                   fieldLengthOffset - prevOffset);
                            printVoidString(attributeData, sizeof(charLength));
                            predicateHolds = checkPredicate(attributeData, fieldLengthOffset - prevOffset, a.type);
                            free(attributeData);
                        }
                    } // else

                        // if predicate holds true or predicate yet to be checked
                        if (predicateHolds != -1) {
                            // check if current attribute is requested by the user -> attributeNames
                            if (it != attributeNames.end()) {
                                newDataNonNullFeildCount += 1;
                                // copy each non-null field value to newBufferData
                                if (a.type == TypeVarChar) {
                                    memcpy((char *) newBufferData + newBufferRecordLength, &charLength,
                                           sizeof(charLength));
                                    recordLength += sizeof(charLength);
                                    newBufferRecordLength += sizeof(charLength);
                                }
                                memcpy((char *) newBufferData + newBufferRecordLength,
                                       ((char *) recordDataBuffer + offset + prevOffset),
                                       fieldLengthOffset - prevOffset);
                                newBufferRecordLength += fieldLengthOffset - prevOffset;
                                newBufferAttributeCount += 1;
                            }

                        }
                        recordLength += (fieldLengthOffset - prevOffset);

                    } // null bit
                    // if the requested attribute is null then set the null bit
                    else {
                    if (it != attributeNames.end()) {
                        newBufferAttributeCount += 1;
                        int whichByte = newBufferAttributeCount / 8;
                        int whichBit = newBufferAttributeCount % 8;
                        newDataNullFieldsIndicator[whichByte] += pow(2, 7 - whichBit);
                    }
                    }
                        fc = fc + 1;
                    } // while
                        if (predicateHolds == 1) {
                            memcpy((char *) data, newDataNullFieldsIndicator, newDataNullFieldsIndicatorSize);
                            memcpy((char *) data + newDataNullFieldsIndicatorSize, (char *) newBufferData, newBufferRecordLength);
                            break; // if attribute condition holds true then break out of slot for loop
                        }

                        free(recordDataBuffer);
                        free(fieldOffsets);
                        free(newBufferData);
        }
        /* *******************Possibly convert the above logic for new data formation into a function************************** */

        if (sNum == numOfSlots)  recordID.slotNum = 1;
        if (predicateHolds == 1) {
            break; // if attribute condition holds true then break out of slot for loop
        }
    }
    if(predicateHolds == 1){
        //pNum -= 1;
        //sNum -= 1;
        rid.pageNum = pNum;
        rid.slotNum = sNum;
    if (sNum == numOfSlots) {
        recordID.pageNum = pNum + 1;
        recordID.slotNum = 1;
    } else {
        recordID.pageNum = pNum;
        recordID.slotNum = sNum + 1;
    }
        free(pageDataBuffer);
        return 0;
    }

    free(pageDataBuffer);
    return RBFM_EOF;
}

int getStringLength(void* str) {
    char* s = static_cast<char*>(str);
    int length = 0;
    while (*s != '\0') {
        length++;
        s++;
    }
    return length;
}

void printUnknownLengthString(void* ptr) {
    const unsigned char* bytePtr = (unsigned char*)ptr;
    size_t len = 0;

    while (*bytePtr != '\0') {
        bytePtr++;
    }

    size_t length = len;
    char* charPtr = (char*)ptr;

    for (size_t i = 0; i < length; i++) {
        printf("%c", charPtr[i]);
    }
    printf("\n");
}
RC RBFM_ScanIterator::checkPredicate(void* attributeData, short attributeLen, AttrType attributeType)
{
    //printVoidString(attributeData, attributeLen);
    if (attributeType == TypeVarChar) {
        short predLength = getStringLength(value);
        attributeLen =  (predLength > attributeLen) ? predLength : attributeLen;
        std::string attributeString = std::string(static_cast<char*>(attributeData), attributeLen);
        std::string valueString = std::string(static_cast<char*>(value), attributeLen);

        //if(compOp == EQ_OP && memcmp(attributeData, value, attributeLen) != 0) return 1;
        if(compOp == EQ_OP && memcmp(attributeData, value, attributeLen) == 0) {return 1;}
        else if(compOp == LT_OP && memcmp(attributeData, value, attributeLen) < 0) return 1;
        else if(compOp == LE_OP && memcmp(attributeData, value, attributeLen) <= 0) return 1;
        else if(compOp == GT_OP && memcmp(attributeData, value, attributeLen) > 0) return 1;
        else if(compOp == GE_OP && memcmp(attributeData, value, attributeLen) >= 0) return 1;
        else if(compOp == NE_OP && memcmp(attributeData, value, attributeLen) != 0) return 1;
    }
    else if (attributeType == TypeInt) {

        int attributeInt = *(int *)attributeData;
        int valueInt = *(int *)value;
        if(compOp == EQ_OP && attributeInt == valueInt) return 1;
        else if(compOp == LT_OP && attributeInt < valueInt) return 1;
        else if(compOp == LE_OP && attributeInt <= valueInt) return 1;
        else if(compOp == GT_OP && attributeInt > valueInt) return 1;
        else if(compOp == GE_OP && attributeInt >= valueInt) return 1;
        else if(compOp == NE_OP && attributeInt != valueInt) return 1;
    }
    else if (attributeType == TypeReal) {

        int attributeFloat = *(float *)attributeData;
        int valueFloat = *(float *)value;
        if(compOp == EQ_OP && attributeFloat == valueFloat) return 1;
        else if(compOp == LT_OP && attributeFloat < valueFloat) return 1;
        else if(compOp == LE_OP && attributeFloat <= valueFloat) return 1;
        else if(compOp == GT_OP && attributeFloat > valueFloat) return 1;
        else if(compOp == GE_OP && attributeFloat >= valueFloat) return 1;
        else if(compOp == NE_OP && attributeFloat != valueFloat) return 1;
    }
    return -1;
}

short RecordBasedFileManager::transformRecord( const vector<Attribute> &recordDescriptor, void *data, void *recordToInsert) {
    int fieldCount = recordDescriptor.size();
    // calculate the bytes containing nullbitindicators by using the formula: ceil(number of fields in a record/8)
    short nullFieldsIndicatorSize = ceil((double) fieldCount / CHAR_BIT);
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorSize);

    // copy the first n bytes in *data into nullsIndicator
    memcpy(nullsIndicator, data, nullFieldsIndicatorSize);

    bool nullBit = false;
    short nonNullCounts = 0;

    // initial record length set to nullFieldSize and then total length calculated using for loop
    short recordLength = nullFieldsIndicatorSize;
    int offset = 0; // is offset the data length? // seems like offset and prevRecordLength are same
    void *dataValues = (void *) malloc(PAGE_SIZE);

    std::vector<short> recordOffset;
    short prevRecordLength = 0;

    for (int i = 0; i < fieldCount; i++) {
        nullBit = nullsIndicator[0] & (1 << (7 - i));
        if (!nullBit) {
            nonNullCounts += 1;
            Attribute a = recordDescriptor[i];
            if (a.type == TypeInt) {
                int intValue = *(int *) ((char *) data + recordLength);
                memcpy((char *) dataValues + offset, &intValue, sizeof(int));
                prevRecordLength += 4;
                recordOffset.push_back(prevRecordLength);
                recordLength += 4;
                offset += 4;
            } else if (a.type == TypeReal) {
                float realValue = *(float *) ((char *) data + recordLength);
                memcpy((char *) dataValues + offset, &realValue, sizeof(float));
                prevRecordLength += 4;
                recordOffset.push_back(prevRecordLength);
                recordLength += 4;
                offset += 4;
            } else if (a.type == TypeVarChar) {
                // read length value of varchar by de referencing
                int attributeLength = *(int *) ((char *) data + recordLength);

                recordLength += 4; // 4 bytes for storing the length of the varchar attribute

                for (int j = 0; j < attributeLength; j++) {
                    char charValue = *((char *) data + recordLength + j);
                    memcpy((char *) dataValues + offset, &charValue, sizeof(char));
                    offset += 1;
                }
                prevRecordLength += attributeLength;
                recordOffset.push_back(prevRecordLength);
                recordLength += attributeLength;
            }
        }
    }

    short recordInfoLength = SIZE_OF_SHORT + nullFieldsIndicatorSize + nonNullCounts * (SIZE_OF_SHORT);
    recordLength = recordInfoLength + offset;
    char *bufferToInsert = (char *) malloc(recordLength);
    int copyOffset = 0;
    memcpy((char *) bufferToInsert, &fieldCount, SIZE_OF_SHORT);
    copyOffset += SIZE_OF_SHORT;
    memcpy((char *) bufferToInsert + copyOffset, nullsIndicator, nullFieldsIndicatorSize);
    copyOffset += nullFieldsIndicatorSize;

    for (short value: recordOffset) {

        memcpy((char *) bufferToInsert + copyOffset, &value, SIZE_OF_SHORT);
        copyOffset += SIZE_OF_SHORT;
    }

    memcpy((char *) bufferToInsert + recordInfoLength, (char *) dataValues, offset);
    memcpy((char *) recordToInsert, (char *) bufferToInsert , recordLength);

    free(nullsIndicator);
    free(dataValues);
    free(bufferToInsert);

    return recordLength;

}


RC RBFM_ScanIterator::close(){
    rbfm->closeFile(fileHandle);
};

short RecordBasedFileManager:: getNewTableId(FileHandle &fileHandle){
    //calculate latest table_id
    unsigned page_count = fileHandle.getNumberOfPages();

    void *page_buffer = calloc(PAGE_SIZE,sizeof(char));
    int numTables = 0;

    // linearly search for page with sufficient space
    for(int i=0; i<page_count; i++){
        fileHandle.readPage(i,page_buffer);

        Page page((char *)page_buffer);

        numTables+=page.getNoOfSLots();
    }

    free(page_buffer);
    return numTables+1;
}



/************ PAGE ************/
Page ::Page(char *data) {
    pageData = data;
}
Page::~Page(){
}

short Page::getFreeSpace() {
    short freeSpace;
    // extracting the free space available for page from the page footer
    memcpy(&freeSpace,pageData+PAGE_SIZE-4,2);
    return freeSpace;
}

short Page::getFreeSpaceOffset() {
    short freeSpaceOffset;
    // extracting the freeSpaceOffset from the page footer
    memcpy(&freeSpaceOffset,pageData + PAGE_SIZE - 2,2);
    return freeSpaceOffset;
}

short Page::getNoOfSLots() {
    // No of records(N) != No of slots - fix this
    short noOfSLots;
    // extracting the number of records (N) from the page footer
    memcpy(&noOfSLots,pageData + PAGE_SIZE - FREE_OFFSET_LEN - FREE_SPACE_LEN - SLOT_CNT_LEN,2);

    return noOfSLots;
}

char * Page::getPageData() {
    return pageData;
}

void * Page::setPageData(char *data ) {
    pageData = data;
}

RC Page::readRecord(unsigned int slotNum, void *data) {
    short recordOffset = 0;
    short recordLength = 0;

    // extracting record offset and length from the page footer
    getSlotInfo(slotNum, recordOffset, recordLength);

    if (recordLength != 0) {
        if ((recordLength < 0) && (recordOffset < 0)) {
            // detected tombstone
            return 2;
        }
        memcpy(data, pageData + recordOffset, recordLength);
        return 0;
    }
    return -1;
}

RC Page::writeRecord(char *data, short length, unsigned int &slotNum) {
    short freeSpace = getFreeSpace();
    short freeSpaceOffset = getFreeSpaceOffset();
    short noOfSLots = getNoOfSLots();
    // check for deleted slots
    short emptySlotId = -1;
    short rOffset;
    short rLength;
    for (int i=1; i<=noOfSLots; i++) {
        getSlotInfo(i, rOffset, rLength);
        if ((rOffset == -1) && (rLength == 0)) {
            emptySlotId = i;
        }
    }

    if (emptySlotId != -1) {
        setSlotInfo(emptySlotId, freeSpaceOffset, length);
        slotNum = emptySlotId;

    } else {
        memcpy(pageData + PAGE_SIZE - FREE_OFFSET_LEN - FREE_SPACE_LEN - SLOT_CNT_LEN  - (4*(noOfSLots)) -2,&freeSpaceOffset,SLOT_OFFSET_LEN);
        memcpy(pageData + PAGE_SIZE - FREE_OFFSET_LEN - FREE_SPACE_LEN - SLOT_CNT_LEN - (4*(noOfSLots))-4,&length,SLOT_LENGTH_LEN);
        noOfSLots = noOfSLots + 1;
        slotNum = noOfSLots;
        memcpy(pageData + PAGE_SIZE - FREE_OFFSET_LEN - FREE_SPACE_LEN - SLOT_CNT_LEN, &noOfSLots, SLOT_CNT_LEN);
    }

    //add new records to freespace
    memcpy(pageData + freeSpaceOffset, data, length);

    // compute new values for freeSpaceOffset, freeSpace and no of slots
    freeSpaceOffset = freeSpaceOffset + length;
    freeSpace = freeSpace - length - SLOT_LENGTH_LEN - SLOT_OFFSET_LEN;

    setFreeSpace(freeSpace);
    setFreeSpaceOffset(freeSpaceOffset);

    if(!slotNum){
        return -1;
    }
    return 0;

}

RC Page::deleteRecord(unsigned int slotNum) {

    short recordOffset = 0;
    short recordLength = 0;
    short totalSlots;

    getSlotInfo(slotNum, recordOffset, recordLength);

    totalSlots = getNoOfSLots();
    short newOffset = recordOffset;
    short length = recordLength;

    // iterate over each slot to move the record and change their offsets
    for (int i = slotNum + 1; i <= totalSlots; i++) {
        getSlotInfo(i, recordOffset, recordLength);

        if (recordOffset >= newOffset) {

            // move the records from current position to new position
            moveData(newOffset, recordOffset, recordLength);

            setSlotInfo(i, newOffset, recordLength);
            newOffset += recordLength;
        }
    }
    // set offset and length to -1 for deleted slot
    recordOffset = -1;
    recordLength = 0;
    setSlotInfo(slotNum, recordOffset, recordLength);

    // update freeSpaceOffset, freeeSpace and no of slots
    setFreeSpaceOffset(newOffset);
    setFreeSpace(getFreeSpace() + length);

    return 0;
}

short Page::getSlotInfo(unsigned int slotNum, short &rOffset, short &rLength) {
    // This function returns the slot details(offset and length) for the record
    // extracting record offset and length from the page footer
    memcpy(&rOffset, pageData + PAGE_SIZE - FREE_OFFSET_LEN-FREE_SPACE_LEN-SLOT_CNT_LEN - (4 * slotNum) + 2, 2);
    memcpy(&rLength, pageData + PAGE_SIZE - FREE_OFFSET_LEN-FREE_SPACE_LEN-SLOT_CNT_LEN - (4 * slotNum), 2);
    return 0;
}

short Page::setSlotInfo(unsigned int slotNum, short &recordOffset, short &length) {
    // set offset and length values for slotNum (first len then offset)
    memcpy(pageData + PAGE_SIZE - FREE_OFFSET_LEN-FREE_SPACE_LEN-SLOT_CNT_LEN - (4 * slotNum) + 2, &recordOffset, 2);
    memcpy(pageData + PAGE_SIZE - FREE_OFFSET_LEN-FREE_SPACE_LEN-SLOT_CNT_LEN - (4 * slotNum), &length, 2);
    return -1;
}

short Page::moveData(short newOffset, short curOffset, short recordLength) {
    memcpy(pageData + newOffset , pageData + curOffset, recordLength);

}

short Page::setFreeSpaceOffset(short freeSpaceOffset) {
    memcpy( pageData + PAGE_SIZE - FREE_OFFSET_LEN,&freeSpaceOffset,SIZE_OF_SHORT);

}

short Page::setFreeSpace(short freeSpace){
    memcpy( pageData + PAGE_SIZE - FREE_OFFSET_LEN - FREE_SPACE_LEN, &freeSpace, SIZE_OF_SHORT);

}



/************ PAGE ************/

