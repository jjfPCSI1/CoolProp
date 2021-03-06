
#ifndef HELMHOLTZ_H
#define HELMHOLTZ_H

#include <vector>
#include "rapidjson/rapidjson_include.h"
#include "Eigen/Core"
#include "time.h"


namespace CoolProp{

/// The base class class for the Helmholtz energy terms
/**

Residual Helmholtz Energy Terms:

Term                               | Helmholtz Energy Contribution
----------                         | ------------------------------
ResidualHelmholtzPower             | \f$ \alpha^r=\left\lbrace\begin{array}{cc}\displaystyle\sum_i n_i \delta^{d_i} \tau^{t_i} & l_i=0\\ \displaystyle\sum_i n_i \delta^{d_i} \tau^{t_i} \exp(-\delta^{l_i}) & l_i\neq 0\end{array}\right.\f$
ResidualHelmholtzExponential       | \f$ \alpha^r=\displaystyle\sum_i n_i \delta^{d_i} \tau^{t_i} \exp(-\gamma_i\delta^{l_i}) \f$
ResidualHelmholtzLemmon2005        | \f$ \alpha^r=\displaystyle\sum_i n_i \delta^{d_i} \tau^{t_i} \exp(-\delta^{l_i}-\tau^{m_i})\f$
ResidualHelmholtzGaussian          | \f$ \alpha^r=\displaystyle\sum_i n_i \delta^{d_i} \tau^{t_i} \exp(-\eta_i(\delta-\epsilon_i)^2-\beta_i(\tau-\gamma_i)^2)\f$
ResidualHelmholtzGERG2008Gaussian  | \f$ \alpha^r=\displaystyle\sum_i n_i \delta^{d_i} \tau^{t_i} \exp(-\eta_i(\delta-\epsilon_i)^2-\beta_i(\delta-\gamma_i))\f$
ResidualHelmholtzNonAnalytic       | \f$ \begin{array}{c}\alpha^r&=&\displaystyle\sum_i n_i \Delta^{b_i}\delta\psi \\ \Delta & = & \theta^2+B_i[(\delta-1)^2]^{a_i}\\ \theta & = & (1-\tau)+A_i[(\delta-1)^2]^{1/(2\beta_i)}\\ \psi & = & \exp(-C_i(\delta-1)^2-D_i(\tau-1)^2) \end{array}\f$
ResidualHelmholtzSAFTAssociating   | \f$ \alpha^r = am\left(\ln X-\frac{X}{2}+\frac{1}{2}\right); \f$


Ideal-Gas Helmholtz Energy Terms:

Term                                        | Helmholtz Energy Contribution
----------                                  | ------------------------------
IdealHelmholtzLead                          | \f$ \alpha^0 = n_1 + n_2\tau + \ln\delta \f$
IdealHelmholtzEnthalpyEntropyOffset         | \f$ \alpha^0 = \displaystyle\frac{\Delta s}{R_u/M}+\frac{\Delta h}{(R_u/M)T}\tau \f$
IdealHelmholtzLogTau                        | \f$ \alpha^0 = n_1\log\tau \f$
IdealHelmholtzPower                         | \f$ \alpha^0 = \displaystyle\sum_i n_i\tau^{t_i} \f$
IdealHelmholtzPlanckEinsteinGeneralized     | \f$ \alpha^0 = \displaystyle\sum_i n_i\log[c_i+d_i\exp(\theta_i\tau)] \f$
*/
class BaseHelmholtzTerm{
public:
    BaseHelmholtzTerm(){};
    virtual ~BaseHelmholtzTerm(){};
    /// Returns the base, non-dimensional, Helmholtz energy term (no derivatives) [-]
    /** @param tau Reciprocal reduced temperature where \f$\tau=T_c / T\f$
     *  @param delta Reduced density where \f$\delta = \rho / \rho_c \f$
     */
    virtual long double base(const long double &tau, const long double &delta) throw() = 0;
    /// Returns the first partial derivative of Helmholtz energy term with respect to tau [-]
    /** @param tau Reciprocal reduced temperature where \f$\tau=T_c / T\f$
     *  @param delta Reduced density where \f$\delta = \rho / \rho_c \f$
     */
    virtual long double dTau(const long double &tau, const long double &delta) throw() = 0;
    /// Returns the second partial derivative of Helmholtz energy term with respect to tau [-]
    /** @param tau Reciprocal reduced temperature where \f$\tau=T_c / T\f$
     *  @param delta Reduced density where \f$\delta = \rho / \rho_c \f$
     */ 
    virtual long double dTau2(const long double &tau, const long double &delta) throw() = 0;
    /// Returns the second mixed partial derivative (delta1,dtau1) of Helmholtz energy term with respect to delta and tau [-]
    /** @param tau Reciprocal reduced temperature where \f$\tau=T_c / T\f$
     *  @param delta Reduced density where \f$\delta = \rho / \rho_c \f$
     */
    virtual long double dDelta_dTau(const long double &tau, const long double &delta) throw() = 0;
    /// Returns the first partial derivative of Helmholtz energy term with respect to delta [-]
    /** @param tau Reciprocal reduced temperature where \f$\tau=T_c / T\f$
     *  @param delta Reduced density where \f$\delta = \rho / \rho_c \f$
     */
    virtual long double dDelta(const long double &tau, const long double &delta) throw() = 0;
    /// Returns the second partial derivative of Helmholtz energy term with respect to delta [-]
    /** @param tau Reciprocal reduced temperature where \f$\tau=T_c / T\f$
     *  @param delta Reduced density where \f$\delta = \rho / \rho_c \f$
     */
    virtual long double dDelta2(const long double &tau, const long double &delta) throw() = 0;
    /// Returns the third mixed partial derivative (delta2,dtau1) of Helmholtz energy term with respect to delta and tau [-]
    /** @param tau Reciprocal reduced temperature where \f$\tau=T_c / T\f$
     *  @param delta Reduced density where \f$\delta = \rho / \rho_c \f$
     */
    virtual long double dDelta2_dTau(const long double &tau, const long double &delta) throw() = 0;
    /// Returns the third mixed partial derivative (delta1,dtau2) of Helmholtz energy term with respect to delta and tau [-]
    /** @param tau Reciprocal reduced temperature where \f$\tau=T_c / T\f$
     *  @param delta Reduced density where \f$\delta = \rho / \rho_c \f$
     */
    virtual long double dDelta_dTau2(const long double &tau, const long double &delta) throw() = 0;
    /// Returns the third partial derivative of Helmholtz energy term with respect to tau [-]
    /** @param tau Reciprocal reduced temperature where \f$\tau=T_c / T\f$
     *  @param delta Reduced density where \f$\delta = \rho / \rho_c \f$
     */
    virtual long double dTau3(const long double &tau, const long double &delta) throw() = 0;
    /// Returns the third partial derivative of Helmholtz energy term with respect to delta [-]
    /** @param tau Reciprocal reduced temperature where \f$\tau=T_c / T\f$
     *  @param delta Reduced density where \f$\delta = \rho / \rho_c \f$
     */
    virtual long double dDelta3(const long double &tau, const long double &delta) throw() = 0;
    
};

// #############################################################################
// #############################################################################
// #############################################################################
//                                RESIDUAL TERMS
// #############################################################################
// #############################################################################
// #############################################################################

struct HelmholtzDerivatives
{
    double alphar, dalphar_ddelta, dalphar_dtau, d2alphar_ddelta2, d2alphar_dtau2, d2alphar_ddelta_dtau, 
                d3alphar_ddelta3, d3alphar_ddelta_dtau2, d3alphar_ddelta2_dtau, d3alphar_dtau3;
    void reset(){alphar = 0; dalphar_ddelta = 0; dalphar_dtau = 0; d2alphar_ddelta2 = 0; d2alphar_dtau2 = 0; d2alphar_ddelta_dtau = 0;
                d3alphar_ddelta3 = 0; d3alphar_ddelta_dtau2 = 0; d3alphar_ddelta2_dtau = 0; d3alphar_dtau3 = 0;
                }
    HelmholtzDerivatives(){reset();};
};

struct ResidualHelmholtzGeneralizedExponentialElement
{
    /// These variables are for the n*delta^d_i*tau^t_i part
    long double n,d,t;
    /// These variables are for the exp(u) part
    /// u is given by -c*delta^l_i-omega*tau^m_i-eta1*(delta-epsilon1)-eta2*(delta-epsilon2)^2-beta1*(tau-gamma1)-beta2*(tau-gamma2)^2
    long double c, l_double, omega, m_double, eta1, epsilon1, eta2, epsilon2, beta1, gamma1, beta2, gamma2;
    /// If l_i or m_i are integers, we will store them as integers in order to call pow(double, int) rather than pow(double, double)
    int l_int, m_int;
    
