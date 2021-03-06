/*
   VFFlow_DarcyPoisson.h
   A direct solver for the Darcy equation 
    
   (c) 2011 K. Yoshioka, CHEVRON ETC
*/

#ifndef VFFLOW_FEM_H
#define VFFLOW_FEM_H


extern PetscErrorCode VFFlow_FEM(VFCtx *ctx, VFFields *fields);
extern PetscErrorCode VFFlow_SNES_FEM(VFCtx *ctx, VFFields *fields);
extern PetscErrorCode VFFlow_TS_FEM(VFCtx *ctx, VFFields *fields);

#endif /* VFFLOW_FEM_H */
