
#include "IncompressibleFluid.h"
#include "math.h"
#include "MatrixMath.h"
#include "PolyMath.h"
#include "DataStructures.h"

namespace CoolProp {



/// A thermophysical property provider for critical and reducing values as well as derivatives of Helmholtz energy
/**
This fluid instance is populated using an entry from a JSON file
*/
//IncompressibleFluid::IncompressibleFluid();

void IncompressibleFluid::set_reference_state(double T0, double p0, double x0, double h0, double s0){
	this->rhoref = rho(T0,p0,x0);
	this->pref = p0;
	this->uref = h0 - p0/rhoref;
	this->uref = u(T0,p0,x0);
	this->href = h0; // set new reference value
	this->sref = s0; // set new reference value
	this->href = h(T0,p0,x0); // adjust offset to fit to equations
	this->sref = s(T0,p0,x0); // adjust offset to fit to equations
}

void IncompressibleFluid::validate(){
	return;
	// TODO: Implement validation function

	// u and s have to be of the polynomial type!
	throw NotImplementedError("TODO");
}

bool IncompressibleFluid::is_pure() {
	if (density.coeffs.cols()==1) return true;
	return false;
}

/// Base functions that handle the custom data type, just a place holder to show the structure.
double IncompressibleFluid::baseExponential(IncompressibleData data, double y, double ybase){
	Eigen::VectorXd coeffs = makeVector(data.coeffs);
	size_t r=coeffs.rows(),c=coeffs.cols();
	if (strict && (r!=3 || c!=1) ) throw ValueError(format("%s (%d): You have to provide a 3,1 matrix of coefficients, not  (%d,%d).",__FILE__,__LINE__,r,c));
	return exp( (double) (coeffs[0] / ( (y-ybase)+coeffs[1] ) - coeffs[2] ) );
}
double IncompressibleFluid::baseExponentialOffset(IncompressibleData data, double y){
	Eigen::VectorXd coeffs = makeVector(data.coeffs);
	size_t r=coeffs.rows(),c=coeffs.cols();
	if (strict && (r!=4 || c!=1) ) throw ValueError(format("%s (%d): You have to provide a 4,1 matrix of coefficients, not  (%d,%d).",__FILE__,__LINE__,r,c));
	return exp( (double) (coeffs[1] / ( (y-coeffs[0])+coeffs[2] ) - coeffs[3] ) );
}
double IncompressibleFluid::basePolyOffset(IncompressibleData data, double y, double z){
	size_t r=data.coeffs.rows(),c=data.coeffs.cols();
	double offset = 0.0;
	double in     = 0.0;
	Eigen::MatrixXd coeffs;
	if (r>0 && c>0) {
		offset = data.coeffs(0,0);
		if (r==1 && c>1) { // row vector -> function of z
			coeffs = Eigen::MatrixXd(data.coeffs.block(0,1,r,c-1));
			in = z;
		} else if (r>1 && c==1) { // column vector -> function of y
			coeffs = Eigen::MatrixXd(data.coeffs.block(1,0,r-1,c));
			in = y;
		} else {
			throw ValueError(format("%s (%d): You have to provide a vector (1D matrix) of coefficients, not  (%d,%d).",__FILE__,__LINE__,r,c));
		}
		return poly.evaluate(coeffs, in, 0, offset);
	}
	throw ValueError(format("%s (%d): You have to provide a vector (1D matrix) of coefficients, not  (%d,%d).",__FILE__,__LINE__,r,c));
	return _HUGE;
}

/// Density as a function of temperature, pressure and composition.
double IncompressibleFluid::rho (double T, double p, double x){
	switch (density.type) {
		case IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL:
			return poly.evaluate(density.coeffs, T, x, 0, 0, Tbase, xbase);
			break;
		case IncompressibleData::INCOMPRESSIBLE_EXPONENTIAL:
			return baseExponential(density, T, Tbase);
			break;
		case IncompressibleData::INCOMPRESSIBLE_EXPPOLYNOMIAL:
			return exp(poly.evaluate(density.coeffs, T, x, 0, 0, Tbase, xbase));
			break;
		case IncompressibleData::INCOMPRESSIBLE_EXPOFFSET:
			return baseExponentialOffset(density, T);
			break;
		case IncompressibleData::INCOMPRESSIBLE_POLYOFFSET:
			return basePolyOffset(density, T, x);
			break;
		case IncompressibleData::INCOMPRESSIBLE_NOT_SET:
			throw ValueError(format("%s (%d): The function type is not specified (\"[%d]\"), are you sure the coefficients have been set?",__FILE__,__LINE__,density.type));
			break;
		default:
			throw ValueError(format("%s (%d): Your function type \"[%d]\" is unknown.",__FILE__,__LINE__,density.type));
			break;
	}
	return _HUGE;
}

/// Heat capacities as a function of temperature, pressure and composition.
double IncompressibleFluid::c   (double T, double p, double x){
	switch (specific_heat.type) {
		case IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL:
			//throw NotImplementedError("Here you should implement the polynomial.");
			return poly.evaluate(specific_heat.coeffs, T, x, 0, 0, Tbase, xbase);
			break;
		case IncompressibleData::INCOMPRESSIBLE_NOT_SET:
			throw ValueError(format("%s (%d): The function type is not specified (\"[%d]\"), are you sure the coefficients have been set?",__FILE__,__LINE__,specific_heat.type));
			break;
		default:
			throw ValueError(format("%s (%d): There is no predefined way to use this function type \"[%d]\" for specific heat.",__FILE__,__LINE__,specific_heat.type));
			break;
	}
	return _HUGE;
}

/// Entropy as a function of temperature, pressure and composition.
double IncompressibleFluid::s   (double T, double p, double x){
	switch (specific_heat.type) {
		case IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL:
			//throw NotImplementedError("Here you should implement the polynomial.");
			return poly.integral(specific_heat.coeffs, T, x, 0, -1, 0, Tbase, xbase) - sref;
			break;
		case IncompressibleData::INCOMPRESSIBLE_NOT_SET:
			throw ValueError(format("%s (%d): The function type is not specified (\"[%d]\"), are you sure the coefficients have been set?",__FILE__,__LINE__,specific_heat.type));
			break;
		default:
			throw ValueError(format("%s (%d): There is no predefined way to use this function type \"[%d]\" for entropy.",__FILE__,__LINE__,specific_heat.type));
			break;
	}
	return _HUGE;
}

/// Internal energy as a function of temperature, pressure and composition.
double IncompressibleFluid::u   (double T, double p, double x){
	switch (specific_heat.type) {
		case IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL:
			//throw NotImplementedError("Here you should implement the polynomial.");
			return poly.integral(specific_heat.coeffs, T, x, 0, 0, 0, Tbase, xbase) - uref;
			break;
		case IncompressibleData::INCOMPRESSIBLE_NOT_SET:
			throw ValueError(format("%s (%d): The function type is not specified (\"[%d]\"), are you sure the coefficients have been set?",__FILE__,__LINE__,specific_heat.type));
			break;
		default:
			throw ValueError(format("%s (%d): There is no predefined way to use this function type \"[%d]\" for internal energy.",__FILE__,__LINE__,specific_heat.type));
			break;
	}
	return _HUGE;
}

/// Enthalpy as a function of temperature, pressure and composition.
double IncompressibleFluid::h   (double T, double p, double x){return h_u(T,p,x);};

/// Viscosity as a function of temperature, pressure and composition.
double IncompressibleFluid::visc(double T, double p, double x){
	switch (viscosity.type) {
		case IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL:
			return poly.evaluate(viscosity.coeffs, T, x, 0, 0, Tbase, xbase);
			break;
		case IncompressibleData::INCOMPRESSIBLE_EXPONENTIAL:
			return baseExponential(viscosity, T, Tbase);
			break;
		case IncompressibleData::INCOMPRESSIBLE_EXPPOLYNOMIAL:
			return exp(poly.evaluate(viscosity.coeffs, T, x, 0, 0, Tbase, xbase));
			break;
		case IncompressibleData::INCOMPRESSIBLE_EXPOFFSET:
			return baseExponentialOffset(viscosity, T);
			break;
		case IncompressibleData::INCOMPRESSIBLE_POLYOFFSET:
			return basePolyOffset(viscosity, T, x);
			break;
		case IncompressibleData::INCOMPRESSIBLE_NOT_SET:
			throw ValueError(format("%s (%d): The function type is not specified (\"[%d]\"), are you sure the coefficients have been set?",__FILE__,__LINE__,viscosity.type));
			break;
		default:
			throw ValueError(format("%s (%d): Your function type \"[%d]\" is unknown.",__FILE__,__LINE__,viscosity.type));
			break;
	}
	return _HUGE;
}
/// Thermal conductivity as a function of temperature, pressure and composition.
double IncompressibleFluid::cond(double T, double p, double x){
	switch (conductivity.type) {
		case IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL:
			return poly.evaluate(conductivity.coeffs, T, x, 0, 0, Tbase, xbase);
			break;
		case IncompressibleData::INCOMPRESSIBLE_EXPONENTIAL:
			return baseExponential(conductivity, T, Tbase);
			break;
		case IncompressibleData::INCOMPRESSIBLE_EXPPOLYNOMIAL:
			return exp(poly.evaluate(conductivity.coeffs, T, x, 0, 0, Tbase, xbase));
			break;
		case IncompressibleData::INCOMPRESSIBLE_EXPOFFSET:
			return baseExponentialOffset(conductivity, T);
			break;
		case IncompressibleData::INCOMPRESSIBLE_POLYOFFSET:
			return basePolyOffset(conductivity, T, x);
			break;
		case IncompressibleData::INCOMPRESSIBLE_NOT_SET:
			throw ValueError(format("%s (%d): The function type is not specified (\"[%d]\"), are you sure the coefficients have been set?",__FILE__,__LINE__,conductivity.type));
			break;
		default:
			throw ValueError(format("%s (%d): Your function type \"[%d]\" is unknown.",__FILE__,__LINE__,conductivity.type));
			break;
	}
	return _HUGE;
}
/// Saturation pressure as a function of temperature and composition.
double IncompressibleFluid::psat(double T,           double x){
	if (T<=this->TminPsat) return 0.0;
	switch (p_sat.type) {
		case IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL:
			return poly.evaluate(p_sat.coeffs, T, x, 0, 0, Tbase, xbase);
			break;
		case IncompressibleData::INCOMPRESSIBLE_EXPONENTIAL:
			return baseExponential(p_sat, T, Tbase);
			break;
		case IncompressibleData::INCOMPRESSIBLE_EXPPOLYNOMIAL:
			return exp(poly.evaluate(p_sat.coeffs, T, x, 0, 0, Tbase, xbase));
			break;
		case IncompressibleData::INCOMPRESSIBLE_EXPOFFSET:
			return baseExponentialOffset(p_sat, T);
			break;
		case IncompressibleData::INCOMPRESSIBLE_POLYOFFSET:
			return basePolyOffset(p_sat, T, x);
			break;
		case IncompressibleData::INCOMPRESSIBLE_NOT_SET:
			throw ValueError(format("%s (%d): The function type is not specified (\"[%d]\"), are you sure the coefficients have been set?",__FILE__,__LINE__,p_sat.type));
			break;
		default:
			throw ValueError(format("%s (%d): Your function type \"[%d]\" is unknown.",__FILE__,__LINE__,p_sat.type));
			break;
	}
	return _HUGE;
}
/// Freezing temperature as a function of pressure and composition.
double IncompressibleFluid::Tfreeze(       double p, double x){
	switch (T_freeze.type) {
		case IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL:
			return poly.evaluate(T_freeze.coeffs, x, p, 0, 0, xbase, 0.0);
			break;
		case IncompressibleData::INCOMPRESSIBLE_EXPONENTIAL:
			return baseExponential(T_freeze, x, xbase);
			break;
		case IncompressibleData::INCOMPRESSIBLE_EXPPOLYNOMIAL:
			return exp(poly.evaluate(T_freeze.coeffs, x, p, 0, 0, xbase, 0.0));
			break;
		case IncompressibleData::INCOMPRESSIBLE_EXPOFFSET:
			return baseExponentialOffset(T_freeze, x);
			break;
		case IncompressibleData::INCOMPRESSIBLE_POLYOFFSET:
			return basePolyOffset(T_freeze, x, p);
			break;
		case IncompressibleData::INCOMPRESSIBLE_NOT_SET:
			throw ValueError(format("%s (%d): The function type is not specified (\"[%d]\"), are you sure the coefficients have been set?",__FILE__,__LINE__,T_freeze.type));
			break;
		default:
			throw ValueError(format("%s (%d): Your function type \"[%d]\" is unknown.",__FILE__,__LINE__,T_freeze.type));
			break;
	}
	return _HUGE;
}

/// Mass fraction conversion function
/** If the fluid type is mass-based, it does not do anything. Otherwise,
 *  it converts the mass fraction to the required input. */
double IncompressibleFluid::inputFromMass (double T,     double x){
	if (this->xid==ifrac_pure) {
			return _HUGE;
	} else if (this->xid==ifrac_mass) {
		return x;
	} else {
		throw NotImplementedError("Mass composition conversion has not been implemented.");
		switch (mass2input.type) {
			case IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL:
				return poly.evaluate(mass2input.coeffs, T, x, 0, 0, 0.0, 0.0); // TODO: make sure Tbase and xbase is defined in the correct way
				break;
			case IncompressibleData::INCOMPRESSIBLE_EXPONENTIAL:
				return baseExponential(mass2input, T, Tbase);
				break;
			case IncompressibleData::INCOMPRESSIBLE_EXPPOLYNOMIAL:
				return exp(poly.evaluate(mass2input.coeffs, T, x, 0, 0, 0.0, 0.0)); // TODO: make sure Tbase and xbase is defined in the correct way
				break;
			case IncompressibleData::INCOMPRESSIBLE_EXPOFFSET:
				return baseExponentialOffset(mass2input, T);
				break;
			case IncompressibleData::INCOMPRESSIBLE_POLYOFFSET:
				return basePolyOffset(mass2input, T, x);
				break;
			case IncompressibleData::INCOMPRESSIBLE_NOT_SET:
				throw ValueError(format("%s (%d): The function type is not specified (\"[%d]\"), are you sure the coefficients have been set?",__FILE__,__LINE__,mass2input.type));
				break;
			default:
				throw ValueError(format("%s (%d): Your function type \"[%d]\" is unknown.",__FILE__,__LINE__,mass2input.type));
				break;
		}
		return _HUGE;
	}
}

/// Volume fraction conversion function
/** If the fluid type is volume-based, it does not do anything. Otherwise,
 *  it converts the volume fraction to the required input. */
double IncompressibleFluid::inputFromVolume (double T,   double x){
	if (this->xid==ifrac_pure) {
			return _HUGE;
	} else if (this->xid==ifrac_volume) {
		return x;
	} else {
		throw NotImplementedError("Volume composition conversion has not been implemented.");
		switch (volume2input.type) {
			case IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL:
				return poly.evaluate(volume2input.coeffs, T, x, 0, 0, 0.0, 0.0); // TODO: make sure Tbase and xbase is defined in the correct way
				break;
			case IncompressibleData::INCOMPRESSIBLE_EXPONENTIAL:
				return baseExponential(volume2input, T, Tbase);
				break;
			case IncompressibleData::INCOMPRESSIBLE_EXPPOLYNOMIAL:
				return exp(poly.evaluate(volume2input.coeffs, T, x, 0, 0, 0.0, 0.0)); // TODO: make sure Tbase and xbase is defined in the correct way
				break;
			case IncompressibleData::INCOMPRESSIBLE_EXPOFFSET:
				return baseExponentialOffset(volume2input, T);
				break;
			case IncompressibleData::INCOMPRESSIBLE_POLYOFFSET:
				return basePolyOffset(volume2input, T, x);
				break;
			case IncompressibleData::INCOMPRESSIBLE_NOT_SET:
				throw ValueError(format("%s (%d): The function type is not specified (\"[%d]\"), are you sure the coefficients have been set?",__FILE__,__LINE__,volume2input.type));
				break;
			default:
				throw ValueError(format("%s (%d): Your function type \"[%d]\" is unknown.",__FILE__,__LINE__,volume2input.type));
				break;
		}
		return _HUGE;
	}
}

/// Mole fraction conversion function
/** If the fluid type is mole-based, it does not do anything. Otherwise,
 *  it converts the mole fraction to the required input. */
double IncompressibleFluid::inputFromMole (double T,     double x){
	if (this->xid==ifrac_pure) {
			return _HUGE;
	} else if (this->xid==ifrac_mole) {
		return x;
	} else {
		throw NotImplementedError("Mole composition conversion has not been implemented.");
		switch (mole2input.type) {
			case IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL:
				return poly.evaluate(mole2input.coeffs, T, x, 0, 0, 0.0, 0.0); // TODO: make sure Tbase and xbase is defined in the correct way
				break;
			case IncompressibleData::INCOMPRESSIBLE_EXPONENTIAL:
				return baseExponential(mole2input, T, Tbase);
				break;
			case IncompressibleData::INCOMPRESSIBLE_EXPPOLYNOMIAL:
				return exp(poly.evaluate(mole2input.coeffs, T, x, 0, 0, 0.0, 0.0)); // TODO: make sure Tbase and xbase is defined in the correct way
				break;
			case IncompressibleData::INCOMPRESSIBLE_EXPOFFSET:
				return baseExponentialOffset(mole2input, T);
				break;
			case IncompressibleData::INCOMPRESSIBLE_POLYOFFSET:
				return basePolyOffset(mole2input, T, x);
				break;
			case IncompressibleData::INCOMPRESSIBLE_NOT_SET:
				throw ValueError(format("%s (%d): The function type is not specified (\"[%d]\"), are you sure the coefficients have been set?",__FILE__,__LINE__,mole2input.type));
				break;
			default:
				throw ValueError(format("%s (%d): Your function type \"[%d]\" is unknown.",__FILE__,__LINE__,mole2input.type));
				break;
		}
		return _HUGE;
	}
}

/* Some functions can be inverted directly, those are listed
 * here. It is also possible to solve for other quantities, but
 * that involves some more sophisticated processing and is not
 * done here, but in the backend, T(h,p) for example.
 */
/// Temperature as a function of density, pressure and composition.
double IncompressibleFluid::T_rho (double Dmass, double p, double x){
	double d_raw = Dmass; // No changes needed, no reference values...
	switch (density.type) {
		case IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL:
			return poly.solve_limits(density.coeffs, x, d_raw, Tmin, Tmax, 0, 0, 0, Tbase, xbase);
			break;
		case IncompressibleData::INCOMPRESSIBLE_NOT_SET:
			throw ValueError(format("%s (%d): The function type is not specified (\"[%d]\"), are you sure the coefficients have been set?",__FILE__,__LINE__,specific_heat.type));
			break;
		default:
			throw ValueError(format("%s (%d): There is no predefined way to use this function type \"[%d]\" for inverse density.",__FILE__,__LINE__,specific_heat.type));
			break;
	}
	return _HUGE;
}
/// Temperature as a function of heat capacities as a function of temperature, pressure and composition.
double IncompressibleFluid::T_c   (double Cmass, double p, double x){
	double c_raw = Cmass; // No changes needed, no reference values...
	switch (specific_heat.type) {
		case IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL:
			return poly.solve_limits(specific_heat.coeffs, x, c_raw, Tmin, Tmax, 0, 0, 0, Tbase, xbase);
			break;
		case IncompressibleData::INCOMPRESSIBLE_NOT_SET:
			throw ValueError(format("%s (%d): The function type is not specified (\"[%d]\"), are you sure the coefficients have been set?",__FILE__,__LINE__,specific_heat.type));
			break;
		default:
			throw ValueError(format("%s (%d): There is no predefined way to use this function type \"[%d]\" for inverse specific heat.",__FILE__,__LINE__,specific_heat.type));
			break;
	}
	return _HUGE;
}
/// Temperature as a function of entropy as a function of temperature, pressure and composition.
double IncompressibleFluid::T_s   (double Smass, double p, double x){
	double s_raw = Smass + sref;
	switch (specific_heat.type) {
		case IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL:
			return poly.solve_limitsInt(specific_heat.coeffs, x, s_raw, Tmin, Tmax, 0, -1, 0, Tbase, xbase, 0);
			break;
		case IncompressibleData::INCOMPRESSIBLE_NOT_SET:
			throw ValueError(format("%s (%d): The function type is not specified (\"[%d]\"), are you sure the coefficients have been set?",__FILE__,__LINE__,specific_heat.type));
			break;
		default:
			throw ValueError(format("%s (%d): There is no predefined way to use this function type \"[%d]\" for inverse entropy.",__FILE__,__LINE__,specific_heat.type));
			break;
	}
	return _HUGE;
}
/// Temperature as a function of internal energy as a function of temperature, pressure and composition.
double IncompressibleFluid::T_u   (double Umass, double p, double x){
	double u_raw = Umass + uref;
	switch (specific_heat.type) {
		case IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL:
			return poly.solve_limitsInt(specific_heat.coeffs, x, u_raw, Tmin, Tmax, 0, 0, 0, Tbase, xbase, 0);
			break;
		case IncompressibleData::INCOMPRESSIBLE_NOT_SET:
			throw ValueError(format("%s (%d): The function type is not specified (\"[%d]\"), are you sure the coefficients have been set?",__FILE__,__LINE__,specific_heat.type));
			break;
		default:
			throw ValueError(format("%s (%d): There is no predefined way to use this function type \"[%d]\" for inverse entropy.",__FILE__,__LINE__,specific_heat.type));
			break;
	}
	return _HUGE;
}
///// Temperature as a function of enthalpy, pressure and composition.
//double IncompressibleFluid::T_h   (double Hmass, double p, double x){throw NotImplementedError(format("%s (%d): T from enthalpy is not implemented in the fluid, use the backend.",__FILE__,__LINE__));}
///// Viscosity as a function of temperature, pressure and composition.
//double IncompressibleFluid::T_visc(double visc, double p, double x){throw NotImplementedError(format("%s (%d): T from viscosity is not implemented.",__FILE__,__LINE__));}
///// Thermal conductivity as a function of temperature, pressure and composition.
//double IncompressibleFluid::T_cond(double cond, double p, double x){throw NotImplementedError(format("%s (%d): T from conductivity is not implemented.",__FILE__,__LINE__));}
///// Saturation pressure as a function of temperature and composition.
//double IncompressibleFluid::T_psat(double psat,           double x){throw NotImplementedError(format("%s (%d): T from psat is not implemented.",__FILE__,__LINE__));}
///// Composition as a function of freezing temperature and pressure.
//double IncompressibleFluid::x_Tfreeze(       double Tfreeze, double p){throw NotImplementedError(format("%s (%d): x from T_freeze is not implemented.",__FILE__,__LINE__));}


/*
 * Some more functions to provide a single implementation
 * of important routines.
 * We start with the check functions that can validate input
 * in terms of pressure p, temperature T and composition x.
 */
/// Check validity of temperature input.
/** Compares the given temperature T to the result of a
 *  freezing point calculation. This is not necessarily
 *  defined for all fluids, default values do not cause errors. */
bool IncompressibleFluid::checkT(double T, double p, double x) {
	if (Tmin <= 0.) {
		throw ValueError("Please specify the minimum temperature.");
	} else if (Tmax <= 0.) {
		throw ValueError("Please specify the maximum temperature.");
	} else if ((Tmin > T) || (T > Tmax)) {
		throw ValueError(
				format("Your temperature %f is not between %f and %f.",
						T, Tmin, Tmax));
	} else {
		double TF = 0.0;
		if (T_freeze.type!=IncompressibleData::INCOMPRESSIBLE_NOT_SET) {
			TF = Tfreeze(p, x);
		}
		if ( T<Tfreeze(p, x)) {
			throw ValueError(
				format("Your temperature %f is below the freezing point of %f.",
						T, Tfreeze(p, x)));
		} else {
			return true;
		}
	}
	return false;
}

/// Check validity of pressure input.
/** Compares the given pressure p to the saturation pressure at
 *  temperature T and throws and exception if p is lower than
 *  the saturation conditions.
 *  The default value for psat is -1 yielding true if psat
 *  is not redefined in the subclass.
 *  */
bool IncompressibleFluid::checkP(double T, double p, double x) {
	double ps = 0.0;
	if (p_sat.type!=IncompressibleData::INCOMPRESSIBLE_NOT_SET) {
		ps = psat(T, x);
	}
	if (p < 0.0) {
		throw ValueError(
				format("You cannot use negative pressures: %f < %f. ",
						p, 0.0));
	} else if (p < ps) {
		throw ValueError(
				format("Equations are valid for liquid phase only: %f < %f. ",
						p, ps));
	} else {
		return true;
	}
	return false;
}

/// Check validity of composition input.
/** Compares the given composition x to a stored minimum and
 *  maximum value. Enforces the redefinition of xmin and
 *  xmax since the default values cause an error. */
bool IncompressibleFluid::checkX(double x){
	if (xmin < 0. || xmin > 1.0) {
		throw ValueError("Please specify the minimum concentration between 0 and 1.");
	} else if (xmax < 0.0 || xmax > 1.0) {
		throw ValueError("Please specify the maximum concentration between 0 and 1.");
	} else if ((xmin > x) || (x > xmax)) {
		throw ValueError(
				format("Your composition %f is not between %f and %f.",
						x, xmin, xmax));
	} else {
		return true;
	}
	return false;
}

} /* namespace CoolProp */



