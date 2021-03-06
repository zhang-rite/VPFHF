#include "petsc.h"
#include "VFCartFE.h"
#include "VFCommon.h"
#include "VFMech.h"

#define UNILATERAL_THRES 0
/*
  use UNILATERAL_THRES -1e+10 in order to test the tensile part of the energy (equivalent to unilateral none)
  use UNILATERAL_THRES  1e+10 in order to test the compressive part of the energy (equivalent to the defunct shear only)
*/

#undef __FUNCT__
#define __FUNCT__ "ElasticEnergyDensity3D_local"
/*
 ElasticEnergyDensity3D_local: computes the local contribution of the elastic energy desity
 at each Gauss point.
 
 The elastic energy density is defined as
 1/2 A \epsilon : \epsilon
 where epsilon is the inelastic strain:
 \epsilon = e(u) - \alpha (\theta-\theta_0) Id - \beta (p-p_0) A^{-1}Id
 = e(u) - \alpha (\theta-\theta_0) Id - \beta (p-p_0) / (3\lambda + 2\u) Id
 and
 * ":" denotes tensor contraction, i.e. M:N = tr M^tN
 * A is the Hooke's law, \lambda and \mu are the lam\'e coefficients
 * \alpha is the thermal expansion coefficient
 * \beta is the Biot coefficient
 * the index "0" denotes a reference field (temperature or pressure)
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode ElasticEnergyDensity3D_local(PetscReal *ElasticEnergyDensity_local,PetscReal ****u_array,PetscReal ***theta_array,PetscReal ***thetaRef_array,VFMatProp *matprop,PetscInt ek,PetscInt ej,PetscInt ei,VFCartFEElement3D *e)
{
  PetscErrorCode ierr;
  PetscReal      *epsilon11_elem,*epsilon22_elem,*epsilon33_elem,*epsilon12_elem,*epsilon23_elem,*epsilon13_elem;
  PetscReal      *sigma11_elem,*sigma22_elem,*sigma33_elem,*sigma12_elem,*sigma23_elem,*sigma13_elem;
  PetscInt       i,j,k,g;
  PetscReal      lambda,mu,alpha;
  /*
   PetscReal      myElasticEnergyDensity = 0;
   */
  
  PetscFunctionBegin;
  lambda   = matprop->lambda;
  mu       = matprop->mu;
  alpha    = matprop->alpha;
  
  ierr = PetscMalloc3(e->ng,&sigma11_elem,e->ng,&sigma22_elem,e->ng,&sigma33_elem);CHKERRQ(ierr);
  ierr = PetscMalloc3(e->ng,&sigma12_elem,e->ng,&sigma23_elem,e->ng,&sigma13_elem);CHKERRQ(ierr);
  ierr = PetscMalloc3(e->ng,&epsilon11_elem,e->ng,&epsilon22_elem,e->ng,&epsilon33_elem);CHKERRQ(ierr);
  ierr = PetscMalloc3(e->ng,&epsilon12_elem,e->ng,&epsilon23_elem,e->ng,&epsilon13_elem);CHKERRQ(ierr);
  
  for (g = 0; g < e->ng; g++) {
    epsilon11_elem[g]             = 0;
    epsilon22_elem[g]             = 0;
    epsilon33_elem[g]             = 0;
    epsilon12_elem[g]             = 0;
    epsilon23_elem[g]             = 0;
    epsilon13_elem[g]             = 0;
    ElasticEnergyDensity_local[g] = 0.;
  }
  for (k = 0; k < e->nphiz; k++) {
    for (j = 0; j < e->nphiy; j++) {
      for (i = 0; i < e->nphix; i++) {
        for (g = 0; g < e->ng; g++) {
          epsilon11_elem[g] += e->dphi[k][j][i][0][g] * u_array[ek+k][ej+j][ei+i][0]
                            - alpha * e->phi[k][j][i][g] * (theta_array[ek+k][ej+j][ei+i]-thetaRef_array[ek+k][ej+j][ei+i]);
          epsilon22_elem[g] += e->dphi[k][j][i][1][g] * u_array[ek+k][ej+j][ei+i][1]
                             - alpha * e->phi[k][j][i][g] * (theta_array[ek+k][ej+j][ei+i]-thetaRef_array[ek+k][ej+j][ei+i]);
          epsilon33_elem[g] += e->dphi[k][j][i][2][g] * u_array[ek+k][ej+j][ei+i][2]
                             - alpha * e->phi[k][j][i][g] * (theta_array[ek+k][ej+j][ei+i]-thetaRef_array[ek+k][ej+j][ei+i]);
          
          epsilon12_elem[g] += (e->dphi[k][j][i][0][g] * u_array[ek+k][ej+j][ei+i][1]
                                + e->dphi[k][j][i][1][g] * u_array[ek+k][ej+j][ei+i][0]) * .5;
          epsilon23_elem[g] += (e->dphi[k][j][i][1][g] * u_array[ek+k][ej+j][ei+i][2]
                                + e->dphi[k][j][i][2][g] * u_array[ek+k][ej+j][ei+i][1]) * .5;
          epsilon13_elem[g] += (e->dphi[k][j][i][0][g] * u_array[ek+k][ej+j][ei+i][2]
                                + e->dphi[k][j][i][2][g] * u_array[ek+k][ej+j][ei+i][0]) * .5;
        }
      }
    }
  }
  ierr = PetscLogFlops(33 * e->ng * e->nphix * e->nphiy * e->nphiz);CHKERRQ(ierr);
  
  for (g = 0; g < e->ng; g++) {
    sigma11_elem[g]               = (lambda + 2.*mu) * epsilon11_elem[g] + lambda * epsilon22_elem[g] + lambda * epsilon33_elem[g];
    sigma22_elem[g]               = lambda * epsilon11_elem[g] + (lambda + 2.*mu) * epsilon22_elem[g] + lambda * epsilon33_elem[g];
    sigma33_elem[g]               = lambda * epsilon11_elem[g] + lambda * epsilon22_elem[g] + (lambda + 2.*mu) * epsilon33_elem[g];
    sigma12_elem[g]               = 2. * mu * epsilon12_elem[g];
    sigma23_elem[g]               = 2. * mu * epsilon23_elem[g];
    sigma13_elem[g]               = 2. * mu * epsilon13_elem[g];
    ElasticEnergyDensity_local[g] = (sigma11_elem[g] * epsilon11_elem[g]
                                     + sigma22_elem[g] * epsilon22_elem[g]
                                     + sigma33_elem[g] * epsilon33_elem[g]) * .5
                                    + sigma12_elem[g] * epsilon12_elem[g]
                                    + sigma23_elem[g] * epsilon23_elem[g]
                                    + sigma13_elem[g] * epsilon13_elem[g];
  }
  ierr = PetscLogFlops(46 * e->ng);CHKERRQ(ierr);
  
  ierr = PetscFree3(sigma11_elem,sigma22_elem,sigma33_elem);CHKERRQ(ierr);
  ierr = PetscFree3(sigma12_elem,sigma23_elem,sigma13_elem);CHKERRQ(ierr);
  ierr = PetscFree3(epsilon11_elem,epsilon22_elem,epsilon33_elem);CHKERRQ(ierr);
  ierr = PetscFree3(epsilon12_elem,epsilon23_elem,epsilon13_elem);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ElasticEnergyDensitySphericalDeviatoricNoCompression3D_local"
/*
 ElasticEnergyDensitySphericalDeviatoricNoCompression3D_local
 Compute the spherical and positive deviatoric parts of the elastic energy density in an element
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode ElasticEnergyDensitySphericalDeviatoricNoCompression3D_local(PetscReal *ElasticEnergyDensityS_local,PetscReal *ElasticEnergyDensityD_local,PetscReal ****u_array,PetscReal ***theta_array,PetscReal ***thetaRef_array,VFMatProp *matprop,PetscInt ek,PetscInt ej,PetscInt ei,VFCartFEElement3D *e)
{
  PetscErrorCode ierr;
  PetscReal      *epsilon11_elem,*epsilon22_elem,*epsilon33_elem,*epsilon12_elem,*epsilon23_elem,*epsilon13_elem;
  PetscReal      *e0_11_elem,*e0_22_elem,*e0_33_elem;
  PetscReal      *sigma11_elem,*sigma22_elem,*sigma33_elem,*sigma12_elem,*sigma23_elem,*sigma13_elem;
  PetscInt       i,j,k,g;
  PetscReal      lambda,mu,alpha,kappa;
  PetscReal      *ElasticEnergyDensity_local;
  PetscReal      tr_epsilon;
  
  PetscFunctionBegin;
  lambda   = matprop->lambda;
  mu       = matprop->mu;
  alpha    = matprop->alpha;
  kappa    = lambda + 2.* mu / 3.;

  ierr = PetscMalloc3(e->ng,&sigma11_elem,e->ng,&sigma22_elem,e->ng,&sigma33_elem);CHKERRQ(ierr);
  ierr = PetscMalloc3(e->ng,&sigma12_elem,e->ng,&sigma23_elem,e->ng,&sigma13_elem);CHKERRQ(ierr);
  ierr = PetscMalloc3(e->ng,&epsilon11_elem,e->ng,&epsilon22_elem,e->ng,&epsilon33_elem);CHKERRQ(ierr);
  ierr = PetscMalloc3(e->ng,&epsilon12_elem,e->ng,&epsilon23_elem,e->ng,&epsilon13_elem);CHKERRQ(ierr);
  ierr = PetscMalloc3(e->ng,&e0_11_elem,e->ng,&e0_22_elem,e->ng,&e0_33_elem);CHKERRQ(ierr);
  
  ierr = PetscMalloc(e->ng * sizeof(PetscReal),&ElasticEnergyDensity_local);CHKERRQ(ierr);
  for (g = 0; g < e->ng; g++) {
    epsilon11_elem[g] = 0;
    epsilon22_elem[g] = 0;
    epsilon33_elem[g] = 0;
    epsilon12_elem[g] = 0;
    epsilon23_elem[g] = 0;
    epsilon13_elem[g] = 0;
    
    e0_11_elem[g] = 0;
    e0_22_elem[g] = 0;
    e0_33_elem[g] = 0;
    
    ElasticEnergyDensity_local[g]  = 0.;
    ElasticEnergyDensityS_local[g] = 0.;
    ElasticEnergyDensityD_local[g] = 0.;
  }
  for (k = 0; k < e->nphiz; k++) {
    for (j = 0; j < e->nphiy; j++) {
      for (i = 0; i < e->nphix; i++) {
        for (g = 0; g < e->ng; g++) {
          /* strain at Gauss points */
          epsilon11_elem[g] += e->dphi[k][j][i][0][g] * u_array[ek+k][ej+j][ei+i][0];
          epsilon22_elem[g] += e->dphi[k][j][i][1][g] * u_array[ek+k][ej+j][ei+i][1];
          epsilon33_elem[g] += e->dphi[k][j][i][2][g] * u_array[ek+k][ej+j][ei+i][2];
          epsilon12_elem[g] += (e->dphi[k][j][i][0][g] * u_array[ek+k][ej+j][ei+i][1]
                                + e->dphi[k][j][i][1][g] * u_array[ek+k][ej+j][ei+i][0]) * .5;
          epsilon23_elem[g] += (e->dphi[k][j][i][1][g] * u_array[ek+k][ej+j][ei+i][2]
                                + e->dphi[k][j][i][2][g] * u_array[ek+k][ej+j][ei+i][1]) * .5;
          epsilon13_elem[g] += (e->dphi[k][j][i][0][g] * u_array[ek+k][ej+j][ei+i][2]
                                + e->dphi[k][j][i][2][g] * u_array[ek+k][ej+j][ei+i][0]) * .5;

          /* inelastic strain at Gauss points */
          e0_11_elem[g]     += alpha * e->phi[k][j][i][g] * (theta_array[ek+k][ej+j][ei+i] - thetaRef_array[ek+k][ej+j][ei+i]);
          e0_22_elem[g]     += alpha * e->phi[k][j][i][g] * (theta_array[ek+k][ej+j][ei+i] - thetaRef_array[ek+k][ej+j][ei+i]);
          e0_33_elem[g]     += alpha * e->phi[k][j][i][g] * (theta_array[ek+k][ej+j][ei+i] - thetaRef_array[ek+k][ej+j][ei+i]);
          
        }
      }
    }
  }
  ierr = PetscLogFlops(33 * e->ng * e->nphix * e->nphiy * e->nphiz);CHKERRQ(ierr);
  
  for (g = 0; g < e->ng; g++) {
    sigma11_elem[g]               = (lambda + 2.*mu) * (epsilon11_elem[g] - e0_11_elem[g]) 
                                  + lambda * (epsilon22_elem[g] - e0_22_elem[g]) 
                                  + lambda * (epsilon33_elem[g] - e0_33_elem[g]);
    sigma22_elem[g]               = lambda * (epsilon11_elem[g] - e0_11_elem[g]) 
                                  + (lambda + 2.*mu) * (epsilon22_elem[g] - e0_22_elem[g]) 
                                  + lambda * (epsilon33_elem[g] - e0_33_elem[g]);
    sigma33_elem[g]               = lambda * (epsilon11_elem[g] - e0_11_elem[g]) 
                                  + lambda * (epsilon22_elem[g] - e0_22_elem[g]) 
                                  + (lambda + 2.*mu) * (epsilon33_elem[g] - e0_33_elem[g]);
    sigma12_elem[g]               = 2.*mu * epsilon12_elem[g];
    sigma23_elem[g]               = 2.*mu * epsilon23_elem[g];
    sigma13_elem[g]               = 2.*mu * epsilon13_elem[g];
    ElasticEnergyDensity_local[g] = (sigma11_elem[g] * epsilon11_elem[g]
                                   + sigma22_elem[g] * epsilon22_elem[g]
                                   + sigma33_elem[g] * epsilon33_elem[g]) * .5
                                   + sigma12_elem[g] * epsilon12_elem[g]
                                   + sigma23_elem[g] * epsilon23_elem[g]
                                   + sigma13_elem[g] * epsilon13_elem[g];
    tr_epsilon = epsilon11_elem[g] + epsilon22_elem[g] + epsilon33_elem[g];
    
    if (tr_epsilon < UNILATERAL_THRES) {
      ElasticEnergyDensityS_local[g] = 0.;
    } else {
      ElasticEnergyDensityS_local[g] = kappa * tr_epsilon * tr_epsilon * .5;
    }
    ElasticEnergyDensityD_local[g] = ElasticEnergyDensity_local[g] - kappa * tr_epsilon * tr_epsilon * .5;
  }
  ierr = PetscLogFlops(54 * e->ng);CHKERRQ(ierr);
  
  ierr = PetscFree3(sigma11_elem,sigma22_elem,sigma33_elem);CHKERRQ(ierr);
  ierr = PetscFree3(sigma12_elem,sigma23_elem,sigma13_elem);CHKERRQ(ierr);
  ierr = PetscFree3(epsilon11_elem,epsilon22_elem,epsilon33_elem);CHKERRQ(ierr);
  ierr = PetscFree3(epsilon12_elem,epsilon23_elem,epsilon13_elem);CHKERRQ(ierr);
  ierr = PetscFree3(e0_11_elem,e0_22_elem,e0_33_elem);CHKERRQ(ierr);
  ierr = PetscFree(ElasticEnergyDensity_local);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_BilinearFormU3D_local"
