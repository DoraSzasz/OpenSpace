/*****************************************************************************************
*                                                                                       *
* OpenSpace                                                                             *
*                                                                                       *
* Copyright (c) 2014-2015                                                               *
*                                                                                       *
* Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
* software and associated documentation files (the "Software"), to deal in the Software *
* without restriction, including without limitation the rights to use, copy, modify,    *
* merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
* permit persons to whom the Software is furnished to do so, subject to the following   *
* conditions:                                                                           *
*                                                                                       *
* The above copyright notice and this permission notice shall be included in all copies *
* or substantial portions of the Software.                                              *
*                                                                                       *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
* INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
* PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
* HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
* CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
* OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
****************************************************************************************/
#include <modules/iswa/rendering/iswagroup.h>
#include <modules/iswa/rendering/dataplane.h>

namespace {
    const std::string _loggerCat = "ISWAGroup";
}

namespace openspace {

ISWAGroup::ISWAGroup(int id)
    :_enabled("enabled", "Enabled", true)
    ,_useLog("useLog","Use Logarithm", false)
    ,_useHistogram("_useHistogram", "Use Histogram", true)
    ,_normValues("normValues", "Normalize Values", glm::vec2(1.0,1.0), glm::vec2(0), glm::vec2(5.0))
    ,_backgroundValues("backgroundValues", "Background Values", glm::vec2(0.0), glm::vec2(0), glm::vec2(1.0))
    ,_transferFunctionsFile("transferfunctions", "Transfer Functions", "${SCENE}/iswa/tfs/hot.tf")
    ,_delete("delete", "Delete")
    ,_dataOptions("dataOptions", "Data Options")
    ,_id(id)
    ,_type(ISWAManager::CygnetType::NoType)
{
    setName("ISWAGroup" + std::to_string(_id));

    addProperty(_enabled);

    addProperty(_useLog);
    addProperty(_useHistogram);
    addProperty(_normValues);
    addProperty(_backgroundValues);
    addProperty(_transferFunctionsFile);
    addProperty(_dataOptions);

    addProperty(_delete);
}

ISWAGroup::~ISWAGroup(){
    _cygnets.clear();
}

void ISWAGroup::registerCygnet(ISWACygnet* cygnet, ISWAManager::CygnetType type){
    if(_cygnets.empty()){
        _type = type;
        registerProperties();
    }

    if(type != _type){
        LERROR("Can't register cygnet with a different class from the group");
        return;
    }


    if(type == ISWAManager::CygnetType::Data){
        DataPlane* dataplane = static_cast<DataPlane*>(cygnet);
        
        dataplane->useLog(_useLog.value());
        dataplane->useHistogram(_useHistogram.value());
        dataplane->normValues(_normValues.value());
        dataplane->backgroundValues(_backgroundValues.value());
        dataplane->transferFunctionsFile(_transferFunctionsFile.value());
        dataplane->dataOptions(_dataOptions.value());
    }
    _cygnets.push_back(cygnet);
}

void ISWAGroup::unregisterCygnet(ISWACygnet* cygnet){
    auto it = std::find(
        _cygnets.begin(),
        _cygnets.end(),
        cygnet
        );

    if(it != _cygnets.end())
        _cygnets.erase(it);
}


void ISWAGroup::registerOptions(const std::vector<properties::SelectionProperty::Option>& options){
    if(_type == ISWAManager::CygnetType::Data){
        if(_dataOptions.options().empty()){
            for(auto option : options){
                _dataOptions.addOption(option);
            }
            _dataOptions.setValue(std::vector<int>(1,0));
        }
        for(auto cygnet : _cygnets)
            static_cast<DataPlane*>(cygnet)->dataOptions(_dataOptions.value());
    }
}

void ISWAGroup::registerProperties(){
    OsEng.gui()._iSWAproperty.registerProperty(&_enabled);

    _enabled.onChange([this]{
        for(auto cygnet : _cygnets)
            cygnet->enabled(_enabled.value());
    });


    if(_type == ISWAManager::CygnetType::Data){
        OsEng.gui()._iSWAproperty.registerProperty(&_useLog);
        OsEng.gui()._iSWAproperty.registerProperty(&_useHistogram);
        OsEng.gui()._iSWAproperty.registerProperty(&_normValues);
        OsEng.gui()._iSWAproperty.registerProperty(&_backgroundValues);
        OsEng.gui()._iSWAproperty.registerProperty(&_transferFunctionsFile);
        OsEng.gui()._iSWAproperty.registerProperty(&_dataOptions);

        _useLog.onChange([this]{
            for(auto cygnet : _cygnets)
                static_cast<DataPlane*>(cygnet)->useLog(_useLog.value());
        });

        _useHistogram.onChange([this]{
            for(auto cygnet : _cygnets)
                static_cast<DataPlane*>(cygnet)->useHistogram(_useHistogram.value());
        });

        _normValues.onChange([this]{
            for(auto cygnet : _cygnets)
                static_cast<DataPlane*>(cygnet)->normValues(_normValues.value());
        });

        _backgroundValues.onChange([this]{
            for(auto cygnet : _cygnets)
                static_cast<DataPlane*>(cygnet)->backgroundValues(_backgroundValues.value());
        });

        _transferFunctionsFile.onChange([this]{
            for(auto cygnet : _cygnets)
                static_cast<DataPlane*>(cygnet)->transferFunctionsFile(_transferFunctionsFile.value());
        });


        _dataOptions.onChange([this]{
            for(auto cygnet : _cygnets)
                static_cast<DataPlane*>(cygnet)->dataOptions(_dataOptions.value());
        });

    }

    OsEng.gui()._iSWAproperty.registerProperty(&_delete);
    _delete.onChange([this]{
        for(auto it = _cygnets.begin(); it != _cygnets.end();){
            ISWAManager::ref().deleteISWACygnet((*it)->name());
            it = _cygnets.erase(it);
        }
        _dataOptions.removeOptions();
        OsEng.gui()._iSWAproperty.unregisterProperties(name());
        _type = ISWAManager::CygnetType::NoType;
        // ISWAManager::ref().unregisterGroup(_id);
    }); 
}

} //namespace openspace