// Testing still needs to be enhanced.
/* Below, I try to carry out some basic tests for both 2D and 1D
 * polynomials as well as the exponential functions for vapour
 * pressure etc.
 */
#ifdef ENABLE_CATCH
#include <math.h>
#include <iostream>
#include "catch.hpp"


Eigen::MatrixXd makeMatrix(const std::vector<double> &coefficients){
	//IncompressibleClass::checkCoefficients(coefficients,18);
	std::vector< std::vector<double> > matrix;
	std::vector<double> tmpVector;

	tmpVector.clear();
	tmpVector.push_back(coefficients[0]);
	tmpVector.push_back(coefficients[6]);
	tmpVector.push_back(coefficients[11]);
	tmpVector.push_back(coefficients[15]);
	matrix.push_back(tmpVector);

	tmpVector.clear();
	tmpVector.push_back(coefficients[1]*100.0);
	tmpVector.push_back(coefficients[7]*100.0);
	tmpVector.push_back(coefficients[12]*100.0);
	tmpVector.push_back(coefficients[16]*100.0);
	matrix.push_back(tmpVector);

	tmpVector.clear();
	tmpVector.push_back(coefficients[2]*100.0*100.0);
	tmpVector.push_back(coefficients[8]*100.0*100.0);
	tmpVector.push_back(coefficients[13]*100.0*100.0);
	tmpVector.push_back(coefficients[17]*100.0*100.0);
	matrix.push_back(tmpVector);

	tmpVector.clear();
	tmpVector.push_back(coefficients[3]*100.0*100.0*100.0);
	tmpVector.push_back(coefficients[9]*100.0*100.0*100.0);
	tmpVector.push_back(coefficients[14]*100.0*100.0*100.0);
	tmpVector.push_back(0.0);
	matrix.push_back(tmpVector);

	tmpVector.clear();
	tmpVector.push_back(coefficients[4]*100.0*100.0*100.0*100.0);
	tmpVector.push_back(coefficients[10]*100.0*100.0*100.0*100.0);
	tmpVector.push_back(0.0);
	tmpVector.push_back(0.0);
	matrix.push_back(tmpVector);

	tmpVector.clear();
	tmpVector.push_back(coefficients[5]*100.0*100.0*100.0*100.0*100.0);
	tmpVector.push_back(0.0);
	tmpVector.push_back(0.0);
	tmpVector.push_back(0.0);
	matrix.push_back(tmpVector);



	tmpVector.clear();
	return CoolProp::vec_to_eigen(matrix).transpose();
}


