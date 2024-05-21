#include "rm.h"
#include <string.h>


//RecordBasedFileManager * rbfm = RecordBasedFileManager::instance();

RelationManager* RelationManager::instance()
{
    static RelationManager _rm;
    return &_rm;
}

RelationManager::RelationManager()
{
    rbfm = RecordBasedFileManager::instance();
}

RelationManager::~RelationManager()
{
}

RC RelationManager::createCatalog()
{
    rbfm -> createFile("Tables");
    rbfm -> createFile("Columns");

    rbfm ->openFile("Tables",tableFileHandle);
    rbfm ->openFile("Columns",columnFileHandle);

    createTable("Tables", createTableDescriptor());
    createTable("Columns",createColumnDescriptor());

    return 0;
}

RC RelationManager::deleteCatalog()
{
    rbfm ->destroyFile("Tables");
    rbfm ->destroyFile("Columns");
    return 0;
}

RC RelationManager::createTable(const string &tableName, const vector<Attribute> &attrs)
{
    //Only tables file should be created which are not catalog tables.

    rbfm ->openFile("Tables",tableFileHandle);
    rbfm ->openFile("Columns",columnFileHandle);

    nextTableId = rbfm ->getNewTableId(tableFileHandle);

    if(tableName!="Tables" && tableName!="Columns")
        rbfm ->createFile(tableName);

    RID rid;

    //Insert table record in catalog table file
    rbfm ->insertRecord(tableFileHandle, createTableDescriptor(),  createTableRecord(nextTableId, tableName), rid);

    //Insert records for all table attributes in catalog column file
    for (unsigned i = 0; i < attrs.size(); i++)
    {
        cout<<"Attr details::"<<attrs.size()<<":"<<attrs[i].name<<":"<<attrs[i].type<<endl;
        cout<<"nextTableId"<<nextTableId<<endl;
        rbfm ->insertRecord(columnFileHandle, createColumnDescriptor(),  createColumnRecord(nextTableId, attrs[i], i+1), rid);
    }
    cout<<"After Column record insertion"<<endl;
    nextTableId++;
    return 0;
}

RC RelationManager::deleteTable(const string &tableName)
{
    //Check if the table is catalog table
    if(tableName == "Tables" || tableName == "Columns")
        return -1;

    string fileName = tableName;
    /*
    int tableId;

    RM_ScanIterator rmsi;
    RID rid;
    vector<string> attributeNames;

    //Fetch TableId
    if(RelationManager::fetchTableID(tableName, tableId)!=0){
        return -1;
    }
    tableId += 1;
    void* tableID = malloc(SIZE_OF_INT);
    memcpy(tableID, &tableId, SIZE_OF_INT);


    //For catalog table record
    attributeNames.push_back("table-name");


    scan("Tables","table-name",EQ_OP,tableName.c_str(),attributeNames, rmsi);
    char tableData[PAGE_SIZE];

    while(rmsi.getNextTuple(rid,tableData) != RM_EOF){
        if(rbfm->deleteRecord(tableFileHandle,createTableDescriptor(),rid) != 0){
            return -1;
        }
    }



    attributeNames.clear();
    //free(tableData);


    attributeNames.push_back("column-name");


    scan("Columns","table_id",EQ_OP,tableID,attributeNames, rmsi);
    char columnData[PAGE_SIZE];

    while(rmsi.getNextTuple(rid,tableData) != RM_EOF){
        if(rbfm->deleteRecord(columnFileHandle,createColumnDescriptor(),rid) != 0){
            return -1;
        }
    }

    free(tableData);
    free(tableID);

    */
    //Delete User Table File
    if(rbfm -> destroyFile(fileName) != 0){
        return -1;
    }

    return 0;

}

//getAttributes of table
RC RelationManager::getAttributes(const string &tableName, vector<Attribute> &attrs)
{
    if (tableName == "Tables") {
        attrs = createTableDescriptor();
    }
    else if (tableName == "Columns") {
        attrs = createColumnDescriptor();
    }
    else {
//        streambuf* orig_buf = cout.rdbuf();
//        cout.rdbuf(NULL);

        // return -1;
        int tableID;
        // fetch the tableID corresponding to the tableName in "Tables" catalog
        if (RelationManager::fetchTableID(tableName, tableID) != 0) {
            cout << "FETCHTABLEID FAILED" << endl;
            return -1;
        }
//        cout.rdbuf(orig_buf);

        cout << " IN GET ATTRIBUTES Table id returned from fetchTableID " << tableID << endl;

        // form the record descriptor for the fetched tableID using "Columns" catalog
        if (RelationManager::createTableRecordDescriptor(tableID, attrs) != 0) {
            return -1;
        }
    }
    cout << "get attributes size" << attrs.size() << endl;
    return 0;
}

