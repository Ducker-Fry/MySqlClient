#include "inputdata.h"
#include "inputdata.h"
#include "inputdata.h"
#include "inputdata.h"
#include<input/inputdata.h>

void InputManager::addSource(const std::shared_ptr<IInputSource>& source)
{
    inputSources.push_back(source);
}

std::list<InputData> InputManager::readAllInputs()
{
    std::list<InputData> inputDataList;

    for(auto & source : inputSources)
    {
        InputData data = source->readInput();
        data.setSourceType(source->getSourceType());
        inputDataList.push_back(data);
    }

    return inputDataList;
}

void InputManager::removeSource(const std::shared_ptr<IInputSource>& source)
{
    for (auto it = inputSources.begin(); it != inputSources.end(); ++it)
    {
        if (*it == source)
        {
            inputSources.erase(it);
            break;
        }
    }
}

InputData InputManager::readInputFromSource(const std::shared_ptr<IInputSource>& source)
{
    return source->readInput();
}

