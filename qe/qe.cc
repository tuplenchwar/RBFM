
#include "qe.h"
#include <iostream>
#include <stdlib.h>
#include <cfloat>
#include <string>
#include <vector>

std::string removeControlChars(const std::string& input) {
    std::string result;
    for (char c : input) {
        if (!iscntrl(static_cast<unsigned char>(c))) {
            result += c;
        }
    }
    return result;
}

void printMyString(void* ptr, int length) {
    char* charPtr = (char*)ptr; // Cast void* to char*
    printf("Printing string value in func - ");
    for (int i = 0; i < length; i++) {
        printf("%c", charPtr[i]);
    }
    printf("\n");
}


void Filter::readAttribute(const vector<Attribute> &recordDescriptor, string attrName, void *data, float &returnValue, string &returnString){

    int intVal;
    // float input_data_value;
    // float floatVal, returnValue;

    int nullsIndicator = ceil((float)recordDescriptor.size() / CHAR_BIT);
    int offset= nullsIndicator;

    cout<<"Inside agg ReadAttribute:"<<endl;

    cout<<"Attr.name is "<<attrName<<endl;

    for (int i = 0; i < recordDescriptor.size(); i++) {
        bool nullbit = ((char*)data)[i / 8] & ( 1 << ( 7 - ( i % 8 )));

        Attribute a = recordDescriptor[i];
        a.name = removeControlChars(a.name);
        cout<< "Attr name compare here ---- "<<(removeControlChars(a.name))<< " " << attrName << endl;
        cout<< "Attr name compare here ---- "<<(a.name).length() << " " << attrName.length() << endl;

        //if(recordDescriptor[i].name.compare(aggAttr.name) == 0){
        if((a.name)==(attrName)){
            cout<<"Attribute "<<recordDescriptor[i].name<<" is Matching"<<endl;
            if(nullbit){
                break;
            }
            if(recordDescriptor[i].type == TypeInt){
                memcpy(&intVal, ((char *) data + offset), sizeof(int));
                returnValue=float(intVal);
                // cout<<"INT value:"<<returnValue<<endl;
                break;
            }else if(recordDescriptor[i].type == TypeReal){
                memcpy(&intVal, ((char *) data + offset), sizeof(float));
                returnValue=float(intVal);
                //cout<<"FLT value:"<<&returnValue<<endl;
                break;
            }
            else if(recordDescriptor[i].type == TypeVarChar)
            {
                // read the length value of varchar field by de referencing
                int attributeLength;
                memcpy(&attributeLength, ((char *) data + offset), sizeof(int));
                // 4 bytes for storing the length of the varchar attribute
                offset += 4;

                char* charArray = new char[attributeLength];
                memcpy(charArray, (char *)data + offset, attributeLength);
                //std::string returnString;
                for (size_t s = 0; s < attributeLength; ++s) {
                    returnString += charArray[s];
                }
                std::cout << "read attribute returnString " << returnString << endl;
                offset = offset + attributeLength;

            }
        }
        else{
            cout<<"Attribute "<<a.name<<" is Not Matching"<<endl;
            if(!nullbit){
                if(recordDescriptor[i].type == TypeInt )
                    offset += sizeof(int);
                else if(recordDescriptor[i].type == TypeReal)
                    offset += sizeof(float);
                else{
                    int length;
                    memcpy(&length, ((char *) data + offset), sizeof(int));
                    offset += (sizeof(int) + length);
                }
            }
        }

    }
}
int getAttrStrLength(void* str) {
    char* s = static_cast<char*>(str);
    int length = 0;
    while (*s != '\0') {
        length++;
        s++;
    }
    return length;
}