RC RelationManager::fetchTableID(const string &tableName, int& tableID)
{
    //return -1;
    // use rbfm scan iterator to get scan iterator over the "Tables" file to get the ID of the table
    cout << "IN FETCHTABLEID " << endl;
    RecordBasedFileManager* tables = RecordBasedFileManager::instance();
    FileHandle tablesFile;
    if (tables->openFile("Tables",tablesFile) != 0){
        return -1;
    }
    // use rbfm scan iterator to get scan iterator over the "Tables" file to get the ID of the table
    void* value = malloc(tableName.size());
    memcpy((char*)value, tableName.c_str(), tableName.size());
    std::vector<std::string>  attributeNames;
    attributeNames.push_back("table-id");

    RBFM_ScanIterator rbfmsi;
    vector<Attribute> tableRecordDescriptor = createTableDescriptor();

    rbfm->scan(tablesFile,tableRecordDescriptor,"table-name",EQ_OP,value,attributeNames, rbfmsi);

    cout << endl << "Scanning for fetchtable" << endl;


    RID rid;
    void* newDataBuffer = malloc(SIZE_OF_INT);
    if(rbfmsi.getNextRecord(rid, newDataBuffer) != RBFM_EOF){
        cout << endl << "getNextRecord for fetchtable" << endl;
        int nullAttributesIndicatorActualSize = ceil((double) attributeNames.size() / CHAR_BIT);
        cout << endl << "nullAttributesIndicatorActualSize" << endl;
        memcpy(&tableID,(char*)newDataBuffer+nullAttributesIndicatorActualSize,sizeof(int));
        cout << " FETCH ID TABLE ID" << tableID << endl;
        free(value);
        free(newDataBuffer);
        return 0;
    }
    free(value);
    free(newDataBuffer);
    return 0;
}
RC RelationManager::createTableRecordDescriptor(int tableID, vector<Attribute> &attrs)
{
    //return -1;
    // use rbfm scan iterator to get scan iterator over the "Columns" file to get the columns/attributes of the table identifies by tableID
    cout<< "In createTableRecordDescriptor" << endl;
    RecordBasedFileManager* columns = RecordBasedFileManager::instance();
    FileHandle columnsFile;
    if (columns->openFile("Columns",columnsFile) != 0){
        return -1;
    }

    void* value = malloc(SIZE_OF_INT);
    memcpy(value, &tableID, SIZE_OF_INT);
    vector<string> attributeNames;
    attributeNames.push_back("column-name");
    attributeNames.push_back("column-type");
    attributeNames.push_back("column-length");
    attributeNames.push_back("column-position");

    RBFM_ScanIterator rbfmsi;
    vector<Attribute> columnRecordDescriptor = createColumnDescriptor();
    RC scanResult = rbfm->scan(columnsFile,columnRecordDescriptor,"table-id",EQ_OP,value,attributeNames, rbfmsi);
    if(scanResult == -1){
        return -1;
    }
    cout<< "after scan" << endl;
    RID rid;
    void* newDataBuffer = malloc(PAGE_SIZE); // To do change malloc size to adjust the column attribute data
    while(rbfmsi.getNextRecord(rid, newDataBuffer) != RBFM_EOF){
        cout<< "WHILE RID" << rid.pageNum << " "<<rid.slotNum <<endl;
        Attribute tableAttribute;
        short nullAttributesIndicatorActualSize = ceil((double) attributeNames.size() / CHAR_BIT);
        //memcpy(&tableID,(char*)newDataBuffer+nullAttributesIndicatorActualSize,sizeof(int));
        //short nullIndicatorSize = 1; //for now setting 1 as we know there are only 4 column descriptors and assume none of them are null
        unsigned attrNameLen = *(int *)((char *)newDataBuffer+nullAttributesIndicatorActualSize);
        cout << "In create descriptor function attrNameLen " <<attrNameLen<<endl;
        char*  attrName = new char[attrNameLen];
        short newDataPointer = nullAttributesIndicatorActualSize + SIZE_OF_INT;
        strcpy(attrName, (char *)newDataBuffer+newDataPointer);
        tableAttribute.name = attrName;
        delete[] attrName;
        newDataPointer += attrNameLen;
        cout << "In create descriptor function newDataPointer " <<newDataPointer<<endl;
        AttrType attrType;
        memcpy(&attrType, (char *)newDataBuffer+newDataPointer, sizeof(AttrType));
        tableAttribute.type = attrType;
        newDataPointer += sizeof(AttrType);
        int attrLen = *(int *)((char *)newDataBuffer+newDataPointer);
        cout << "In create descriptor function  attrType" <<attrLen<<endl;
        tableAttribute.length = attrLen;
        cout << "PRINTING ATTRIBUTES for tableID " << tableID<< endl;
        cout << "tableAttribute.name " << tableAttribute.name << endl;
        cout << "tableAttribute.type " << tableAttribute.type << endl;
        cout << "tableAttribute.length " << tableAttribute.length << endl;

        attrs.push_back(tableAttribute);
        cout<< "WHILE end after attr pushback RID" << rid.pageNum << " "<<rid.pageNum <<endl;

    }
    //free(value);
    //free(newDataBuffer);
    rbfmsi.close();
    return 0;

}

