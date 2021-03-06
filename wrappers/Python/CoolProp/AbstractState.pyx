# This file is embedded directly in CoolProp.pyx

cimport constants_header
        
cdef class PyPhaseEnvelopeData:
    pass
    
cdef class AbstractState:
    """
    This class is a one-to-one python wrapper of the :cpapi:`AbstractState` class
    """
    
    def __cinit__(self, string backend, string fluid):
        self.thisptr = cAbstractState.factory(backend, fluid)
        
    def __dealloc__(self):
        del self.thisptr
        
    cpdef specify_phase(self, constants_header.phases phase):
        """ Specify the phase - wrapper of c++ function :cpapi:`CoolProp::AbstractState::specify_phase` """
        self.thisptr.specify_phase(phase)
    cpdef unspecify_phase(self):
        """ Unspecify the phase - wrapper of c++ function :cpapi:`CoolProp::AbstractState::specify_phase` """
        self.thisptr.unspecify_phase()
    
    cpdef update(self, constants_header.input_pairs ipair, double Value1, double Value2):
        """ Update function - wrapper of c++ function :cpapi:`CoolProp::AbstractState::update` """
        self.thisptr.update(ipair, Value1, Value2)
    
    cpdef set_mole_fractions(self, vector[double] z): 
        """ Set the mole fractions - wrapper of c++ function :cpapi:`CoolProp::AbstractState::set_mole_fractions` """
        self.thisptr.set_mole_fractions(z)
    cpdef set_mass_fractions(self, vector[double] z): 
        """ Set the mass fractions - wrapper of c++ function :cpapi:`CoolProp::AbstractState::set_mass_fractions` """
        self.thisptr.set_mass_fractions(z)
    cpdef set_volu_fractions(self, vector[double] z): 
        """ Set the volume fractions - wrapper of c++ function :cpapi:`CoolProp::AbstractState::set_volu_fractions` """
        self.thisptr.set_volu_fractions(z)
        
    ## ----------------------------------------	
    ##        Fluid property accessors
    ## ----------------------------------------
    
    cpdef double keyed_output(self, parameters iOutput) except *: 
        """ Update :cpapi:`CoolProp::AbstractState::keyed_output(parameters key)` """
        return self.thisptr.keyed_output(iOutput)
    
    cpdef double T(self) except *: 
        """ Get the temperature in K - wrapper of c++ function :cpapi:`CoolProp::AbstractState::T(void)` """
        return self.thisptr.T()
    cpdef double p(self) except *: 
        """ Get the pressure in Pa - wrapper of c++ function :cpapi:`CoolProp::AbstractState::p(void)` """
        return self.thisptr.p()
    cpdef double rhomolar(self) except *: 
        """ Get the density in mol/m^3 - wrapper of c++ function :cpapi:`CoolProp::AbstractState::rhomolar(void)` """
        return self.thisptr.rhomolar()
    cpdef double rhomass(self) except *: 
        """ Get the density in kg/m^3 - wrapper of c++ function :cpapi:`CoolProp::AbstractState::rhomass(void)` """
        return self.thisptr.rhomass()
    cpdef double hmolar(self) except *: 
        """ Get the enthalpy in J/mol - wrapper of c++ function :cpapi:`CoolProp::AbstractState::hmolar(void)` """        
        return self.thisptr.hmolar()
    cpdef double hmass(self) except *: 
        """ Get the enthalpy in J/kg - wrapper of c++ function :cpapi:`CoolProp::AbstractState::hmass(void)` """
        return self.thisptr.hmass()        
    cpdef double umolar(self) except *: 
        """ Get the internal energy in J/mol - wrapper of c++ function :cpapi:`CoolProp::AbstractState::umolar(void)` """
        return self.thisptr.umolar()
    cpdef double umass(self) except *: 
        """ Get the internal energy in J/kg - wrapper of c++ function :cpapi:`CoolProp::AbstractState::umass(void)` """
        return self.thisptr.umass()
    cpdef double smolar(self) except *: 
        """ Get the entropy in J/mol/K - wrapper of c++ function :cpapi:`CoolProp::AbstractState::smolar(void)` """
        return self.thisptr.smolar()
    cpdef double smass(self) except *: 
        """ Get the entropy in J/kg/K - wrapper of c++ function :cpapi:`CoolProp::AbstractState::smass(void)` """
        return self.thisptr.smass()
    cpdef double cpmolar(self) except *: 
        """ Get the constant pressure specific heat in J/mol/K - wrapper of c++ function :cpapi:`CoolProp::AbstractState::cpmolar(void)` """
        return self.thisptr.cpmolar()
    cpdef double cpmass(self) except *: 
        """ Get the constant pressure specific heat in J/kg/K - wrapper of c++ function :cpapi:`CoolProp::AbstractState::cpmass(void)` """
        return self.thisptr.cpmass()
    cpdef double cp0molar(self) except *: 
        """ Get the ideal gas constant pressure specific heat in J/mol/K - wrapper of c++ function :cpapi:`CoolProp::AbstractState::cp0molar(void)` """
        return self.thisptr.cp0molar()
    cpdef double cp0mass(self) except *: 
        """ Get the ideal gas constant pressure specific heat in J/kg/K - wrapper of c++ function :cpapi:`CoolProp::AbstractState::cp0mass(void)` """
        return self.thisptr.cp0mass()
    cpdef double cvmolar(self) except *: 
        """ Get the constant volume specific heat in J/mol/K - wrapper of c++ function :cpapi:`CoolProp::AbstractState::cvmolar(void)` """
        return self.thisptr.cvmolar()
    cpdef double cvmass(self) except *: 
        """ Get the constant volume specific heat in J/kg/K - wrapper of c++ function :cpapi:`CoolProp::AbstractState::cvmass(void)` """
        return self.thisptr.cvmass()
    cpdef double speed_sound(self) except *: 
        """ Get the speed of sound in m/s - wrapper of c++ function :cpapi:`CoolProp::AbstractState::speed_sound(void)` """
        return self.thisptr.speed_sound()
    cpdef double molar_mass(self) except *: 
        """ Get the molar mass in kg/mol - wrapper of c++ function :cpapi:`CoolProp::AbstractState::molar_mass(void)` """
        return self.thisptr.molar_mass()
    
    cpdef mole_fractions_liquid(self):
        """ Get the mole fractions of the liquid phase - wrapper of c++ function :cpapi:`CoolProp::AbstractState::mole_fractions_liquid(void)` """
        return self.thisptr.mole_fractions_liquid()
    cpdef mole_fractions_vapor(self):
        """ Get the mole fractions of the vapor phase - wrapper of c++ function :cpapi:`CoolProp::AbstractState::mole_fractions_vapor(void)` """
        return self.thisptr.mole_fractions_vapor()
        
    ## ----------------------------------------	
    ##        Derivatives
    ## ----------------------------------------
    
    cpdef long double first_partial_deriv(self, constants_header.parameters OF , constants_header.parameters WRT, constants_header.parameters CONSTANT) except *: 
        """ Get the first partial derivative - wrapper of c++ function :cpapi:`CoolProp::AbstractState::first_partial_deriv` """
        return self.thisptr.first_partial_deriv(OF, WRT, CONSTANT)
    cpdef long double second_partial_deriv(self, constants_header.parameters OF , constants_header.parameters WRT1, constants_header.parameters CONSTANT1, constants_header.parameters WRT2, constants_header.parameters CONSTANT2) except *: 
        """ Get the second partial derivative - wrapper of c++ function :cpapi:`CoolProp::AbstractState::second_partial_deriv` """
        return self.thisptr.second_partial_deriv(OF, WRT1, CONSTANT1, WRT2, CONSTANT2)
        
    ## ----------------------------------------	
    ##        Melting Line
    ## ----------------------------------------
    
    cpdef bint has_melting_line(self) except *: 
        """ Check if the fluid has a melting line - True if is does, False otherwise - wrapper of c++ function :cpapi:`CoolProp::AbstractState::has_melting_line` """
        return self.thisptr.has_melting_line()
    cpdef double melting_line(self, int param, int given, double value) except *: 
        """ Get values from the melting line - wrapper of c++ function :cpapi:`CoolProp::AbstractState::melting_line` """
        return self.thisptr.melting_line(param, given, value)
    
    
    ## ----------------------------------------	
    ##        Phase envelope
    ## ----------------------------------------
    
    cpdef build_phase_envelope(self, string type):
        """ Build the phase envelope - wrapper of c++ function :cpapi:`CoolProp::AbstractState::build_phase_envelope` """
        self.thisptr.build_phase_envelope(type)
    cpdef PyPhaseEnvelopeData get_phase_envelope_data(self):
        """ Get the phase envelope data - wrapper of c++ function :cpapi:`CoolProp::AbstractState::get_phase_envelope_data` """
        cdef cAbstractState.PhaseEnvelopeData pe_data = self.thisptr.get_phase_envelope_data()
        cdef PyPhaseEnvelopeData pe_out = PyPhaseEnvelopeData()
        pe_out.T = pe_data.T
        pe_out.p = pe_data.p
        pe_out.Q = pe_data.Q
        pe_out.rhomolar_liq = pe_data.rhomolar_liq
        pe_out.rhomolar_vap = pe_data.rhomolar_vap
        pe_out.hmolar_liq = pe_data.hmolar_liq
        pe_out.hmolar_vap = pe_data.hmolar_vap
        pe_out.smolar_liq = pe_data.smolar_liq
        pe_out.smolar_vap = pe_data.smolar_vap
        pe_out.iTsat_max = pe_data.iTsat_max
        pe_out.ipsat_max = pe_data.ipsat_max
        pe_out.TypeI = pe_data.TypeI
        return pe_out
        