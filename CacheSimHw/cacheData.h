#ifndef CACHE_DATA_H
#define CACHE_DATA_H
#include <iostream>
#include <ostream>

#ifdef __cplusplus
extern "C" {
#endif


int init_cache(unsigned MemCyc,unsigned BSize,unsigned L1Size,unsigned L2Size,
                unsigned L1Cyc,unsigned L2Cyc,unsigned L1Assoc,unsigned L2Assoc,
                unsigned WrAllo);

void access_cache(char operation,int address);


void stats_update(double* L1MissRate,double* L2MissRate,double* avgAccTime);

void printHitMiss(int cacheNum,bool hitOrMiss);



//Functions
void handleL1eviction(int tagL1,int setL1,int tagL2,int setL2,int address,int dirtyL1);
void handleL2eviction(int tagL1,int setL1,int tagL2,int setL2,int address,int operatio);
//Enum's
enum {DATA_SIZE_BYTES = 4};//Our default size of data in storage
enum {INIT_TAG = -111,NO_DATA_PRESENT = -201};//Initial tag for sets
enum {NO_WRITE_ALLOCATE = 0,WRITE_ALLOCATE = 1};//Write allocate states
enum {INIT_NUM_OF_SETS = -909};
enum {LRU_ERROR_CODE = -1000};
enum {DIRTY = 1,NOT_DIRTY = 0};
enum {HIT = true,MISS = false};
enum {LRU_UPDATE = true,LRU_NOT_UPDATE = false};
enum {VALID = 200,INVALID = 201};
enum {WAY_NOT_FOUND = -1000};
enum {NEED_EVICT = true,NO_EVICT = false};
//NameSpaces
using std::cout;
using std::endl;
//LRU OBJECT
class LRU_data {//LRU each line gets one
private:
    int* waysRanking = {};
    int numOfWays = -1;
public:
    void LRU_init(int numOfWays) {//Initalizes LRU all ranks are 0
        this->numOfWays = numOfWays;
        waysRanking = new int[numOfWays];
        for(int i = 0;i < numOfWays;i++) waysRanking[i] = 0;
    }

    void LRU_update(int accessWay) {//Updates LRU according to accessed WAY
        int oldRank = waysRanking[accessWay];
        waysRanking[accessWay] = this->numOfWays - 1;
        for(int i = 0;i < numOfWays;i++) {
            if(i != accessWay && waysRanking[i] > oldRank)
                waysRanking[i] -= 1;
        }
    }

    void setZeroRank(int way) {//Reseting some entry
       waysRanking[way] = 0;
    }

    int getZeroWay() {//Returns the least recently used way
        for(int i = 0;i < numOfWays;i++) {
            if(waysRanking[i] == 0)
                return i;
        }
        return LRU_ERROR_CODE;//Not supposed to happen under any circumstances
    }
};
//Cache OBJECTS
//1.Cache set object
class CACHE_SET {//A cache set ,contains the tag,address,dirty+valid bits not much more
public:
    int tag;
    int address;
    int dirty;
    int valid;
    CACHE_SET():tag(INIT_TAG),dirty(NOT_DIRTY),valid(INVALID) {
    }
};
//2.Cache way object
class CACHE_WAY {//This is a cache way in case of Direce mapped there will be only one
public:
    CACHE_SET* cacheSets{};
    int numOfSets = INIT_NUM_OF_SETS;
    void init_cache_way(int numOfSets,int blockOffset) {
        this->numOfSets = numOfSets;
        cacheSets = new CACHE_SET[numOfSets];
    }
};
//2.Cache level object
class CACHE_LEVEL {//This is a cache level can be L1/L2
public:
    CACHE_WAY* cacheWays;
    LRU_data* lruRanker;
    int wrAlloc;
    int numOfWays;
    int blockOffset;
    int numOfSets;
    //Cache level init saves the data of the cache:How many ways,write allocate/not allocate,sets in a way
    //and intializes the LRU and all the entry's
    CACHE_LEVEL(int numOfWays,int wrAlloc,int blockOffset,int numOfSets):wrAlloc(wrAlloc),numOfWays(numOfWays) {
        this->blockOffset = blockOffset;
        this->numOfSets = numOfSets;
        cacheWays = new CACHE_WAY[numOfWays];
        for(int i = 0;i < numOfWays;i++) cacheWays[i].init_cache_way(numOfSets,blockOffset);
        lruRanker = new LRU_data[numOfSets];
        for(int i = 0;i < numOfSets;i++) lruRanker[i].LRU_init(numOfWays);
    }

    bool checkIfDataPresent(int tag,int set) {//Checks if data present in case is present returns True if not False
        if(numOfWays == 1) {
            if(cacheWays[0].cacheSets[set].tag == tag
                && cacheWays[0].cacheSets[set].valid == VALID) return true;
            return false;
        }
        else {
            for(int way = 0;way < numOfWays;way++) {
                if(cacheWays[way].cacheSets[set].tag == tag
                    && cacheWays[way].cacheSets[set].valid == VALID) return true;
            }
            return false;
        }
    }

    int locateTagWay(int tag,int set) {//Gets tag and set returns the way it is in
        if(numOfWays == 1) return 0;
        else {
            for(int way = 0;way < numOfWays;way++) {
                if(cacheWays[way].cacheSets[set].tag == tag )
                    return way;
            }
            return WAY_NOT_FOUND;
        }
    }

    void updateLRU(int tag,int set) {//Updates LRU according to tag and set
        if(numOfWays != 1) {
            int wayToUpdate = locateTagWay(tag,set);
            if(wayToUpdate != WAY_NOT_FOUND)
                lruRanker[set].LRU_update(wayToUpdate);
        }
    }

    bool isEvictionNeeded(int set) {//Checks if some line full or not,if direct map then direct comparison
        if(numOfWays == 1) {
            if(cacheWays[0].cacheSets[set].valid == VALID) return NEED_EVICT;
            return NO_EVICT;
        }
        else {
            for(int way = 0;way < numOfWays;way++) {
                if(cacheWays[way].cacheSets[set].valid == INVALID) return NO_EVICT;
            }
            return NEED_EVICT;
        }
    }

    int getWayToWrite(int set) {//Gets set returns Invalid's entry Way
        if(numOfWays == 1) return 0;
        else {
            for(int way = 0;way < numOfWays;way++) {
                if(cacheWays[way].cacheSets[set].valid == INVALID)
                    return way;
            }
        }
    }

    int getVictimWay(int set) {//Gets victim's Way
        if(numOfWays == 1) return 0;
        return lruRanker[set].getZeroWay();
    }

    //Writes to cahce,finds the entry/way ,validates and setting the dirty bit accordingly
    void writeToCacheLRU(int tag,int set,int dirty,int address) {
        if(numOfWays == 1) {
         //   cout << "1 way cahce\n";//DEBUG
            cacheWays[0].cacheSets[set].tag = tag;
            cacheWays[0].cacheSets[set].dirty = dirty;
            cacheWays[0].cacheSets[set].valid = VALID;
            cacheWays[0].cacheSets[set].address = address;
        }
        else {
            int wayToWrite = lruRanker[set].getZeroWay();
          //  cout << "Tag: " << tag << " Way: " << wayToWrite << " Set: " << set << endl;//DEBUG
            cacheWays[wayToWrite].cacheSets[set].tag = tag;
            cacheWays[wayToWrite].cacheSets[set].dirty = dirty;
            cacheWays[wayToWrite].cacheSets[set].valid = VALID;
            cacheWays[wayToWrite].cacheSets[set].address = address;
        }
    }

    void writeCacheWay(int tag,int set,int dirty,int way,int address) {//Not sure if used ,mostly debbug
        cacheWays[way].cacheSets[set].tag = tag;
        cacheWays[way].cacheSets[set].dirty = dirty;
        cacheWays[way].cacheSets[set].valid = VALID;
        cacheWays[way].cacheSets[set].address = address;
    }

    void invalidateBlock(int set,int way) {//Invalidates a certain block
        if(numOfWays == 1) cacheWays[0].cacheSets[set].valid = INVALID;
        else {
            cacheWays[way].cacheSets[set].valid = INVALID;
            lruRanker[set].setZeroRank(way);
        }
    }

    void setBitDirty(int tag,int set) {//Set's bit dirty for exapmle after write
        if(numOfWays == 1) cacheWays[0].cacheSets[set].dirty = DIRTY;
        else {
            int way = locateTagWay(tag,set);
            cacheWays[way].cacheSets[set].dirty = DIRTY;
        }
    }

    //GET BLOCK DATA
    int getBlockOffsetInt(int address) {//Not sure if used or needed
        int boMask = log2(this->blockOffset);
        boMask = (1 << boMask) - 1;
        address =  address & boMask;
        return address;
    }

    int getSetInt(int address) {//Gets set as integer
        if(this->numOfSets == 1) return 0;
        address = address >> (int)log2(this->blockOffset);
        int setOffset = (int)log2(this->numOfSets);
        int setMask = (1 << setOffset) - 1;
        address = address & setMask;
        return address;
    }

    int getTagInt(int address) {//Gets Tag as integer
        int totalOffset = (int)log2(this->blockOffset) + (int)log2(this->numOfSets);
        address = address >> totalOffset;
        return address;
    }
};

//Globals
double L1Miss = 0;
double L2Miss = 0;
double TotalAccessTime = 0;
double numOfAccessL1 = 0;
double numOfAccessL2 = 0;
double cacheAccessNum = 0;
double MemAccessTime = 0;
double L1AccessTime = 0;
double L2AccessTime = 0;
//Cache global pointers
CACHE_LEVEL* cache_L1;
CACHE_LEVEL* cache_L2;

//init our cache objects here:
int init_cache(unsigned MemCyc,unsigned BSize,unsigned L1Size,unsigned L2Size,
                unsigned L1Cyc,unsigned L2Cyc,unsigned L1Assoc,unsigned L2Assoc,
                unsigned WrAllo) {
    //Calculations for L1:
    int blockSizeInt = pow(2,BSize);//Calculate block size(Bytes)
    int L1SizeInt = pow(2,L1Size);//Total size of L1
    int L1WaysInt = pow(2,L1Assoc);//L1 num of ways
    int numOfBlocksL1 = L1SizeInt/blockSizeInt;//Number of blocks L1
    int numOfSetsL1 = numOfBlocksL1/L1WaysInt;//Number of sets in L1
    int blockOffset = blockSizeInt;
    //INIT L1
    cache_L1 = new CACHE_LEVEL(L1WaysInt,WrAllo,blockOffset,numOfSetsL1);
    //Calculations for L2;
    int L2Sizeint = pow(2,L2Size);
    int L2WaysInt = pow(2,L2Assoc);
    int numOfBlocksL2 = L2Sizeint/blockSizeInt;
    int numOfSetsL2 = numOfBlocksL2/L2WaysInt;
    cache_L2 = new CACHE_LEVEL(L2WaysInt,WrAllo,blockOffset,numOfSetsL2);
    //Access times variables init
    L1AccessTime = L1Cyc;//Set L1 time for global variable
    L2AccessTime = L2Cyc;//Set L2 time for global variable
    MemAccessTime = MemCyc;//Set mem time for global variable
    return 0;
}

//Here the access happens for each address write or read
void access_cache(char operation,int address) {
    cacheAccessNum+=1;
 //   cout << "Access #:" << cacheAccessNum << " Operation: " << operation << " Address: " << address << endl;//DEBUG
    //here we get the tags sets and write alloc policy
    int wrPolicy = cache_L1->wrAlloc;
    int tagL1 = cache_L1->getTagInt(address);
    int setL1 = cache_L1->getSetInt(address);
    int tagL2 = cache_L2->getTagInt(address);
    int setL2 = cache_L2->getSetInt(address);
  //   cout << "L1-tag: " << tagL1 << " L1-set: " << setL1 << " L2-tag: " << tagL2 << " L2-set: " << setL2 << endl;
  //  if(cacheAccessNum == 16)
   //     cout << "DEBUG\n";
    //In case of read operation the same happens disregarding the wrAlloc policy
    if(operation == 'r'){
        bool L1Check = cache_L1->checkIfDataPresent(tagL1,setL1);
        TotalAccessTime += L1AccessTime;
        numOfAccessL1++;
        if(L1Check == HIT) {//If L1 hit then were done
            cache_L1->updateLRU(tagL1,setL1);
            return;
        }
        L1Miss++;
        bool L2Check = cache_L2->checkIfDataPresent(tagL2,setL2);
        TotalAccessTime += L2AccessTime;
        numOfAccessL2++;
        if(L2Check == HIT) {//If L1 miss and L2 hit we have to check for eviction and update nessesary blocks according to inclusion policy
            cache_L2->updateLRU(tagL2,setL2);
            handleL1eviction(tagL1,setL1,tagL2,setL2,address,NOT_DIRTY);
            return;
        }
        L2Miss++;
        TotalAccessTime += MemAccessTime;
        handleL2eviction(tagL1,setL1,tagL2,setL2,address,operation);//Here we checking if wrting to mem needed (not actually doing )
}                                                                   //And doing evictions if needed
    else {//Case of write operation
        if(wrPolicy == WRITE_ALLOCATE) {//wr policy is Write allocate
            bool L1Check = cache_L1->checkIfDataPresent(tagL1,setL1);
            TotalAccessTime+= L1AccessTime;
            numOfAccessL1++;
            if(L1Check == HIT) {//L1 hit were updating to dirty and done
                cache_L1->setBitDirty(tagL1,setL1);
                cache_L1->updateLRU(tagL1,setL1);
                return;
            }
            L1Miss++;
            bool L2Check = cache_L2->checkIfDataPresent(tagL2,setL2);
            TotalAccessTime+=L2AccessTime;
            numOfAccessL2++;
            if(L2Check == HIT) {//L1 miss L2 Hit same as read but this time writing dirty bit to L1
                cache_L2->updateLRU(tagL2,setL2);
                handleL1eviction(tagL1,setL1,tagL2,setL2,address,DIRTY);
                return;
            }
            L2Miss++;
            TotalAccessTime+=MemAccessTime;
            handleL2eviction(tagL1,setL1,tagL2,setL2,address,operation);//as in read L1,L2 Miss
        }
        if(wrPolicy == NO_WRITE_ALLOCATE) {//No write allocate policy much easier
            bool L1Check = cache_L1->checkIfDataPresent(tagL1,setL1);
            TotalAccessTime+=L1AccessTime;
            numOfAccessL1++;
            if(L1Check == HIT) {
                cache_L1->setBitDirty(tagL1,setL1);
                cache_L1->updateLRU(tagL1,setL1);
                return;
            }
            L1Miss++;
            bool L2Check = cache_L2->checkIfDataPresent(tagL2,setL2);
            TotalAccessTime+=L2AccessTime;
            numOfAccessL2++;
            if(L2Check == HIT) {
                cache_L2->setBitDirty(tagL2,setL2);
                cache_L2->updateLRU(tagL2,setL2);
                return;
            }
            L2Miss++;
            TotalAccessTime+=MemAccessTime;
        }
    }
}

//Here we update the final stats of the run Miss for L1 L2 and access times
void stats_update(double* L1MissRate,double* L2MissRate,double* avgAccTime) {
    //Update statistics according to trace run
    if(TotalAccessTime != 0 && cacheAccessNum != 0){
   // cout << "L1 MISS:" << L1Miss << "/" << numOfAccessL1 <<
     //   ",L2MISS:" << L2Miss << "/" << numOfAccessL2 << endl;
    *L1MissRate = L1Miss/numOfAccessL1;
    *L2MissRate = L2Miss/numOfAccessL2;
    *avgAccTime = TotalAccessTime/cacheAccessNum;
    }
    delete cache_L1;
    delete cache_L2;
}

void printHitMiss(int cacheNum,bool hitOrMiss) {//Debug function not used
    if(hitOrMiss == HIT) cout << "L:" << cacheNum << ":HIT\n";
    else cout << "L:" << cacheNum << ":MISS\n";
}

//This function handles the L1 eviction if needed
void handleL1eviction(int tagL1,int setL1,int tagL2,int setL2,int address,int dirtyL1) {
    bool isL1Evict = cache_L1->isEvictionNeeded(setL1);
    if(isL1Evict == NEED_EVICT) {
        int victimWay = cache_L1->getVictimWay(setL1);
        int addressVic = cache_L1->cacheWays[victimWay].cacheSets[setL1].address;
        int tagVicL2 = cache_L2->getTagInt(addressVic);
        int setVicL2 = cache_L2->getSetInt(addressVic);
        if(cache_L1->cacheWays[victimWay].cacheSets[setL1].dirty == DIRTY) {
            bool L2VicFind = cache_L2->checkIfDataPresent(tagVicL2,setVicL2);
            if(L2VicFind == MISS)
                cache_L2->writeToCacheLRU(tagVicL2,setVicL2,DIRTY,addressVic);
            cache_L2->updateLRU(tagVicL2,setVicL2);
        }
        cache_L1->invalidateBlock(setL1,victimWay);
    }
    cache_L1->writeToCacheLRU(tagL1,setL1,dirtyL1,address);
    cache_L1->updateLRU(tagL1,setL1);
}
//Handles L2 eviciton if needed
void handleL2eviction(int tagL1,int setL1,int tagL2,int setL2,int address,int operation) {
    bool isL2Evict = cache_L2->isEvictionNeeded(setL2);
    if(isL2Evict == NEED_EVICT) {
        int victimL2way = cache_L2->getVictimWay(setL2);
        int addressL2Vic = cache_L2->cacheWays[victimL2way].cacheSets[setL2].address;
        cache_L2->invalidateBlock(setL2,victimL2way);
        int tagVic = cache_L1->getTagInt(addressL2Vic);
        int setVic = cache_L1->getSetInt(addressL2Vic);
        bool L1snoop = cache_L1->checkIfDataPresent(tagVic,setVic);
        if(L1snoop == HIT) {
            int vicL1Way = cache_L1->locateTagWay(tagVic,setVic);
            cache_L1->invalidateBlock(setVic,vicL1Way);
        }
    }
    cache_L2->writeToCacheLRU(tagL2,setL2,NOT_DIRTY,address);
    cache_L2->updateLRU(tagL2,setL2);
    if(operation == 'r')
        handleL1eviction(tagL1,setL1,tagL2,setL2,address,NOT_DIRTY);
    else
        handleL1eviction(tagL1,setL1,tagL2,setL2,address,DIRTY);
}
#ifdef __cplusplus
}
#endif

#endif