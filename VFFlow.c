/*
   VFFlow.c
   Generic interface to flow solvers

     (c) 2010-2011 Blaise Bourdin, LSU. bourdin@lsu.edu
                    Keita Yoshioka, Chevron ETC
*/
#include "petsc.h"
#include "CartFE.h"
#include "VFCommon.h"
#include "VFFlow_FEM.h"
#include "VFFlow_Fake.h"
#include "VFFlow_MixedFEM.h"


#undef __FUNCT__
#define __FUNCT__ "FlowSolverFinalize"
/* 
   FlowSolverFinalize
   Keita Yoshioka yoshk@chevron.com
*/
extern PetscErrorCode FlowSolverFinalize(VFCtx *ctx,VFFields *fields)
{
	PetscErrorCode ierr;
	
	PetscFunctionBegin;
	switch (ctx->flowsolver) {
		case FLOWSOLVER_DARCYMIXEDFEMSTEADYSTATE:       
			ierr = MixedFEMFlowSolverFinalize(ctx,fields);CHKERRQ(ierr);
			break;
		case FLOWSOLVER_FEM:
			break; 
		case FLOWSOLVER_FAKE:
			break; 
		case FLOWSOLVER_READFROMFILES:
			break;
	}
	PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "FlowSolverInitialize"
extern PetscErrorCode FlowSolverInitialize(VFCtx *ctx,VFFields *fields)
{
	PetscErrorCode ierr;
	
	PetscFunctionBegin;
	switch (ctx->flowsolver) {
		case FLOWSOLVER_DARCYMIXEDFEMSTEADYSTATE:       
			ierr = MixedFEMFlowSolverInitialize(ctx);CHKERRQ(ierr);
			break;
		case FLOWSOLVER_FEM:
			break; 
		case FLOWSOLVER_FAKE:
			break; 
		case FLOWSOLVER_READFROMFILES:
			break;
	}
	PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BCPInit"
extern PetscErrorCode BCPInit(BC *BCP,VFCtx *ctx)
{
  PetscErrorCode ierr;
 
  PetscFunctionBegin;
  ierr = BCInit(BCP,1);CHKERRQ(ierr);

/*
  When positive value is given, boundary condiction has a numerical value
*/
  if(ctx->BCpres[0] > -1.e-8) BCP[0].face[X0] = VALUE;
  if(ctx->BCpres[1] > -1.e-8) BCP[0].face[X1] = VALUE;
  if(ctx->BCpres[2] > -1.e-8) BCP[0].face[Y0] = VALUE;
  if(ctx->BCpres[3] > -1.e-8) BCP[0].face[Y1] = VALUE;
  if(ctx->BCpres[4] > -1.e-8) BCP[0].face[Z0] = VALUE;
  if(ctx->BCpres[5] > -1.e-8) BCP[0].face[Z1] = VALUE;

  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BCTInit"
/*
  BCTInit
  Keita Yoshioka yoshk@chevron.com
*/
extern PetscErrorCode BCTInit(BC *BCT,VFCtx *ctx)
{
  PetscErrorCode ierr;
  
  PetscFunctionBegin;
  ierr = BCInit(BCT,1);CHKERRQ(ierr);
  
/*
  When positive value is given, boundary condiction has a numecial value
*/
  if(ctx->BCtheta[0] > -1.e-8) BCT[0].face[X0] = VALUE;
  if(ctx->BCtheta[1] > -1.e-8) BCT[0].face[X1] = VALUE;
  if(ctx->BCtheta[2] > -1.e-8) BCT[0].face[Y0] = VALUE;
  if(ctx->BCtheta[3] > -1.e-8) BCT[0].face[Y1] = VALUE;
  if(ctx->BCtheta[4] > -1.e-8) BCT[0].face[Z0] = VALUE;
  if(ctx->BCtheta[5] > -1.e-8) BCT[0].face[Z1] = VALUE;

  PetscFunctionReturn(0);
}
  
  

/*
  VFFlowTimeStep: Does one time step of the flow solver selected in ctx.flowsolver
*/
#undef __FUNCT__
#define __FUNCT__ "VFFlowTimeStep"
extern PetscErrorCode VFFlowTimeStep(VFCtx *ctx,VFFields *fields)
{
  char           filename[FILENAME_MAX];
  PetscViewer    viewer;
  PetscErrorCode     ierr;

  PetscFunctionBegin;
  switch (ctx->flowsolver) {
    case FLOWSOLVER_FEM:       
      ierr = VFFlow_FEM(ctx,fields);CHKERRQ(ierr);
      break;
	case FLOWSOLVER_SNES:
//	  ierr = VFFlow_FEM(ctx,fields);CHKERRQ(ierr);
	  ierr = VFFlow_SNES_FEM(ctx,fields);CHKERRQ(ierr);
      break;
    case FLOWSOLVER_DARCYMIXEDFEMSTEADYSTATE:
      ierr = VFFlow_DarcyMixedFEMSteadyState(ctx,fields);CHKERRQ(ierr);
      break;
    case FLOWSOLVER_FAKE:
      ierr = VFFlow_Fake(ctx,fields);CHKERRQ(ierr);
      break; 
    case FLOWSOLVER_READFROMFILES:
      ierr = PetscLogStagePush(ctx->vflog.VF_IOStage);CHKERRQ(ierr);
      switch (ctx->fileformat) {
        case FILEFORMAT_HDF5:
#ifdef PETSC_HAVE_HDF5
          ierr = PetscViewerHDF5Open(PETSC_COMM_WORLD,filename,FILE_MODE_READ,&viewer);CHKERRQ(ierr);
          /*
            ierr = VecLoad(viewer,fields->theta);CHKERRQ(ierr);
          */
          ierr = VecLoad(fields->pressure,viewer);CHKERRQ(ierr);
          ierr = PetscViewerDestroy(&viewer);CHKERRQ(ierr);    
#endif
          break;
        case FILEFORMAT_BIN:
          SETERRQ(PETSC_COMM_WORLD,PETSC_ERR_SUP,"Reading from binary files not implemented yet");
          break;
      }
      ierr = PetscLogStagePop();CHKERRQ(ierr);
	// eventually replace FLOWSOLVER_FEM with this after confirming it provides the same results

  }
  PetscFunctionReturn(0);
}



  