RC Filter::checkCondition(void* attributeData, AttrType attributeType, void *value, CompOp compOp)
{
    //printVoidString(attributeData, attributeLen);
    if (attributeType == TypeVarChar) {
        cout << " in varchar check checkCondition " <<endl;


        int attributeLen = *(int *) ((char *) attributeData);
        cout << " checkCondition attributeLen " << attributeLen << endl;
        void *attributeString = (char *) malloc(attributeLen);
        memcpy((char *)attributeString, (char *)attributeData + 4, attributeLen);
        //attributeString[attributeLen] = '\0';
        printMyString(attributeString, attributeLen);
        int ValueLen = *(int *) ((char *) value);
        cout << " checkCondition ValueLen " << ValueLen<< endl;;
        void *valueString = (char *) malloc(ValueLen);
        memcpy((char *)valueString, (char *)value + 4, ValueLen);
        //attributeDataString[ValueLen] = '\0';
        printMyString(valueString, ValueLen);

        attributeLen =  (ValueLen > attributeLen) ? ValueLen : attributeLen;
        //std::string attributeString = std::string(static_cast<char*>(attributeDataString), attributeLen);
        //std::string valueString = std::string(static_cast<char*>(valueString), attributeLen);
        cout<< "String varchar values in check predicate "<< attributeString << " " << valueString << endl;
        //if(compOp == EQ_OP && memcmp(attributeData, value, attributeLen) != 0) return 1;
        if(compOp == EQ_OP && memcmp(attributeString, valueString, attributeLen) == 0) {return 1;}
        else if(compOp == LT_OP && memcmp(attributeString, valueString, attributeLen) < 0) return 1;
        else if(compOp == LE_OP && memcmp(attributeString, valueString, attributeLen) <= 0) return 1;
        else if(compOp == GT_OP && memcmp(attributeString, valueString, attributeLen) > 0) return 1;
        else if(compOp == GE_OP && memcmp(attributeString, valueString, attributeLen) >= 0) return 1;
        else if(compOp == NE_OP && memcmp(attributeString, valueString, attributeLen) != 0) return 1;
    }
    else if (attributeType == TypeInt) {

        //int attributeInt = *(int *)attributeData;
        int attributeInt;
        memcpy(&attributeInt, attributeData, SIZE_OF_INT);

        int valueInt = *(int *)value;
        cout<< "attributeInt "<< attributeInt<<endl;
        cout<< "valueInt "<< valueInt<<endl;

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
        cout<< "attributeFloat"<< attributeFloat<<endl;
        cout<< "valueFloat"<< valueFloat<<endl;

        if(compOp == EQ_OP && attributeFloat == valueFloat) return 1;
        else if(compOp == LT_OP && attributeFloat < valueFloat) return 1;
        else if(compOp == LE_OP && attributeFloat <= valueFloat) return 1;
        else if(compOp == GT_OP && attributeFloat > valueFloat) return 1;
        else if(compOp == GE_OP && attributeFloat >= valueFloat) return 1;
        else if(compOp == NE_OP && attributeFloat != valueFloat) return 1;
    }
    return -1;
}



BNLJoin::BNLJoin(Iterator* leftIn, TableScan* rightIn, const Condition& condition, const unsigned numPages)
{
    // Get attributes from input iterators
    this->leftIn = leftIn;
    this->rightIn = rightIn;
    this->numPages = numPages;
    this->condition = condition;
    leftIn->getAttributes(leftAttrs);
    rightIn->getAttributes(rightAttrs);
}

RC BNLJoin::readAttribute(vector<Attribute> Attrs, string Attr, void* &tuple, float &value, int &tupleSize) {

    int nullsIndicator = ceil((float)Attrs.size() / CHAR_BIT);
    tupleSize = nullsIndicator;
    bool flag = false;

    cout << "BNLJoin::readAttribute offset Attrs.size()" << tupleSize << Attrs.size() << endl;

    for (int i = 0; i < Attrs.size(); i++) {
        cout << "In for loop i" << i << endl;
        bool nullbit = ((char*)tuple)[i / 8] & ( 1 << ( 7 - ( i % 8 )));
        cout << "Attr " << Attr << "nullbit" << nullbit << endl;
        //if(recordDescriptor[i].name.compare(aggAttr.name) == 0){
        if(Attrs[i].name==Attr){
            cout<<"Attribute "<<Attrs[i].name<<" Matching"<<endl;
            if(nullbit){
                flag = true;
                break;
            }
            if(Attrs[i].type == TypeInt){
                memcpy(&value, ((char *) tuple + tupleSize), sizeof(int));
                tupleSize += sizeof(int);
//                value=float(intVal);
                // cout<<"INT value:"<<returnValue<<endl;
                break;
            }else if(Attrs[i].type == TypeReal){
                memcpy(&value, ((char *) tuple + tupleSize), sizeof(float));
                //cout<<"FLT value:"<<&returnValue<<endl;
                tupleSize += sizeof(float);
                break;
            }
        }
        else{
            cout<<"Attribute "<<Attrs[i].name<<" Not Matching"<<endl;
            if(!nullbit){
                if(Attrs[i].type == TypeInt )
                    tupleSize += sizeof(int);
                else if(Attrs[i].type == TypeReal)
                    tupleSize += sizeof(float);
                else{
                    int length;
                    memcpy(&length, ((char *) tuple + tupleSize), sizeof(int));
                    tupleSize += (sizeof(int) + length);
                }
            }
        }

    }
    if (flag){
        return -1;
    } else {
        return 0;
    }

}