    ResidualHelmholtzGeneralizedExponentialElement()
    {
        n = 0; d = 0; t = 0; c = 0; 
        l_double = 0; omega = 0; m_double = 0; 
        eta1 = 0; epsilon1 = 0; eta2 = 0; epsilon2 = 0;
        beta1 = 0; gamma1 = 0; beta2 = 0; gamma2 = 0;
        l_int = 0; m_int = 0;
    }
};
class ResidualHelmholtzGeneralizedExponential : public BaseHelmholtzTerm{
    
public:
    bool delta_li_in_u, tau_mi_in_u, eta1_in_u, eta2_in_u, beta1_in_u, beta2_in_u, finished;
    std::vector<long double> s;
    std::size_t N;
    
    /// These variables are for the exp(u) part
    /// u is given by -c*delta^l_i-omega*tau^m_i-eta1*(delta-epsilon1)-eta2*(delta-epsilon2)^2-beta1*(tau-gamma1)-beta2*(tau-gamma2)^2
    std::vector<double> n,d,t,c, l_double, omega, m_double, eta1, epsilon1, eta2, epsilon2, beta1, gamma1, beta2, gamma2;
    /// If l_i or m_i are integers, we will store them as integers in order to call pow(double, int) rather than pow(double, double)
    std::vector<int> l_int, m_int;
    
    Eigen::ArrayXd uE, du_ddeltaE, du_dtauE, d2u_ddelta2E, d2u_dtau2E, d3u_ddelta3E, d3u_dtau3E;
        
    std::vector<ResidualHelmholtzGeneralizedExponentialElement> elements;
    // Default Constructor
    ResidualHelmholtzGeneralizedExponential(){N = 0; 
                                              delta_li_in_u = false; 
                                              tau_mi_in_u = false;
                                              eta1_in_u = false;
                                              eta2_in_u = false;
                                              beta1_in_u = false;
                                              beta2_in_u = false;
                                              finished = false;
                                              };
    
    void add_Power(const std::vector<long double> &n, const std::vector<long double> &d, 
                   const std::vector<long double> &t, const std::vector<long double> &l)
    {
        for (std::size_t i = 0; i < n.size(); ++i)
        {
            ResidualHelmholtzGeneralizedExponentialElement el;
            el.n = n[i];
            el.d = d[i];
            el.t = t[i];
            el.l_double = l[i];
            el.l_int = (int)el.l_double;
            if (el.l_double > 0)
                el.c = 1.0;
            else
                el.c = 0.0;
            elements.push_back(el);
        }
        delta_li_in_u = true;
    };
    void add_Exponential(const std::vector<long double> &n, const std::vector<long double> &d, 
                         const std::vector<long double> &t, const std::vector<long double> &g, 
                         const std::vector<long double> &l)
    {
        for (std::size_t i = 0; i < n.size(); ++i)
        {
            ResidualHelmholtzGeneralizedExponentialElement el;
            el.n = n[i];
            el.d = d[i];
            el.t = t[i];
            el.c = g[i];
            el.l_double = l[i];
            el.l_int = (int)el.l_double;
            elements.push_back(el);
        }
        delta_li_in_u = true;
    }   
    void add_Gaussian(const std::vector<long double> &n, 
                      const std::vector<long double> &d, 
                      const std::vector<long double> &t, 
                      const std::vector<long double> &eta, 
                      const std::vector<long double> &epsilon,
                      const std::vector<long double> &beta,
                      const std::vector<long double> &gamma
                      )
    { 
        for (std::size_t i = 0; i < n.size(); ++i)
        {
            ResidualHelmholtzGeneralizedExponentialElement el;
            el.n = n[i];
            el.d = d[i];
            el.t = t[i];
            el.eta2 = eta[i];
            el.epsilon2 = epsilon[i];
            el.beta2 = beta[i];
            el.gamma2 = gamma[i];
            elements.push_back(el);
        }
        eta2_in_u = true;
        beta2_in_u = true;
    };
    void add_GERG2008Gaussian(const std::vector<long double> &n, 
                              const std::vector<long double> &d, 
                              const std::vector<long double> &t, 
                              const std::vector<long double> &eta, 
                              const std::vector<long double> &epsilon,
                              const std::vector<long double> &beta,
                              const std::vector<long double> &gamma)
    { 
        for (std::size_t i = 0; i < n.size(); ++i)
        {
            ResidualHelmholtzGeneralizedExponentialElement el;
            el.n = n[i];
            el.d = d[i];
            el.t = t[i];
            el.eta2 = eta[i];
            el.epsilon2 = epsilon[i];
            el.eta1 = beta[i];
            el.epsilon1 = gamma[i];
            elements.push_back(el);
        }
        eta2_in_u = true;
        eta1_in_u = true;
    };
    void add_Lemmon2005(const std::vector<long double> &n, 
                        const std::vector<long double> &d, 
                        const std::vector<long double> &t, 
                        const std::vector<long double> &l, 
                        const std::vector<long double> &m)
    {
        for (std::size_t i = 0; i < n.size(); ++i)
        {
            ResidualHelmholtzGeneralizedExponentialElement el;
            el.n = n[i];
            el.d = d[i];
            el.t = t[i];
            el.c = 1.0;
            el.omega = 1.0;
            el.l_double = l[i];
            el.m_double = m[i];
            el.l_int = (int)el.l_double;
            el.m_int = (int)el.m_double;
            elements.push_back(el);
        }
        delta_li_in_u = true;
        tau_mi_in_u = true;
    };
    
