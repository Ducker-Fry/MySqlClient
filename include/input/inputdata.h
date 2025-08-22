#pragma once
#include<string>
#include<list>
#include<memory>

class InputData
{
private:
    std::string rawData;
    std::string sourceType;
public:
    InputData() = default;
    virtual ~InputData() = default;


    virtual std::string getRawData() const { return rawData; }
    virtual void setRawData(const std::string& data) { rawData = data; }

    virtual std::string getSourceType() const { return sourceType; }
    virtual void setSourceType(const std::string& type) { sourceType = type; }
};

// InputSource Interface
class IInputSource
{
    public:
    virtual ~IInputSource() = default;
    // Method to read data from the source
    virtual InputData readInput() = 0;
    // Method to get the type of the input source
    virtual std::string getSourceType() const = 0;
};

// InputManager Class
class InputManager
{
private:
    std::list<std::shared_ptr<IInputSource>> inputSources;
public:
    void addSource(const std::shared_ptr<IInputSource>& source);
    std::list<InputData> readAllInputs();
    void removeSource(const std::shared_ptr<IInputSource>& source);
    InputData readInputFromSource(const std::shared_ptr<IInputSource>& source);
};