void BNLJoin::combine(void* &leftTuple, void* &rightTuple, void* &combinedTuple) {
    cout << "combine function" <<endl;
    int totalnullbits = leftAttrs.size() + rightAttrs.size();
    int totalNullFieldsIndicator = ceil((double)totalnullbits/CHAR_BIT);

    int leftNullFieldIndicator = ceil((double)leftAttrs.size() /CHAR_BIT);
    int leftOffset = leftNullFieldIndicator;

    int rightNullFieldIndicator = ceil((double)rightAttrs.size()/CHAR_BIT);
    int rightOffset = rightNullFieldIndicator;

    unsigned char *finalNullsIndicator = (unsigned char *) malloc(totalNullFieldsIndicator);
    memset(finalNullsIndicator, 0, totalNullFieldsIndicator);

    for (int i=0; i < leftAttrs.size(); i++) {
        bool nullbit = ((char*)leftTuple)[i / 8] & ( 1 << ( 7 - ( i % 8 )));

        if (!nullbit) {
            cout << "leftAttrs[i].name " << leftAttrs[i].name << "is not null" << endl;
            // count the left tuple offset and set the nullIndicator
            if (leftAttrs[i].type == TypeInt) {
                leftOffset += sizeof(int);
            } else if (leftAttrs[i].type == TypeReal) {
                leftOffset += sizeof(float);
            } else if (leftAttrs[i].type == TypeVarChar) {
                int length;
                memcpy(&length, ((char *) leftTuple + leftOffset), sizeof(int));
                leftOffset += (sizeof(int) + length);
            }
        } else {
            cout << "leftAttrs[i].name " << leftAttrs[i].name << "is NULL" << endl;
            finalNullsIndicator[1 / 8] = finalNullsIndicator[1 / 8] | ( 1 << ( 7 - ( i % 8 )));
        }
    }

    for (int i=0; i < rightAttrs.size(); i++) {
        bool nullbit = ((char*)rightTuple)[i / 8] & ( 1 << ( 7 - ( i % 8 )));

        if (!nullbit) {
            cout << "rightAttrs[i].name " << rightAttrs[i].name << "is not null" << endl;
            // count the left tuple offset and set the nullIndicator
            if (rightAttrs[i].type == TypeInt) {
                rightOffset += sizeof(int);
            } else if (rightAttrs[i].type == TypeReal) {
                rightOffset += sizeof(float);
            } else if (rightAttrs[i].type == TypeVarChar) {
                int length;
                memcpy(&length, ((char *) rightTuple + rightOffset), sizeof(int));
                rightOffset += (sizeof(int) + length);
            }
        } else {
            cout << "rightAttrs[i].name " << rightAttrs[i].name << "is NULL" << endl;
            finalNullsIndicator[1 / 8] = finalNullsIndicator[1 / 8] | ( 1 << ( 7 - ( i % 8 )));
        }
    }

    int offset = totalNullFieldsIndicator;
    memcpy((char*)combinedTuple, finalNullsIndicator, totalNullFieldsIndicator);

    memcpy((char*)combinedTuple + offset, (char*)leftTuple + leftNullFieldIndicator, leftOffset - leftNullFieldIndicator);
    offset += leftOffset;
    memcpy((char*)combinedTuple + offset, (char*)rightTuple + rightNullFieldIndicator, rightOffset - rightNullFieldIndicator);
    offset += rightOffset;
    cout << "leftOffset " << leftOffset << endl;
    cout << "rightOffset " << rightOffset << endl;
    cout << "offset " << offset << endl;
    free(finalNullsIndicator);
}

std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = str.find(delimiter);
    while (end != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(delimiter, start);
    }
    tokens.push_back(str.substr(start));
    return tokens;
}