    void finish(){
        n.resize(elements.size()); d.resize(elements.size());
        t.resize(elements.size()); c.resize(elements.size());
        l_double.resize(elements.size()); l_int.resize(elements.size());
        epsilon2.resize(elements.size()); eta2.resize(elements.size());
        gamma2.resize(elements.size()); beta2.resize(elements.size());
        
        for (std::size_t i = 0; i < elements.size(); ++i){
            n[i] = elements[i].n;
            d[i] = elements[i].d;
            t[i] = elements[i].t;
            c[i] = elements[i].c;
            l_double[i] = elements[i].l_double;
            l_int[i] = elements[i].l_int;
            epsilon2[i] = elements[i].epsilon2;
            eta2[i] = elements[i].eta2;
            gamma2[i] = elements[i].gamma2;
            beta2[i] = elements[i].beta2;
        }
        uE.resize(elements.size());
        du_ddeltaE.resize(elements.size());
        du_dtauE.resize(elements.size());
        d2u_ddelta2E.resize(elements.size());
        d2u_dtau2E.resize(elements.size());
        d3u_ddelta3E.resize(elements.size());
        d3u_dtau3E.resize(elements.size());
        
        finished = true;
    };

    ///< Destructor for the class.  No implementation
    ~ResidualHelmholtzGeneralizedExponential(){};

    void to_json(rapidjson::Value &el, rapidjson::Document &doc);
    
    long double base(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.alphar;};
    long double dDelta(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.dalphar_ddelta;};
    long double dTau(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.dalphar_dtau;};
    long double dDelta2(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.d2alphar_ddelta2;};
    long double dDelta_dTau(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.d2alphar_ddelta_dtau;};
    long double dTau2(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.d2alphar_dtau2;};
    long double dDelta3(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.d3alphar_ddelta3;};
    long double dDelta2_dTau(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.d3alphar_ddelta2_dtau;};
    long double dDelta_dTau2(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.d3alphar_ddelta_dtau2;};
    long double dTau3(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.d3alphar_dtau3;};
    
    void all(const long double &tau, const long double &delta, HelmholtzDerivatives &derivs) throw();
    //void allEigen(const long double &tau, const long double &delta, HelmholtzDerivatives &derivs) throw();
};

struct ResidualHelmholtzNonAnalyticElement
{
    long double n, a, b, beta, A, B, C, D;
};
class ResidualHelmholtzNonAnalytic : public BaseHelmholtzTerm{

public:
    std::size_t N;
    std::vector<long double> s;
    std::vector<ResidualHelmholtzNonAnalyticElement> elements;
    /// Default Constructor
    ResidualHelmholtzNonAnalytic(){N = 0;};
    /// Destructor. No implementation
    ~ResidualHelmholtzNonAnalytic(){};
    /// Constructor
    ResidualHelmholtzNonAnalytic(const std::vector<long double> &n, 
                                 const std::vector<long double> &a, 
                                 const std::vector<long double> &b, 
                                 const std::vector<long double> &beta, 
                                 const std::vector<long double> &A,
                                 const std::vector<long double> &B,
                                 const std::vector<long double> &C,
                                 const std::vector<long double> &D
                                 )
    {
        N = n.size(); 
        s.resize(N); 
        for (std::size_t i = 0; i < n.size(); ++i)
        {
            ResidualHelmholtzNonAnalyticElement el;
            el.n = n[i];
            el.a = a[i];
            el.b = b[i];
            el.beta = beta[i];
            el.A = A[i];
            el.B = B[i];
            el.C = C[i];
            el.D = D[i];
            elements.push_back(el);
        }
    };

    void to_json(rapidjson::Value &el, rapidjson::Document &doc);

    long double base(const long double &tau, const long double &delta) throw();
    long double dDelta(const long double &tau, const long double &delta) throw();
    long double dTau(const long double &tau, const long double &delta) throw();
    long double dDelta2(const long double &tau, const long double &delta) throw();
    long double dDelta_dTau(const long double &tau, const long double &delta) throw();
    long double dTau2(const long double &tau, const long double &delta) throw();
    long double dDelta3(const long double &tau, const long double &delta) throw();
    long double dDelta2_dTau(const long double &tau, const long double &delta) throw();
    long double dDelta_dTau2(const long double &tau, const long double &delta) throw();
    long double dTau3(const long double &tau, const long double &delta) throw();
    
    void all(const long double &tau, const long double &delta, HelmholtzDerivatives &derivs) throw();
};

class ResidualHelmholtzSAFTAssociating : public BaseHelmholtzTerm{
    
protected:
    double a, m,epsilonbar, vbarn, kappabar;

    long double Deltabar(const long double &tau, const long double &delta);
    long double dDeltabar_ddelta__consttau(const long double &tau, const long double &delta);
    long double d2Deltabar_ddelta2__consttau(const long double &tau, const long double &delta);
    long double dDeltabar_dtau__constdelta(const long double &tau, const long double &delta);
    long double d2Deltabar_dtau2__constdelta(const long double &tau, const long double &delta);
    long double d2Deltabar_ddelta_dtau(const long double &tau, const long double &delta);
    long double d3Deltabar_dtau3__constdelta(const long double &tau, const long double &delta);
    long double d3Deltabar_ddelta_dtau2(const long double &tau, const long double &delta);
    long double d3Deltabar_ddelta3__consttau(const long double &tau, const long double &delta);
    long double d3Deltabar_ddelta2_dtau(const long double &tau, const long double &delta);

