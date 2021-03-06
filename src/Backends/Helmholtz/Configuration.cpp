#include "Configuration.h"

namespace CoolProp
{
//
//Configuration::Configuration()
//{
//}
//
//Configuration::~Configuration()
//{
//}

bool get_config_bool(configuration_keys key)
{
    switch(key)
    {
        case NORMALIZE_GAS_CONSTANTS:
            return true;
        case CRITICAL_SPLINES_ENABLED:
            return true;
        default:
            throw ValueError(format("%d is invalid key to get_config_bool",key));
    }
}
double get_config_double(configuration_keys key)
{
    switch(key)
    {
        default:
            throw ValueError(format("%d is invalid key to get_config_double",key));
    }
}
std::string get_config_string(configuration_keys key)
{
    switch(key)
    {
        default:
            throw ValueError(format("%d is invalid key to get_config_string",key));
    }
}

}