RC BNLJoin::getNextTuple(void *data) {
    cout << "IN BNLJoin::getNextTuple";


    int tuplesSize = 0;
    int totalTuplesSize = 0;
    void *leftTuple = (char*)malloc(PAGE_SIZE);
    void *rightTuple = (char*)malloc(PAGE_SIZE);
    vector<void *> leftTuples;

//    leftIn->getNextTuple(leftTuple);
//    rightIn->getNextTuple(rightTuple);
//    combine(leftTuple, rightTuple, data);
//    return 0;

    bool sent = false;

    std::vector<void *> resultantTuples;
    resultantTuples.reserve(PAGE_SIZE);
    if (extraLeftTuples.size() > 0) {
        leftTuple = extraLeftTuples.back();
        extraLeftTuples.pop_back();
        combine(leftTuple, rightTuple, data);
        return  0;
    }

    while (leftIn->getNextTuple(leftTuple) != QE_EOF) {

        for (int i = 0; i < leftAttrs.size(); i++) {
            string lhs = splitString(condition.lhsAttr, '.')[1];
            cout << "lhs " << lhs << "condition.lhsAttr" << condition.lhsAttr << endl;
            cout << "leftAttrs[i].name " << leftAttrs[i].name << endl;

            if (leftAttrs[i].name == condition.lhsAttr) {
                cout << "LHS matched " << lhs << endl;
                if (leftAttrs[i].type == TypeInt) {

                    float value;
                    RC rc = readAttribute(leftAttrs, condition.lhsAttr, leftTuple, value, tuplesSize);
                    if (rc == -1) {
                        // attribute is null bit
                        cout << "leftTuple attribute lhs " << condition.lhsAttr << "was null" << endl;

                        break;
                    }
                    cout << "value " << (int) value << endl;
                    int key = (int) value;

                    auto it = intHashMap.find(key);

                    if (it != intHashMap.end()) {
                        leftTuples = it->second;
                    }
                    leftTuples.push_back(leftTuple);
                    intHashMap.insert(pair<int, vector<void *> >(key, leftTuples));

                    totalTuplesSize += tuplesSize;
                    cout << "totalTuplesSize " << totalTuplesSize << "tuplesSize " << tuplesSize << endl;
                    // we have added numPages * PAGE_SIZE records in hashmap so block is ready
                    if (totalTuplesSize >= 900) {
                        cout << "wuthin while loop" << endl;
                        cout << "tuplesSize > maxHashMapSize" << endl;
                        int rightTuplesSize = 0;
                        int totalRightTuplesSize = 0;

                        while (totalRightTuplesSize < PAGE_SIZE) {
                            float rvalue;
                            vector<void *> result;
                            rightIn->getNextTuple(rightTuple);
                            cout << "loaded rightTuple " << endl;
//                            string rhs = splitString(condition.rhsAttr, '.')[1];
                            RC rc = readAttribute(rightAttrs, condition.rhsAttr, rightTuple, rvalue, rightTuplesSize);
                            if (rc == -1) {
                                // attribute is null bit
                                cout << "rightTuple attribute rhs " << condition.rhsAttr << "was null" << endl;

                                break;
                            }
                            cout << "rvalue for rhs " << rvalue << "rhs "  << endl;
                            totalRightTuplesSize += rightTuplesSize;
                            auto it = intHashMap.find(int(rvalue));
                            if (it != intHashMap.end()) {
                                result = it->second;
                            }
                            if (result.size() > 1 ) {
                                cout << "result.size() > 1" << endl;
                                extraLeftTuples = result;
                                leftTuple = result.back();
                                result.pop_back();
                            } else {
                                cout << "result.size() = 1 " << endl;
                                leftTuple = result[0];
                            }

                            currentRightTuple = rightTuple;
                            combine(leftTuple, rightTuple, data);
                            sent = true;
                            break;

                        } // loading rightTuples

                    }

                }


            }


        } // end of for
    } // end of while left
    if (totalTuplesSize >= 900) {
        cout << "hashmapsize " << intHashMap.size() << endl;
        cout << "tuplesSize > maxHashMapSize" << endl;
        int rightTuplesSize = 0;
        int totalRightTuplesSize = 0;

        while (totalRightTuplesSize < PAGE_SIZE) {
            float rvalue;
            vector<void *> result;
            rightIn->getNextTuple(rightTuple);
            cout << "loaded rightTuple " << endl;
//            string rhs = splitString(condition.rhsAttr, '.')[1];
            RC rc = readAttribute(rightAttrs, condition.rhsAttr, rightTuple, rvalue, rightTuplesSize);
            if (rc == -1) {
                // attribute is null bit
                cout << "rightTuple attribute rhs " << condition.rhsAttr << "was null" << endl;
                break;
            }
            totalRightTuplesSize += rightTuplesSize;
            cout << "rvalue for rhs " << (int)rvalue << "rhs " << endl;
            auto it = intHashMap.find(int(rvalue));
            if (it != intHashMap.end()) {
                result = it->second;
            }
            if (result.size() > 1) {
                cout << "result.size() > 1" << endl;
                extraLeftTuples = result;
                leftTuple = result.back();
                result.pop_back();
            } else {
                cout << "result.size() = 1 " << endl;
                leftTuple = result[0];
            }

            currentRightTuple = rightTuple;
            combine(leftTuple, rightTuple, data);
            sent = true;
            cout << "returned data " << endl;
            break;
        } // loading rightTuples
    }

//    free(leftTuple);
//    free(rightTuple);
//    intHashMap.clear();
    return 0;
}



// Method to get the attributes of the joined tuples
void BNLJoin::getAttributes(vector<Attribute>& attrs) const  {
// Concatenate attributes from leftIn and rightIn

    attrs.insert(attrs.end(), leftAttrs.begin(), leftAttrs.end());
    attrs.insert(attrs.end(), rightAttrs.begin(), rightAttrs.end());
}

BNLJoin::~BNLJoin() {
    delete leftIn; // Clean up leftIn iterator
    delete rightIn; // Clean up rightIn iterator
    intHashMap.clear();
}




// ... the rest of your implementations go here

Aggregate::Aggregate(Iterator *input, Attribute aggAttr, AggregateOp op) {

    //vector<Attribute> resultAttr;
    vector<Attribute> allAttr;
    this->op = op;
    this->aggAttr = aggAttr;

    input->getAttributes(allAttr);

    float result;
    int attrlength;

    //Create Output Attribute
    Attribute outAttr;
    outAttr.type = TypeReal;
    outAttr.length = sizeof(float);
    string name = "";
    if(op == MIN) name += "MIN";
    else if(op == MAX) name += "MAX";
    else if(op == COUNT) name += "COUNT";
    else if(op == SUM) name += "SUM";
    else if(op == AVG) name += "AVG";

    name += "("+aggAttr.name + ")";
    outAttr.name = name;
    cout<<"ATTR NAMR:"<<name<<endl;
    resultAttr.push_back(outAttr);

    //Initialize result value
    if(op == MIN){
        result = FLT_MAX;
    }
    else if(op == MAX){
        result = FLT_MIN;
    }
    else{
        result = 0.0;
    }

    int cnt=0;
    float returnValue;

    char *data= (char *)malloc(PAGE_SIZE);
   // void* attrValue = malloc(sizeof(float));

    while(input->getNextTuple(data) != QE_EOF){
        cnt++;
        //readAttribute(allAttr,aggAttr,data,returnValue);
        readValue(allAttr, aggAttr, data, attrlength);
        returnValue=floatVector.back();
        cout<<"Name->"<<aggAttr.name<<": Value->"<<returnValue<<endl;
        floatVector.clear();

        if(op == MIN){
            result = min(returnValue,result);
        }
        else if(op == MAX){
            result = max(returnValue,result);
        }
        else if(op == SUM){
            result += returnValue;
        }
        else if(op == AVG){
            result += returnValue;
        }
        else if(op == COUNT){
            result == cnt;
        }
    }

    cout<<"While Count->"<<cnt<<endl;

    if(op == AVG){
        result = result/cnt;
    }

    cout<<"Result"<<result<<endl;

    //check if the result is null
    char nullField = (cnt == 0) ? (1 << 7) : 0;

    //create aggregation with nullbitindicator and result value
    void *out = malloc(sizeof(char)+sizeof(float));
    memcpy(out,&nullField,sizeof(char));
    memcpy((char *)out+sizeof(char),&result,sizeof(float));


    pair<int, void*> pair1;
    pair1.first = sizeof(char) + sizeof(float);
    pair1.second = out;
    resultsQ.push(pair1);
}

