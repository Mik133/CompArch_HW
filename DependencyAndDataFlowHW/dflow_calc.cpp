/* 046267 Computer Architecture - HW #3 */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"
//Include libs
#include <iostream>
#include <map>
#include <utility>
#include <algorithm>
#include <queue>
//Enums
enum {ENTRY_NODE = -1};
enum {NOT_FOUND_ERROR = -1};
//Func declaration

//Using
using std::cout;
using std::endl;
using std::map;
using std::make_pair;
using std::max_element;
using std::queue;
//Node class
class dependecyNode {
private:
    int cmdNum;
    int cmdLatency;
    int opCode;
    int dstIdx;
    int src1Idx;
    int src2Idx;
    int myLongestPath;
    dependecyNode* src1Node = nullptr;
    dependecyNode* src2Node = nullptr;
public:
    dependecyNode(int cmdNum,int cmdLatency,int opCode,int dstIdx,int src1Idx,int src2Idx)://Constructor
        cmdNum(cmdNum),cmdLatency(cmdLatency),opCode(opCode),dstIdx(dstIdx),src1Idx(src1Idx),src2Idx(src2Idx) {
        this->myLongestPath = 0;
    }

    void addSrc1Noode(dependecyNode* src1NewNode) {//Adds a node as src1 dependency
        src1Node = src1NewNode;
    }

    void addSrc2Noode(dependecyNode* src2NewNode) {//Adds a node as src2 dependency
        src2Node = src2NewNode;
    }

    bool dependencyCheck() {//Checks if node has 0 dependencies
        if(src1Node == nullptr && src2Node == nullptr) return true;
        return false;
    }

    int getPathWeight() const {//Get latency
        return this->cmdLatency;
    }

    int getLongestPathVal() const {//Get whole path value
        return this->myLongestPath;
    }
    void calcMyLongestPath() {//Calculate node's longest path as dependant on the sons
        int path1 = 0;//src 1 latency
        int path2 = 0;//src 2 latency
        int longest1 = 0;//src 1 longest path
        int longest2 = 0;//src 2 longest path
        if(src1Node != nullptr && src1Node->getPathWeight() != ENTRY_NODE) {//Check if src node is viable
            path1 = src1Node->getPathWeight();
            longest1 = src1Node->getLongestPathVal();
        }
        if(src2Node != nullptr && src2Node->getPathWeight() != ENTRY_NODE) {
            path2 = src2Node->getPathWeight();
            longest2 = src2Node->getLongestPathVal();
        }//Now the calculation is made according to the longest path
        if(path1 + longest1 > path2 + longest2) myLongestPath += (path1 + longest1);
        else if(path1 + longest1 < path2 + longest2) myLongestPath += (path2 + longest2);
        else if(path1 + longest1 == path2 + longest2) myLongestPath += (path1 + longest1);
        else myLongestPath = 0;
    }

    int getNodeId() const {//Get cmdNum
        return this->cmdNum;
    }

    dependecyNode* getSrcNode(int nodeNum) {//Get src node according to son 1/2
        if(nodeNum == 1) return this->src1Node;
        if(nodeNum == 2) return this->src2Node;
        return nullptr;
    }

    int getSrcId(int nodeNum) {//Gey son 1/2 node cmdNum
        if(nodeNum == 1) {
            if(src1Node == nullptr) return NOT_FOUND_ERROR;
            return this->src1Node->getNodeId();
        }
        if(nodeNum == 2) {
            if(src2Node == nullptr) return NOT_FOUND_ERROR;
            return this->src2Node->getNodeId();
        }
        return NOT_FOUND_ERROR;
    }
};
//Tree object
class dependencyTree {
private:
    map<int,dependecyNode*> exitNodes;//All nodes connected to exit are reside here

public:
    dependencyTree(map<int,dependecyNode*> exitNodes) {//Ctor :copying the map into the object
        this->exitNodes = std::move(exitNodes);
    }

    int getMaxDepth() {//This method returns the maximum depth of the program from exit to entry
        int* allDepths = new int[exitNodes.size()];
        int indexExit = 0;
        for(auto itrExit = exitNodes.begin();itrExit != exitNodes.end();itrExit++) {
            allDepths[indexExit] = itrExit->second->getLongestPathVal() + itrExit->second->getPathWeight();
            indexExit++;
        }
        int* maxDepth = max_element(allDepths,allDepths + exitNodes.size());
        return *maxDepth;
    }