/*
 VF_BilinearFormU3D_local
 
 (c) 2010-2014 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_BilinearFormU3D_local(PetscReal *Mat_local,PetscReal ***v_array,VFMatProp *matprop,VFProp *vfprop,PetscInt ek,PetscInt ej,PetscInt ei,VFCartFEElement3D *e)
{
  PetscInt       g,i1,i2,j1,j2,k1,k2,c1,c2,l;
  PetscReal      *s_elem;
  PetscReal       mat_gauss;
  PetscErrorCode ierr;
  
  PetscFunctionBegin;
  ierr   = PetscMalloc(e->ng * sizeof(PetscReal),&s_elem);CHKERRQ(ierr);

  /*
    s_elem is the material's compliance multiplicator (s(v) = v^2+\eta)
  */
  for (g = 0; g < e->ng; g++) s_elem[g] = 0.;
  for (k1 = 0; k1 < e->nphiz; k1++) {
    for (j1 = 0; j1 < e->nphiy; j1++) {
      for (i1 = 0; i1 < e->nphix; i1++) {
        for (g = 0; g < e->ng; g++) {
          s_elem[g] += v_array[ek+k1][ej+j1][ei+i1] * e->phi[k1][j1][i1][g];
        }
      }
    }
  }
  for (g = 0; g < e->ng; g++) {
    s_elem[g] = s_elem[g] * s_elem[g] + vfprop->eta;
  }
  ierr = PetscLogFlops(2 * e->ng * (1. + e->nphix * e->nphiy * e->nphiz));CHKERRQ(ierr);
  
  for (l = 0; l < 9 * e->nphix * e->nphiy * e->nphiz * e->nphix * e->nphiy * e->nphiz; l++) {
  Mat_local[l] = 0;
  }
  for (l = 0,k1 = 0; k1 < e->nphiz; k1++) {
    for (j1 = 0; j1 < e->nphiy; j1++) {
      for (i1 = 0; i1 < e->nphix; i1++) {
        for (c1 = 0; c1 < e->dim; c1++) {
          for (k2 = 0; k2 < e->nphiz; k2++) {
            for (j2 = 0; j2 < e->nphiy; j2++) {
              for (i2 = 0; i2 < e->nphix; i2++) {
                for (c2 = 0; c2 < e->dim; c2++,l++) {
                  for (g = 0; g < e->ng; g++){
                    mat_gauss = matprop->lambda * s_elem[g] * e->dphi[k1][j1][i1][c1][g] * e->dphi[k2][j2][i2][c2][g];
                    mat_gauss += matprop->mu * s_elem[g] * e->dphi[k1][j1][i1][c2][g] * e->dphi[k2][j2][i2][c1][g];
                    /* 7 flops */
                    if (c1 == c2) {
                      mat_gauss += matprop->mu * s_elem[g] * 
                                    (e->dphi[k1][j1][i1][0][g] * e->dphi[k2][j2][i2][0][g] + 
                                     e->dphi[k1][j1][i1][1][g] * e->dphi[k2][j2][i2][1][g] + 
                                     e->dphi[k1][j1][i1][2][g] * e->dphi[k2][j2][i2][2][g]);
                      /* 8 flops */
                    }
                  Mat_local[l] += e->weight[g] * mat_gauss;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  ierr = PetscLogFlops(e->nphix * e->nphiy * e->nphiz * e->nphix * e->nphiy * e->nphiz * (9 + e->ng * (9*7+3*8)));CHKERRQ(ierr);  
  ierr = PetscFree(s_elem);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_BilinearFormUNoCompression3D_local"
/*
 VF_BilinearFormUNoCompression3D_local
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_BilinearFormUNoCompression3D_local(PetscReal *Mat_local,PetscReal ****u_array,PetscReal ***v_array,VFMatProp *matprop,VFProp *vfprop,PetscInt ek,PetscInt ej,PetscInt ei,VFCartFEElement3D *e)
{
  PetscInt       g,i1,i2,j1,j2,k1,k2,c1,c2,l;
  PetscReal      kappa;
  PetscReal      *s_elem,*tr_epsilon;
  PetscReal      mat_gauss;
  PetscErrorCode ierr;
  
  PetscFunctionBegin;
  ierr   = PetscMalloc2(e->ng,&s_elem,
                        e->ng,&tr_epsilon);CHKERRQ(ierr);
  kappa  = matprop->lambda + 2. * matprop->mu / 3.;
  
  for (g = 0; g < e->ng; g++) {
    s_elem[g] = 0.;
    tr_epsilon[g] = 0;
  }
  for (k1 = 0; k1 < e->nphiz; k1++) {
    for (j1 = 0; j1 < e->nphiy; j1++) {
      for (i1 = 0; i1 < e->nphix; i1++) {
        for (g = 0; g < e->ng; g++) {
          s_elem[g]     += v_array[ek+k1][ej+j1][ei+i1] * e->phi[k1][j1][i1][g];
          tr_epsilon[g] += e->dphi[k1][j1][i1][0][g] * u_array[ek+k1][ej+j1][ei+i1][0] +
                           e->dphi[k1][j1][i1][1][g] * u_array[ek+k1][ej+j1][ei+i1][1] +
                           e->dphi[k1][j1][i1][2][g] * u_array[ek+k1][ej+j1][ei+i1][2];         
        }
      }
    }
  }
  for (g = 0; g < e->ng; g++) {
    s_elem[g] = s_elem[g] * s_elem[g] + vfprop->eta;
  }
  ierr = PetscLogFlops(e->ng * (2 + 8 * e->nphix * e->nphiy * e->nphiz));CHKERRQ(ierr);
    
  for (l = 0; l < 9 * e->nphix * e->nphiy * e->nphiz * e->nphix * e->nphiy * e->nphiz; l++) {
  Mat_local[l] = 0;
  }
  for (l = 0,k1 = 0; k1 < e->nphiz; k1++) {
    for (j1 = 0; j1 < e->nphiy; j1++) {
      for (i1 = 0; i1 < e->nphix; i1++) {
        for (c1 = 0; c1 < e->dim; c1++) {
          for (k2 = 0; k2 < e->nphiz; k2++) {
            for (j2 = 0; j2 < e->nphiy; j2++) {
              for (i2 = 0; i2 < e->nphix; i2++) {
                for (c2 = 0; c2 < e->dim; c2++,l++) {
                  for (g = 0; g < e->ng; g++){
                    /* spherical part */
                    if (tr_epsilon[g] < UNILATERAL_THRES) {
                      mat_gauss = kappa * (1. + vfprop->eta) * e->dphi[k1][j1][i1][c1][g] * e->dphi[k2][j2][i2][c2][g];
                    } else {
                      mat_gauss = kappa * s_elem[g] * e->dphi[k1][j1][i1][c1][g] * e->dphi[k2][j2][i2][c2][g];
                    }
                    /* deviatoric part */
                    mat_gauss -= 2. * matprop->mu * s_elem[g] * e->dphi[k1][j1][i1][c1][g] * e->dphi[k2][j2][i2][c2][g] / 3.;
                    mat_gauss += matprop->mu * s_elem[g] * e->dphi[k1][j1][i1][c2][g] * e->dphi[k2][j2][i2][c1][g];
                    /* 13 flops */
                    if (c1 == c2) {
                      mat_gauss += matprop->mu * s_elem[g] * 
                                    (e->dphi[k1][j1][i1][0][g] * e->dphi[k2][j2][i2][0][g] + 
                                     e->dphi[k1][j1][i1][1][g] * e->dphi[k2][j2][i2][1][g] + 
                                     e->dphi[k1][j1][i1][2][g] * e->dphi[k2][j2][i2][2][g]);
                      /* 8 flops */
                    }
                  Mat_local[l] += e->weight[g] * mat_gauss;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  ierr = PetscLogFlops(e->nphix * e->nphiy * e->nphiz * e->nphix * e->nphiy * e->nphiz * (9 + e->ng * (13*9+8*3)));CHKERRQ(ierr);  

  ierr = PetscFree2(s_elem,tr_epsilon);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VF_GradientUThermoPoro3D_local"
/*
 VF_GradientUThermoPoro3D_local
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_GradientUThermoPoro3D_local(PetscReal *residual_local,PetscReal ***v_array,PetscReal ***theta_array,PetscReal ***thetaRef_array,PetscReal ***pressure_array,VFMatProp *matprop,VFProp *vfprop,PetscInt ek,PetscInt ej,PetscInt ei,VFCartFEElement3D *e)
{
  PetscErrorCode ierr;
  PetscInt       l,i,j,k,g,c;
  PetscInt       dim=3;
  PetscReal      *theta_elem,*pressure_elem,*v_elem;
  PetscReal      coefalpha,beta;
  
  PetscFunctionBegin;
  coefalpha = (3.* matprop->lambda + 2. * matprop->mu) * matprop->alpha;
  beta      = matprop->beta;
  
  ierr = PetscMalloc3(e->ng,&theta_elem,
                      e->ng,&pressure_elem,
                      e->ng,&v_elem);CHKERRQ(ierr);
  /*
   Initialize pressure_Elem, theta_Elem and v_elem
   */
  for (g = 0; g < e->ng; g++) {
    theta_elem[g]    = 0;
    pressure_elem[g] = 0;
    v_elem[g]        = 0.;
  }
  /*
   Compute theta_elem
   */
  for (g = 0; g < e->ng; g++) {
    for (k = 0; k < e->nphiz; k++) {
      for (j = 0; j < e->nphiy; j++) {
        for (i = 0; i < e->nphix; i++) {
          theta_elem[g] += e->phi[k][j][i][g]
          * (theta_array[ek+k][ej+j][ei+i] - thetaRef_array[ek+k][ej+j][ei+i]);
          pressure_elem[g] += e->phi[k][j][i][g] * pressure_array[ek+k][ej+j][ei+i];
          v_elem[g] += e->phi[k][j][i][g] * v_array[ek+k][ej+j][ei+i];
        }
      }
    }
  }
  ierr = PetscLogFlops(8 * e->ng * e->nphix * e->nphiy * e->nphiz);CHKERRQ(ierr);
  /*
   Accumulate the contribution of the current element to the local
   version of the RHS
   */
  for (g = 0; g < e->ng; g++) {
    for (l = 0,k = 0; k < e->nphiz; k++) {
      for (j = 0; j < e->nphiy; j++) {
        for (i = 0; i < e->nphix; i++) {
          for (c = 0; c < dim; c++,l++) {
            residual_local[l] += e->weight[g] * e->dphi[k][j][i][c][g] 
                                 * (coefalpha * theta_elem[g] * (v_elem[g] * v_elem[g] + vfprop->eta) + beta * pressure_elem[g] * v_elem[g]);
          }
        }
      }
    }
  }
  ierr = PetscLogFlops(10 * dim * e->ng * e->nphix * e->nphiy * e->nphiz);CHKERRQ(ierr);
  /*
   Clean up
   */
  ierr = PetscFree3(theta_elem,pressure_elem,v_elem);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_GradientUThermoPoroNoCompression3D_local"
/*
 VF_GradientUThermoPoroNoCompression3D_local
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_GradientUThermoPoroNoCompression3D_local(PetscReal *residual_local,PetscReal ****u_array,PetscReal ***v_array,PetscReal ***theta_array,PetscReal ***thetaRef_array,PetscReal ***pressure_array,VFMatProp *matprop,VFProp *vfprop,PetscInt ek,PetscInt ej,PetscInt ei,VFCartFEElement3D *e)
{
  PetscErrorCode ierr;
  PetscInt       l,i,j,k,g,c;
  PetscInt       dim=3;
  PetscReal      *theta_elem,*pressure_elem,*v_elem,*tr_epsilon;
  PetscReal      coefalpha,beta;
  
  PetscFunctionBegin;
  coefalpha = (3.* matprop->lambda + 2. * matprop->mu) * matprop->alpha;
  beta      = matprop->beta;
  ierr      = PetscMalloc4(e->ng,&theta_elem,
                           e->ng,&pressure_elem,
                           e->ng,&v_elem,
                           e->ng,&tr_epsilon);CHKERRQ(ierr);
  /*
   Initialize theta_Elem and v_elem
   */
  for (g = 0; g < e->ng; g++) {
    theta_elem[g]    = 0;
    pressure_elem[g] = 0;
    tr_epsilon[g]    = 0;
    v_elem[g]        = 0;
  }
  /*
   Compute theta_elem
   */
  for (k = 0; k < e->nphiz; k++) {
    for (j = 0; j < e->nphiy; j++) {
      for (i = 0; i < e->nphix; i++) {
        for (g = 0; g < e->ng; g++) {
          theta_elem[g] += e->phi[k][j][i][g] * (theta_array[ek+k][ej+j][ei+i] - thetaRef_array[ek+k][ej+j][ei+i]);
          pressure_elem[g] += e->phi[k][j][i][g] * pressure_array[ek+k][ej+j][ei+i];
          v_elem[g] += e->phi[k][j][i][g] * v_array[ek+k][ej+j][ei+i];
          tr_epsilon[g] += e->dphi[k][j][i][0][g] * u_array[ek+k][ej+j][ei+i][0] +
                           e->dphi[k][j][i][1][g] * u_array[ek+k][ej+j][ei+i][1] +
                           e->dphi[k][j][i][2][g] * u_array[ek+k][ej+j][ei+i][2];         
        }
      }
    }
  }
  ierr = PetscLogFlops(14 * e->ng * e->nphix * e->nphiy * e->nphiz);CHKERRQ(ierr);
  /*
   Accumulate the contribution of the current element to the local version of the RHS.
   */
  for (g = 0; g < e->ng; g++) {
    for (l= 0,k = 0; k < e->nphiz; k++) {
      for (j = 0; j < e->nphiy; j++) {
        for (i = 0; i < e->nphix; i++) {
          for (c = 0; c < dim; c++,l++) {
            if (tr_epsilon[g] < UNILATERAL_THRES) {
              residual_local[l] += e->weight[g] * e->dphi[k][j][i][c][g] 
                                   * (coefalpha * theta_elem[g] * (1. + vfprop->eta) + beta * pressure_elem[g]);
            } else {
              residual_local[l] += e->weight[g] * e->dphi[k][j][i][c][g] 
                                   * (coefalpha * theta_elem[g] * (v_elem[g] * v_elem[g] + vfprop->eta) + beta * pressure_elem[g] * v_elem[g]);
            }
          }
        }
      }
    }
  }
  ierr = PetscLogFlops(6 * dim * e->ng * e->nphix * e->nphiy * e->nphiz);CHKERRQ(ierr);
  /*
   Clean up
   */
  ierr = PetscFree4(theta_elem,pressure_elem,v_elem,tr_epsilon);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_GradientUCrackPressure3D_local"
/*
 VF_GradientUCrackPressure3D_local
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_GradientUCrackPressure3D_local(PetscReal *residual_local,PetscReal ***v_array,PetscReal ***pressure_array,VFMatProp *matprop,VFProp *vfprop,PetscInt ek,PetscInt ej,PetscInt ei,VFCartFEElement3D *e)
{
  PetscErrorCode ierr;
  PetscInt       l,i,j,k,g,c;
  PetscReal      *pressure_elem,*gradv_elem[3];
  
  PetscFunctionBegin;
  ierr = PetscMalloc4(e->ng,&pressure_elem,
                      e->ng,&gradv_elem[0],
                      e->ng,&gradv_elem[1],
                      e->ng,&gradv_elem[2]);CHKERRQ(ierr);
  
  /*
   Compute the projection of the fields in the local base functions basis
   */
  for (g = 0; g < e->ng; g++) {
    pressure_elem[g] = 0;
    for (c= 0; c<3; c++) gradv_elem[c][g] = 0.;
  }
  for (k = 0; k < e->nphiz; k++) {
    for (j = 0; j < e->nphiy; j++) {
      for (i = 0; i < e->nphix; i++) {
        for (g = 0; g < e->ng; g++) {
          pressure_elem[g] += e->phi[k][j][i][g] * pressure_array[ek+k][ej+j][ei+i];
          for (c= 0; c<3; c++) {
            gradv_elem[c][g] += e->dphi[k][j][i][c][g] * v_array[ek+k][ej+j][ei+i];
          }
        }
      }
    }
  }
  ierr = PetscLogFlops(9 * e->ng * e->nphix * e->nphiy * e->nphiz);CHKERRQ(ierr);
  
  /*
   Accumulate the contribution of the current element to the local
   version of the RHS
   */
  for (l= 0,k = 0; k < e->nphiz; k++) {
    for (j = 0; j < e->nphiy; j++) {
      for (i = 0; i < e->nphix; i++) {
        for (c = 0; c < 3; c++,l++) {
          for (g = 0; g < e->ng; g++) {
            residual_local[l] += e->weight[g] * pressure_elem[g]
            * gradv_elem[c][g]
            * e->phi[k][j][i][g];
          }
        }
      }
    }
  }
  ierr = PetscLogFlops(12 * e->ng * e->nphix * e->nphiy * e->nphiz);CHKERRQ(ierr);
  /*
   Clean up
   */
  ierr = PetscFree4(pressure_elem,gradv_elem[0],gradv_elem[1],gradv_elem[2]);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_GradientUInSituStresses3D_local"
/*
 VF_GradientUInSituStresses3D_local: Accumulates the contribution of surface forces along the face of an element
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_GradientUInSituStresses3D_local(PetscReal *residual_local,PetscReal ****f_array,PetscInt ek,PetscInt ej,PetscInt ei,FACE face,VFCartFEElement3D *e)
{
  PetscInt       i,j,k,l,c,g;
  PetscReal      *f_elem[3];
  PetscErrorCode ierr;
  PetscFunctionBegin;
  /*
   Initialize f_Elem
   */
  ierr = PetscMalloc3(e->ng,&f_elem[0],e->ng,&f_elem[1],e->ng,&f_elem[2]);CHKERRQ(ierr);
  for (c = 0; c < e->dim; c++) {
    for (g = 0; g < e->ng; g++) {
      f_elem[c][g] = 0;
    }
  }
  /*
   Compute f_elem
   */
  switch (face) {
    case X0:
      for (k = 0; k < e->nphiz; k++) {
        for (j = 0; j < e->nphiy; j++) {
          for (i = 0; i < e->nphix; i++) {
            for (c = 0; c < e->dim; c++) {
              for (g = 0; g < e->ng; g++) {
                f_elem[c][g] += e->phi[k][j][i][g] * f_array[ek+k][ej+j][ei][c] / e->lx;
              }
            }
          }
        }
      }
      break;
    case X1:
      for (k = 0; k < e->nphiz; k++) {
        for (j = 0; j < e->nphiy; j++) {
          for (i = 0; i < e->nphix; i++) {
            for (c = 0; c < e->dim; c++) {
              for (g = 0; g < e->ng; g++) {
                f_elem[c][g] += e->phi[k][j][i][g] * f_array[ek+k][ej+j][ei+e->nphix-1][c] / e->lx;
              }
            }
          }
        }
      }
      break;
    case Y0:
      for (k = 0; k < e->nphiz; k++) {
        for (j = 0; j < e->nphiy; j++) {
          for (i = 0; i < e->nphix; i++) {
            for (c = 0; c < e->dim; c++) {
              for (g = 0; g < e->ng; g++) {
                f_elem[c][g] += e->phi[k][j][i][g] * f_array[ek+k][ej][ei+i][c] / e->ly;
              }
            }
          }
        }
      }
      break;
    case Y1:
      for (k = 0; k < e->nphiz; k++) {
        for (j = 0; j < e->nphiy; j++) {
          for (i = 0; i < e->nphix; i++) {
            for (c = 0; c < e->dim; c++) {
              for (g = 0; g < e->ng; g++) {
                f_elem[c][g] += e->phi[k][j][i][g] * f_array[ek+k][ej+e->nphiy-1][ei+i][c] / e->ly;
              }
            }
          }
        }
      }
      break;
    case Z0:
      for (k = 0; k < e->nphiz; k++) {
        for (j = 0; j < e->nphiy; j++) {
          for (i = 0; i < e->nphix; i++) {
            for (c = 0; c < e->dim; c++) {
              for (g = 0; g < e->ng; g++) {
                f_elem[c][g] += e->phi[k][j][i][g] * f_array[ek][ej+j][ei+i][c] / e->lz;
              }
            }
          }
        }
      }
      break;
    case Z1:
      for (k = 0; k < e->nphiz; k++) {
        for (j = 0; j < e->nphiy; j++) {
          for (i = 0; i < e->nphix; i++) {
            for (c = 0; c < e->dim; c++) {
              for (g = 0; g < e->ng; g++) {
                f_elem[c][g] += e->phi[k][j][i][g] * f_array[ek+e->nphiz-1][ej+j][ei+i][c] / e->lz;
              }
            }
          }
        }
      }
      break;
  }
  /*
   Accumulate
   */
  for (l= 0,k = 0; k < e->nphiz; k++) {
    for (j = 0; j < e->nphiy; j++) {
      for (i = 0; i < e->nphix; i++) {
        for (c = 0; c < e->dim; c++,l++) {
          for (g = 0; g < e->ng; g++) {
            residual_local[l] += e->weight[g] * e->phi[k][j][i][g] * f_elem[c][g];
          }
        }
      }
    }
  }
  /*
   Clean up
   */
  ierr = PetscFree3(f_elem[0],f_elem[1],f_elem[2]);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VF_ElasticEnergy3D_local"
/*
 VF_ElasticEnergy3D_local:
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_ElasticEnergy3D_local(PetscReal *ElasticEnergy_local,PetscReal ****u_array,PetscReal ***v_array,PetscReal ***theta_array,PetscReal ***thetaRef_array,PetscReal ***pressure_array,VFMatProp *matprop,VFProp *vfprop,PetscInt ek,PetscInt ej,PetscInt ei,VFCartFEElement3D *e)
{
  PetscInt       g,i,j,k;
  PetscReal      *trepsilon_elem;
  PetscReal      *D11_elem,*D22_elem,*D33_elem,*D12_elem,*D23_elem,*D13_elem;
  PetscReal      lambda,mu,alpha,threekappa,coefbeta,WD;
  PetscReal      *v_elem,*pressure_elem,*theta_elem;
  PetscErrorCode ierr;
  
  PetscFunctionBegin;
  ierr     = PetscMalloc3(e->ng,&D11_elem,e->ng,&D22_elem,e->ng,&D33_elem);CHKERRQ(ierr);
  ierr     = PetscMalloc3(e->ng,&D12_elem,e->ng,&D23_elem,e->ng,&D13_elem);CHKERRQ(ierr);
  ierr     = PetscMalloc2(e->ng,&v_elem,e->ng,&trepsilon_elem);CHKERRQ(ierr);
  ierr     = PetscMalloc2(e->ng,&pressure_elem,e->ng,&theta_elem);CHKERRQ(ierr);
  lambda   = matprop->lambda;
  mu       = matprop->mu;
  alpha    = matprop->alpha;
  threekappa = 3.*lambda + 2. *mu;
  coefbeta = matprop->beta / threekappa;
  
  ierr = PetscMemzero(D11_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemzero(D22_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemzero(D33_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemzero(D23_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemzero(D13_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemzero(D12_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemzero(trepsilon_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemzero(v_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemzero(pressure_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemzero(theta_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);

  for (k = 0; k < e->nphiz; k++) {
    for (j = 0; j < e->nphiy; j++) {
      for (i = 0; i < e->nphix; i++) {
        for (g = 0; g < e->ng; g++) {
          D11_elem[g] +=  e->dphi[k][j][i][0][g] * u_array[ek+k][ej+j][ei+i][0];
          D22_elem[g] +=  e->dphi[k][j][i][1][g] * u_array[ek+k][ej+j][ei+i][1];
          D33_elem[g] +=  e->dphi[k][j][i][2][g] * u_array[ek+k][ej+j][ei+i][2];
          
          D12_elem[g] += (e->dphi[k][j][i][0][g] * u_array[ek+k][ej+j][ei+i][1]
                        + e->dphi[k][j][i][1][g] * u_array[ek+k][ej+j][ei+i][0]) * .5;
          D23_elem[g] += (e->dphi[k][j][i][1][g] * u_array[ek+k][ej+j][ei+i][2]
                        + e->dphi[k][j][i][2][g] * u_array[ek+k][ej+j][ei+i][1]) * .5;
          D13_elem[g] += (e->dphi[k][j][i][0][g] * u_array[ek+k][ej+j][ei+i][2]
                        + e->dphi[k][j][i][2][g] * u_array[ek+k][ej+j][ei+i][0]) * .5;
                        
          v_elem[g] += e->phi[k][j][i][g] * v_array[ek+k][ej+j][ei+i];
          pressure_elem[g] += e->phi[k][j][i][g] * pressure_array[ek+k][ej+j][ei+i];
          theta_elem[g] += e->phi[k][j][i][g] * (theta_array[ek+k][ej+j][ei+i] - thetaRef_array[ek+k][ej+j][ei+i]);
        }
      }
    }
  }
  
  *ElasticEnergy_local = 0.;
  for (g = 0; g < e->ng; g++) {
    trepsilon_elem[g] =  D11_elem[g] + D22_elem[g] + D33_elem[g];
     
    D11_elem[g] -= trepsilon_elem[g] / 3.;
    D22_elem[g] -= trepsilon_elem[g] / 3.;
    D33_elem[g] -= trepsilon_elem[g] / 3.;
    /* 
      Deviatoric part
      mu (1+eta) * v^2e^D:e^D
    */
    *ElasticEnergy_local += (  D11_elem[g] * D11_elem[g]
                            +  D22_elem[g] * D22_elem[g]
                            +  D33_elem[g] * D33_elem[g] 
                            + (D12_elem[g] * D12_elem[g]
                            +  D23_elem[g] * D23_elem[g]
                            +  D13_elem[g] * D13_elem[g]) * 2.) * v_elem[g] * v_elem[g] * (1. + vfprop->eta) * mu * e->weight[g];
    /*
      Spherical part
      9\kappa/2 [s(tr(e)/3 - \alpha \theta)  - \beta p / 3/\kappa)]:[s(tr(e)/3 - \alpha \theta)  - \beta p / 3/\kappa)]
    */
    WD = (trepsilon_elem[g] / 3. - alpha * theta_elem[g]) * v_elem[g] - coefbeta * pressure_elem[g];
    *ElasticEnergy_local += 3. * threekappa / 2. * WD * WD * e->weight[g];
    *ElasticEnergy_local += threekappa / 6. * trepsilon_elem[g] * trepsilon_elem[g] * vfprop->eta * e->weight[g];
  }
  ierr = PetscFree3(D11_elem,D22_elem,D33_elem);CHKERRQ(ierr);
  ierr = PetscFree3(D12_elem,D23_elem,D13_elem);CHKERRQ(ierr);
  ierr = PetscFree2(v_elem,trepsilon_elem);CHKERRQ(ierr);
  ierr = PetscFree2(pressure_elem,theta_elem);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VF_ElasticEnergyNoCompression3D_local"
/*
 VF_ElasticEnergyNoCompression3D_local:
 
 (c) 2010-2014 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_ElasticEnergyNoCompression3D_local(PetscReal *ElasticEnergy_local,PetscReal ****u_array,PetscReal ***v_array,PetscReal ***theta_array,PetscReal ***thetaRef_array,PetscReal ***pressure_array,VFMatProp *matprop,VFProp *vfprop,PetscInt ek,PetscInt ej,PetscInt ei,VFCartFEElement3D *e)
{
  PetscInt       g,i,j,k;
  PetscReal      *trepsilon_elem;
  PetscReal      *D11_elem,*D22_elem,*D33_elem,*D12_elem,*D23_elem,*D13_elem;
  PetscReal      lambda,mu,alpha,threekappa,coefbeta,WD;
  PetscReal      *v_elem,*s_elem,*pressure_elem,*theta_elem;
  PetscErrorCode ierr;
  
  PetscFunctionBegin;
  ierr     = PetscMalloc3(e->ng,&D11_elem,e->ng,&D22_elem,e->ng,&D33_elem);CHKERRQ(ierr);
  ierr     = PetscMalloc3(e->ng,&D12_elem,e->ng,&D23_elem,e->ng,&D13_elem);CHKERRQ(ierr);
  ierr     = PetscMalloc3(e->ng,&v_elem,e->ng,&s_elem,e->ng,&trepsilon_elem);CHKERRQ(ierr);
  ierr     = PetscMalloc2(e->ng,&pressure_elem,e->ng,&theta_elem);CHKERRQ(ierr);
  lambda   = matprop->lambda;
  mu       = matprop->mu;
  alpha    = matprop->alpha;
  threekappa = 3.*lambda + 2. *mu;
  coefbeta = matprop->beta / threekappa;
  
  ierr = PetscMemzero(D11_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemzero(D22_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemzero(D33_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemzero(D23_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemzero(D13_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemzero(D12_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemzero(trepsilon_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemzero(v_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemzero(pressure_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemzero(theta_elem,e->ng * sizeof(PetscReal));CHKERRQ(ierr);

  for (k = 0; k < e->nphiz; k++) {
    for (j = 0; j < e->nphiy; j++) {
      for (i = 0; i < e->nphix; i++) {
        for (g = 0; g < e->ng; g++) {
          D11_elem[g] +=  e->dphi[k][j][i][0][g] * u_array[ek+k][ej+j][ei+i][0];
          D22_elem[g] +=  e->dphi[k][j][i][1][g] * u_array[ek+k][ej+j][ei+i][1];
          D33_elem[g] +=  e->dphi[k][j][i][2][g] * u_array[ek+k][ej+j][ei+i][2];
          
          D12_elem[g] += (e->dphi[k][j][i][0][g] * u_array[ek+k][ej+j][ei+i][1]
                        + e->dphi[k][j][i][1][g] * u_array[ek+k][ej+j][ei+i][0]) * .5;
          D23_elem[g] += (e->dphi[k][j][i][1][g] * u_array[ek+k][ej+j][ei+i][2]
                        + e->dphi[k][j][i][2][g] * u_array[ek+k][ej+j][ei+i][1]) * .5;
          D13_elem[g] += (e->dphi[k][j][i][0][g] * u_array[ek+k][ej+j][ei+i][2]
                        + e->dphi[k][j][i][2][g] * u_array[ek+k][ej+j][ei+i][0]) * .5;
          v_elem[g] += e->phi[k][j][i][g] * v_array[ek+k][ej+j][ei+i];
          pressure_elem[g] += e->phi[k][j][i][g] * pressure_array[ek+k][ej+j][ei+i];
          theta_elem[g] += e->phi[k][j][i][g] * (theta_array[ek+k][ej+j][ei+i] - thetaRef_array[ek+k][ej+j][ei+i]);
        }
      }
    }
  }
  *ElasticEnergy_local = 0.;
  for (g = 0; g < e->ng; g++) {
    trepsilon_elem[g] =  D11_elem[g] + D22_elem[g] + D33_elem[g];
    if (trepsilon_elem[g] < UNILATERAL_THRES) {
      s_elem[g] = 1.;
    } else {
      s_elem[g] = v_elem[g];
    }
     
    D11_elem[g] -= trepsilon_elem[g] / 3.;
    D22_elem[g] -= trepsilon_elem[g] / 3.;
    D33_elem[g] -= trepsilon_elem[g] / 3.;
    
    /* 
      Deviatoric part
      mu (1+eta/2) * v^2e^D:e^D
    */
    *ElasticEnergy_local += (  D11_elem[g] * D11_elem[g]
                            +  D22_elem[g] * D22_elem[g]
                            +  D33_elem[g] * D33_elem[g] 
                            + (D12_elem[g] * D12_elem[g]
                            +  D23_elem[g] * D23_elem[g]
                            +  D13_elem[g] * D13_elem[g]) * 2.) * v_elem[g] * v_elem[g] * (1. + vfprop->eta) * mu * e->weight[g];

    /*
      Spherical part
      9\kappa/2 [s(tr(e)/3 - \alpha \theta)  - \beta p / 3/\kappa)]:[s(tr(e)/3 - \alpha \theta)  - \beta p / 3/\kappa)]
    */
    WD = (trepsilon_elem[g] / 3. - alpha * theta_elem[g]) * s_elem[g] - coefbeta * pressure_elem[g];
    *ElasticEnergy_local += 3. * threekappa / 2. * WD * WD * e->weight[g];
    *ElasticEnergy_local += threekappa / 6. * trepsilon_elem[g] * trepsilon_elem[g] * vfprop->eta * e->weight[g];
  }
  ierr = PetscFree3(D11_elem,D22_elem,D33_elem);CHKERRQ(ierr);
  ierr = PetscFree3(D12_elem,D23_elem,D13_elem);CHKERRQ(ierr);
  ierr = PetscFree3(v_elem,s_elem,trepsilon_elem);CHKERRQ(ierr);
  ierr = PetscFree2(pressure_elem,theta_elem);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VF_PressureWork3D_local"
/*
 VF_PressureWork3D_local: Compute the contribution of an element to the work of the pressure forces along the crack walls,
 given by
 \int_e p(x)\nabla v(x) \cdot u(x) \, dx
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_PressureWork3D_local(PetscReal *PressureWork_local,PetscReal ****u_array,PetscReal ***v_array,PetscReal ***pressure_array,VFMatProp *matprop,VFProp *vfprop,PetscInt ek,PetscInt ej,PetscInt ei,VFCartFEElement3D *e)
{
  PetscErrorCode ierr;
  PetscInt       i,j,k,g,c;
  PetscReal      *pressure_elem,*gradv_elem[3],*u_elem[3];
  
  PetscFunctionBegin;
  ierr = PetscMalloc4(e->ng,&pressure_elem,
                      e->ng,&gradv_elem[0],
                      e->ng,&gradv_elem[1],
                      e->ng,&gradv_elem[2]);CHKERRQ(ierr);
  ierr = PetscMalloc3(e->ng,&u_elem[0],
                      e->ng,&u_elem[1],
                      e->ng,&u_elem[2]);CHKERRQ(ierr);
  
  /*
   Compute the projection of the fields in the local base functions basis
   */
  for (g = 0; g < e->ng; g++) {
    pressure_elem[g] = 0;
    for (c= 0; c<3; c++) {
      gradv_elem[c][g] = 0.;
      u_elem[c][g]     = 0.;
    }
  }
  for (k = 0; k < e->nphiz; k++) {
    for (j = 0; j < e->nphiy; j++) {
      for (i = 0; i < e->nphix; i++) {
        for (g = 0; g < e->ng; g++) {
          pressure_elem[g] += e->phi[k][j][i][g] * pressure_array[ek+k][ej+j][ei+i];
          for (c = 0; c < 3; c++) {
            gradv_elem[c][g] += e->dphi[k][j][i][c][g] * v_array[ek+k][ej+j][ei+i];
            u_elem[c][g]     += e->phi[k][j][i][g]     * u_array[ek+k][ej+j][ei+i][c];
          }
        }
      }
    }
  }
  ierr = PetscLogFlops(15 * e->ng * e->nphix * e->nphiy * e->nphiz);CHKERRQ(ierr);
  
  /*
   Accumulate the contribution of the current element
   */
  for (g = 0; g < e->ng; g++) {
    *PressureWork_local += e->weight[g] * pressure_elem[g]
    * (u_elem[0][g] * gradv_elem[0][g] + u_elem[1][g] * gradv_elem[1][g] + u_elem[2][g] * gradv_elem[2][g]);
  }
  ierr = PetscLogFlops(e->ng * e->nphix * e->nphiy * e->nphiz);CHKERRQ(ierr);
  /*
   Clean up
   */
  ierr = PetscFree4(pressure_elem,gradv_elem[0],gradv_elem[1],gradv_elem[2]);CHKERRQ(ierr);
  ierr = PetscFree3(u_elem[0],u_elem[1],u_elem[2]);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_InSituStressWork3D_local"
/*
 VF_InSituStressWork3D_local
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_InSituStressWork3D_local(PetscReal *Work_local,PetscReal ****u_array,PetscReal ****f_array,PetscInt ek,PetscInt ej,PetscInt ei,FACE face,VFCartFEElement3D *e)
{
  PetscErrorCode ierr;
  PetscInt       i,j,k,c,g;
  PetscInt       dim=3;
  PetscReal      *u_elem[3],*f_elem[3];
  
  PetscFunctionBegin;
  /*
   Initialize
   */
  ierr = PetscMalloc3(e->ng,&u_elem[0],e->ng,&u_elem[1],e->ng,&u_elem[2]);CHKERRQ(ierr);
  ierr = PetscMalloc3(e->ng,&f_elem[0],e->ng,&f_elem[1],e->ng,&f_elem[2]);CHKERRQ(ierr);
  for (c = 0; c < dim; c++) {
    for (g = 0; g < e->ng; g++) {
      u_elem[c][g] = 0;
      f_elem[c][g] = 0;
    }
  }
  /*
   Compute
   */
  switch (face) {
    case X0:
      for (k = 0; k < e->nphiz; k++) {
        for (j = 0; j < e->nphiy; j++) {
          for (i = 0; i < e->nphix; i++) {
            for (c = 0; c < dim; c++) {
              for (g = 0; g < e->ng; g++) {
                u_elem[c][g] += e->phi[k][j][i][g] * u_array[ek+k][ej+j][ei][c] / e->lx;
                f_elem[c][g] += e->phi[k][j][i][g] * f_array[ek+k][ej+j][ei][c];
              }
            }
          }
        }
      }
      break;
    case X1:
      for (k = 0; k < e->nphiz; k++) {
        for (j = 0; j < e->nphiy; j++) {
          for (i = 0; i < e->nphix; i++) {
            for (c = 0; c < dim; c++) {
              for (g = 0; g < e->ng; g++) {
                u_elem[c][g] += e->phi[k][j][i][g] * u_array[ek+k][ej+j][ei+e->nphix-1][c] / e->lx;
                f_elem[c][g] += e->phi[k][j][i][g] * f_array[ek+k][ej+j][ei+e->nphix-1][c];
              }
            }
          }
        }
      }
      break;
    case Y0:
      for (k = 0; k < e->nphiz; k++) {
        for (j = 0; j < e->nphiy; j++) {
          for (i = 0; i < e->nphix; i++) {
            for (c = 0; c < dim; c++) {
              for (g = 0; g < e->ng; g++) {
                u_elem[c][g] += e->phi[k][j][i][g] * u_array[ek+k][ej][ei+i][c] / e->ly;
                f_elem[c][g] += e->phi[k][j][i][g] * f_array[ek+k][ej][ei+i][c];
              }
            }
          }
        }
      }
      break;
    case Y1:
      for (k = 0; k < e->nphiz; k++) {
        for (j = 0; j < e->nphiy; j++) {
          for (i = 0; i < e->nphix; i++) {
            for (c = 0; c < dim; c++) {
              for (g = 0; g < e->ng; g++) {
                u_elem[c][g] += e->phi[k][j][i][g] * u_array[ek+k][ej+e->nphiy-1][ei+i][c] / e->ly;
                f_elem[c][g] += e->phi[k][j][i][g] * f_array[ek+k][ej+e->nphiy-1][ei+i][c];
              }
            }
          }
        }
      }
      break;
    case Z0:
      for (k = 0; k < e->nphiz; k++) {
        for (j = 0; j < e->nphiy; j++) {
          for (i = 0; i < e->nphix; i++) {
            for (c = 0; c < dim; c++) {
              for (g = 0; g < e->ng; g++) {
                u_elem[c][g] += e->phi[k][j][i][g] * u_array[ek][ej+j][ei+i][c] / e->lz;
                f_elem[c][g] += e->phi[k][j][i][g] * f_array[ek][ej+j][ei+i][c];
              }
            }
          }
        }
      }
      break;
    case Z1:
      for (k = 0; k < e->nphiz; k++) {
        for (j = 0; j < e->nphiy; j++) {
          for (i = 0; i < e->nphix; i++) {
            for (c = 0; c < dim; c++) {
              for (g = 0; g < e->ng; g++) {
                u_elem[c][g] += e->phi[k][j][i][g] * u_array[ek+e->nphiz-1][ej+j][ei+i][c] / e->lz;
                f_elem[c][g] += e->phi[k][j][i][g] * f_array[ek+e->nphiz-1][ej+j][ei+i][c];
              }
            }
          }
        }
      }
      break;
  }
  /*
   Accumulate
   */
  for (c = 0; c < dim; c++) {
    for (g = 0; g < e->ng; g++) {
      *Work_local += e->weight[g] * u_elem[c][g] * f_elem[c][g];
    }
  }
  ierr = PetscFree3(u_elem[0],u_elem[1],u_elem[2]);CHKERRQ(ierr);
  ierr = PetscFree3(f_elem[0],f_elem[1],f_elem[2]);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_UEnergy3D"
/*
 VF_UEnergy3D
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_UEnergy3D(PetscReal *ElasticEnergy,PetscReal *InsituWork,PetscReal *PressureWork,Vec U,VFCtx *ctx)
{
  PetscErrorCode ierr;
  PetscInt       xs,xm,nx;
  PetscInt       ys,ym,ny;
  PetscInt       zs,zm,nz;
  PetscInt       ei,ej,ek;
  PetscInt       i,j,k,c;
  Vec            u_localVec,v_localVec;
  Vec            theta_localVec,thetaRef_localVec;
  Vec            pressure_localVec;
  PetscReal      ****u_array;
  PetscReal      ***v_array;
  PetscReal      ***theta_array;
  PetscReal      ***thetaRef_array;
  PetscReal      ***pressure_array;
  PetscReal      myInsituWork,myPressureWork;
  PetscReal      myElasticEnergy,myElasticEnergyLocal;
  PetscReal      hx,hy,hz;
  PetscReal      ****coords_array;
  PetscReal      ****f_array;
  Vec            f_localVec;
  FACE           face;
  PetscReal      z;
  int            stresscomp[3];
  PetscReal      stressdir[3];
  PetscReal      stressmag;
  PetscReal      BBmin[3],BBmax[3];
  
  PetscFunctionBegin;
  myElasticEnergy = 0.;
  myInsituWork = 0.;
  myPressureWork = 0.;
  myElasticEnergyLocal = 0;
  
  ierr = DMDAGetInfo(ctx->daScalCell,NULL,&nx,&ny,&nz,NULL,NULL,NULL,
                     NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
  ierr = DMDAGetCorners(ctx->daScalCell,&xs,&ys,&zs,&xm,&ym,&zm);CHKERRQ(ierr);
  
  /*
   Get coordinates
   */
  ierr = DMDAVecGetArrayDOF(ctx->daVect,ctx->coordinates,&coords_array);CHKERRQ(ierr);
  /*
   Get bounding box from petsc DA
   */
  ierr = DMDAGetBoundingBox(ctx->daVect,BBmin,BBmax);CHKERRQ(ierr);
  /*
   get U_array
   */
  ierr = DMGetLocalVector(ctx->daVect,&u_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daVect,U,INSERT_VALUES,u_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daVect,U,INSERT_VALUES,u_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArrayDOF(ctx->daVect,u_localVec,&u_array);CHKERRQ(ierr);
  /*
   get v_array
   */
  ierr = DMGetLocalVector(ctx->daScal,&v_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daScal,ctx->fields->V,INSERT_VALUES,v_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daScal,ctx->fields->V,INSERT_VALUES,v_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,v_localVec,&v_array);CHKERRQ(ierr);
  /*
   get theta_array, thetaRef_array
   */
  ierr = DMGetLocalVector(ctx->daScal,&theta_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daScal,ctx->fields->theta,INSERT_VALUES,theta_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daScal,ctx->fields->theta,INSERT_VALUES,theta_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,theta_localVec,&theta_array);CHKERRQ(ierr);
  ierr = DMGetLocalVector(ctx->daScal,&thetaRef_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daScal,ctx->fields->thetaRef,INSERT_VALUES,thetaRef_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daScal,ctx->fields->thetaRef,INSERT_VALUES,thetaRef_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,thetaRef_localVec,&thetaRef_array);CHKERRQ(ierr);
  /*
   get pressure_array
   */
  ierr = DMGetLocalVector(ctx->daScal,&pressure_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daScal,ctx->fields->pressure,INSERT_VALUES,pressure_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daScal,ctx->fields->pressure,INSERT_VALUES,pressure_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,pressure_localVec,&pressure_array);CHKERRQ(ierr);
  /*
   Allocating multi-dimensional vectors in C is a pain, so for the in-situ stresses / surface stresses,
   I get a 3 component Vec for the external forces,
   then get a full array. This is a bit wastefull since we never use the internal values...
   Note that we cannot fully initialize it since we need the normal direction to each face.
   */
  if (ctx->hasInsitu) {
    ierr = DMGetLocalVector(ctx->daVect,&f_localVec);CHKERRQ(ierr);
    ierr = VecSet(f_localVec,0.);
    ierr = DMDAVecGetArrayDOF(ctx->daVect,f_localVec,&f_array);CHKERRQ(ierr);
  }
  
  *ElasticEnergy = 0;
  *InsituWork    = 0;
  *PressureWork  = 0.;
  for (ek = zs; ek < zs + zm; ek++) {
    for (ej = ys; ej < ys+ym; ej++) {
      for (ei = xs; ei < xs+xm; ei++) {
        hx   = coords_array[ek][ej][ei+1][0]-coords_array[ek][ej][ei][0];
        hy   = coords_array[ek][ej+1][ei][1]-coords_array[ek][ej][ei][1];
        hz   = coords_array[ek+1][ej][ei][2]-coords_array[ek][ej][ei][2];
        ierr = VFCartFEElement3DInit(&ctx->e3D,hx,hy,hz);CHKERRQ(ierr);

        switch (ctx->unilateral) {
          case UNILATERAL_NONE:
            ierr = VF_ElasticEnergy3D_local(&myElasticEnergyLocal,u_array,v_array,
                                            theta_array,thetaRef_array,pressure_array,
                                            &ctx->matprop[ctx->layer[ek]],&ctx->vfprop,
                                            ek,ej,ei,&ctx->e3D);CHKERRQ(ierr);
            break;
          case UNILATERAL_NOCOMPRESSION:
            ierr = VF_ElasticEnergyNoCompression3D_local(&myElasticEnergyLocal,u_array,v_array,
                                            theta_array,thetaRef_array,pressure_array,
                                            &ctx->matprop[ctx->layer[ek]],&ctx->vfprop,
                                            ek,ej,ei,&ctx->e3D);CHKERRQ(ierr);
            //ierr = VF_ElasticEnergy3D_local(&myElasticEnergyLocal,u_array,v_array,
            //                                theta_array,thetaRef_array,pressure_array,
            //                                &ctx->matprop[ctx->layer[ek]],&ctx->vfprop,
            //                                ek,ej,ei,&ctx->e3D);CHKERRQ(ierr);
            break;
        }
        myElasticEnergy += myElasticEnergyLocal;
        if (ctx->hasCrackPressure) {
          ierr = VF_PressureWork3D_local(&myPressureWork,u_array,v_array,pressure_array,
                                         &ctx->matprop[ctx->layer[ek]],&ctx->vfprop,
                                         ek,ej,ei,&ctx->e3D);CHKERRQ(ierr);
        }
        
        if (ctx->hasInsitu) {
          /*
           We need to reconstruct the external forces before computing their work.
           This take a bit more effort
           */
          if (ek == 0) {
            /*
             Face Z0
             sigma.(0,0,-1) = (-s_13,-s_23,-s_33) = (-S4,-S3,-S2)
             */
            face          = Z0;
            stresscomp[0] = 4; stressdir[0] = -1.;
            stresscomp[1] = 3; stressdir[1] = -1.;
            stresscomp[2] = 2; stressdir[2] = -1.;
            for (c = 0; c < 3; c++) {
              if (ctx->bcU[c].face[face] == NONE) {
                for (k = 0; k < ctx->e3D.nphiz; k++) {
                  z         = coords_array[ek+k][ej][ei][2];
                  stressmag = stressdir[c] *
                  (ctx->insitumin[stresscomp[c]] + (z - BBmin[2]) / (BBmax[2] - BBmin[2])
                   * (ctx->insitumax[stresscomp[c]] - ctx->insitumin[stresscomp[c]]));
                  for (j = 0; j < ctx->e3D.nphiy; j++)
                    for (i = 0; i < ctx->e3D.nphix; i++)
                      f_array[ek+k][ej+j][ei+i][c] = stressmag;
                }
              }
            }
            ierr = VF_InSituStressWork3D_local(&myInsituWork,u_array,f_array,ek,ej,ei,face,&ctx->e3D);CHKERRQ(ierr);
          }
          
          if (ek == nz-2) {
            /*
             Face Z1
             sigma.(0,0,1) = (s_13,s_23,s_33) = (S4,S3,S2)
             */
            face          = Z1;
            stresscomp[0] = 4; stressdir[0] = 1.;
            stresscomp[1] = 3; stressdir[1] = 1.;
            stresscomp[2] = 2; stressdir[2] = 1.;
            for (c = 0; c < 3; c++) {
              if (ctx->bcU[c].face[face] == NONE) {
                for (k = 0; k < ctx->e3D.nphiz; k++) {
                  z         = coords_array[ek+k][ej][ei][2];
                  stressmag = stressdir[c] *
                  (ctx->insitumin[stresscomp[c]] + (z - BBmin[2]) / (BBmax[2] - BBmin[2])
                   * (ctx->insitumax[stresscomp[c]] - ctx->insitumin[stresscomp[c]]));
                  for (j = 0; j < ctx->e3D.nphiy; j++)
                    for (i = 0; i < ctx->e3D.nphix; i++)
                      f_array[ek+k][ej+j][ei+i][c] = stressmag;
                }
              }
            }
            ierr = VF_InSituStressWork3D_local(&myInsituWork,u_array,f_array,ek,ej,ei,face,&ctx->e3D);CHKERRQ(ierr);
          }
          
          if (ej == 0) {
            /*
             Face Y0
             sigma.(0,-1,0) = (-s_12,-s_22,-s_23) = (-S5,-S1,-S3)
             */
            face          = Y0;
            stresscomp[0] = 5; stressdir[0] = -1.;
            stresscomp[1] = 1; stressdir[1] = -1.;
            stresscomp[2] = 3; stressdir[2] = -1.;
            for (k = 0; k < ctx->e3D.nphiz; k++) {
              for (j = 0; j < ctx->e3D.nphiy; j++) {
                for (i = 0; i < ctx->e3D.nphix; i++) {
                  z = coords_array[ek+k][ej+j][ei+i][2];
                  for (c = 0; c < 3; c++) {
                    if (ctx->bcU[c].face[face] == NONE) {
                      f_array[ek+k][ej+j][ei+i][c] = stressdir[c] *
                      (ctx->insitumin[stresscomp[c]] +
                       (z - BBmin[2]) / (BBmax[2] - BBmin[2])
                       * (ctx->insitumax[stresscomp[c]] - ctx->insitumin[stresscomp[c]]));
                    }
                  }
                }
              }
            }
            ierr = VF_InSituStressWork3D_local(&myInsituWork,u_array,f_array,ek,ej,ei,face,&ctx->e3D);CHKERRQ(ierr);
          }
          
          if (ej == ny-2) {
            /*
             Face Y1
             sigma.(0,1,0) = (s_12,s_22,s_23) = (S5,S1,S3)
             */
            face          = Y1;
            stresscomp[0] = 5; stressdir[0] = 1.;
            stresscomp[1] = 1; stressdir[1] = 1.;
            stresscomp[2] = 3; stressdir[2] = 1.;
            for (k = 0; k < ctx->e3D.nphiz; k++) {
              for (j = 0; j < ctx->e3D.nphiy; j++) {
                for (i = 0; i < ctx->e3D.nphix; i++) {
                  z = coords_array[ek+k][ej+j][ei+i][2];
                  for (c = 0; c < 3; c++) {
                    if (ctx->bcU[c].face[face] == NONE) {
                      f_array[ek+k][ej+j][ei+i][c] = stressdir[c] *
                      (ctx->insitumin[stresscomp[c]] +
                       (z - BBmin[2]) / (BBmax[2] - BBmin[2])
                       * (ctx->insitumax[stresscomp[c]] - ctx->insitumin[stresscomp[c]]));
                    }
                  }
                }
              }
            }
            ierr = VF_InSituStressWork3D_local(&myInsituWork,u_array,f_array,ek,ej,ei,face,&ctx->e3D);CHKERRQ(ierr);
          }
          
          if (ei == 0) {
            /*
             Face X0
             sigma.(-1,0,0) = (-s_11,-s_12,-s_13) = (-S0,-S5,-S4)
             */
            face          = X0;
            stresscomp[0] = 0; stressdir[0] = -1.;
            stresscomp[1] = 5; stressdir[1] = -1.;
            stresscomp[2] = 4; stressdir[2] = -1.;
            for (k = 0; k < ctx->e3D.nphiz; k++) {
              for (j = 0; j < ctx->e3D.nphiy; j++) {
                for (i = 0; i < ctx->e3D.nphix; i++) {
                  z = coords_array[ek+k][ej+j][ei+i][2];
                  for (c = 0; c < 3; c++) {
                    if (ctx->bcU[c].face[face] == NONE) {
                      f_array[ek+k][ej+j][ei+i][c] = stressdir[c] *
                      (ctx->insitumin[stresscomp[c]] +
                       (z - BBmin[2]) / (BBmax[2] - BBmin[2])
                       * (ctx->insitumax[stresscomp[c]] - ctx->insitumin[stresscomp[c]]));
                    }
                  }
                }
              }
            }
            ierr = VF_InSituStressWork3D_local(&myInsituWork,u_array,f_array,ek,ej,ei,face,&ctx->e3D);CHKERRQ(ierr);
          }
          
          if (ei == nx-2) {
            /*
             Face X1
             sigma.(1,0,0) = (s_11,s_12,s_13) = (S0,S5,S4)
             */
            face          = X1;
            stresscomp[0] = 0; stressdir[0] = 1.;
            stresscomp[1] = 5; stressdir[1] = 1.;
            stresscomp[2] = 4; stressdir[2] = 1.;
            for (k = 0; k < ctx->e3D.nphiz; k++) {
              for (j = 0; j < ctx->e3D.nphiy; j++) {
                for (i = 0; i < ctx->e3D.nphix; i++) {
                  z = coords_array[ek+k][ej+j][ei+i][2];
                  for (c = 0; c < 3; c++) {
                    if (ctx->bcU[c].face[face] == NONE) {
                      f_array[ek+k][ej+j][ei+i][c] = stressdir[c] *
                      (ctx->insitumin[stresscomp[c]] +
                       (z - BBmin[2]) / (BBmax[2] - BBmin[2])
                       * (ctx->insitumax[stresscomp[c]] - ctx->insitumin[stresscomp[c]]));
                    }
                  }
                }
              }
            }
            ierr = VF_InSituStressWork3D_local(&myInsituWork,u_array,f_array,ek,ej,ei,face,&ctx->e3D);CHKERRQ(ierr);
          }
        }
      }
    }
  }
  
  ierr = MPI_Allreduce(&myElasticEnergy,ElasticEnergy,1,MPIU_SCALAR,MPI_SUM,PETSC_COMM_WORLD);CHKERRQ(ierr);
  ierr = MPI_Allreduce(&myPressureWork,PressureWork,1,MPIU_SCALAR,MPI_SUM,PETSC_COMM_WORLD);CHKERRQ(ierr);
  ierr = MPI_Allreduce(&myInsituWork,InsituWork,1,MPIU_SCALAR,MPI_SUM,PETSC_COMM_WORLD);CHKERRQ(ierr);

  ierr = DMDAVecRestoreArrayDOF(ctx->daVect,ctx->coordinates,&coords_array);CHKERRQ(ierr);
  
  ierr = DMDAVecRestoreArrayDOF(ctx->daVect,u_localVec,&u_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daVect,&u_localVec);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(ctx->daScal,v_localVec,&v_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daScal,&v_localVec);CHKERRQ(ierr);
  
  ierr = DMDAVecRestoreArray(ctx->daScal,theta_localVec,&theta_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daScal,&theta_localVec);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(ctx->daScal,thetaRef_localVec,&thetaRef_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daScal,&thetaRef_localVec);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(ctx->daScal,pressure_localVec,&pressure_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daScal,&pressure_localVec);CHKERRQ(ierr);
  if (ctx->hasInsitu) {
    ierr = DMDAVecRestoreArrayDOF(ctx->daVect,f_localVec,&f_array);CHKERRQ(ierr);
    ierr = DMRestoreLocalVector(ctx->daVect,&f_localVec);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_StepU"
/*
 VF_StepU
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_StepU(VFFields *fields,VFCtx *ctx)
{
  PetscErrorCode      ierr;
  SNESConvergedReason  reason;
  PetscInt            its;
  
  PetscFunctionBegin;
  ierr = SNESSolve(ctx->snesU,NULL,fields->U);CHKERRQ(ierr);
  ierr = SNESGetConvergedReason(ctx->snesU,&reason);CHKERRQ(ierr);
  if (reason < 0) {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"[ERROR] snesU diverged with reason %d\n",(int)reason);CHKERRQ(ierr);
  } else {
    ierr = SNESGetIterationNumber(ctx->snesU,&its);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"      snesU converged in %d iterations %d.\n",(int)its,(int)reason);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_UResidual"
extern PetscErrorCode VF_UResidual(SNES snesU,Vec U,Vec residual,void *user)
{
  VFCtx          *ctx=(VFCtx*)user;

  PetscErrorCode ierr;
  PetscInt       xs,xm,nx;
  PetscInt       ys,ym,ny;
  PetscInt       zs,zm,nz;
  PetscInt       ei,ej,ek;
  PetscInt       i,j,k,c,l;
  PetscInt       i1,j1,k1,c1;
  PetscInt       i2,j2,k2,c2;
  PetscInt       dim  = 3;
  PetscInt       nrow = dim * ctx->e3D.nphix * ctx->e3D.nphiy * ctx->e3D.nphiz;
  Vec            residual_localVec,V_localVec,U_localVec;
  Vec            theta_localVec,thetaRef_localVec;
  Vec            pressure_localVec;
  PetscReal      ****residual_array,***v_array,****u_array;
  PetscReal      ***theta_array,***thetaRef_array;
  PetscReal      ***pressure_array;
  PetscReal      *residual_local,*bilinearForm_local;
  PetscReal      hx,hy,hz;
  PetscReal      ****coords_array;
  PetscReal      ****f_array;
  Vec            f_localVec;
  FACE           face;
  PetscReal      z;
  int            stresscomp[3];
  PetscReal      stressdir[3];
  PetscReal      stressmag;
  PetscReal      BBmin[3],BBmax[3];
  
  PetscFunctionBegin;
  ierr = VecSet(residual,0.0);CHKERRQ(ierr);
  ierr = DMDAGetInfo(ctx->daScalCell,NULL,&nx,&ny,&nz,NULL,NULL,NULL,
                     NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
  ierr = DMDAGetCorners(ctx->daScalCell,&xs,&ys,&zs,&xm,&ym,&zm);CHKERRQ(ierr);
  /*
   Get coordinates
   */
  ierr = DMDAVecGetArrayDOF(ctx->daVect,ctx->coordinates,&coords_array);CHKERRQ(ierr);
  /*
   Get bounding box from petsc DA
   */
  ierr = DMDAGetBoundingBox(ctx->daVect,BBmin,BBmax);CHKERRQ(ierr);
  /*
   get v_array
   */
  ierr = DMGetLocalVector(ctx->daScal,&V_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daScal,ctx->fields->V,INSERT_VALUES,V_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daScal,ctx->fields->V,INSERT_VALUES,V_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,V_localVec,&v_array);CHKERRQ(ierr);
  /*
   get u_array
   */
  ierr = DMGetLocalVector(ctx->daVect,&U_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daVect,U,INSERT_VALUES,U_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daVect,U,INSERT_VALUES,U_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArrayDOF(ctx->daVect,U_localVec,&u_array);CHKERRQ(ierr);
  /*
   get theta_array, thetaRef_array
   */
  ierr = DMGetLocalVector(ctx->daScal,&theta_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daScal,ctx->fields->theta,INSERT_VALUES,theta_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daScal,ctx->fields->theta,INSERT_VALUES,theta_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,theta_localVec,&theta_array);CHKERRQ(ierr);
  ierr = DMGetLocalVector(ctx->daScal,&thetaRef_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daScal,ctx->fields->thetaRef,INSERT_VALUES,thetaRef_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daScal,ctx->fields->thetaRef,INSERT_VALUES,thetaRef_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,thetaRef_localVec,&thetaRef_array);CHKERRQ(ierr);
  /*
   get pressure_array
   */
  ierr = DMGetLocalVector(ctx->daScal,&pressure_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daScal,ctx->fields->pressure,INSERT_VALUES,pressure_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daScal,ctx->fields->pressure,INSERT_VALUES,pressure_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,pressure_localVec,&pressure_array);CHKERRQ(ierr);
  /*
   Allocating multi-dimensional vectors in C is a pain, so for the in-situ stresses / surface stresses, I get a
   3 component Vec for the external forces, then get a full array. This is a bit wastefull since we never use the
   internal values...
   Note that we cannot fully initialize it since we need the normal direction to each face.
   */
  if (ctx->hasInsitu) {
    ierr = DMGetLocalVector(ctx->daVect,&f_localVec);CHKERRQ(ierr);
    ierr = DMDAVecGetArrayDOF(ctx->daVect,f_localVec,&f_array);CHKERRQ(ierr);
  }
  /*
   get local mat and RHS
   */
  ierr = PetscMalloc2(nrow,&residual_local,
                      nrow * nrow,&bilinearForm_local);CHKERRQ(ierr);
  ierr = DMGetLocalVector(ctx->daVect,&residual_localVec);CHKERRQ(ierr);
  ierr = VecSet(residual_localVec,0.);CHKERRQ(ierr);
  ierr = DMDAVecGetArrayDOF(ctx->daVect,residual_localVec,&residual_array);CHKERRQ(ierr);
 
  /*
   loop through all elements (ei,ej)
   */
  for (ek = zs; ek < zs + zm; ek++) {
    for (ej = ys; ej < ys+ym; ej++) {
      for (ei = xs; ei < xs+xm; ei++) {
        hx   = coords_array[ek][ej][ei+1][0]-coords_array[ek][ej][ei][0];
        hy   = coords_array[ek][ej+1][ei][1]-coords_array[ek][ej][ei][1];
        hz   = coords_array[ek+1][ej][ei][2]-coords_array[ek][ej][ei][2];
        ierr = VFCartFEElement3DInit(&ctx->e3D,hx,hy,hz);CHKERRQ(ierr);
        /*
          Compute and accumulate the local contribution of the bilinear form
        */
        for (l = 0; l < nrow * nrow; l++) bilinearForm_local[l] = 0.;
        switch (ctx->unilateral) {
          case UNILATERAL_NONE:
            ierr = VF_BilinearFormU3D_local(bilinearForm_local,v_array,&ctx->matprop[ctx->layer[ek]],&ctx->vfprop,
                                   ek,ej,ei,&ctx->e3D);
            break;
          case UNILATERAL_NOCOMPRESSION:
            ierr = VF_BilinearFormUNoCompression3D_local(bilinearForm_local,u_array,v_array,&ctx->matprop[ctx->layer[ek]],&ctx->vfprop,
                                            ek,ej,ei,&ctx->e3D);
            break;
        }
        /*
          Accumulate residual += BilinearForm . U in local indexing
          Note that the local indexing for matrices and arrays are different...
        */
        for (l = 0,k1 = 0; k1 < ctx->e3D.nphiz; k1++) {
          for (j1 = 0; j1 < ctx->e3D.nphiy; j1++) {
            for (i1 = 0; i1 < ctx->e3D.nphix; i1++) {
              for (c1 = 0; c1 < ctx->e3D.dim; c1++) {
                for (k2 = 0; k2 < ctx->e3D.nphiz; k2++) {
                  for (j2 = 0; j2 < ctx->e3D.nphiy; j2++) {
                    for (i2 = 0; i2 < ctx->e3D.nphix; i2++) {
                      for (c2 = 0; c2 < ctx->e3D.dim; c2++,l++) {
                        residual_array[ek+k1][ej+j1][ei+i1][c1] += bilinearForm_local[l] * u_array[ek+k2][ej+j2][ei+i2][c2];
                      }
                    }
                  }
                }
              }
            }
          }
        }
        /*
         Compute and accumulate the local contribution of the effective strain contribution to the global RHS
         */
        for (l = 0; l < nrow; l++) residual_local[l] = 0.;
        switch (ctx->unilateral) {
          case UNILATERAL_NONE:
            ierr = VF_GradientUThermoPoro3D_local(residual_local,v_array,theta_array,thetaRef_array,pressure_array,
                                           &ctx->matprop[ctx->layer[ek]],&ctx->vfprop,
                                           ek,ej,ei,&ctx->e3D);CHKERRQ(ierr);
            break;
          case UNILATERAL_NOCOMPRESSION:
            ierr = VF_GradientUThermoPoroNoCompression3D_local(residual_local,u_array,v_array,theta_array,thetaRef_array,
                                                    pressure_array,&ctx->matprop[ctx->layer[ek]],&ctx->vfprop,
                                                    ek,ej,ei,&ctx->e3D);CHKERRQ(ierr);
            break;
        }
        if (ctx->hasCrackPressure) {
          ierr = VF_GradientUCrackPressure3D_local(residual_local,v_array,pressure_array,
                                                    &ctx->matprop[ctx->layer[ek]],&ctx->vfprop,ek,ej,ei,&ctx->e3D);CHKERRQ(ierr);
        }
        for (l = 0,k = 0; k < ctx->e3D.nphiz; k++) {
          for (j = 0; j < ctx->e3D.nphiy; j++) {
            for (i = 0; i < ctx->e3D.nphix; i++) {
              for (c = 0; c < dim; c++,l++) {
                residual_array[ek+k][ej+j][ei+i][c] -= residual_local[l];
                residual_local[l]                    = 0;
              }
            }
          }
        }
        /*
         Compute and accumulate the local contribution of the insitu stresses to the global RHS
         */
        if (ctx->hasInsitu) {
          if (ek == 0) {
            /*
             Face Z0
             sigma.(0,0,-1) = (-s_13,-s_23,-s_33) = (-S4,-S3,-S2)
             */
            face          = Z0;
            stresscomp[0] = 4; stressdir[0] = -1.;
            stresscomp[1] = 3; stressdir[1] = -1.;
            stresscomp[2] = 2; stressdir[2] = -1.;
            for (c = 0; c < 3; c++) {
              if (ctx->bcU[c].face[face] == NONE) {
                for (k = 0; k < ctx->e3D.nphiz; k++) {
                  z = coords_array[ek+k][ej][ei][2];
                  stressmag = stressdir[c] *
                  (ctx->insitumin[stresscomp[c]] + (z - BBmin[2]) / (BBmax[2] - BBmin[2])
                   * (ctx->insitumax[stresscomp[c]] - ctx->insitumin[stresscomp[c]]));
                  for (j = 0; j < ctx->e3D.nphiy; j++)
                    for (i = 0; i < ctx->e3D.nphix; i++)
                      f_array[ek+k][ej+j][ei+i][c] = stressmag;
                }
              }
            }
            ierr = VF_GradientUInSituStresses3D_local(residual_local,f_array,ek,ej,ei,face,&ctx->e3D);CHKERRQ(ierr);
            for (l= 0,k = 0; k < ctx->e3D.nphiz; k++) {
              for (j = 0; j < ctx->e3D.nphiy; j++) {
                for (i = 0; i < ctx->e3D.nphix; i++) {
                  for (c = 0; c < dim; c++,l++) {
                    residual_array[0][ej+j][ei+i][c] -= residual_local[l];
                    residual_local[l]                 = 0;
                  }
                }
              }
            }
          }
          if (ek == nz-1) {
            /*
             Face Z1
             sigma.(0,0,1) = (s_13,s_23,s_33) = (S4,S3,S2)
             */
            face          = Z1;
            stresscomp[0] = 4; stressdir[0] = 1.;
            stresscomp[1] = 3; stressdir[1] = 1.;
            stresscomp[2] = 2; stressdir[2] = 1.;
            for (k = 0; k < ctx->e3D.nphiz; k++) {
              for (j = 0; j < ctx->e3D.nphiy; j++) {
                for (i = 0; i < ctx->e3D.nphix; i++) {
                  z = coords_array[ek+k][ej+j][ei+i][2];
                  for (c = 0; c < 3; c++) {
                    if (ctx->bcU[c].face[face] == NONE) {
                      f_array[ek+k][ej+j][ei+i][c] = stressdir[c] *
                      (ctx->insitumin[stresscomp[c]] +
                       (z - BBmin[2]) / (BBmax[2] - BBmin[2]) *
                       (ctx->insitumax[stresscomp[c]] - ctx->insitumin[stresscomp[c]]));
                    }
                  }
                }
              }
            }
            ierr = VF_GradientUInSituStresses3D_local(residual_local,f_array,ek,ej,ei,face,&ctx->e3D);CHKERRQ(ierr);
            for (l= 0,k = 0; k < ctx->e3D.nphiz; k++) {
              for (j = 0; j < ctx->e3D.nphiy; j++) {
                for (i = 0; i < ctx->e3D.nphix; i++) {
                  for (c = 0; c < dim; c++,l++) {
                    residual_array[nz][ej+j][ei+i][c] -= residual_local[l];
                    residual_local[l]                  = 0;
                  }
                }
              }
            }
          }
          
          if (ej == 0) {
            /*
             Face Y0
             sigma.(0,-1,0) = (-s_12,-s_22,-s_23) = (-S5,-S1,-S3)
             */
            face          = Y0;
            stresscomp[0] = 5; stressdir[0] = -1.;
            stresscomp[1] = 1; stressdir[1] = -1.;
            stresscomp[2] = 3; stressdir[2] = -1.;
            for (k = 0; k < ctx->e3D.nphiz; k++) {
              for (j = 0; j < ctx->e3D.nphiy; j++) {
                for (i = 0; i < ctx->e3D.nphix; i++) {
                  z = coords_array[ek+k][ej+j][ei+i][2];
                  for (c = 0; c < 3; c++) {
                    if (ctx->bcU[c].face[face] == NONE) {
                      f_array[ek+k][ej+j][ei+i][c] = stressdir[c] *
                      (ctx->insitumin[stresscomp[c]] +
                       (z - BBmin[2]) / (BBmax[2] - BBmin[2]) *
                       (ctx->insitumax[stresscomp[c]] - ctx->insitumin[stresscomp[c]]));
                    }
                  }
                }
              }
            }
            ierr = VF_GradientUInSituStresses3D_local(residual_local,f_array,ek,ej,ei,face,&ctx->e3D);CHKERRQ(ierr);
            for (l= 0,k = 0; k < ctx->e3D.nphiz; k++) {
              for (j = 0; j < ctx->e3D.nphiy; j++) {
                for (i = 0; i < ctx->e3D.nphix; i++) {
                  for (c = 0; c < dim; c++,l++) {
                    residual_array[ek+k][0][ei+i][c] -= residual_local[l];
                    residual_local[l]                 = 0;
                  }
                }
              }
            }
          }
          if (ej == ny-1) {
            /*
             Face Y1
             sigma.(0,1,0) = (s_12,s_22,s_23) = (S5,S1,S3)
             */
            face          = Y1;
            stresscomp[0] = 5; stressdir[0] = 1.;
            stresscomp[1] = 1; stressdir[1] = 1.;
            stresscomp[2] = 3; stressdir[2] = 1.;
            for (k = 0; k < ctx->e3D.nphiz; k++) {
              for (j = 0; j < ctx->e3D.nphiy; j++) {
                for (i = 0; i < ctx->e3D.nphix; i++) {
                  z = coords_array[ek+k][ej+j][ei+i][2];
                  for (c = 0; c < 3; c++) {
                    if (ctx->bcU[c].face[face] == NONE) {
                      f_array[ek+k][ej+j][ei+i][c] = stressdir[c] *
                      (ctx->insitumin[stresscomp[c]] +
                       (z - BBmin[2]) / (BBmax[2] - BBmin[2]) *
                       (ctx->insitumax[stresscomp[c]] - ctx->insitumin[stresscomp[c]]));
                    }
                  }
                }
              }
            }
            ierr = VF_GradientUInSituStresses3D_local(residual_local,f_array,ek,ej,ei,face,&ctx->e3D);CHKERRQ(ierr);
            for (l= 0,k = 0; k < ctx->e3D.nphiz; k++) {
              for (j = 0; j < ctx->e3D.nphiy; j++) {
                for (i = 0; i < ctx->e3D.nphix; i++) {
                  for (c = 0; c < dim; c++,l++) {
                    residual_array[ek+k][ny][ei+i][c] -= residual_local[l];
                    residual_local[l]                    = 0;
                  }
                }
              }
            }
          }
          
          if (ei == 0) {
            /*
             Face X0
             sigma.(-1,0,0) = (-s_11,-s_12,-s_13) = (-S0,-S5,-S4)
             */
            face          = X0;
            stresscomp[0] = 0; stressdir[0] = -1.;
            stresscomp[1] = 5; stressdir[1] = -1.;
            stresscomp[2] = 4; stressdir[2] = -1.;
            for (k = 0; k < ctx->e3D.nphiz; k++) {
              for (j = 0; j < ctx->e3D.nphiy; j++) {
                for (i = 0; i < ctx->e3D.nphix; i++) {
                  z = coords_array[ek+k][ej+j][ei+i][2];
                  for (c = 0; c < 3; c++) {
                    if (ctx->bcU[c].face[face] == NONE) {
                      f_array[ek+k][ej+j][ei+i][c] = stressdir[c] *
                      (ctx->insitumin[stresscomp[c]] +
                       (z - BBmin[2]) / (BBmax[2] - BBmin[2]) *
                       (ctx->insitumax[stresscomp[c]] - ctx->insitumin[stresscomp[c]]));
                    }
                  }
                }
              }
            }
            ierr = VF_GradientUInSituStresses3D_local(residual_local,f_array,ek,ej,ei,face,&ctx->e3D);CHKERRQ(ierr);
            for (l= 0,k = 0; k < ctx->e3D.nphiz; k++) {
              for (j = 0; j < ctx->e3D.nphiy; j++) {
                for (i = 0; i < ctx->e3D.nphix; i++) {
                  for (c = 0; c < dim; c++,l++) {
                    residual_array[ek+k][ej+j][0][c] -= residual_local[l];
                    residual_local[l]                 = 0;
                  }
                }
              }
            }
          }
          if (ei == nx-1) {
            /*
             Face X1
             sigma.(1,0,0) = (s_11,s_12,s_13) = (S0,S5,S4)
             the negative sign in the 3rd component comes from thaty the z-axis is pointing down
             */
            face          = X1;
            stresscomp[0] = 0; stressdir[0] = 1.;
            stresscomp[1] = 5; stressdir[1] = 1.;
            stresscomp[2] = 4; stressdir[2] = 1.;
            for (k = 0; k < ctx->e3D.nphiz; k++) {
              for (j = 0; j < ctx->e3D.nphiy; j++) {
                for (i = 0; i < ctx->e3D.nphix; i++) {
                  z = coords_array[ek+k][ej+j][ei+i][2];
                  for (c = 0; c < 3; c++) {
                    if (ctx->bcU[c].face[face] == NONE) {
                      f_array[ek+k][ej+j][ei+i][c] = stressdir[c] *
                      (ctx->insitumin[stresscomp[c]] +
                       (z - BBmin[2]) / (BBmax[2] - BBmin[2]) *
                       (ctx->insitumax[stresscomp[c]] - ctx->insitumin[stresscomp[c]]));
                    }
                  }
                }
              }
            }
            ierr = VF_GradientUInSituStresses3D_local(residual_local,f_array,ek,ej,ei,face,&ctx->e3D);CHKERRQ(ierr);
            for (l= 0,k = 0; k < ctx->e3D.nphiz; k++) {
              for (j = 0; j < ctx->e3D.nphiy; j++) {
                for (i = 0; i < ctx->e3D.nphix; i++) {
                  for (c = 0; c < dim; c++,l++) {
                    residual_array[ek+k][ej+j][nx][c] -= residual_local[l];
                    residual_local[l]                    = 0;
                  }
                }
              }
            }
          }
        }
        /*
         Jump to next element
         */
      }
    }
  }
  /*
   Global Assembly and Boundary Conditions
   */
  ierr = DMDAVecRestoreArrayDOF(ctx->daVect,residual_localVec,&residual_array);CHKERRQ(ierr);
  ierr = DMLocalToGlobalBegin(ctx->daVect,residual_localVec,ADD_VALUES,residual);CHKERRQ(ierr);
  ierr = DMLocalToGlobalEnd(ctx->daVect,residual_localVec,ADD_VALUES,residual);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daVect,&residual_localVec);CHKERRQ(ierr);

  //ierr = residualApplyDirichletBC(residual,&ctx->bcU[0]);CHKERRQ(ierr);
  ierr = ResidualApplyDirichletBC(residual,U,ctx->fields->BCU,&ctx->bcU[0]);CHKERRQ(ierr);
  /*
   Cleanup
   */
  ierr = DMDAVecRestoreArrayDOF(ctx->daVect,ctx->coordinates,&coords_array);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(ctx->daScal,V_localVec,&v_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daScal,&V_localVec);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(ctx->daScal,theta_localVec,&theta_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daScal,&theta_localVec);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(ctx->daScal,thetaRef_localVec,&thetaRef_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daScal,&thetaRef_localVec);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(ctx->daScal,pressure_localVec,&pressure_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daScal,&pressure_localVec);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArrayDOF(ctx->daVect,U_localVec,&u_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daVect,&U_localVec);CHKERRQ(ierr);

  if (ctx->hasInsitu) {
    ierr = DMDAVecRestoreArrayDOF(ctx->daVect,f_localVec,&f_array);CHKERRQ(ierr);
    ierr = DMRestoreLocalVector(ctx->daVect,&f_localVec);CHKERRQ(ierr);
  }
  ierr = PetscFree2(residual_local,bilinearForm_local);CHKERRQ(ierr);
 
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_UIJacobian"
/*
  
  VF_UIJacobian:
  
  (c) 2014 Blaise Bourdin bourdin@lsu.edu
*/
extern PetscErrorCode VF_UIJacobian(SNES snesU,Vec U,Mat K,Mat KPC,void *user)
{
  VFCtx          *ctx=(VFCtx*)user;
  
  PetscErrorCode ierr;
  PetscInt       xs,xm,nx;
  PetscInt       ys,ym,ny;
  PetscInt       zs,zm,nz;
  PetscInt       ei,ej,ek,i,j,k,c,l;
  PetscInt       dim  = 3;
  PetscInt       nrow = dim * ctx->e3D.nphix * ctx->e3D.nphiy * ctx->e3D.nphiz;
  Vec            V_localVec,U_localVec;
  PetscReal      ***v_array,****u_array;
  PetscReal      *bilinearForm_local,*bilinearFormPC_local;
  MatStencil     *row;
  PetscReal      hx,hy,hz;
  PetscReal      ****coords_array;
  VFProp         PCvfprop;
  VFMatProp      PCmatprop;
  
  PetscFunctionBegin;
  ierr = DMDAGetInfo(ctx->daScalCell,NULL,&nx,&ny,&nz,NULL,NULL,NULL,
                     NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
  ierr = DMDAGetCorners(ctx->daScalCell,&xs,&ys,&zs,&xm,&ym,&zm);CHKERRQ(ierr);
  
  ierr = MatZeroEntries(K);CHKERRQ(ierr);
  if (KPC != K) {
    ierr = MatZeroEntries(KPC);CHKERRQ(ierr);
  }
  /*
   Get coordinates
   */
  ierr = DMDAVecGetArrayDOF(ctx->daVect,ctx->coordinates,&coords_array);CHKERRQ(ierr);

  /*
   get v_array
   */
  ierr = DMGetLocalVector(ctx->daScal,&V_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daScal,ctx->fields->V,INSERT_VALUES,V_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daScal,ctx->fields->V,INSERT_VALUES,V_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,V_localVec,&v_array);CHKERRQ(ierr);

  /*
    get u_array if necessary
  */
  if (ctx->unilateral == UNILATERAL_NOCOMPRESSION) {
    ierr = DMGetLocalVector(ctx->daVect,&U_localVec);CHKERRQ(ierr);
    ierr = DMGlobalToLocalBegin(ctx->daVect,U,INSERT_VALUES,U_localVec);CHKERRQ(ierr);
    ierr = DMGlobalToLocalEnd(ctx->daVect,U,INSERT_VALUES,U_localVec);CHKERRQ(ierr);
    ierr = DMDAVecGetArrayDOF(ctx->daVect,U_localVec,&u_array);CHKERRQ(ierr);
    ierr = MatZeroEntries(KPC);CHKERRQ(ierr);
  }
  /*
   get local mat
   */
  ierr = PetscMalloc3(nrow * nrow,&bilinearForm_local,
                      nrow * nrow,&bilinearFormPC_local,
                      nrow,&row);CHKERRQ(ierr);

  /*
   loop through all elements (ei,ej)
   */
  for (ek = zs; ek < zs + zm; ek++) {
    for (ej = ys; ej < ys+ym; ej++) {
      for (ei = xs; ei < xs+xm; ei++) {
        hx   = coords_array[ek][ej][ei+1][0]-coords_array[ek][ej][ei][0];
        hy   = coords_array[ek][ej+1][ei][1]-coords_array[ek][ej][ei][1];
        hz   = coords_array[ek+1][ej][ei][2]-coords_array[ek][ej][ei][2];
        ierr = VFCartFEElement3DInit(&ctx->e3D,hx,hy,hz);CHKERRQ(ierr);
        /*
         Compute and accumulate the contribution of the local stiffness matrix to the global stiffness matrix
         */
        ierr = PetscMemzero(bilinearForm_local,nrow * nrow * sizeof(PetscReal));
        ierr = PetscMemzero(bilinearFormPC_local,nrow * nrow * sizeof(PetscReal));
        switch (ctx->unilateral) {
          case UNILATERAL_NONE:
            ierr = VF_BilinearFormU3D_local(bilinearForm_local,v_array,&ctx->matprop[ctx->layer[ek]],&ctx->vfprop,
                                            ek,ej,ei,&ctx->e3D); 
            ierr = PetscMemcpy(bilinearFormPC_local,bilinearForm_local,nrow * nrow * sizeof(PetscReal));
            break;
          case UNILATERAL_NOCOMPRESSION:
            ierr = VF_BilinearFormUNoCompression3D_local(bilinearForm_local,u_array,v_array,&ctx->matprop[ctx->layer[ek]],&ctx->vfprop,
                                                         ek,ej,ei,&ctx->e3D);
            if (KPC != K) {
              ierr = PetscMemcpy(&PCvfprop,&ctx->vfprop,sizeof(VFProp));CHKERRQ(ierr);
              PCvfprop.eta = PCvfprop.PCeta;
              ierr = PetscMemcpy(&PCmatprop,&ctx->matprop[ctx->layer[ek]],sizeof(VFMatProp));CHKERRQ(ierr);
              ierr = VF_BilinearFormUNoCompression3D_local(bilinearFormPC_local,u_array,v_array,&PCmatprop,&PCvfprop,ek,ej,ei,&ctx->e3D);
            }
            break;
        }
        
        for (l = 0,k = 0; k < ctx->e3D.nphiz; k++) {
          for (j = 0; j < ctx->e3D.nphiy; j++) {
            for (i = 0; i < ctx->e3D.nphix; i++) {
              for (c = 0; c < dim; c++,l++) {
                row[l].i = ei + i; row[l].j = ej + j; row[l].k = ek + k; row[l].c = c;
              }
            }
          }
        }

        ierr = MatSetValuesStencil(K,nrow,row,nrow,row,bilinearForm_local,ADD_VALUES);CHKERRQ(ierr); 
        if (KPC != K) {
          ierr = MatSetValuesStencil(KPC,nrow,row,nrow,row,bilinearFormPC_local,ADD_VALUES);CHKERRQ(ierr); 
        }
      }
    }
  }
  ierr = MatAssemblyBegin(K,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(K,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);  
  ierr = MatApplyDirichletBC(K,&ctx->bcU[0]);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(K,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(K,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  if (KPC != K) {
      ierr = MatAssemblyBegin(KPC,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
      ierr = MatAssemblyEnd(KPC,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);  
      ierr = MatApplyDirichletBC(KPC,&ctx->bcU[0]);CHKERRQ(ierr);
      ierr = MatAssemblyBegin(KPC,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
      ierr = MatAssemblyEnd(KPC,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  }

  ierr = DMDAVecRestoreArrayDOF(ctx->daVect,ctx->coordinates,&coords_array);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(ctx->daScal,V_localVec,&v_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daScal,&V_localVec);CHKERRQ(ierr);
  if (ctx->unilateral == UNILATERAL_NOCOMPRESSION) {
    ierr = DMDAVecRestoreArrayDOF(ctx->daVect,U_localVec,&u_array);CHKERRQ(ierr);
    ierr = DMRestoreLocalVector(ctx->daVect,&U_localVec);CHKERRQ(ierr);
  }
  ierr = PetscFree3(bilinearForm_local,bilinearFormPC_local,row);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


/* ====================== */
#undef __FUNCT__
#define __FUNCT__ "VF_BilinearFormVAT23D_local"
/*
 VF_BilinearFormVAT23D_local
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_BilinearFormVAT23D_local(PetscReal *Mat_local,VFMatProp *matprop,VFProp *vfprop,VFCartFEElement3D *e)
{
  PetscInt  g,i1,i2,j1,j2,k1,k2,l;
  PetscReal coef = matprop->Gc / vfprop->atCv * .5;
  
  PetscFunctionBegin;
  for (l = 0,k1 = 0; k1 < e->nphiz; k1++)
    for (j1 = 0; j1 < e->nphiy; j1++)
      for (i1 = 0; i1 < e->nphix; i1++)
        for (k2 = 0; k2 < e->nphiz; k2++)
          for (j2 = 0; j2 < e->nphiy; j2++)
            for (i2 = 0; i2 < e->nphix; i2++,l++)
              for (g = 0; g < e->ng; g++)
                Mat_local[l] += coef * e->weight[g] * (e->phi[k1][j1][i1][g] *     e->phi[k2][j2][i2][g] / vfprop->epsilon + (
                                                        e->dphi[k1][j1][i1][0][g] * e->dphi[k2][j2][i2][0][g]
                                                      + e->dphi[k1][j1][i1][1][g] * e->dphi[k2][j2][i2][1][g]
                                                      + e->dphi[k1][j1][i1][2][g] * e->dphi[k2][j2][i2][2][g] ) * vfprop->epsilon);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_BilinearFormVAT13D_local"
/*
 VF_BilinearFormVAT13D_local
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_BilinearFormVAT13D_local(PetscReal *Mat_local,VFMatProp *matprop,VFProp *vfprop,VFCartFEElement3D *e)
{
  PetscInt  g,i1,i2,j1,j2,k1,k2,l;
  PetscReal coef = matprop->Gc / vfprop->atCv * .5;
  
  PetscFunctionBegin;
  for (l = 0,k1 = 0; k1 < e->nphiz; k1++)
    for (j1 = 0; j1 < e->nphiy; j1++)
      for (i1 = 0; i1 < e->nphix; i1++)
        for (k2 = 0; k2 < e->nphiz; k2++)
          for (j2 = 0; j2 < e->nphiy; j2++)
            for (i2 = 0; i2 < e->nphix; i2++,l++)
              for (g = 0; g < e->ng; g++)
                Mat_local[l] += coef * e->weight[g] * (e->dphi[k1][j1][i1][0][g] * e->dphi[k2][j2][i2][0][g]
                                                     + e->dphi[k1][j1][i1][1][g] * e->dphi[k2][j2][i2][1][g]
                                                     + e->dphi[k1][j1][i1][2][g] * e->dphi[k2][j2][i2][2][g]) * vfprop->epsilon;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_BilinearFormVCoupling3D_local"
/*
 VF_BilinearFormVCoupling3D_local
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_BilinearFormVCoupling3D_local(PetscReal *Mat_local,PetscReal ****U_array,PetscReal ***theta_array,PetscReal ***thetaRef_array,VFMatProp *matprop,VFProp *vfprop,PetscInt ek,PetscInt ej,PetscInt ei,VFCartFEElement3D *e)
{
  PetscErrorCode ierr;
  PetscInt       g,i1,i2,j1,j2,k1,k2,l;
  PetscReal      *ElasticEnergyDensity_local;
  
  PetscFunctionBegin;
  ierr = PetscMalloc(e->ng * sizeof(PetscReal),&ElasticEnergyDensity_local);CHKERRQ(ierr);
  for (g = 0; g < e->ng; g++) ElasticEnergyDensity_local[g] = 0;
  ierr = ElasticEnergyDensity3D_local(ElasticEnergyDensity_local,U_array,
                                      theta_array,thetaRef_array,matprop,ek,ej,ei,e);CHKERRQ(ierr);
  
  for (l = 0,k1 = 0; k1 < e->nphiz; k1++)
    for (j1 = 0; j1 < e->nphiy; j1++)
      for (i1 = 0; i1 < e->nphix; i1++)
        for (k2 = 0; k2 < e->nphiz; k2++)
          for (j2 = 0; j2 < e->nphiy; j2++)
            for (i2 = 0; i2 < e->nphix; i2++,l++)
              for (g = 0; g < e->ng; g++)
                Mat_local[l] += e->weight[g] * e->phi[k1][j1][i1][g] * e->phi[k2][j2][i2][g] * ElasticEnergyDensity_local[g] * 2.;
  ierr = PetscFree(ElasticEnergyDensity_local);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VF_BilinearFormVCouplingNoCompression3D_local"
/*
 VF_BilinearFormVCouplingNoCompression3D_local
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_BilinearFormVCouplingNoCompression3D_local(PetscReal *Mat_local,PetscReal ****U_array,PetscReal ***theta_array,PetscReal ***thetaRef_array,VFMatProp *matprop,VFProp *vfprop,PetscInt ek,PetscInt ej,PetscInt ei,VFCartFEElement3D *e)
{
  PetscErrorCode ierr;
  PetscInt       g,i1,i2,j1,j2,k1,k2,l;
  PetscReal      *ElasticEnergyDensityS_local,*ElasticEnergyDensityD_local;
  
  PetscFunctionBegin;
  ierr = PetscMalloc2(e->ng,&ElasticEnergyDensityS_local,
                      e->ng,&ElasticEnergyDensityD_local);CHKERRQ(ierr);
  ierr = ElasticEnergyDensitySphericalDeviatoricNoCompression3D_local(ElasticEnergyDensityS_local,ElasticEnergyDensityD_local,
                                                         U_array,theta_array,thetaRef_array,matprop,ek,ej,ei,e);CHKERRQ(ierr);
  for (l = 0,k1 = 0; k1 < e->nphiz; k1++) {
    for (j1 = 0; j1 < e->nphiy; j1++) {
      for (i1 = 0; i1 < e->nphix; i1++) {
        for (k2 = 0; k2 < e->nphiz; k2++) {
          for (j2 = 0; j2 < e->nphiy; j2++) {
            for (i2 = 0; i2 < e->nphix; i2++,l++) {
              for (g = 0; g < e->ng; g++) {
                Mat_local[l] += e->weight[g] * e->phi[k1][j1][i1][g] * e->phi[k2][j2][i2][g] * (ElasticEnergyDensityS_local[g] + ElasticEnergyDensityD_local[g]) * 2.;
              }
            }
          }
        }
      }
    }
  }
  ierr = PetscFree2(ElasticEnergyDensityS_local,ElasticEnergyDensityD_local);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_ResidualVThermoPoro3D_local"
/*
  
  VF_ResidualVThermoPoro3D_local: local assembly of the contribution of the thermo and poroelasticity terms to the V residual
  
  (c) 2014 Blaise Bourdin bourdin@lsu.edu
*/
extern PetscErrorCode VF_ResidualVThermoPoro3D_local(PetscReal *residual_local,PetscReal ****u_array,PetscReal ***theta_array,PetscReal ***thetaRef_array,PetscReal ***pressure_array,VFMatProp *matprop,VFProp *vfprop,PetscInt ek,PetscInt ej,PetscInt ei,VFCartFEElement3D *e)
{
  PetscErrorCode ierr;
  PetscInt       l,i,j,k,g,c;
  PetscReal      *Dtheta_elem,*pressure_elem,*divu_elem;
  PetscReal      coefalpha,beta;
  
  PetscFunctionBegin;
  coefalpha = (3.* matprop->lambda + 2. * matprop->mu) * matprop->alpha;
  beta      = matprop->beta;
  
  ierr = PetscMalloc3(e->ng,&Dtheta_elem,
                      e->ng,&pressure_elem,
                      e->ng,&divu_elem);CHKERRQ(ierr);
  /*
   Initialize pressure_Elem, theta_Elem and v_elem
   */
  for (g = 0; g < e->ng; g++) {
    Dtheta_elem[g]   = 0.;
    pressure_elem[g] = 0.;
    divu_elem[g]     = 0.;
  }
  /*
   Compute theta_elem
   */
  for (g = 0; g < e->ng; g++) {
    for (k = 0; k < e->nphiz; k++) {
      for (j = 0; j < e->nphiy; j++) {
        for (i = 0; i < e->nphix; i++) {
          Dtheta_elem[g] += e->phi[k][j][i][g] * (theta_array[ek+k][ej+j][ei+i] - thetaRef_array[ek+k][ej+j][ei+i]);
          pressure_elem[g] += e->phi[k][j][i][g] * pressure_array[ek+k][ej+j][ei+i];
          for (c = 0; c < 3; c++) {
            divu_elem[g] += e->dphi[k][j][i][c][g] * u_array[ek+k][ej+j][ei+i][c];
          }
        }
      }
    }
  }
  ierr = PetscLogFlops(11 * e->ng * e->nphix * e->nphiy * e->nphiz);CHKERRQ(ierr);
  
  for (g = 0; g < e->ng; g++) {
    for (l = 0,k = 0; k < e->nphiz; k++) {
      for (j = 0; j < e->nphiy; j++) {
        for (i = 0; i < e->nphix; i++) {
          residual_local[l] += e->weight[g] * e->phi[k][j][i][g] 
                             * pressure_elem[g] * (coefalpha * Dtheta_elem[g] - beta * divu_elem[g]);
        }
      }
    }
  }
  ierr = PetscLogFlops(7 * e->ng * e->nphix * e->nphiy * e->nphiz);CHKERRQ(ierr);
  ierr = PetscFree3(Dtheta_elem,pressure_elem,divu_elem);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_ResidualVThermoPoroNoCompression3D_local"
/*
  
  VF_ResidualVThermoPoroNoCompression3D_local: local assembly of the contribution of the thermo and poroelasticity terms to the V residual
  
  (c) 2014 Blaise Bourdin bourdin@lsu.edu
*/
extern PetscErrorCode VF_ResidualVThermoPoroNoCompression3D_local(PetscReal *residual_local,PetscReal ****u_array,PetscReal ***theta_array,PetscReal ***thetaRef_array,PetscReal ***pressure_array,VFMatProp *matprop,VFProp *vfprop,PetscInt ek,PetscInt ej,PetscInt ei,VFCartFEElement3D *e)
{
  PetscErrorCode ierr;
  PetscInt       l,i,j,k,g,c;
  PetscReal      *Dtheta_elem,*pressure_elem,*divu_elem,*tr_epsilon;
  PetscReal      coefalpha,beta;
  
  PetscFunctionBegin;
  coefalpha = (3.* matprop->lambda + 2. * matprop->mu) * matprop->alpha;
  beta      = matprop->beta;
  
  ierr = PetscMalloc4(e->ng,&Dtheta_elem,
                      e->ng,&pressure_elem,
                      e->ng,&divu_elem,
                      e->ng,&tr_epsilon);CHKERRQ(ierr);
  /*
   Initialize pressure_Elem, theta_Elem and v_elem
   */
  for (g = 0; g < e->ng; g++) {
    Dtheta_elem[g]    = 0;
    pressure_elem[g] = 0;
    divu_elem[g]        = 0.;
  }
  /*
   Compute theta_elem
   */
  for (g = 0; g < e->ng; g++) {
    for (k = 0; k < e->nphiz; k++) {
      for (j = 0; j < e->nphiy; j++) {
        for (i = 0; i < e->nphix; i++) {
          Dtheta_elem[g] += e->phi[k][j][i][g] * (theta_array[ek+k][ej+j][ei+i] - thetaRef_array[ek+k][ej+j][ei+i]);
          pressure_elem[g] += e->phi[k][j][i][g] * pressure_array[ek+k][ej+j][ei+i];
          for (c = 0; c < 3; c++) {
            divu_elem[g] += e->dphi[k][j][i][c][g] * u_array[ek+k][ej+j][ei+i][c];
          }
          tr_epsilon[g] += e->dphi[k][j][i][0][g] * u_array[ek+k][ej+j][ei+i][0] +
                           e->dphi[k][j][i][1][g] * u_array[ek+k][ej+j][ei+i][1] +
                           e->dphi[k][j][i][2][g] * u_array[ek+k][ej+j][ei+i][2];         
        }
      }
    }
  }
  ierr = PetscLogFlops(11 * e->ng * e->nphix * e->nphiy * e->nphiz);CHKERRQ(ierr);
  
  for (g = 0; g < e->ng; g++) {
    if (tr_epsilon[g] > 0.) {
      for (l = 0,k = 0; k < e->nphiz; k++) {
        for (j = 0; j < e->nphiy; j++) {
          for (i = 0; i < e->nphix; i++) {
            residual_local[l] += e->weight[g] * e->phi[k][j][i][g] 
                               * pressure_elem[g] * (coefalpha * Dtheta_elem[g] - beta * divu_elem[g]);
          }
        }
      }
    }
  }
  ierr = PetscLogFlops(7 * e->ng * e->nphix * e->nphiy * e->nphiz);CHKERRQ(ierr);
  ierr = PetscFree4(Dtheta_elem,pressure_elem,divu_elem,tr_epsilon);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_ResidualVCrackPressure3D_local"
/*
 VF_ResidualVCrackPressure3D_local
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_ResidualVCrackPressure3D_local(PetscReal *residual_local,PetscReal ****u_array,PetscReal ***pressure_array,VFMatProp *matprop,VFProp *vfprop,PetscInt ek,PetscInt ej,PetscInt ei,VFCartFEElement3D *e)
{
  PetscErrorCode ierr;
  PetscInt       l,i,j,k,g,c;
  PetscReal      *pressure_elem,*u_elem[3];
  
  PetscFunctionBegin;
  ierr = PetscMalloc4(e->ng,&pressure_elem,
                      e->ng,&u_elem[0],
                      e->ng,&u_elem[1],
                      e->ng,&u_elem[2]);CHKERRQ(ierr);
  
  /*
   Compute the projection of the fields in the local base functions basis
   */
  for (g = 0; g < e->ng; g++) {
    pressure_elem[g] = 0;
    for (c= 0; c<3; c++) u_elem[c][g] = 0.;
  }
  for (k = 0; k < e->nphiz; k++)
    for (j = 0; j < e->nphiy; j++) {
      for (i = 0; i < e->nphix; i++)
        for (g = 0; g < e->ng; g++) {
          pressure_elem[g] += e->phi[k][j][i][g] * pressure_array[ek+k][ej+j][ei+i];
          for (c= 0; c<3; c++) u_elem[c][g] += e->phi[k][j][i][g] * u_array[ek+k][ej+j][ei+i][c];
        }
    }
  ierr = PetscLogFlops(9 * e->ng * e->nphix * e->nphiy * e->nphiz);CHKERRQ(ierr);
  
  /*
   Accumulate the contribution of the current element to the local
   version of the residual
   */
  for (l= 0,k = 0; k < e->nphiz; k++)
    for (j = 0; j < e->nphiy; j++)
      for (i = 0; i < e->nphix; i++,l++)
        for (c = 0; c < 3; c++)
          for (g = 0; g < e->ng; g++)
            residual_local[l] += e->weight[g] * pressure_elem[g]
            * u_elem[c][g]
            * e->dphi[k][j][i][c][g];
  ierr = PetscLogFlops(12 * e->ng * e->nphix * e->nphiy * e->nphiz);CHKERRQ(ierr);
  /*
   Clean up
   */
  ierr = PetscFree4(pressure_elem,u_elem[0],u_elem[1],u_elem[2]);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_ResidualVAT23D_local"
/*
 VF_ResidualVAT23D_local
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_ResidualVAT23D_local(PetscReal *residual_local,VFMatProp *matprop,VFProp *vfprop,VFCartFEElement3D *e)
{
  PetscInt  g,i,j,k,l;
  PetscReal coef = matprop->Gc / vfprop->atCv / vfprop->epsilon *.5;
  
  PetscFunctionBegin;
  for (l= 0,k= 0; k < e->nphiz; k++)
    for (j = 0; j < e->nphiy; j++)
      for (i = 0; i < e->nphix; i++,l++)
        for (g = 0; g < e->ng; g++)
          residual_local[l] += e->weight[g] * e->phi[k][j][i][g] * coef;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VF_ResidualVAT13D_local"
/*
 VF_ResidualVAT13D_local
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_ResidualVAT13D_local(PetscReal *residual_local,VFMatProp *matprop,VFProp *vfprop,VFCartFEElement3D *e)
{
  PetscInt  g,i,j,k,l;
  PetscReal coef = matprop->Gc / vfprop->atCv / vfprop->epsilon *.25;
  
  PetscFunctionBegin;
  for (l= 0,k= 0; k < e->nphiz; k++)
    for (j = 0; j < e->nphiy; j++)
      for (i = 0; i < e->nphix; i++,l++)
        for (g = 0; g < e->ng; g++)
          residual_local[l] += e->weight[g] * e->phi[k][j][i][g] * coef;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VF_AT2SurfaceEnergy3D_local"
/*
 VF_AT2SurfaceEnergy3D_local:
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_AT2SurfaceEnergy3D_local(PetscReal *SurfaceEnergy_local,PetscReal ***v_array,VFMatProp *matprop,VFProp *vfprop,PetscInt ek,PetscInt ej,PetscInt ei,VFCartFEElement3D *e)
{
  PetscInt       g,i,j,k;
  PetscReal      *v_elem,*gradv_elem[3];
  PetscErrorCode ierr;
  PetscReal      coef = matprop->Gc / vfprop->atCv * .25;
  
  PetscFunctionBegin;
  ierr = PetscMalloc4(e->ng,&v_elem,e->ng,&gradv_elem[0],e->ng,&gradv_elem[1],e->ng,&gradv_elem[2]);CHKERRQ(ierr);
  
  for (g = 0; g < e->ng; g++) {
    v_elem[g]        = 0.;
    gradv_elem[0][g] = 0.;
    gradv_elem[1][g] = 0.;
    gradv_elem[2][g] = 0.;
  }
  for (k = 0; k < e->nphiz; k++)
    for (j = 0; j < e->nphiy; j++) {
      for (i = 0; i < e->nphix; i++)
        for (g = 0; g < e->ng; g++) {
          v_elem[g]        += v_array[ek+k][ej+j][ei+i] * e->phi[k][j][i][g];
          gradv_elem[0][g] += v_array[ek+k][ej+j][ei+i] * e->dphi[k][j][i][0][g];
          gradv_elem[1][g] += v_array[ek+k][ej+j][ei+i] * e->dphi[k][j][i][1][g];
          gradv_elem[2][g] += v_array[ek+k][ej+j][ei+i] * e->dphi[k][j][i][2][g];
        }
    }
  for (g = 0; g < e->ng; g++)
    *SurfaceEnergy_local += e->weight[g] * ((1. - v_elem[g]) * (1. - v_elem[g]) / vfprop->epsilon
                                            + (gradv_elem[0][g] * gradv_elem[0][g] + gradv_elem[1][g] * gradv_elem[1][g] + gradv_elem[2][g] * gradv_elem[2][g]) * vfprop->epsilon) * coef;
  ierr = PetscFree4(v_elem,gradv_elem[0],gradv_elem[1],gradv_elem[2]);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_AT1SurfaceEnergy3D_local"
/*
 VF_AT1SurfaceEnergy3D_local:
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_AT1SurfaceEnergy3D_local(PetscReal *SurfaceEnergy_local,PetscReal ***v_array,VFMatProp *matprop,VFProp *vfprop,PetscInt ek,PetscInt ej,PetscInt ei,VFCartFEElement3D *e)
{
  PetscInt       g,i,j,k;
  PetscReal      *v_elem,*gradv_elem[3];
  PetscErrorCode ierr;
  PetscReal      coef = matprop->Gc / vfprop->atCv * .25;
  
  PetscFunctionBegin;
  ierr = PetscMalloc4(e->ng,&v_elem,e->ng,&gradv_elem[0],e->ng,&gradv_elem[1],e->ng,&gradv_elem[2]);CHKERRQ(ierr);
  
  for (g = 0; g < e->ng; g++) {
    v_elem[g]        = 0.;
    gradv_elem[0][g] = 0.;
    gradv_elem[1][g] = 0.;
    gradv_elem[2][g] = 0.;
  }
  for (k = 0; k < e->nphiz; k++)
    for (j = 0; j < e->nphiy; j++) {
      for (i = 0; i < e->nphix; i++)
        for (g = 0; g < e->ng; g++) {
          v_elem[g]        += v_array[ek+k][ej+j][ei+i] * e->phi[k][j][i][g];
          gradv_elem[0][g] += v_array[ek+k][ej+j][ei+i] * e->dphi[k][j][i][0][g];
          gradv_elem[1][g] += v_array[ek+k][ej+j][ei+i] * e->dphi[k][j][i][1][g];
          gradv_elem[2][g] += v_array[ek+k][ej+j][ei+i] * e->dphi[k][j][i][2][g];
        }
    }
  for (g = 0; g < e->ng; g++)
    *SurfaceEnergy_local += e->weight[g] * ((1. - v_elem[g]) / vfprop->epsilon
                                            + (gradv_elem[0][g] * gradv_elem[0][g] + gradv_elem[1][g] * gradv_elem[1][g] + gradv_elem[2][g] * gradv_elem[2][g]) * vfprop->epsilon) * coef;
  ierr = PetscFree4(v_elem,gradv_elem[0],gradv_elem[1],gradv_elem[2]);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_VEnergy3D"
/*
 VF_VEnergy3D
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_VEnergy3D(PetscReal *SurfaceEnergy,VFFields *fields,VFCtx *ctx)
{
  PetscErrorCode ierr;
  PetscInt       xs,xm,nx;
  PetscInt       ys,ym,ny;
  PetscInt       zs,zm,nz;
  PetscInt       ei,ej,ek;
  Vec            v_localVec;
  PetscReal      ***v_array;
  PetscReal      mySurfaceEnergy= 0.;
  PetscReal      hx,hy,hz;
  PetscReal      ****coords_array;
  
  PetscFunctionBegin;
  
  ierr = DMDAGetInfo(ctx->daScalCell,NULL,&nx,&ny,&nz,NULL,NULL,NULL,
                     NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
  ierr = DMDAGetCorners(ctx->daScalCell,&xs,&ys,&zs,&xm,&ym,&zm);CHKERRQ(ierr);
  
  /*
   Get coordinates, if necessary
   */
  ierr = DMDAVecGetArrayDOF(ctx->daVect,ctx->coordinates,&coords_array);CHKERRQ(ierr);
  ierr = DMGetLocalVector(ctx->daScal,&v_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daScal,fields->V,INSERT_VALUES,v_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daScal,fields->V,INSERT_VALUES,v_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,v_localVec,&v_array);CHKERRQ(ierr);
  
  for (ek = zs; ek < zs + zm; ek++) {
    for (ej = ys; ej < ys + ym; ej++)
      for (ei = xs; ei < xs + xm; ei++) {
        hx   = coords_array[ek][ej][ei+1][0]-coords_array[ek][ej][ei][0];
        hy   = coords_array[ek][ej+1][ei][1]-coords_array[ek][ej][ei][1];
        hz   = coords_array[ek+1][ej][ei][2]-coords_array[ek][ej][ei][2];
        ierr = VFCartFEElement3DInit(&ctx->e3D,hx,hy,hz);CHKERRQ(ierr);
        switch (ctx->vfprop.atnum ) {
          case 1:
            ierr = VF_AT1SurfaceEnergy3D_local(&mySurfaceEnergy,v_array,&ctx->matprop[ctx->layer[ek]],&ctx->vfprop,ek,ej,ei,&ctx->e3D);CHKERRQ(ierr);
            break;
          case 2:
            ierr = VF_AT2SurfaceEnergy3D_local(&mySurfaceEnergy,v_array,&ctx->matprop[ctx->layer[ek]],&ctx->vfprop,ek,ej,ei,&ctx->e3D);CHKERRQ(ierr);
            break;
        }
      }
  }
  *SurfaceEnergy = 0.;
  
  ierr = MPI_Allreduce(&mySurfaceEnergy,SurfaceEnergy,1,MPIU_SCALAR,MPI_SUM,PETSC_COMM_WORLD);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(ctx->daScal,v_localVec,&v_array);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArrayDOF(ctx->daVect,ctx->coordinates,&coords_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daScal,&v_localVec);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_IrrevApplyEQ"
/*
 VF_IrrevApplyEQ: Apply irreversibility conditions using truncation and equality constraints
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_IrrevApplyEQ(Mat K,Vec RHS,Vec V,Vec VIrrev,VFProp *vfprop,VFCtx *ctx)
{
  PetscErrorCode ierr;
  PetscInt       myirrevnum = 0;
  PetscInt       xs,xm,nx;
  PetscInt       ys,ym,ny;
  PetscInt       zs,zm,nz;
  PetscInt       i,j,k,l = 0;
  PetscReal      ***V_array,***VIrrev_array,***residual_array;
  PetscReal      one = 1.;
  MatStencil     *row;
  PetscFunctionBegin;
  
  ierr = DMDAGetInfo(ctx->daScal,NULL,&nx,&ny,&nz,NULL,NULL,NULL,
                     NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
  ierr = DMDAGetCorners(ctx->daScal,&xs,&ys,&zs,&xm,&ym,&zm);CHKERRQ(ierr);
  
  ierr = DMDAVecGetArray(ctx->daScal,V,&V_array);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,VIrrev,&VIrrev_array);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,RHS,&residual_array);CHKERRQ(ierr);
  
  /*
   first pass: RHS, V, and count
   */
  for (k = zs; k < zs + zm; k++)
    for (j = ys; j < ys + ym; j++) {
      for (i = xs; i < xs + xm; i++)
        if (VIrrev_array[k][j][i] <= vfprop->irrevtol) {
          myirrevnum++;
          residual_array[k][j][i] = 0.;
          V_array[k][j][i]   = 0.;
        }
    }
  ierr = PetscMalloc(myirrevnum * sizeof(MatStencil),&row);CHKERRQ(ierr);
  /*
   second pass: Matrix
   */
  for (k = zs; k < zs + zm; k++)
    for (j = ys; j < ys + ym; j++) {
      for (i = xs; i < xs + xm; i++)
        if (VIrrev_array[k][j][i] <= vfprop->irrevtol) {
          row[l].i = i; row[l].j = j; row[l].k = k; row[l].c = 0;
          l++;
        }
    }
  ierr = MatZeroRowsStencil(K,myirrevnum,row,one,NULL,NULL);CHKERRQ(ierr);
  ierr = PetscFree(row);CHKERRQ(ierr);
  
  ierr = DMDAVecRestoreArray(ctx->daScal,V,&V_array);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(ctx->daScal,VIrrev,&VIrrev_array);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(ctx->daScal,RHS,&residual_array);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_IrrevApplyEQVec"

/*
 VF_IrrevApplyEQVec: Apply irreversibility conditions using truncation and equality constraints
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_IrrevApplyEQVec(Vec RHS,Vec VIrrev,VFProp *vfprop,VFCtx *ctx)
{
  PetscErrorCode ierr;
  PetscInt       xs,xm,nx;
  PetscInt       ys,ym,ny;
  PetscInt       zs,zm,nz;
  PetscInt       i,j,k;
  PetscReal      ***VIrrev_array,***residual_array;
  PetscFunctionBegin;
  
  ierr = DMDAGetInfo(ctx->daScal,NULL,&nx,&ny,&nz,NULL,NULL,NULL,
                     NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
  ierr = DMDAGetCorners(ctx->daScal,&xs,&ys,&zs,&xm,&ym,&zm);CHKERRQ(ierr);
  
  ierr = DMDAVecGetArray(ctx->daScal,VIrrev,&VIrrev_array);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,RHS,&residual_array);CHKERRQ(ierr);
  
  /*
   first pass: RHS, V, and count
   */
  for (k = zs; k < zs + zm; k++)
    for (j = ys; j < ys + ym; j++) {
      for (i = xs; i < xs + xm; i++)
        if (VIrrev_array[k][j][i] <= vfprop->irrevtol) {
          residual_array[k][j][i] = 0.;
        }
    }
  
  ierr = DMDAVecRestoreArray(ctx->daScal,VIrrev,&VIrrev_array);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(ctx->daScal,RHS,&residual_array);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_IrrevApplyEQMat"
/*
 VF_IrrevApplyEQMat: Apply irreversibility conditions using truncation and equality constraints
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_IrrevApplyEQMat(Mat K,Vec VIrrev,VFProp *vfprop,VFCtx *ctx)
{
  PetscErrorCode ierr;
  PetscInt       myirrevnum = 0;
  PetscInt       xs,xm,nx;
  PetscInt       ys,ym,ny;
  PetscInt       zs,zm,nz;
  PetscInt       i,j,k,l = 0;
  PetscReal      ***VIrrev_array;
  MatStencil     *row;
  PetscFunctionBegin;
  
  ierr = DMDAGetInfo(ctx->daScal,NULL,&nx,&ny,&nz,NULL,NULL,NULL,
                     NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
  ierr = DMDAGetCorners(ctx->daScal,&xs,&ys,&zs,&xm,&ym,&zm);CHKERRQ(ierr);
  
  ierr = DMDAVecGetArray(ctx->daScal,VIrrev,&VIrrev_array);CHKERRQ(ierr);
  
  /*
   first pass: count number of BC
   */
  for (k = zs; k < zs + zm; k++)
    for (j = ys; j < ys + ym; j++) {
      for (i = xs; i < xs + xm; i++)
        if (VIrrev_array[k][j][i] <= vfprop->irrevtol) {
          myirrevnum++;
        }
    }
  ierr = PetscMalloc(myirrevnum * sizeof(MatStencil),&row);CHKERRQ(ierr);
  /*
   second pass: Apply BC
   */
  for (k = zs; k < zs + zm; k++)
    for (j = ys; j < ys + ym; j++) {
      for (i = xs; i < xs + xm; i++)
        if (VIrrev_array[k][j][i] <= vfprop->irrevtol) {
          row[l].i = i; row[l].j = j; row[l].k = k; row[l].c = 0;
          l++;
        }
    }
  ierr = MatZeroRowsColumnsStencil(K,myirrevnum,row,1.,NULL,NULL);CHKERRQ(ierr);
  ierr = PetscFree(row);CHKERRQ(ierr);
  
  ierr = DMDAVecRestoreArray(ctx->daScal,VIrrev,&VIrrev_array);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VF_VResidual"
extern PetscErrorCode VF_VResidual(SNES snes,Vec V,Vec residual,void *user)
{
  PetscErrorCode ierr;
  VFCtx          *ctx=(VFCtx*)user;
  PetscInt       xs,xm,nx;
  PetscInt       ys,ym,ny;
  PetscInt       zs,zm,nz;
  PetscInt       ei,ej,ek,i,j,k,l,m;
  PetscInt       nrow = ctx->e3D.nphix * ctx->e3D.nphiy * ctx->e3D.nphiz;
  Vec            residual_localVec,U_localVec,V_localVec;
  Vec            theta_localVec,thetaRef_localVec;
  Vec            pressure_localVec;
  PetscReal      ***residual_array,****U_array,***V_array;
  PetscReal      ***theta_array,***thetaRef_array;
  PetscReal      ***pressure_array;
  PetscReal      *residual_local;
  PetscReal      *K_local;
  PetscReal      hx,hy,hz;
  PetscReal      ****coords_array;
  
  PetscFunctionBegin;
  ierr = VecSet(residual,0.0);CHKERRQ(ierr);
  /*
   Get global number of vertices along each coordinate axis on the ENTIRE mesh
   */
  ierr = DMDAGetInfo(ctx->daScalCell,NULL,&nx,&ny,&nz,NULL,NULL,NULL,
                     NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
  /*
   Get informations on LOCAL slice (i.e. subdomain)
   xs, ys, ym = 1st index in x,y,z direction
   xm, ym, zm = number of vertices in the x, y, z directions
   */
  ierr = DMDAGetCorners(ctx->daScalCell,&xs,&ys,&zs,&xm,&ym,&zm);CHKERRQ(ierr);
  ierr = VecSet(residual,0.);CHKERRQ(ierr);
  /*
   Get coordinates
   */
  ierr = DMDAVecGetArrayDOF(ctx->daVect,ctx->coordinates,&coords_array);CHKERRQ(ierr);
  
  /*
   get U_array
   */
  ierr = DMGetLocalVector(ctx->daVect,&U_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daVect,ctx->fields->U,INSERT_VALUES,U_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daVect,ctx->fields->U,INSERT_VALUES,U_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArrayDOF(ctx->daVect,U_localVec,&U_array);CHKERRQ(ierr);
  /*
   get V_array
   */
  ierr = DMGetLocalVector(ctx->daScal,&V_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daScal,V,INSERT_VALUES,V_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daScal,V,INSERT_VALUES,V_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,V_localVec,&V_array);CHKERRQ(ierr);
  /*
   get theta_array, thetaRef_array
   */
  ierr = DMGetLocalVector(ctx->daScal,&theta_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daScal,ctx->fields->theta,INSERT_VALUES,theta_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daScal,ctx->fields->theta,INSERT_VALUES,theta_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,theta_localVec,&theta_array);CHKERRQ(ierr);
  
  ierr = DMGetLocalVector(ctx->daScal,&thetaRef_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daScal,ctx->fields->thetaRef,INSERT_VALUES,thetaRef_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daScal,ctx->fields->thetaRef,INSERT_VALUES,thetaRef_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,thetaRef_localVec,&thetaRef_array);CHKERRQ(ierr);
  /*
   get pressure_array
   */
  ierr = DMGetLocalVector(ctx->daScal,&pressure_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daScal,ctx->fields->pressure,INSERT_VALUES,pressure_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daScal,ctx->fields->pressure,INSERT_VALUES,pressure_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,pressure_localVec,&pressure_array);CHKERRQ(ierr);
  
  /*
   get local mat and residual
   */
  ierr = PetscMalloc2(nrow,&residual_local,
                      nrow * nrow,&K_local);CHKERRQ(ierr);
  ierr = DMGetLocalVector(ctx->daScal,&residual_localVec);CHKERRQ(ierr);
  ierr = VecSet(residual_localVec,0.);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,residual_localVec,&residual_array);CHKERRQ(ierr);
  
  /*
   loop through all elements (ei,ej)
   */
  for (ek = zs; ek < zs+zm; ek++) {
    for (ej = ys; ej < ys+ym; ej++)
      for (ei = xs; ei < xs+xm; ei++) {
        hx   = coords_array[ek][ej][ei+1][0]-coords_array[ek][ej][ei][0];
        hy   = coords_array[ek][ej+1][ei][1]-coords_array[ek][ej][ei][1];
        hz   = coords_array[ek+1][ej][ei][2]-coords_array[ek][ej][ei][2];
        ierr = VFCartFEElement3DInit(&ctx->e3D,hx,hy,hz);CHKERRQ(ierr);
        /*
         Accumulate stiffness matrix
         */
        ierr = PetscMemzero(K_local,nrow * nrow * sizeof(PetscReal));
        switch (ctx->vfprop.atnum ) {
          case 1:
            ierr = VF_BilinearFormVAT13D_local(K_local,&ctx->matprop[ctx->layer[ek]],&ctx->vfprop,&ctx->e3D);CHKERRQ(ierr);
            break;
          case 2:
            ierr = VF_BilinearFormVAT23D_local(K_local,&ctx->matprop[ctx->layer[ek]],&ctx->vfprop,&ctx->e3D);CHKERRQ(ierr);
            break;
        }
        switch (ctx->unilateral) {
          case UNILATERAL_NONE:
            ierr = VF_BilinearFormVCoupling3D_local(K_local,U_array,theta_array,thetaRef_array,
                                           &ctx->matprop[ctx->layer[ek]],&ctx->vfprop,ek,ej,ei,
                                           &ctx->e3D);CHKERRQ(ierr);
            ierr = VF_ResidualVThermoPoro3D_local(residual_local,U_array,theta_array,thetaRef_array,pressure_array,&ctx->matprop[ctx->layer[ek]],&ctx->vfprop,ek,ej,ei,&ctx->e3D);
            break;
          case UNILATERAL_NOCOMPRESSION:
            ierr = VF_BilinearFormVCouplingNoCompression3D_local(K_local,U_array,theta_array,thetaRef_array,
                                                    &ctx->matprop[ctx->layer[ek]],&ctx->vfprop,ek,ej,ei,
                                                    &ctx->e3D);CHKERRQ(ierr);
            ierr = VF_ResidualVThermoPoroNoCompression3D_local(residual_local,U_array,theta_array,thetaRef_array,pressure_array,&ctx->matprop[ctx->layer[ek]],&ctx->vfprop,ek,ej,ei,&ctx->e3D);
            break;
        }
        
        /*
         Accumulate local contributions to residual
         */
        for (m = 0; m < nrow; m++) {
          residual_local[m] = 0.;
          for (l= 0,k = 0; k < ctx->e3D.nphiz; k++) {
            for (j = 0; j < ctx->e3D.nphiy; j++) {
              for (i = 0; i < ctx->e3D.nphix; i++,l++) {
                residual_local[m] -= K_local[m*nrow+l] * V_array[ek+k][ej+j][ei+i];
              }
            }
          }
        }
        /*
          Now need to assemble -beta p div u v~ + 3 alpha beta kappa theta p v~
        */
        
        switch (ctx->vfprop.atnum) {
          case 1:
            ierr = VF_ResidualVAT13D_local(residual_local,&ctx->matprop[ctx->layer[ek]],&ctx->vfprop,&ctx->e3D);CHKERRQ(ierr);
            break;
          case 2:
            ierr = VF_ResidualVAT23D_local(residual_local,&ctx->matprop[ctx->layer[ek]],&ctx->vfprop,&ctx->e3D);CHKERRQ(ierr);
            break;
        }
        if (ctx->hasCrackPressure) {
          ierr = VF_ResidualVCrackPressure3D_local(residual_local,U_array,pressure_array,
                                         &ctx->matprop[ctx->layer[ek]],&ctx->vfprop,ek,ej,ei,
                                         &ctx->e3D);CHKERRQ(ierr);
        }
        
        for (l = 0,k = 0; k < ctx->e3D.nphiz; k++) {
          for (j = 0; j < ctx->e3D.nphiy; j++) {
            for (i = 0; i < ctx->e3D.nphix; i++,l++) {
              residual_array[ek+k][ej+j][ei+i] -= residual_local[l];
            }
          }
        }
        /*
         Jump to next element
         */
      }
  }
  ierr = DMDAVecRestoreArray(ctx->daScal,residual_localVec,&residual_array);CHKERRQ(ierr);
  ierr = DMLocalToGlobalBegin(ctx->daScal,residual_localVec,ADD_VALUES,residual);CHKERRQ(ierr);
  ierr = DMLocalToGlobalEnd(ctx->daScal,residual_localVec,ADD_VALUES,residual);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daScal,&residual_localVec);CHKERRQ(ierr);
  /*
   Cleanup
   */
  ierr = DMDAVecRestoreArrayDOF(ctx->daVect,ctx->coordinates,&coords_array);CHKERRQ(ierr);
  
  ierr = DMDAVecRestoreArrayDOF(ctx->daVect,U_localVec,&U_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daVect,&U_localVec);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(ctx->daScal,V_localVec,&V_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daScal,&V_localVec);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(ctx->daScal,theta_localVec,&theta_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daScal,&theta_localVec);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(ctx->daScal,thetaRef_localVec,&thetaRef_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daScal,&thetaRef_localVec);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(ctx->daScal,pressure_localVec,&pressure_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daScal,&pressure_localVec);CHKERRQ(ierr);  
  ierr = PetscFree2(residual_local,K_local);CHKERRQ(ierr);
  
  if (ctx->vfprop.atnum == 2)
    ierr = VF_IrrevApplyEQVec(residual,ctx->fields->VIrrev,&(ctx->vfprop),ctx);CHKERRQ(ierr);
  /*
   UGLY HACK:
   We can pass V for the BC, because we know that the only BC used in the V problem are 0 and 1, so
   the 3rd argument is never used
   */
  ierr = ResidualApplyDirichletBC(residual,V,V,ctx->bcV);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_VIJacobian"
extern PetscErrorCode VF_VIJacobian(SNES snes,Vec V,Mat Jac,Mat Jacpre,void *user)
{
  PetscErrorCode ierr;
  VFCtx          *ctx=(VFCtx*)user;
  PetscInt       xs,xm,nx;
  PetscInt       ys,ym,ny;
  PetscInt       zs,zm,nz;
  PetscInt       ei,ej,ek,i,j,k,l;
  PetscInt       nrow = ctx->e3D.nphix * ctx->e3D.nphiy * ctx->e3D.nphiz;
  Vec            U_localVec;
  Vec            theta_localVec,thetaRef_localVec;
  PetscReal      ****U_array;
  PetscReal      ***theta_array,***thetaRef_array;
  PetscReal      *Jac_local;
  MatStencil     *row;
  PetscReal      hx,hy,hz;
  PetscReal      ****coords_array;
  
  PetscFunctionBegin;
  /*
   Get global number of vertices along each coordinate axis on the ENTIRE mesh
   */
  ierr = DMDAGetInfo(ctx->daScalCell,NULL,&nx,&ny,&nz,NULL,NULL,NULL,
                     NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
  /*
   Get informations on LOCAL slice (i.e. subdomain)
   xs, ys, ym = 1st index in x,y,z direction
   xm, ym, zm = number of vertices in the x, y, z directions
   */
  ierr = DMDAGetCorners(ctx->daScalCell,&xs,&ys,&zs,&xm,&ym,&zm);CHKERRQ(ierr);
  
  ierr = MatZeroEntries(Jacpre);CHKERRQ(ierr);
  /*
   Get coordinates
   */
  ierr = DMDAVecGetArrayDOF(ctx->daVect,ctx->coordinates,&coords_array);CHKERRQ(ierr);
  
  /*
   get U_array
   */
  ierr = DMGetLocalVector(ctx->daVect,&U_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daVect,ctx->fields->U,INSERT_VALUES,U_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daVect,ctx->fields->U,INSERT_VALUES,U_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArrayDOF(ctx->daVect,U_localVec,&U_array);CHKERRQ(ierr);
  /*
   get theta_array, thetaRef_array
   */
  ierr = DMGetLocalVector(ctx->daScal,&theta_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daScal,ctx->fields->theta,INSERT_VALUES,theta_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daScal,ctx->fields->theta,INSERT_VALUES,theta_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,theta_localVec,&theta_array);CHKERRQ(ierr);
  
  ierr = DMGetLocalVector(ctx->daScal,&thetaRef_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(ctx->daScal,ctx->fields->thetaRef,INSERT_VALUES,thetaRef_localVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(ctx->daScal,ctx->fields->thetaRef,INSERT_VALUES,thetaRef_localVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(ctx->daScal,thetaRef_localVec,&thetaRef_array);CHKERRQ(ierr);
  /*
   get local mat and RHS
   */
  ierr = PetscMalloc2(nrow * nrow,&Jac_local,
                      nrow,&row);CHKERRQ(ierr);
  
  /*
   loop through all elements (ei,ej)
   */
  for (ek = zs; ek < zs+zm; ek++) {
    for (ej = ys; ej < ys+ym; ej++) {
      for (ei = xs; ei < xs+xm; ei++) {
        hx   = coords_array[ek][ej][ei+1][0]-coords_array[ek][ej][ei][0];
        hy   = coords_array[ek][ej+1][ei][1]-coords_array[ek][ej][ei][1];
        hz   = coords_array[ek+1][ej][ei][2]-coords_array[ek][ej][ei][2];
        ierr = VFCartFEElement3DInit(&ctx->e3D,hx,hy,hz);CHKERRQ(ierr);
        /*
         Accumulate stiffness matrix
         */
        ierr = PetscMemzero(Jac_local,nrow * nrow * sizeof(PetscReal));
        switch (ctx->vfprop.atnum ) {
          case 1:
            ierr = VF_BilinearFormVAT13D_local(Jac_local,&ctx->matprop[ctx->layer[ek]],&ctx->vfprop,&ctx->e3D);CHKERRQ(ierr);
            break;
          case 2:
            ierr = VF_BilinearFormVAT23D_local(Jac_local,&ctx->matprop[ctx->layer[ek]],&ctx->vfprop,&ctx->e3D);CHKERRQ(ierr);
            break;
        }
        switch (ctx->unilateral) {
          case UNILATERAL_NONE:
            ierr = VF_BilinearFormVCoupling3D_local(Jac_local,U_array,theta_array,thetaRef_array,
                                           &ctx->matprop[ctx->layer[ek]],&ctx->vfprop,ek,ej,ei,
                                           &ctx->e3D);CHKERRQ(ierr);
            break;
          case UNILATERAL_NOCOMPRESSION:
            ierr = VF_BilinearFormVCouplingNoCompression3D_local(Jac_local,U_array,theta_array,thetaRef_array,
                                                    &ctx->matprop[ctx->layer[ek]],&ctx->vfprop,ek,ej,ei,
                                                    &ctx->e3D);CHKERRQ(ierr);
            break;
        }
        /*
         Generate array of grid indices in the linear system's ordering.
         i.e. tells MatSetValuesStencil where to store values from  K_local
         if the element is indexed by (ek, ej, ei), the associated degrees of freedom
         have indices (ek ... ek + nphiz), (ej .. ej + nphiy), (ei ... ei + nphix)
         */
        
        for (l = 0,k = 0; k < ctx->e3D.nphiz; k++) {
          for (j = 0; j < ctx->e3D.nphiy; j++) {
            for (i = 0; i < ctx->e3D.nphix; i++,l++) {
              row[l].i = ei + i; row[l].j = ej + j; row[l].k = ek + k; row[l].c = 0;
            }
          }
        }
        
        /*
         Add local stiffness matrix to global stiffness natrix
         */
        ierr = MatSetValuesStencil(Jacpre,nrow,row,nrow,row,Jac_local,ADD_VALUES);CHKERRQ(ierr);
        
        /*
         Jump to next element
         */
      }
    }
  }
  ierr = MatAssemblyBegin(Jacpre,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(Jacpre,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  
  /*
   Cleanup
   */
  ierr = DMDAVecRestoreArrayDOF(ctx->daVect,ctx->coordinates,&coords_array);CHKERRQ(ierr);
  
  ierr = DMDAVecRestoreArrayDOF(ctx->daVect,U_localVec,&U_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daVect,&U_localVec);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(ctx->daScal,theta_localVec,&theta_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daScal,&theta_localVec);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(ctx->daScal,thetaRef_localVec,&thetaRef_array);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(ctx->daScal,&thetaRef_localVec);CHKERRQ(ierr);
  
  ierr = PetscFree2(Jac_local,row);CHKERRQ(ierr);
  
  ierr = MatApplyDirichletBCRowCol(Jacpre,ctx->bcV);CHKERRQ(ierr);
  if (ctx->vfprop.atnum == 2)
    ierr = VF_IrrevApplyEQMat(Jacpre,ctx->fields->VIrrev,&(ctx->vfprop),ctx);CHKERRQ(ierr);
  if (Jac != Jacpre) {
    ierr = MatCopy(Jacpre,Jac,DIFFERENT_NONZERO_PATTERN);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_VSNESMonitor"
extern PetscErrorCode VF_VSNESMonitor(SNES snes,PetscInt its,PetscReal fnorm,void * ptr)
{
  PetscErrorCode ierr;
  PetscReal      norm,vmax,vmin;
  MPI_Comm       comm;
  Vec            V;
  
  PetscFunctionBegin;
  ierr = SNESGetSolution(snes,&V);CHKERRQ(ierr);
  ierr = VecNorm(V,NORM_1,&norm);CHKERRQ(ierr);
  ierr = VecMax(V,NULL,&vmax);CHKERRQ(ierr);
  ierr = VecMin(V,NULL,&vmin);CHKERRQ(ierr);
  ierr = PetscObjectGetComm((PetscObject)snes,&comm);CHKERRQ(ierr);
  ierr = PetscPrintf(comm,"V_snes_iter_step %d :solution norm = %g, max sol. value  = %g, min sol. value = %g\n",its,norm,vmax,vmin);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VF_StepV"
/*
 VF_StepV
 
 (c) 2010-2012 Blaise Bourdin bourdin@lsu.edu
 */
extern PetscErrorCode VF_StepV(VFFields *fields,VFCtx *ctx)
{
  PetscErrorCode      ierr;
  SNESConvergedReason reason;
  PetscInt            its,flg= 0;
  PetscReal           Vmin,Vmax;
  
  PetscFunctionBegin;
  ierr = SNESSolve(ctx->snesV,NULL,fields->V);CHKERRQ(ierr);
  if (ctx->verbose > 1) {
    ierr = VecView(fields->V,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  }
  ierr = SNESGetConvergedReason(ctx->snesV,&reason);CHKERRQ(ierr);
  if (reason < 0) {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"[ERROR] snesV diverged with reason %d\n",(int)reason);CHKERRQ(ierr);
    flg = reason;
  } else {
    ierr = SNESGetIterationNumber(ctx->snesV,&its);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"      snesV converged in %d iterations %d.\n",(int)its,(int)reason);CHKERRQ(ierr);
  }
  /* Get Min / Max of V */
  ierr = VecMin(fields->V,NULL,&Vmin);CHKERRQ(ierr);
  ierr = VecMax(fields->V,NULL,&Vmax);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"      V min / max:     %e %e\n",Vmin,Vmax);CHKERRQ(ierr);
  PetscFunctionReturn(flg);
}











