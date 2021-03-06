#if defined(_MSC_VER)
#define _CRTDBG_MAP_ALLOC
#define _CRT_SECURE_NO_WARNINGS
#include <crtdbg.h>
#endif

#include "CoolPropLib.h"
#include "CoolProp.h"
#include "HumidAirProp.h"
#include "DataStructures.h"
#include "Exceptions.h"

#include <string.h>
double convert_from_kSI_to_SI(long iInput, double value)
{
	if (get_debug_level() > 8){
		std::cout << format("%s:%d: convert_from_kSI_to_SI(i=%d,value=%g)\n",__FILE__,__LINE__,iInput,value).c_str();
	}

	switch (iInput)
	{
    case CoolProp::iP:
	case CoolProp::iCpmass:
	case CoolProp::iCp0mass:
	case CoolProp::iSmass:
	case CoolProp::iGmass:
	case CoolProp::iCvmass:
	case CoolProp::iHmass:
	case CoolProp::iUmass:
	case CoolProp::iconductivity:
        return value*1000.0;
	case CoolProp::iDmass:
	case CoolProp::ispeed_sound:
	case CoolProp::iQ:
	case CoolProp::iviscosity:
	case CoolProp::iT:
	case CoolProp::iPrandtl:
	case CoolProp::isurface_tension:
		return value;
	default:
		throw CoolProp::ValueError(format("index [%d] is invalid in convert_from_kSI_to_SI",iInput).c_str());
		break;
	}
	return _HUGE;
}

double convert_from_SI_to_kSI(long iInput, double value)
{
	if (get_debug_level() > 8){
		std::cout << format("%s:%d: convert_from_SI_to_kSI(%d,%g)\n",__FILE__,__LINE__,iInput,value).c_str();
	}

	switch (iInput)
	{
    case CoolProp::iP:
	case CoolProp::iCpmass:
	case CoolProp::iCp0mass:
	case CoolProp::iSmass:
	case CoolProp::iGmass:
	case CoolProp::iCvmass:
	case CoolProp::iHmass:
	case CoolProp::iUmass:
	case CoolProp::iconductivity:
        return value/1000.0;
	case CoolProp::iDmass:
	case CoolProp::iQ:
	case CoolProp::ispeed_sound:
	case CoolProp::iviscosity:
	case CoolProp::iT:
	case CoolProp::isurface_tension:
		return value;
	default:
		throw CoolProp::ValueError(format("index [%d] is invalid in convert_from_SI_to_kSI", iInput).c_str());
		break;
	}
	return _HUGE;
}

EXPORT_CODE long CONVENTION redirect_stdout(const char* file){
	freopen(file, "a+", stdout);
	return 0;
}
EXPORT_CODE int CONVENTION set_reference_stateS(const char *Ref, const char *reference_state)
{
    std::string _Ref = Ref, _reference_state = reference_state;
    try{
        CoolProp::set_reference_stateS(_Ref, _reference_state);
        return true;
    }
    catch(std::exception &e){
        return false;
    }
}
EXPORT_CODE int CONVENTION set_reference_stateD(const char *Ref, double T, double rho, double h0, double s0)
{
    try{
        CoolProp::set_reference_stateD(std::string(Ref), T, rho, h0, s0);
        return true;
    }
    catch(std::exception &e){
        return false;
    }
    
}