Aggregate::Aggregate(Iterator *input, Attribute aggAttr, Attribute groupAttr, AggregateOp op) {

    int cnt=0, attrlength,grpNameLength;
    float returnValue, result, aggrValue, grpreturnValue;
    int grpValue;
    string groupName=groupAttr.name;
    string grpName;
    vector<Attribute> allAttr;
    unordered_map<int, int> grpCount;

    unordered_map<string, int> grpVCount;
    unordered_map<float, int> grpFCount;

    input->getAttributes(allAttr);

    for(int j=0;j<allAttr.size();j++){
        cout<<"Attr.name["<<j<<"]->"<<allAttr[j].name<<endl;
    }

    cout<<"getAttributes size "<<allAttr.size()<<endl;

    Attribute outAttr;
    outAttr.type = TypeReal;
    outAttr.length = sizeof(float);
    string name = groupAttr.name;
    if(op == MIN) name += "MIN";
    else if(op == MAX) name += "MAX";
    else if(op == COUNT) name += "COUNT";
    else if(op == SUM) name += "SUM";
    else if(op == AVG) name += "AVG";

    name += "("+aggAttr.name + ")";
    outAttr.name = name;
    cout<<"ATTR NAMR:"<<name<<endl;

    //resultAttr.push_back(outAttr);


    char *data= (char *)malloc(PAGE_SIZE);
    void* attrValue = malloc(200);

    while(input->getNextTuple(data) != QE_EOF){
        cout<<"Inside while"<<endl;
        cnt++;
        cout<<"count"<<cnt<<endl;


        cout<<"ReadValue for groupAttr "<<groupAttr.name<<endl;
        //cout<<"Grpvalue start"<<endl;
        readValue(allAttr, groupAttr, data, attrlength);
        if(groupAttr.type==TypeVarChar){
            grpName = varcharVector.back();
            cout<<"VgrpValue->"<<grpName<<endl;

        }else{
            grpValue = floatVector.back();
            cout<<"FgrpValue->"<<grpValue<<endl;
        }
        grpNameLength =attrlength;
        floatVector.clear();

        cout<<"ReadValue for aggAttr "<<aggAttr.name<<endl;
        readValue(allAttr, aggAttr, data, attrlength);
        aggrValue = floatVector.back();
        cout<<"FAttrvalue->"<<aggrValue<<endl;
        floatVector.clear();


     /* Orig
       readAttribute(allAttr,groupAttr,data,grpreturnValue);
        grpValue = grpreturnValue;

        readAttribute(allAttr,aggAttr,data,returnValue);
        aggrValue = returnValue;*/

        //cout<<"grpValue->"<<grpValue<<"aggrValue->"<<aggrValue<<":"<<endl;



        if(groupAttr.type==TypeVarChar){
            cout<<"Inside varchar if cond"<<endl;
            if (grpVAggrMap.find(grpName) == grpVAggrMap.end()){
                grpVCount[grpName] =1;
                if(op == COUNT){
                    grpVAggrMap[grpName] = 1;
                }
                else
                    grpVAggrMap[grpName] = aggrValue;
            }
            else{
                grpVCount[grpName] = grpVCount[grpName]+1;
                if(op == MIN){
                    grpVAggrMap[grpName]  = min(grpVAggrMap[grpName] , aggrValue);
                }
                else if(op == MAX){
                    grpVAggrMap[grpName] = max(grpVAggrMap[grpName], aggrValue);
                }
                else if(op == SUM){
                    grpVAggrMap[grpName] = grpVAggrMap[grpName] + aggrValue;
                }
                else if(op == AVG){
                    grpVAggrMap[grpName] = grpVAggrMap[grpName] + aggrValue;
                }
                else if(op == COUNT){
                    grpVAggrMap[grpName] = grpVAggrMap[grpName] + 1;
                }
            }
        }
        else{
            if (grpByMap.find(grpValue) == grpByMap.end()){
                grpCount[grpValue] =1;
                if(op == COUNT){
                    grpByMap[grpValue] = 1;
                }
                else
                    grpByMap[grpValue] = aggrValue;
            }
            else{
                grpCount[grpValue] = grpCount[grpValue]+1;
                if(op == MIN){
                    grpByMap[grpValue] = min(grpByMap[grpValue], aggrValue);
                }
                else if(op == MAX){
                    grpByMap[grpValue] = max(grpByMap[grpValue], aggrValue);
                }
                else if(op == SUM){
                    grpByMap[grpValue]= grpByMap[grpValue]+ aggrValue;
                }
                else if(op == AVG){
                    grpByMap[grpValue]= grpByMap[grpValue]+ aggrValue;
                }
                else if(op == COUNT){
                    grpByMap[grpValue]= grpByMap[grpValue]+ 1;
                }
            }
        }
    }

    if(op==AVG){
        if(groupAttr.type==TypeVarChar){
            for (auto it = grpVAggrMap.begin(); it != grpVAggrMap.end(); ++it){
                grpVAggrMap[it->first] = grpVAggrMap[it->first]/grpVCount[it->first];
            }
        }
        else{
            for (auto it = grpByMap.begin(); it != grpByMap.end(); ++it){
                grpByMap[it->first] = grpByMap[it->first]/grpCount[it->first];
            }
        }

    }

    cout << "Elements of the hash table:" << endl;
    int size,offset=0;
    if(groupAttr.type==TypeVarChar){
        for (auto it = grpVAggrMap.begin(); it != grpVAggrMap.end(); ++it) {

            int len = it->first.length();
            char nullField = 0;
            cout << "grpValue: " << it->first << ", aggrValue: " << it->second << endl;
            cout <<" grpValue len"<<len<<endl;

            //create aggregation with nullbitindicator and result value
            size=sizeof(char) + sizeof(int)+ len + sizeof(float);
            void *out = malloc(size);
            cout<<"tuple size:"<<size<<endl;

            memcpy(out, &nullField, sizeof(char));
            offset+=sizeof(char);
            memcpy((char *) out + offset, &len,sizeof(int));
            offset+=sizeof(int);
            memcpy((char *) out + offset, &it->first, len);
            offset+=len;
            memcpy((char *) out + offset, &it->second, sizeof(float));
            offset+=sizeof(float);

            pair<int, void *> pair1;
            pair1.first = size;
            pair1.second = out;
            resultsQ.push(pair1);
        }
    }
    else {
        for (auto it = grpByMap.begin(); it != grpByMap.end(); ++it) {
        cout << "grpValue: " << it->first << ", aggrValue: " << it->second << endl;
        char nullField = 0;

        //create aggregation with nullbitindicator and result value
        void *out = malloc(sizeof(char)+sizeof(float)+sizeof(float));
        memcpy(out,&nullField,sizeof(char));
        memcpy((char *)out+sizeof(char),&it->first,sizeof(float));
        memcpy((char *)out+sizeof(char)+ sizeof(float),&it->second,sizeof(float));


        pair<int, void*> pair1;
        pair1.first = sizeof(char)+sizeof(float)+sizeof(float);
        pair1.second = out;
        resultsQ.push(pair1);
        }
    }
}

