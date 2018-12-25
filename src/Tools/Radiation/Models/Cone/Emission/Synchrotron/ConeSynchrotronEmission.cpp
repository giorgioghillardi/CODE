/**
 * Module for evaluating synchrotron emission in
 * the cone model.
 */

#include "Tools/Radiation/synchrotron_func.h"
#include "Tools/Radiation/Models/Cone/ConeSynchrotronEmission.h"
#include "Tools/Radiation/RadiationParticle.h"

using namespace __Radiation;

/**
 * Calculates the total emission and/or spectrum and/or
 * Stokes parameters of synchrotron radiation.
 *
 * rp:           Object representing particle emitting state.
 * polarization: If true, calculate polarization components.
 */
void ConeSynchrotronEmission::HandleParticle(RadiationParticle *rp, bool polarization) {
    if (nwavelengths == 0)
        CalculateTotalEmission(rp);
    else {
        if (!polarization) {
            CalculateSpectrum(rp);
            IntegrateSpectrum();
        } else {
            CalculatePolarization(rp);
            IntegrateSpectrumStokes();
        }
    }
}

/**
 * Calculate total emission of synchrotron radiation.
 * This is equation (16) in the SOFT paper.
 * 
 * rp: Object representing the particle emitting state.
 */
void ConeSynchrotronEmission::CalculateTotalEmission(RadiationParticle *rp) {
    slibreal_t B = rp->GetB();
    slibreal_t gamma2 = rp->GetGamma() * rp->GetGamma();
    slibreal_t pperp2 = rp->GetPperp() * rp->GetPperp();
    slibreal_t ppar2 = rp->GetPpar() * rp->GetPpar();
    slibreal_t betaperp2 = pperp2 / gamma2;
    slibreal_t betapar2 = ppar2 / gamma2;
    slibreal_t beta = sqrt(betaperp2 + betapar2);
    slibreal_t gammapar2 = gamma2 / (1.0 + pperp2);
    const slibreal_t q = rp->GetCharge(),
                     c = LIGHTSPEED,
                     m = rp->GetMass();

    slibreal_t pf = q*q*q*q * B*B / (6.0*M_PI*EPS0*m*m*c);

    // Eq. (16) in [Hoppe et al., NF 58 026032 (2018)]
    this->power = pf * gamma2 * gammapar2 * betaperp2 * (1.0 - betapar2/beta);
}

/**
 * Calculate the synchrotron spectrum.
 *
 * rp: Object representing the particle emitting state.
 */
void ConeSynchrotronEmission::CalculateSpectrum(RadiationParticle *rp) {
    __CalculateSpectrum<false>(rp);
}

/**
 * Calculate the Stokes parameter of synchrotron radiation.
 * 
 * rp: Object representing the particle emitting state.
 */
void ConeSynchrotronEmission::CalculatePolarization(RadiationParticle *rp) {
    __CalculateSpectrum<true>(rp);
}

/**
 * Complete function for calculating synchrotron spectrum
 * and, if requested, the associated Stokes parameters.
 *
 * calculatePolarization: If true, calculates the polarization
 *                        components of synchrotron radiation.
 * rp:                    Object representing the particle
 *                        emitting state.
 */
template<bool calculatePolarization>
void ConeSynchrotronEmission::__CalculateSpectrum(RadiationParticle *rp) {
    unsigned int i;
    slibreal_t gamma2 = rp->GetGamma() * rp->GetGamma(),
        ppar2 = rp->GetPpar() * rp->GetPpar(),
        pperp2 = rp->GetPperp() * rp->GetPperp(),
        betapar2 = ppar2 / gamma2,
        betaperp2 = pperp2 / gamma2,
        beta = sqrt(betapar2+betaperp2),
        gammapar2 = gamma2 / (1.0 + pperp2),
        gammapar = sqrt(gammapar2),
        prefactorpol, betaparpar, sinb, cosb;

    slibreal_t c = LIGHTSPEED, e = fabs(rp->GetCharge()), l, lcl,
        m = rp->GetMass(), B = rp->GetB(),
        pf = c*e*e / (sqrt(3.0)*EPS0*gamma2) * gammapar2 * (1.0 - betapar2/beta),
        lc = 4.0*M_PI*m*c*gammapar / (3*gamma2*e*B), ikf;

    if (calculatePolarization) {
        Vector<3> &e1 = detector->GetEHat1(),
                  &e2 = detector->GetEHat2(),
                  &nhat = detector->GetDirection(),
                  rcp = rp->GetRCP(),
                  &phat = rp->GetPHat();

        rcp.Normalize();

        slibreal_t
            nb = rcp.Dot(phat),
            nb2 = nb*nb,
            divfac = 1.0 / sqrt(1.0-nb2),
            nDotNhat = rcp.Dot(nhat);

        cosb =-divfac * (
            e1[0] * (phat[1]*rcp[2] - phat[2]*rcp[1]) +
            e1[1] * (phat[2]*rcp[0] - phat[0]*rcp[2]) +
            e1[2] * (phat[0]*rcp[1] - phat[1]*rcp[0])
        ) / sqrt(1-nDotNhat*nDotNhat);

        sinb = divfac * (
            e2[0] * (phat[1]*rcp[2] - phat[2]*rcp[1]) +
            e2[1] * (phat[2]*rcp[0] - phat[0]*rcp[2]) +
            e2[2] * (phat[0]*rcp[1] - phat[1]*rcp[0])
        ) / sqrt(1-nDotNhat*nDotNhat);

        betaparpar = ppar2 / sqrt(gamma2*gamma2 - gamma2);
        prefactorpol = 
            9.0*sqrt(3.0)*e*e*e*e*e / (64.0*M_PI*M_PI*M_PI*m*m*m*c*c*EPS0)*
            gamma2*gamma2 * B*B*B * (1-betaparpar) / gammapar;
    }

    // Evaluate spectrum at each wavelength
    for (i = 0; i < nwavelengths; i++) {
        l = wavelengths[i];
        lcl = lc/l;

        ikf = synchrotron_func1(lcl);
        I[i] = (pf/(l*l*lc)) * ikf;

        if (calculatePolarization) {
            slibreal_t k, Apar2, Aperp2;

            k = prefactorpol * (lcl*lcl) * synchrotron_func2(lcl);
            Apar2  = 0.5 * (ikf + k);
            Aperp2 = 0.5 * (ikf - k);

            U[i] = (Aperp2 - Apar2) * (sinb*sinb - cosb*cosb);
            Q[i] = 2.0 * (Aperp2 - Apar2) * sinb*cosb;

            V[i] = 0.0;
        }
    }
}

/**
 * Integrates the synchrotron spectrum to produce a
 * total emitted power in a given spectral range.
 */
void ConeSynchrotronEmission::IntegrateSpectrum() {
    unsigned int i;
    slibreal_t s = 0.5*(I[0] + I[nwavelengths-1]);

    for (i = 1; i < nwavelengths-1; i++)
        s += I[i];

    this->power = s * (wavelengths[1]-wavelengths[0]);
}

void ConeSynchrotronEmission::IntegrateSpectrumStokes() {
    unsigned x;
    slibreal_t i=0, q=0, u=0;

    for (x = 1; x < nwavelengths-1; x++)
        i += I[x];
    for (x = 1; x < nwavelengths-1; x++)
        q += Q[x];
    for (x = 1; x < nwavelengths-1; x++)
        u += U[x];

    this->power = i;
    this->totQ  = q;
    this->totU  = u;
    this->totV  = 0;
}