// All the function interfaces that point to the single-input Props function
EXPORT_CODE double CONVENTION Props1(const char *FluidName, const char *Output){
    return PropsS(Output, "", 0, "", 0, FluidName);
}
EXPORT_CODE double CONVENTION PropsS(const char *Output, const char* Name1, double Prop1, const char* Name2, double Prop2, const char * Ref){
	double val = Props(Output,Name1[0],Prop1,Name2[0],Prop2,Ref);
	return val;
}
EXPORT_CODE double CONVENTION Props(const char *Output, const char Name1, double Prop1, const char Name2, double Prop2, const char * Ref)
{
    try
    {
        // Get parameter indices
        std::string sName1 = std::string(1, Name1), sName2 = std::string(1, Name2);
        long iOutput = get_param_index(Output);
        long iName1 = CoolProp::get_parameter_index(sName1);
        long iName2 = CoolProp::get_parameter_index(sName2);

        // Convert inputs to SI
        Prop1 = convert_from_kSI_to_SI(iName1, Prop1);
        Prop2 = convert_from_kSI_to_SI(iName2, Prop2);

	    // Call the SI function
        double val = PropsSI(Output, sName1.c_str(), Prop1, sName2.c_str(), Prop2, Ref);

	    // Convert back to unit system
        return convert_from_SI_to_kSI(iOutput, val);
    }
    catch(std::exception &e){CoolProp::set_error_string(e.what()); return _HUGE;}
    catch(...){CoolProp::set_error_string("Undefined error"); return _HUGE;}
}
EXPORT_CODE double CONVENTION Props1SI(const char *FluidName, const char *Output)
{
    std::string _Output = Output, _FluidName = FluidName;
    return CoolProp::Props1SI(_FluidName, _Output);
}
EXPORT_CODE double CONVENTION PropsSI(const char *Output, const char *Name1, double Prop1, const char *Name2, double Prop2, const char * FluidName)
{
    std::string _Output = Output, _Name1 = Name1, _Name2 = Name2, _FluidName = FluidName;
    return CoolProp::PropsSI(_Output, _Name1, Prop1, _Name2, Prop2, _FluidName);
}
EXPORT_CODE long CONVENTION PhaseSI(const char *Output, const char *Name1, double Prop1, const char *Name2, double Prop2, const char * FluidName, char *phase)
{
    std::string _Name1 = Name1, _Name2 = Name2, _FluidName = FluidName;
    std::string ph = CoolProp::PhaseSI(_Name1, Prop1, _Name2, Prop2, _FluidName);
    if (ph.size() > strlen(phase)){ strcpy(phase, ph.c_str()); }
    return 0;
}
EXPORT_CODE double CONVENTION PropsSIZ(const char *Output, const char *Name1, double Prop1, const char *Name2, double Prop2, const char * FluidName, const double *z, int n)
{
    std::string _Output = Output, _Name1 = Name1, _Name2 = Name2, _FluidName = FluidName;
    return CoolProp::PropsSI(_Output, _Name1, Prop1, _Name2, Prop2, _FluidName, std::vector<double>(z, z+n));
}
EXPORT_CODE void CONVENTION propssi_(const char *Output, const char *Name1, double *Prop1, const char *Name2, double *Prop2, const char * FluidName, double *output)
{
    std::string _Output = Output, _Name1 = Name1, _Name2 = Name2, _FluidName = FluidName;
    *output = CoolProp::PropsSI(_Output, _Name1, *Prop1, _Name2, *Prop2, _FluidName);
}

EXPORT_CODE double CONVENTION K2F(double T){
    return T * 9 / 5 - 459.67;
}
EXPORT_CODE double CONVENTION F2K(double T_F){
    return (T_F + 459.67) * 5 / 9;
}
EXPORT_CODE int CONVENTION get_debug_level(){
    return CoolProp::get_debug_level();
}
EXPORT_CODE void CONVENTION set_debug_level(int level){
    CoolProp::set_debug_level(level);
}
EXPORT_CODE long CONVENTION get_param_index(const char * param){
    return CoolProp::get_parameter_index(param);
}
EXPORT_CODE long CONVENTION get_global_param_string(const char *param, char * Output)
{
	strcpy(Output,CoolProp::get_global_param_string(param).c_str());
	return 0;
}
EXPORT_CODE long CONVENTION get_parameter_information_string(const char *param, char * Output)
{
    int key = CoolProp::get_parameter_index(param);
    if (key >= 0){
        strcpy(Output, CoolProp::get_parameter_information(key, Output).c_str());
    }
    else{
        strcpy(Output, format("parameter is invalid: %s", param).c_str());
    }
	return 0;
}
//EXPORT_CODE long CONVENTION get_fluid_param_string(const char *fluid, const char *param, char * Output)
//{
//	strcpy(Output, get_fluid_param_string(std::string(fluid), std::string(param)).c_str());
//	return 0;
//}


EXPORT_CODE double CONVENTION HAPropsSI(const char *Output, const char *Name1, double Prop1, const char *Name2, double Prop2, const char * Name3, double Prop3)
{
    std::string _Output = Output, _Name1 = Name1, _Name2 = Name2, _Name3 = Name3;
	return HumidAir::HAPropsSI(Output, _Name1, Prop1, _Name2, Prop2, _Name3, Prop3);
}
EXPORT_CODE void CONVENTION hapropssi_(const char *Output, const char *Name1, double *Prop1, const char *Name2, double *Prop2, const char * Name3, double * Prop3, double *output)
{
    std::string _Output = Output, _Name1 = Name1, _Name2 = Name2, _Name3 = Name3;
	*output = HumidAir::HAPropsSI(_Output, _Name1, *Prop1, _Name2, *Prop2, _Name3, *Prop3);
}
EXPORT_CODE double CONVENTION HAProps(const char *Output, const char *Name1, double Prop1, const char *Name2, double Prop2, const char * Name3, double Prop3)
{
    std::string _Output = Output, _Name1 = Name1, _Name2 = Name2, _Name3 = Name3;
	return HumidAir::HAProps(Output, _Name1, Prop1, _Name2, Prop2, _Name3, Prop3);
}
EXPORT_CODE void CONVENTION haprops_(const char *Output, const char *Name1, double *Prop1, const char *Name2, double *Prop2, const char * Name3, double * Prop3, double *output)
{
	*output = HAProps(Output, Name1, *Prop1, Name2, *Prop2, Name3, *Prop3);
}