Aggregate::~Aggregate() {}

void Aggregate::getAttributes(vector<Attribute> &attrs) const {
    attrs = this->resultAttr;
}

RC Aggregate::getNextTuple(void * data) {
    //pair<int, void *> pair1 = resultsQ.pop();
    if(resultsQ.empty())
        return QE_EOF;
    pair<int, void *> pair2 = resultsQ.front();
    cout<<"AAAAAAA size:"<<pair2.first<<endl;
    memcpy(data, pair2.second, pair2.first);
    resultsQ.pop();

    return 0;
}

void Aggregate :: readValue(const vector<Attribute> &recordDescriptor, Attribute attr, void *data, int &attrLength){

    //string readString;
    char *readString = (char *)malloc(1);

    int nullsIndicator = ceil((float)recordDescriptor.size() / CHAR_BIT);
    int offset= nullsIndicator;

    cout<<"INSIDE READ VALUE"<<endl;
    cout<<"Attr.name:"<<attr.name<<endl;
    int attrLen = attr.name.length();

    for (int i = 0; i < recordDescriptor.size(); i++) {
        bool nullbit = ((char*)data)[i / 8] & ( 1 << ( 7 - ( i % 8 )));

        //if(recordDescriptor[i].name == attr.name){
       if(memcmp(recordDescriptor[i].name.c_str(), attr.name.c_str(), attrLen) == 0) {
                matchStatus = 1;
               // cout<<"Attribute "<<recordDescriptor[i].name<<"->"<<matchStatus<<endl;
                if(nullbit){
                    break;
                }
                if(recordDescriptor[i].type == TypeInt){
                    int intVal;
                    memcpy(&intVal, ((char *) data + offset), sizeof(int));
                    floatVector.push_back(intVal);
                    attrLength=sizeof(int);
                   // cout<<"INT value:"<<intVal<<":"<<attrLength<<endl;
                    break;
                }else if(recordDescriptor[i].type == TypeReal){
                    float floatVal;
                    memcpy(&floatVal, ((char *) data + offset), sizeof(float));
                    attrLength=sizeof(float);
                    floatVector.push_back(floatVal);
                   // cout<<"FLT value:"<<floatVal<<":"<<attrLength<<endl;
                    break;
                }else if(recordDescriptor[i].type == TypeVarChar){
                    int nameLength;
                    memcpy(&nameLength, (char *)data + offset, sizeof(int));
                    offset += sizeof(int);
                    memcpy(readString, (char *)data+offset, nameLength);
                    cout<<"Varchar value by new:"<<readString<<":"<<nameLength<<endl;

                    varcharVector.push_back(readString);
                    break;
                }
        }
        else{
            matchStatus = 0;
            //cout<<"Attribute "<<recordDescriptor[i].name.c_str()<<"->"<<matchStatus<<endl;
            if(!nullbit){
                if(recordDescriptor[i].type == TypeInt )
                    offset += sizeof(int);
                else if(recordDescriptor[i].type == TypeReal)
                    offset += sizeof(float);
                else{
                    int length;
                    memcpy(&length, ((char *) data + offset), sizeof(int));
                    offset += (sizeof(int) + length);
                }
            }
        }
    }
}