RC RelationManager::insertTuple(const string &tableName, const void *data, RID &rid)
{
    //return -1;
    // use getAttributes to get attribute and record Descriptor
    // use RBFM insertRecord to insert tuple
    FileHandle fileHandle;
    if(rbfm->openFile(tableName, fileHandle) != 0){
        return -1;
    }
    RelationManager* rm = RelationManager::instance();
    vector<Attribute> attrs;
    if(rm->getAttributes(tableName, attrs) != 0){
        return -1;
    }
    if(rbfm->insertRecord(fileHandle, attrs, data, rid) != 0){
        return -1;
    }
    if(rbfm->closeFile(fileHandle) != 0){
        return -1;
    }
    return 0;
}

RC RelationManager::deleteTuple(const string &tableName, const RID &rid)
{
    //return -1;
    // use getAttributes to get attribute and record Descriptor
    // use RBFM deleteRecord to delete tuple
    FileHandle fileHandle;
    if(rbfm->openFile(tableName, fileHandle) != 0){
        return -1;
    }
    RelationManager* rm = RelationManager::instance();
    vector<Attribute> attrs;
    if(rm->getAttributes(tableName, attrs) != 0){
        return -1;
    }
    if(rbfm->deleteRecord(fileHandle, attrs, rid) != 0){
        return -1;
    }
    if(rbfm->closeFile(fileHandle) != 0){
        return -1;
    }
    return 0;


}

RC RelationManager::updateTuple(const string &tableName, const void *data, const RID &rid)
{
    FileHandle fileHandle;
    if(rbfm->openFile(tableName, fileHandle) != 0){
        return -1;
    }
    RelationManager* rm = RelationManager::instance();
    vector<Attribute> attrs;
    if(rm->getAttributes(tableName, attrs) != 0){
        return -1;
    }
    if(rbfm->updateRecord(fileHandle, attrs, data, rid) != 0){
        return -1;
    }
    if(rbfm->closeFile(fileHandle) != 0){
        return -1;
    }
    return 0;

}

RC RelationManager::readTuple(const string &tableName, const RID &rid, void *data)
{
    //return -1;
    FileHandle fileHandle;
    if(rbfm->openFile(tableName, fileHandle) != 0){
        return -1;
    }
    RelationManager* rm = RelationManager::instance();
    vector<Attribute> attrs;
    cout<< "before get attributes "<< endl;
    if(rm->getAttributes(tableName, attrs) != 0){
        return -1;
    }

    cout<< "before read record "<< endl;

    if(rbfm->readRecord(fileHandle, attrs, rid, data) != 0){
        return -1;
    }
    cout<< "after read record "<< endl;

    if(rbfm->closeFile(fileHandle) != 0){
        return -1;
    }
    return 0;

}

RC RelationManager::printTuple(const vector<Attribute> &attrs, const void *data)
{

    if(rbfm->printRecord(attrs, data) != 0){
        return -1;
    }
    if(rbfm->closeFile(fileHandle) != 0){
        return -1;
    }
    return 0;

}

RC RelationManager::readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data)
{

    FileHandle fileHandle;
    if(rbfm->openFile(tableName, fileHandle) != 0){
        return -1;
    }
    RelationManager* rm = RelationManager::instance();
    vector<Attribute> attrs;
    if(rm->getAttributes(tableName, attrs) != 0){
        return -1;
    }
    if(rbfm->readAttribute(fileHandle, attrs, rid, attributeName, data) != 0){
        return -1;
    }
    if(rbfm->closeFile(fileHandle) != 0){
        return -1;
    }
    return 0;

}

