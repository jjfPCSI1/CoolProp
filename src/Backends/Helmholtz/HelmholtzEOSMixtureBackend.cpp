/*
 * AbstractBackend.cpp
 *
 *  Created on: 20 Dec 2013
 *      Author: jowr
 */

#include <memory>

#if defined(_MSC_VER)
#define _CRTDBG_MAP_ALLOC
#define _CRT_SECURE_NO_WARNINGS
#include <crtdbg.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#endif

#include <string>
//#include "CoolProp.h"

#include "HelmholtzEOSMixtureBackend.h"
#include "Fluids/FluidLibrary.h"
#include "Solvers.h"
#include "MatrixMath.h"
#include "VLERoutines.h"
#include "FlashRoutines.h"
#include "TransportRoutines.h"
#include "MixtureDerivatives.h"
#include "PhaseEnvelopeRoutines.h"
#include "ReducingFunctions.h"
#include "MixtureParameters.h"

static int deriv_counter = 0;

namespace CoolProp {

HelmholtzEOSMixtureBackend::HelmholtzEOSMixtureBackend(std::vector<std::string> &component_names, bool generate_SatL_and_SatV) {
    std::vector<CoolPropFluid*> components;
    components.resize(component_names.size());

    for (unsigned int i = 0; i < components.size(); ++i)
    {
        components[i] = &(get_library().get(component_names[i]));
    }

    // Set the components and associated flags
    set_components(components, generate_SatL_and_SatV);
    
    // Set the phase to default unknown value
    _phase = iphase_unknown;
}
HelmholtzEOSMixtureBackend::HelmholtzEOSMixtureBackend(std::vector<CoolPropFluid*> components, bool generate_SatL_and_SatV) {

    // Set the components and associated flags
    set_components(components, generate_SatL_and_SatV);
    
    // Set the phase to default unknown value
    _phase = iphase_unknown;
}
void HelmholtzEOSMixtureBackend::set_components(std::vector<CoolPropFluid*> components, bool generate_SatL_and_SatV) {

    // Copy the components
    this->components = components;
    this->N = components.size();

    if (components.size() == 1){
        is_pure_or_pseudopure = true;
        mole_fractions = std::vector<long double>(1, 1);
    }
    else{
        is_pure_or_pseudopure = false;
    }

    // Set the excess Helmholtz energy if a mixture
    if (!is_pure_or_pseudopure)
    {
        // Set the mixture parameters - binary pair reducing functions, departure functions, F_ij, etc.
        set_mixture_parameters();
    }

    imposed_phase_index = iphase_not_imposed;

    // Top-level class can hold copies of the base saturation classes,
    // saturation classes cannot hold copies of the saturation classes
    if (generate_SatL_and_SatV)
    {
        SatL.reset(new HelmholtzEOSMixtureBackend(components, false));
        SatL->specify_phase(iphase_liquid);
        SatV.reset(new HelmholtzEOSMixtureBackend(components, false));
        SatV->specify_phase(iphase_gas);
    }
}
void HelmholtzEOSMixtureBackend::set_mole_fractions(const std::vector<long double> &mole_fractions)
{
    if (mole_fractions.size() != N)
    {
        throw ValueError(format("size of mole fraction vector [%d] does not equal that of component vector [%d]",mole_fractions.size(), N));
    }
    // Copy values without reallocating memory
    this->resize(N);
    std::copy( mole_fractions.begin(), mole_fractions.end(), this->mole_fractions.begin() );
    // Resize the vectors for the liquid and vapor,  but only if they are in use
    if (this->SatL.get() != NULL){
        this->SatL->resize(N);
    }
    if (this->SatV.get() != NULL){
        this->SatV->resize(N);
    }
};
void HelmholtzEOSMixtureBackend::resize(unsigned int N)
{
    this->mole_fractions.resize(N);
    this->K.resize(N);
    this->lnK.resize(N);
}
void HelmholtzEOSMixtureBackend::calc_phase_envelope(const std::string &type)
{
    // Clear the phase envelope data
    PhaseEnvelope = PhaseEnvelopeData();
    // Build the phase envelope
    PhaseEnvelopeRoutines::build(*this);
    // Finalize the phase envelope
    PhaseEnvelopeRoutines::finalize(*this);
};
void HelmholtzEOSMixtureBackend::set_mixture_parameters()
{
    // Build the matrix of binary-pair reducing functions
    MixtureParameters::set_mixture_parameters(*this);
}
void HelmholtzEOSMixtureBackend::update_states(void)
{
    CoolPropFluid &component = *(components[0]);
    EquationOfState &EOS = component.EOSVector[0];
    
    // Clear the state class
    clear();
    
    // Calculate the new enthalpy and entropy values
    update(DmolarT_INPUTS, EOS.hs_anchor.rhomolar, EOS.hs_anchor.T);
    EOS.hs_anchor.hmolar = hmolar();
    EOS.hs_anchor.smolar = smolar();
    
    // Calculate the new enthalpy and entropy values at the reducing state
    update(DmolarT_INPUTS, EOS.reduce.rhomolar, EOS.reduce.T);
    EOS.reduce.hmolar = hmolar();
    EOS.reduce.smolar = smolar();
    
    // Clear again just to be sure
    clear();
}
const CoolProp::SimpleState & HelmholtzEOSMixtureBackend::calc_state(const std::string &state)
{
    if (is_pure_or_pseudopure)
    {
        if (!state.compare("hs_anchor")){
            return components[0]->pEOS->hs_anchor;
        }
        else if (!state.compare("max_sat_T")){
            return components[0]->pEOS->max_sat_T;
        }
        else if (!state.compare("max_sat_p")){
            return components[0]->pEOS->max_sat_p;
        }
        else if (!state.compare("reducing")){
            return components[0]->pEOS->reduce;
        }
        else{
            throw ValueError(format("This state [%s] is invalid to calc_state",state.c_str()));
        }
    }
    else{
        throw ValueError(format("calc_state not supported for mixtures"));
    }
};
long double HelmholtzEOSMixtureBackend::calc_gas_constant(void)
{
    if (is_pure_or_pseudopure){
        return components[0]->gas_constant();
    }
    else{
        if (get_config_bool(NORMALIZE_GAS_CONSTANTS)){
            return R_u_CODATA;
        }
        else{
            // mass fraction weighted average of the components
            double summer = 0;
            for (unsigned int i = 0; i < components.size(); ++i)
            {
                summer += mole_fractions[i]*components[i]->gas_constant();
            }
            return summer;
        }
    }
}
long double HelmholtzEOSMixtureBackend::calc_molar_mass(void)
{
    double summer = 0;
    for (unsigned int i = 0; i < components.size(); ++i)
    {
        summer += mole_fractions[i]*components[i]->molar_mass();
    }
    return summer;
}
long double HelmholtzEOSMixtureBackend::calc_saturation_ancillary(parameters param, int Q, parameters given, double value)
{
    if (is_pure_or_pseudopure)
    {
        if (param == iP && given == iT){
            // p = f(T), direct evaluation
            switch (Q)
            {
                case 0:
                    return components[0]->ancillaries.pL.evaluate(value);
                case 1:
                    return components[0]->ancillaries.pV.evaluate(value);
            }
        }
        else if (param == iT && given == iP){
            // T = f(p), inverse evaluation
            switch (Q)
            {
                case 0:
                    return components[0]->ancillaries.pL.invert(value);
                case 1:
                    return components[0]->ancillaries.pV.invert(value);
            }
        }
        else if (param == iDmolar && given == iT){
            // rho = f(T), inverse evaluation
            switch (Q)
            {
                case 0:
                    return components[0]->ancillaries.rhoL.evaluate(value);
                case 1:
                    return components[0]->ancillaries.rhoV.evaluate(value);
            }
        }
        else if (param == iT && given == iDmolar){
            // T = f(rho), inverse evaluation
            switch (Q)
            {
                case 0:
                    return components[0]->ancillaries.rhoL.invert(value);
                case 1:
                    return components[0]->ancillaries.rhoV.invert(value);
            }
        }
        else{
            throw ValueError(format("calc of %s given %s is invalid in calc_saturation_ancillary", 
                                    get_parameter_information(param,"short").c_str(), 
                                    get_parameter_information(given,"short").c_str()));
        }
        
        throw ValueError(format("Q [%d] is invalid in calc_saturation_ancillary", Q));
    }
    else
    {
        throw NotImplementedError(format("calc_saturation_ancillary not implemented for mixtures"));
    }
}

long double HelmholtzEOSMixtureBackend::calc_melting_line(int param, int given, long double value)
{
    if (is_pure_or_pseudopure)
    {
        return components[0]->ancillaries.melting_line.evaluate(param, given, value);
    }
    else
    {
        throw NotImplementedError(format("calc_melting_line not implemented for mixtures"));
    }
}
long double HelmholtzEOSMixtureBackend::calc_viscosity_dilute(void)
{
    if (is_pure_or_pseudopure)
    {
        long double eta_dilute;
        switch(components[0]->transport.viscosity_dilute.type)
        {
        case ViscosityDiluteVariables::VISCOSITY_DILUTE_KINETIC_THEORY:
            eta_dilute = TransportRoutines::viscosity_dilute_kinetic_theory(*this); break;
        case ViscosityDiluteVariables::VISCOSITY_DILUTE_COLLISION_INTEGRAL:
            eta_dilute = TransportRoutines::viscosity_dilute_collision_integral(*this); break;
        case ViscosityDiluteVariables::VISCOSITY_DILUTE_POWERS_OF_T:
            eta_dilute = TransportRoutines::viscosity_dilute_powers_of_T(*this); break;
        case ViscosityDiluteVariables::VISCOSITY_DILUTE_COLLISION_INTEGRAL_POWERS_OF_TSTAR:
            eta_dilute = TransportRoutines::viscosity_dilute_collision_integral_powers_of_T(*this); break;
        case ViscosityDiluteVariables::VISCOSITY_DILUTE_ETHANE:
            eta_dilute = TransportRoutines::viscosity_dilute_ethane(*this); break;
        case ViscosityDiluteVariables::VISCOSITY_DILUTE_CYCLOHEXANE:
            eta_dilute = TransportRoutines::viscosity_dilute_cyclohexane(*this); break;
        default:
            throw ValueError(format("dilute viscosity type [%d] is invalid for fluid %s", components[0]->transport.viscosity_dilute.type, name().c_str()));
        }
        return eta_dilute;
    }
    else
    {
        throw NotImplementedError(format("dilute viscosity not implemented for mixtures"));
    }

}
long double HelmholtzEOSMixtureBackend::calc_viscosity_background()
{
    long double eta_dilute = calc_viscosity_dilute();
    return calc_viscosity_background(eta_dilute);
}
long double HelmholtzEOSMixtureBackend::calc_viscosity_background(long double eta_dilute)
{
    long double initial_part = 0.0;
    
    switch(components[0]->transport.viscosity_initial.type){        
        case ViscosityInitialDensityVariables::VISCOSITY_INITIAL_DENSITY_RAINWATER_FRIEND:
        {
            long double B_eta_initial = TransportRoutines::viscosity_initial_density_dependence_Rainwater_Friend(*this);
            long double rho = rhomolar();
            initial_part = eta_dilute*B_eta_initial*rho;
            break;
        }
        case ViscosityInitialDensityVariables::VISCOSITY_INITIAL_DENSITY_EMPIRICAL:
        {
            initial_part = TransportRoutines::viscosity_initial_density_dependence_empirical(*this);
            break;
        }
        case ViscosityInitialDensityVariables::VISCOSITY_INITIAL_DENSITY_NOT_SET:
        {
            break;
        }
    }

    // Higher order terms
    long double delta_eta_h = 0.0;
    switch(components[0]->transport.viscosity_higher_order.type)
    {
    case ViscosityHigherOrderVariables::VISCOSITY_HIGHER_ORDER_BATSCHINKI_HILDEBRAND:
        delta_eta_h = TransportRoutines::viscosity_higher_order_modified_Batschinski_Hildebrand(*this); break;
    case ViscosityHigherOrderVariables::VISCOSITY_HIGHER_ORDER_FRICTION_THEORY:
        delta_eta_h = TransportRoutines::viscosity_higher_order_friction_theory(*this); break;
    case ViscosityHigherOrderVariables::VISCOSITY_HIGHER_ORDER_HYDROGEN:
        delta_eta_h = TransportRoutines::viscosity_hydrogen_higher_order_hardcoded(*this); break;
    case ViscosityHigherOrderVariables::VISCOSITY_HIGHER_ORDER_HEXANE:
        delta_eta_h = TransportRoutines::viscosity_hexane_higher_order_hardcoded(*this); break;
    case ViscosityHigherOrderVariables::VISCOSITY_HIGHER_ORDER_HEPTANE:
        delta_eta_h = TransportRoutines::viscosity_heptane_higher_order_hardcoded(*this); break;
    case ViscosityHigherOrderVariables::VISCOSITY_HIGHER_ORDER_ETHANE:
        delta_eta_h = TransportRoutines::viscosity_ethane_higher_order_hardcoded(*this); break;
    case ViscosityHigherOrderVariables::VISCOSITY_HIGHER_ORDER_BENZENE:
        delta_eta_h = TransportRoutines::viscosity_benzene_higher_order_hardcoded(*this); break;
    default:
        throw ValueError(format("higher order viscosity type [%d] is invalid for fluid %s", components[0]->transport.viscosity_dilute.type, name().c_str()));
    }

    long double eta_residual = initial_part + delta_eta_h;

    return eta_residual;
}

long double HelmholtzEOSMixtureBackend::calc_viscosity(void)
{
    if (is_pure_or_pseudopure)
    {
        // Get a reference for code cleanness
        CoolPropFluid &component = *(components[0]);

        // Check if using ECS
        if (component.transport.viscosity_using_ECS)
        {
            // Get reference fluid name
            std::string fluid_name  = component.transport.viscosity_ecs.reference_fluid;
            std::vector<std::string> names(1, fluid_name);
            // Get a managed pointer to the reference fluid for ECS
            shared_ptr<HelmholtzEOSMixtureBackend> ref_fluid(new HelmholtzEOSMixtureBackend(names));
            // Get the viscosity using ECS
            return TransportRoutines::viscosity_ECS(*this, *(ref_fluid.get()));
        }

        if (component.transport.hardcoded_viscosity != CoolProp::TransportPropertyData::VISCOSITY_NOT_HARDCODED)
        {
            switch(component.transport.hardcoded_viscosity)
            {
            case CoolProp::TransportPropertyData::VISCOSITY_HARDCODED_WATER:
                return TransportRoutines::viscosity_water_hardcoded(*this);
            case CoolProp::TransportPropertyData::VISCOSITY_HARDCODED_HELIUM:
                return TransportRoutines::viscosity_helium_hardcoded(*this);
            case CoolProp::TransportPropertyData::VISCOSITY_HARDCODED_R23:
                return TransportRoutines::viscosity_R23_hardcoded(*this);
            case CoolProp::TransportPropertyData::VISCOSITY_HARDCODED_METHANOL:
                return TransportRoutines::viscosity_methanol_hardcoded(*this);
            default:
                throw ValueError(format("hardcoded viscosity type [%d] is invalid for fluid %s", component.transport.hardcoded_viscosity, name().c_str()));
            }
        }
        // Dilute part
        long double eta_dilute = calc_viscosity_dilute();

        // Background viscosity given by the sum of the initial density dependence and higher order terms
        long double eta_back = calc_viscosity_background(eta_dilute);

        // Critical part (no fluids have critical enhancement for viscosity currently)
        long double eta_critical = 0;

        return eta_dilute + eta_back + eta_critical;
    }
    else
    {
        throw NotImplementedError(format("viscosity not implemented for mixtures"));
    }
}
long double HelmholtzEOSMixtureBackend::calc_conductivity_background(void)
{
    // Residual part
    long double lambda_residual = _HUGE;
    switch(components[0]->transport.conductivity_residual.type)
    {
    case ConductivityResidualVariables::CONDUCTIVITY_RESIDUAL_POLYNOMIAL:
        lambda_residual = TransportRoutines::conductivity_residual_polynomial(*this); break;
    case ConductivityResidualVariables::CONDUCTIVITY_RESIDUAL_POLYNOMIAL_AND_EXPONENTIAL:
        lambda_residual = TransportRoutines::conductivity_residual_polynomial_and_exponential(*this); break;
    default:
        throw ValueError(format("residual conductivity type [%d] is invalid for fluid %s", components[0]->transport.conductivity_residual.type, name().c_str()));
    }
    return lambda_residual;
}
long double HelmholtzEOSMixtureBackend::calc_conductivity(void)
{
    if (is_pure_or_pseudopure)
    {
        // Get a reference for code cleanness
        CoolPropFluid &component = *(components[0]);

        // Check if using ECS
        if (component.transport.conductivity_using_ECS)
        {
            // Get reference fluid name
            std::string fluid_name  = component.transport.conductivity_ecs.reference_fluid;
            std::vector<std::string> name(1, fluid_name);
            // Get a managed pointer to the reference fluid for ECS
            shared_ptr<HelmholtzEOSMixtureBackend> ref_fluid(new HelmholtzEOSMixtureBackend(name,false));
            // Get the viscosity using ECS
            return TransportRoutines::conductivity_ECS(*this, *(ref_fluid.get()));
        }

        if (component.transport.hardcoded_conductivity != CoolProp::TransportPropertyData::CONDUCTIVITY_NOT_HARDCODED)
        {
            switch(component.transport.hardcoded_conductivity)
            {
            case CoolProp::TransportPropertyData::CONDUCTIVITY_HARDCODED_WATER:
                return TransportRoutines::conductivity_hardcoded_water(*this);
            case CoolProp::TransportPropertyData::CONDUCTIVITY_HARDCODED_R23:
                return TransportRoutines::conductivity_hardcoded_R23(*this);
            case CoolProp::TransportPropertyData::CONDUCTIVITY_HARDCODED_HELIUM:
                return TransportRoutines::conductivity_hardcoded_helium(*this);
            default:
                throw ValueError(format("hardcoded viscosity type [%d] is invalid for fluid %s", components[0]->transport.hardcoded_conductivity, name().c_str()));
            }
        }

        // Dilute part
        long double lambda_dilute = _HUGE;
        switch(component.transport.conductivity_dilute.type)
        {
        case ConductivityDiluteVariables::CONDUCTIVITY_DILUTE_RATIO_POLYNOMIALS:
            lambda_dilute = TransportRoutines::conductivity_dilute_ratio_polynomials(*this); break;
        case ConductivityDiluteVariables::CONDUCTIVITY_DILUTE_ETA0_AND_POLY:
            lambda_dilute = TransportRoutines::conductivity_dilute_eta0_and_poly(*this); break;
        case ConductivityDiluteVariables::CONDUCTIVITY_DILUTE_CO2:
            lambda_dilute = TransportRoutines::conductivity_dilute_hardcoded_CO2(*this); break;
        case ConductivityDiluteVariables::CONDUCTIVITY_DILUTE_ETHANE:
            lambda_dilute = TransportRoutines::conductivity_dilute_hardcoded_ethane(*this); break;
        case ConductivityDiluteVariables::CONDUCTIVITY_DILUTE_NONE:
            lambda_dilute = 0.0; break;
        default:
            throw ValueError(format("dilute conductivity type [%d] is invalid for fluid %s", components[0]->transport.conductivity_dilute.type, name().c_str()));
        }

        long double lambda_residual = calc_conductivity_background();

        // Critical part
        long double lambda_critical = _HUGE;
        switch(component.transport.conductivity_critical.type)
        {
        case ConductivityCriticalVariables::CONDUCTIVITY_CRITICAL_SIMPLIFIED_OLCHOWY_SENGERS:
            lambda_critical = TransportRoutines::conductivity_critical_simplified_Olchowy_Sengers(*this); break;
        case ConductivityCriticalVariables::CONDUCTIVITY_CRITICAL_R123:
            lambda_critical = TransportRoutines::conductivity_critical_hardcoded_R123(*this); break;
        case ConductivityCriticalVariables::CONDUCTIVITY_CRITICAL_AMMONIA:
            lambda_critical = TransportRoutines::conductivity_critical_hardcoded_ammonia(*this); break;
        case ConductivityCriticalVariables::CONDUCTIVITY_CRITICAL_NONE:
            lambda_critical = 0.0; break;
        case ConductivityCriticalVariables::CONDUCTIVITY_CRITICAL_CARBONDIOXIDE_SCALABRIN_JPCRD_2006:
            lambda_critical = TransportRoutines::conductivity_critical_hardcoded_CO2_ScalabrinJPCRD2006(*this); break;
        default:
            throw ValueError(format("critical conductivity type [%d] is invalid for fluid %s", components[0]->transport.viscosity_dilute.type, name().c_str()));
        }

        return lambda_dilute + lambda_residual + lambda_critical;
    }
    else
    {
        throw NotImplementedError(format("viscosity not implemented for mixtures"));
    }
}
long double HelmholtzEOSMixtureBackend::calc_Ttriple(void)
{
    double summer = 0;
    for (unsigned int i = 0; i < components.size(); ++i){
        summer += mole_fractions[i]*components[i]->pEOS->Ttriple;
    }
    return summer;
}
long double HelmholtzEOSMixtureBackend::calc_p_triple(void)
{
    double summer = 0;
    for (unsigned int i = 0; i < components.size(); ++i){
        summer += mole_fractions[i]*components[i]->pEOS->ptriple;
    }
    return summer;
}
std::string HelmholtzEOSMixtureBackend::calc_name(void)
{
    if (components.size() != 1){
        throw ValueError(format("calc_name is only valid for pure and pseudo-pure fluids, %d components", components.size()));
    }
    else{
        return components[0]->name;
    }
}
long double HelmholtzEOSMixtureBackend::calc_T_critical(void)
{
    if (components.size() != 1){
        throw ValueError(format("For now, calc_T_critical is only valid for pure and pseudo-pure fluids, %d components", components.size()));
    }
    else{
        return components[0]->crit.T;
    }
}
long double HelmholtzEOSMixtureBackend::calc_p_critical(void)
{
    if (components.size() != 1){
        throw ValueError(format("For now, calc_p_critical is only valid for pure and pseudo-pure fluids, %d components", components.size()));
    }
    else{
        return components[0]->crit.p;
    }
}
long double HelmholtzEOSMixtureBackend::calc_rhomolar_critical(void)
{
    if (components.size() != 1){
        throw ValueError(format("For now, calc_rhomolar_critical is only valid for pure and pseudo-pure fluids, %d components", components.size()));
    }
    else{
        return components[0]->crit.rhomolar;
    }
}
long double HelmholtzEOSMixtureBackend::calc_pmax_sat(void)
{
	if (is_pure_or_pseudopure)
	{
		if (components[0]->pEOS->pseudo_pure)
		{
			return components[0]->pEOS->max_sat_p.p;
		}
		else{
			return p_critical();
		}
	}
	else{
		throw ValueError("calc_pmax_sat not yet defined for mixtures");
	}
}
long double HelmholtzEOSMixtureBackend::calc_Tmax_sat(void)
{
	if (is_pure_or_pseudopure)
	{
		if (components[0]->pEOS->pseudo_pure)
		{
			return components[0]->pEOS->max_sat_T.T;
		}
		else{
			return T_critical();
		}
	}
	else{
		throw ValueError("calc_Tmax_sat not yet defined for mixtures");
	}
}

void HelmholtzEOSMixtureBackend::calc_Tmin_sat(long double &Tmin_satL, long double &Tmin_satV)
{
	if (is_pure_or_pseudopure)
	{
		Tmin_satL = components[0]->pEOS->sat_min_liquid.T;
		Tmin_satV = components[0]->pEOS->sat_min_vapor.T;
		return;
	}
	else{
		throw ValueError("calc_Tmin_sat not yet defined for mixtures");
	}
}

void HelmholtzEOSMixtureBackend::calc_pmin_sat(long double &pmin_satL, long double &pmin_satV)
{
	if (is_pure_or_pseudopure)
	{
		pmin_satL = components[0]->pEOS->sat_min_liquid.p;
		pmin_satV = components[0]->pEOS->sat_min_vapor.p;
		return;
	}
	else{
		throw ValueError("calc_pmin_sat not yet defined for mixtures");
	}
}

// Minimum allowed saturation temperature the maximum of the saturation temperatures of liquid and vapor
		// For pure fluids, both values are the same, for pseudo-pure they are probably the same, for mixtures they are definitely not the same

long double HelmholtzEOSMixtureBackend::calc_Tmax(void)
{
    double summer = 0;
    for (unsigned int i = 0; i < components.size(); ++i)
    {
        summer += mole_fractions[i]*components[i]->pEOS->limits.Tmax;
    }
    return summer;
}
long double HelmholtzEOSMixtureBackend::calc_Tmin(void)
{
    double summer = 0;
    for (unsigned int i = 0; i < components.size(); ++i)
    {
        summer += mole_fractions[i]*components[i]->pEOS->limits.Tmin;
    }
    return summer;
}
long double HelmholtzEOSMixtureBackend::calc_pmax(void)
{
    double summer = 0;
    for (unsigned int i = 0; i < components.size(); ++i)
    {
        summer += mole_fractions[i]*components[i]->pEOS->limits.pmax;
    }
    return summer;
}

void HelmholtzEOSMixtureBackend::update_DmolarT_direct(long double rhomolar, long double T)
{
    CoolProp::input_pairs pair = DmolarT_INPUTS;
    // Set up the state
    pre_update(pair, rhomolar, T);
    
    _rhomolar = rhomolar;
    _T = T;
    _p = calc_pressure();
    _Q = -1;
    
    // Cleanup
    post_update();
}

void HelmholtzEOSMixtureBackend::update_TP_guessrho(long double T, long double p, long double rhomolar_guess)
{
    CoolProp::input_pairs pair = PT_INPUTS;
    // Set up the state
    pre_update(pair, p, T);
    
    // Do the flash call
    long double rhomolar = solver_rho_Tp(T, p, rhomolar_guess);
    
    // Update the class with the new calculated density
    update(DmolarT_INPUTS, rhomolar, T);
    
    // Cleanup
    post_update();
}

void HelmholtzEOSMixtureBackend::mass_to_molar_inputs(CoolProp::input_pairs &input_pair, long double &value1, long double &value2)
{
    // Check if a mass based input, convert it to molar units

    switch(input_pair)
    {
        case DmassT_INPUTS: ///< Mass density in kg/m^3, Temperature in K
        case HmassT_INPUTS: ///< Enthalpy in J/kg, Temperature in K
        case SmassT_INPUTS: ///< Entropy in J/kg/K, Temperature in K
        case TUmass_INPUTS: ///< Temperature in K, Internal energy in J/kg
        case DmassP_INPUTS: ///< Mass density in kg/m^3, Pressure in Pa
        case HmassP_INPUTS: ///< Enthalpy in J/kg, Pressure in Pa
        case PSmass_INPUTS: ///< Pressure in Pa, Entropy in J/kg/K
        case PUmass_INPUTS: ///< Pressure in Pa, Internal energy in J/kg
        case HmassSmass_INPUTS: ///< Enthalpy in J/kg, Entropy in J/kg/K
        case SmassUmass_INPUTS: ///< Entropy in J/kg/K, Internal energy in J/kg
        case DmassHmass_INPUTS: ///< Mass density in kg/m^3, Enthalpy in J/kg
        case DmassSmass_INPUTS: ///< Mass density in kg/m^3, Entropy in J/kg/K
        case DmassUmass_INPUTS: ///< Mass density in kg/m^3, Internal energy in J/kg
        {
            // Set the cache value for the molar mass if it hasn't been set yet
            molar_mass();

            // Molar mass (just for compactness of the following switch)
            long double mm = static_cast<long double>(_molar_mass);

            switch(input_pair)
            {
                case DmassT_INPUTS: input_pair = DmolarT_INPUTS; value1 /= mm;  break;
                case HmassT_INPUTS: input_pair = HmolarT_INPUTS; value1 *= mm;  break;
                case SmassT_INPUTS: input_pair = SmolarT_INPUTS; value1 *= mm;  break;
                case TUmass_INPUTS: input_pair = TUmolar_INPUTS; value2 *= mm;  break;
                case DmassP_INPUTS: input_pair = DmolarP_INPUTS; value1 /= mm;  break;
                case HmassP_INPUTS: input_pair = HmolarP_INPUTS; value1 *= mm;  break;
                case PSmass_INPUTS: input_pair = PSmolar_INPUTS; value2 *= mm;  break;
                case PUmass_INPUTS: input_pair = PUmolar_INPUTS; value2 *= mm;  break;
                case HmassSmass_INPUTS: input_pair = HmolarSmolar_INPUTS; value1 *= mm; value2 *= mm;  break;
                case SmassUmass_INPUTS: input_pair = SmolarUmolar_INPUTS; value1 *= mm; value2 *= mm;  break;
                case DmassHmass_INPUTS: input_pair = DmolarHmolar_INPUTS; value1 /= mm; value2 *= mm;  break;
                case DmassSmass_INPUTS: input_pair = DmolarSmolar_INPUTS; value1 /= mm; value2 *= mm;  break;
                case DmassUmass_INPUTS: input_pair = DmolarUmolar_INPUTS; value1 /= mm; value2 *= mm;  break;
                default: break;
            }
            break;
        }
        default:
            return;
    }
}

void HelmholtzEOSMixtureBackend::pre_update(CoolProp::input_pairs &input_pair, long double &value1, long double &value2 )
{
    // Clear the state
    clear();

    if (is_pure_or_pseudopure == false && mole_fractions.size() == 0) {
        throw ValueError("Mole fractions must be set");
    }
    
    // If the inputs are in mass units, convert them to molar units
    mass_to_molar_inputs(input_pair, value1, value2);

    // Set the mole-fraction weighted gas constant for the mixture
    // (or the pure/pseudo-pure fluid) if it hasn't been set yet
    gas_constant();

    // Calculate and cache the reducing state
    calc_reducing_state();
}

void HelmholtzEOSMixtureBackend::update(CoolProp::input_pairs input_pair, double value1, double value2 )
{
    if (get_debug_level() > 10){std::cout << format("%s (%d): update called with (%d: (%s), %g, %g)",__FILE__,__LINE__, input_pair, get_input_pair_short_desc(input_pair).c_str(), value1, value2) << std::endl;}
    
    long double ld_value1 = value1, ld_value2 = value2;
    pre_update(input_pair, ld_value1, ld_value2);
    value1 = ld_value1; value2 = ld_value2;

    switch(input_pair)
    {
        case PT_INPUTS:
            _p = value1; _T = value2; FlashRoutines::PT_flash(*this); break;
        case DmolarT_INPUTS:
            _rhomolar = value1; _T = value2; FlashRoutines::DHSU_T_flash(*this, iDmolar); break;
        case SmolarT_INPUTS:
            _smolar = value1; _T = value2; FlashRoutines::DHSU_T_flash(*this, iSmolar); break;
        case HmolarT_INPUTS:
            _hmolar = value1; _T = value2; FlashRoutines::DHSU_T_flash(*this, iHmolar); break;
        case TUmolar_INPUTS:
            _T = value1; _umolar = value2; FlashRoutines::DHSU_T_flash(*this, iUmolar); break;
        case DmolarP_INPUTS:
            _rhomolar = value1; _p = value2; FlashRoutines::PHSU_D_flash(*this, iP); break;
        case DmolarHmolar_INPUTS:
            _rhomolar = value1; _hmolar = value2; FlashRoutines::PHSU_D_flash(*this, iHmolar); break;
        case DmolarSmolar_INPUTS:
            _rhomolar = value1; _smolar = value2; FlashRoutines::PHSU_D_flash(*this, iSmolar); break;
        case DmolarUmolar_INPUTS:
            _rhomolar = value1; _umolar = value2; FlashRoutines::PHSU_D_flash(*this, iUmolar); break;
        case HmolarP_INPUTS:
            _hmolar = value1; _p = value2; FlashRoutines::HSU_P_flash(*this, iHmolar); break;
        case PSmolar_INPUTS:
            _p = value1; _smolar = value2; FlashRoutines::HSU_P_flash(*this, iSmolar); break;
        case PUmolar_INPUTS:
            _p = value1; _umolar = value2; FlashRoutines::HSU_P_flash(*this, iUmolar); break;
        case HmolarSmolar_INPUTS:
            _hmolar = value1; _smolar = value2; FlashRoutines::HS_flash(*this); break;
        case QT_INPUTS:
            _Q = value1; _T = value2; FlashRoutines::QT_flash(*this); break;
        case PQ_INPUTS:
            _p = value1; _Q = value2; FlashRoutines::PQ_flash(*this); break;
        case QS_INPUTS:
            _Q = value1; _smolar = value2; FlashRoutines::QS_flash(*this); break;
        case HQ_INPUTS:
            _hmolar = value1; _Q = value2; FlashRoutines::HQ_flash(*this); break;
        default:
            throw ValueError(format("This pair of inputs [%s] is not yet supported", get_input_pair_short_desc(input_pair).c_str()));
    }
    
    post_update();
    
}

void HelmholtzEOSMixtureBackend::post_update()
{
    // Check the values that must always be set
    //if (_p < 0){ throw ValueError("p is less than zero");}
    if (!ValidNumber(_p)){ 
        throw ValueError("p is not a valid number");}
    //if (_T < 0){ throw ValueError("T is less than zero");}
    if (!ValidNumber(_T)){ throw ValueError("T is not a valid number");}
    if (_rhomolar < 0){ throw ValueError("rhomolar is less than zero");}
    if (!ValidNumber(_rhomolar)){ throw ValueError("rhomolar is not a valid number");}
    if (!ValidNumber(_Q)){ throw ValueError("Q is not a valid number");}
    if (_phase == iphase_unknown){ 
            throw ValueError("_phase is unknown");
    }

    // Set the reduced variables
    _tau = _reducing.T/_T;
    _delta = _rhomolar/_reducing.rhomolar;
}

long double HelmholtzEOSMixtureBackend::calc_Bvirial()
{
    return 1/get_reducing_state().rhomolar*calc_alphar_deriv_nocache(0,1,mole_fractions,_tau,1e-12);
}
long double HelmholtzEOSMixtureBackend::calc_dBvirial_dT()
{
    long double dtau_dT =-get_reducing_state().T/pow(_T,2);
    return 1/get_reducing_state().rhomolar*calc_alphar_deriv_nocache(1,1,mole_fractions,_tau,1e-12)*dtau_dT;
}
long double HelmholtzEOSMixtureBackend::calc_Cvirial()
{
    return 1/pow(get_reducing_state().rhomolar,2)*calc_alphar_deriv_nocache(0,2,mole_fractions,_tau,1e-12);
}
long double HelmholtzEOSMixtureBackend::calc_dCvirial_dT()
{
    long double dtau_dT =-get_reducing_state().T/pow(_T,2);
    return 1/pow(get_reducing_state().rhomolar,2)*calc_alphar_deriv_nocache(1,2,mole_fractions,_tau,1e-12)*dtau_dT;
}
void HelmholtzEOSMixtureBackend::p_phase_determination_pure_or_pseudopure(int other, long double value, bool &saturation_called)
{
    /*
    Determine the phase given p and one other state variable
    */
    saturation_called = false;
    
    // Reference declaration to save indexing
    CoolPropFluid &component = *(components[0]);
    
    // Check supercritical pressure
    if (_p > _crit.p)
    {
        _Q = 1e9;
        switch (other)
        {
            case iT:
            {
                if (_T > _crit.T){
                    this->_phase = iphase_supercritical; return;
                }
                else{
                    this->_phase = iphase_supercritical_liquid; return;
                }
            }
            case iDmolar:
            {
                if (_rhomolar < _crit.rhomolar){
                    this->_phase = iphase_supercritical_gas; return;
                }
                else{
                    this->_phase = iphase_supercritical_liquid; return;
                }
            }
            case iSmolar:
            {
                if (_smolar.pt() > _crit.smolar){
                    this->_phase = iphase_supercritical_gas; return;
                }
                else{
                    this->_phase = iphase_supercritical_liquid; return;
                }
            }
            case iHmolar:
            {
                if (_hmolar.pt() > _crit.hmolar){
                    this->_phase = iphase_supercritical_gas; return;
                }
                else{
                    this->_phase = iphase_supercritical_liquid; return;
                }
            }
            case iUmolar:
            {
                if (_umolar.pt() > _crit.umolar){
                    this->_phase = iphase_supercritical_gas; return;
                }
                else{
                    this->_phase = iphase_supercritical_liquid; return;
                }
            }
            default:
            {
                throw ValueError("supercritical pressure but other invalid for now");
            }
        }
    }
    // Check between triple point pressure and psat_max
    else if (_p >= components[0]->pEOS->ptriple*0.9999 && _p <= _crit.p)
    {
        // First try the ancillaries, use them to determine the state if you can
        
        // Calculate dew and bubble temps from the ancillaries (everything needs them)
        _TLanc = components[0]->ancillaries.pL.invert(_p);
        _TVanc = components[0]->ancillaries.pV.invert(_p);
        
        bool definitely_two_phase = false;
        
        // Try using the ancillaries for P,H,S if they are there
        switch (other)
        {
            case iT:
            {
                long double T_vap =  0.1 + static_cast<double>(_TVanc);
                long double T_liq = -0.1 + static_cast<double>(_TLanc);

                if (value > T_vap){
                    this->_phase = iphase_gas; _Q = -1000; return;
                }
                else if (value < T_liq){
                    this->_phase = iphase_liquid; _Q = 1000; return;
                }
                break;
            }
            case iHmolar:
            {
                if (!component.ancillaries.hL.enabled()){break;}
                // Ancillaries are h-h_anchor, so add back h_anchor
                long double h_liq = component.ancillaries.hL.evaluate(_TLanc) + component.pEOS->hs_anchor.hmolar;
                long double h_liq_error_band = component.ancillaries.hL.get_max_abs_error();
                long double h_vap = h_liq + component.ancillaries.hLV.evaluate(_TLanc);
                long double h_vap_error_band = h_liq_error_band + component.ancillaries.hLV.get_max_abs_error();
                                
//                HelmholtzEOSMixtureBackend HEOS(components);
//                HEOS.update(QT_INPUTS, 1, _TLanc);
//                double h1 = HEOS.hmolar();
//                HEOS.update(QT_INPUTS, 0, _TLanc);
//                double h0 = HEOS.hmolar();
                
                // Check if in range given the accuracy of the fit
                if (value > h_vap + h_vap_error_band){
                    this->_phase = iphase_gas; _Q = -1000; return;
                }
                else if (value < h_liq - h_liq_error_band){
                    this->_phase = iphase_liquid; _Q = 1000; return;
                }
                else if (value > h_liq + h_liq_error_band && value < h_vap - h_vap_error_band){ definitely_two_phase = true;}
                break;
            }
            case iSmolar:
            {
                if (!component.ancillaries.sL.enabled()){break;}
                // Ancillaries are s-s_anchor, so add back s_anchor
                long double s_anchor = component.EOSVector[0].hs_anchor.smolar;
                long double s_liq = component.ancillaries.sL.evaluate(_TLanc) + s_anchor;
                long double s_liq_error_band = component.ancillaries.sL.get_max_abs_error();
                long double s_vap = s_liq + component.ancillaries.sLV.evaluate(_TVanc);
                long double s_vap_error_band = s_liq_error_band + component.ancillaries.sLV.get_max_abs_error();
                
                // Check if in range given the accuracy of the fit
                if (value > s_vap + s_vap_error_band){
                    this->_phase = iphase_gas; _Q = -1000; return;
                }
                else if (value < s_liq - s_liq_error_band){
                    this->_phase = iphase_liquid; _Q = 1000; return;
                }
                else if (value > s_liq + s_liq_error_band && value < s_vap - s_vap_error_band){ definitely_two_phase = true;}
				break;
            }
            case iUmolar:
            {
                if (!component.ancillaries.hL.enabled()){break;}
				// u = h-p/rho
                
				// Ancillaries are h-h_anchor, so add back h_anchor
                long double h_liq = component.ancillaries.hL.evaluate(_TLanc) + component.EOSVector[0].hs_anchor.hmolar;
                long double h_liq_error_band = component.ancillaries.hL.get_max_abs_error();
                long double h_vap = h_liq + component.ancillaries.hLV.evaluate(_TLanc);
                long double h_vap_error_band = h_liq_error_band + component.ancillaries.hLV.get_max_abs_error();
				long double rho_vap = component.ancillaries.rhoV.evaluate(_TVanc);
				long double rho_liq = component.ancillaries.rhoL.evaluate(_TLanc);
				long double u_liq = h_liq-_p/rho_liq;
				long double u_vap = h_vap-_p/rho_vap;
				long double u_liq_error_band = 1.5*h_liq_error_band; // Most of error is in enthalpy
				long double u_vap_error_band = 1.5*h_vap_error_band; // Most of error is in enthalpy
				
				// Check if in range given the accuracy of the fit
                if (value > u_vap + u_vap_error_band){
                    this->_phase = iphase_gas; _Q = -1000; return;
                }
                else if (value < u_liq - u_liq_error_band){
                    this->_phase = iphase_liquid; _Q = 1000; return;
                }
                else if (value > u_liq + u_liq_error_band && value < u_vap - u_vap_error_band){ definitely_two_phase = true;}
				break;
                
            }
            default:
            {
            }
        }
        
        // Now either density is an input, or an ancillary for h,s,u is missing
        // Always calculate the densities using the ancillaries
        if (!definitely_two_phase)
        {
            _rhoVanc = component.ancillaries.rhoV.evaluate(_TVanc);
            _rhoLanc = component.ancillaries.rhoL.evaluate(_TLanc);
            long double rho_vap = 0.95*static_cast<double>(_rhoVanc);
            long double rho_liq = 1.05*static_cast<double>(_rhoLanc);
            switch (other)
            {
                case iDmolar:
                {
                    if (value < rho_vap){
                        this->_phase = iphase_gas; return;
                    }
                    else if (value > rho_liq){
                        this->_phase = iphase_liquid; return;
                    }
                    break;
                }
            }
        }

        if (!is_pure_or_pseudopure){throw ValueError("possibly two-phase inputs not supported for pseudo-pure for now");}

        // Actually have to use saturation information sadly
        // For the given pressure, find the saturation state
        // Run the saturation routines to determine the saturation densities and pressures
        HelmholtzEOSMixtureBackend HEOS(components);
        HEOS._p = this->_p;
        HEOS._Q = 0; // ?? What is the best to do here? Doesn't matter for our purposes since pure fluid
        FlashRoutines::PQ_flash(HEOS);

        // We called the saturation routines, so HEOS.SatL and HEOS.SatV are now updated
        // with the saturated liquid and vapor values, which can therefore be used in
        // the other solvers
        saturation_called = true;
        
        long double Q;

        if (other == iT){
            if (value < HEOS.SatL->T()-100*DBL_EPSILON){
                this->_phase = iphase_liquid; _Q = -1000;  return;
            }
            else if (value > HEOS.SatV->T()+100*DBL_EPSILON){
                this->_phase = iphase_gas; _Q = 1000; return;
            }
            else{
                this->_phase = iphase_twophase;
            }
        }
        switch (other)
        {
            case iDmolar:
                Q = (1/value-1/HEOS.SatL->rhomolar())/(1/HEOS.SatV->rhomolar()-1/HEOS.SatL->rhomolar()); break;
            case iSmolar:
                Q = (value - HEOS.SatL->smolar())/(HEOS.SatV->smolar() - HEOS.SatL->smolar()); break;
            case iHmolar:
                Q = (value - HEOS.SatL->hmolar())/(HEOS.SatV->hmolar() - HEOS.SatL->hmolar()); break;
            case iUmolar:
                Q = (value - HEOS.SatL->umolar())/(HEOS.SatV->umolar() - HEOS.SatL->umolar()); break;
            default:
                throw ValueError(format("bad input for other"));
        }
        // Update the states
        this->SatL->update(DmolarT_INPUTS, HEOS.SatL->rhomolar(), HEOS.SatL->T());
        this->SatV->update(DmolarT_INPUTS, HEOS.SatV->rhomolar(), HEOS.SatV->T());
        if (Q < -100*DBL_EPSILON){
            this->_phase = iphase_liquid; _Q = -1000;  return;
        }
        else if (Q > 1+100*DBL_EPSILON){
            this->_phase = iphase_gas; _Q = 1000; return;
        }
        else{
            this->_phase = iphase_twophase;
        }
        
        _Q = Q;
        // Load the outputs
        _T = _Q*HEOS.SatV->T() + (1-_Q)*HEOS.SatL->T();
        _rhomolar = 1/(_Q/HEOS.SatV->rhomolar() + (1-_Q)/HEOS.SatL->rhomolar());
        return;
    }
    else if (_p < components[0]->pEOS->ptriple*0.9999)
    {
        throw NotImplementedError(format("for now, we don't support p [%g Pa] below ptriple [%g Pa]",_p, components[0]->pEOS->ptriple));
    }
    else{
        throw ValueError(format("The pressure [%g Pa] cannot be used in p_phase_determination",_p));
    }
}
void HelmholtzEOSMixtureBackend::calc_ssat_max(void)
{
    class Residual : public FuncWrapper1D
    {
        public:
        HelmholtzEOSMixtureBackend *HEOS;
        Residual(HelmholtzEOSMixtureBackend &HEOS): HEOS(&HEOS){};
        double call(double T){
            HEOS->update(QT_INPUTS, 1, T);
            // dTdp_along_sat
            double dTdp_along_sat = HEOS->T()*(1/HEOS->SatV->rhomolar()-1/HEOS->SatL->rhomolar())/(HEOS->SatV->hmolar()-HEOS->SatL->hmolar());
            // dsdT_along_sat;
            return HEOS->SatV->first_partial_deriv(iSmolar,iT,iP)+HEOS->SatV->first_partial_deriv(iSmolar,iP,iT)/dTdp_along_sat;
        }
    };
    if (!ssat_max.is_valid())
    {
        Residual resid(*this);
        std::string errstr;
        Secant(resid, T_critical() - 1, 0.1, 1e-8, 30, errstr);
        ssat_max.T = resid.HEOS->T();
        ssat_max.p = resid.HEOS->p();
        ssat_max.rhomolar = resid.HEOS->rhomolar();
        ssat_max.hmolar = resid.HEOS->hmolar();
        ssat_max.smolar = resid.HEOS->smolar();
    }
}
void HelmholtzEOSMixtureBackend::calc_hsat_max(void)
{
    class Residualhmax : public FuncWrapper1D
    {
        public:
        HelmholtzEOSMixtureBackend *HEOS;
        Residualhmax(HelmholtzEOSMixtureBackend &HEOS): HEOS(&HEOS){};
        double call(double T){
            HEOS->update(QT_INPUTS, 1, T);
            // dTdp_along_sat
            double dTdp_along_sat = HEOS->T()*(1/HEOS->SatV->rhomolar()-1/HEOS->SatL->rhomolar())/(HEOS->SatV->hmolar()-HEOS->SatL->hmolar());
            // dhdT_along_sat;
            return HEOS->SatV->first_partial_deriv(iHmolar,iT,iP)+HEOS->SatV->first_partial_deriv(iHmolar,iP,iT)/dTdp_along_sat;
        }
    };
    if (!hsat_max.is_valid())
    {
        Residualhmax residhmax(*this);
        std::string errstrhmax;
        Secant(residhmax, T_critical() - 1, 0.1, 1e-8, 30, errstrhmax);
        hsat_max.T = residhmax.HEOS->T();
        hsat_max.p = residhmax.HEOS->p();
        hsat_max.rhomolar = residhmax.HEOS->rhomolar();
        hsat_max.hmolar = residhmax.HEOS->hmolar();
        hsat_max.smolar = residhmax.HEOS->smolar();
    }
}
void HelmholtzEOSMixtureBackend::T_phase_determination_pure_or_pseudopure(int other, long double value)
{
    if (!ValidNumber(value)){
        throw ValueError(format("value to T_phase_determination_pure_or_pseudopure is invalid"));};
    
    // T is known, another input P, T, H, S, U is given (all molar)
    if (_T < _crit.T && _p > _crit.p){
        _phase = iphase_supercritical_liquid;
    }
    else if (std::abs(_T - _crit.T) < 10*DBL_EPSILON)
    {
        switch (other)
        {
            case iDmolar:
                if (std::abs(_rhomolar - _crit.rhomolar) < 10*DBL_EPSILON){
                    _phase = iphase_critical_point; break;
                }
                else if (_rhomolar > _crit.rhomolar){
                    _phase = iphase_supercritical_liquid; break;
                }
                else{
                    _phase = iphase_supercritical_gas; break;
                }
            default:
                throw ValueError(format("T=Tcrit; invalid input for other to T_phase_determination_pure_or_pseudopure"));
        }
    }
    else if (_T < _crit.T)
    {
        // Start to think about the saturation stuff
        // First try to use the ancillary equations if you are far enough away
        // You know how accurate the ancillary equations are thanks to using CoolProp code to refit them
        switch (other)
        {
            case iP:
            {
                _pLanc = components[0]->ancillaries.pL.evaluate(_T);
                _pVanc = components[0]->ancillaries.pV.evaluate(_T);
                long double p_vap = 0.98*static_cast<double>(_pVanc);
                long double p_liq = 1.02*static_cast<double>(_pLanc);

                if (value < p_vap){
                    this->_phase = iphase_gas; _Q = -1000; return;
                }
                else if (value > p_liq){
                    this->_phase = iphase_liquid; _Q = 1000; return;
                }
                break;
            }
            default:
            {
                // Always calculate the densities using the ancillaries
                _rhoVanc = components[0]->ancillaries.rhoV.evaluate(_T);
                _rhoLanc = components[0]->ancillaries.rhoL.evaluate(_T);
                long double rho_vap = 0.95*static_cast<double>(_rhoVanc);
                long double rho_liq = 1.05*static_cast<double>(_rhoLanc);
                switch (other)
                {
                    case iDmolar:
                    {
                        if (value < rho_vap){
                            this->_phase = iphase_gas; return;
                        }
                        else if (value > rho_liq){
                            this->_phase = iphase_liquid; return;
                        }
                        break;
                    }
                    default:
                    {
                        // If it is not density, update the states
                        SatV->update(DmolarT_INPUTS, rho_vap, _T);
                        SatL->update(DmolarT_INPUTS, rho_liq, _T);

                        // First we check ancillaries
                        switch (other)
                        {
                            case iSmolar:
                            {
                                if (value > SatV->calc_smolar()){
                                    this->_phase = iphase_gas; return;
                                }
                                if (value < SatL->calc_smolar()){
                                    this->_phase = iphase_liquid; return;
                                }
                                break;
                            }
                            case iHmolar:
                            {
                                if (value > SatV->calc_hmolar()){
                                    this->_phase = iphase_gas; return;
                                }
                                else if (value < SatL->calc_hmolar()){
                                    this->_phase = iphase_liquid; return;
                                }
                            }
                            case iUmolar:
                            {
                                if (value > SatV->calc_umolar()){
                                    this->_phase = iphase_gas; return;
                                }
                                else if (value < SatL->calc_umolar()){
                                    this->_phase = iphase_liquid; return;
                                }
                                break;
                            }
                            default:
                                throw ValueError(format("invalid input for other to T_phase_determination_pure_or_pseudopure"));
                        }
                    }
                }
            }
        }

        // Determine Q based on the input provided
        if (!is_pure_or_pseudopure){throw ValueError("possibly two-phase inputs not supported for pseudo-pure for now");}

        // Actually have to use saturation information sadly
        // For the given temperature, find the saturation state
        // Run the saturation routines to determine the saturation densities and pressures
        HelmholtzEOSMixtureBackend HEOS(components);
        SaturationSolvers::saturation_T_pure_options options;
        SaturationSolvers::saturation_T_pure(HEOS, _T, options);

        long double Q;

        if (other == iP)
        {
            if (value > HEOS.SatL->p()*(100*DBL_EPSILON + 1)){
                this->_phase = iphase_liquid; _Q = -1000; return;
            }
            else if (value < HEOS.SatV->p()*(1 - 100*DBL_EPSILON)){
                this->_phase = iphase_gas; _Q = 1000; return;
            }
            else{
                throw ValueError(format("subcrit T, funny p"));
            }
        }

        switch (other)
        {
            case iDmolar:
                Q = (1/value-1/HEOS.SatL->rhomolar())/(1/HEOS.SatV->rhomolar()-1/HEOS.SatL->rhomolar()); break;
            case iSmolar:
                Q = (value - HEOS.SatL->smolar())/(HEOS.SatV->smolar() - HEOS.SatL->smolar()); break;
            case iHmolar:
                Q = (value - HEOS.SatL->hmolar())/(HEOS.SatV->hmolar() - HEOS.SatL->hmolar()); break;
            case iUmolar:
                Q = (value - HEOS.SatL->umolar())/(HEOS.SatV->umolar() - HEOS.SatL->umolar()); break;
            default:
                throw ValueError(format("bad input for other"));
        }
        
        // Update the states
        this->SatL->update(DmolarT_INPUTS, HEOS.SatL->rhomolar(), HEOS.SatL->T());
        this->SatV->update(DmolarT_INPUTS, HEOS.SatV->rhomolar(), HEOS.SatV->T());

        if (Q < -100*DBL_EPSILON){
            this->_phase = iphase_liquid; _Q = -1000; return;
        }
        else if (Q > 1+100*DBL_EPSILON){
            this->_phase = iphase_gas; _Q = 1000; return;
        }
        else{
            this->_phase = iphase_twophase;
        }
        _Q = Q;
        // Load the outputs
        _p = _Q*HEOS.SatV->p() + (1-_Q)*HEOS.SatL->p();
        _rhomolar = 1/(_Q/HEOS.SatV->rhomolar() + (1-_Q)/HEOS.SatL->rhomolar());
        return;
    }
    else if (_T > _crit.T && _T > components[0]->pEOS->Ttriple)
    {
        _Q = 1e9;
        switch (other)
        {
            case iP:
            {
                if (_p > _crit.p){
                    this->_phase = iphase_supercritical; return;
                }
                else{
                    this->_phase = iphase_supercritical_gas; return;
                }
            }
            case iDmolar:
            {
                if (_rhomolar > _crit.rhomolar){
                    this->_phase = iphase_supercritical_liquid; return;
                }
                else{
                    this->_phase = iphase_supercritical_gas; return;
                }
            }
            case iSmolar:
            {
                if (_smolar.pt() > _crit.smolar){
                    this->_phase = iphase_supercritical_gas; return;
                }
                else{
                    this->_phase = iphase_supercritical_liquid; return;
                }
            }
            case iHmolar:
            {
                if (_hmolar.pt() > _crit.hmolar){
                    this->_phase = iphase_supercritical_gas; return;
                }
                else{
                    this->_phase = iphase_supercritical_liquid; return;
                }
            }
            case iUmolar:
            {
                if (_umolar.pt() > _crit.umolar){
                    this->_phase = iphase_supercritical_gas; return;
                }
                else{
                    this->_phase = iphase_supercritical_liquid; return;
                }
            }
            default:
            {
                throw ValueError("supercritical temp but other invalid for now");
            }
        }
    }
    else
    {
        throw ValueError(format("For now, we don't support T [%g K] below Ttriple [%g K]", _T, components[0]->pEOS->Ttriple));
    }
}
void get_dT_drho(HelmholtzEOSMixtureBackend *HEOS, parameters index, long double &dT, long double &drho)
{
    long double T = HEOS->T(),
                rho = HEOS->rhomolar(),
                rhor = HEOS->get_reducing_state().rhomolar,
                Tr = HEOS->get_reducing_state().T,
                dT_dtau = -pow(T, 2)/Tr,
                R = HEOS->gas_constant(),
                delta = rho/rhor,
                tau = Tr/T;
    
    switch (index)
    {
    case iT:
        dT = 1; drho = 0; break;
    case iDmolar:
        dT = 0; drho = 1; break;
    case iDmass:
        dT = 0; drho = HEOS->molar_mass(); break;
    case iP:
    {
        // dp/drho|T
        drho = R*T*(1+2*delta*HEOS->dalphar_dDelta()+pow(delta, 2)*HEOS->d2alphar_dDelta2());
        // dp/dT|rho
        dT = rho*R*(1+delta*HEOS->dalphar_dDelta() - tau*delta*HEOS->d2alphar_dDelta_dTau());
        break;
    }
    case iHmass:
    case iHmolar:
    {
        // dh/dT|rho
        dT = R*(-pow(tau,2)*(HEOS->d2alpha0_dTau2()+HEOS->d2alphar_dTau2()) + (1+delta*HEOS->dalphar_dDelta()-tau*delta*HEOS->d2alphar_dDelta_dTau()));
        // dh/drhomolar|T
        drho = T*R/rho*(tau*delta*HEOS->d2alphar_dDelta_dTau()+delta*HEOS->dalphar_dDelta()+pow(delta,2)*HEOS->d2alphar_dDelta2());
        if (index == iHmass){
            // dhmolar/drhomolar|T * dhmass/dhmolar where dhmass/dhmolar = 1/mole mass
            drho /= HEOS->molar_mass();
            dT /= HEOS->molar_mass();
        }
        break;
    }
    case iSmass:
    case iSmolar:
    {
        // ds/dT|rho
        dT = R/T*(-pow(tau,2)*(HEOS->d2alpha0_dTau2()+HEOS->d2alphar_dTau2()));
        // ds/drho|T
        drho = R/rho*(-(1+delta*HEOS->dalphar_dDelta()-tau*delta*HEOS->d2alphar_dDelta_dTau()));
        if (index == iSmass){
            // ds/drho|T / drhomass/drhomolar where drhomass/drhomolar = mole mass
            drho /= HEOS->molar_mass();
            dT /= HEOS->molar_mass();
        }
        break;
    }
    case iUmass:
    case iUmolar:
    {
        // du/dT|rho
        dT = R*(-pow(tau,2)*(HEOS->d2alpha0_dTau2()+HEOS->d2alphar_dTau2()));
        // du/drho|T
        drho = HEOS->T()*R/rho*(tau*delta*HEOS->d2alphar_dDelta_dTau());
        if (index == iUmass){
            // du/drho|T / drhomass/drhomolar where drhomass/drhomolar = mole mass
            drho /= HEOS->molar_mass();
            dT /= HEOS->molar_mass();
        }
        break;
    }
    case iTau:
        dT = 1/dT_dtau; drho = 0; break;
    case iDelta:
        dT = 0; drho = 1/rhor; break;
    default:
        throw ValueError(format("input to get_dT_drho[%s] is invalid",get_parameter_information(index,"short").c_str()));
    }
}
void get_dT_drho_second_derivatives(HelmholtzEOSMixtureBackend *HEOS, int index, long double &dT2, long double &drho_dT, long double &drho2)
{
    long double T = HEOS->T(),
                rho = HEOS->rhomolar(),
                rhor = HEOS->get_reducing_state().rhomolar,
                Tr = HEOS->get_reducing_state().T,
                R = HEOS->gas_constant(),
                delta = rho/rhor,
                tau = Tr/T;

    // Here we use T and rho as independent variables since derivations are already done by Thorade, 2013, 
    // Partial derivatives of thermodynamic state propertiesfor dynamic simulation, DOI 10.1007/s12665-013-2394-z
    
    HelmholtzDerivatives derivs = HEOS->get_components()[0]->pEOS->alphar.all(tau, delta);
    
    switch (index)
    {
    case iT:
        dT2 = 0;  // d2T_dT2
        drho_dT = 0; // d2T_drho_dT
        drho2 = 0; 
        break;
    case iDmolar:
        dT2 = 0; // d2rhomolar_dtau2
        drho2 = 0;
        drho_dT = 0;
        break;
    case iTau:
        dT2 = 2*Tr/pow(T,3); drho_dT = 0; drho2 = 0; break;
    case iDelta:
        dT2 = 0; drho_dT = 0; drho2 = 0; break;
    case iP:
    {
        drho2 = T*R/rho*(2*delta*derivs.dalphar_ddelta+4*pow(delta,2)*derivs.d2alphar_ddelta2+pow(delta,3)*derivs.d3alphar_ddelta3);
        dT2 = rho*R/T*(pow(tau,2)*delta*derivs.d3alphar_ddelta_dtau2);
        drho_dT = R*(1+2*delta*derivs.dalphar_ddelta +pow(delta,2)*derivs.d2alphar_ddelta2 - 2*delta*tau*derivs.d2alphar_ddelta_dtau - tau*pow(delta, 2)*derivs.d3alphar_ddelta2_dtau);
        break;
    }
    case iHmass:
    case iHmolar:
    {
        // d2h/drho2|T
        drho2 = R*T*pow(delta/rho,2)*(tau*derivs.d3alphar_ddelta2_dtau + 2*derivs.d2alphar_ddelta2 + delta*derivs.d3alphar_ddelta3);
        // d2h/dT2|rho
        dT2 = R/T*pow(tau, 2)*(tau*(HEOS->d3alpha0_dTau3()+derivs.d3alphar_dtau3) + 2*(HEOS->d2alpha0_dTau2()+derivs.d2alphar_dtau2) + delta*derivs.d3alphar_ddelta_dtau2);
        // d2h/drho/dT
        drho_dT = R/rho*delta*(delta*derivs.d2alphar_ddelta2 - pow(tau,2)*derivs.d3alphar_ddelta_dtau2 + derivs.dalphar_ddelta - tau*delta*derivs.d3alphar_ddelta2_dtau - tau*derivs.d2alphar_ddelta_dtau);
        if (index == iHmass){
            drho2 /= HEOS->molar_mass();
            drho_dT /= HEOS->molar_mass();
            dT2 /= HEOS->molar_mass();
        }
        break;
    }
    case iSmass:
    case iSmolar:
    {
        // d2s/rho2|T
        drho2 = R/pow(rho,2)*(1-pow(delta,2)*derivs.d2alphar_ddelta2 + tau*pow(delta,2)*derivs.d3alphar_ddelta2_dtau);
        // d2s/dT2|rho
        dT2 = R*pow(tau/T, 2)*(tau*(HEOS->d3alpha0_dTau3()+derivs.d3alphar_dtau3)+3*(HEOS->d2alpha0_dTau2()+derivs.d2alphar_dtau2));
        // d2s/drho/dT
        drho_dT = R/(T*rho)*(-pow(tau,2)*delta*derivs.d3alphar_ddelta_dtau2);
        if (index == iSmass){
            drho2 /= HEOS->molar_mass();
            drho_dT /= HEOS->molar_mass();
            dT2 /= HEOS->molar_mass();
        }
        break;
    }
    case iUmass:
    case iUmolar:
    {
        // d2u/rho2|T
        drho2 = R*T*tau*pow(delta/rho,2)*derivs.d3alphar_ddelta2_dtau;
        // d2u/dT2|rho
        dT2 = R/T*pow(tau, 2)*(tau*(HEOS->d3alpha0_dTau3()+derivs.d3alphar_dtau3)+2*(HEOS->d2alpha0_dTau2()+derivs.d2alphar_dtau2));
        // d2u/drho/dT
        drho_dT = R/rho*(-pow(tau,2)*delta*derivs.d3alphar_ddelta_dtau2);
        if (index == iUmass){
            drho2 /= HEOS->molar_mass();
            drho_dT /= HEOS->molar_mass();
            dT2 /= HEOS->molar_mass();
        }
        break;
    }
    default:
        throw ValueError(format("input to get_dT_drho_second_derivatives[%s] is invalid", get_parameter_information(index,"short").c_str()));
    }
}
long double HelmholtzEOSMixtureBackend::calc_first_partial_deriv(parameters Of, parameters Wrt, parameters Constant)
{
    long double dOf_dT, dOf_drho, dWrt_dT, dWrt_drho, dConstant_dT, dConstant_drho;

    get_dT_drho(this, Of, dOf_dT, dOf_drho);
    get_dT_drho(this, Wrt, dWrt_dT, dWrt_drho);
    get_dT_drho(this, Constant, dConstant_dT, dConstant_drho);

    return (dOf_dT*dConstant_drho-dOf_drho*dConstant_dT)/(dWrt_dT*dConstant_drho-dWrt_drho*dConstant_dT);
}

long double HelmholtzEOSMixtureBackend::calc_second_partial_deriv(parameters Of1, parameters Wrt1, parameters Constant1, parameters Wrt2, parameters Constant2)
{
    long double dOf1_dT, dOf1_drho, dWrt1_dT, dWrt1_drho, dConstant1_dT, dConstant1_drho, d2Of1_dT2, d2Of1_drhodT, 
                d2Of1_drho2, d2Wrt1_dT2, d2Wrt1_drhodT, d2Wrt1_drho2, d2Constant1_dT2, d2Constant1_drhodT, d2Constant1_drho2,
                dWrt2_dT, dWrt2_drho, dConstant2_dT, dConstant2_drho, N, D, dNdrho__T, dDdrho__T, dNdT__rho, dDdT__rho,
                dderiv1_drho, dderiv1_dT, second;

    // First and second partials needed for terms involved in first derivative
    get_dT_drho(this, Of1, dOf1_dT, dOf1_drho);
    get_dT_drho(this, Wrt1, dWrt1_dT, dWrt1_drho);
    get_dT_drho(this, Constant1, dConstant1_dT, dConstant1_drho);
    get_dT_drho_second_derivatives(this, Of1, d2Of1_dT2, d2Of1_drhodT, d2Of1_drho2);
    get_dT_drho_second_derivatives(this, Wrt1, d2Wrt1_dT2, d2Wrt1_drhodT, d2Wrt1_drho2);
    get_dT_drho_second_derivatives(this, Constant1, d2Constant1_dT2, d2Constant1_drhodT, d2Constant1_drho2);
    
    // First derivatives of terms involved in the second derivative
    get_dT_drho(this, Wrt2, dWrt2_dT, dWrt2_drho);
    get_dT_drho(this, Constant2, dConstant2_dT, dConstant2_drho);
    
    // Numerator and denominator of first partial derivative term
    N = dOf1_dT*dConstant1_drho - dOf1_drho*dConstant1_dT;
    D = dWrt1_dT*dConstant1_drho - dWrt1_drho*dConstant1_dT;
    
    // Derivatives of the numerator and denominator of the first partial derivative term with respect to rho, T held constant
    // They are of similar form, with Of1 and Wrt1 swapped
    dNdrho__T = dOf1_dT*d2Constant1_drho2 + d2Of1_drhodT*dConstant1_drho - dOf1_drho*d2Constant1_drhodT - d2Of1_drho2*dConstant1_dT;
    dDdrho__T = dWrt1_dT*d2Constant1_drho2 + d2Wrt1_drhodT*dConstant1_drho - dWrt1_drho*d2Constant1_drhodT - d2Wrt1_drho2*dConstant1_dT;
    
    // Derivatives of the numerator and denominator of the first partial derivative term with respect to T, rho held constant
    // They are of similar form, with Of1 and Wrt1 swapped
    dNdT__rho = dOf1_dT*d2Constant1_drhodT + d2Of1_dT2*dConstant1_drho - dOf1_drho*d2Constant1_dT2 - d2Of1_drhodT*dConstant1_dT;
    dDdT__rho = dWrt1_dT*d2Constant1_drhodT + d2Wrt1_dT2*dConstant1_drho - dWrt1_drho*d2Constant1_dT2 - d2Wrt1_drhodT*dConstant1_dT;
    
    // First partial of first derivative term with respect to T
    dderiv1_drho = (D*dNdrho__T - N*dDdrho__T)/pow(D, 2);
    
    // First partial of first derivative term with respect to rho
    dderiv1_dT = (D*dNdT__rho - N*dDdT__rho)/pow(D, 2);

    // Complete second derivative
    second = (dderiv1_dT*dConstant2_drho - dderiv1_drho*dConstant2_dT)/(dWrt2_dT*dConstant2_drho - dWrt2_drho*dConstant2_dT);
    
    return second;
}

long double HelmholtzEOSMixtureBackend::calc_pressure_nocache(long double T, long double rhomolar)
{
    SimpleState reducing = calc_reducing_state_nocache(mole_fractions);
    long double delta = rhomolar/reducing.rhomolar;
    long double tau = reducing.T/T;

    // Calculate derivative if needed
    int nTau = 0, nDelta = 1;
    long double dalphar_dDelta = calc_alphar_deriv_nocache(nTau, nDelta, mole_fractions, tau, delta);

    // Get pressure
    return rhomolar*gas_constant()*T*(1+delta*dalphar_dDelta);
}
long double HelmholtzEOSMixtureBackend::solver_for_rho_given_T_oneof_HSU(long double T, long double value, int other)
{
    long double ymelt, yc, ymin, y;

    // Define the residual to be driven to zero
    class solver_resid : public FuncWrapper1D
    {
    public:
        int other;
        long double T, value, r, eos, rhomolar;
        HelmholtzEOSMixtureBackend *HEOS;

        solver_resid(HelmholtzEOSMixtureBackend *HEOS, long double T, long double value, int other){
            this->HEOS = HEOS; this->T = T; this->value = value; this->other = other;
        };
        double call(double rhomolar){
            this->rhomolar = rhomolar;
            switch(other)
            {
            case iSmolar:
                eos = HEOS->calc_smolar_nocache(T,rhomolar); break;
            case iHmolar:
                eos = HEOS->calc_hmolar_nocache(T,rhomolar); break;
            case iUmolar:
                eos = HEOS->calc_umolar_nocache(T,rhomolar); break;
            default:
                throw ValueError(format("Input not supported"));
            }

            r = eos-value;
            return r;
        };
    };
    solver_resid resid(this, T, value, other);
    std::string errstring;

    // Supercritical temperature
    if (_T > _crit.T)
    {
        long double rhomelt = components[0]->triple_liquid.rhomolar;
        long double rhoc = components[0]->crit.rhomolar;
        long double rhomin = 1e-10;

        switch(other)
        {

            case iSmolar:
            {
                ymelt = calc_smolar_nocache(_T, rhomelt);
                yc = calc_smolar_nocache(_T, rhoc);
                ymin = calc_smolar_nocache(_T, rhomin);
                y = _smolar;
                break;
            }
            case iHmolar:
            {
                ymelt = calc_hmolar_nocache(_T, rhomelt);
                yc = calc_hmolar_nocache(_T, rhoc);
                ymin = calc_hmolar_nocache(_T, rhomin);
                y = _hmolar;
                break;
            }
            case iUmolar:
            {
                ymelt = calc_umolar_nocache(_T, rhomelt);
                yc = calc_umolar_nocache(_T, rhoc);
                ymin = calc_umolar_nocache(_T, rhomin);
                y = _umolar;
                break;
            }
            default:
                throw ValueError();
        }

        if (is_in_closed_range(ymelt, yc, y))
        {
            long double rhomolar = Brent(resid, rhomelt, rhoc, LDBL_EPSILON, 1e-12, 100, errstring);
            return rhomolar;
        }
        else if (is_in_closed_range(yc, ymin, y))
        {
            long double rhomolar = Brent(resid, rhoc, rhomin, LDBL_EPSILON, 1e-12, 100, errstring);
            return rhomolar;
        }
        else
        {
            throw ValueError();
        }
        // Update the state (T > Tc)
        if (_p < p_critical()){
            _phase = iphase_supercritical_gas;
        }
        else {
            _phase = iphase_supercritical;
        }
    }
    // Subcritical temperature liquid
    else if (_phase == iphase_liquid)
    {
        long double ymelt, yL, y;
        long double rhomelt = components[0]->triple_liquid.rhomolar;
        long double rhoL = static_cast<double>(_rhoLanc);

        switch(other)
        {
            case iSmolar:
            {
                ymelt = calc_smolar_nocache(_T, rhomelt);  yL = calc_smolar_nocache(_T, rhoL); y = _smolar; break;
            }
            case iHmolar:
            {
                ymelt = calc_hmolar_nocache(_T, rhomelt);  yL = calc_hmolar_nocache(_T, rhoL); y = _hmolar; break;
            }
            case iUmolar:
            {
                ymelt = calc_umolar_nocache(_T, rhomelt);  yL = calc_umolar_nocache(_T, rhoL); y = _umolar; break;
            }
            default:
                throw ValueError();
        }

        long double rhomolar_guess = (rhomelt-rhoL)/(ymelt-yL)*(y-yL) + rhoL;

        long double rhomolar = Secant(resid, rhomolar_guess, 0.0001*rhomolar_guess, 1e-12, 100, errstring);
        return rhomolar;
    }
    // Subcritical temperature gas
    else if (_phase == iphase_gas)
    {
        long double rhomin = 1e-14;
        long double rhoV = static_cast<double>(_rhoVanc);

        try
        {
            long double rhomolar = Brent(resid, rhomin, rhoV, LDBL_EPSILON, 1e-12, 100, errstring);
            return rhomolar;
        }
        catch(std::exception &)
        {
            throw ValueError();
        }
    }
    else{
        throw ValueError(format("phase to solver_for_rho_given_T_oneof_HSU is invalid"));
    }
}
long double HelmholtzEOSMixtureBackend::solver_rho_Tp(long double T, long double p, long double rhomolar_guess)
{
    phases phase;

    // Define the residual to be driven to zero
    class solver_TP_resid : public FuncWrapper1D
    {
    public:
        long double T, p, r, peos, rhomolar, rhor, tau, R_u, delta, dalphar_dDelta;
        HelmholtzEOSMixtureBackend *HEOS;

        solver_TP_resid(HelmholtzEOSMixtureBackend *HEOS, long double T, long double p){
            this->HEOS = HEOS; this->T = T; this->p = p; this->rhor = HEOS->get_reducing_state().rhomolar;
            this->tau = HEOS->get_reducing_state().T/T; this->R_u = HEOS->gas_constant();
        };
        double call(double rhomolar){
            this->rhomolar = rhomolar;
            delta = rhomolar/rhor; // needed for derivative
            HEOS->update_DmolarT_direct(rhomolar, T);
            peos = HEOS->p();
            r = (peos-p)/p;
            return r;
        };
        double deriv(double rhomolar){
            // dp/drho|T / pspecified
            return R_u*T*(1+2*delta*HEOS->dalphar_dDelta()+pow(delta, 2)*HEOS->d2alphar_dDelta2())/p;
        };
    };
    solver_TP_resid resid(this,T,p);
    std::string errstring;

    // Check if the phase is imposed
    if (imposed_phase_index != iphase_not_imposed)
        // Use the imposed phase index
        phase = imposed_phase_index;
    else
        // Use the phase index in the class
        phase = _phase;
        
    if (rhomolar_guess < 0) // Not provided
    {
        // Calculate a guess value using SRK equation of state
        rhomolar_guess = solver_rho_Tp_SRK(T, p, phase);

        // A gas-like phase, ideal gas might not be the perfect model, but probably good enough
        if (phase == iphase_gas || phase == iphase_supercritical_gas || phase == iphase_supercritical)
        {
            if (rhomolar_guess < 0 || !ValidNumber(rhomolar_guess)) // If the guess is bad, probably high temperature, use ideal gas
            {
                rhomolar_guess = p/(gas_constant()*T);
            }
        }
        // It's liquid at subcritical pressure, we can use ancillaries as a backup
        else if (phase == iphase_liquid)
        {
            long double _rhoLancval = static_cast<long double>(components[0]->ancillaries.rhoL.evaluate(T));
            // Next we try with a Brent method bounded solver since the function is 1-1
            double rhomolar = Brent(resid, _rhoLancval*0.9, _rhoLancval*1.3, DBL_EPSILON,1e-8,100,errstring);
            if (!ValidNumber(rhomolar)){throw ValueError();}
            return rhomolar;
        }
        else if (phase == iphase_supercritical_liquid){
            
            long double rhoLancval = static_cast<long double>(components[0]->ancillaries.rhoL.evaluate(T));
            
            // Next we try with a Brent method bounded solver since the function is 1-1
            double rhomolar = Brent(resid, rhoLancval*0.99, rhomolar_critical()*4, DBL_EPSILON,1e-8,100,errstring);
            if (!ValidNumber(rhomolar)){throw ValueError();}
            return rhomolar;
        }
    }

    try{
        
        // First we try with Newton's method with analytic derivative
        double rhomolar = Newton(resid, rhomolar_guess, 1e-8, 100, errstring);
        if (!ValidNumber(rhomolar)){
            throw ValueError();
        }
        if (phase == iphase_liquid && !is_pure_or_pseudopure && first_partial_deriv(iP, iDmolar, iT) < 0){
            
            // Try again with a larger density in order to end up at the right solution
            rhomolar = Newton(resid, rhomolar_guess*1.5, 1e-8, 100, errstring);
            return rhomolar;
        }
        return rhomolar;
    }
    catch(std::exception &)
    {
        try{
            // Next we try with Secant method shooting off from the guess value
            double rhomolar = Secant(resid, rhomolar_guess, 1.1*rhomolar_guess, 1e-8, 100, errstring);
            if (!ValidNumber(rhomolar)){throw ValueError();}
            return rhomolar;
            
        }
        catch(std::exception &)
        {
			try{
                // Next we try with a Brent method bounded solver since the function is 1-1
                double rhomolar = Brent(resid, 0.1*rhomolar_guess, 2*rhomolar_guess,DBL_EPSILON,1e-8,100,errstring);
                if (!ValidNumber(rhomolar)){throw ValueError();}
                return rhomolar;
			}
			catch(...){
				
				throw ValueError(format("solver_rho_Tp was unable to find a solution for T=%10Lg, p=%10Lg, with guess value %10Lg",T,p,rhomolar_guess));
			}
            return _HUGE;
        }
    }
}
long double HelmholtzEOSMixtureBackend::solver_rho_Tp_SRK(long double T, long double p, int phase)
{
    long double rhomolar, R_u = gas_constant(), a = 0, b = 0, k_ij = 0;

    for (std::size_t i = 0; i < components.size(); ++i)
    {
        long double Tci = components[i]->pEOS->reduce.T, pci = components[i]->pEOS->reduce.p, accentric_i = components[i]->pEOS->accentric;
        long double m_i = 0.480+1.574*accentric_i-0.176*pow(accentric_i, 2);
        long double b_i = 0.08664*R_u*Tci/pci;
        b += mole_fractions[i]*b_i;

        long double a_i = 0.42747*pow(R_u*Tci,2)/pci*pow(1+m_i*(1-sqrt(T/Tci)),2);

        for (std::size_t j = 0; j < components.size(); ++j)
        {
            long double Tcj = components[j]->pEOS->reduce.T, pcj = components[j]->pEOS->reduce.p, accentric_j = components[j]->pEOS->accentric;
            long double m_j = 0.480+1.574*accentric_j-0.176*pow(accentric_j, 2);

            long double a_j = 0.42747*pow(R_u*Tcj,2)/pcj*pow(1+m_j*(1-sqrt(T/Tcj)),2);

            if (i == j){
                k_ij = 0;
            }
            else{
                k_ij = 0;
            }

            a += mole_fractions[i]*mole_fractions[j]*sqrt(a_i*a_j)*(1-k_ij);
        }
    }

    long double A = a*p/pow(R_u*T,2);
    long double B = b*p/(R_u*T);

    //Solve the cubic for solutions for Z = p/(rho*R*T)
    double Z0, Z1, Z2; int Nsolns;
    solve_cubic(1, -1, A-B-B*B, -A*B, Nsolns, Z0, Z1, Z2);

    // Determine the guess value
    if (Nsolns == 1){
        rhomolar = p/(Z0*R_u*T);
    }
    else{
        long double rhomolar0 = p/(Z0*R_u*T);
        long double rhomolar1 = p/(Z1*R_u*T);
        long double rhomolar2 = p/(Z2*R_u*T);

        // Check if only one solution is positive, return the solution if that is the case
        if (rhomolar0  > 0 && rhomolar1 <= 0 && rhomolar2 <= 0){ return rhomolar0; }
        if (rhomolar0 <= 0 && rhomolar1 >  0 && rhomolar2 <= 0){ return rhomolar1; }
        if (rhomolar0 <= 0 && rhomolar1 <= 0 && rhomolar2  > 0){ return rhomolar2; }

        switch(phase)
        {
        case iphase_liquid:
            rhomolar = max3(rhomolar0, rhomolar1, rhomolar2); break;
        case iphase_gas:
        case iphase_supercritical_gas:
            rhomolar = min3(rhomolar0, rhomolar1, rhomolar2); break;
        default:
            throw ValueError("Bad phase to solver_rho_Tp_SRK");
        };
    }
    return rhomolar;
}

long double HelmholtzEOSMixtureBackend::calc_pressure(void)
{
    // Calculate the reducing parameters
    _delta = _rhomolar/_reducing.rhomolar;
    _tau = _reducing.T/_T;

    // Calculate derivative if needed
    long double dar_dDelta = dalphar_dDelta();
    long double R_u = gas_constant();

    // Get pressure
    _p = _rhomolar*R_u*_T*(1 + _delta.pt()*dar_dDelta);

    //std::cout << format("p: %13.12f %13.12f %10.9f %10.9f %10.9f %10.9f %g\n",_T,_rhomolar,_tau,_delta,mole_fractions[0],dar_dDelta,_p);
    //if (_p < 0){
    //    throw ValueError("Pressure is less than zero");
    //}

    return static_cast<long double>(_p);
}
long double HelmholtzEOSMixtureBackend::calc_hmolar_nocache(long double T, long double rhomolar)
{
    // Calculate the reducing parameters
    long double delta = rhomolar/_reducing.rhomolar;
    long double tau = _reducing.T/T;

    // Calculate derivatives if needed, or just use cached values
    // Calculate derivative if needed
    long double dar_dDelta = calc_alphar_deriv_nocache(0, 1, mole_fractions, tau, delta);
    long double dar_dTau = calc_alphar_deriv_nocache(1, 0, mole_fractions, tau, delta);
    long double da0_dTau = calc_alpha0_deriv_nocache(1, 0, mole_fractions, tau, delta, _reducing.T, _reducing.rhomolar);
    long double R_u = gas_constant();

    // Get molar enthalpy
    return R_u*T*(1 + tau*(da0_dTau+dar_dTau) + delta*dar_dDelta);
}
long double HelmholtzEOSMixtureBackend::calc_hmolar(void)
{
    if (isTwoPhase())
    {
        _hmolar = _Q*SatV->hmolar() + (1 - _Q)*SatL->hmolar();
        return static_cast<long double>(_hmolar);
    }
    else if (isHomogeneousPhase())
    {
            // Calculate the reducing parameters
        _delta = _rhomolar/_reducing.rhomolar;
        _tau = _reducing.T/_T;

        // Calculate derivatives if needed, or just use cached values
        long double da0_dTau = dalpha0_dTau();
        long double dar_dTau = dalphar_dTau();
        long double dar_dDelta = dalphar_dDelta();
        long double R_u = gas_constant();

        // Get molar enthalpy
        _hmolar = R_u*_T*(1 + _tau.pt()*(da0_dTau+dar_dTau) + _delta.pt()*dar_dDelta);

        return static_cast<long double>(_hmolar);
    }
    else{
        throw ValueError(format("phase is invalid in calc_hmolar"));
    }
}
long double HelmholtzEOSMixtureBackend::calc_smolar_nocache(long double T, long double rhomolar)
{
    // Calculate the reducing parameters
    long double delta = rhomolar/_reducing.rhomolar;
    long double tau = _reducing.T/T;

    // Calculate derivatives if needed, or just use cached values
    // Calculate derivative if needed
    long double dar_dTau = calc_alphar_deriv_nocache(1, 0, mole_fractions, tau, delta);
    long double ar = calc_alphar_deriv_nocache(0, 0, mole_fractions, tau, delta);
    long double da0_dTau = calc_alpha0_deriv_nocache(1, 0, mole_fractions, tau, delta, _reducing.T, _reducing.rhomolar);
    long double a0 = calc_alpha0_deriv_nocache(0, 0, mole_fractions, tau, delta, _reducing.T, _reducing.rhomolar);
    long double R_u = gas_constant();

    // Get molar entropy
    return R_u*(tau*(da0_dTau+dar_dTau) - a0 - ar);
}
long double HelmholtzEOSMixtureBackend::calc_smolar(void)
{
    if (isTwoPhase())
    {
        _smolar = _Q*SatV->smolar() + (1 - _Q)*SatL->smolar();
        return static_cast<long double>(_smolar);
    }
    else if (isHomogeneousPhase())
    {
        // Calculate the reducing parameters
        _delta = _rhomolar/_reducing.rhomolar;
        _tau = _reducing.T/_T;

        // Calculate derivatives if needed, or just use cached values
        long double da0_dTau = dalpha0_dTau();
        long double ar = alphar();
        long double a0 = alpha0();
        long double dar_dTau = dalphar_dTau();
        long double R_u = gas_constant();

        // Get molar entropy
        _smolar = R_u*(_tau.pt()*(da0_dTau+dar_dTau) - a0 - ar);

        return static_cast<long double>(_smolar);
    }
    else{
        throw ValueError(format("phase is invalid in calc_smolar"));
    }
}
long double HelmholtzEOSMixtureBackend::calc_umolar_nocache(long double T, long double rhomolar)
{
    // Calculate the reducing parameters
    long double delta = rhomolar/_reducing.rhomolar;
    long double tau = _reducing.T/T;

    // Calculate derivatives
    long double dar_dTau = calc_alphar_deriv_nocache(1, 0, mole_fractions, tau, delta);
    long double da0_dTau = calc_alpha0_deriv_nocache(1, 0, mole_fractions, tau, delta, _reducing.T, _reducing.rhomolar);
    long double R_u = gas_constant();

    // Get molar internal energy
    return R_u*T*tau*(da0_dTau+dar_dTau);
}
long double HelmholtzEOSMixtureBackend::calc_umolar(void)
{
	if (isTwoPhase())
    {
        _umolar = _Q*SatV->umolar() + (1 - _Q)*SatL->umolar();
        return static_cast<long double>(_umolar);
    }
    else if (isHomogeneousPhase())
    {
		// Calculate the reducing parameters
		_delta = _rhomolar/_reducing.rhomolar;
		_tau = _reducing.T/_T;

		// Calculate derivatives if needed, or just use cached values
		long double da0_dTau = dalpha0_dTau();
		long double dar_dTau = dalphar_dTau();
		long double R_u = gas_constant();

		// Get molar internal energy
		_umolar = R_u*_T*_tau.pt()*(da0_dTau+dar_dTau);

		return static_cast<long double>(_umolar);
	}
    else{
        throw ValueError(format("phase is invalid in calc_umolar"));
    }
}
long double HelmholtzEOSMixtureBackend::calc_cvmolar(void)
{
    // Calculate the reducing parameters
    _delta = _rhomolar/_reducing.rhomolar;
    _tau = _reducing.T/_T;

    // Calculate derivatives if needed, or just use cached values
    long double d2ar_dTau2 = d2alphar_dTau2();
    long double d2a0_dTau2 = d2alpha0_dTau2();
    long double R_u = gas_constant();

    // Get cv
    _cvmolar = -R_u*pow(_tau.pt(),2)*(d2ar_dTau2 + d2a0_dTau2);

    return static_cast<double>(_cvmolar);
}
long double HelmholtzEOSMixtureBackend::calc_cpmolar(void)
{
    // Calculate the reducing parameters
    _delta = _rhomolar/_reducing.rhomolar;
    _tau = _reducing.T/_T;

    // Calculate derivatives if needed, or just use cached values
    long double d2a0_dTau2 = d2alpha0_dTau2();
    long double dar_dDelta = dalphar_dDelta();
    long double d2ar_dDelta2 = d2alphar_dDelta2();
    long double d2ar_dDelta_dTau = d2alphar_dDelta_dTau();
    long double d2ar_dTau2 = d2alphar_dTau2();
    long double R_u = gas_constant();

    // Get cp
    _cpmolar = R_u*(-pow(_tau.pt(),2)*(d2ar_dTau2 + d2a0_dTau2)+pow(1+_delta.pt()*dar_dDelta-_delta.pt()*_tau.pt()*d2ar_dDelta_dTau,2)/(1+2*_delta.pt()*dar_dDelta+pow(_delta.pt(),2)*d2ar_dDelta2));

    return static_cast<double>(_cpmolar);
}
long double HelmholtzEOSMixtureBackend::calc_cpmolar_idealgas(void)
{
    // Calculate the reducing parameters
    _delta = _rhomolar/_reducing.rhomolar;
    _tau = _reducing.T/_T;

    // Calculate derivatives if needed, or just use cached values
    long double d2a0_dTau2 = d2alpha0_dTau2();
    long double R_u = gas_constant();

    // Get cp of the ideal gas
    return R_u*(1+(-pow(_tau.pt(),2))*d2a0_dTau2);
}
long double HelmholtzEOSMixtureBackend::calc_speed_sound(void)
{
    // Calculate the reducing parameters
    _delta = _rhomolar/_reducing.rhomolar;
    _tau = _reducing.T/_T;

    // Calculate derivatives if needed, or just use cached values
    long double d2a0_dTau2 = d2alpha0_dTau2();
    long double dar_dDelta = dalphar_dDelta();
    long double d2ar_dDelta2 = d2alphar_dDelta2();
    long double d2ar_dDelta_dTau = d2alphar_dDelta_dTau();
    long double d2ar_dTau2 = d2alphar_dTau2();
    long double R_u = gas_constant();
    long double mm = molar_mass();

    // Get speed of sound
    _speed_sound = sqrt(R_u*_T/mm*(1+2*_delta.pt()*dar_dDelta+pow(_delta.pt(),2)*d2ar_dDelta2 - pow(1+_delta.pt()*dar_dDelta-_delta.pt()*_tau.pt()*d2ar_dDelta_dTau,2)/(pow(_tau.pt(),2)*(d2ar_dTau2 + d2a0_dTau2))));

    return static_cast<double>(_speed_sound);
}
long double HelmholtzEOSMixtureBackend::calc_gibbsmolar(void)
{
    if (isTwoPhase())
    {
        _gibbsmolar = _Q*SatV->gibbsmolar() + (1 - _Q)*SatL->gibbsmolar();
        return static_cast<long double>(_gibbsmolar);
    }
    else if (isHomogeneousPhase())
    {
        // Calculate the reducing parameters
        _delta = _rhomolar/_reducing.rhomolar;
        _tau = _reducing.T/_T;

        // Calculate derivatives if needed, or just use cached values
        long double ar = alphar();
        long double a0 = alpha0();
        long double dar_dDelta = dalphar_dDelta();
        long double R_u = gas_constant();

        // Get molar gibbs function
        _gibbsmolar = R_u*_T*(1 + a0 + ar +_delta.pt()*dar_dDelta);

        return static_cast<long double>(_gibbsmolar);
    }
    else{
        throw ValueError(format("phase is invalid in calc_gibbsmolar"));
    }
}
long double HelmholtzEOSMixtureBackend::calc_fugacity_coefficient(int i)
{
    x_N_dependency_flag xN_flag = XN_DEPENDENT;
    return exp(MixtureDerivatives::ln_fugacity_coefficient(*this, i, xN_flag));
}
long double HelmholtzEOSMixtureBackend::calc_phase_identification_parameter(void)
{
    return 2 - rhomolar()*(second_partial_deriv(iP, iDmolar, iT, iT, iDmolar)/first_partial_deriv(iP, iT, iDmolar) -  second_partial_deriv(iP, iDmolar, iT, iDmolar, iT)/first_partial_deriv(iP, iDmolar, iT));
}

SimpleState HelmholtzEOSMixtureBackend::calc_reducing_state_nocache(const std::vector<long double> & mole_fractions)
{
    SimpleState reducing;
    if (is_pure_or_pseudopure){
        reducing = components[0]->pEOS->reduce;

    }
    else{
        reducing.T = Reducing->Tr(mole_fractions);
        reducing.rhomolar = Reducing->rhormolar(mole_fractions);
    }
    return reducing;
}
void HelmholtzEOSMixtureBackend::calc_reducing_state(void)
{
    /// \todo set critical independently
    _reducing = calc_reducing_state_nocache(mole_fractions);
    _crit = _reducing;
}
void HelmholtzEOSMixtureBackend::calc_all_alphar_deriv_cache(const std::vector<long double> &mole_fractions, const long double &tau, const long double &delta)
{
    deriv_counter++;
    //std::cout << ".";
    if (is_pure_or_pseudopure){
        HelmholtzDerivatives derivs = components[0]->pEOS->alphar.all(tau, delta);
        _alphar = derivs.alphar;
        _dalphar_dDelta = derivs.dalphar_ddelta;
        _dalphar_dTau = derivs.dalphar_dtau;
        _d2alphar_dDelta2 = derivs.d2alphar_ddelta2;
        _d2alphar_dDelta_dTau = derivs.d2alphar_ddelta_dtau;
        _d2alphar_dTau2 = derivs.d2alphar_dtau2;
        _d3alphar_dDelta3 = derivs.d3alphar_ddelta3;
        _d3alphar_dDelta2_dTau = derivs.d3alphar_ddelta2_dtau;
        _d3alphar_dDelta_dTau2 = derivs.d3alphar_ddelta_dtau2;
        _d3alphar_dTau3 = derivs.d3alphar_dtau3;
    }
    else{
        std::size_t N = mole_fractions.size();
        long double summer_base = 0, summer_dTau = 0, summer_dDelta = 0, 
                    summer_dTau2 = 0, summer_dDelta2 = 0, summer_dDelta_dTau = 0;
        for (std::size_t i = 0; i < N; ++i){
            HelmholtzDerivatives derivs = components[i]->pEOS->alphar.all(tau, delta);
            long double xi = mole_fractions[i];
            long double R_u_ratio = 1;
            if (get_config_bool(NORMALIZE_GAS_CONSTANTS)){
                R_u_ratio = components[0]->pEOS->R_u/R_u_CODATA;
            }
            summer_base += xi*derivs.alphar;
            summer_dDelta += xi*derivs.dalphar_ddelta;
            summer_dTau += xi*derivs.dalphar_dtau;
            summer_dDelta2 += xi*derivs.d2alphar_ddelta2;
            summer_dDelta_dTau += xi*derivs.d2alphar_ddelta_dtau;
            summer_dTau2 += xi*derivs.d2alphar_dtau2;
        }
        _alphar = summer_base + Excess.alphar(tau, delta, mole_fractions);
        _dalphar_dDelta = summer_dDelta + Excess.dalphar_dDelta(tau, delta, mole_fractions);
        _dalphar_dTau = summer_dTau + Excess.dalphar_dTau(tau, delta, mole_fractions);
        _d2alphar_dDelta2 = summer_dDelta2 + Excess.d2alphar_dDelta2(tau, delta, mole_fractions);
        _d2alphar_dDelta_dTau = summer_dDelta_dTau + Excess.d2alphar_dDelta_dTau(tau, delta, mole_fractions);
        _d2alphar_dTau2 = summer_dTau2 + Excess.d2alphar_dTau2(tau, delta, mole_fractions);
    }
}

long double HelmholtzEOSMixtureBackend::calc_alphar_deriv_nocache(const int nTau, const int nDelta, const std::vector<long double> &mole_fractions, const long double &tau, const long double &delta)
{
    if (is_pure_or_pseudopure)
    {
        if (nTau == 0 && nDelta == 0){
            return components[0]->pEOS->baser(tau, delta);
        }
        else if (nTau == 0 && nDelta == 1){
            return components[0]->pEOS->dalphar_dDelta(tau, delta);
        }
        else if (nTau == 1 && nDelta == 0){
            return components[0]->pEOS->dalphar_dTau(tau, delta);
        }
        else if (nTau == 0 && nDelta == 2){
            return components[0]->pEOS->d2alphar_dDelta2(tau, delta);
        }
        else if (nTau == 1 && nDelta == 1){
            return components[0]->pEOS->d2alphar_dDelta_dTau(tau, delta);
        }
        else if (nTau == 2 && nDelta == 0){
            return components[0]->pEOS->d2alphar_dTau2(tau, delta);
        }
        else if (nTau == 0 && nDelta == 3){
            return components[0]->pEOS->d3alphar_dDelta3(tau, delta);
        }
        else if (nTau == 1 && nDelta == 2){
            return components[0]->pEOS->d3alphar_dDelta2_dTau(tau, delta);
        }
        else if (nTau == 2 && nDelta == 1){
            return components[0]->pEOS->d3alphar_dDelta_dTau2(tau, delta);
        }
        else if (nTau == 3 && nDelta == 0){
            return components[0]->pEOS->d3alphar_dTau3(tau, delta);
        }
        else
        {
            throw ValueError();
        }
    }
    else{

        std::size_t N = mole_fractions.size();
        long double summer = 0;
        if (nTau == 0 && nDelta == 0){
            for (unsigned int i = 0; i < N; ++i){ summer += mole_fractions[i]*components[i]->pEOS->baser(tau, delta); }
            return summer + Excess.alphar(tau, delta, mole_fractions);
        }
        else if (nTau == 0 && nDelta == 1){
            for (unsigned int i = 0; i < N; ++i){ summer += mole_fractions[i]*components[i]->pEOS->dalphar_dDelta(tau, delta); }
            return summer + Excess.dalphar_dDelta(tau, delta, mole_fractions);
        }
        else if (nTau == 1 && nDelta == 0){
            for (unsigned int i = 0; i < N; ++i){ summer += mole_fractions[i]*components[i]->pEOS->dalphar_dTau(tau, delta); }
            return summer + Excess.dalphar_dTau(tau, delta, mole_fractions);
        }
        else if (nTau == 0 && nDelta == 2){
            for (unsigned int i = 0; i < N; ++i){ summer += mole_fractions[i]*components[i]->pEOS->d2alphar_dDelta2(tau, delta); }
            return summer + Excess.d2alphar_dDelta2(tau, delta, mole_fractions);
        }
        else if (nTau == 1 && nDelta == 1){
            for (unsigned int i = 0; i < N; ++i){ summer += mole_fractions[i]*components[i]->pEOS->d2alphar_dDelta_dTau(tau, delta); }
            return summer + Excess.d2alphar_dDelta_dTau(tau, delta, mole_fractions);
        }
        else if (nTau == 2 && nDelta == 0){
            for (unsigned int i = 0; i < N; ++i){ summer += mole_fractions[i]*components[i]->pEOS->d2alphar_dTau2(tau, delta); }
            return summer + Excess.d2alphar_dTau2(tau, delta, mole_fractions);
        }
        /*else if (nTau == 0 && nDelta == 3){
            for (unsigned int i = 0; i < N; ++i){ summer += mole_fractions[i]*components[i]->pEOS->d3alphar_dDelta3(tau, delta); }
            return summer + pExcess.d3alphar_dDelta3(tau, delta);
        }
        else if (nTau == 1 && nDelta == 2){
            for (unsigned int i = 0; i < N; ++i){ summer += mole_fractions[i]*components[i]->pEOS->d3alphar_dDelta2_dTau(tau, delta); }
            return summer + pExcess.d3alphar_dDelta2_dTau(tau, delta);
        }
        else if (nTau == 2 && nDelta == 1){
            for (unsigned int i = 0; i < N; ++i){ summer += mole_fractions[i]*components[i]->pEOS->d3alphar_dDelta_dTau2(tau, delta); }
            return summer + pExcess.d3alphar_dDelta_dTau2(tau, delta);
        }
        else if (nTau == 3 && nDelta == 0){
            for (unsigned int i = 0; i < N; ++i){ summer += mole_fractions[i]*components[i]->pEOS->d3alphar_dTau3(tau, delta); }
            return summer + pExcess.d3alphar_dTau3(tau, delta);
        }*/
        else
        {
            throw ValueError();
        }
    }
}
long double HelmholtzEOSMixtureBackend::calc_alpha0_deriv_nocache(const int nTau, const int nDelta, const std::vector<long double> &mole_fractions,
                                                                  const long double &tau, const long double &delta, const long double &Tr, const long double &rhor)
{
    long double val;
    if (is_pure_or_pseudopure)
    {
        if (nTau == 0 && nDelta == 0){
            val = components[0]->pEOS->base0(tau, delta);
        }
        else if (nTau == 0 && nDelta == 1){
            val = components[0]->pEOS->dalpha0_dDelta(tau, delta);
        }
        else if (nTau == 1 && nDelta == 0){
            val = components[0]->pEOS->dalpha0_dTau(tau, delta);
        }
        else if (nTau == 0 && nDelta == 2){
            val = components[0]->pEOS->d2alpha0_dDelta2(tau, delta);
        }
        else if (nTau == 1 && nDelta == 1){
            val = components[0]->pEOS->d2alpha0_dDelta_dTau(tau, delta);
        }
        else if (nTau == 2 && nDelta == 0){
            val = components[0]->pEOS->d2alpha0_dTau2(tau, delta);
        }
        else if (nTau == 0 && nDelta == 3){
            val = components[0]->pEOS->d3alpha0_dDelta3(tau, delta);
        }
        else if (nTau == 1 && nDelta == 2){
            val = components[0]->pEOS->d3alpha0_dDelta2_dTau(tau, delta);
        }
        else if (nTau == 2 && nDelta == 1){
            val = components[0]->pEOS->d3alpha0_dDelta_dTau2(tau, delta);
        }
        else if (nTau == 3 && nDelta == 0){
            val = components[0]->pEOS->d3alpha0_dTau3(tau, delta);
        }
        else
        {
            throw ValueError();
        }
        if (!ValidNumber(val)){
           //calc_alpha0_deriv_nocache(nTau,nDelta,mole_fractions,tau,delta,Tr,rhor);
           throw ValueError(format("calc_alpha0_deriv_nocache returned invalid number with inputs nTau: %d, nDelta: %d, tau: %Ld, delta: %Ld", nTau, nDelta, tau, delta));
        }
        else{
            return val;
        }
    }
    else{
        // See Table B5, GERG 2008 from Kunz Wagner, JCED, 2012
        std::size_t N = mole_fractions.size();
        long double summer = 0;
        long double tau_i, delta_i, rho_ci, T_ci;
        for (unsigned int i = 0; i < N; ++i){
            rho_ci = components[i]->pEOS->reduce.rhomolar;
            T_ci = components[i]->pEOS->reduce.T;
            tau_i = T_ci*tau/Tr;
            delta_i = delta*rhor/rho_ci;

            if (nTau == 0 && nDelta == 0){
                summer += mole_fractions[i]*(components[i]->pEOS->base0(tau_i, delta_i)+log(mole_fractions[i]));
            }
            else if (nTau == 0 && nDelta == 1){
                summer += mole_fractions[i]*rhor/rho_ci*components[i]->pEOS->dalpha0_dDelta(tau_i, delta_i);
            }
            else if (nTau == 1 && nDelta == 0){
                summer += mole_fractions[i]*T_ci/Tr*components[i]->pEOS->dalpha0_dTau(tau_i, delta_i);
            }
            else if (nTau == 0 && nDelta == 2){
                summer += mole_fractions[i]*pow(rhor/rho_ci,2)*components[i]->pEOS->d2alpha0_dDelta2(tau_i, delta_i);
            }
            else if (nTau == 1 && nDelta == 1){
                summer += mole_fractions[i]*rhor/rho_ci*T_ci/Tr*components[i]->pEOS->d2alpha0_dDelta_dTau(tau_i, delta_i);
            }
            else if (nTau == 2 && nDelta == 0){
                summer += mole_fractions[i]*pow(T_ci/Tr,2)*components[i]->pEOS->d2alpha0_dTau2(tau_i, delta_i);
            }
            else
            {
                throw ValueError();
            }
        }
        return summer;
    }
}
long double HelmholtzEOSMixtureBackend::calc_alphar(void)
{
    calc_all_alphar_deriv_cache(mole_fractions, _tau, _delta);
    return static_cast<long double>(_alphar);
}
long double HelmholtzEOSMixtureBackend::calc_dalphar_dDelta(void)
{
    calc_all_alphar_deriv_cache(mole_fractions, _tau, _delta);
    return static_cast<long double>(_dalphar_dDelta);
}
long double HelmholtzEOSMixtureBackend::calc_dalphar_dTau(void)
{
    calc_all_alphar_deriv_cache(mole_fractions, _tau, _delta);
    return static_cast<long double>(_dalphar_dTau);
}
long double HelmholtzEOSMixtureBackend::calc_d2alphar_dTau2(void)
{
    calc_all_alphar_deriv_cache(mole_fractions, _tau, _delta);
    return static_cast<long double>(_d2alphar_dTau2);
}
long double HelmholtzEOSMixtureBackend::calc_d2alphar_dDelta_dTau(void)
{
    calc_all_alphar_deriv_cache(mole_fractions, _tau, _delta);
    return static_cast<long double>(_d2alphar_dDelta_dTau);
}
long double HelmholtzEOSMixtureBackend::calc_d2alphar_dDelta2(void)
{
    calc_all_alphar_deriv_cache(mole_fractions, _tau, _delta);
    return static_cast<long double>(_d2alphar_dDelta2);
}
long double HelmholtzEOSMixtureBackend::calc_alpha0(void)
{
    const int nTau = 0, nDelta = 0;
    return calc_alpha0_deriv_nocache(nTau, nDelta, mole_fractions, _tau, _delta, _reducing.T, _reducing.rhomolar);
}
long double HelmholtzEOSMixtureBackend::calc_dalpha0_dDelta(void)
{
    const int nTau = 0, nDelta = 1;
    return calc_alpha0_deriv_nocache(nTau, nDelta, mole_fractions, _tau, _delta, _reducing.T, _reducing.rhomolar);
}
long double HelmholtzEOSMixtureBackend::calc_dalpha0_dTau(void)
{
    const int nTau = 1, nDelta = 0;
    return calc_alpha0_deriv_nocache(nTau, nDelta, mole_fractions, _tau, _delta, _reducing.T, _reducing.rhomolar);
}
long double HelmholtzEOSMixtureBackend::calc_d2alpha0_dDelta2(void)
{
    const int nTau = 0, nDelta = 2;
    return calc_alpha0_deriv_nocache(nTau, nDelta, mole_fractions, _tau, _delta, _reducing.T, _reducing.rhomolar);
}
long double HelmholtzEOSMixtureBackend::calc_d2alpha0_dDelta_dTau(void)
{
    const int nTau = 1, nDelta = 1;
    return calc_alpha0_deriv_nocache(nTau, nDelta, mole_fractions, _tau, _delta, _reducing.T, _reducing.rhomolar);
}
long double HelmholtzEOSMixtureBackend::calc_d2alpha0_dTau2(void)
{
    const int nTau = 2, nDelta = 0;
    return calc_alpha0_deriv_nocache(nTau, nDelta, mole_fractions, _tau, _delta, _reducing.T, _reducing.rhomolar);
}
long double HelmholtzEOSMixtureBackend::calc_d3alpha0_dDelta3(void)
{
    const int nTau = 0, nDelta = 3;
    return calc_alpha0_deriv_nocache(nTau, nDelta, mole_fractions, _tau, _delta, _reducing.T, _reducing.rhomolar);
}
long double HelmholtzEOSMixtureBackend::calc_d3alpha0_dDelta2_dTau(void)
{
    const int nTau = 1, nDelta = 2;
    return calc_alpha0_deriv_nocache(nTau, nDelta, mole_fractions, _tau, _delta, _reducing.T, _reducing.rhomolar);
}
long double HelmholtzEOSMixtureBackend::calc_d3alpha0_dDelta_dTau2(void)
{
    const int nTau = 2, nDelta = 1;
    return calc_alpha0_deriv_nocache(nTau, nDelta, mole_fractions, _tau, _delta, _reducing.T, _reducing.rhomolar);
}
long double HelmholtzEOSMixtureBackend::calc_d3alpha0_dTau3(void)
{
    const int nTau = 3, nDelta = 0;
    return calc_alpha0_deriv_nocache(nTau, nDelta, mole_fractions, _tau, _delta, _reducing.T, _reducing.rhomolar);
}


} /* namespace CoolProp */