    dependecyNode* findNodeById(int idNode) {//This method returns the Node by looking for matchin cmdNum
        queue<dependecyNode*> nodesQueue;//Using que to find instead of diving into the recurrsion
        dependecyNode* tmpNode;
        for(auto itrExit = exitNodes.begin();itrExit != exitNodes.end();itrExit++) {
            nodesQueue.push(itrExit->second);
        }
        while(!nodesQueue.empty()) {
            if(nodesQueue.front() == nullptr) {//If the son node is NULL we throw it out and go on to the next iteration
                nodesQueue.pop();
                continue;
            }
            tmpNode = nodesQueue.front();
            nodesQueue.pop();
            if(tmpNode->getNodeId() == idNode) return tmpNode;
            nodesQueue.push(tmpNode->getSrcNode(1));
            nodesQueue.push(tmpNode->getSrcNode(2));
        }
        return nullptr;
    }
};

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    int read1;
    int read2;
    bool depen1 = false;
    bool depen2 = false;
    map<int,dependecyNode*> allNodesTmp;
    map<int,dependecyNode*> exitNodes;
    for(int i = numOfInsts - 1;i > -1; i--) {//For each CMD we find the most recent dependencies
        depen1 = false;
        depen2 = false;
        //cout << "CMD#:" << i << ",opCode:" << progTrace[i].opcode << ",dest:" << progTrace[i].dstIdx << ",src1:" <<//DEBUG
            //progTrace[i].src1Idx << ",src2:" << progTrace[i].src2Idx << ",lateny:" << opsLatency[progTrace[i].opcode] << endl;//DEBUG
        if(allNodesTmp.find(i) == allNodesTmp.end()) {//If we havent added the CMD we add it
            dependecyNode* newNode = new dependecyNode(i,opsLatency[progTrace[i].opcode],progTrace[i].opcode,
                                  progTrace[i].dstIdx,progTrace[i].src1Idx,progTrace[i].src2Idx);
            allNodesTmp.insert(make_pair(i,newNode));
            exitNodes.insert(make_pair(i,newNode));//Its an exit node
        }
        read1 = progTrace[i].src1Idx;//src 1 reg
        read2 = progTrace[i].src2Idx;//src 2 reg
        for(int j = i - 1;j > -1;j--) {
            if(progTrace[j].dstIdx == read1 && depen1 == false) {//Check if were dependat on this cmd for src reg 1
                //cout << "CMD#:" << i << ",Depents on CMD#:" << j << ",Register:" << read1 << endl;//DEBUG
                if(allNodesTmp.find(j) == allNodesTmp.end()) {
                    dependecyNode* newNodeSrc1 = new dependecyNode(j,opsLatency[progTrace[j].opcode],progTrace[j].opcode,
                                      progTrace[j].dstIdx,progTrace[j].src1Idx,progTrace[j].src2Idx);
                    allNodesTmp.insert(make_pair(j,newNodeSrc1));
                }
                allNodesTmp[i]->addSrc1Noode(allNodesTmp[j]);
                depen1 = true;
            }
            if(progTrace[j].dstIdx == read2 && depen2 == false) {//Check if were dependat on this cmd for src reg 2
                //cout << "CMD#:" << i << ",Depents on CMD#:" << j << ",Register:" << read2 << endl;//DEBUG
                if(allNodesTmp.find(j) == allNodesTmp.end()) {
                    dependecyNode* newNodeSrc2 = new dependecyNode(j,opsLatency[progTrace[j].opcode],progTrace[j].opcode,
                                      progTrace[j].dstIdx,progTrace[j].src1Idx,progTrace[j].src2Idx);
                    allNodesTmp.insert(make_pair(j,newNodeSrc2));
                }
                allNodesTmp[i]->addSrc2Noode(allNodesTmp[j]);
                depen2 = true;
            }
            if(depen1 == true && depen2 == true) break;//If we found 2 dependencies were done with this CMD
        }
    }
    //Make entry node and assign
    dependecyNode* entryNode = new dependecyNode(ENTRY_NODE,ENTRY_NODE,ENTRY_NODE,ENTRY_NODE,ENTRY_NODE,ENTRY_NODE);
    for(auto itrCmd = allNodesTmp.begin();itrCmd != allNodesTmp.end();itrCmd++) {//If this cmd has no dependencies its sons are Entry
        if(itrCmd->second->dependencyCheck()) {
            itrCmd->second->addSrc1Noode(entryNode);
            itrCmd->second->addSrc2Noode(entryNode);
        }
    }
    for(auto itrPath = allNodesTmp.begin();itrPath != allNodesTmp.end();itrPath++)//We made the tree now we calculate the path weight
        itrPath->second->calcMyLongestPath();
    dependencyTree* dependencyTreeInst = new dependencyTree(exitNodes);
    return dependencyTreeInst;
}

void freeProgCtx(ProgCtx ctx) {//Simple as just free the tree pointer
    delete (dependencyTree*)ctx;
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    dependencyTree* myDepTree = (dependencyTree*)ctx;
    dependecyNode* nodeInst = myDepTree->findNodeById(theInst);//Find the wanted CMD
    if(nodeInst != nullptr)
        return nodeInst->getLongestPathVal();//Extract the depth
    return NOT_FOUND_ERROR;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    dependencyTree* myDepTree = (dependencyTree*)ctx;
    dependecyNode* nodeInst = myDepTree->findNodeById(theInst);//Find the wanted CMD
    if(nodeInst != nullptr) {
        *src1DepInst = nodeInst->getSrcId(1);//Extract src 1 cmdNum
        *src2DepInst = nodeInst->getSrcId(2);//Extract src 2 cmdNum
        return 0;
    }
    return NOT_FOUND_ERROR;
}

int getProgDepth(ProgCtx ctx) {//Invokes our trees max depth method
    dependencyTree* myDepTree = (dependencyTree*)ctx;
    int maxProgDepth = myDepTree->getMaxDepth();
    return maxProgDepth;
}