TEST_CASE("Internal consistency checks and example use cases for the incompressible fluids","[IncompressibleFluids]")
{
	bool PRINT = false;
	std::string tmpStr;
	std::vector<double> tmpVector;
	std::vector< std::vector<double> > tmpMatrix;


	SECTION("Test case for \"SylthermXLT\" by Dow Chemicals") {

		std::vector<double> cRho;
		cRho.push_back(+1.1563685145E+03);
		cRho.push_back(-1.0269048032E+00);
		cRho.push_back(-9.3506079577E-07);
		cRho.push_back(+1.0368116627E-09);
		CoolProp::IncompressibleData density;
		density.type = CoolProp::IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL;
		density.coeffs = CoolProp::vec_to_eigen(cRho);

		std::vector<double> cHeat;
		cHeat.push_back(+1.1562261074E+03);
		cHeat.push_back(+2.0994549103E+00);
		cHeat.push_back(+7.7175381057E-07);
		cHeat.push_back(-3.7008444051E-20);
		CoolProp::IncompressibleData specific_heat;
		specific_heat.type = CoolProp::IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL;
		specific_heat.coeffs = CoolProp::vec_to_eigen(cHeat);

		std::vector<double> cCond;
		cCond.push_back(+1.6121957379E-01);
		cCond.push_back(-1.3023781944E-04);
		cCond.push_back(-1.4395238766E-07);
		CoolProp::IncompressibleData conductivity;
		conductivity.type = CoolProp::IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL;
		conductivity.coeffs = CoolProp::vec_to_eigen(cCond);

		std::vector<double> cVisc;
		cVisc.push_back(+1.0337654989E+03);
		cVisc.push_back(-4.3322764383E+01);
		cVisc.push_back(+1.0715062356E+01);
		CoolProp::IncompressibleData viscosity;
		viscosity.type = CoolProp::IncompressibleData::INCOMPRESSIBLE_EXPONENTIAL;
		viscosity.coeffs = CoolProp::vec_to_eigen(cVisc);

		CoolProp::IncompressibleFluid XLT;
		XLT.setName("XLT");
		XLT.setDescription("SylthermXLT");
		XLT.setReference("Dow Chemicals data sheet");
		XLT.setTmax(533.15);
		XLT.setTmin(173.15);
		XLT.setxmax(0.0);
		XLT.setxmin(0.0);
		XLT.setTminPsat(533.15);

		XLT.setTbase(0.0);
		XLT.setxbase(0.0);

		/// Setters for the coefficients
		XLT.setDensity(density);
		XLT.setSpecificHeat(specific_heat);
		XLT.setViscosity(viscosity);
		XLT.setConductivity(conductivity);
		//XLT.setPsat(parse_coefficients(fluid_json, "saturation_pressure", false));
		//XLT.setTfreeze(parse_coefficients(fluid_json, "T_freeze", false));
		//XLT.setVolToMass(parse_coefficients(fluid_json, "volume2mass", false));
		//XLT.setMassToMole(parse_coefficients(fluid_json, "mass2mole", false));

		//XLT.set_reference_state(25+273.15, 1.01325e5, 0.0, 0.0, 0.0);
		double Tref = 25+273.15;
		double pref = 0.0;
		double xref = 0.0;
		double href = 0.0;
		double sref = 0.0;
		XLT.set_reference_state(Tref, pref, xref, href, sref);

		/// A function to check coefficients and equation types.
		//XLT.validate();


		// Prepare the results and compare them to the calculated values
		double acc = 0.0001;
		double T = 273.15+50;
		double p = 10e5;
		double x = xref;
		double val = 0;
		double res = 0;

		// Compare density
		val = 824.4615702148608;
		res = XLT.rho(T,p,x);
		{
		CAPTURE(T);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( check_abs(val,res,acc) );
		}

		// Compare cp
		val = 1834.7455527670554;
		res = XLT.c(T,p,x);
		{
		CAPTURE(T);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( check_abs(val,res,acc) );
		}

		// Compare s
		val = 145.59157247249246;
		res = XLT.s(T,p,x);
		{
		CAPTURE(T);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( check_abs(val,res,acc) );
		}

		val = 0.0;
		res = XLT.s(Tref,pref,xref);
		{
		CAPTURE(T);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( val==res );
		}

		// Compare u
		val = 45212.407309106304;
		res = XLT.u(T,p,x);
		{
		CAPTURE(T);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( check_abs(val,res,acc) );
		}

		val = href - pref/XLT.rho(Tref,pref,xref);
		res = XLT.u(Tref,pref,xref);
		{
		CAPTURE(T);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( val==res );
		}

		// Compare h
		val = 46425.32011926845;
		res = XLT.h(T,p,x);
		{
		CAPTURE(T);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( check_abs(val,res,acc) );
		}

		val = 0.0;
		res = XLT.h(Tref,pref,xref);
		{
		CAPTURE(T);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( val==res );
		}

		// Compare v
		val = 0.0008931435169681835;
		res = XLT.visc(T,p,x);
		{
		CAPTURE(T);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( check_abs(val,res,acc) );
		}

		// Compare l
		val = 0.10410086156049088;
		res = XLT.cond(T,p,x);
		{
		CAPTURE(T);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( check_abs(val,res,acc) );
		}
	}


	SECTION("Test case for Methanol from SecCool") {

		tmpVector.clear();
		tmpVector.push_back( 960.24665800);
		tmpVector.push_back(-1.2903839100);
		tmpVector.push_back(-0.0161042520);
		tmpVector.push_back(-0.0001969888);
		tmpVector.push_back( 1.131559E-05);
		tmpVector.push_back( 9.181999E-08);
		tmpVector.push_back(-0.4020348270);
		tmpVector.push_back(-0.0162463989);
		tmpVector.push_back( 0.0001623301);
		tmpVector.push_back( 4.367343E-06);
		tmpVector.push_back( 1.199000E-08);
		tmpVector.push_back(-0.0025204776);
		tmpVector.push_back( 0.0001101514);
		tmpVector.push_back(-2.320217E-07);
		tmpVector.push_back( 7.794999E-08);
		tmpVector.push_back( 9.937483E-06);
		tmpVector.push_back(-1.346886E-06);
		tmpVector.push_back( 4.141999E-08);
		CoolProp::IncompressibleData density;
		density.type = CoolProp::IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL;
		density.coeffs = makeMatrix(tmpVector);

		tmpVector.clear();
		tmpVector.push_back( 3822.9712300);
		tmpVector.push_back(-23.122409500);
		tmpVector.push_back( 0.0678775826);
		tmpVector.push_back( 0.0022413893);
		tmpVector.push_back(-0.0003045332);
		tmpVector.push_back(-4.758000E-06);
		tmpVector.push_back( 2.3501449500);
		tmpVector.push_back( 0.1788839410);
		tmpVector.push_back( 0.0006828000);
		tmpVector.push_back( 0.0002101166);
		tmpVector.push_back(-9.812000E-06);
		tmpVector.push_back(-0.0004724176);
		tmpVector.push_back(-0.0003317949);
		tmpVector.push_back( 0.0001002032);
		tmpVector.push_back(-5.306000E-06);
		tmpVector.push_back( 4.242194E-05);
		tmpVector.push_back( 2.347190E-05);
		tmpVector.push_back(-1.894000E-06);
		CoolProp::IncompressibleData specific_heat;
		specific_heat.type = CoolProp::IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL;
		specific_heat.coeffs = makeMatrix(tmpVector);

		tmpVector.clear();
		tmpVector.push_back( 0.4082066700);
		tmpVector.push_back(-0.0039816870);
		tmpVector.push_back( 1.583368E-05);
		tmpVector.push_back(-3.552049E-07);
		tmpVector.push_back(-9.884176E-10);
		tmpVector.push_back( 4.460000E-10);
		tmpVector.push_back( 0.0006629321);
		tmpVector.push_back(-2.686475E-05);
		tmpVector.push_back( 9.039150E-07);
		tmpVector.push_back(-2.128257E-08);
		tmpVector.push_back(-5.562000E-10);
		tmpVector.push_back( 3.685975E-07);
		tmpVector.push_back( 7.188416E-08);
		tmpVector.push_back(-1.041773E-08);
		tmpVector.push_back( 2.278001E-10);
		tmpVector.push_back( 4.703395E-08);
		tmpVector.push_back( 7.612361E-11);
		tmpVector.push_back(-2.734000E-10);
		CoolProp::IncompressibleData conductivity;
		conductivity.type = CoolProp::IncompressibleData::INCOMPRESSIBLE_POLYNOMIAL;
		conductivity.coeffs = makeMatrix(tmpVector);

		tmpVector.clear();
		tmpVector.push_back( 1.4725525500);
		tmpVector.push_back( 0.0022218998);
		tmpVector.push_back(-0.0004406139);
		tmpVector.push_back( 6.047984E-06);
		tmpVector.push_back(-1.954730E-07);
		tmpVector.push_back(-2.372000E-09);
		tmpVector.push_back(-0.0411841566);
		tmpVector.push_back( 0.0001784479);
		tmpVector.push_back(-3.564413E-06);
		tmpVector.push_back( 4.064671E-08);
		tmpVector.push_back( 1.915000E-08);
		tmpVector.push_back( 0.0002572862);
		tmpVector.push_back(-9.226343E-07);
		tmpVector.push_back(-2.178577E-08);
		tmpVector.push_back(-9.529999E-10);
		tmpVector.push_back(-1.699844E-06);
		tmpVector.push_back(-1.023552E-07);
		tmpVector.push_back( 4.482000E-09);
		CoolProp::IncompressibleData viscosity;
		viscosity.type = CoolProp::IncompressibleData::INCOMPRESSIBLE_EXPPOLYNOMIAL;
		viscosity.coeffs = makeMatrix(tmpVector);


		tmpVector.clear();
		tmpVector.push_back( 27.755555600/100.0); // reference concentration in per cent
		tmpVector.push_back(-22.973221700+273.15);
		tmpVector.push_back(-1.1040507200*100.0);
		tmpVector.push_back(-0.0120762281*100.0*100.0);
		tmpVector.push_back(-9.343458E-05*100.0*100.0*100.0);
		CoolProp::IncompressibleData T_freeze;
		T_freeze.type = CoolProp::IncompressibleData::INCOMPRESSIBLE_POLYOFFSET;
		T_freeze.coeffs = CoolProp::vec_to_eigen(tmpVector);


		// After preparing the coefficients, we have to create the objects
		CoolProp::IncompressibleFluid CH3OH;
		CH3OH.setName("CH3OH");
		CH3OH.setDescription("Methanol solution");
		CH3OH.setReference("SecCool software");
		CH3OH.setTmax( 20 + 273.15);
		CH3OH.setTmin(-50 + 273.15);
		CH3OH.setxmax(0.5);
		CH3OH.setxmin(0.0);
		CH3OH.setTminPsat( 20 + 273.15);

		CH3OH.setTbase(-4.48 + 273.15);
		CH3OH.setxbase(31.57 / 100.0);

		/// Setters for the coefficients
		CH3OH.setDensity(density);
		CH3OH.setSpecificHeat(specific_heat);
		CH3OH.setViscosity(viscosity);
		CH3OH.setConductivity(conductivity);
		//CH3OH.setPsat(saturation_pressure);
		CH3OH.setTfreeze(T_freeze);
		//CH3OH.setVolToMass(volume2mass);
		//CH3OH.setMassToMole(mass2mole);

		//XLT.set_reference_state(25+273.15, 1.01325e5, 0.0, 0.0, 0.0);
		double Tref = 25+273.15;
		double pref = 0.0;
		double xref = 0.25;
		double href = 0.0;
		double sref = 0.0;
		CH3OH.set_reference_state(Tref, pref, xref, href, sref);

		/// A function to check coefficients and equation types.
		//CH3OH.validate();

		// Prepare the results and compare them to the calculated values
		double acc = 0.0001;
		double T   = 273.15+10;
		double p   = 10e5;
		double x   = 0.25;
		double val = 0;
		double res = 0;

		// Compare density
		val = 963.2886528091547;
		res = CH3OH.rho(T,p,x);
		{
		CAPTURE(T);
		CAPTURE(p);
		CAPTURE(x);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( check_abs(val,res,acc) );
		}

		// Compare cp
		val = 3993.9748117022423;
		res = CH3OH.c(T,p,x);
		{
		CAPTURE(T);
		CAPTURE(p);
		CAPTURE(x);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( check_abs(val,res,acc) );
		}

		// Compare s
		val = -206.62646783739274;
		res = CH3OH.s(T,p,x);
		{
		CAPTURE(T);
		CAPTURE(p);
		CAPTURE(x);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( check_abs(val,res,acc) );
		}

		val = 0.0;
		res = CH3OH.s(Tref,pref,xref);
		{
		CAPTURE(T);
		CAPTURE(p);
		CAPTURE(x);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( val==res );
		}

		// Compare u
		val = -60043.78429641827;
		res = CH3OH.u(T,p,x);
		{
		CAPTURE(T);
		CAPTURE(p);
		CAPTURE(x);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( check_abs(val,res,acc) );
		}

		val = href - pref/CH3OH.rho(Tref,pref,xref);
		res = CH3OH.u(Tref,pref,xref);
		{
		CAPTURE(T);
		CAPTURE(p);
		CAPTURE(x);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( val==res );
		}

		// Compare h
		val = -59005.67386390795;
		res = CH3OH.h(T,p,x);
		{
		CAPTURE(T);
		CAPTURE(p);
		CAPTURE(x);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( check_abs(val,res,acc) );
		}

		val = 0.0;
		res = CH3OH.h(Tref,pref,xref);
		{
		CAPTURE(T);
		CAPTURE(p);
		CAPTURE(x);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( val==res );
		}

		// Compare v
		val = 0.0023970245009602097;
		res = CH3OH.visc(T,p,x)/1e3;
		{
		CAPTURE(T);
		CAPTURE(p);
		CAPTURE(x);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( check_abs(val,res,acc) );
		}

		// Compare l
		val = 0.44791148414693727;
		res = CH3OH.cond(T,p,x);
		{
		CAPTURE(T);
		CAPTURE(p);
		CAPTURE(x);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( check_abs(val,res,acc) );
		}

		// Compare Tfreeze
		val = -20.02+273.15;// 253.1293105454671;
		res = CH3OH.Tfreeze(p,x);
		{
		CAPTURE(T);
		CAPTURE(p);
		CAPTURE(x);
		CAPTURE(val);
		CAPTURE(res);
		CHECK( check_abs(val,res,acc) );
		}


	}


}

#endif /* ENABLE_CATCH */