#include "objectContainer.h"
#include "app.h"
#include "simConst.h"

CObjectContainer::CObjectContainer()
{
    _nextObjectHandle=0;
    newSceneProcedure();
}

CObjectContainer::~CObjectContainer()
{
    removeAllObjects();
}

void CObjectContainer::newSceneProcedure()
{
    removeAllObjects();
}

void CObjectContainer::removeAllObjects()
{

    while (objectList.size()!=0)
    {
        CSceneObject* it=getObject(objectList[0]);
        if (it!=nullptr)
            eraseObject(it);
    }
    // The above loop destroys all 3DObjects, and normally automatically
    // all resources linked to them. So we don't have to destroy the resources

    objectList.clear();

    _objectIndex.clear();


    jointList.clear();
    dummyList.clear();
    orphanList.clear();
}

bool CObjectContainer::makeObjectChildOf(CSceneObject* childObject,CSceneObject* parentObject)
{   // This will trigger an actualization (important! orphanList needs also an update and other things too)
    // Careful: this routine should be able to be used for objects in the object
    // container, but also for objects in the copy buffer!!!! So don't make
    // use of any 'getObject(id)' or similar function!!!!! <-- this is a very old comment. Is it still true??
    if (childObject==nullptr)
        return(false);
    // Check if the child has already his desired parent (so that we don't have to call the actualization (heavy and will also refresh all dialogs) (added on 2009/12/15)
    if (childObject->getParentObject()==parentObject)
        return(true);
    if (parentObject==nullptr)
    {
        C7Vector oldAbsoluteTransf(childObject->getCumulativeTransformationPart1());
        childObject->setParentObject(nullptr);
        childObject->setLocalTransformation(oldAbsoluteTransf);
        actualizeObjectInformation();
        return(true);
    }
    // Illegal loop checking:
    if (parentObject->isObjectAffiliatedWith(childObject))
        return(false);
    C7Vector oldAbsoluteTransf(childObject->getCumulativeTransformationPart1());
    C7Vector parentInverse(parentObject->getCumulativeTransformation().getInverse());
    childObject->setLocalTransformation(parentInverse*oldAbsoluteTransf);
    childObject->setParentObject(parentObject);

    

    actualizeObjectInformation();
    return(true);
}

void CObjectContainer::setAbsoluteConfiguration(int objectHandle,const C7Vector& v,bool keepChildrenInPlace)
{
    CSceneObject* it=getObject(objectHandle);
    if (it!=nullptr)
    {
        C7Vector childPreTr(it->getLocalTransformation());
        it->setLocalTransformation(it->getParentCumulativeTransformation().getInverse()*v);
        if (keepChildrenInPlace)
        {
            childPreTr=it->getLocalTransformation().getInverse()*childPreTr;
            for (size_t i=0;i<it->childList.size();i++)
                it->childList[i]->setLocalTransformation(childPreTr*it->childList[i]->getLocalTransformationPart1());
        }
    }
}

void CObjectContainer::actualizeObjectInformation()
{
    // Actualize each object's child list
    // Following rewritten on 2009/03/15 to make it faster:
    for (size_t i=0;i<objectList.size();i++)
    {
        CSceneObject* it=_objectIndex[size_t(objectList[i])];
        it->childList.clear();
    }
    for (size_t i=0;i<objectList.size();i++)
    {
        CSceneObject* it=_objectIndex[size_t(objectList[i])];
        CSceneObject* parent=it->getParentObject();
        if (parent!=nullptr)
            parent->childList.push_back(it);
    }

    // Actualize the light, graph, sensor & joint list
    jointList.clear();
    dummyList.clear();
    orphanList.clear();

    for (size_t i=0;i<objectList.size();i++)
    {
        if (_objectIndex[size_t(objectList[i])]->getObjectType()==sim_object_joint_type)
            jointList.push_back(objectList[i]);
        if (_objectIndex[size_t(objectList[i])]->getObjectType()==sim_object_dummy_type)
            dummyList.push_back(objectList[i]);
        if (_objectIndex[size_t(objectList[i])]->getParentObject()==nullptr)
            orphanList.push_back(objectList[i]);
    }
    // We actualize the direct linked joint list of each joint: (2009-01-27)
    for (size_t i=0;i<jointList.size();i++)
    {
        CJoint* it=getJoint(jointList[i]);
        it->dependentJoints.clear();
        for (size_t j=0;j<jointList.size();j++)
        {
            CJoint* anAct=getJoint(App::currentInstance->objectContainer->jointList[j]);
            if (anAct!=it)
            {
                if (anAct->getJointMode()==sim_jointmode_dependent)
                {
                    if (anAct->getDependencyJointHandle()==it->getObjectHandle())
                        it->dependentJoints.push_back(anAct);
                }
            }
        }
    }
}