    long double X(const long double &delta, const long double &Deltabar);
    long double dX_dDeltabar__constdelta(const long double &delta, const long double &Deltabar);
    long double dX_ddelta__constDeltabar(const long double &delta, const long double &Deltabar);
    long double dX_dtau(const long double &tau, const long double &delta);
    long double dX_ddelta(const long double &tau, const long double &delta);
    long double d2X_dtau2(const long double &tau, const long double &delta);
    long double d2X_ddeltadtau(const long double &tau, const long double &delta);
    long double d2X_ddelta2(const long double &tau, const long double &delta);

    long double d3X_dtau3(const long double &tau, const long double &delta);
    long double d3X_ddelta3(const long double &tau, const long double &delta);
    long double d3X_ddeltadtau2(const long double &tau, const long double &delta);
    long double d3X_ddelta2dtau(const long double &tau, const long double &delta);

    long double g(const long double &eta);
    long double dg_deta(const long double &eta);
    long double d2g_deta2(const long double &eta);
    long double d3g_deta3(const long double &eta);
    long double eta(const long double &delta);

public:
    /// Default constructor
    ResidualHelmholtzSAFTAssociating(){ disabled = true; };
    // Constructor
    ResidualHelmholtzSAFTAssociating(double a, double m, double epsilonbar, double vbarn, double kappabar)
        : a(a), m(m), epsilonbar(epsilonbar), vbarn(vbarn), kappabar(kappabar)
    {
        disabled = false;
    };

    bool disabled;

    //Destructor. No Implementation
    ~ResidualHelmholtzSAFTAssociating(){};

    void to_json(rapidjson::Value &el, rapidjson::Document &doc);

    long double base(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.alphar;};
    long double dDelta(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.dalphar_ddelta;};
    long double dTau(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.dalphar_dtau;};
    long double dDelta2(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.d2alphar_ddelta2;};
    long double dDelta_dTau(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.d2alphar_ddelta_dtau;};
    long double dTau2(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.d2alphar_dtau2;};
    long double dDelta3(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.d3alphar_ddelta3;};
    long double dDelta2_dTau(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.d3alphar_ddelta2_dtau;};
    long double dDelta_dTau2(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.d3alphar_ddelta_dtau2;};
    long double dTau3(const long double &tau, const long double &delta) throw(){HelmholtzDerivatives deriv; all(tau,delta,deriv); return deriv.d3alphar_dtau3;};
    
    void all(const long double &tau, const long double &delta, HelmholtzDerivatives &deriv) throw();
};

class ResidualHelmholtzContainer
{
    
public:
    ResidualHelmholtzNonAnalytic NonAnalytic;
    ResidualHelmholtzSAFTAssociating SAFT;
    ResidualHelmholtzGeneralizedExponential GenExp; 

    HelmholtzDerivatives all(const long double tau, const long double delta)
    {
        HelmholtzDerivatives derivs; // zeros out the elements
        GenExp.all(tau, delta, derivs);
        NonAnalytic.all(tau, delta, derivs);
        SAFT.all(tau, delta, derivs);
        return derivs;
    };
    long double base(long double tau, long double delta) { return all(tau,delta).alphar; };
    long double dDelta(long double tau, long double delta) { return all(tau,delta).dalphar_ddelta; };
    long double dTau(long double tau, long double delta) { return all(tau, delta).dalphar_dtau; };
    long double dDelta2(long double tau, long double delta) {  return all(tau, delta).d2alphar_ddelta2; };
    long double dDelta_dTau(long double tau, long double delta) { return all(tau, delta).d2alphar_ddelta_dtau; };
    long double dTau2(long double tau, long double delta) { return all(tau, delta).d2alphar_dtau2; };
    long double dDelta3(long double tau, long double delta) { return all(tau, delta).d3alphar_ddelta3; };
    long double dDelta2_dTau(long double tau, long double delta) { return all(tau, delta).d3alphar_ddelta2_dtau; };
    long double dDelta_dTau2(long double tau, long double delta) { return all(tau, delta).d3alphar_ddelta_dtau2; };
    long double dTau3(long double tau, long double delta) { return all(tau, delta).d3alphar_dtau3; };
};

// #############################################################################
// #############################################################################
// #############################################################################
//                                 IDEAL GAS TERMS
// #############################################################################
// #############################################################################
// #############################################################################

/// The leading term in the EOS used to set the desired reference state
/**
\f[
\alpha^0 = \log(\delta)+a_1+a_2\tau
\f]
*/
class IdealHelmholtzLead : public BaseHelmholtzTerm{

private:
    long double a1, a2;
    bool enabled;
public:
    // Default constructor
    IdealHelmholtzLead(){enabled = false;};

    // Constructor
    IdealHelmholtzLead(const long double a1, const long double a2)
    :a1(a1), a2(a2)
    {enabled = true;};

    //Destructor
    ~IdealHelmholtzLead(){};

    bool is_enabled(){return enabled;};

    void to_json(rapidjson::Value &el, rapidjson::Document &doc){
        el.AddMember("type","IdealHelmholtzLead",doc.GetAllocator());
        el.AddMember("a1", static_cast<double>(a1), doc.GetAllocator());
        el.AddMember("a2", static_cast<double>(a2), doc.GetAllocator());
    };