RC Filter::attributesSatisfyingPredicate(void *recordDataBuffer, const vector<Attribute> &recordDescriptor, string conditionAttribute, CompOp compOp, void *value, const vector<string> &attributeNames, void* returnData){
    short offset = 0;
    int fieldCount = 0;
    // predicateHolds = 0 => attribute not found yet
    // 1 => holds true
    // -1 => does not hold true
    short predicateHolds = 0;

    // get field count from first 2 bytes of the record
    memcpy(&fieldCount, ((char *) recordDataBuffer + offset), SIZE_OF_SHORT);
    offset += SIZE_OF_SHORT;
    short nullFieldsIndicatorSize = ceil((double) fieldCount / CHAR_BIT);
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorSize);
    memcpy(nullsIndicator, ((char *) recordDataBuffer + offset), nullFieldsIndicatorSize);
    offset += nullFieldsIndicatorSize;
    //string conditionAttribute = attributeNames;
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
                    //printVoidString(attributeData, sizeof(charLength));
                    //predicateHolds = checkPredicate(attributeData, fieldLengthOffset - prevOffset, a.type);
                    predicateHolds = checkCondition(attributeData, condition.rhsValue.type, condition.rhsValue.data, condition.op);

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
        memcpy((char *) returnData, newDataNullFieldsIndicator, newDataNullFieldsIndicatorSize);
        memcpy((char *) returnData + newDataNullFieldsIndicatorSize, (char *) newBufferData, newBufferRecordLength);
        //break; // if attribute condition holds true then break out of slot for loop
    }

    return predicateHolds;

}


Filter::Filter(Iterator* input, const Condition &condition) {
    this->input = input;
    this->condition = condition;
    input->getAttributes(attrs);
}

RC Filter::getNextTuple(void *data) {

    while(input->getNextTuple(data)!= -1){
         if(condition.op == NO_OP){
             return 0;
         }
        void *attributeData = malloc(200);
        float attributeValueNum;
        string attributeValueStr;

         readAttribute(attrs, condition.lhsAttr, data, attributeValueNum, attributeValueStr);
        /*
         vector<string> attributeNames;
        attributeNames.push_back(condition.lhsAttr);

        // predicateHolds = 0 => attribute not found yet
        // 1 => holds true
        // -1 => does not hold true
        short predicateHolds = 0;

        predicateHolds = attributesSatisfyingPredicate(data, attrs, condition.lhsAttr, condition.op, condition.rhsValue.data, attributeNames, attributeData);

        ////////////////////////////////////////////////////////////////////////////////
        short nullAttributesIndicatorActualSize = ceil((double) attributeNames.size() / CHAR_BIT);
        //memcpy(&tableID,(char*)newDataBuffer+nullAttributesIndicatorActualSize,sizeof(int));
        //short nullIndicatorSize = 1; //for now setting 1 as we know there are only 4 column descriptors and assume none of them are null
        unsigned attrInt = *(int *)((char *)attributeData+nullAttributesIndicatorActualSize);
        cout << "In filter next tuple attr int " <<attrInt<<endl;

        char*  attrName = new char[attrNameLen];
        short newDataPointer = nullAttributesIndicatorActualSize + SIZE_OF_INT;
        strcpy(attrName, (char *)newDataBuffer+newDataPointer);
        tableAttribute.name = attrName;
        delete[] attrName;
        newDataPointer += attrNameLen;
        cout << "In create descriptor function newDataPointer " <<newDataPointer<<endl;

        ///////////////////////////////////////////////////////////////////////////////
        */
         //memcpy(attributeData, &attributeValue, SIZE_OF_INT);
        // predicateHolds = 0 => attribute not found yet
        // 1 => holds true
        // -1 => does not hold true
        short predicateHolds = 0;

        if(condition.rhsValue.type == TypeInt | condition.rhsValue.type == TypeReal){
             cout<< "Value returned by readAttribute "<< attributeValueNum<<endl;

             int attributeInt;
             *(int *) attributeData = attributeValueNum;
             memcpy(&attributeInt, attributeData, SIZE_OF_INT);
             cout<< "Get next tuple attributeInt value = "<< attributeInt<<endl;
             predicateHolds = checkCondition(attributeData, condition.rhsValue.type, condition.rhsValue.data, condition.op);

         }
        else if (condition.rhsValue.type == TypeVarChar){
             cout<< "Value returned by readAttribute "<< attributeValueStr<<endl;

             string attributeString;
             int length = attributeValueStr.length();
             attributeData = malloc(4+length);
             *(int *) ((char *) attributeData) = length;
             for (unsigned i = 0; i < length; ++i) {
                 *(char *) ((char*) attributeData + 4 + i) = attributeValueStr[i];
             }
             memcpy(&attributeString, (char*)attributeData+4, length);
            cout<< "Get next tuple string value = "<< attributeString<< " len = "<< length<<endl;
             predicateHolds = checkCondition(attributeData, condition.rhsValue.type, condition.rhsValue.data, condition.op);
             cout<< "predicateHolds returned "<< predicateHolds <<endl;
         }

         if(predicateHolds == 1){
             return 0;
         }
        free(attributeData);

    }
     return QE_EOF;
}