int CObjectContainer::getObjectHandle(const std::string& objectName) const
{
    for (size_t i=0;i<objectList.size();i++)
    {
        if (_objectIndex[size_t(objectList[i])]->getObjectName().compare(objectName)==0)
            return(objectList[i]);
    }
    return(-1);
}

int CObjectContainer::getHighestObjectHandle() const
{
    int highest=-1;
    for (size_t i=0;i<objectList.size();i++)
    {
        if (objectList[i]>highest)
            highest=objectList[i];
    }
    return(highest);
}

CSceneObject* CObjectContainer::getObject(int objectHandle) const
{
    if ( (objectHandle>=0)&&(objectHandle<int(_objectIndex.size())) )
        return(_objectIndex[size_t(objectHandle)]);
    return(nullptr);
}

CDummy* CObjectContainer::getDummy(int objectHandle) const
{
    CSceneObject* it=getObject(objectHandle);
    if (it!=nullptr)
    {
        if (it->getObjectType()==sim_object_dummy_type)
            return(static_cast<CDummy*>(it));
    }
    return(nullptr);
}

CJoint* CObjectContainer::getJoint(int objectHandle) const
{
    CSceneObject* it=getObject(objectHandle);
    if (it!=nullptr)
    {
        if (it->getObjectType()==sim_object_joint_type)
            return(static_cast<CJoint*>(it));
    }
    return(nullptr);
}

CSceneObject* CObjectContainer::getObject(const std::string& name) const
{
    return(getObject(getObjectHandle(name)));
}

int CObjectContainer::createDummy(const char* objectName)
{
    CDummy* it=new CDummy();
    if (objectName!=nullptr)
        it->setObjectName(objectName);
    addObjectToScene(it);
    return(it->getObjectHandle());
}

int CObjectContainer::createJoint(const char* objectName,int jointType)
{
    CJoint* it=new CJoint(jointType);
    if (objectName!=nullptr)
        it->setObjectName(objectName);
    addObjectToScene(it);
    return(it->getObjectHandle());
}

bool CObjectContainer::eraseObject(CSceneObject* it)
{
    if (it==nullptr)
        return(false);

    // We announce the object will be erased:
    announceObjectWillBeErased(it->getObjectHandle()); // this may trigger other "eraseObject" calls (not really, since we don't have versatiles anymore!)
    // We remove the object from the object list
    size_t i;
    for (i=0;i<objectList.size();i++)
    {
        if (objectList[i]==it->getObjectHandle())
            break;
    }
    objectList.erase(objectList.begin()+i);
    // Now remove the object from the index
    _objectIndex[size_t(it->getObjectHandle())]=nullptr;
    delete it;
    actualizeObjectInformation();

    return(true);
}

//------------------ Object destruction announcement -------------------------
void CObjectContainer::announceObjectWillBeErased(int objectHandle)
{
    for (int i=0;i<int(objectList.size());i++)
    {
        CSceneObject* it=getObject(objectList[size_t(i)]);
        if (it->getObjectHandle()!=objectHandle)
        {
            if (it->announceSceneObjectWillBeErased(objectHandle))
            { // We should never enter here since one obj destruction cannot trigger another obj destruction (anymore, no more versatiles!) 
                eraseObject(it);
                i=-1; // ordering might have changed!
            }
        }
    }
    App::currentInstance->ikGroupContainer->announceSceneObjectWillBeErased(objectHandle);
}

