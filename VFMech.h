/*
   VFMech.h
   (c) 2013 B. Bourdin, LSU
*/
#if !defined(VF_Standalone_VFMech_h)
#define VF_Standalone_VFMech_h

/*
extern PetscErrorCode ElasticEnergyDensity3D_local(PetscReal *ElasticEnergyDensity_local,
                                                   PetscReal ****u_array,
                                                   PetscReal ***theta_array,PetscReal ***thetaRef_array,
                                                   VFMatProp *matprop,PetscInt ek,PetscInt ej,PetscInt ei,
                                                   VFCartFEElement3D *e);
extern PetscErrorCode ElasticEnergyDensitySphericalDeviatoric3D_local(PetscReal *ElasticEnergyDensityS_local,
                                                                      PetscReal *ElasticEnergyDensityD_local,
                                                                      PetscReal ****u_array,
                                                                      PetscReal ***theta_array,PetscReal ***thetaRef_array,
                                                                      VFMatProp *matprop,PetscInt ek,PetscInt ej,PetscInt ei,
                                                                      VFCartFEElement3D *e);
*/
extern PetscErrorCode VF_UEnergy3D(PetscReal *ElasticEnergy,PetscReal *OverbdnWork,PetscReal *PressureWork,Vec U,VFCtx *ctx);

extern PetscErrorCode VF_StepU(VFFields *fields,VFCtx *ctx);
extern PetscErrorCode VF_VEnergy3D(PetscReal *SurfaceEnergy,VFFields *fields,VFCtx *ctx);
extern PetscErrorCode VF_StepV(VFFields *fields,VFCtx *ctx);
/*
 These functions are not meant to be called outside of VFMech,
 but since snesU and snesV are initialized outside of VFMech, I have no other choice
 */
extern PetscErrorCode VF_VIJacobian(SNES snes,Vec V,Mat Jac,Mat Jac1,void *user);
extern PetscErrorCode VF_VResidual(SNES snes,Vec V,Vec Func,void *user);
extern PetscErrorCode VF_VSNESMonitor(SNES snes,PetscInt its,PetscReal fnorm,void* ptr);

extern PetscErrorCode VF_UIJacobian(SNES snes,Vec U,Mat Jac,Mat Jac1,void *user);
extern PetscErrorCode VF_UResidual(SNES snes,Vec U,Vec Func,void *user);

#endif
