#include "picoNodeVectorClass.h"

picoNodeVector::picoNodeVector(int picoID = 0){

    this->nodeID = picoID;

    std::vector<int> nodeIDVector;

    nodeIDVector.push_back(nodeID);

}



int picoNodeVector::addItem(int picoID){

    for(int i = 0; i < nodeIDVector.size(); i++){
        if(nodeIDVector[i] == picoID){
            return -2;
        }
    }

    nodeIDVector.push_back(picoID);

    vectorSize = vectorSize + 1;

    return 0;
}

int picoNodeVector::removeItem(int picoID){

    for(int i = 0; i < nodeIDVector.size(); i++){
        if(nodeIDVector[i] == picoID){

            if(nodeIDVector[i] == nodeID){
            
                return -3;

            }

            nodeIDVector.erase(nodeIDVector.begin() + i);

            vectorSize = vectorSize - 1;

            return 0;
        }
    }

    return -4;
}

int picoNodeVector::handleOperation(int operationType, int picoID){
    
    if(operationType = 0){
        return addItem(picoID);
    }

    else if(operationType = 1){
        return removeItem(picoID);
    }

    else{
        return -1;
    }

}



bool picoNodeVector::compareWithCurrentList(std::vector<int> inputVector){

    bool itemFound = false;

    if(nodeIDVector.size() != inputVector.size()){
        return false;
    }

    for(int i = 0; i < inputVector.size(); i++){

        itemFound = false;

        for(int j = 0; j < nodeIDVector.size(); j++){

            if(nodeIDVector[j] == inputVector[i]){
                itemFound = true;
            }

        }

        if(itemFound == false){
            return false;
        }
    }

    return true;

}



void picoNodeVector::clearList(){

    nodeIDVector.clear();
    nodeIDVector.push_back(nodeID);

    vectorSize = 1;

    return;
}