void CObjectContainer::announceIkGroupWillBeErased(int ikGroupHandle)
{
    for (size_t i=0;i<objectList.size();i++)
        getObject(objectList[i])->announceIkGroupWillBeErased(ikGroupHandle); // this never triggers 3DObject destruction!
    App::currentInstance->ikGroupContainer->announceIkGroupWillBeErased(ikGroupHandle); // This will never trigger an Ik group destruction
}

void CObjectContainer::importKinematicsData(CSerialization& ar)
{
    removeAllObjects();
    App::currentInstance->ikGroupContainer->removeAllIkGroups(); // just in case

    int versionNumber=ar.readInt(); // this is the ext IK serialization version. Not forward nor backward compatible!

    int objCnt=ar.readInt();

    std::vector<int> objectMapping;
    for (int i=0;i<objCnt;i++)
    {
        int objType=ar.readInt();

        CSceneObject* it;

        if (objType==sim_object_joint_type)
        {
            CJoint* joint=new CJoint(sim_joint_revolute_subtype);
            joint->serialize(ar);
            it=joint;
        }
        else
        {
            CDummy* dum=new CDummy();
            dum->serialize(ar);
            it=dum;
        }
        objectMapping.push_back(it->getObjectHandle());
        addObjectToScene(it);
        objectMapping.push_back(it->getObjectHandle());
    }

    // We do it the slow way (in order not to use too much RAM)
    // prepareFastLoadingMapping(objectMapping);

    for (size_t i=0;i<objectList.size();i++)
    {
        CSceneObject* it=getObject(objectList[i]);
        it->performSceneObjectLoadingMapping(&objectMapping);
    }

    int ikGroupCnt=ar.readInt();

    for (int i=0;i<ikGroupCnt;i++)
    {
        CikGroup* it=new CikGroup();
        it->serialize(ar);
        App::currentInstance->ikGroupContainer->addIkGroup(it);
    }

    for (size_t i=0;i<App::currentInstance->ikGroupContainer->ikGroups.size();i++)
    {
        CikGroup* it=App::currentInstance->ikGroupContainer->ikGroups[i];
        it->performObjectLoadingMapping(&objectMapping);
    }
}

void CObjectContainer::addObjectToScene(CSceneObject* newObject)
{
    std::string newObjName=newObject->getObjectName();
    if (newObjName.size()==0)
    {
        newObjName="0";
        newObject->setObjectName(newObjName);
    }
    while (getObject(newObjName)!=nullptr)
    {
        char aChar=newObjName[newObjName.length()-1];
        std::string oldNumber("");
        while ( (aChar>='0')&&(aChar<='9') )
        {
            oldNumber.insert(oldNumber.begin(),aChar);
            newObjName.erase(newObjName.end()-1);
            if (newObjName.length()!=0)
                aChar=newObjName[newObjName.length()-1];
            else
                aChar='a';
        }
        if (oldNumber.length()==0)
            newObjName=newObjName+"0";
        else
            newObjName=newObjName+std::to_string(atoi(oldNumber.c_str())+1);
    }

    // Give the object a new identifier
    int id=-1;
    for (size_t i=0;i<_objectIndex.size();i++)
    {
        if (_objectIndex[i]==nullptr)
        {
            _objectIndex[i]=newObject;
            id=int(i);
            break;
        }
    }
    if (id==-1)
    {
        id=int(_objectIndex.size());
        _objectIndex.push_back(newObject);
    }

    // set the new handle to the object:
    newObject->setObjectHandle(id);
    objectList.push_back(id);

    // Actualize the object information
    actualizeObjectInformation();
}