void readAttribute(const vector<Attribute> &recordDescriptor, string attrName, void *data, float &returnValue, string &returnString){

    int intVal;
    // float input_data_value;
    // float floatVal, returnValue;

    int nullsIndicator = ceil((float)recordDescriptor.size() / CHAR_BIT);
    int offset= nullsIndicator;

    cout<<"Inside agg ReadAttribute:"<<endl;

    cout<<"Attr.name is "<<attrName<<endl;

    for (int i = 0; i < recordDescriptor.size(); i++) {
        bool nullbit = ((char*)data)[i / 8] & ( 1 << ( 7 - ( i % 8 )));

        Attribute a = recordDescriptor[i];
        a.name = removeControlChars(a.name);
        cout<< "Attr name compare here ---- "<<(removeControlChars(a.name))<< " " << attrName << endl;
        cout<< "Attr name compare here ---- "<<(a.name).length() << " " << attrName.length() << endl;

        //if(recordDescriptor[i].name.compare(aggAttr.name) == 0){
        if((a.name)==(attrName)){
            cout<<"Attribute "<<recordDescriptor[i].name<<" is Matching"<<endl;
            if(nullbit){
                break;
            }
            if(recordDescriptor[i].type == TypeInt){
                memcpy(&intVal, ((char *) data + offset), sizeof(int));
                returnValue=float(intVal);
                // cout<<"INT value:"<<returnValue<<endl;
                break;
            }else if(recordDescriptor[i].type == TypeReal){
                memcpy(&intVal, ((char *) data + offset), sizeof(float));
                returnValue=float(intVal);
                //cout<<"FLT value:"<<&returnValue<<endl;
                break;
            }
            else if(recordDescriptor[i].type == TypeVarChar)
            {
                // read the length value of varchar field by de referencing
                int attributeLength;
                memcpy(&attributeLength, ((char *) data + offset), sizeof(int));
                // 4 bytes for storing the length of the varchar attribute
                offset += 4;

                char* charArray = new char[attributeLength];
                memcpy(charArray, (char *)data + offset, attributeLength);
                //std::string returnString;
                for (size_t s = 0; s < attributeLength; ++s) {
                    returnString += charArray[s];
                }
                std::cout << "read attribute returnString " << returnString << endl;
                offset = offset + attributeLength;

            }
        }
        else{
            cout<<"Attribute "<<a.name<<" is Not Matching"<<endl;
            if(!nullbit){
                if(recordDescriptor[i].type == TypeInt )
                    offset += sizeof(int);
                else if(recordDescriptor[i].type == TypeReal)
                    offset += sizeof(float);
                else{
                    int length;
                    memcpy(&length, ((char *) data + offset), sizeof(int));
                    offset += (sizeof(int) + length);
                }
            }
        }

    }
}

Project::Project(Iterator *input, const vector <std::string> &attrNames) {
    this->input = input;
    this->attrNames = attrNames;
    input->getAttributes(attrs);
}
RC Project::getNextTuple(void *data) {
    void *attributeData = malloc(200);
    void *tupleData = malloc(PAGE_SIZE);
    while(input->getNextTuple(tupleData)!= -1){
        for(int i=0; i<attrNames.size(); i++) {
            string attributeValueStr;
            float attributeValueNum;
            readAttribute(attrs, attrNames[i], tupleData, attributeValueNum, attributeValueStr);
            cout<< "Value returned by readAttribute "<< attributeValueStr<<endl;
//            unsigned char *newDataNullFieldsIndicator = (unsigned char *) malloc(newDataNullFieldsIndicatorSize);
//            memset(nullsIndicator, 0, newDataNullFieldsIndicatorSize);

            int whichByte = i / 8;
            int whichBit = i % 8;
//            newDataNullFieldsIndicator[whichByte] += pow(2, 7 - whichBit);

        }
        //write into data

    }
    free(attributeData);
    free(tupleData);
    return QE_EOF;
}