RC RelationManager::scan(const string &tableName,
      const string &conditionAttribute,
      const CompOp compOp,                  
      const void *value,                    
      const vector<string> &attributeNames,
      RM_ScanIterator &rm_ScanIterator)
{
   vector<Attribute> recordDescriptor;

   if (tableName == "Tables") {
       recordDescriptor = createTableDescriptor();
   } else if (tableName == "Columns") {
       recordDescriptor = createColumnDescriptor();
       }
   else {
       if (getAttributes(tableName, recordDescriptor) != 0) {
           return -1;
       }
   }

    RecordBasedFileManager* rbfm_in_rm = RecordBasedFileManager::instance();
    FileHandle scan_fileHandle;
    if (rbfm_in_rm->openFile(tableName,scan_fileHandle) != 0){
        return -1;
    }
   RC rc = rbfm_in_rm->scan(scan_fileHandle, recordDescriptor, conditionAttribute, compOp, value, attributeNames, rm_ScanIterator.rbfmsi);
   return rc;
}
/*
RC RM_ScanIterator::getNextTuple(RID &rid, void* data) {
    cout<< "In getNextTuple " << rid.pageNum << "  "<< rid.slotNum<<endl;
    RC rbfmsi_result = rbfmsi.getNextRecord(rid, data);
    if (rbfmsi_result == RBFM_EOF) {
        return RM_EOF;
    }
    return rbfmsi_result;
}

RC RM_ScanIterator::close() {
    if (rbfmsi.close() != 0) {
        return -1;
    }
    return 0;
}
*/

// Extra credit work
RC RelationManager::dropAttribute(const string &tableName, const string &attributeName)
{
    return -1;
}

// Extra credit work
RC RelationManager::addAttribute(const string &tableName, const Attribute &attr)
{
    return -1;
}

std::vector<Attribute> RelationManager::createTableDescriptor()
{
    std::vector<Attribute> recordDescriptor;
    Attribute attr;
    attr.name = "table-id";
    attr.type = TypeInt;
    attr.length = (AttrLength)4;
    recordDescriptor.push_back(attr);

    attr.name = "table-name";
    attr.type = TypeVarChar;
    attr.length = (AttrLength)50;
    recordDescriptor.push_back(attr);

    attr.name = "file-name";
    attr.type = TypeVarChar;
    attr.length = (AttrLength)50;
    recordDescriptor.push_back(attr);

    return recordDescriptor;
}

std::vector<Attribute> RelationManager::createColumnDescriptor()
{
    std::vector<Attribute> recordDescriptor;
    Attribute attr;
    attr.name = "table-id";
    attr.type = TypeInt;
    attr.length = (AttrLength)4;
    recordDescriptor.push_back(attr);

    attr.name = "column-name";
    attr.type = TypeVarChar;
    attr.length = (AttrLength)50;
    recordDescriptor.push_back(attr);

    attr.name = "column-type";
    attr.type = TypeInt;
    attr.length = (AttrLength)4;
    recordDescriptor.push_back(attr);

    attr.name = "column-length";
    attr.type = TypeInt;
    attr.length = (AttrLength)4;
    recordDescriptor.push_back(attr);

    attr.name = "column-position";
    attr.type = TypeInt;
    attr.length = (AttrLength)4;
    recordDescriptor.push_back(attr);

    return recordDescriptor;
}

char* RelationManager:: createTableRecord(int &tableId, const string &tableName)
{
    char * data = (char*) calloc(1,PAGE_SIZE);

    //set nullBitIndicator to zero
    int offset=1;


    //First byte is nulls indicator, in our case nothing is null
    memcpy(data + offset, &tableId,sizeof(int));
    offset += sizeof(int);


    //Insert Table length
    int tableNameLen=tableName.length();
    memcpy(data +offset, &tableNameLen, sizeof(int));
    offset += sizeof(int);

    //Insert TableName
    memcpy(data + offset,tableName.c_str(),tableNameLen);
    offset += tableNameLen;

    //Insert Filename Length
    memcpy(data +offset, &tableNameLen, sizeof(int));
    offset += sizeof(int);

    //Insert Filename
    memcpy(data + offset,tableName.c_str(),tableNameLen);

    return data;
}


char* RelationManager:: createColumnRecord(int &tableId, Attribute attr, const int &position)
{
    char * data = (char*) calloc(1,PAGE_SIZE);

    //set nullBitIndicator to zero
    int offset = 1;

    //Insert tableId
    memcpy(data + offset, &tableId, sizeof(int));
    offset += sizeof(int);

    //Insert ColumnName Length
    int columnNameLength = attr.name.length();
    memcpy(data+offset, &columnNameLength, sizeof(int));
    offset += sizeof(int);

    //Insert ColumnName
    memcpy(data+offset, attr.name.c_str(), columnNameLength);
    offset += columnNameLength;

    //Insert ColumnType
    memcpy(data+offset, &attr.type, sizeof(int));
    offset += sizeof(int);

    //Insert Column Size
    memcpy(data+offset, &attr.length, sizeof (int));
    offset += sizeof(int);

    //Insert Column position
    memcpy(data+offset, &position, sizeof (int));

    return data;
}

