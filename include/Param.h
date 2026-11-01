#pragma once

#include <functional>

class ParamBase {
public:
    ParamBase(const char* name) : name(name) {}
    virtual ~ParamBase() = default;

    const char* getName() const { return name; }
    virtual void setFromFloat(float value) = 0;
    virtual float toFloat() const = 0;

protected:
    const char* name;
};

template<typename T>
class Param : public ParamBase {
public:
    Param(const char* name, T defaultValue = T{}) 
        : ParamBase(name), value(defaultValue) {}

    T get() const { return value; }
    void set(T v) { value = v; }

    operator T() const { return value; }
    Param& operator=(T v) { value = v; return *this; }

    void setFromFloat(float v) override { value = static_cast<T>(v); }
    float toFloat() const override { return static_cast<float>(value); }

private:
    T value;
};
