#pragma once

#include "ik.h"
#include <vector>
#include "ikGroup.h"

class CIkGroupContainer
{
public:
    CIkGroupContainer();
    virtual ~CIkGroupContainer();

    CikGroup* getIkGroup(int groupID) const;
    CikGroup* getIkGroup(std::string groupName) const;
    void addIkGroup(CikGroup* anIkGroup);
    void removeIkGroup(int ikGroupHandle);
    void removeAllIkGroups();
    void announceSceneObjectWillBeErased(int objectHandle);
    void announceIkGroupWillBeErased(int ikGroupHandle);
    int computeAllIkGroups(bool exceptExplicitHandling);
    void resetCalculationResults();

    std::vector<CikGroup*> ikGroups;
};
