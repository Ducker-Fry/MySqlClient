#include<input/inputdata.h>
#include<input/input_console.h>
#include<input/input_file.h>

//输入源管理器
class InputManager
{
private:
    std::vector<std::shared_ptr<IInputSource>> inputSources;
public:
    void addInputSource(const std::shared_ptr<IInputSource>& source)
    {
        inputSources.push_back(source);
    }
    void clearInputSources()
    {
        inputSources.clear();
    }
    std::vector<std::shared_ptr<IInputSource>> getInputSources() const
    {
        return inputSources;
    }

    InputData getInputFromSource(std::shared_ptr<IInputSource> source)
    {
        if (!source)
        {
            throw std::invalid_argument("Input source is null");
        }
        return source->readInput();
    }

    InputData getInputFromSource(std::string& source_type)
    {
        for(auto & source : inputSources)
        {
            if(source->getSourceType() == source_type)
            {
                return source->readInput();
            }
            else 
            {
                throw std::invalid_argument("No matching input source found for type: " + source_type);
            }
        }
    }

};