    // Term and its derivatives
    long double base(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        return log(delta)+a1+a2*tau;
    };
    long double dDelta(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        return 1.0/delta;
    };
    long double dTau(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        return a2;
    };
    long double dDelta2(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        return -1.0/delta/delta;
    };
    long double dDelta_dTau(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dTau2(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta3(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        return 2/delta/delta/delta;
    };
    long double dDelta2_dTau(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta_dTau2(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dTau3(const long double &tau, const long double &delta) throw(){return 0.0;};
};

/// The term in the EOS used to shift the reference state of the fluid
/**
\f[
\alpha^0 = a_1+a_2\tau
\f]
*/
class IdealHelmholtzEnthalpyEntropyOffset : public BaseHelmholtzTerm{
private:
    long double a1,a2; // Use these variables internally
    bool enabled;
    std::string reference;
public:
    IdealHelmholtzEnthalpyEntropyOffset(){enabled = false;};

    // Constructor
    IdealHelmholtzEnthalpyEntropyOffset(long double a1, long double a2, std::string reference):a1(a1), a2(a2){this->reference = reference; enabled = true;};

    // Set the values in the class
    void set(long double a1, long double a2, std::string reference){this->a1 = a1; this->a2 = a2; this->reference = reference; enabled = true;}

    //Destructor
    ~IdealHelmholtzEnthalpyEntropyOffset(){};

    bool is_enabled(){return enabled;};

    void to_json(rapidjson::Value &el, rapidjson::Document &doc){
        el.AddMember("type","IdealHelmholtzEnthalpyEntropyOffset",doc.GetAllocator());
        el.AddMember("a1", static_cast<double>(a1), doc.GetAllocator());
        el.AddMember("a2", static_cast<double>(a2), doc.GetAllocator());
    };

    // Term and its derivatives
    long double base(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        return a1+a2*tau;
    };
    long double dDelta(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dTau(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        return a2;
    };
    long double dDelta2(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta_dTau(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dTau2(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta3(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta2_dTau(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta_dTau2(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dTau3(const long double &tau, const long double &delta) throw(){return 0.0;};
};


/**
\f[
\alpha^0 = a_1\ln\tau
\f]
*/
class IdealHelmholtzLogTau : public BaseHelmholtzTerm
{
private:
    long double a1;
    bool enabled;
public:

    /// Default constructor
    IdealHelmholtzLogTau(){enabled = false;};

    // Constructor
    IdealHelmholtzLogTau(long double a1){this->a1=a1; enabled = true;};

    bool is_enabled(){return enabled;};

    //Destructor
    ~IdealHelmholtzLogTau(){};

    void to_json(rapidjson::Value &el, rapidjson::Document &doc){
        el.AddMember("type", "IdealHelmholtzLogTau", doc.GetAllocator());
        el.AddMember("a1", static_cast<double>(a1), doc.GetAllocator());
    };

    // Term and its derivatives
    long double base(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        return a1*log(tau);
    };
    long double dTau(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        return a1/tau;
    };
    long double dTau2(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        return -a1/tau/tau;
    };
    long double dTau3(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        return 2*a1/tau/tau/tau;
    };
    long double dDelta(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta2(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta2_dTau(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta_dTau(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta_dTau2(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta3(const long double &tau, const long double &delta) throw(){return 0.0;};
};

/**
\f[
\alpha^0 = \displaystyle\sum_i n_i\tau^{t_i}
\f]
*/
class IdealHelmholtzPower : public BaseHelmholtzTerm{
    
private:
    std::vector<long double> n, t; // Use these variables internally
    std::size_t N;
    bool enabled;
public:
    IdealHelmholtzPower(){enabled = false;};
    // Constructor
    IdealHelmholtzPower(const std::vector<long double> &n, const std::vector<long double> &t)
    :n(n), t(t)
    {
        this->N = n.size();
        enabled = true;
    };

    //Destructor
    ~IdealHelmholtzPower(){};

    bool is_enabled(){return enabled;};

    void to_json(rapidjson::Value &el, rapidjson::Document &doc)
    {
        el.AddMember("type","IdealHelmholtzPower",doc.GetAllocator());
        cpjson::set_long_double_array("n",n,el,doc);
        cpjson::set_long_double_array("t",t,el,doc);
    };

    // Term and its derivatives
    long double base(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        long double s=0; for (std::size_t i = 0; i<N; ++i){s += n[i]*pow(tau, t[i]);} return s;
    };
    long double dTau(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        long double s=0; for (std::size_t i = 0; i<N; ++i){s += n[i]*t[i]*pow(tau, t[i]-1);} return s;
    };
    long double dTau2(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        long double s=0; for (std::size_t i = 0; i<N; ++i){s += n[i]*t[i]*(t[i]-1)*pow(tau, t[i]-2);} return s;
    };
    long double dTau3(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        long double s=0; for (std::size_t i = 0; i<N; ++i){s += n[i]*t[i]*(t[i]-1)*(t[i]-2)*pow(tau, t[i]-3);} return s;
    };
    long double dDelta(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta2(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta2_dTau(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta_dTau(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta_dTau2(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta3(const long double &tau, const long double &delta) throw(){return 0.0;};
};

/**
\f[
\alpha^0 = \displaystyle\sum_i n_i\log[c_i+d_i\exp(\theta_i\tau)] 
\f]

To convert conventional Plank-Einstein forms, given by 
\f$
\frac{c_p^0}{R} = a_k\displaystyle\frac{\left( b_k/T \right)^2\exp \left( b_k/T \right)}{\left(\exp \left(b_k/T\right) - 1 \right)^2}
\f$
and
\f$
\alpha^0 = a_k\ln \left[1 - \exp \left( \frac{-b_k\tau}{T_c} \right) \right]
\f$
use \f$c = 1\f$, \f$d = -1\f$, \f$n = a\f$, \f$\theta = -\displaystyle\frac{b_k}{T_c}\f$

To convert the second form of Plank-Einstein terms, given by 
\f$
\frac{c_p^0}{R} = a_k\displaystyle\frac{\left( -b_k/T \right)^2\exp \left( b_k/T \right)}{c\left(\exp \left(-b_k/T\right) + 1 \right)^2}
\f$
and
\f$
\alpha^0 = a_k\ln \left[c + \exp \left( \frac{b_k\tau}{T_c} \right) \right]
\f$
use \f$c = 1\f$, \f$d = 1\f$, \f$n = -a\f$, \f$\theta = \displaystyle\frac{b_k}{T_c}\f$

Converting Aly-Lee tems is a bit more complex

Aly-Lee starts as
\f[\frac{c_p^0}{R_u} = A + B\left(\frac{C/T}{\sinh(C/T)}\right)^2 + D\left(\frac{E/T}{\cosh(E/T)}\right)^2\f]

Constant is separated out, and handled separately.  sinh part can be expanded as
\f[B\left(\frac{C/T}{\sinh(C/T)}\right)^2 = \frac{B(-2C/T)^2\exp(-2C/T)}{(1-\exp(-2C/T))^2}\f]
where
\f[n_k = B\f]
\f[\theta_k = -\frac{2C}{T_c}\f]
\f[c_k = 1\f]
\f[d_k = -1\f]

cosh part can be expanded as
\f[D\left(\frac{E/T}{\cosh(E/T)}\right)^2 = \frac{D(-2E/T)^2\exp(-2E/T)}{(1+\exp(-2E/T))^2}\f]
where
\f[n_k = -D\f]
\f[\theta_k = -\frac{2E}{T_c}\f]
\f[c_k = 1\f]
\f[d_k = 1\f]
*/
class IdealHelmholtzPlanckEinsteinGeneralized : public BaseHelmholtzTerm{
    
private:
    std::vector<long double> n,theta,c,d; // Use these variables internally
    std::size_t N;
    bool enabled;
public:
    IdealHelmholtzPlanckEinsteinGeneralized(){N = 0; enabled = false;}
    // Constructor with std::vector instances
    IdealHelmholtzPlanckEinsteinGeneralized(std::vector<long double> n, std::vector<long double> theta, std::vector<long double> c, std::vector<long double> d)
    :n(n), theta(theta), c(c), d(d)
    {
        N = n.size();
        enabled = true;
    };

    // Destructor
    ~IdealHelmholtzPlanckEinsteinGeneralized(){};

    // Extend the vectors to allow for multiple instances feeding values to this function
    void extend(std::vector<long double> n, std::vector<long double> theta, std::vector<long double> c, std::vector<long double> d)
    {
        this->n.insert(this->n.end(), n.begin(), n.end());
        this->theta.insert(this->theta.end(), theta.begin(), theta.end());
        this->c.insert(this->c.end(), c.begin(), c.end());
        this->d.insert(this->d.end(), d.begin(), d.end());
        N += n.size();
    }

    bool is_enabled(){return enabled;};
  
    void to_json(rapidjson::Value &el, rapidjson::Document &doc)
    {
        el.AddMember("type","IdealHelmholtzPlanckEinsteinGeneralized",doc.GetAllocator());
        cpjson::set_long_double_array("n",n,el,doc);
        cpjson::set_long_double_array("theta",theta,el,doc);
    };

    // Term and its derivatives
    long double base(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        long double s=0; for (std::size_t i=0; i < N; ++i){
            s += n[i]*log(c[i]+d[i]*exp(theta[i]*tau));
        } 
        return s;
    };
    long double dTau(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        long double s=0; for (std::size_t i=0; i < N; ++i){s += n[i]*theta[i]*d[i]*exp(theta[i]*tau)/(c[i]+d[i]*exp(theta[i]*tau));} 
        return s;
    };
    long double dTau2(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        long double s=0; for (std::size_t i=0; i < N; ++i){s += n[i]*pow(theta[i],2)*c[i]*d[i]*exp(theta[i]*tau)/pow(c[i]+d[i]*exp(theta[i]*tau),2);} return s;
    };
    long double dTau3(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        long double s=0; for (std::size_t i=0; i < N; ++i){s += n[i]*pow(theta[i],3)*c[i]*d[i]*(c[i]-d[i]*exp(theta[i]*tau))*exp(theta[i]*tau)/pow(c[i]+d[i]*exp(theta[i]*tau),3);} return s;
    };
    long double dDelta(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta2(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta2_dTau(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta_dTau(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta_dTau2(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta3(const long double &tau, const long double &delta) throw(){return 0;};
};

class IdealHelmholtzCP0Constant : public BaseHelmholtzTerm{

private:
    double cp_over_R,Tc,T0,tau0; // Use these variables internally
    bool enabled;
public:
    /// Default constructor
    IdealHelmholtzCP0Constant(){enabled = false;};

    /// Constructor with just a single double value
    IdealHelmholtzCP0Constant(long double cp_over_R, long double Tc, long double T0) 
    : cp_over_R(cp_over_R), Tc(Tc), T0(T0)
    { 
        enabled = true; tau0 = Tc/T0;
    };

    /// Destructor
    ~IdealHelmholtzCP0Constant(){};

    bool is_enabled(){return enabled;};

    void to_json(rapidjson::Value &el, rapidjson::Document &doc)
    {
        el.AddMember("type","IdealGasHelmholtzCP0Constant", doc.GetAllocator());
        el.AddMember("cp_over_R", cp_over_R, doc.GetAllocator());
        el.AddMember("Tc", Tc, doc.GetAllocator());
        el.AddMember("T0", T0, doc.GetAllocator());
    };

    // Term and its derivatives
    long double base(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        return cp_over_R-cp_over_R*tau/tau0+cp_over_R*log(tau/tau0);
    };
    long double dTau(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        return cp_over_R/tau-cp_over_R/tau0;
    };
    long double dTau2(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        return -cp_over_R/(tau*tau);
    };
    long double dTau3(const long double &tau, const long double &delta) throw(){
        if (!enabled){return 0.0;}
        return 2*cp_over_R/(tau*tau*tau);
    };
    long double dDelta(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta2(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta2_dTau(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta_dTau(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta_dTau2(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta3(const long double &tau, const long double &delta) throw(){return 0.0;};
};

class IdealHelmholtzCP0PolyT : public BaseHelmholtzTerm{
private:
    std::vector<long double> c, t;
    long double Tc, T0, tau0; // Use these variables internally
    std::size_t N;
    bool enabled;
public:
    /// Destructor
    IdealHelmholtzCP0PolyT(){N = 0; enabled = false;};

    /// Constructor with std::vectors
    IdealHelmholtzCP0PolyT(const std::vector<long double> &c, const std::vector<long double> &t, double Tc, double T0) 
    : c(c), t(t), Tc(Tc), T0(T0)
    { 
        assert(c.size() == t.size());
        tau0 = Tc/T0;
        enabled = true;
        N = c.size();
    };

    void extend(const std::vector<long double> &c, const std::vector<long double> &t)
    {
        this->c.insert(this->c.end(), c.begin(), c.end());
        this->t.insert(this->t.end(), t.begin(), t.end());
        N += c.size();
    }

    /// Destructor
    ~IdealHelmholtzCP0PolyT(){};

    bool is_enabled(){return enabled;};

    void to_json(rapidjson::Value &el, rapidjson::Document &doc);

    // Term and its derivatives
    long double base(const long double &tau, const long double &delta) throw();
    long double dDelta(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dTau(const long double &tau, const long double &delta) throw();
    long double dDelta2(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta_dTau(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dTau2(const long double &tau, const long double &delta) throw();
    long double dDelta3(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta2_dTau(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta_dTau2(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dTau3(const long double &tau, const long double &delta) throw();
    
};

/// Term in the ideal-gas specific heat equation that is based on Aly-Lee formulation
/** Specific heat is of the form:
\f[
\frac{c_p^0}{R_u} = A + B\left(\frac{C/T}{\sinh(C/T)}\right)^2 + D\left(\frac{E/T}{\cosh(E/T)}\right)^2
\f]
Second partial of ideal-gas Helmholtz energy given directly by specific heat (\f$\displaystyle\alpha_{\tau\tau}^0=-\frac{1}{\tau^2}\frac{c_p^0}{R_u} \f$) - this is obtained by real gas \f$c_p\f$ relationship, and killing off residual Helmholtz terms
\f[
\alpha^0_{\tau\tau} = -\frac{A}{\tau^2} - \frac{B}{\tau^2}\left(\frac{C/T}{\sinh(C/T)}\right)^2 - \frac{D}{\tau^2}\left(\frac{E/T}{\cosh(E/T)}\right)^2
\f]
or in terms of \f$ \tau \f$:
\f[
\alpha^0_{\tau\tau} = -\frac{A}{\tau^2} - \frac{BC^2}{T_c^2}\left(\frac{1}{\sinh(C\tau/T_c)}\right)^2 - \frac{DE^2}{T_c^2}\left(\frac{1}{\cosh(E\tau/T_c)}\right)^2
\f]
Third partial:
\f[
\alpha^0_{\tau\tau\tau} = 2\frac{A}{\tau^3} + 2\frac{BC^3}{T_c^3}\frac{\cosh(C\tau/T_c)}{\sinh^3(C\tau/T_c)} +2 \frac{DE^3}{T_c^3}\frac{\sinh(E\tau/T_c)}{\cosh^3(E\tau/T_c)}
\f]
Now coming back to the ideal gas Helmholtz energy definition:
\f[
\alpha^0 = -\tau\displaystyle\int_{\tau_0}^{\tau} \frac{1}{\tau^2}\frac{c_p^0}{R_u}d\tau+\displaystyle\int_{\tau_0}^{\tau} \frac{1}{\tau}\frac{c_p^0}{R_u}d\tau
\f]
Applying derivative
\f[
\alpha^0_{\tau} = -\displaystyle\int_{\tau_0}^{\tau} \frac{1}{\tau^2}\frac{c_p^0}{R_u}d\tau-\tau\frac{\partial}{\partial \tau}\left[\displaystyle\int_{\tau_0}^{\tau} \frac{1}{\tau^2}\frac{c_p^0}{R_u}d\tau \right]+\frac{\partial}{\partial \tau}\left[\displaystyle\int_{\tau_0}^{\tau} \frac{1}{\tau}\frac{c_p^0}{R_u}d\tau \right]
\f]
Fundamental theorem of calculus
\f[
\alpha^0_{\tau} = -\int_{\tau_0}^{\tau} \frac{1}{\tau^2}\frac{c_p^0}{R_u}d\tau-\tau \frac{1}{\tau^2}\frac{c_p^0}{R_u}d\tau+\frac{1}{\tau}\frac{c_p^0}{R_u}
\f]
Last two terms cancel, leaving
\f[
\alpha^0_{\tau} = -\int_{\tau_0}^{\tau} \frac{1}{\tau^2}\frac{c_p^0}{R_u}d\tau
\f]
Another derivative yields (from fundamental theorem of calculus)
\f[
\alpha^0_{\tau\tau} = - \frac{1}{\tau^2}\frac{c_p^0}{R_u}
\f]

see also Jaeschke and Schley, 1995, (http://link.springer.com/article/10.1007%2FBF02083547#page-1)
*/
class IdealHelmholtzCP0AlyLee : public BaseHelmholtzTerm{
private:
    std::vector<long double> c;
    long double Tc, tau0, T0; // Use these variables internally
    bool enabled;
public:
    IdealHelmholtzCP0AlyLee(){enabled = false;};

    /// Constructor with std::vectors
    IdealHelmholtzCP0AlyLee(std::vector<long double> c, double Tc, double T0)
    :c(c), Tc(Tc), T0(T0)
    {
        tau0=Tc/T0;
        enabled = true;
    };

    /// Destructor
    ~IdealHelmholtzCP0AlyLee(){};

    bool is_enabled(){return enabled;};

    void to_json(rapidjson::Value &el, rapidjson::Document &doc);

    
    /// The antiderivative given by \f$ \displaystyle\int \frac{1}{\tau^2}\frac{c_p^0}{R_u}d\tau \f$
    /**
    sympy code for this derivative:

        from sympy import *
        a1,a2,a3,a4,a5,Tc,tau = symbols('a1,a2,a3,a4,a5,Tc,tau', real = True)
        integrand = a1 + a2*(a3/Tc/sinh(a3*tau/Tc))**2 + a4*(a5/Tc/cosh(a5*tau/Tc))**2
        integrand = integrand.rewrite(exp)
        antideriv = trigsimp(integrate(integrand,tau))
        display(antideriv)
        print latex(antideriv)
        print ccode(antideriv)

    \f[
    \displaystyle\int \frac{1}{\tau^2}\frac{c_p^0}{R_u}d\tau = -\frac{a_0}{\tau}+\frac{2a_1a_2}{T_c\left[\exp\left(-\frac{2a_2\tau}{T_c}\right)-1\right]}+\frac{2a_3a_4}{T_c\left[\exp\left(-\frac{2a_4\tau}{T_c}\right)+1\right]}
    \f]
    */
    long double anti_deriv_cp0_tau2(const long double &tau);

    /// The antiderivative given by \f$ \displaystyle\int \frac{1}{\tau}\frac{c_p^0}{R_u}d\tau \f$
    /**
    sympy code for this derivative:

        a_0,a_1,a_2,a_3,a_4,Tc,tau = symbols('a_0,a_1,a_2,a_3,a_4,Tc,tau', real = True)
        integrand = a_0/tau + a_1/tau*(a_2*tau/Tc/sinh(a_2*tau/Tc))**2 + a_3/tau*(a_4*tau/Tc/cosh(a_4*tau/Tc))**2

        term2 = a_1/tau*(a_2*tau/Tc/sinh(a_2*tau/Tc))**2
        term2 = term2.rewrite(exp)  # Unpack the sinh to exp functions
        antideriv2 = trigsimp(integrate(term2,tau))
        display(antideriv2)
        print latex(antideriv2)
        print ccode(antideriv2)

        term3 = a_3/tau*(a_4*tau/Tc/cosh(a_4*tau/Tc))**2
        term3 = term3.rewrite(exp)  # Unpack the cosh to exp functions
        antideriv3 = factor(trigsimp(integrate(term3,tau).rewrite(exp)))
        display(antideriv3)
        print latex(antideriv3)
        print ccode(antideriv3)

    Can be broken into three parts (trick is to express \f$sinh\f$ and \f$cosh\f$ in terms of \f$exp\f$ function)

    Term 2:
    \f[
    \displaystyle\int \frac{a_1a_2^2}{T_c^2}\frac{\tau}{\sinh\left(\displaystyle\frac{a_2\tau}{T_c}\right)^2} d\tau = \frac{2 a_{1} a_{2} \tau}{- Tc + Tc e^{- \frac{2 a_{2}}{Tc} \tau}} + a_{1} \log{\left (-1 + e^{- \frac{2 a_{2}}{Tc} \tau} \right )} + \frac{2 a_{1}}{Tc} a_{2} \tau
    \f]

    Term 3:
    \f[
    \displaystyle\int \frac{a_1a_2^2}{T_c^2}\frac{\tau}{\cosh\left(\displaystyle\frac{a_2\tau}{T_c}\right)^2} d\tau = - \frac{a_{3}}{Tc \left(e^{\frac{2 a_{4}}{Tc} \tau} + 1\right)} \left(Tc e^{\frac{2 a_{4}}{Tc} \tau} \log{\left (e^{\frac{2 a_{4}}{Tc} \tau} + 1 \right )} + Tc \log{\left (e^{\frac{2 a_{4}}{Tc} \tau} + 1 \right )} - 2 a_{4} \tau e^{\frac{2 a_{4}}{Tc} \tau}\right)
    \f]
    */
    long double anti_deriv_cp0_tau(const long double &tau);

    long double base(const long double &tau, const long double &delta) throw();
    long double dDelta(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dTau(const long double &tau, const long double &delta) throw();
    long double dDelta2(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta_dTau(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dTau2(const long double &tau, const long double &delta) throw();
    long double dDelta3(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta2_dTau(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dDelta_dTau2(const long double &tau, const long double &delta) throw(){return 0.0;};
    long double dTau3(const long double &tau, const long double &delta) throw();
    
};


class IdealHelmholtzContainer
{
    
public:
    IdealHelmholtzLead Lead;
    IdealHelmholtzEnthalpyEntropyOffset EnthalpyEntropyOffsetCore, EnthalpyEntropyOffset;
    IdealHelmholtzLogTau LogTau;
    IdealHelmholtzPower Power;
    IdealHelmholtzPlanckEinsteinGeneralized PlanckEinstein;

    IdealHelmholtzCP0Constant CP0Constant;
    IdealHelmholtzCP0PolyT CP0PolyT;

    long double base(const long double &tau, const long double &delta)
    {
        return (Lead.base(tau, delta) + EnthalpyEntropyOffset.base(tau, delta)
                + EnthalpyEntropyOffsetCore.base(tau, delta)
                + LogTau.base(tau, delta) + Power.base(tau, delta) 
                + PlanckEinstein.base(tau, delta)
                + CP0Constant.base(tau, delta) + CP0PolyT.base(tau, delta)
                );
    };
    long double dDelta(const long double &tau, const long double &delta)
    {
        return (Lead.dDelta(tau, delta) + EnthalpyEntropyOffset.dDelta(tau, delta)
                + EnthalpyEntropyOffsetCore.dDelta(tau, delta)
                + LogTau.dDelta(tau, delta) + Power.dDelta(tau, delta) 
                + PlanckEinstein.dDelta(tau, delta)
                + CP0Constant.dDelta(tau, delta) + CP0PolyT.dDelta(tau, delta)
                );
    };
    long double dTau(const long double &tau, const long double &delta)
    {
        return (Lead.dTau(tau, delta) + EnthalpyEntropyOffset.dTau(tau, delta)
                + EnthalpyEntropyOffsetCore.dTau(tau, delta)
                + LogTau.dTau(tau, delta) + Power.dTau(tau, delta) 
                + PlanckEinstein.dTau(tau, delta)
                + CP0Constant.dTau(tau, delta) + CP0PolyT.dTau(tau, delta)
                );
    };
    long double dDelta2(const long double &tau, const long double &delta)
    {
        return (Lead.dDelta2(tau, delta) + EnthalpyEntropyOffset.dDelta2(tau, delta)
                + EnthalpyEntropyOffsetCore.dDelta2(tau, delta)
                + LogTau.dDelta2(tau, delta) + Power.dDelta2(tau, delta) 
                + PlanckEinstein.dDelta2(tau, delta)
                + CP0Constant.dDelta2(tau, delta) + CP0PolyT.dDelta2(tau, delta)
                );
    };
    long double dDelta_dTau(const long double &tau, const long double &delta)
    {
        return (Lead.dDelta_dTau(tau, delta) + EnthalpyEntropyOffset.dDelta_dTau(tau, delta)
                + EnthalpyEntropyOffsetCore.dDelta_dTau(tau, delta)
                + LogTau.dDelta_dTau(tau, delta) + Power.dDelta_dTau(tau, delta) 
                + PlanckEinstein.dDelta_dTau(tau, delta)
                + CP0Constant.dDelta_dTau(tau, delta) + CP0PolyT.dDelta_dTau(tau, delta)
                );
    };
    long double dTau2(const long double &tau, const long double &delta)
    {
        return (Lead.dTau2(tau, delta) + EnthalpyEntropyOffset.dTau2(tau, delta)
                + EnthalpyEntropyOffsetCore.dTau2(tau, delta)
                + LogTau.dTau2(tau, delta) + Power.dTau2(tau, delta) 
                + PlanckEinstein.dTau2(tau, delta)
                + CP0Constant.dTau2(tau, delta) + CP0PolyT.dTau2(tau, delta)
                );
    };
    long double dDelta3(const long double &tau, const long double &delta) 
    {
        return (Lead.dDelta3(tau, delta) + EnthalpyEntropyOffset.dDelta3(tau, delta)
                + EnthalpyEntropyOffsetCore.dDelta3(tau, delta)
                + LogTau.dDelta3(tau, delta) + Power.dDelta3(tau, delta) 
                + PlanckEinstein.dDelta3(tau, delta)
                + CP0Constant.dDelta3(tau, delta) + CP0PolyT.dDelta3(tau, delta)
                );
    };
    long double dDelta2_dTau(const long double &tau, const long double &delta)
    {
        return (Lead.dDelta2_dTau(tau, delta) + EnthalpyEntropyOffset.dDelta2_dTau(tau, delta)
                + EnthalpyEntropyOffsetCore.dDelta2_dTau(tau, delta)
                + LogTau.dDelta2_dTau(tau, delta) + Power.dDelta2_dTau(tau, delta) 
                + PlanckEinstein.dDelta2_dTau(tau, delta)
                + CP0Constant.dDelta2_dTau(tau, delta) + CP0PolyT.dDelta2_dTau(tau, delta)
                );
    };
    long double dDelta_dTau2(const long double &tau, const long double &delta)
    {
        return (Lead.dDelta_dTau2(tau, delta) + EnthalpyEntropyOffset.dDelta_dTau2(tau, delta)
                + EnthalpyEntropyOffsetCore.dDelta_dTau2(tau, delta)
                + LogTau.dDelta_dTau2(tau, delta) + Power.dDelta_dTau2(tau, delta) 
                + PlanckEinstein.dDelta_dTau2(tau, delta)
                + CP0Constant.dDelta_dTau2(tau, delta) + CP0PolyT.dDelta_dTau2(tau, delta)
                );
    };
    long double dTau3(const long double &tau, const long double &delta)
    {
        return (Lead.dTau3(tau, delta) + EnthalpyEntropyOffset.dTau3(tau, delta)
                + EnthalpyEntropyOffsetCore.dTau3(tau, delta)
                + LogTau.dTau3(tau, delta) + Power.dTau3(tau, delta) 
                + PlanckEinstein.dTau3(tau, delta)
                + CP0Constant.dTau3(tau, delta) + CP0PolyT.dTau3(tau, delta)
                );
    };
};

};



#endif
