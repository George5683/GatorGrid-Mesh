#include <vector>

//picoNodeVector class created by Maxwell Evans and Mateo Slivka
//Last edited on 3/27/2025

//Class for handling the pico's vector of other nodes on the network.
//Each pico must be assigned an ID. This is defaulted to 0 if none is given.
//Vector item 0 is always the pico's own nodeID.
//There are a number of negative return values to represent different errors.

//handleOperation() should be the only called function that can return errors. Below are the potential codes.
//-1 = Invalid operation requested
//-2 = Attempted to add an ID already present in the array
//-3 = Attempted to remove the current node's ID from the array
//-4 = Attempted to remove an ID not present in the array

class picoNodeVector{

    public:

        int vectorSize;
        int nodeID;

        std::vector<int> nodeIDVector;

        //Constructor assigns index 0 to be the current node's ID
        picoNodeVector(int nodeID = 0);

        //Function for adding picoID to the nodeIDVector
        int addItem(int picoID);
        //Returns -2 if the input ID is already in the array

        //Function for removing picoID from the nodeIDVector
        int removeItem(int picoID);
        //Returns -3 if the input ID is the current node's ID
        //Returns -4 if the input ID is not in the vector

        //Wrapper function for calling add/remove. operationType = 0 for add, operationType = 1 for remove
        int handleOperation(int operationType, int picoID);
        //Returns -1 on an unsupported operation type
        //Calls removeItem and addItem respectively, returning their error codes if an error occurs

        //Iterates through the input list, returning false if it is not equal in contents to the nodeIDVector
        bool compareWithCurrentList(std::vector<int> inputVector);

        //Clears the current nodeIDVector and sets index 0 to the current node's ID
        void clearList();

};